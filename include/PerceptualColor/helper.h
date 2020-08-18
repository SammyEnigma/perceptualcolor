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

#ifndef HELPER_H
#define HELPER_H

#include <QImage>
#include <QWheelEvent>

#include <lcms2.h>

/** @file
 * 
 * Declaration of the @ref PerceptualColor::Helper namespace and its members.
 * 
 * Also contains some general documentation for this library.
 * 
 * The source code of the library is in UTF8. A static_assert within the
 * header @ref helper.h makes sure your compiler actually treats it as UTF8.
 */
// Test if the compiler treats the source code actually as UTF8.
// A test string is converted to UTF8 code units (u8"") and each
// code unit is checked to be correct.
static_assert(
    (static_cast<quint8>(*((u8"🖌")+0)) == 0xF0)
        && (static_cast<quint8>(*((u8"🖌")+1)) == 0x9F)
        && (static_cast<quint8>(*((u8"🖌")+2)) == 0x96)
        && (static_cast<quint8>(*((u8"🖌")+3)) == 0x8C)
        && (static_cast<quint8>(*((u8"🖌")+4)) == 0x00),
    "This source code has to be read-in as UTF8 by the compiler."
);

/** @mainpage
 * This library provides various Qt GUI components for choosing colors, with
 * focus on an intuitive and perceptually uniform presentation. The GUI
 * widgets are based internally on the LCh color model, which does reflect
 * the human perceptuan much better than RGB or its transforms like HSV.
 * However, the widgets do not require the user itself to know anything
 * about LCh at all, because the graphical representations is
 * intuitive enough. This library lives in the namespace #PerceptualColor.
 * 
 * How to get started? @ref PerceptualColor::ColorDialog provides a perceptual
 * replacement for QColorDialog:
 * @snippet test/testcolordialog.cpp ColorDialog Get color
 * 
 * And there are also individual widgets available. Among others:
 * - @ref PerceptualColor::WheelColorPicker (a full-featureed color wheel)
 * - @ref PerceptualColor::ColorPatch (to show a particular color)
 * - @ref PerceptualColor::ChromaHueDiagram (for selecting colors at a given
 *   lightness)
 * 
 * The library depends on (and therefore you has to link
 * against) <a href="https://www.qt.io/">Qt</a> 5 (only the modules
 * Qt Core, Qt Gui and Qt Widgets are necessary) and
 * <a href="http://www.littlecms.com/">LittleCMS 2</a>.
 * 
 * The library uses in general @c int for integer values, because @c QSize()
 * and @c QPoint() also do. As the library relies heavily on the usage of
 * @c QSize() and @c QPoint(), this seems reasonable. For the same reason, it
 * uses generally @c qreal for floating point values, because @c QPointF()
 * also does. Output colors that are shown on the screen, are usually
 * 8-bit-per-channel colors. For internal transformation, usually @c qreal
 * is used for each channel, giving a better precision and reducing rounding
 * errors.
 * 
 * The source code of the library is in UTF8. A static_assert within the
 * header @ref helper.h makes sure your compiler actually treats it as UTF8. */

/** @brief The namespace of this library.
 * 
 * Everything that is provides in this library is encapsulated within this
 * namespace.
 * 
 * @todo Translations: Color picker/Select Color → Farbwähler/Farbauswahl etc…
 * @todo Provide more than 8 bit per channel for more precision? 10 bit? 12 bit?
 * @todo Only expose in the headers and in the public API what is absolutely
 * necessary.
 * @todo Switch to the pimpl ideom?
 * @todo Qt Designer support for the widgets
 * @todo QPainter: Quote from documentation: For optimal performance only use
 * the format types @c QImage::Format_ARGB32_Premultiplied,
 * @c QImage::Format_RGB32 or @c QImage::Format_RGB16. Any other format,
 * including @c QImage::Format_ARGB32, has significantly worse performance.
 * @todo Follow KDE's <b><a href="https://hig.kde.org/index.html">HIG</a></b>
 * @todo Add screenshots of widgets to the documentation
 * @todo Would it be a good idea to implement Q_PROPERTY RESET overall? See
 * also https://phabricator.kde.org/T12359
 * @todo Spell checking for the documentation */
namespace PerceptualColor {

/** @brief Various smaller help elements.
 * 
 * This namespace groups together various smaller elements that are used
 * across the library but do not belong stricly to one of the classes.
 * 
 * @todo Decide for each member of this namespace if it can be moved into
 * a class because it’s only used in this single class. */
namespace Helper {

