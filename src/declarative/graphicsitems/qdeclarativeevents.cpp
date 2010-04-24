/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativeevents_p_p.h"

QT_BEGIN_NAMESPACE
/*!
    \qmlclass KeyEvent QDeclarativeKeyEvent
    \since 4.7
    \brief The KeyEvent object provides information about a key event.

    For example, the following changes the Item's state property when the Enter
    key is pressed:
    \qml
Item {
    focus: true
    Keys.onPressed: { if (event.key == Qt.Key_Enter) state = 'ShowDetails'; }
}
    \endqml
*/

/*!
    \internal
    \class QDeclarativeKeyEvent
*/

/*!
    \qmlproperty int KeyEvent::key

    This property holds the code of the key that was pressed or released.

    See \l {Qt::Key}{Qt.Key} for the list of keyboard codes. These codes are
    independent of the underlying window system. Note that this
    function does not distinguish between capital and non-capital
    letters, use the text() function (returning the Unicode text the
    key generated) for this purpose.

    A value of either 0 or \l {Qt::Key_unknown}{Qt.Key_Unknown} means that the event is not
    the result of a known key; for example, it may be the result of
    a compose sequence, a keyboard macro, or due to key event
    compression.
*/

/*!
    \qmlproperty string KeyEvent::text

    This property holds the Unicode text that the key generated.
    The text returned can be an empty string in cases where modifier keys,
    such as Shift, Control, Alt, and Meta, are being pressed or released.
    In such cases \c key will contain a valid value
*/

/*!
    \qmlproperty bool KeyEvent::isAutoRepeat

    This property holds whether this event comes from an auto-repeating key.
*/

/*!
    \qmlproperty int KeyEvent::count

    This property holds the number of keys involved in this event. If \l KeyEvent::text
    is not empty, this is simply the length of the string.
*/

/*!
    \qmlproperty bool KeyEvent::accepted

    Setting \a accepted to true prevents the key event from being
    propagated to the item's parent.

    Generally, if the item acts on the key event then it should be accepted
    so that ancestor items do not also respond to the same event.
*/


/*!
    \qmlclass MouseEvent QDeclarativeMouseEvent
    \since 4.7
    \brief The MouseEvent object provides information about a mouse event.

    The position of the mouse can be found via the x and y properties.
    The button that caused the event is available via the button property.
*/

/*!
    \internal
    \class QDeclarativeMouseEvent
*/

/*!
    \qmlproperty int MouseEvent::x
    \qmlproperty int MouseEvent::y

    These properties hold the position of the mouse event.
*/


/*!
    \qmlproperty bool MouseEvent::accepted

    Setting \a accepted to true prevents the mouse event from being
    propagated to items below this item.

    Generally, if the item acts on the mouse event then it should be accepted
    so that items lower in the stacking order do not also respond to the same event.
*/

/*!
    \qmlproperty enumeration MouseEvent::button

    This property holds the button that caused the event.  It can be one of:
    \list
    \o Qt.LeftButton
    \o Qt.RightButton
    \o Qt.MidButton
    \endlist
*/

/*!
    \qmlproperty bool MouseEvent::wasHeld

    This property is true if the mouse button has been held pressed longer the
    threshold (800ms).
*/

/*!
    \qmlproperty int MouseEvent::buttons

    This property holds the mouse buttons pressed when the event was generated.
    For mouse move events, this is all buttons that are pressed down. For mouse
    press and double click events this includes the button that caused the event.
    For mouse release events this excludes the button that caused the event.

    It contains a bitwise combination of:
    \list
    \o Qt.LeftButton
    \o Qt.RightButton
    \o Qt.MidButton
    \endlist
*/

/*!
    \qmlproperty int MouseEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \o Qt.NoModifier - No modifier key is pressed.
    \o Qt.ShiftModifier	- A Shift key on the keyboard is pressed.
    \o Qt.ControlModifier - A Ctrl key on the keyboard is pressed.
    \o Qt.AltModifier - An Alt key on the keyboard is pressed.
    \o Qt.MetaModifier - A Meta key on the keyboard is pressed.
    \o Qt.KeypadModifier - A keypad button is pressed.
    \endlist

    For example, to react to a Shift key + Left mouse button click:
    \qml
MouseArea {
    onClicked: { if (mouse.button == Qt.LeftButton && mouse.modifiers & Qt.ShiftModifier) doSomething(); }
}
    \endqml
*/

QT_END_NAMESPACE
