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

// First included header is the public header of the class we are testing;
// this forces the header to be self-contained.
#include "PerceptualColor/constpropagatinguniquepointer.h"

#include <QtTest>

#include <QRectF>

static void snippet01()
{
    //! [example]
    // A ConstPropagatingUniquePointer pointing to a new QObject
    PerceptualColor::ConstPropagatingUniquePointer<QObject> myPointer(new QObject());
    //! [example]
}

namespace PerceptualColor
{
class TestConstPropagatingUniquePointer : public QObject
{
    Q_OBJECT

public:
    TestConstPropagatingUniquePointer(QObject *parent = nullptr)
        : QObject(parent)
        , pointerToQRectF(new QRectF)
    {
    }

private:
    ConstPropagatingUniquePointer<QRectF> pointerToQRectF;

private Q_SLOTS:

    void initTestCase()
    {
        // Called before the first test function is executed
    }

    void cleanupTestCase()
    {
        // Called after the last test function was executed
    }

    void init()
    {
        // Called before each test function is executed
    }
    void cleanup()
    {
        // Called after every test function
    }

    void testConstructorDestructor()
    {
        ConstPropagatingUniquePointer<QObject> test;
    }

    void testDefaultConstructor()
    {
        ConstPropagatingUniquePointer<QObject> test;
        QCOMPARE(test, nullptr);
    }

    // NOTE Should break on compile time when the function is const.
    void testNonConstAccess()
    {
        // The following line should not break
        pointerToQRectF->setHeight(5);
    }

    // NOTE Should break on compile time when the function is const.
    void testBackCopy01()
    {
        QRectF temp;
        *pointerToQRectF = temp;
    }

    void testConstAccess01() const
    {
        // The following line should not break
        qreal height = pointerToQRectF->height();
        Q_UNUSED(height)
    }

    void testConstAccess02()
    {
        // The following line should not break
        qreal height = pointerToQRectF->height();
        Q_UNUSED(height)
    }

    void testCopy01() const
    {
        QRectF temp = *pointerToQRectF;
        Q_UNUSED(temp);
    }

    void testCopy02()
    {
        QRectF temp = *pointerToQRectF;
        Q_UNUSED(temp);
    }

    void testSnippet01()
    {
        snippet01();
    }
};

} // namespace PerceptualColor

QTEST_MAIN(PerceptualColor::TestConstPropagatingUniquePointer)
// The following “include” is necessary because we do not use a header file:
#include "testconstpropagatinguniquepointer.moc"
