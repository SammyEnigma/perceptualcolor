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

// Own header
#include "polarpointf.h"

// Other includes
#include <QtMath>

namespace PerceptualColor
{
/** @brief Constructor
 *
 * Normalizes the given polar coordinates and constructs an object with
 * the <em>normalized</em> polar coordinates. See the general class
 * description for details about the normalization.
 *
 * @param newRadial the @ref radial() value
 * @param newAngleDegree the @ref angleDegree() value */
PolarPointF::PolarPointF(const qreal newRadial, const qreal newAngleDegree)
{
    if (newRadial < 0) {
        m_radial = newRadial * (-1);
        m_angleDegree = normalizedAngleDegree(newAngleDegree + 180);
    } else {
        m_radial = newRadial;
        m_angleDegree = normalizedAngleDegree(newAngleDegree);
    }
}

/** @brief Constructor
 *
 * Constructs an object converting from the given Cartesian coordinates.
 *
 * If the Cartesian coordinates are (0, 0) than the @ref angleDegree (which is
 * meaningless for a @ref radial of 0) is set to 0°.
 *
 * @param cartesianCoordiantes the Cartesian coordinates */
PolarPointF::PolarPointF(const QPointF cartesianCoordiantes)
{
    m_radial = sqrt(pow(cartesianCoordiantes.x(), 2) + pow(cartesianCoordiantes.y(), 2));
    if (m_radial == 0) {
        m_angleDegree = 0;
        return;
    }
    if (cartesianCoordiantes.y() >= 0) {
        m_angleDegree = qRadiansToDegrees(acos(cartesianCoordiantes.x() / m_radial));
    } else {
        // M_PI is defined by QtMath (also on platforms that don’t
        // support it native)
        m_angleDegree = qRadiansToDegrees(2 * (M_PI)-acos(cartesianCoordiantes.x() / m_radial));
    }
}

/** @brief Compares with another @ref PolarPointF
 *
 * @param other the polar coordinates to compare with
 *
 * @returns <tt>true</tt> if both, <tt>this</tt> and <tt>other</tt>,
 * are the same point in the coordinate space. <tt>false</tt> otherwise.
 * Therefore <tt>[@ref radial() 0, @ref angleDegree() 50]</tt> is considered
 * to be the same point as <tt>[@ref radial() 0, @ref angleDegree() 80]</tt>
   because the @ref angleDegree() is meaningless if the @ref radial() is 0.*/
bool PolarPointF::isSamePoint(const PolarPointF other) const
{
    return (
        // radial has to be identical
        (m_radial == other.m_radial) &&
        // angle has to be identical (except when radial is 0, because
        // then angle is meaningless)
        ((m_angleDegree == other.m_angleDegree) || (m_radial == 0)));
}

/** @brief Normalized radial
 *
 * @returns the normalized radial value, guaranteed to be ≥ 0. */
qreal PolarPointF::radial() const
{
    return m_radial;
}

/** @brief Normalized angle
 *
 * @returns the normalized angle value (coordinates in degree), guaranteed to
 * be 0° ≤ value < 360° */
qreal PolarPointF::angleDegree() const
{
    return m_angleDegree;
}

/** @brief Normalizes an angle.
 *
 * |      Value      | Normalized Value |
 * | :-------------: | :--------------: |
 * | <tt>  0°  </tt> | <tt>  0°  </tt>  |
 * | <tt>359.9°</tt> | <tt>359.9°</tt>  |
 * | <tt>360°  </tt> | <tt>  0°  </tt>  |
 * | <tt>361.2°</tt> | <tt>  1.2°</tt>  |
 * | <tt>720°  </tt> | <tt>  0°  </tt>  |
 * | <tt> −1°  </tt> | <tt>359°  </tt>  |
 * | <tt> −1.3°</tt> | <tt>358.7°</tt>  |
 *
 * @param angleDegree an angle (coordinates in degree)
 * @returns the angle, normalized to the range 0° ≤ value < 360° */
qreal PolarPointF::normalizedAngleDegree(const qreal angleDegree)
{
    qreal temp = fmod(angleDegree, 360);
    if (temp < 0) {
        temp += 360;
    }
    return temp;
}

/** @brief Convert to Cartesian coordinates
 *
 * @returns the corresponding Cartesian coordinates */
QPointF PolarPointF::toCartesian() const
{
    return QPointF(m_radial * cos(qDegreesToRadians(m_angleDegree)), m_radial * sin(qDegreesToRadians(m_angleDegree)));
}

/** @internal
 *
 * @brief Adds QDebug() support for data type
 * @ref PerceptualColor::PolarPointF
 *
 * @param dbg Existing debug object
 * @param value Value to stream into the debug object
 * @returns Debug object with value streamed in */
QDebug operator<<(QDebug dbg, const PerceptualColor::PolarPointF value)
{
    dbg.nospace() << "PolarPointF(radial: " << value.radial() << ", angleDegree: " << value.angleDegree() << "°)";
    return dbg.maybeSpace();
}

static_assert(std::is_trivially_copyable_v<PolarPointF>);

static_assert(std::is_standard_layout_v<PolarPointF>);

} // namespace PerceptualColor
