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

#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII

// Own headers
// First the interface, which forces the header to be self-contained.
#include "PerceptualColor/simplecolorwheel.h"
// Second, the private implementation.
#include "simplecolorwheel_p.h"

#include "PerceptualColor/lchdouble.h"
#include "PerceptualColor/helper.h"
#include "PerceptualColor/polarpointf.h"

#include <math.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include <lcms2.h>

namespace PerceptualColor {

/** @brief Constructor */
SimpleColorWheel::SimpleColorWheel(RgbColorSpace *colorSpace, QWidget *parent) :
    AbstractCircularDiagram(parent),
    d_pointer(new SimpleColorWheelPrivate(this))
{
    // Setup LittleCMS (must be first thing because other operations
    // rely on working LittleCMS)
    d_pointer->m_rgbColorSpace = colorSpace;

    // Simple initialization
    // We don't use the reset methods as they would update the image/pixmap
    // each time, and this could crash if done before everything is
    // initialized.
    d_pointer->m_hue = Helper::LchDefaults::defaultHue;
    d_pointer->m_mouseEventActive = false;
}

/** @brief Default destructor */
SimpleColorWheel::~SimpleColorWheel() noexcept
{
}

/** @brief Constructor
 * 
 * @param backLink Pointer to the object from which <em>this</em> object
 * is the private implementation. */
SimpleColorWheel::SimpleColorWheelPrivate::SimpleColorWheelPrivate(
    SimpleColorWheel *backLink
) : q_pointer(backLink)
{
}

/** @brief The diameter of the widget content
 * 
 * @returns the diameter of the content of this widget, coordinates in pixel.
 * The content is always in form of a circle. This value includes the space
 * for the focus indicator, independently if currently the focus indicator
 * is actually displayed or not. This value corresponds to the smaller one
 * of width() and height().
 */
int SimpleColorWheel::contentDiameter() const
{
    return qMin(width(), height());
}

/** @brief Converts window coordinates to wheel coordinates.
 * 
 * @param widgetCoordinates coordinates in the coordinate system of this widget
 * @returns “wheel” coordinates: Coordinates in a polar coordinate system who's
 * center is exactly in the middle of the displayed wheel.
 */
PolarPointF SimpleColorWheel
    ::SimpleColorWheelPrivate
    ::fromWidgetCoordinatesToWheelCoordinates
(
    const QPoint widgetCoordinates
) const
{
    qreal radius = q_pointer->contentDiameter() / static_cast<qreal>(2);
    return PolarPointF(
        QPointF(widgetCoordinates.x() - radius, radius - widgetCoordinates.y())
    );
}

/** @brief Converts wheel coordinates to widget coordinates.
 * 
 * @param wheelCoordinates Coordinates in a polar coordinate system who's
 * center is exactly in the middle of the displayed wheel.
 * @returns coordinates in the coordinate system of this widget
 */
QPointF SimpleColorWheel
    ::SimpleColorWheelPrivate
    ::fromWheelCoordinatesToWidgetCoordinates
(
    const PolarPointF wheelCoordinates
) const
{
    qreal radius = q_pointer->contentDiameter() / static_cast<qreal>(2);
    QPointF temp = wheelCoordinates.toCartesian();
    temp.setX(temp.x() + radius);
    temp.setY(radius - temp.y());
    return temp;
}

/** @brief React on a mouse press event.
 * 
 * Reimplemented from base class.
 *
 * Does not differentiate between left, middle and right mouse click.
 * If the mouse is clicked within the wheel ribbon, than the marker is placed
 * here and further mouse movements are tracked.
 * 
 * @param event The corresponding mouse event
 */
void SimpleColorWheel::mousePressEvent(QMouseEvent *event)
{
    qreal radius = contentDiameter() / static_cast<qreal>(2) - border();
    PolarPointF myPolarPoint =
        d_pointer->fromWidgetCoordinatesToWheelCoordinates(event->pos());
    if (
        Helper::inRange<qreal>(
            radius-m_wheelThickness,
            myPolarPoint.radial(),
            radius
        ) 
    ) {
        setFocus(Qt::MouseFocusReason);
        d_pointer->m_mouseEventActive = true;
        setHue(myPolarPoint.angleDegree());
    } else {
        // Make sure default coordinates like drag-window
        // in KDE's Breeze widget style works
        event->ignore();
    }
}

/** @brief React on a mouse move event.
 *
 * Reimplemented from base class.
 *
 * Reacts only on mouse move events if previously there had been a mouse press
 * event that had been accepted.
 * 
 * If previously there had not been a mouse press event, the mouse move event
 * is ignored.
 * 
 * @param event The corresponding mouse event
 */
void SimpleColorWheel::mouseMoveEvent(QMouseEvent *event)
{
    if (d_pointer->m_mouseEventActive) {
        setHue(
            d_pointer->fromWidgetCoordinatesToWheelCoordinates(
                event->pos()
            ).angleDegree()
        );
    } else {
        // Make sure default coordinates like drag-window in KDE's Breeze
        // widget style works
        event->ignore();
    }
}

/** @brief React on a mouse release event.
 *
 * Reimplemented from base class. Does not differentiate between left,
 * middle and right mouse click.
 * 
 * @param event The corresponding mouse event
 */
void SimpleColorWheel::mouseReleaseEvent(QMouseEvent *event)
{
    if (d_pointer->m_mouseEventActive) {
        d_pointer->m_mouseEventActive = false;
        setHue(
            d_pointer->fromWidgetCoordinatesToWheelCoordinates(
                event->pos()
            ).angleDegree()
        );
    } else {
        // Make sure default coordinates like drag-window in KDE's Breeze
        // widget style works
        event->ignore();
    }
}

/** @brief React on a mouse wheel event.
 *
 * Reimplemented from base class.
 * 
 * Scrolling up raises the hue value, scrolling down lowers the hue value.
 * Of course, the point at 0°/360° it not blocking.
 * 
 * @param event The corresponding mouse event
 */
void SimpleColorWheel::wheelEvent(QWheelEvent *event)
{
    // The step (coordinates in degree) that the hue angle is changed when
    // a mouse wheel event occurs.
    // TODO What is a reasonable value for this?
    static constexpr qreal wheelStep = 5;
    qreal radius = contentDiameter() / static_cast<qreal>(2) - border();
    PolarPointF myPolarPoint =
        d_pointer->fromWidgetCoordinatesToWheelCoordinates(event->pos());
    if (
        // Do nothing while mouse movement is tracked anyway. This would
        // be confusing.
        (!d_pointer->m_mouseEventActive) &&
        // Only react on wheel events when its in the wheel ribbon or in
        // the inner hole.
        (myPolarPoint.radial() <= radius) &&
        // Only react on good old vertical wheels, and not on horizontal wheels.
        (event->angleDelta().y() != 0)
    ) {
        setHue(
            d_pointer->m_hue + Helper::standardWheelSteps(event) * wheelStep
        );
    } else {
        event->ignore();
    }
}

// TODO How to catch and treat invalid ranges? For QImage creation for
// example? Use also minimalSize()?

// TODO deliver also arrow keys to the child widget! WARNING DANGER

/** @brief React on key press events.
 * 
 * Reimplemented from base class.
 * 
 * Reacts on key press events. When the plus key or the minus key are pressed,
 * it raises or lowers the hue. When Qt::Key_Insert or Qt::Key_Delete are
 * pressed, it raises or lowers the hue faster.
 * 
 * @param event the paint event
 */
void SimpleColorWheel::keyPressEvent(QKeyEvent *event)
{
    constexpr qreal wheelStep = 5;
    constexpr qreal bigWheelStep = 15;
    switch (event->key()) {
        case Qt::Key_Plus: 
            setHue(d_pointer->m_hue + wheelStep);
            break;
        case Qt::Key_Minus:
            setHue(d_pointer->m_hue - wheelStep);
            break;
        case Qt::Key_Insert:
            setHue(d_pointer->m_hue + bigWheelStep);
            break;
        case Qt::Key_Delete:
            setHue(d_pointer->m_hue - bigWheelStep);
            break;
        default:
            /* Quote from Qt documentation:
             * 
             * If you reimplement this handler, it is very important
             * that you call the base class implementation if you do not
             * act upon the key.
             * 
             * The default implementation closes popup widgets if the user
             * presses the key sequence for QKeySequence::Cancel (typically
             * the Escape key). Otherwise the event is ignored, so that the
             * widget's parent can interpret it. */
            QWidget::keyPressEvent(event);
            break;
    }
}

/* TODO Support more mouse buttons?
 * 
 * enum Qt::MouseButton
 * flags Qt::MouseButtons
 * 
 * Qt::BackButton	0x00000008
 * The 'Back' button. (Typically present on the 'thumb' side of a mouse
 * with extra buttons. This is NOT the tilt wheel.)
 * 
 * Qt::ForwardButton	0x00000010
 * The 'Forward' Button. (Typically present beside the 'Back' button, and
 * also pressed by the thumb.)
 */

/** @brief Paint the widget.
 * 
 * Reimplemented from base class.
 * 
 * Paints the widget. Takes the existing m_wheelPixmap and paints
 * them on the widget. Paints, if appropriate, the focus indicator.
 * Paints the marker. Relies on that m_wheelPixmap are up to date.
 * 
 * @param event the paint event
 * 
 * @todo Make the wheel to be drawn horizontally and vertically aligned.
 * 
 * @todo Better design on small widget sizes
 */
void SimpleColorWheel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    // We do not paint directly on the widget, but on a QImage buffer first:
    // Render anti-aliased looks better. But as Qt documentation says:
    //
    //      “Renderhints are used to specify flags to QPainter that may or
    //       may not be respected by any given engine.”
    //
    // Painting here directly on the widget might lead to different
    // anti-aliasing results depending on the underlying window system. This
    // is especially problematic as anti-aliasing might shift or not a pixel
    // to the left or to the right. So we paint on a QImage first. As QImage
    // (at difference to QPixmap and a QWidget) is independent of native
    // platform rendering, it guarantees identical anti-aliasing results on
    // all platforms. Here the quote from QPainter class documentation:
    //
    //      “To get the optimal rendering result using QPainter, you should
    //       use the platform independent QImage as paint device; i.e. using
    //       QImage will ensure that the result has an identical pixel
    //       representation on any platform.”
    QImage paintBuffer(size(), QImage::Format_ARGB32_Premultiplied);
    paintBuffer.fill(Qt::transparent);
    QPainter painter(&paintBuffer);