    /** @brief An RGB color.
     * 
     * Storage of floating point RGB values in a format that is practical
     * for working with * <a href="http://www.littlecms.com/">LittleCMS</a>
     * (can be treated as buffer). The valid range for each component is 0‥1.
     * 
     * Example:
     * @snippet test/testhelper.cpp Helper Use cmsRGB */
    struct cmsRGB {
        /** @brief The red value. */
        cmsFloat64Number red;
        /** @brief The green value. */
        cmsFloat64Number green;
        /** @brief The blue value. */
        cmsFloat64Number blue;
    };

    /** @brief Precision for gamut boundary search
     * 
     * We have to search sometimes for the gamut boundary. This value defines
     * the precision of the search:  Smaller values mean better precision and
     * slower processing. */
    constexpr qreal gamutPrecision = 0.001;

    /** @brief Template function to test if a value is within a certain range
     * @param low the lower limit
     * @param x the value that will be tested
     * @param high the higher limit
     * @returns @snippet this Helper inRange */
    template<typename T> bool inRange(const T& low, const T& x, const T& high)
    {
        return (
            //! [Helper inRange]
            (low <= x) && (x <= high)
            //! [Helper inRange]
        );
    }

    /** @brief LCh default values
     * 
     * According to the
     * <a href="https://de.wikipedia.org/w/index.php?title=Lab-Farbraum&oldid=197156292">
     * German Wikipedia</a>, the Lab color space has the following ranges:
     * 
     * | Lab axis  | Usual software implementation | Actual human perception |
     * | :-------- | ----------------------------: | ----------------------: |
     * | lightness |                        0..100 |                   0‥100 |
     * | a         |                      −128‥127 |                −170‥100 |
     * | b         |                      −128‥127 |                −100‥150 |
     * 
     * The range of −128‥127 is in C++ a signed 8‑bit integer. But this
     * data type usually used in software implementations is (as the table
     * clearly shows) not enough to cover the hole range of actual human color
     * perception.
     *
     * Unfortunately, there is no information about the LCh color space. But
     * we can deduce:
     * - @b Lightness: Same range as for Lab: <b>0‥100</b>
     * - @b Chroma: The Chroma value is the distance from the center of the
     *   coordinate system in the a‑b‑plane. Therefore it cannot be bigger
     *   than Pythagoras of the biggest possible absolute value on a axis and
     *   b axis: √[(−170)² + 150²] ≈ 227. Very likely the real range is
     *   smaller, but we do not know exactly how much. At least, we can be
     *   sure that this range is big enough: <b>0‥227</b>
     * - @b Hue: As the angle in a polar coordinate system, it has to
     *   be: <b>0°‥360°</b>
     * 
     * But what could be useful default values? This struct provides some
     * proposals. All values are <tt>constexpr</tt>. */
    struct LchDefaults {
        /** @brief Default chroma value
         * 
         *  For chroma, a default value of 0 might be a good
         *  choise because it is less likely to make  out-of-gamut problems on
         *  any lightness (except maybe extreme white or extreme black). And
         *  it results in an achromatic color and is therefore perceived as
         *  neutral.
         *  @sa @ref versatileSrgbChroma
         *  @sa @ref maxSrgbChroma */
        static constexpr qreal defaultChroma = 0;
        /** @brief Default hue value
         *
         *  For the hue, a default value of 0 might be used by convention. */
        static constexpr qreal defaultHue = 0;
        /** @brief Default lightness value
         * 
         *  For the lightness, a default value of 50 seems a good choise as it
         *  is half the way in the defined lightness range of 0‥100 (thought
         *  not all gamuts offer the hole range from 0 to 100). As it is quite
         *  in the middle of the gamut solid, it allows for quite big values
         *  for chroma at different hues without falling out-of-gamut.
         *  Together with a chroma of 0, it also approximates the color with
         *  the highest possible contrast against the hole surface of the
         *  gamut solid; this is interesting for background colors of
         *  gamut diagrams. */
        static constexpr qreal defaultLightness = 50;
        /** @brief Maximum chroma value in
         * <a href="http://www.littlecms.com/">LittleCMS</a>'
         * build-in sRGB gamut
         * 
         *  @sa @ref defaultChroma
         *  @sa @ref versatileSrgbChroma */
        static constexpr qreal maxSrgbChroma = 132;
        /** @brief Versatile chroma value in
         * <a href="http://www.littlecms.com/">LittleCMS</a>'
         * build-in sRGB gamut
         * 
         *  Depending on the use case, there might be an alternative to
         *  the neutral gray defaultChroma(). For a lightness of 50, this
         *  value is the maximum chroma available at all possible hues within
         *  a usual sRGB gamut.
         *  @sa @ref defaultChroma
         *  @sa @ref maxSrgbChroma */
        static constexpr qreal versatileSrgbChroma = 32;
    };

    qreal standardWheelSteps(QWheelEvent *event);

    QImage transparencyBackground();

}

}

#endif // HELPER_H
