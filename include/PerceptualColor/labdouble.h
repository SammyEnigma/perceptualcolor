// SPDX-License-Identifier: MIT
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

#ifndef LABDOUBLE_H
#define LABDOUBLE_H

#include "PerceptualColor/perceptualcolorlib_global.h"

#include <lcms2.h>

#include <QDebug>

namespace PerceptualColor {

/** @brief A LAB color.
 *
 * Storage of floating point LAB values with <tt>double</tt> precision.
 *
 * The data is not default-initializad; it is undefined when the object
 * is created.
 *
 * Example:
 * @snippet test/testlabdouble.cpp Use LabDouble
 *
 * Data members:
 *
 * - L: Lightness, mesured in percent. The valid range  is <tt>[0, 100]</tt>.
 * - a: a axis (red-green-axis).
 * - b: b_axis (yellow-blue-axis).
 *
 * @sa @ref LchValues explains more details about the valid range.
 *
 * This class is declared as type to Qt's type system:
 * <tt>Q_DECLARE_METATYPE(PerceptualColor::LabDouble)</tt>.
 * Depending on your use case (for example if you want to use it
 * reliably in Qt's signals and slots), you might consider calling
 * <tt>qRegisterMetaType()</tt> for this type, once you have a QApplication
 * object. */
using LabDouble = cmsCIELab;

} // namespace PerceptualColor

QDebug operator<<(QDebug dbg, const PerceptualColor::LabDouble &value);

Q_DECLARE_METATYPE(PerceptualColor::LabDouble)

#endif // LABDOUBLE_H