    // paint the wheel from the cache
    if (!d_pointer->m_wheelImageReady) {
        d_pointer->updateWheelImage();
    }
    painter.drawImage(0, 0, d_pointer->m_wheelImage);

    // paint the marker
    qreal radius = contentDiameter() / static_cast<qreal>(2) - border();
    // get widget coordinates for our marker
    QPointF myMarkerInner = d_pointer->fromWheelCoordinatesToWidgetCoordinates(
        PolarPointF(
            radius - m_wheelThickness,
            d_pointer->m_hue
        )
    );
    QPointF myMarkerOuter = d_pointer->fromWheelCoordinatesToWidgetCoordinates(
        PolarPointF(
            radius,
            d_pointer->m_hue
        )
    );
    // draw the line
    QPen pen;
    pen.setWidth(markerThickness);
    pen.setCapStyle(Qt::FlatCap);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawLine(myMarkerInner, myMarkerOuter);

    // Paint a focus indicator if the widget has the focus
    if (hasFocus()) {
        pen.setWidth(markerThickness);
        pen.setColor(focusIndicatorColor());
        painter.setPen(pen);
        painter.drawEllipse(
            markerThickness / 2, // Integer division (rounding down)
            markerThickness / 2, // Integer division (rounding down)
            contentDiameter() - markerThickness,
            contentDiameter() - markerThickness
        );
    }

