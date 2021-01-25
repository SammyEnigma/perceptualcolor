// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2020 Lukas Sommer somerluk@gmail.com
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

#include "perceptualcolorlib_qtconfiguration.h"

// Own headers
// First the interface, which forces the header to be self-contained.
#include "colorwheelimage.h"

#include "PerceptualColor/polarpointf.h"
#include "helper.h"
#include "lchvalues.h"

#include <QPainter>

namespace PerceptualColor {

/** @brief Constructor */
ColorWheelImage::ColorWheelImage(
    const QSharedPointer<PerceptualColor::RgbColorSpace> &colorSpace
) :
    m_rgbColorSpace (colorSpace)
{
    // TODO Check here (and in all other classes taking a colorSpace)
    //      that the color space is actually not a nullptr?!
}

/** @brief Setter for the border property.
 * 
 * The border is the space between the outer outline of the wheel and the
 * limits of the image. The wheel is always centered within the limits of
 * the image. The default value is <tt>0</tt>, which means that the wheel
 * touchs the limits of the image.
 * 
 * @param newBorder The new border size, measured in <em>physical</em>
 * pixels. */
void ColorWheelImage::setBorder(const qreal newBorder)
{
    qreal tempBorder;
    if (newBorder >= 0) {
        tempBorder = newBorder;
    } else {
        tempBorder = 0;
    }
    if (m_borderPhysical != tempBorder) {
        m_borderPhysical = tempBorder;
        // Free the memory used by the old image.
        m_image = QImage();
    }
}

/** @brief Setter for the device pixel ratio (floating point).
 * 
 * This value is set as device pixel ratio (floating point) in the
 * <tt>QImage</tt> that this class holds. It does <em>not</em> change
 * the <em>pixel</em> size of the image or the pixel size of wheel
 * thickness or border.
 * 
 * This is for HiDPI support. You can set this to
 * <tt>QWidget::devicePixelRatioF()</tt> to get HiDPI images in the correct
 * resolution for your widgets. Within a method of a class derived
 * from <tt>QWidget</tt>, you could write:
 * 
 * @snippet test/testcolorwheelimage.cpp ColorWheelImage HiDPI usage
 * 
 * The default value is <tt>1</tt> which means no special scaling.
 * 
 * @param newDevicePixelRatioF the new device pixel ratio as a
 * floating point data type. */
void ColorWheelImage::setDevicePixelRatioF(const qreal newDevicePixelRatioF)
{
    qreal tempDevicePixelRatioF;
    if (newDevicePixelRatioF >= 1) {
        tempDevicePixelRatioF = newDevicePixelRatioF;
    } else {
        tempDevicePixelRatioF = 1;
    }
    if (m_devicePixelRatioF != tempDevicePixelRatioF) {
        m_devicePixelRatioF = tempDevicePixelRatioF;
        // Free the memory used by the old image.
        m_image = QImage();
    }
}

/** @brief Setter for the image size property.
 * 
 * This value fixes the size of the image. The image will be a square
 * of <tt>QSize(newImageSize, newImageSize)</tt>.
 * 
 * @param newImageSize The new image size, measured in <em>physical</em>
 * pixels. */
void ColorWheelImage::setImageSize(const int newImageSize)
{
    int tempImageSize;
    if (newImageSize >= 0) {
        tempImageSize = newImageSize;
    } else {
        tempImageSize = 0;
    }
    if (m_imageSizePhysical != tempImageSize) {
        m_imageSizePhysical = tempImageSize;
        // Free the memory used by the old image.
        m_image = QImage();
    }
}

/** @brief Setter for the wheel thickness property.
 * 
 * The wheel thickness is the distance between the inner outline and the
 * outer outline of the wheel.
 * 
 * @param newWheelThickness The new wheel thickness, measured
 * in <em>logical</em> pixels. */
void ColorWheelImage::setWheelThickness(const qreal newWheelThickness)
{
    qreal temp;
    if (newWheelThickness >= 0) {
        temp = newWheelThickness;
    } else {
        temp = 0;
    }
    if (m_wheelThicknessPhysical != temp) {
        m_wheelThicknessPhysical = temp;
        // Free the memory used by the old image.
        m_image = QImage();
    }
}

/** @brief Delivers an image of a color wheel
* 
* @returns Delivers a square image of a color wheel. Its size
* is <tt>QSize(imageSize, imageSize)</tt>. All pixels
* that do not belong to the wheel itself will be transparent.
* Antialiasing is used, so there is no sharp border between
* transparent and non-transparent parts. Depending on the
* values for lightness and chroma and the available colors in
* the current color space, there may be some hue who is out of
*  gamut; if so, this part of the wheel will be transparent.
* 
* @todo Out-of-gamut situations should automatically be handled. */
QImage ColorWheelImage::getImage()
{
    if (m_image.isNull()) {
        generateNewImage();
    }
    return m_image;
}

/** @brief Generates a new wheel image.
 * 
 * The result is stored in @ref m_image. (If @ref m_imageSizePhysical is
 * <tt>0</tt> than the image will be a <tt>null</tt> image.) */
void ColorWheelImage::generateNewImage()
{
    // Assert that static_cast<int> always rounds down.
    static_assert(static_cast<int>(1.9) == 1);
    static_assert(static_cast<int>(1.5) == 1);
    static_assert(static_cast<int>(1.0) == 1);

    // Special case: zero-size-image
    if (m_imageSizePhysical <= 0) {
        m_image = QImage();
        return;
    }

    // Firsts of all, generate a non-anti-aliased, intermediate, color wheel,
    // but with some pixels extra at the inner and outer side. The overlap
    // defines an overlap for the wheel, so there are some more pixels that
    // are drawn at the outer and at the inner border of the wheel, to allow
    // later clipping with anti-aliasing
    constexpr int overlap = 1;
    PolarPointF polarCoordinates;
    int x;
    int y;
    QColor rgbColor;
    LchDouble lch;
    // Calculate maximum value for x index and y index
    int maxExtension = m_imageSizePhysical - 1; 
    qreal center = maxExtension / static_cast<qreal>(2);
    QImage tempWheel = QImage(
        QSize(m_imageSizePhysical, m_imageSizePhysical),
        QImage::Format_ARGB32_Premultiplied
    );
    // Because there may be out-of-gamut colors for some hue (depending on the
    // given lightness and chroma value) which are drawn transparent, it is
    // important to initialize this image with a transparent background.
    tempWheel.fill(Qt::transparent);
    lch.L = LchValues::defaultLightness;
    lch.C = LchValues::srgbVersatileChroma;
    // minimumRadial: Adding "+ 1" would reduce the workload (less pixel to
    // process) and still work mostly, but not completely. It creates sometimes
    // artifacts in the anti-aliasing process. So we don't do that.
    const qreal minimumRadial =
        center
            - m_wheelThicknessPhysical
            - m_borderPhysical
            - overlap;
    const qreal maximumRadial = center - m_borderPhysical + overlap;
    for (x = 0; x <= maxExtension; ++x) {
        for (y = 0; y <= maxExtension; ++y) {
            polarCoordinates = PolarPointF(QPointF(x - center, center - y));
            if (
                inRange<qreal>(
                    minimumRadial,
                    polarCoordinates.radial(),
                    maximumRadial
                )
                
            ) {
                // We are within the wheel
                lch.h = polarCoordinates.angleDegree();
                rgbColor = m_rgbColorSpace->colorRgb(lch);
                if (rgbColor.isValid())
                {
                    tempWheel.setPixelColor(
                        x,
                        y,
                        rgbColor
                    );
                }
            }
        }
    }

    // construct our final QImage with transparent background
    m_image = QImage(
        QSize(m_imageSizePhysical, m_imageSizePhysical),
        QImage::Format_ARGB32_Premultiplied
    );
    m_image.fill(Qt::transparent);

    // Calculate diameter of the outer circle
    const qreal outerCircleDiameter =
        m_imageSizePhysical - 2 * m_borderPhysical;

    // Special case: zero-size-image
    if (outerCircleDiameter <= 0) {
        // Make sure to return a completly transparent image.
        // If we would continue, in spite of an outer diameter of 0,
        // we might get a non-transparent pixel in the middle.
        return;
    }

    // Paint an anti-aliased wheel, using the the temporary (non-anti-aliased)
    // color wheel as brush
    QPainter myPainter(&m_image);
    myPainter.setRenderHint(QPainter::Antialiasing, true);
    myPainter.setPen(QPen(Qt::NoPen));
    myPainter.setBrush(QBrush(tempWheel));
    myPainter.drawEllipse(
        QRectF(
            m_borderPhysical,
            m_borderPhysical,
            outerCircleDiameter,
            outerCircleDiameter
        )
    );

    // set the inner circle of the wheel to anti-aliased transparency
    const qreal innerCircleDiameter =
        m_imageSizePhysical
            - 2 * (m_wheelThicknessPhysical + m_borderPhysical);
    if (innerCircleDiameter > 0) {
        myPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        myPainter.setRenderHint(QPainter::Antialiasing, true);
        myPainter.setPen(QPen(Qt::NoPen));
        myPainter.setBrush(QBrush(Qt::SolidPattern));
        myPainter.drawEllipse(
            QRectF(
                m_wheelThicknessPhysical + m_borderPhysical,
                m_wheelThicknessPhysical + m_borderPhysical,
                innerCircleDiameter,
                innerCircleDiameter
            )
        );
    }

    // Set the correct scaling information for the image
    m_image.setDevicePixelRatio(m_devicePixelRatioF);
}

}
