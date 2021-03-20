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

#include "perceptualcolorlib_internal.h"

// Own headers
// First the interface, which forces the header to be self-contained.
#include "PerceptualColor/gradientslider.h"
// Second, the private implementation.
#include "gradientslider_p.h"

#include <QDebug>
#include <QGuiApplication>
#include <QPainter>

#include <PerceptualColor/fullcolordescription.h>
#include <helper.h>

namespace PerceptualColor {

GradientSlider::GradientSlider(
    const QSharedPointer<PerceptualColor::RgbColorSpace> &colorSpace,
    QWidget *parent
) :
    AbstractDiagram(parent),
    d_pointer(new GradientSliderPrivate(this))
{
    d_pointer->initialize(colorSpace, Qt::Orientation::Vertical);
}

GradientSlider::GradientSlider(
    const QSharedPointer<PerceptualColor::RgbColorSpace> &colorSpace,
    Qt::Orientation orientation,
    QWidget* parent
) :
    AbstractDiagram(parent),
    d_pointer(new GradientSliderPrivate(this))
{
    d_pointer->initialize(colorSpace, orientation);
}

/** @brief Default destructor */
GradientSlider::~GradientSlider() noexcept
{
}

/** @brief Constructor
 *
 * @param backLink Pointer to the object from which <em>this</em> object
 * is the private implementation. */
GradientSlider::GradientSliderPrivate::GradientSliderPrivate(
    GradientSlider *backLink
) : q_pointer(backLink)
{
}

void GradientSlider::GradientSliderPrivate::initialize(
    const QSharedPointer<RgbColorSpace> &colorSpace,
    Qt::Orientation orientation
)
{
    q_pointer->setFocusPolicy(Qt::StrongFocus);
    m_rgbColorSpace = colorSpace;
    q_pointer->setOrientation(orientation); // also updates the size policy
    LchaDouble one;
    one.l = 50;
    one.c = 65;
    one.h = 100;
    one.a = 1;
    LchaDouble two;
    two.l = 60;
    two.c = 85;
    two.h = 300;
    two.a = 1;
    q_pointer->setColors(
        one,
        two
    );
    m_gradientImageReady = false;
}

QSize GradientSlider::sizeHint() const
{
    return minimumSizeHint();
}

QSize GradientSlider::minimumSizeHint() const
{
    QSize temp;
    if (d_pointer->m_orientation == Qt::Orientation::Vertical) {
        temp = QSize(
            d_pointer->m_gradientThickness,
            d_pointer->m_gradientMinimumLength
        );
    } else {
        temp = QSize(
            d_pointer->m_gradientMinimumLength,
            d_pointer->m_gradientThickness
        );
    }
    return temp;
}

qreal GradientSlider
    ::GradientSliderPrivate
    ::fromWindowCoordinatesToValue
(
    QPoint windowCoordinates
)
{
    qreal temp;
    if (m_orientation == Qt::Orientation::Vertical) {
        temp = (q_pointer->size().height() - windowCoordinates.y())
            / static_cast<qreal>(q_pointer->size().height());
    } else {
        if (q_pointer->layoutDirection() == Qt::LayoutDirection::LeftToRight) {
            temp = windowCoordinates.x()
                / static_cast<qreal>(q_pointer->size().width());
        } else {
            temp = (q_pointer->size().width() - windowCoordinates.x())
                / static_cast<qreal>(q_pointer->size().width());
        }
    }
    return qBound<qreal>(0, temp, 1);
}

void GradientSlider::mousePressEvent(QMouseEvent* event)
{
    setValue(
        d_pointer->fromWindowCoordinatesToValue(
            event->pos()
        )
    );
}

void GradientSlider::mouseReleaseEvent(QMouseEvent* event)
{
    setValue(
        d_pointer->fromWindowCoordinatesToValue(
            event->pos()
        )
    );
}

void GradientSlider::mouseMoveEvent(QMouseEvent* event)
{
    setValue(
        d_pointer->fromWindowCoordinatesToValue(
            event->pos()
        )
    );
}

qreal GradientSlider::value()
{
    return d_pointer->m_value;
}

void GradientSlider::setValue(qreal newValue)
{
    qreal temp = qBound<qreal>(0, newValue, 1);
    if (d_pointer->m_value != temp) {
        d_pointer->m_value = temp;
        update();
        Q_EMIT valueChanged(temp);
    }
}

void GradientSlider::wheelEvent(QWheelEvent* event)
{
    qreal steps = standardWheelSteps(event);
    //  Only react on good old vertical wheels, and not on horizontal wheels
    if (steps != 0) {
        qreal stepSize;
        if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)
            || QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)
        ) {
            stepSize = pageStep();
        } else {
            stepSize = singleStep();
        };
        setValue(d_pointer->m_value + steps * stepSize);
    } else {
        // Don’t accept the event and let it up to the default treatment:
        event->ignore();
    }
}