    // Paint the buffer to the actual widget
    QPainter(this).drawImage(0, 0, paintBuffer);
}

/** @brief React on a resize event.
 *
 * Reimplemented from base class.
 * 
 * @param event The corresponding resize event
 */
void SimpleColorWheel::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    // TODO The image cache is not necessarily invalid now. Thought the widget
    // was resized, the image itself might stay in the same size. See also
    // same problem for ChromaHueDiagram and for ChromaLightnessDiagram
    // ChromaLightnessDiagram’s resize() call done here within the child class
    // WheelColorPicker. This situation is relevant for the real-world usage,
    // when the user scales the window only horizontally or only vertically.
    d_pointer->m_wheelImageReady = false;
    /* As by Qt documentation:
     *     “The widget will be erased and receive a paint event immediately
     *      after processing the resize event. No drawing need be (or should
     *      be) done inside this handler.” */
}

qreal SimpleColorWheel::hue() const
{
    return d_pointer->m_hue;
}

qreal SimpleColorWheel::wheelRibbonChroma() const
{
    return Helper::LchDefaults::versatileSrgbChroma;
}

/**
 * Set the hue property. The hue property is the hue angle degree in the
 * range from 0 to 360, where the circle is again at its beginning. The
 * value is gets normalized to this range. So
 * \li 0 gets 0
 * \li 359.9 gets 359.9
 * \li 360 gets 0
 * \li 361.2 gets 1.2
 * \li 720 gets 0
 * \li -1 gets 359
 * \li -1.3 gets 358.7
 * 
 * After changing the hue property, the widget gets updated.
 * @param newHue The new hue value to set.
 */
