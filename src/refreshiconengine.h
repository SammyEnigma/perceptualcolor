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

#ifndef REFRESHICONENGINE_H
#define REFRESHICONENGINE_H

#include "PerceptualColor/perceptualcolorglobal.h"
#include "perceptualcolorinternal.h"

#include <QIconEngine>
#include <QPointer>

namespace PerceptualColor
{
/** @internal
 *
 * @brief An fail-save icon engine that provides a <em>refresh</em> icon.
 *
 * This icon engine provides a <em>refresh</em> icon. It is fail-save,
 * which means that it will never fail to provide an icon: This icon
 * engine will always return a valid, non-null icon. It does a best
 * effort to find an icon that integrates well with the current icon
 * theme and widget style. It searches in various places to provide
 * this icon, eventually using the first one that it finds:
 * 1. The <tt>view-refresh</tt> icon provided by the current FreeDesktop
 *    icon theme, either on your system or boundled as resource with
 *    the application that uses this library. On Linux, it is common
 *    that an icon theme is provided. Which icon formats (SVG, PNG …)
 *    are supported depends on your Qt installation. On
 *    <a href="https://kate-editor.org/post/2021/2021-03-07-cross-platform-light-dark-themes-and-icons/">
 *    some platforms like KDE</a> the icons get automatically adapted to
 *    dark and light mode, on others not.
 * 2. The <tt>QStyle::StandardPixmap::SP_BrowserReload</tt> icon provided
 *    by the current <tt>QStyle</tt>.
 * 3. The fallback build-in icon that is hardcoded within this class.
 *    This icon is resolution-independed with High-DPI support (and
 *    is does <em>not</em> require SVG support in Qt for this) and
 *    adapts automatically to the current palette, thus providing
 *    automatically appropriate icon colors for dark mode and light
 *    mode. With the @ref setReferenceWidget() function it can
 *    integrate with a specific widget’s color palette (rather than
 *    the default color palette).
 *
 * This icon engine does not use a cache. That means, the icon will
 * be recalculated each time again. This is less efficient, but it
 * makes sure the icon is always up-to-date, also inmediatly after
 * the widget style or the icon theme or both have changed. */
class RefreshIconEngine : public QIconEngine
{
public:
    explicit RefreshIconEngine();
    /** @brief Default destructor. */
    virtual ~RefreshIconEngine() override = default;
    virtual QIconEngine *clone() const override;
    virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    void setReferenceWidget(QWidget *referenceWidget);

private:
    // Functions
    explicit RefreshIconEngine(const RefreshIconEngine &other);
    void paintFallbackIcon(QPainter *painter, const QRect rect, QIcon::Mode mode);

    // Data members
    /** @brief Holds a guarded pointer to the reference widget.
     * @sa @ref setReferenceWidget() */
    QPointer<QWidget> m_referenceWidget = nullptr;

    /** @internal @brief Only for unit tests. */
    friend class TestRefreshIconEngine;
};

} // namespace PerceptualColor

#endif // REFRESHICONENGINE_H