void GradientSlider::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Plus:
            setValue(d_pointer->m_value + d_pointer->m_singleStep);
            break;
        case Qt::Key_Down:
        case Qt::Key_Minus:
            setValue(d_pointer->m_value - d_pointer->m_singleStep);
            break;
        case Qt::Key_Left:
            if (layoutDirection() == Qt::LayoutDirection::LeftToRight) {
                setValue(d_pointer->m_value - d_pointer->m_singleStep);
            } else {
                setValue(d_pointer->m_value + d_pointer->m_singleStep);
            }
            break;
        case Qt::Key_Right:
            if (layoutDirection() == Qt::LayoutDirection::LeftToRight) {
                setValue(d_pointer->m_value + d_pointer->m_singleStep);
            } else {
                setValue(d_pointer->m_value - d_pointer->m_singleStep);
            }
            break;
        case Qt::Key_PageUp:
            setValue(d_pointer->m_value + d_pointer->m_pageStep);
            break;
        case Qt::Key_PageDown:
            setValue(d_pointer->m_value - d_pointer->m_pageStep);
            break;
        case Qt::Key_Home:
            setValue(0);
            break;
        case Qt::Key_End:
            setValue(1);
            break;
        default:
            /* Quote from Qt documentation:
             *
             *     “If you reimplement this handler, it is very important that
             *      you call the base class implementation if you do not act
             *      upon the key.
             *
             *      The default implementation closes popup widgets if the
             *      user presses the key sequence for QKeySequence::Cancel
             *      (typically the Escape key). Otherwise the event is
             *      ignored, so that the widget's parent can interpret it.“ */
            QWidget::keyPressEvent(event);
    }
}

qreal GradientSlider::singleStep()
{
    return d_pointer->m_singleStep;
}

qreal GradientSlider::pageStep()
{
    return d_pointer->m_pageStep;
}

void GradientSlider::setSingleStep(qreal newSingleStep)
{
    if (newSingleStep != d_pointer->m_singleStep) {
        d_pointer->m_singleStep = newSingleStep;
        Q_EMIT singleStepChanged(d_pointer->m_singleStep);
    }
}

void GradientSlider::setPageStep(qreal newPageStep)
{
    if (newPageStep != d_pointer->m_pageStep) {
        d_pointer->m_pageStep = newPageStep;
        Q_EMIT pageStepChanged(d_pointer->m_pageStep);
    }
}

// TODO It would be better to have an arrow outside the slider. This
// could be conform with the current QStyle, and would guarantee
// a consistent contrast between the arrow and its background.
// TODO When zoom factor is 1,25, then background scaling is 1,25².
// TODO Draw a focus rectangle like this?:
//     if (hasFocus()) {
//         QStyleOptionFocusRect opt;
//         opt.palette = palette();
//         opt.rect = rect;
//         opt.state = QStyle::State_None | QStyle::State_KeyboardFocusChange;
//         style()->drawPrimitive(
//             QStyle::PE_FrameFocusRect,
//             &opt,
//             &widgetPainter,
//             this
//         );
//     }
void GradientSlider::paintEvent(QPaintEvent* event)
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

    painter.setTransform(d_pointer->getTransform());
    if (!d_pointer->m_gradientImageReady) {
        d_pointer->updateGradientImage();
    }
    painter.drawImage(0, 0, d_pointer->m_gradientImage);


    int actualLength;
    if (d_pointer->m_orientation == Qt::Orientation::Vertical) {
        actualLength = size().height();
    } else {
        actualLength = size().width();
    }

    QPolygonF arrowPolygon;
    const qreal cursorPosition = actualLength * d_pointer->m_value;
    const qreal arrowSize = 6;
    arrowPolygon
        << QPointF(cursorPosition, arrowSize)
        << QPointF(cursorPosition + arrowSize, 0)
        << QPointF(cursorPosition - arrowSize, 0);
    painter.setBrush(QBrush(Qt::black));
    QPen pen(Qt::transparent);
    pen.setWidth(0);
    painter.setPen(pen);
    painter.drawPolygon(arrowPolygon);
    arrowPolygon = QPolygonF(); // re-initialize
    arrowPolygon
        << QPointF(cursorPosition, d_pointer-> m_gradientThickness - arrowSize)
        << QPointF(cursorPosition + arrowSize, d_pointer->m_gradientThickness)
        << QPointF(cursorPosition - arrowSize, d_pointer->m_gradientThickness);
    painter.setBrush(QBrush(Qt::white));
    painter.drawPolygon(arrowPolygon);
    if (hasFocus()) {
        pen.setWidth(2);
        pen.setColor(
            focusIndicatorColor()
        );
        painter.setPen(pen);
        painter.drawLine(
            qRound(cursorPosition + arrowSize + 1),
            0,
            qRound(cursorPosition + arrowSize + 1),
            d_pointer->m_gradientThickness
        );
        painter.drawLine(
            qRound(cursorPosition - arrowSize),
            0,
            qRound(cursorPosition - arrowSize),
            d_pointer->m_gradientThickness
        );
    }

    // Paint the buffer to the actual widget
    QPainter(this).drawImage(0, 0, paintBuffer);
}