void SimpleColorWheel::setHue(const qreal newHue)
{
    qreal temp = PolarPointF::normalizedAngleDegree(newHue);
    if (d_pointer->m_hue != temp) {
        d_pointer->m_hue = temp;
        Q_EMIT hueChanged(d_pointer->m_hue);
        update();
    }
}

// TODO The widget content should be centered horizontally and vertically

/** @brief Provide the size hint.
 *
 * Reimplemented from base class.
 * 
 * @returns the size hint
 * 
 * @sa minimumSizeHint()
 */
QSize SimpleColorWheel::sizeHint() const
{
    return QSize(300, 300);
}

/** @brief Provide the minimum size hint.
 *
 * Reimplemented from base class.
 * 
 * @returns the minimum size hint
 * 
 * @sa sizeHint()
 */
QSize SimpleColorWheel::minimumSizeHint() const
{
    return QSize(100, 100);
}

// TODO What when some of the wheel colors are out of gamut?

/** @brief Refresh the wheel ribbon pixmap
 * 
 * This class has a cache for the pixmap of the wheel ribbon.
 * 
 * This data is cached because it is often needed and it would be expensive
 * to calculate it again and again on the fly.
 * 
 * Calling this function updates this cached data.
 * 
 * An update might be necessary if the data the pixmap relies on, has changed.
 * For example, if the wheelRibbonChroma() property or the widget size have
 * changed, this function has to be called. But do not call this function
 * directly. Rather set m_wheelPixmapReady to @e false, so that the update
 * is only calculated when really a paint event is done, and not when the
 * widget is invisible anyway.
 * 
 * This function does not repaint the widget! After calling this function,
 * you have to call manually <tt>update()</tt> to schedule a re-paint of the
 * widget, if you wish so. */
void SimpleColorWheel::SimpleColorWheelPrivate::updateWheelImage()
{
    if (m_wheelImageReady) {
        return;
    }

    m_wheelImage = generateWheelImage(
        m_rgbColorSpace,
        // TODO How to treat QSize(0, 0)? Also in ChromaLightnessDiagram!!
        qMin(q_pointer->size().width(), q_pointer->size().height()),
        q_pointer->border(),
        m_wheelThickness,
        Helper::LchDefaults::defaultLightness,
        q_pointer->wheelRibbonChroma()
    );
    m_wheelImageReady = true;
}

/** @brief Reset the hue() property. */
void SimpleColorWheel::resetHue()
{
    setHue(Helper::LchDefaults::defaultHue);
}

/** @brief The border between the outer border of the wheel ribbon and
* the border of the widget.
* 
* The diagram is not painted on the whole extend of the widget. A border
* is left to allow that the focus indicator can be painted completely
* even when the widget has the focus. The border is determined
* automatically, its value depends on markerThickness(). */
int SimpleColorWheel::border() const
{
    return 2 * markerThickness;
}


