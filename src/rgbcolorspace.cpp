﻿// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2020 Lukas Sommer sommerluk@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "PerceptualColor/perceptualcolorglobal.h"
#include "perceptualcolorinternal.h"

// Own headers
// First the interface, which forces the header to be self-contained.
#include "rgbcolorspace.h"
// Second, the private implementation.
#include "rgbcolorspace_p.h"

#include "helper.h"
#include "iohandlerfactory.h"
#include "polarpointf.h"

#include <QDebug>

// TODO There should be no dependency on Posix headers, but only on standard C++.
#include <unistd.h> // Posix header

namespace PerceptualColor
{
/** @internal
 *
 * @brief Constructor
 *
 * Creates an ininitialized, invalid (broken) color space. You have to
 * call @ref RgbColorSpacePrivate::initialize() before using this object. */
RgbColorSpace::RgbColorSpace(QObject *parent)
    : QObject(parent)
    , d_pointer(new RgbColorSpacePrivate(this))
{
}

/** @brief Create an sRGB color space object.
 *
 * @returns A shared pointer to a newly created color space object. */
QSharedPointer<PerceptualColor::RgbColorSpace> RgbColorSpace::createSrgb()
{
    // Create an invalid object:
    QSharedPointer<PerceptualColor::RgbColorSpace> result {new RgbColorSpace()};

    // Transform it into a valid object:
    cmsHPROFILE srgb = cmsCreate_sRGBProfile(); // Use build-in profile
    result->d_pointer->initialize(srgb);
    cmsCloseProfile(srgb);

    // Fine-tuning (and localization) of profile information for this
    // build-in profile:
    result->d_pointer->m_cmsInfoDescription = tr("sRGB color space");
    // Leaving m_cmsInfoCopyright without change.
    result->d_pointer->m_cmsInfoManufacturer = tr("LittleCMS");
    result->d_pointer->m_cmsInfoModel = QString();

    // Return:
    return result;
}

/** @brief Create a color space object for a given ICC file.
 *
 * This function may fail to create the color space object when it cannot
 * open the given file, or when the file cannot be interpreted by LittleCMS.
 *
 * @param fileName The file name. TODO Must have a form that is compliant with TODO Update also doc on RGbcolospacefactory.
 * <tt>QFile</tt>.
 *
 * @returns A shared pointer to a newly created color space object on success.
 * A shared pointer to <tt>nullptr</tt> otherwise. */
QSharedPointer<PerceptualColor::RgbColorSpace> RgbColorSpace::createFromFile(const QString &fileName)
{
    cmsIOHANDLER *myIOHandler = IOHandlerFactory::createReadOnly(nullptr, fileName);
    if (myIOHandler == nullptr) {
        return nullptr;
    }

    cmsHPROFILE myProfileHandle = cmsOpenProfileFromIOhandlerTHR( //
        nullptr,                                                  // ContextID
        myIOHandler                                               // IO handler
    );
    if (myProfileHandle == nullptr) {
        // We do not have to delete myIOHandler manually.
        // (cmsOpenProfileFromIOhandlerTHR did that when
        // failing to open the profile handle.)
        return nullptr;
    }

    // Create an invalid object:
    QSharedPointer<PerceptualColor::RgbColorSpace> newObject {new RgbColorSpace()};
    // Try to transform it into a valid object:
    const bool success = newObject->d_pointer->initialize(myProfileHandle);
    // Clean up
    cmsCloseProfile(myProfileHandle);
    // We do not have to delete myIOHandler manually.
    // (myCmsProfileHandle did  that when cmsCloseProfile() was invoked.)

    // Return
    if (success) {
        return newObject;
    }
    return nullptr;
}

/** @brief Basic initialization.
 *
 * Code that is shared between the various overloaded constructors.
 *
 * @param rgbProfileHandle Handle for the RGB profile
 *
 * @pre rgbProfileHandle is valid.
 *
 * @returns <tt>true</tt> on success. <tt>false</tt> otherwise (for example
 * when it’s not an RGB profile but an CMYK profile). When <tt>false</tt>
 * is returned, the object is still in an undefined state; it cannot
 * be used, but only be destoyed. */
bool RgbColorSpace::RgbColorSpacePrivate::initialize(cmsHPROFILE rgbProfileHandle)
{
    m_cmsInfoDescription = getInformationFromProfile(rgbProfileHandle, cmsInfoDescription);
    m_cmsInfoCopyright = getInformationFromProfile(rgbProfileHandle, cmsInfoCopyright);
    m_cmsInfoManufacturer = getInformationFromProfile(rgbProfileHandle, cmsInfoManufacturer);
    m_cmsInfoModel = getInformationFromProfile(rgbProfileHandle, cmsInfoModel);

    // Create an ICC v4 profile object for the Lab color space.
    cmsHPROFILE labProfileHandle = cmsCreateLab4Profile(
        // nullptr means: Default white point (D50)
        // TODO Does this make sense? sRGB white point is D65!
        nullptr);

    // Create the transforms
    // We use the flag cmsFLAGS_NOCACHE which disables the 1-pixel-cache
    // which is normally used in the transforms. We do this because transforms
    // that use the 1-pixel-cache are not thread-save. And disabling it
    // should not have negative impacts as we usually work with gradients,
    // so anyway it is not likely to have two consecutive pixels with
    // the same color, which is the only situation where the 1-pixel-cache
    // makes processing faster.
    m_transformLabToRgbHandle = cmsCreateTransform(
        // Create a transform function and get a handle to this function:
        labProfileHandle,             // input profile handle
        TYPE_Lab_DBL,                 // input buffer format
        rgbProfileHandle,             // output profile handle
        TYPE_RGB_DBL,                 // output buffer format
        INTENT_ABSOLUTE_COLORIMETRIC, // rendering intent
        cmsFLAGS_NOCACHE              // flags
    );
    m_transformLabToRgb16Handle = cmsCreateTransform(
        // Create a transform function and get a handle to this function:
        labProfileHandle,             // input profile handle
        TYPE_Lab_DBL,                 // input buffer format
        rgbProfileHandle,             // output profile handle
        TYPE_RGB_16,                  // output buffer format
        INTENT_ABSOLUTE_COLORIMETRIC, // rendering intent
        cmsFLAGS_NOCACHE              // flags
    );
    m_transformRgbToLabHandle = cmsCreateTransform(
        // Create a transform function and get a handle to this function:
        rgbProfileHandle,             // input profile handle
        TYPE_RGB_DBL,                 // input buffer format
        labProfileHandle,             // output profile handle
        TYPE_Lab_DBL,                 // output buffer format
        INTENT_ABSOLUTE_COLORIMETRIC, // rendering intent
        cmsFLAGS_NOCACHE              // flags
    );
    // It is mandatory to close the profiles to prevent memory leaks:
    cmsCloseProfile(labProfileHandle);

    // After having closed the profiles, we can now return
    // (if appropriate) without having memory leaks:
    if ((m_transformLabToRgbHandle == nullptr)      //
        || (m_transformLabToRgb16Handle == nullptr) //
        || (m_transformRgbToLabHandle == nullptr)   //
    ) {
        RgbColorSpacePrivate::deleteTransform(m_transformLabToRgb16Handle);
        RgbColorSpacePrivate::deleteTransform(m_transformLabToRgbHandle);
        RgbColorSpacePrivate::deleteTransform(m_transformRgbToLabHandle);
        return false;
    }

    // Maximum chroma:
    // TODO m_maximumChroma should depend on the actual profile.
    // m_maximumChroma = LchValues::humanMaximumChroma;
    // m_maximumChroma = 350;

    // Now we know for sure that lowerChroma is in-gamut and upperChroma is
    // out-of-gamut…
    LchDouble candidate;
    candidate.l = 0;
    candidate.c = 0;
    candidate.h = 0;
    while (!q_pointer->isInGamut(candidate) && (candidate.l < 100)) {
        candidate.l += gamutPrecision;
    }
    m_blackpointL = candidate.l;
    candidate.l = 100;
    while (!q_pointer->isInGamut(candidate) && (candidate.l > 0)) {
        candidate.l -= gamutPrecision;
    }
    m_whitepointL = candidate.l;
    if (m_whitepointL <= m_blackpointL) {
        qCritical() << "Unable to find blackpoint and whitepoint on gray axis.";
        throw 0;
    }

    // Dirty hacks:
    QSharedPointer<PerceptualColor::RgbColorSpace> pointerToMainObject;
    pointerToMainObject.reset(q_pointer);
    // TODO Self-reference is always a bad idea…
    m_nearestNeighborSearchImage = new ChromaLightnessImage(pointerToMainObject);
    const QSize nearestNeighborImageSize = QSize(
        // width:
        qRound(nearestNeighborSearchImageHeight / 100.0 * LchValues::humanMaximumChroma) + 1,
        // height:
        nearestNeighborSearchImageHeight);
    m_nearestNeighborSearchImage->setImageSize(nearestNeighborImageSize);
    m_nearestNeighborSearchImage->setBackgroundColor(Qt::transparent);

    return true;
}

/** @brief Destructor */
RgbColorSpace::~RgbColorSpace() noexcept
{
    RgbColorSpacePrivate::deleteTransform(d_pointer->m_transformLabToRgb16Handle);
    RgbColorSpacePrivate::deleteTransform(d_pointer->m_transformLabToRgbHandle);
    RgbColorSpacePrivate::deleteTransform(d_pointer->m_transformRgbToLabHandle);
}

/** @brief Constructor
 *
 * @param backLink Pointer to the object from which <em>this</em> object
 * is the private implementation. */
RgbColorSpace::RgbColorSpacePrivate::RgbColorSpacePrivate(RgbColorSpace *backLink)
    : q_pointer(backLink)
{
}

/** @brief Conveniance function for deleting LittleCMS transforms
 *
 * <tt>cmsDeleteTransform()</tt> is not comfortable. Calling it on a
 * <tt>nullptr</tt> crashes. If called on a valid handle, it does not
 * reset the handle to <tt>nullptr</tt>. Calling it again on the now
 * invalid handle crashes. This conveniance function can be used instead
 * of <tt>cmsDeleteTransform()</tt>: It provides  some more comfort,
 * by adding support for <tt>nullptr</tt>.
 *
 * @param transformHandle handle to the transform
 *
 * @post If the handle is <tt>nullptr</tt>, nothing happens. Otherwise,
 * <tt>cmsDeleteTransform()</tt> is called and then, the handle is set
 * to <tt>nullptr</tt>.
 *
 * @todo TODO Use pointer instead of reference as argument to make sure
 * that the caller sees that this function changes the argument.
 *
 * @todo TODO Implement a similar function to delete profiles!
 */
void RgbColorSpace::RgbColorSpacePrivate::deleteTransform(cmsHTRANSFORM &transformHandle)
{
    if (transformHandle != nullptr) {
        cmsDeleteTransform(transformHandle);
        transformHandle = nullptr;
    }
}

/** @brief Calculates the Lab value
 *
 * @param rgbColor the color that will be converted. (If this is not an
 * RGB color, it will be converted first into an RGB color by QColor methods.)
 * @returns If the color is valid, the corresponding LCh value might also
 * be invalid.
 *
 * @returns ???
 *
 * @note By definition, each RGB color in a given color space is an in-gamut
 * color in this very same color space. Nevertheless, because of rounding
 * errors, when converting colors that are near to the outer hull of the
 * gamut/color space, than @ref isInGamut() might return <tt>false</tt> for
 * a return value of this function.
 */
cmsCIELab RgbColorSpace::RgbColorSpacePrivate::toLab(const QColor &rgbColor) const
{
    RgbDouble my_rgb;
    my_rgb.red = rgbColor.redF();
    my_rgb.green = rgbColor.greenF();
    my_rgb.blue = rgbColor.blueF();
    return colorLab(my_rgb);
}

/**
 * @returns ???
 *
 * @note By definition, each RGB color in a given color space is an in-gamut
 * color in this very same color space. Nevertheless, because of rounding
 * errors, when converting colors that are near to the outer hull of the
 * gamut/color space, than @ref isInGamut() might return <tt>false</tt> for
 * a return value of this function.
 */
PerceptualColor::LchDouble RgbColorSpace::toLch(const QColor &rgbColor) const
{
    cmsCIELab lab = d_pointer->toLab(rgbColor);
    cmsCIELCh lch;
    cmsLab2LCh(&lch, &lab);
    LchDouble result;
    result.l = lch.L;
    result.c = lch.C;
    result.h = lch.h;
    return result;
}

/** @brief Calculates the Lab value
 *
 * @param rgb the color that will be converted.
 * @returns If the color is valid, the corresponding LCh value might also
 * be invalid. */
cmsCIELab RgbColorSpace::RgbColorSpacePrivate::colorLab(const RgbDouble &rgb) const
{
    cmsCIELab lab;
    cmsDoTransform(m_transformRgbToLabHandle, // handle to transform function
                   &rgb,                      // input
                   &lab,                      // output
                   1                          // convert exactly 1 value
    );
    return lab;
}

/** @brief Calculates the RGB value
 *
 * @param Lab a L*a*b* color
 * @returns If the color is within the RGB gamut, a QColor with the RGB values.
 * An invalid QColor otherwise.
 */
QColor RgbColorSpace::toQColorRgbUnbound(const cmsCIELab &Lab) const
{
    QColor temp; // By default, without initialization this is an invalid color
    RgbDouble rgb;
    cmsDoTransform(
        // Parameters:
        d_pointer->m_transformLabToRgbHandle, // handle to transform function
        &Lab,                                 // input
        &rgb,                                 // output
        1                                     // convert exactly 1 value
    );
    if (isInRange<cmsFloat64Number>(0, rgb.red, 1)      //
        && isInRange<cmsFloat64Number>(0, rgb.green, 1) //
        && isInRange<cmsFloat64Number>(0, rgb.blue, 1)  //
    ) {
        // We are within the gamut
        temp = QColor::fromRgbF(rgb.red, rgb.green, rgb.blue);
    }
    return temp;
}

/** @brief Calculates the RGB value
 *
 * @param lch an LCh color
 * @returns If the color is within the RGB gamut, a QColor with the RGB values.
 * An invalid QColor otherwise.
 */
QColor RgbColorSpace::toQColorRgbUnbound(const LchDouble &lch) const
{
    cmsCIELab lab; // uses cmsFloat64Number internally
    const cmsCIELCh myCmsCieLch = toCmsCieLch(lch);
    // convert from LCh to Lab
    cmsLCh2Lab(&lab, &myCmsCieLch);
    cmsCIELab temp;
    temp.L = lab.L;
    temp.a = lab.a;
    temp.b = lab.b;
    return toQColorRgbUnbound(temp);
}

RgbDouble RgbColorSpace::RgbColorSpacePrivate::colorRgbBoundSimple(const cmsCIELab &Lab) const
{
    cmsUInt16Number rgb_int[3];
    cmsDoTransform(
        // Parameters:
        m_transformLabToRgb16Handle, // handle to transform function
        &Lab,                        // input
        rgb_int,                     // output
        1                            // convert exactly 1 value
    );
    RgbDouble temp;
    temp.red = rgb_int[0] / static_cast<qreal>(65535);
    temp.green = rgb_int[1] / static_cast<qreal>(65535);
    temp.blue = rgb_int[2] / static_cast<qreal>(65535);
    return temp;
}

/** @brief Calculates the RGB value
 *
 * @param Lab a L*a*b* color
 * @returns If the color is within the RGB gamut, a QColor with the RGB values.
 * A nearby (in-gamut) RGB QColor otherwise.
 */
QColor RgbColorSpace::RgbColorSpacePrivate::toQColorRgbBound(const cmsCIELab &Lab) const
{
    RgbDouble temp = colorRgbBoundSimple(Lab);
    return QColor::fromRgbF(temp.red, temp.green, temp.blue);
}

/** @brief Calculates the RGB value
 *
 * @param lch an LCh color
 * @returns If the color is within the RGB gamut, a QColor with the RGB values.
 * A nearby (in-gamut) RGB QColor otherwise.
 */
QColor RgbColorSpace::toQColorRgbBound(const LchDouble &lch) const
{
    cmsCIELab lab; // uses cmsFloat64Number internally
    const cmsCIELCh myCmsCieLch = toCmsCieLch(lch);
    // convert from LCh to Lab
    cmsLCh2Lab(&lab, &myCmsCieLch);
    cmsCIELab temp;
    temp.L = lab.L;
    temp.a = lab.a;
    temp.b = lab.b;
    return d_pointer->toQColorRgbBound(temp);
}

QColor RgbColorSpace::toQColorRgbBound(const PerceptualColor::LchaDouble &lcha) const
{
    LchDouble lch;
    lch.l = lcha.l;
    lch.c = lcha.c;
    lch.h = lcha.h;
    QColor result = toQColorRgbBound(lch);
    result.setAlphaF(lcha.a);
    return result;
}

// TODO What to do with in-gamut tests if LittleCMS has fallen back to
// bounded mode because of too complicate profiles? Out in-gamut detection
// would not work anymore!

/** @brief check if an LCh value is within a specific RGB gamut
 * @param lch the LCh color
 * @returns Returns true if lightness/chroma/hue is in the specified
 * RGB gamut. Returns false otherwise. */
bool RgbColorSpace::isInGamut(const LchDouble &lch) const
{
    // variables
    cmsCIELab lab; // uses cmsFloat64Number internally
    const cmsCIELCh myCmsCieLch = toCmsCieLch(lch);

    // code
    cmsLCh2Lab(&lab,
               &myCmsCieLch); // TODO no normalization necessary previously?
    cmsCIELab temp;
    temp.L = lab.L;
    temp.a = lab.a;
    temp.b = lab.b;
    return isInGamut(temp);
}

/** @brief check if a Lab value is within a specific RGB gamut
 * @param lab the Lab color
 * @returns Returns true if it is in the specified RGB gamut. Returns
 * false otherwise. */
bool RgbColorSpace::isInGamut(const cmsCIELab &lab) const
{
    RgbDouble rgb;

    cmsDoTransform(
        // Parameters:
        d_pointer->m_transformLabToRgbHandle, // handle to transform function
        &lab,                                 // input
        &rgb,                                 // output
        1                                     // convert exactly 1 value
    );

    return (isInRange<cmsFloat64Number>(0, rgb.red, 1) && isInRange<cmsFloat64Number>(0, rgb.green, 1) && isInRange<cmsFloat64Number>(0, rgb.blue, 1));
}

QString RgbColorSpace::profileInfoCopyright() const
{
    return d_pointer->m_cmsInfoCopyright;
}

/** Returns the description of the RGB color space. */
QString RgbColorSpace::profileInfoDescription() const
{
    return d_pointer->m_cmsInfoDescription;
}

QString RgbColorSpace::profileInfoManufacturer() const
{
    return d_pointer->m_cmsInfoManufacturer;
}

/** @brief Convert to LCh
 *
 * @param lab a point in Lab representation
 * @returns the same point in LCh representation */
PerceptualColor::LchDouble RgbColorSpace::toLch(const cmsCIELab &lab) const
{
    cmsCIELCh tempLch;
    cmsLab2LCh(&tempLch, &lab);
    return toLchDouble(tempLch);
}

QString RgbColorSpace::profileInfoModel() const
{
    return d_pointer->m_cmsInfoModel;
}

/** @returns A <em>normalized</em> (this is guaranteed!) in-gamut color,
 * maybe with different chroma (and even lightness??)
 *
 * @todo This function should never change anything than chroma. If it fails,
 * it should throw an exception.
 */
PerceptualColor::LchDouble RgbColorSpace::nearestInGamutColorByAdjustingChroma(const PerceptualColor::LchDouble &color) const
{
    LchDouble result = color;
    PolarPointF temp(result.c, result.h);
    result.c = temp.radial();
    result.h = temp.angleDegree();

    // Test special case: If we are yet in-gamut…
    if (isInGamut(result)) {
        return result;
    }

    // Now we know: We are out-of-gamut…
    LchDouble lowerChroma {result.l, 0, result.h};
    LchDouble upperChroma {result};
    LchDouble candidate;
    if (isInGamut(lowerChroma)) {
        // Now we know for sure that lowerChroma is in-gamut
        // and upperChroma is out-of-gamut…
        candidate = upperChroma;
        while (upperChroma.c - lowerChroma.c > gamutPrecision) {
            // Our test candidate is half the way between lowerChroma
            // and upperChroma:
            candidate.c = ((lowerChroma.c + upperChroma.c) / 2);
            if (isInGamut(candidate)) {
                lowerChroma = candidate;
            } else {
                upperChroma = candidate;
            }
        }
        result = lowerChroma;
    } else {
        if (result.l < d_pointer->m_blackpointL) {
            result.l = d_pointer->m_blackpointL;
            result.c = 0;
        } else {
            if (result.l > d_pointer->m_whitepointL) {
                result.l = d_pointer->m_blackpointL;
                result.c = 0;
            }
        }
    }

    return result;
}

PerceptualColor::LchDouble RgbColorSpace::nearestInGamutColorByAdjustingChromaLightness(const PerceptualColor::LchDouble &color)
{
    // TODO Would be great if this function could be const

    // Initialization
    LchDouble temp = color;
    if (temp.c < 0) {
        temp.c = 0;
    }

    if (isInGamut(temp)) {
        return temp;
    }

    QPoint myPixelPosition( //
        qRound(temp.c * (d_pointer->nearestNeighborSearchImageHeight - 1) / 100.0),
        qRound(d_pointer->nearestNeighborSearchImageHeight - 1 - temp.l * (d_pointer->nearestNeighborSearchImageHeight - 1) / 100.0));
    d_pointer->m_nearestNeighborSearchImage->setHue(temp.h);
    myPixelPosition = d_pointer->nearestNeighborSearch( //
        myPixelPosition,
        d_pointer->m_nearestNeighborSearchImage->getImage());
    LchDouble result = temp;
    result.c = myPixelPosition.x() * 100.0 / (d_pointer->nearestNeighborSearchImageHeight - 1);
    result.l = 100 - myPixelPosition.y() * 100.0 / (d_pointer->nearestNeighborSearchImageHeight - 1);
    return result;
}

/** @brief Search the nearest non-transparent neighbor pixel
 *
 * This implements a
 * <a href="https://en.wikipedia.org/wiki/Nearest_neighbor_search">
 * Nearest-neighbor-search</a>.
 *
 * @note This code is an inefficient implementation. See
 * <a href="https://stackoverflow.com/questions/307445/finding-closest-non-black-pixel-in-an-image-fast">here</a>
 * for a better approach.
 *
 * @param originalPoint The point for which you search the nearest neighbor,
 * expressed in the coordinate system of the image. This point may be within
 * or outside the image.
 * @param image The image in which the nearest neighbor is searched.
 * Must contain at least one pixel with an alpha value that is fully opaque.
 * @returns
 * \li If originalPoint itself is within the image and a
 *     non-transparent pixel, it returns originalPoint.
 * \li Else, if there is are non-transparent pixels in the image, the nearest
 *     non-transparent pixel is returned. (If there are various nearest
 *     neighbors at the same distance, it is undefined which one is returned.)
 * \li Else there are no non-transparent pixels, and simply the point
 *     <tt>0, 0</tt> is returned, but this is a very slow case.
 *
 * TODO Do not use nearest neighbor or other pixel based search algorithms,
 * but work directly with LittleCMS, maybe with a limited, but well-defined,
 * precision… */
QPoint RgbColorSpace::RgbColorSpacePrivate::nearestNeighborSearch(const QPoint originalPoint, const QImage &image)
{
    // Test for special case:
    // originalPoint itself is within the image and non-transparent
    if (image.valid(originalPoint)) {
        if (image.pixelColor(originalPoint).alpha() == 255) {
            return originalPoint;
        }
    }

    // No special case. So we have to actually perform
    // a nearest-neighbor-search.
    int x;
    int y;
    int currentBestX = 0; // 0 is the fallback value
    int currentBestY = 0; // 0 is the fallback value
    int currentBestDistanceSquare = std::numeric_limits<int>::max();
    int x_distance;
    int y_distance;
    int temp;
    for (x = 0; x < image.width(); x++) {
        for (y = 0; y < image.height(); y++) {
            if (image.pixelColor(x, y).alpha() == 255) {
                x_distance = originalPoint.x() - x;
                y_distance = originalPoint.y() - y;
                temp = x_distance * x_distance + y_distance * y_distance;
                if (temp < currentBestDistanceSquare) {
                    currentBestX = x;
                    currentBestY = y;
                    currentBestDistanceSquare = temp;
                }
            }
        }
    }
    return QPoint(currentBestX, currentBestY);
}

/** @brief Get information from an ICC profile via LittleCMS
 *
 * @param profileHandle handle to the ICC profile in which will be searched
 * @param infoType the type of information that is searched
 * @returns A QString with the information. First, it searches the
 * information in the current locale (language code and country code as
 * provided currently by <tt>QLocale</tt>). If the information is not
 * available in this locale, it silently falls back to another available
 * localization. Note that the returned QString() might be empty if the
 * requested information is not available in the ICC profile. */
QString RgbColorSpace::RgbColorSpacePrivate::getInformationFromProfile(cmsHPROFILE profileHandle, cmsInfoType infoType)
{
    // Initialize a char array of 3 values (two for actual characters and a
    // one for a terminating null)cmsFloat64Number
    // The recommended default value for language
    // following LittleCMS documentation is “en”.
    char languageCode[3] = "en";
    // The recommended default value for country
    // following LittleCMS documentation is “US”.
    char countryCode[3] = "US";
    // Update languageCode and countryCode to the actual locale (if possible)
    const QStringList list = QLocale().name().split(QStringLiteral(u"_"));
    // The locale codes should be ASCII only, so QString::toUtf8() should
    // return ASCII-only valid results. We do not know what character encoding
    // LittleCMS expects, but ASCII seems a save choise.
    const QByteArray currentLocaleLanguage = list.at(0).toUtf8();
    const QByteArray currentLocaleCountry = list.at(1).toUtf8();
    if (currentLocaleLanguage.size() == 2) {
        languageCode[0] = currentLocaleLanguage.at(0);
        languageCode[1] = currentLocaleLanguage.at(1);
        // No need for languageCode[2] = 0; for null-terminated string,
        // because the array was yet initialized
        if (currentLocaleCountry.size() == 2) {
            countryCode[0] = currentLocaleCountry.at(0);
            countryCode[1] = currentLocaleCountry.at(1);
            // No need for countryCode[2] = 0; for null-terminated string,
            // because the array was yet initialized
        }
    }
    // Calculate the size of the buffer that we have to provide for
    // cmsGetProfileInfo in order to return a value.
    const cmsUInt32Number resultLength = cmsGetProfileInfo(
        // Profile in which we search:
        profileHandle,
        // The type of information we search:
        infoType,
        // The preferred language in which we want to get the information:
        languageCode,
        // The preferred country for which we want to get the information:
        countryCode,
        // Do not actually provide the information,
        // just return the required buffer size:
        nullptr,
        // Do not actually provide the information,
        // just return the required buffer size:
        0);
    // For the actual buffer size, increment by 1. This helps us to
    // guarantee a null-terminated string later on.
    const cmsUInt32Number bufferLength = resultLength + 1;

    // Allocate the buffer
    wchar_t *buffer = new wchar_t[bufferLength];
    // Initialize the buffer with 0
    for (cmsUInt32Number i = 0; i < bufferLength; ++i) {
        *(buffer + i) = 0;
    }

    // Write the actual information to the buffer
    cmsGetProfileInfo(
        // profile in which we search
        profileHandle,
        // the type of information we search
        infoType,
        // the preferred language in which we want to get the information
        languageCode,
        // the preferred country for which we want to get the information
        countryCode,
        // the buffer into which the requested information will be written
        buffer,
        // the buffer size as previously calculated
        resultLength);
    // Make absolutely sure the buffer is null-terminated by marking its last
    // element (the one that was the +1 "extra" element) as null.
    *(buffer + (bufferLength - 1)) = 0;

    // Create a QString() from the from the buffer
    // cmsGetProfileInfo returns often strings that are smaller than the
    // previously calculated buffer size. But it seems they are
    // null-terminated strings. So we read only up to the first null value.
    // This is save, because we made sure previously that the buffer is
    // indeed null-terminated.
    // QString::fromWCharArray will return a QString. It accepts arrays of
    // wchar_t. wchar_t might have different sizes, either 16 bit or 32 bit
    // depending on the operating system. As Qt’s documantation of
    // QString::fromWCharArray() says:
    //
    //     “If wchar is 4 bytes, the string is interpreted as UCS-4,
    //      if wchar is 2 bytes it is interpreted as UTF-16.”
    //
    // However, apparently this is not exact: When wchar is 4 bytes, surrogate
    // pairs in the code unit array are interpretated like UTF-16: The
    // surrogate pair is recognized as such. TODO static_assert that this is
    // true (which seems complicate: better provide a unit test!) (Which
    // is not strictly UTF-32 conform). Single surrogates cannot be
    // interpretated correctly, but there will be no crash:
    // QString::fromWCharArray will continue to read, also the part after
    // the first UTF error. We can rely on this behaviour: As we do
    // not really know the encoding of the buffer that LittleCMS returns.
    // Therefore, it is a good idea to be compatible for various
    // interpretations.
    QString result = QString::fromWCharArray(
        // Convert to string with these parameters:
        buffer, // read from this buffer
        -1      // read until the first null element
    );

    // Free allocated memory of the buffer
    delete[] buffer;

    // Return
    return result;
}

int RgbColorSpace::maximumChroma() const
{
    return d_pointer->m_maximumChroma;
}

} // namespace PerceptualColor
