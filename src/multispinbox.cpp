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
#include "PerceptualColor/multispinbox.h"
// Second, the private implementation.
#include "multispinbox_p.h"

#include "extendeddoublevalidator.h"
#include "helper.h"

#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOption>
#include <QtMath>

namespace PerceptualColor {

/** @brief Test if a cursor position is at the current value.
 *
 * Everything from the cursor position exactly before the value itself up
 * to the cursor position exactly after the value itself. Example: “ab12cd”
 * (prefix “ab”, value 12, suffix “cd”). The cursor positions 2, 3 and 4 are
 * <em>at</em> the current value.
 *
 * @param cursorPosition the cursor position to test
 *
 * @returns <tt>true</tt> if the indicated cursor position is at the
 * <em>value</em> text of the current section. <tt>false</tt> otherwise. */
bool MultiSpinBox::MultiSpinBoxPrivate::isCursorPositionAtCurrentSectionValue(
    const int cursorPosition
) const
{
    const bool newPositionIsHighEnough = (
        cursorPosition >= m_textBeforeCurrentValue.length()
    );
    const bool newPositionIsLowEnough = (
        cursorPosition <= (
            q_pointer->lineEdit()->text().length()
                - m_textAfterCurrentValue.length()
        )
    );
    return (newPositionIsHighEnough && newPositionIsLowEnough);
}

/** @brief The recommended minimum size for the widget
 *
 * Reimplemented from base class.
 *
 * @note The minimum size of the widget is the same as @ref sizeHint(). This
 * behavior is different from <tt>QSpinBox</tt> and <tt>QDoubleSpinBox</tt>
 * that have a minimum size hint that allows for displaying only prefix and
 * value, but not the suffix. However, such a behavior does not seem
 * appropriate for a @ref MultiSpinBox because it could be confusing, given
 * that its content is more complex.
 *
 * @returns the recommended minimum size for the widget */
QSize MultiSpinBox::minimumSizeHint() const
{
    return sizeHint();
}

/** @brief Constructor
 *
 * @param parent the parent widget, if any */
MultiSpinBox::MultiSpinBox(QWidget *parent) :
    QAbstractSpinBox(parent),
    d_pointer(new MultiSpinBoxPrivate(this))
{
    // Set up the m_validator
    d_pointer->m_validator = new ExtendedDoubleValidator(this);
    d_pointer->m_validator->setLocale(locale());
    lineEdit()->setValidator(d_pointer->m_validator);

    // Connect signals and slots
    connect(
        lineEdit(),
        &QLineEdit::textChanged,
        this,
        [this](const QString &lineEditText) {
            d_pointer->updateCurrentValueFromText(lineEditText);
        }
    );
    connect(
        lineEdit(),
        &QLineEdit::cursorPositionChanged,
        this,
        [this](const int oldPos, const int newPos) {
            d_pointer->reactOnCursorPositionChange(oldPos, newPos);
        }
    );
    connect(
        this,
        &QAbstractSpinBox::editingFinished,
        this,
        [this]() {
            d_pointer->setCurrentIndexToZeroAndUpdateTextAndSelectValue();
        }
    );

    // Initialize the configuration (default: only one section)
    setSections(
        QList<SectionData>{SectionData()}
    );
    d_pointer->m_currentIndex = -1; // This will force
    // setCurrentIndexAndUpdateTextAndSelectValue()
    // to really apply the changes, including updating
    // the validator.
    d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(0);
}

/** @brief Default destructor */
MultiSpinBox::~MultiSpinBox() noexcept
{
}

/** @brief Constructor
 *
 * @param backLink Pointer to the object from which <em>this</em> object
 * is the private implementation. */
MultiSpinBox::MultiSpinBoxPrivate::MultiSpinBoxPrivate(
    MultiSpinBox *backLink
) : q_pointer(backLink)
{
}

/** @brief The recommended size for the widget
 *
 * Reimplemented from base class.
 *
 * @returns the size hint
 * @sa @ref minimumSizeHint() */
QSize MultiSpinBox::sizeHint() const
{
    // This function intentionally does not cache the text string.
    // Which variant is the longest text string, that depends on the current
    // font policy. This might have changed since the last call. Therefore,
    // each time this functin is called, we calculate again the longest
    // test string (“completeString”).

    ensurePolished();

    const QFontMetrics myFontMetrics(fontMetrics());
    QList<MultiSpinBox::SectionData> myConfiguration = d_pointer->m_sections;
    int height = lineEdit()->sizeHint().height();
    int width = 0;
    QString textOfMinimumValue;
    QString textOfMaximumValue;
    QString completeString;

    // Get the text for all the sections
    for (int i = 0; i < myConfiguration.count(); ++i) {
        // Prefix
        completeString += myConfiguration.at(i).prefix;
        // For each section, test if the minimum value or the maximum
        // takes more space (width). Choose the one that takes more place
        // (width).
        textOfMinimumValue = locale().toString(
            myConfiguration.at(i).minimum,
            'f',
            myConfiguration.at(i).decimals
        );
        textOfMaximumValue = locale().toString(
            myConfiguration.at(i).maximum,
            'f',
            myConfiguration.at(i).decimals
        );
        if (
            myFontMetrics.horizontalAdvance(textOfMinimumValue)
                > myFontMetrics.horizontalAdvance(textOfMaximumValue)
        ) {
            completeString += textOfMinimumValue;
        } else {
            completeString += textOfMaximumValue;
        }
        // Suffix
        completeString += myConfiguration.at(i).suffix;
    }

    // Add some extra space, just as QSpinBox seems to do also.
    completeString += QStringLiteral(u" ");

    // Calculate string width and add two extra pixel for cursor
    // blinking space.
    width = myFontMetrics.horizontalAdvance(completeString) + 2;

    // Calculate the final size in pixel
    QStyleOptionSpinBox myStyleOptionsForSpinBoxes;
    initStyleOption(&myStyleOptionsForSpinBoxes);
    QSize contentSize(width, height);
    QSize result = style()->sizeFromContents(
        QStyle::CT_SpinBox,
        &myStyleOptionsForSpinBoxes,
        contentSize,
        this
    ).expandedTo(QApplication::globalStrut());

    if (d_pointer->m_actionButtonCount > 0) {
        // Determine the size of icons for actions similar to what Qt
        // does in QLineEditPrivate::sideWidgetParameters() and than
        // add this to the size hint.
        //
        // This code leads in general to good results. However, some
        // styles like CDE and Motif calculate badly the size for
        // QAbstractSpinBox and therefore also for this widget.
        //
        // The Kvantum style has a strange behaviour: While behaving
        // correctly on QSpinBox, it does not behave correctly on small
        // widget sizes: On small widget sizes, style()->sizeFromContents()
        // returns a value that is too small, but the actual size of
        // the widget in the layout system will be bigger than the
        // reported size, so this looks okay. On big sizes however,
        // the actual size in the layout system is identical to the
        // reported size, which is too small. For these styles,
        // there seems to be no good work-around. These very same
        // styles also have problems with normal QSpinBox widgets and
        // cut some of the text there, so I suppose this is rather a
        // problem of these styles, and not of our code.
        //
        // TODO That‘s wrong. Normal spinboxes work fine in all CDE,
        // Motif and Kvantum styles. It should work also for MultiSpinBox!
        const int actionButtonIconSize = style()->pixelMetric(
            QStyle::PM_SmallIconSize,
            nullptr,
            lineEdit()
        );
        const int actionButtonMargin = actionButtonIconSize / 4;
        const int actionButtonWidth = actionButtonIconSize + 6;
        const int actionButtonSpace =
            actionButtonWidth + actionButtonMargin; // Only 1 margin per button
        result.setWidth(
            result.width() + d_pointer->m_actionButtonCount * actionButtonSpace
        );
    }

    return result;
}

/** @brief Adds to the widget a button associated with the given action.
 *
 * The icon of the action will be displayed as button. If the action has
 * no icon, just an empty space will be displayed.
 *
 * It is possible to add more than one action.
 *
 * @param action This action that will be executed when clicking the button.
 * (The parentship of the action object remains unchanged.)
 * @param position The position of the button within the widget (left
 * or right)
 * @note See @ref hidpisupport "High DPI support" about how to enable
 * support for high-DPI icons.
 * @note The action will <em>not</em> appear in
 * <tt>QWidget::actions()</tt>. */
void MultiSpinBox::addActionButton(
    QAction *action,
    QLineEdit::ActionPosition position
)
{
    lineEdit()->addAction(action, position);
    d_pointer->m_actionButtonCount += 1;
    // The size hints have changed, because an additional button needs
    // more space.
    updateGeometry();
}

/** @brief Formats the value of a given section.
 *
 * @param mySection the section that will be formatted
 * @returns the value, formatted (without prefix or suffix), as text */
QString MultiSpinBox::MultiSpinBoxPrivate::formattedValue(
    const SectionData &mySection
) const {
    return q_pointer->locale().toString(
        mySection.value,   // the value to be formatted
        'f',               // format as floating point with decimal digits
        mySection.decimals // number of decimal digits
    );
}

/** @brief Updates prefix, value and suffix text
 *
 * Updates @ref m_textBeforeCurrentValue,
 * @ref m_textOfCurrentValue,
 * @ref m_textAfterCurrentValue
 * to the correct values based on @ref m_currentIndex. */
void MultiSpinBox::MultiSpinBoxPrivate::updatePrefixValueSuffixText()
{
    int i;

    // Update m_currentSectionTextBeforeValue
    m_textBeforeCurrentValue = QString();
    for (i = 0; i < m_currentIndex; ++i) {
        m_textBeforeCurrentValue.append( m_sections.at(i).prefix);
        m_textBeforeCurrentValue.append(
            formattedValue( m_sections.at(i))
        );
        m_textBeforeCurrentValue.append( m_sections.at(i).suffix);
    }
    m_textBeforeCurrentValue.append(
        m_sections.at(m_currentIndex).prefix
    );

    // Update m_currentSectionTextOfTheValue
    m_textOfCurrentValue = formattedValue(
                                         m_sections.at(m_currentIndex)
    );

    // Update m_currentSectionTextAfterValue
    m_textAfterCurrentValue = QString();
    m_textAfterCurrentValue.append(
        m_sections.at(m_currentIndex).suffix
    );
    for (i = m_currentIndex + 1; i < m_sections.count(); ++i) {
        m_textAfterCurrentValue.append( m_sections.at(i).prefix);

        m_textAfterCurrentValue.append(
            formattedValue(m_sections.at(i))
        );
        m_textAfterCurrentValue.append( m_sections.at(i).suffix);
    }
}

/** @brief Sets the current section index to <tt>0</tt>.
 *
 * Convenience function that simply calls
 * @ref setCurrentIndexAndUpdateTextAndSelectValue with the
 * argument <tt>0</tt>. */
void MultiSpinBox
    ::MultiSpinBoxPrivate
    ::setCurrentIndexToZeroAndUpdateTextAndSelectValue()
{
    setCurrentIndexAndUpdateTextAndSelectValue(0);
}

/** @brief Sets the current section index.
 *
 * Updates the text in the QLineEdit of this widget. If the widget has focus,
 * it also selects the value of the new current section.
 *
 * @param newIndex The index of the new current section. Must be a valid index.
 * Default is <tt>0</tt> (which is always valid as @ref m_sections is
 * guaranteed to contain at least <em>one</em> section). The update will
 * be done even if this argument is identical to the @ref m_currentIndex.
 *
 * @sa @ref setCurrentIndexToZeroAndUpdateTextAndSelectValue
 * @sa @ref setCurrentIndexWithoutUpdatingText */
void MultiSpinBox
    ::MultiSpinBoxPrivate
    ::setCurrentIndexAndUpdateTextAndSelectValue(
    int newIndex
)
{
    QSignalBlocker myBlocker(q_pointer->lineEdit());
    setCurrentIndexWithoutUpdatingText(newIndex);
    // Update the line edit widget
    q_pointer->lineEdit()->setText(
        m_textBeforeCurrentValue
            + m_textOfCurrentValue
            + m_textAfterCurrentValue
    );
    if (q_pointer->hasFocus()) {
        q_pointer->lineEdit()->setSelection(
            m_textBeforeCurrentValue.length(),
            m_textOfCurrentValue.length()
        );
    } else {
        q_pointer->lineEdit()->setCursorPosition(
            m_textBeforeCurrentValue.length()
                + m_textOfCurrentValue.length()
        );
    };
    // Make sure that the buttons for step up and step down are updated.
    q_pointer->update();
}

/** @brief Sets the current section index without updating
 * the <tt>QLineEdit</tt>.
 *
 * Does not change neither the text nor the cursor in the <tt>QLineEdit</tt>.
 *
 * @param newIndex The index of the new current section. Must be a valid index.
 *
 * @sa @ref setCurrentIndexAndUpdateTextAndSelectValue */
void MultiSpinBox::MultiSpinBoxPrivate::setCurrentIndexWithoutUpdatingText(
    int newIndex
)
{
    if (!inRange(0, newIndex, m_sections.count() - 1)) {
        qWarning()
            << "The function"
            << __func__
            << "in file"
            << __FILE__
            << "near to line"
            << __LINE__
            << "was called with an invalid “newIndex“ argument of"
            << newIndex
            << "thought the valid range is currently ["
            << 0
            << ", "
            << m_sections.count() - 1
            << "]. This is a bug.";
        throw 0;
    }

    if (newIndex == m_currentIndex) {
        // There is nothing to do here.
        return;
    }

    // Apply the changes
    m_currentIndex = newIndex;
    updatePrefixValueSuffixText();
    m_validator->setPrefix(m_textBeforeCurrentValue);
    m_validator->setSuffix(m_textAfterCurrentValue);
    m_validator->setRange(
        m_sections.at(m_currentIndex).minimum,
        m_sections.at(m_currentIndex).maximum
    );

    // The state (enabled/disabled) of the buttons “Step up” and “Step down”
    // has to be updated. To force this, update() is called manually here:
    q_pointer->update();
}

/** @brief Virtual function that determines whether stepping up and down is
 * legal at any given time.
 *
 * Reimplemented from base class.
 *
 * @returns whether stepping up and down is legal */
QAbstractSpinBox::StepEnabled MultiSpinBox::stepEnabled() const
{
    const SectionData currentSection = d_pointer->m_sections.at(
        d_pointer->m_currentIndex
    );

    // When wrapping is enabled, step up and step down are always possible.
    if (currentSection.isWrapping) {
        return QAbstractSpinBox::StepEnabled(StepUpEnabled | StepDownEnabled);
    }

    // When wrapping is not enabled, we have to compare the value with
    // maximum and minimum.
    QAbstractSpinBox::StepEnabled result;
    // Test is step up should be enabled…
    if (currentSection.value < currentSection.maximum) {
        result.setFlag(StepUpEnabled, true);
    }

    // Test is step down should be enabled…
    if (currentSection.value > currentSection.minimum) {
        result.setFlag(StepDownEnabled, true);
    }
    return result;
}

/** @brief Get fixed section data
 *
 * @param section the original section data
 * @returns A copy of this section data, with @ref SectionData.value fixed
 * to be conform to @ref SectionData.minimum, @ref SectionData.maximum and
 * @ref SectionData.isWrapping. */
MultiSpinBox::SectionData MultiSpinBox::MultiSpinBoxPrivate::fixedSection(
    const MultiSpinBox::SectionData &section
)
{
    MultiSpinBox::SectionData result = section;
    if (result.isWrapping) {
        double rangeWidth = result.maximum - result.minimum;
        if (rangeWidth <= 0) {
            // This is a speciel case.
            // This happens when minimum == maximum (or if minimum > maximum,
            // which is invalid).
            result.value = result.minimum;
        } else {
            qreal temp = fmod(
                result.value - result.minimum,
                rangeWidth
            );
            if (temp < 0) {
                temp += rangeWidth;
            }
            result.value = temp + result.minimum;
        }
    } else {
        result.value = qBound(result.minimum, result.value, result.maximum);
    }
    return result;
}

/** @brief Adds QDebug() support for this data type. */
QDebug operator<<(
    QDebug dbg,
    const PerceptualColor::MultiSpinBox::SectionData &value
)
{
    dbg.nospace()
        << "\nMultiSpinBox::SectionData(\n    prefix: "
        << value.prefix
        << "\n    minimum: "
        << value.minimum
        << "\n    value: "
        << value.value
        << "\n    decimals: "
        << value.decimals
        << "\n    isWrapping: "
        << value.isWrapping
        << "\n    maximum: "
        << value.maximum
        << "\n    suffix: "
        << value.suffix
        << "\n)";
    return dbg.maybeSpace();
}

/** @brief Sets the data for the sections.
 *
 * @post The old data for the sections is completly destroyed. The new
 * data is used now.
 *
 * The first section will be selected as current section.
 *
 * @param newSections The new sections. If this list is empty, the function
 * call will be ignored. Each section should have valid
 * values: <tt>@ref SectionData.minimum ≤ @ref SectionData.value ≤
 * @ref SectionData.maximum </tt> If the values are not valid, automatically
 * fixed section data will be used.
 *
 * @sa @ref sections()
 * @sa @ref MultiSpinBoxPrivate::m_sections */
void MultiSpinBox::setSections(
    const QList<MultiSpinBox::SectionData> &newSections
)
{
    if (newSections.count() < 1) {
        return;
    }

    d_pointer->m_sections.clear();

    // Make sure the new SectionData is valid (minimum <= value <= maximum)
    // before adding it.
    SectionData tempSection;
    for (int i = 0; i < newSections.count(); ++i) {
        tempSection = newSections.at(i);
        if (tempSection.maximum < tempSection.minimum) {
            tempSection.maximum = tempSection.minimum;
        }
        d_pointer->m_sections.append(
            d_pointer->fixedSection(tempSection)
        );
    }
    d_pointer->updatePrefixValueSuffixText();
    lineEdit()->setText(
        d_pointer->m_textBeforeCurrentValue
            + d_pointer->m_textOfCurrentValue
            + d_pointer->m_textAfterCurrentValue
    );
    d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(0);

    // Make sure that the buttons for step up and step down are updated.
    update();
}

/** @brief Returns the data of all sections.
 *
 * @returns the data of all sections.
 *
 * @sa @ref setSections()
 * @sa @ref MultiSpinBoxPrivate::m_sections */
QList<MultiSpinBox::SectionData> MultiSpinBox::sections() const
{
    return d_pointer->m_sections;
}

/** @brief Focus handling for <em>Tab</em> respectively <em>Shift+Tab</em>.
 *
 * Reimplemented from base class.
 *
 * @note If it’s about moving the focus <em>within</em> this widget, the focus
 * move is actually done. If it’s about moving the focus to <em>another</em>
 * widget, the focus move is <em>not</em> actually done.
 * The documentation of the base class is not very detailed. This
 * reimplementation does not exactly behave as the documentation of the
 * base class suggests. Especially, it handles directly the focus move
 * <em>within</em> the widget itself. This was, however, the only working
 * solution we found.
 *
 * @param next <tt>true</tt> stands for focus handling for <em>Tab</em>.
 * <tt>false</tt> stands for focus handling for <em>Shift+Tab</em>.
 *
 * @returns <tt>true</tt> if the focus has actually been moved within
 * this widget or if a move to another widget is possible. <tt>false</tt>
 * otherwise. */
bool MultiSpinBox::focusNextPrevChild(bool next)
{
    if (next == true) { // Move focus forward (Tab)
        if (d_pointer->m_currentIndex < (d_pointer->m_sections.count() - 1)) {
            d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(
                d_pointer->m_currentIndex + 1
            );
            // Make sure that the buttons for step up and step down
            // are updated.
            update();
            return true;
        }
    } else { // Move focus backward (Shift+Tab)
        if (d_pointer->m_currentIndex > 0) {
            d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(
                d_pointer->m_currentIndex - 1
            );
            // Make sure that the buttons for step up and step down
            // are updated.
            update();
            return true;
        }
    }

    // Make sure that the buttons for step up and step down are updated.
    update();

    // Return
    return QWidget::focusNextPrevChild(next);
}

/** @brief Handles a <tt>QEvent::FocusOut</tt>.
 *
 * Reimplemented from base class.
 *
 * Updates the widget (except for windows that do not
 * specify a <tt>focusPolicy()</tt>).
 *
 * @param event the <tt>QEvent::FocusOut</tt> to be handled. */
void MultiSpinBox::focusOutEvent(QFocusEvent* event)
{
    QAbstractSpinBox::focusOutEvent(event);
    switch (event->reason()) {
        case Qt::ShortcutFocusReason:
        case Qt::TabFocusReason:
        case Qt::BacktabFocusReason:
        case Qt::MouseFocusReason:
            d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(0);
            // Make sure that the buttons for step up and step down
            // are updated.
            update();
            return;
        case Qt::ActiveWindowFocusReason:
        case Qt::PopupFocusReason:
        case Qt::MenuBarFocusReason:
        case Qt::OtherFocusReason:
        case Qt::NoFocusReason:
        default:
            update();
            return;
    }
}

/** @brief Handles a <tt>QEvent::FocusIn</tt>.
 *
 * Reimplemented from base class.
 *
 * Updates the widget (except for windows that do not
 * specify a <tt>focusPolicy()</tt>).
 *
 * @param event the <tt>QEvent::FocusIn</tt> to be handled. */
void MultiSpinBox::focusInEvent(QFocusEvent* event)
{
    QAbstractSpinBox::focusInEvent(event);
    switch (event->reason()) {
        case Qt::ShortcutFocusReason:
        case Qt::TabFocusReason:
            d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(0);
            // Make sure that the buttons for step up and step down
            // are updated.
            update();
            return;
        case Qt::BacktabFocusReason:
            d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(
                d_pointer->m_sections.count() - 1
            );
            // Make sure that the buttons for step up and step down
            // are updated.
            update();
            return;
        case Qt::MouseFocusReason:
        case Qt::ActiveWindowFocusReason:
        case Qt::PopupFocusReason:
        case Qt::MenuBarFocusReason:
        case Qt::OtherFocusReason:
        case Qt::NoFocusReason:
        default:
            update();
            return;
    }
}

/** @brief Increase or decrese the current section’s value.
 *
 * Reimplemented from base class.
 *
 * As of the base class’s documentation:
 *
 * > Virtual function that is called whenever the user triggers a step.
 * > For example, pressing <tt>Qt::Key_Down</tt> will trigger a call
 * > to <tt>stepBy(-1)</tt>, whereas pressing <tt>Qt::Key_PageUp</tt> will
 * > trigger a call to <tt>stepBy(10)</tt>.
 *
 * The step size in this function is <em>always</em> <tt>1</tt>. Therefore,
 * calling <tt>stepBy(1)</tt> will increase the current section’s value
 * by <tt>1</tt>; no additional factor is applied.
 *
 * @param steps The <em>steps</em> parameter indicates how many steps were
 * taken. A positive step count increases the value, a negative step count
 * decreases it. */
void MultiSpinBox::stepBy(int steps)
{
    // As explained in QAbstractSpinBox documentation:
    //    “Note that this function is called even if the resulting value will
    //     be outside the bounds of minimum and maximum. It's this function's
    //     job to handle these situations.”
    // Therefore, the result is bound to the actual minimum and maximum
    // values:
    d_pointer->m_sections[d_pointer->m_currentIndex].value += steps;
    d_pointer->m_sections[d_pointer->m_currentIndex] =
        d_pointer->fixedSection(
            d_pointer->m_sections[d_pointer->m_currentIndex]
        );
    // Update the internal representation
    d_pointer->updatePrefixValueSuffixText();
    // Update the content of the QLineEdit and select the current
    // value (as cursor text selection):
    d_pointer->setCurrentIndexAndUpdateTextAndSelectValue(
        d_pointer->m_currentIndex
    );
    // Make sure that the buttons for step up and step down are updated.
    update();
}

/** @brief Updates the value of the current section in @ref m_sections.
 *
 * This slot is meant to be connected to the
 * <tt>&QLineEdit::textChanged()</tt> signal of
 * the <tt>MultiSpinBox::lineEdit()</tt> child widget.
 * ,
 * @param lineEditText The text of the <tt>lineEdit()</tt>. The value
 * will be updated according to this parameter. Only changes in
 * the <em>current</em> section’s value are expected, no changes in
 * other sectins. (If this parameter has an ivalid value, a warning will
 * be printed to stderr and the function returns without further action.) */
void MultiSpinBox::MultiSpinBoxPrivate::updateCurrentValueFromText(
    const QString &lineEditText
)
{
    // Get the clean test. That means, we start with “text”, but
    // we remove the m_currentSectionTextBeforeValue and the
    // m_currentSectionTextAfterValue, so that only the text of
    // the value itself remains.
    QString cleanText = lineEditText;
    if (cleanText.startsWith(m_textBeforeCurrentValue)) {
        cleanText.remove(0, m_textBeforeCurrentValue.count());
    } else {
        // The text does not start with the correct characters.
        // This is an error.
        qWarning()
            << "The function"
            << __func__
            << "in file"
            << __FILE__
            << "near to line"
            << __LINE__
            << "was called with the invalid “lineEditText“ argument “"
            << lineEditText
            << "” that does not start with the expected character sequence “"
            << m_textBeforeCurrentValue
            << ". The call is ignored. This is a bug.";
        return;
    }
    if (cleanText.endsWith(m_textAfterCurrentValue)) {
        cleanText.chop(m_textAfterCurrentValue.count());
    } else {
        // The text does not start with the correct characters.
        // This is an error.
        qWarning()
            << "The function"
            << __func__
            << "in file"
            << __FILE__
            << "near to line"
            << __LINE__
            << "was called with the invalid “lineEditText“ argument “"
            << lineEditText
            << "” that does not end with the expected character sequence “"
            << m_textAfterCurrentValue
            << ". The call is ignored. This is a bug.";
        return;
    }

    // Update…
    bool ok;
    m_sections[m_currentIndex].value =
        q_pointer->locale().toDouble(cleanText, &ok);
    m_sections[m_currentIndex] = fixedSection(m_sections[m_currentIndex]);
    updatePrefixValueSuffixText();
    // Make sure that the buttons for step up and step down are updated.
    q_pointer->update();
    // The lineEdit()->text() property is intentionally not updated because
    // this function is ment to receive signals of the very same lineEdit().
}

/** @brief The main event handler.
 *
 * Reimplemented from base class.
 *
 * On <tt>QEvent::Type::LocaleChange</tt> it updates the spinbox content
 * accordingly. Apart from that, it calls the implementation in the parent
 * class. */
bool MultiSpinBox::event(QEvent *event)
{
    if (event->type() == QEvent::Type::LocaleChange) {
        d_pointer->updatePrefixValueSuffixText();
        d_pointer->m_validator->setPrefix(d_pointer->m_textBeforeCurrentValue);
        d_pointer->m_validator->setSuffix(d_pointer->m_textAfterCurrentValue);
        d_pointer->m_validator->setRange(
            d_pointer->m_sections.at(d_pointer->m_currentIndex).minimum,
            d_pointer->m_sections.at(d_pointer->m_currentIndex).maximum
        );
        lineEdit()->setText(
            d_pointer->m_textBeforeCurrentValue
                + d_pointer->m_textOfCurrentValue
                + d_pointer->m_textAfterCurrentValue
        );
    }
    return QAbstractSpinBox::event(event);
}

/** @brief Updates the widget according to the new cursor position.
 *
 * This slot is meant to be connected to the
 * <tt>QLineEdit::cursorPositionChanged()</tt> signal of
 * the <tt>MultiSpinBox::lineEdit()</tt> child widget.
 *
 * @param oldPos the old cursor position (previous position)
 * @param newPos the new cursor position (current position) */
void MultiSpinBox::MultiSpinBoxPrivate::reactOnCursorPositionChange(
    const int oldPos,
    const int newPos
)
{
    // This slot is meant to be connected to the
    // QLineEdit::cursorPositionChanged() signal of
    // the MultiSpinBox::lineEdit() child widget.
    // This signal emits the two “int” parameters “oldPos”
    // and “newPos”. We only need the second one, but we have
    // to take both of them as parameter if we want to stay
    // compatible. Therefore, we mark the first one
    // with Q_UNUSED to avoid compiler warnings.
    Q_UNUSED(oldPos);

    // We are working here with QString::length() and
    // QLineEdit::cursorPosition(). Both are of type “int”, and both are
    // measured in UTF-16 code units.  While it feels quite uncomfortable
    // to measure cursor positions in code _units_ and not at least in
    // in code _points_, it does not matter for this code, as the behaviour
    // is consistent between both usages.

    if (isCursorPositionAtCurrentSectionValue(newPos)) {
        // We are within the value text of our current section value.
        // There is nothing to do here.
        return;
    }

    QSignalBlocker myBlocker(q_pointer->lineEdit());

    // The new position is not at the current value, but the old one might
    // have been. So maybe we have to correct the value, which might change
    // its length. If the new cursor position is after this value, it will
    // have to be adapted (if the value had been changed or alternated).
    const int oldTextLength = q_pointer->lineEdit()->text().length();
    const bool cursorPositionHasToBeAdaptedToTextLenghtChange = (
        newPos > (oldTextLength - m_textAfterCurrentValue.length())
    );

    // Calculat in which section the cursor is
    int sectionOfTheNewCursorPosition;
    int reference = 0;
    for (
        sectionOfTheNewCursorPosition = 0;
        sectionOfTheNewCursorPosition < m_sections.count() - 1;
        ++sectionOfTheNewCursorPosition
    ) {
        reference +=
            m_sections.at(sectionOfTheNewCursorPosition).prefix.length();
        reference +=
            formattedValue(
                m_sections.at(sectionOfTheNewCursorPosition)
            ).length();
        reference +=
            m_sections.at(sectionOfTheNewCursorPosition).suffix.length();
        if (newPos <= reference) {
            break;
        }
    }

    updatePrefixValueSuffixText();
    setCurrentIndexWithoutUpdatingText(
        sectionOfTheNewCursorPosition
    );
    q_pointer->lineEdit()->setText(
        m_textBeforeCurrentValue
            + m_textOfCurrentValue
            + m_textAfterCurrentValue
    );
    int correctedCursorPosition = newPos;
    if (cursorPositionHasToBeAdaptedToTextLenghtChange) {
        correctedCursorPosition =
            newPos
                + q_pointer->lineEdit()->text().length()
                - oldTextLength;
    }
    q_pointer->lineEdit()->setCursorPosition(correctedCursorPosition);

    // Make sure that the buttons for step up and step down are updated.

    q_pointer->update();
}

}