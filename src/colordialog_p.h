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

#ifndef COLORDIALOG_P_H
#define COLORDIALOG_P_H

// Include the header of the public class of this private implementation.
#include "PerceptualColor/colordialog.h"

#include "constpropagatingrawpointer.h"

#include "PerceptualColor/chromahuediagram.h"
#include "PerceptualColor/colorpatch.h"
#include "PerceptualColor/gradientslider.h"
#include "PerceptualColor/multispinbox.h"
#include "PerceptualColor/wheelcolorpicker.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QTabWidget>

namespace PerceptualColor
{
/** @internal
 *
 *  @brief Private implementation within the <em>Pointer to
 *  implementation</em> idiom */
class ColorDialog::ColorDialogPrivate final
{
public:
    ColorDialogPrivate(ColorDialog *backLink);
    /** @brief Default destructor
     *
     * The destructor is non-<tt>virtual</tt> because
     * the class as a whole is <tt>final</tt>. */
    ~ColorDialogPrivate() noexcept = default;

    /** @brief @ref GradientSlider widget for the alpha channel. */
    QPointer<GradientSlider> m_alphaGradientSlider;
    /** @brief Pointer to the QLabel for the alpha value.
     *
     * We store this in a pointer to allow toggle the visibility later. */
    QPointer<QLabel> m_alphaLabel;
    /** @brief Spin box for the alpha channel.
     *
     * This spin box shows always the value of @ref m_alphaGradientSlider.
     *
     * @note It’s value is not set directly, but is updated via signals from
     * @ref m_alphaGradientSlider. Do not use it directly! */
    QPointer<QDoubleSpinBox> m_alphaSpinBox;
    /** @brief Pointer to the QButtonBox of this dialog.
     *
     * We store this in a pointer
     * to allow toggle the visibility later. */
    QPointer<QDialogButtonBox> m_buttonBox;
    /** @brief Pointer to the @ref ChromaHueDiagram. */
    QPointer<ChromaHueDiagram> m_chromaHueDiagram;
    /** @brief Pointer to the @ref ColorPatch widget. */
    QPointer<ColorPatch> m_colorPatch;
    /** @brief Holds the current color without alpha information
     *
     * @note The alpha information within this data member is meaningless.
     * Ignore it. The information about the alpha channel is actually stored
     * within @ref m_alphaGradientSlider.
     *
     * @sa @ref currentColor() */
    LchDouble m_currentOpaqueColor;
    /** @brief Pointer to the @ref GradientSlider for LCh lightness. */
    QPointer<GradientSlider> m_lchLightnessSelector;
    /** @brief Pointer to the @ref MultiSpinBox for HLC. */
    QPointer<MultiSpinBox> m_hlcSpinBox;
    /** @brief Pointer to the @ref MultiSpinBox for HSV. */
    QPointer<MultiSpinBox> m_hsvSpinBox;
    /** @brief Holds whether currently a color change is ongoing, or not.
     *
     * Used to avoid infinite recursions when updating the different widgets
     * within this dialog.
     * @sa @ref setCurrentOpaqueColor() */
    bool m_isColorChangeInProgress = false;
    /** @brief Internal storage for property @ref layoutDimensions */
    PerceptualColor::ColorDialog::DialogLayoutDimensions m_layoutDimensions = ColorDialog::DialogLayoutDimensions::collapsed;
    /** @brief Pointer to the graphical selector widget that groups lightness
     *  and chroma-hue selector. */
    QPointer<QWidget> m_lightnessFirstWidget;
    /** @brief Holds the receiver slot (if any) to be disconnected
     *  automatically after closing the dialog.
     *
     * Its value is only meaningful if
     * @ref m_receiverToBeDisconnected is not null.
     * @sa @ref m_receiverToBeDisconnected
     * @sa @ref open() */
    QByteArray m_memberToBeDisconnected;
    /** @brief Pointer to the widget that holds the numeric color
     *         representation. */
    QPointer<QWidget> m_numericalWidget;
    /** @brief Holds the receiver object (if any) to be disconnected
     *  automatically after closing the dialog.
     *
     * @sa @ref m_memberToBeDisconnected
     * @sa @ref open() */
    QPointer<QObject> m_receiverToBeDisconnected;
    /** @brief Internal storage for property @ref options */
    ColorDialogOptions m_options;
    /** @brief Pointer to the RgbColorSpace object. */
    QSharedPointer<RgbColorSpace> m_rgbColorSpace;
    /** @brief Pointer to the QLineEdit that represents the hexadecimal
     *  RGB value. */
    QPointer<QLineEdit> m_rgbLineEdit;
    /** @brief Pointer to the @ref MultiSpinBox for RGB. */
    QPointer<MultiSpinBox> m_rgbSpinBox;
    /** @brief Internal storage for selectedColor(). */
    QColor m_selectedColor;
    /** @brief Layout that holds the graphical and numeric selectors. */
    QPointer<QHBoxLayout> m_selectorLayout;
    /** @brief Pointer to the tab widget. */
    QPointer<QTabWidget> m_tabWidget;
    /** @brief Pointer to the @ref WheelColorPicker widget. */
    QPointer<WheelColorPicker> m_wheelColorPicker;

    void applyLayoutDimensions();
    void initialize();
    QWidget *initializeNumericPage();
    void setCurrentFullColor(const LchaDouble &color);

public Q_SLOTS:
    void readHlcNumericValues();
    void readHsvNumericValues();
    void readLightnessValue();
    void readRgbHexValues();
    void readRgbNumericValues();
    void setCurrentOpaqueColor(const PerceptualColor::LchDouble &color);
    void setCurrentOpaqueQColor(const QColor &color);
    void updateColorPatch();

private:
    Q_DISABLE_COPY(ColorDialogPrivate)

    /** @brief Pointer to the object from which <em>this</em> object
     *  is the private implementation. */
    ConstPropagatingRawPointer<ColorDialog> q_pointer;
};

} // namespace PerceptualColor

#endif // COLORDIALOG_P_H