Qt::Orientation	GradientSlider::orientation() const
{
    return d_pointer->m_orientation;
}

void GradientSlider::GradientSliderPrivate::setOrientationAndForceUpdate(
    Qt::Orientation newOrientation
)
{
    if (newOrientation == Qt::Orientation::Vertical) {
        q_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    } else {
        q_pointer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    m_orientation = newOrientation;
    m_gradientImageReady = false;
    // Notify the layout system the the geometry has changed
    q_pointer->updateGeometry();
}

void GradientSlider::setOrientation(Qt::Orientation newOrientation)
{
    if (newOrientation != d_pointer->m_orientation) {
        d_pointer->setOrientationAndForceUpdate(newOrientation);
        Q_EMIT orientationChanged(d_pointer->m_orientation);
    }
}

void GradientSlider::setColors(
    const PerceptualColor::LchaDouble &newFirstColor,
    const PerceptualColor::LchaDouble &newSecondColor
)
{
    if (
        (d_pointer->m_firstColor.hasSameCoordinates(newFirstColor))
            && (d_pointer->m_secondColor.hasSameCoordinates(newSecondColor))
    ) {
        return;
    }
    d_pointer->m_firstColor = newFirstColor;
    d_pointer->m_secondColor = newSecondColor;
    d_pointer->m_gradientImageReady = false;
    update();
}

void GradientSlider::setFirstColor(
    const PerceptualColor::LchaDouble &newFirstColor
)
{
    setColors(newFirstColor, d_pointer->m_secondColor);
}

void GradientSlider::setSecondColor(
    const PerceptualColor::LchaDouble &newSecondColor
)
{
    setColors(d_pointer->m_firstColor, newSecondColor);
}

/** @returns LchaDouble for intermediate color, and its corresponding alpha
 * value. */
QPair<LchDouble, qreal> GradientSlider
    ::GradientSliderPrivate
    ::intermediateColor
(
    const LchaDouble &firstColor,
    const LchaDouble &secondColor,
    qreal value
)
{
    LchDouble color;
    color.l = firstColor.l + (secondColor.l - firstColor.l) * value;
    color.c = firstColor.c + (secondColor.c - firstColor.c) * value;
    color.h = firstColor.h + (secondColor.h - firstColor.h) * value;
    qreal alpha = m_firstColor.a
        + (m_secondColor.a - m_firstColor.a) * value;
    return QPair<LchDouble, qreal>(color, alpha);
}

QTransform GradientSlider::GradientSliderPrivate::getTransform() const
{
    QTransform transform;
    if (m_orientation == Qt::Orientation::Vertical) {
        transform.translate(0, q_pointer->size().height());
        transform.rotate(270);
    } else {
        if (q_pointer->layoutDirection() == Qt::RightToLeft) {
            transform.translate(q_pointer->size().width(), 0);
            transform.scale(-1, 1);
        }
    }
    return transform;
}

void GradientSlider::GradientSliderPrivate::updateGradientImage()
{
    int actualLength;
    if (m_orientation == Qt::Orientation::Vertical) {
        actualLength = q_pointer->size().height();
    } else {
        actualLength = q_pointer->size().width();
    }
    QImage temp(actualLength, 1, QImage::Format_ARGB32_Premultiplied);
    temp.fill(Qt::transparent); // Initialize the image with transparency.
    LchaDouble firstColor = m_firstColor;
    LchaDouble secondColor = m_secondColor;
    if (qAbs(firstColor.h - secondColor.h) > 180) {
        if (firstColor.h > secondColor.h) {
            secondColor.h += 360;
        } else {
            secondColor.h -= 360;
        }
    }
    QPair<LchDouble, qreal> color;
    FullColorDescription fullColor;
    for (int i = 0; i < actualLength; ++i) {
        color = intermediateColor(
            firstColor,
            secondColor,
            i / static_cast<qreal>(actualLength)
        );
        // TODO Assert that this actually makes sure also out-of-gamut
        // colors are converted to nearby in-gamut colors.
        temp.setPixelColor(
            i,
            0,
            FullColorDescription(
                m_rgbColorSpace,
                color.first,
                FullColorDescription::OutOfGamutBehaviour::preserve,
                color.second
            ).toRgbQColor()
        );
    }
    QImage result = QImage(
        actualLength,
        m_gradientThickness,
        QImage::Format_ARGB32_Premultiplied
    );
    QPainter painter(&result);
    painter.fillRect(
        0,
        0,
        actualLength,
        m_gradientThickness,
        QBrush(q_pointer->transparencyBackground())
    );
    for (int i = 0; i < m_gradientThickness; ++i) {
        painter.drawImage(0, i, temp);
    }
    m_gradientImage = result;
    m_gradientImageReady = true;
}

void GradientSlider::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    d_pointer->m_gradientImageReady = false;
}

}