/** @brief Generates an image of a color wheel
* 
* @param colorSpace the color space that is used
* @param outerDiameter the outer diameter of the wheel in pixel
* @param border the border that is left empty around the wheel
* @param thickness the thickness of the wheel
* @param lightness the  (LCh lightness, range 0..100)
* @param chroma the LCh chroma value
* @returns Generates a square image of a color wheel. Its size
* is <tt>QSize(outerDiameter, outerDiameter)</tt>. All pixels
* that do not belong to the wheel itself will be transparent.
* Antialiasing is used, so there is no sharp border between
* transparent and non-transparent parts. Depending on the
* values for lightness and chroma, there may be some hue
* who is out of gamut; if so, it will be transparent. TODO
* Out-of-gamut situations should automatically be handled.
*/
QImage SimpleColorWheel::generateWheelImage(
    RgbColorSpace *colorSpace,
    const int outerDiameter,
    const qreal border,
    const qreal thickness,
    const qreal lightness,
    const qreal chroma)
{
QElapsedTimer myTimer;
myTimer.start();
    if (outerDiameter <= 0) {
        return QImage();
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
    LchDouble LCh; // uses cmsFloat64Number internally
    // Calculate maximum value for x index and y index
    int maxExtension = outerDiameter - 1; 
    qreal center = maxExtension / static_cast<qreal>(2);
    QImage rawWheel = QImage(
        QSize(outerDiameter, outerDiameter),
        QImage::Format_ARGB32_Premultiplied
    );
    // Because there may be out-of-gamut colors for some hue (depending on the
    // given lightness and chroma value) which are drawn transparent, it is
    // important to initialize this image with a transparent background.
    rawWheel.fill(Qt::transparent);
    LCh.L = lightness;
    LCh.C = chroma;
    // minimalRadial: Adding "+ 1" would reduce the workload (less pixel to
    // process) and still work mostly, but not completely. It creates sometimes
    // artifacts in the anti-aliasing process. So we don't do that.
    qreal minimumRadial = center - thickness - border - overlap;
    qreal maximumRadial = center - border + overlap;
    for (x = 0; x <= maxExtension; ++x) {
        for (y = 0; y <= maxExtension; ++y) {
            polarCoordinates = PolarPointF(QPointF(x - center, center - y));
            if (Helper::inRange<qreal>(minimumRadial, polarCoordinates.radial(), maximumRadial)) {
                // We are within the wheel
                LCh.h = polarCoordinates.angleDegree();
                rgbColor = colorSpace->colorRgb(LCh);
                if (rgbColor.isValid())
                {
                    rawWheel.setPixelColor(
                        x,
                        y,
                        rgbColor
                    );
                }
            }
        }
    }

    // construct our final QImage with transparent background
    QImage finalWheel = QImage(
        QSize(outerDiameter, outerDiameter),
        QImage::Format_ARGB32_Premultiplied
    );
    finalWheel.fill(Qt::transparent);
    
    // paint an anti-aliased circle with the raw (non-anti-aliased)
    // color wheel as brush
    QPainter myPainter(&finalWheel);
    myPainter.setRenderHint(QPainter::Antialiasing, true);
    myPainter.setPen(QPen(Qt::NoPen));
    myPainter.setBrush(QBrush(rawWheel));
    myPainter.drawEllipse(
        QRectF(
            border,
            border,
            outerDiameter - 2 * border,
            outerDiameter - 2 * border
        )
    );
    
    // set the inner circle of the wheel to anti-aliased transparency
    myPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    myPainter.setRenderHint(QPainter::Antialiasing, true);
    myPainter.setPen(QPen(Qt::NoPen));
    myPainter.setBrush(QBrush(Qt::SolidPattern));
    myPainter.drawEllipse(
        QRectF(
            thickness + border,
            thickness + border,
            outerDiameter - 2 * (thickness + border),
            outerDiameter - 2 * (thickness + border)
        )
    );

    qDebug()
        << "Generating simple color wheel image took"
        << myTimer.restart()
        << "ms.";
    // return
    return finalWheel;
}

}
