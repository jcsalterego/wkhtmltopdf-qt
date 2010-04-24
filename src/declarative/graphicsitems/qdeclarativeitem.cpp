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

#include "private/qdeclarativeitem_p.h"
#include "qdeclarativeitem.h"

#include "private/qdeclarativeevents_p_p.h"
#include <private/qdeclarativeengine_p.h>

#include <qdeclarativeengine.h>
#include <qdeclarativeopenmetaobject_p.h>
#include <qdeclarativestate_p.h>
#include <qdeclarativeview.h>
#include <qdeclarativestategroup_p.h>
#include <qdeclarativecomponent.h>
#include <qdeclarativeinfo.h>

#include <QDebug>
#include <QPen>
#include <QFile>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QtCore/qnumeric.h>
#include <QtScript/qscriptengine.h>
#include <QtGui/qgraphicstransform.h>
#include <qlistmodelinterface_p.h>

QT_BEGIN_NAMESPACE

#ifndef FLT_MAX
#define FLT_MAX 1E+37
#endif

/*!
    \qmlclass Transform QGraphicsTransform
    \since 4.7
    \brief The Transform elements provide a way of building advanced transformations on Items.

    The Transform element is a base type which cannot be instantiated directly.
    The following concrete Transform types are available:

    \list
    \o \l Rotation
    \o \l Scale
    \o \l Translate
    \endlist

    The Transform elements let you create and control advanced transformations that can be configured
    independently using specialized properties.

    You can assign any number of Transform elements to an Item. Each Transform is applied in order,
    one at a time, to the Item it's assigned to.
*/

/*!
    \qmlclass Translate QGraphicsTranslate
    \since 4.7
    \brief The Translate object provides a way to move an Item without changing its x or y properties.

    The Translate object provides independent control over position in addition to the Item's x and y properties.

    The following example moves the Y axis of the Rectangles while still allowing the Row element
    to lay the items out as if they had not been transformed:
    \qml
    Row {
        Rectangle {
            width: 100; height: 100
            color: "blue"
            transform: Translate { y: 20 }
        }
        Rectangle {
            width: 100; height: 100
            color: "red"
            transform: Translate { y: -20 }
        }
    }
    \endqml
*/

/*!
    \qmlproperty real Translate::x

    The translation along the X axis.
*/

/*!
    \qmlproperty real Translate::y

    The translation along the Y axis.
*/

/*!
    \qmlclass Scale QGraphicsScale
    \since 4.7
    \brief The Scale object provides a way to scale an Item.

    The Scale object gives more control over scaling than using Item's scale property. Specifically,
    it allows a different scale for the x and y axes, and allows the scale to be relative to an
    arbitrary point.

    The following example scales the X axis of the Rectangle, relative to its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Scale { origin.x: 25; origin.y: 25; xScale: 3}
    }
    \endqml
*/

/*!
    \qmlproperty real Scale::origin.x
    \qmlproperty real Scale::origin.y

    The point that the item is scaled from (i.e., the point that stays fixed relative to the parent as
    the rest of the item grows). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real Scale::xScale

    The scaling factor for the X axis.
*/

/*!
    \qmlproperty real Scale::yScale

    The scaling factor for the Y axis.
*/

/*!
    \qmlclass Rotation QGraphicsRotation
    \since 4.7
    \brief The Rotation object provides a way to rotate an Item.

    The Rotation object gives more control over rotation than using Item's rotation property.
    Specifically, it allows (z axis) rotation to be relative to an arbitrary point.

    The following example rotates a Rectangle around its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Rotation { origin.x: 25; origin.y: 25; angle: 45}
    }
    \endqml

    Rotation also provides a way to specify 3D-like rotations for Items. For these types of
    rotations you must specify the axis to rotate around in addition to the origin point.

    The following example shows various 3D-like rotations applied to an \l Image.
    \snippet doc/src/snippets/declarative/rotation.qml 0

    \image axisrotation.png
*/

/*!
    \qmlproperty real Rotation::origin.x
    \qmlproperty real Rotation::origin.y

    The origin point of the rotation (i.e., the point that stays fixed relative to the parent as
    the rest of the item rotates). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real Rotation::axis.x
    \qmlproperty real Rotation::axis.y
    \qmlproperty real Rotation::axis.z

    The axis to rotate around. For simple (2D) rotation around a point, you do not need to specify an axis,
    as the default axis is the z axis (\c{ axis { x: 0; y: 0; z: 1 } }).

    For a typical 3D-like rotation you will usually specify both the origin and the axis.

    \image 3d-rotation-axis.png
*/

/*!
    \qmlproperty real Rotation::angle

    The angle to rotate, in degrees clockwise.
*/


/*!
    \group group_animation
    \title Animation
*/

/*!
    \group group_coreitems
    \title Basic Items
*/

/*!
    \group group_layouts
    \title Layouts
*/

/*!
    \group group_states
    \title States and Transitions
*/

/*!
    \group group_utility
    \title Utility
*/

/*!
    \group group_views
    \title Views
*/

/*!
    \group group_widgets
    \title Widgets
*/

/*!
    \internal
    \class QDeclarativeContents
    \ingroup group_utility
    \brief The QDeclarativeContents class gives access to the height and width of an item's contents.

*/

QDeclarativeContents::QDeclarativeContents() : m_x(0), m_y(0), m_width(0), m_height(0)
{
}

QRectF QDeclarativeContents::rectF() const
{
    return QRectF(m_x, m_y, m_width, m_height);
}

//TODO: optimization: only check sender(), if there is one
void QDeclarativeContents::calcHeight()
{
    qreal oldy = m_y;
    qreal oldheight = m_height;

    qreal top = FLT_MAX;
    qreal bottom = 0;

    QList<QGraphicsItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *child = qobject_cast<QDeclarativeItem *>(children.at(i));
        if(!child)//### Should this be ignoring non-QDeclarativeItem graphicsobjects?
            continue;
        qreal y = child->y();
        if (y + child->height() > bottom)
            bottom = y + child->height();
        if (y < top)
            top = y;
    }
    if (!children.isEmpty())
        m_y = top;
    m_height = qMax(bottom - top, qreal(0.0));

    if (m_height != oldheight || m_y != oldy)
        emit rectChanged(rectF());
}

//TODO: optimization: only check sender(), if there is one
void QDeclarativeContents::calcWidth()
{
    qreal oldx = m_x;
    qreal oldwidth = m_width;

    qreal left = FLT_MAX;
    qreal right = 0;

    QList<QGraphicsItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *child = qobject_cast<QDeclarativeItem *>(children.at(i));
        if(!child)//### Should this be ignoring non-QDeclarativeItem graphicsobjects?
            continue;
        qreal x = child->x();
        if (x + child->width() > right)
            right = x + child->width();
        if (x < left)
            left = x;
    }
    if (!children.isEmpty())
        m_x = left;
    m_width = qMax(right - left, qreal(0.0));

    if (m_width != oldwidth || m_x != oldx)
        emit rectChanged(rectF());
}

void QDeclarativeContents::setItem(QDeclarativeItem *item)
{
    m_item = item;

    QList<QGraphicsItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *child = qobject_cast<QDeclarativeItem *>(children.at(i));
        if(!child)//### Should this be ignoring non-QDeclarativeItem graphicsobjects?
            continue;
        connect(child, SIGNAL(heightChanged()), this, SLOT(calcHeight()));
        connect(child, SIGNAL(yChanged()), this, SLOT(calcHeight()));
        connect(child, SIGNAL(widthChanged()), this, SLOT(calcWidth()));
        connect(child, SIGNAL(xChanged()), this, SLOT(calcWidth()));
        connect(this, SIGNAL(rectChanged(QRectF)), m_item, SIGNAL(childrenRectChanged(QRectF)));
    }

    calcHeight();
    calcWidth();
}

QDeclarativeItemKeyFilter::QDeclarativeItemKeyFilter(QDeclarativeItem *item)
: m_next(0)
{
    QDeclarativeItemPrivate *p =
        item?static_cast<QDeclarativeItemPrivate *>(QGraphicsItemPrivate::get(item)):0;
    if (p) {
        m_next = p->keyHandler;
        p->keyHandler = this;
    }
}

QDeclarativeItemKeyFilter::~QDeclarativeItemKeyFilter()
{
}

void QDeclarativeItemKeyFilter::keyPressed(QKeyEvent *event)
{
    if (m_next) m_next->keyPressed(event);
}

void QDeclarativeItemKeyFilter::keyReleased(QKeyEvent *event)
{
    if (m_next) m_next->keyReleased(event);
}

void QDeclarativeItemKeyFilter::inputMethodEvent(QInputMethodEvent *event)
{
    if (m_next) m_next->inputMethodEvent(event);
}

QVariant QDeclarativeItemKeyFilter::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (m_next) return m_next->inputMethodQuery(query);
    return QVariant();
}

void QDeclarativeItemKeyFilter::componentComplete()
{
    if (m_next) m_next->componentComplete();
}


/*!
    \qmlclass KeyNavigation
    \since 4.7
    \brief The KeyNavigation attached property supports key navigation by arrow keys.

    It is common in key-based UIs to use arrow keys to navigate
    between focussed items.  The KeyNavigation property provides a
    convenient way of specifying which item will gain focus
    when an arrow key is pressed.  The following example provides
    key navigation for a 2x2 grid of items.

    \code
    Grid {
        columns: 2
        width: 100; height: 100
        Rectangle {
            id: item1
            focus: true
            width: 50; height: 50
            color: focus ? "red" : "lightgray"
            KeyNavigation.right: item2
            KeyNavigation.down: item3
        }
        Rectangle {
            id: item2
            width: 50; height: 50
            color: focus ? "red" : "lightgray"
            KeyNavigation.left: item1
            KeyNavigation.down: item4
        }
        Rectangle {
            id: item3
            width: 50; height: 50
            color: focus ? "red" : "lightgray"
            KeyNavigation.right: item4
            KeyNavigation.up: item1
        }
        Rectangle {
            id: item4
            width: 50; height: 50
            color: focus ? "red" : "lightgray"
            KeyNavigation.left: item3
            KeyNavigation.up: item2
        }
    }
    \endcode

    KeyNavigation receives key events after the item it is attached to.
    If the item accepts an arrow key event, the KeyNavigation
    attached property will not receive an event for that key.

    If an item has been set for a direction and the KeyNavigation
    attached property receives the corresponding
    key press and release events, the events will be accepted by
    KeyNaviagtion and will not propagate any further.

    \sa {Keys}{Keys attached property}
*/

/*!
    \qmlproperty Item KeyNavigation::left
    \qmlproperty Item KeyNavigation::right
    \qmlproperty Item KeyNavigation::up
    \qmlproperty Item KeyNavigation::down

    These properties hold the item to assign focus to
    when Key_Left, Key_Right, Key_Up or Key_Down are
    pressed.
*/

QDeclarativeKeyNavigationAttached::QDeclarativeKeyNavigationAttached(QObject *parent)
: QObject(*(new QDeclarativeKeyNavigationAttachedPrivate), parent),
  QDeclarativeItemKeyFilter(qobject_cast<QDeclarativeItem*>(parent))
{
}

QDeclarativeKeyNavigationAttached *
QDeclarativeKeyNavigationAttached::qmlAttachedProperties(QObject *obj)
{
    return new QDeclarativeKeyNavigationAttached(obj);
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::left() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->left;
}

void QDeclarativeKeyNavigationAttached::setLeft(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->left = i;
    emit changed();
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::right() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->right;
}

void QDeclarativeKeyNavigationAttached::setRight(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->right = i;
    emit changed();
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::up() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->up;
}

void QDeclarativeKeyNavigationAttached::setUp(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->up = i;
    emit changed();
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::down() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->down;
}

void QDeclarativeKeyNavigationAttached::setDown(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->down = i;
    emit changed();
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::tab() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->tab;
}

void QDeclarativeKeyNavigationAttached::setTab(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->tab = i;
    emit changed();
}

QDeclarativeItem *QDeclarativeKeyNavigationAttached::backtab() const
{
    Q_D(const QDeclarativeKeyNavigationAttached);
    return d->backtab;
}

void QDeclarativeKeyNavigationAttached::setBacktab(QDeclarativeItem *i)
{
    Q_D(QDeclarativeKeyNavigationAttached);
    d->backtab = i;
    emit changed();
}

void QDeclarativeKeyNavigationAttached::keyPressed(QKeyEvent *event)
{
    Q_D(QDeclarativeKeyNavigationAttached);

    event->ignore();

    switch(event->key()) {
    case Qt::Key_Left:
        if (d->left) {
            d->left->setFocus(true);
            event->accept();
        }
        break;
    case Qt::Key_Right:
        if (d->right) {
            d->right->setFocus(true);
            event->accept();
        }
        break;
    case Qt::Key_Up:
        if (d->up) {
            d->up->setFocus(true);
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            d->down->setFocus(true);
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            d->tab->setFocus(true);
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            d->backtab->setFocus(true);
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QDeclarativeItemKeyFilter::keyPressed(event);
}

void QDeclarativeKeyNavigationAttached::keyReleased(QKeyEvent *event)
{
    Q_D(QDeclarativeKeyNavigationAttached);

    event->ignore();

    switch(event->key()) {
    case Qt::Key_Left:
        if (d->left) {
            event->accept();
        }
        break;
    case Qt::Key_Right:
        if (d->right) {
            event->accept();
        }
        break;
    case Qt::Key_Up:
        if (d->up) {
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QDeclarativeItemKeyFilter::keyReleased(event);
}

/*!
    \qmlclass Keys
    \since 4.7
    \brief The Keys attached property provides key handling to Items.

    All visual primitives support key handling via the \e Keys
    attached property.  Keys can be handled via the \e onPressed
    and \e onReleased signal properties.

    The signal properties have a \l KeyEvent parameter, named
    \e event which contains details of the event.  If a key is
    handled \e event.accepted should be set to true to prevent the
    event from propagating up the item heirarchy.

    \code
    Item {
        focus: true
        Keys.onPressed: {
            if (event.key == Qt.Key_Left) {
                console.log("move left");
                event.accepted = true;
            }
        }
    }
    \endcode

    Some keys may alternatively be handled via specific signal properties,
    for example \e onSelectPressed.  These handlers automatically set
    \e event.accepted to true.

    \code
    Item {
        focus: true
        Keys.onLeftPressed: console.log("move left")
    }
    \endcode

    See \l {Qt::Key}{Qt.Key} for the list of keyboard codes.

    \sa KeyEvent, {KeyNavigation}{KeyNavigation attached property}
*/

/*!
    \qmlproperty bool Keys::enabled

    This flags enables key handling if true (default); otherwise
    no key handlers will be called.
*/

/*!
    \qmlproperty List<Object> Keys::forwardTo

    This property provides a way to forward key presses, key releases, and keyboard input
    coming from input methods to other items. This can be useful when you want
    one item to handle some keys (e.g. the up and down arrow keys), and another item to
    handle other keys (e.g. the left and right arrow keys).  Once an item that has been
    forwarded keys accepts the event it is no longer forwarded to items later in the
    list.

    This example forwards key events to two lists:
    \qml
    ListView { id: list1 ... }
    ListView { id: list2 ... }
    Keys.forwardTo: [list1, list2]
    focus: true
    \endqml
*/

/*!
    \qmlsignal Keys::onPressed(event)

    This handler is called when a key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onReleased(event)

    This handler is called when a key has been released. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit0Pressed(event)

    This handler is called when the digit '0' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit1Pressed(event)

    This handler is called when the digit '1' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit2Pressed(event)

    This handler is called when the digit '2' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit3Pressed(event)

    This handler is called when the digit '3' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit4Pressed(event)

    This handler is called when the digit '4' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit5Pressed(event)

    This handler is called when the digit '5' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit6Pressed(event)

    This handler is called when the digit '6' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit7Pressed(event)

    This handler is called when the digit '7' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit8Pressed(event)

    This handler is called when the digit '8' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDigit9Pressed(event)

    This handler is called when the digit '9' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onLeftPressed(event)

    This handler is called when the Left arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onRightPressed(event)

    This handler is called when the Right arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onUpPressed(event)

    This handler is called when the Up arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDownPressed(event)

    This handler is called when the Down arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onAsteriskPressed(event)

    This handler is called when the Asterisk '*' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onEscapePressed(event)

    This handler is called when the Escape key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onReturnPressed(event)

    This handler is called when the Return key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onEnterPressed(event)

    This handler is called when the Enter key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onDeletePressed(event)

    This handler is called when the Delete key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onSpacePressed(event)

    This handler is called when the Space key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onBackPressed(event)

    This handler is called when the Back key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onCancelPressed(event)

    This handler is called when the Cancel key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onSelectPressed(event)

    This handler is called when the Select key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onYesPressed(event)

    This handler is called when the Yes key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onNoPressed(event)

    This handler is called when the No key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onContext1Pressed(event)

    This handler is called when the Context1 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onContext2Pressed(event)

    This handler is called when the Context2 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onContext3Pressed(event)

    This handler is called when the Context3 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onContext4Pressed(event)

    This handler is called when the Context4 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onCallPressed(event)

    This handler is called when the Call key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onHangupPressed(event)

    This handler is called when the Hangup key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onFlipPressed(event)

    This handler is called when the Flip key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onMenuPressed(event)

    This handler is called when the Menu key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onVolumeUpPressed(event)

    This handler is called when the VolumeUp key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal Keys::onVolumeDownPressed(event)

    This handler is called when the VolumeDown key has been pressed. The \a event
    parameter provides information about the event.
*/

const QDeclarativeKeysAttached::SigMap QDeclarativeKeysAttached::sigMap[] = {
    { Qt::Key_Left, "leftPressed" },
    { Qt::Key_Right, "rightPressed" },
    { Qt::Key_Up, "upPressed" },
    { Qt::Key_Down, "downPressed" },
    { Qt::Key_Tab, "tabPressed" },
    { Qt::Key_Backtab, "backtabPressed" },
    { Qt::Key_Asterisk, "asteriskPressed" },
    { Qt::Key_NumberSign, "numberSignPressed" },
    { Qt::Key_Escape, "escapePressed" },
    { Qt::Key_Return, "returnPressed" },
    { Qt::Key_Enter, "enterPressed" },
    { Qt::Key_Delete, "deletePressed" },
    { Qt::Key_Space, "spacePressed" },
    { Qt::Key_Back, "backPressed" },
    { Qt::Key_Cancel, "cancelPressed" },
    { Qt::Key_Select, "selectPressed" },
    { Qt::Key_Yes, "yesPressed" },
    { Qt::Key_No, "noPressed" },
    { Qt::Key_Context1, "context1Pressed" },
    { Qt::Key_Context2, "context2Pressed" },
    { Qt::Key_Context3, "context3Pressed" },
    { Qt::Key_Context4, "context4Pressed" },
    { Qt::Key_Call, "callPressed" },
    { Qt::Key_Hangup, "hangupPressed" },
    { Qt::Key_Flip, "flipPressed" },
    { Qt::Key_Menu, "menuPressed" },
    { Qt::Key_VolumeUp, "volumeUpPressed" },
    { Qt::Key_VolumeDown, "volumeDownPressed" },
    { 0, 0 }
};

bool QDeclarativeKeysAttachedPrivate::isConnected(const char *signalName)
{
    return isSignalConnected(signalIndex(signalName));
}

QDeclarativeKeysAttached::QDeclarativeKeysAttached(QObject *parent)
: QObject(*(new QDeclarativeKeysAttachedPrivate), parent),
  QDeclarativeItemKeyFilter(qobject_cast<QDeclarativeItem*>(parent))
{
    Q_D(QDeclarativeKeysAttached);
    d->item = qobject_cast<QDeclarativeItem*>(parent);
}

QDeclarativeKeysAttached::~QDeclarativeKeysAttached()
{
}

void QDeclarativeKeysAttached::componentComplete()
{
    Q_D(QDeclarativeKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QGraphicsItem *targetItem = d->finalFocusProxy(d->targets.at(ii));
            if (targetItem && (targetItem->flags() & QGraphicsItem::ItemAcceptsInputMethod)) {
                d->item->setFlag(QGraphicsItem::ItemAcceptsInputMethod);
                break;
            }
        }
    }
}

void QDeclarativeKeysAttached::keyPressed(QKeyEvent *event)
{
    Q_D(QDeclarativeKeysAttached);
    if (!d->enabled || d->inPress) {
        event->ignore();
        return;
    }

    // first process forwards
    if (d->item && d->item->scene()) {
        d->inPress = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QGraphicsItem *i = d->finalFocusProxy(d->targets.at(ii));
            if (i) {
                d->item->scene()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inPress = false;
                    return;
                }
            }
        }
        d->inPress = false;
    }

    QDeclarativeKeyEvent ke(*event);
    QByteArray keySignal = keyToSignal(event->key());
    if (!keySignal.isEmpty()) {
        keySignal += "(QDeclarativeKeyEvent*)";
        if (d->isConnected(keySignal)) {
            // If we specifically handle a key then default to accepted
            ke.setAccepted(true);
            int idx = QDeclarativeKeysAttached::staticMetaObject.indexOfSignal(keySignal);
            metaObject()->method(idx).invoke(this, Qt::DirectConnection, Q_ARG(QDeclarativeKeyEvent*, &ke));
        }
    }
    if (!ke.isAccepted())
        emit pressed(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QDeclarativeItemKeyFilter::keyPressed(event);
}

void QDeclarativeKeysAttached::keyReleased(QKeyEvent *event)
{
    Q_D(QDeclarativeKeysAttached);
    if (!d->enabled || d->inRelease) {
        event->ignore();
        return;
    }

    if (d->item && d->item->scene()) {
        d->inRelease = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QGraphicsItem *i = d->finalFocusProxy(d->targets.at(ii));
            if (i) {
                d->item->scene()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inRelease = false;
                    return;
                }
            }
        }
        d->inRelease = false;
    }

    QDeclarativeKeyEvent ke(*event);
    emit released(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QDeclarativeItemKeyFilter::keyReleased(event);
}

void QDeclarativeKeysAttached::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QDeclarativeKeysAttached);
    if (d->item && !d->inIM && d->item->scene()) {
        d->inIM = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QGraphicsItem *i = d->finalFocusProxy(d->targets.at(ii));
            if (i && (i->flags() & QGraphicsItem::ItemAcceptsInputMethod)) {
                d->item->scene()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->imeItem = i;
                    d->inIM = false;
                    return;
                }
            }
        }
        d->inIM = false;
    }
    if (!event->isAccepted()) QDeclarativeItemKeyFilter::inputMethodEvent(event);
}

class QDeclarativeItemAccessor : public QGraphicsItem
{
public:
    QVariant doInputMethodQuery(Qt::InputMethodQuery query) const {
        return QGraphicsItem::inputMethodQuery(query);
    }
};

QVariant QDeclarativeKeysAttached::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QDeclarativeKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
                QGraphicsItem *i = d->finalFocusProxy(d->targets.at(ii));
            if (i && (i->flags() & QGraphicsItem::ItemAcceptsInputMethod) && i == d->imeItem) { //### how robust is i == d->imeItem check?
                QVariant v = static_cast<QDeclarativeItemAccessor *>(i)->doInputMethodQuery(query);
                if (v.userType() == QVariant::RectF)
                    v = d->item->mapRectFromItem(i, v.toRectF());  //### cost?
                return v;
            }
        }
    }
    return QDeclarativeItemKeyFilter::inputMethodQuery(query);
}

QDeclarativeKeysAttached *QDeclarativeKeysAttached::qmlAttachedProperties(QObject *obj)
{
    return new QDeclarativeKeysAttached(obj);
}

/*!
    \class QDeclarativeItem
    \since 4.7
    \brief The QDeclarativeItem class provides the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from QDeclarativeItem.  Although QDeclarativeItem
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    You can subclass QDeclarativeItem to provide your own custom visual item that inherits
    these features.
*/

/*!
    \qmlclass Item QDeclarativeItem
    \since 4.7
    \brief The Item is the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from Item.  Although Item
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    Item is also useful for grouping items together.

    \qml
    Item {
        Image {
            source: "tile.png"
        }
        Image {
            x: 80
            width: 100
            height: 100
            source: "tile.png"
        }
        Image {
            x: 190
            width: 100
            height: 100
            fillMode: Image.Tile
            source: "tile.png"
        }
    }
    \endqml

    \section1 Identity

    Each item has an "id" - the identifier of the Item.

    The identifier can be used in bindings and other expressions to
    refer to the item. For example:

    \qml
    Text { id: myText; ... }
    Text { text: myText.text }
    \endqml

    The identifier is available throughout to the \l {components}{component}
    where it is declared.  The identifier must be unique in the component.

    The id should not be thought of as a "property" - it makes no sense
    to write \c myText.id, for example.

    \section1 Key Handling

    Key handling is available to all Item-based visual elements via the \l {Keys}{Keys}
    attached property.  The \e Keys attached property provides basic handlers such
    as \l {Keys::onPressed}{onPressed} and \l {Keys::onReleased}{onReleased},
    as well as handlers for specific keys, such as
    \l {Keys::onCancelPressed}{onCancelPressed}.  The example below
    assigns \l {qmlfocus}{focus} to the item and handles
    the Left key via the general \e onPressed handler and the Select key via the
    onSelectPressed handler:

    \qml
    Item {
        focus: true
        Keys.onPressed: {
            if (event.key == Qt.Key_Left) {
                console.log("move left");
                event.accepted = true;
            }
        }
        Keys.onSelectPressed: console.log("Selected");
    }
    \endqml

    See the \l {Keys}{Keys} attached property for detailed documentation.

    \section1 Property Change Signals

    Most properties on Item and Item derivatives have a signal
    emitted when they change. By convention, the signals are
    named <propertyName>Changed, e.g. xChanged will be emitted when an item's
    x property changes. Note that these also have signal handers e.g.
    the onXChanged signal handler will be called when an item's x property
    changes. For many properties in Item or Item derivatives this can be used
    to add a touch of imperative logic to your application (when absolutely
    necessary).

    \ingroup group_coreitems
*/

/*!
    \property QDeclarativeItem::baseline
    \internal
*/

/*!
    \property QDeclarativeItem::effect
    \internal
*/

/*!
    \property QDeclarativeItem::focus
    \internal
*/

/*!
    \property QDeclarativeItem::wantsFocus
    \internal
*/

/*!
    \property QDeclarativeItem::transformOrigin
    \internal
*/

/*!
    \fn void QDeclarativeItem::childrenRectChanged(const QRectF &)
    \internal
*/

/*!
    \fn void QDeclarativeItem::baselineOffsetChanged(qreal)
    \internal
*/

/*!
    \fn void QDeclarativeItem::stateChanged(const QString &state)
    \internal
*/

/*!
    \fn void QDeclarativeItem::parentChanged(QDeclarativeItem *)
    \internal
*/

/*!
    \fn void QDeclarativeItem::smoothChanged(bool)
    \internal
*/

/*!
    \fn void QDeclarativeItem::clipChanged(bool)
    \internal
*/

/*! \fn void QDeclarativeItem::transformOriginChanged(TransformOrigin)
  \internal
*/

/*!
    \fn void QDeclarativeItem::childrenChanged()
    \internal
*/

/*!
    \fn void QDeclarativeItem::focusChanged(bool)
    \internal
*/

/*!
    \fn void QDeclarativeItem::wantsFocusChanged(bool)
    \internal
*/

// ### Must fix
struct RegisterAnchorLineAtStartup {
    RegisterAnchorLineAtStartup() {
        qRegisterMetaType<QDeclarativeAnchorLine>("QDeclarativeAnchorLine");
    }
};
static RegisterAnchorLineAtStartup registerAnchorLineAtStartup;


/*!
    \fn QDeclarativeItem::QDeclarativeItem(QDeclarativeItem *parent)

    Constructs a QDeclarativeItem with the given \a parent.
*/
QDeclarativeItem::QDeclarativeItem(QDeclarativeItem* parent)
  : QGraphicsObject(*(new QDeclarativeItemPrivate), parent, 0)
{
    Q_D(QDeclarativeItem);
    d->init(parent);
}

/*! \internal
*/
QDeclarativeItem::QDeclarativeItem(QDeclarativeItemPrivate &dd, QDeclarativeItem *parent)
  : QGraphicsObject(dd, parent, 0)
{
    Q_D(QDeclarativeItem);
    d->init(parent);
}

/*!
    Destroys the QDeclarativeItem.
*/
QDeclarativeItem::~QDeclarativeItem()
{
    Q_D(QDeclarativeItem);
    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        QDeclarativeAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
        if (anchor)
            anchor->clearItem(this);
    }
    if (!d->parent || (parentItem() && !parentItem()->QGraphicsItem::d_ptr->inDestructor)) {
        for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
            QDeclarativeAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
            if (anchor && anchor->item && anchor->item->parentItem() != this) //child will be deleted anyway
                anchor->updateOnComplete();
        }
    }
    for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QDeclarativeItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QDeclarativeItemPrivate::Destroyed)
            change.listener->itemDestroyed(this);
    }
    d->changeListeners.clear();
    delete d->_anchorLines; d->_anchorLines = 0;
    delete d->_anchors; d->_anchors = 0;
    delete d->_stateGroup; d->_stateGroup = 0;
}

/*!
    \qmlproperty enumeration Item::transformOrigin
    This property holds the origin point around which scale and rotation transform.

    Nine transform origins are available, as shown in the image below.

    \image declarative-transformorigin.png

    This example rotates an image around its bottom-right corner.
    \qml
    Image {
        source: "myimage.png"
        transformOrigin: Item.BottomRight
        rotation: 45
    }
    \endqml

    The default transform origin is \c Center.
*/

/*!
    \qmlproperty Item Item::parent
    This property holds the parent of the item.
*/

/*!
    \property QDeclarativeItem::parent
    This property holds the parent of the item.
*/
void QDeclarativeItem::setParentItem(QDeclarativeItem *parent)
{
    QGraphicsObject::setParentItem(parent);
}

/*!
    Returns the QDeclarativeItem parent of this item.
*/
QDeclarativeItem *QDeclarativeItem::parentItem() const
{
    return qobject_cast<QDeclarativeItem *>(QGraphicsObject::parentItem());
}

/*!
    \qmlproperty real Item::childrenRect.x
    \qmlproperty real Item::childrenRect.y
    \qmlproperty real Item::childrenRect.width
    \qmlproperty real Item::childrenRect.height

    The childrenRect properties allow an item access to the geometry of its
    children. This property is useful if you have an item that needs to be
    sized to fit its children.
*/


/*!
    \qmlproperty list<Item> Item::children
    \qmlproperty list<Object> Item::resources

    The children property contains the list of visual children of this item.
    The resources property contains non-visual resources that you want to
    reference by name.

    Generally you can rely on Item's default property to handle all this for
    you, but it can come in handy in some cases.

    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Component {
                id: myComponent
                Text {}
            }
        ]
    }
    \endqml
*/

/*!
    \property QDeclarativeItem::resources
    \internal
*/

/*!
    Returns true if construction of the QML component is complete; otherwise
    returns false.

    It is often desireable to delay some processing until the component is
    completed.

    \sa componentComplete()
*/
bool QDeclarativeItem::isComponentComplete() const
{
    Q_D(const QDeclarativeItem);
    return d->_componentComplete;
}

/*!
    \property QDeclarativeItem::anchors
    \internal
*/

/*! \internal */
QDeclarativeAnchors *QDeclarativeItem::anchors()
{
    Q_D(QDeclarativeItem);
    return d->anchors();
}

void QDeclarativeItemPrivate::data_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    QDeclarativeItem *that = static_cast<QDeclarativeItem *>(prop->object);

    // This test is measurably (albeit only slightly) faster than qobject_cast<>()
    const QMetaObject *mo = o->metaObject();
    while (mo && mo != &QGraphicsObject::staticMetaObject) mo = mo->d.superdata;

    if (mo) {
        QGraphicsItemPrivate::get(static_cast<QGraphicsObject *>(o))->setParentItemHelper(that, 0, 0);
    } else {
        o->setParent(that);
    }
}

QObject *QDeclarativeItemPrivate::resources_at(QDeclarativeListProperty<QObject> *prop, int index)
{
    QObjectList children = prop->object->children();
    if (index < children.count())
        return children.at(index);
    else
        return 0;
}

void QDeclarativeItemPrivate::resources_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    o->setParent(prop->object);
}

int QDeclarativeItemPrivate::resources_count(QDeclarativeListProperty<QObject> *prop)
{
    return prop->object->children().count();
}

int QDeclarativeItemPrivate::transform_count(QDeclarativeListProperty<QGraphicsTransform> *list)
{
    QGraphicsObject *object = qobject_cast<QGraphicsObject *>(list->object);
    if (object) {
        QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(object);
        return d->transformData ? d->transformData->graphicsTransforms.size() : 0;
    } else {
        return 0;
    }
}

void QDeclarativeItemPrivate::transform_append(QDeclarativeListProperty<QGraphicsTransform> *list, QGraphicsTransform *item)
{
    QGraphicsObject *object = qobject_cast<QGraphicsObject *>(list->object);
    if (object) // QGraphicsItem applies the list in the wrong order, so we prepend.
        QGraphicsItemPrivate::get(object)->prependGraphicsTransform(item);
}

QGraphicsTransform *QDeclarativeItemPrivate::transform_at(QDeclarativeListProperty<QGraphicsTransform> *list, int idx)
{
    QGraphicsObject *object = qobject_cast<QGraphicsObject *>(list->object);
    if (object) {
        QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(object);
        if (!d->transformData)
            return 0;
        return d->transformData->graphicsTransforms.at(idx);
    } else {
        return 0;
    }
}

void QDeclarativeItemPrivate::transform_clear(QDeclarativeListProperty<QGraphicsTransform> *list)
{
    QGraphicsObject *object = qobject_cast<QGraphicsObject *>(list->object);
    if (object) {
        QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(object);
        if (!d->transformData)
            return;
        object->setTransformations(QList<QGraphicsTransform *>());
    }
}

void QDeclarativeItemPrivate::parentProperty(QObject *o, void *rv, QDeclarativeNotifierEndpoint *e)
{
    QDeclarativeItem *item = static_cast<QDeclarativeItem*>(o);
    if (e)
        e->connect(&item->d_func()->parentNotifier);
    *((QDeclarativeItem **)rv) = item->parentItem();
}

/*!
    \qmlproperty list<Object> Item::data
    \default

    The data property is allows you to freely mix visual children and resources
    of an item.  If you assign a visual item to the data list it becomes
    a child and if you assign any other object type, it is added as a resource.

    So you can write:
    \qml
    Item {
        Text {}
        Rectangle {}
        Timer {}
    }
    \endqml

    instead of:
    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Timer {}
        ]
    }
    \endqml

    data is a behind-the-scenes property: you should never need to explicitly
    specify it.
 */

/*!
    \property QDeclarativeItem::data
    \internal
*/

/*! \internal */
QDeclarativeListProperty<QObject> QDeclarativeItem::data()
{
    return QDeclarativeListProperty<QObject>(this, 0, QDeclarativeItemPrivate::data_append);
}

/*!
    \property QDeclarativeItem::childrenRect
    \brief The geometry of an item's children.

    childrenRect provides an easy way to access the (collective) position and size of the item's children.
*/
QRectF QDeclarativeItem::childrenRect()
{
    Q_D(QDeclarativeItem);
    if (!d->_contents) {
        d->_contents = new QDeclarativeContents;
        QDeclarative_setParent_noEvent(d->_contents, this);
        d->_contents->setItem(this);
    }
    return d->_contents->rectF();
}

bool QDeclarativeItem::clip() const
{
    return flags() & ItemClipsChildrenToShape;
}

void QDeclarativeItem::setClip(bool c)
{
    if (clip() == c)
        return;
    setFlag(ItemClipsChildrenToShape, c);
    emit clipChanged(c);
}

/*!
  \qmlproperty real Item::x
  \qmlproperty real Item::y
  \qmlproperty real Item::width
  \qmlproperty real Item::height

  Defines the item's position and size relative to its parent.

  \qml
  Item { x: 100; y: 100; width: 100; height: 100 }
  \endqml
 */

/*!
  \qmlproperty real Item::z

  Sets the stacking order of the item.  By default the stacking order is 0.

  Items with a higher stacking value are drawn on top of items with a
  lower stacking order.  Items with the same stacking value are drawn
  bottom up in the order they appear.  Items with a negative stacking
  value are drawn under their parent's content.

  The following example shows the various effects of stacking order.

  \table
  \row
  \o \image declarative-item_stacking1.png
  \o Same \c z - later children above earlier children:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking2.png
  \o Higher \c z on top:
  \qml
  Item {
      Rectangle {
          z: 1
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking3.png
  \o Same \c z - children above parents:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking4.png
  \o Lower \c z below:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              z: -1
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \endtable
 */

/*!
    \qmlproperty bool Item::visible

    Whether the item is visible. By default this is true.

    \note visible is not linked to actual visibility; if an item
    moves off screen, or the opacity changes to 0, this will
    not affect the visible property.
*/


/*!
  This function is called to handle this item's changes in
  geometry from \a oldGeometry to \a newGeometry. If the two
  geometries are the same, it doesn't do anything.
 */
void QDeclarativeItem::geometryChanged(const QRectF &newGeometry,
                              const QRectF &oldGeometry)
{
    Q_D(QDeclarativeItem);

    if (d->_anchors)
        d->_anchors->d_func()->updateMe();

    if (transformOrigin() != QDeclarativeItem::TopLeft
        && (newGeometry.width() != oldGeometry.width() || newGeometry.height() != oldGeometry.height())) {
        if (d->transformData) {
            QPointF origin = d->computeTransformOrigin();
            if (transformOriginPoint() != origin)
                setTransformOriginPoint(origin);
        } else {
            d->transformOriginDirty = true;
        }
    }

    if (newGeometry.x() != oldGeometry.x())
        emit xChanged();
    if (newGeometry.width() != oldGeometry.width())
        emit widthChanged();
    if (newGeometry.y() != oldGeometry.y())
        emit yChanged();
    if (newGeometry.height() != oldGeometry.height())
        emit heightChanged();

    for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QDeclarativeItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QDeclarativeItemPrivate::Geometry)
            change.listener->itemGeometryChanged(this, newGeometry, oldGeometry);
    }
}

void QDeclarativeItemPrivate::removeItemChangeListener(QDeclarativeItemChangeListener *listener, ChangeTypes types)
{
    ChangeListener change(listener, types);
    changeListeners.removeOne(change);
}

/*! \internal */
void QDeclarativeItem::keyPressEvent(QKeyEvent *event)
{
    Q_D(QDeclarativeItem);
    if (d->keyHandler)
        d->keyHandler->keyPressed(event);
    else
        event->ignore();
}

/*! \internal */
void QDeclarativeItem::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QDeclarativeItem);
    if (d->keyHandler)
        d->keyHandler->keyReleased(event);
    else
        event->ignore();
}

/*! \internal */
void QDeclarativeItem::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QDeclarativeItem);
    if (d->keyHandler)
        d->keyHandler->inputMethodEvent(event);
    else
        event->ignore();
}

/*! \internal */
QVariant QDeclarativeItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QDeclarativeItem);
    QVariant v;
    if (d->keyHandler)
        v = d->keyHandler->inputMethodQuery(query);

    if (!v.isValid())
        v = QGraphicsObject::inputMethodQuery(query);

    return v;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::left() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->left;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::right() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->right;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::horizontalCenter() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->hCenter;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::top() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->top;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::bottom() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->bottom;
}

/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::verticalCenter() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->vCenter;
}


/*!
    \internal
*/
QDeclarativeAnchorLine QDeclarativeItem::baseline() const
{
    Q_D(const QDeclarativeItem);
    return d->anchorLines()->baseline;
}

/*!
  \property QDeclarativeItem::top
  \internal
*/

/*!
  \property QDeclarativeItem::bottom
  \internal
*/

/*!
  \property QDeclarativeItem::left
  \internal
*/

/*!
  \property QDeclarativeItem::right
  \internal
*/

/*!
  \property QDeclarativeItem::horizontalCenter
  \internal
*/

/*!
  \property QDeclarativeItem::verticalCenter
  \internal
*/

/*!
  \qmlproperty AnchorLine Item::top
  \qmlproperty AnchorLine Item::bottom
  \qmlproperty AnchorLine Item::left
  \qmlproperty AnchorLine Item::right
  \qmlproperty AnchorLine Item::horizontalCenter
  \qmlproperty AnchorLine Item::verticalCenter
  \qmlproperty AnchorLine Item::baseline

  The anchor lines of the item.

  For more information see \l {anchor-layout}{Anchor Layouts}.
*/

/*!
  \qmlproperty AnchorLine Item::anchors.top
  \qmlproperty AnchorLine Item::anchors.bottom
  \qmlproperty AnchorLine Item::anchors.left
  \qmlproperty AnchorLine Item::anchors.right
  \qmlproperty AnchorLine Item::anchors.horizontalCenter
  \qmlproperty AnchorLine Item::anchors.verticalCenter
  \qmlproperty AnchorLine Item::anchors.baseline

  \qmlproperty Item Item::anchors.fill
  \qmlproperty Item Item::anchors.centerIn

  \qmlproperty real Item::anchors.margins
  \qmlproperty real Item::anchors.topMargin
  \qmlproperty real Item::anchors.bottomMargin
  \qmlproperty real Item::anchors.leftMargin
  \qmlproperty real Item::anchors.rightMargin
  \qmlproperty real Item::anchors.horizontalCenterOffset
  \qmlproperty real Item::anchors.verticalCenterOffset
  \qmlproperty real Item::anchors.baselineOffset

  Anchors provide a way to position an item by specifying its
  relationship with other items.

  Margins apply to top, bottom, left, right, and fill anchors.
  The margins property can be used to set all of the various margins at once, to the same value.

  Offsets apply for horizontal center, vertical center, and baseline anchors.

  \table
  \row
  \o \image declarative-anchors_example.png
  \o Text anchored to Image, horizontally centered and vertically below, with a margin.
  \qml
  Image { id: pic; ... }
  Text {
      id: label
      anchors.horizontalCenter: pic.horizontalCenter
      anchors.top: pic.bottom
      anchors.topMargin: 5
      ...
  }
  \endqml
  \row
  \o \image declarative-anchors_example2.png
  \o
  Left of Text anchored to right of Image, with a margin. The y
  property of both defaults to 0.

  \qml
    Image { id: pic; ... }
    Text {
        id: label
        anchors.left: pic.right
        anchors.leftMargin: 5
        ...
    }
  \endqml
  \endtable

  anchors.fill provides a convenient way for one item to have the
  same geometry as another item, and is equivalent to connecting all
  four directional anchors.

  \note You can only anchor an item to siblings or a parent.

  For more information see \l {anchor-layout}{Anchor Layouts}.
*/

/*!
  \property QDeclarativeItem::baselineOffset
  \brief The position of the item's baseline in local coordinates.

  The baseline of a Text item is the imaginary line on which the text
  sits. Controls containing text usually set their baseline to the
  baseline of their text.

  For non-text items, a default baseline offset of 0 is used.
*/
qreal QDeclarativeItem::baselineOffset() const
{
    Q_D(const QDeclarativeItem);
    if (!d->_baselineOffset.isValid()) {
        return 0.0;
    } else
        return d->_baselineOffset;
}

void QDeclarativeItem::setBaselineOffset(qreal offset)
{
    Q_D(QDeclarativeItem);
    if (offset == d->_baselineOffset)
        return;

    d->_baselineOffset = offset;

    for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QDeclarativeItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QDeclarativeItemPrivate::Geometry) {
            QDeclarativeAnchorsPrivate *anchor = change.listener->anchorPrivate();
            if (anchor)
                anchor->updateVerticalAnchors();
        }
    }
    emit baselineOffsetChanged(offset);
}

/*!
  \qmlproperty real Item::rotation
  This property holds the rotation of the item in degrees clockwise.

  This specifies how many degrees to rotate the item around its transformOrigin.
  The default rotation is 0 degrees (i.e. not rotated at all).

  \table
  \row
  \o \image declarative-rotation.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          rotation: 30
      }
  }
  \endqml
  \endtable
*/

/*!
  \qmlproperty real Item::scale
  This property holds the scale of the item.

  A scale of less than 1 means the item will be displayed smaller than
  normal, and a scale of greater than 1 means the item will be
  displayed larger than normal.  A negative scale means the item will
  be mirrored.

  By default, items are displayed at a scale of 1 (i.e. at their
  normal size).

  Scaling is from the item's transformOrigin.

  \table
  \row
  \o \image declarative-scale.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "green"
          width: 25; height: 25
      }
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          scale: 1.4
      }
  }
  \endqml
  \endtable
*/

/*!
  \qmlproperty real Item::opacity

  The opacity of the item.  Opacity is specified as a number between 0
  (fully transparent) and 1 (fully opaque).  The default is 1.

  Opacity is an \e inherited attribute.  That is, the opacity is
  also applied individually to child items.  In almost all cases this
  is what you want, but in some cases (like the following example)
  it may produce undesired results.

  \table
  \row
  \o \image declarative-item_opacity1.png
  \o
  \qml
    Item {
        Rectangle {
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \row
  \o \image declarative-item_opacity2.png
  \o
  \qml
    Item {
        Rectangle {
            opacity: 0.5
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \endtable
*/

/*!
  Returns a value indicating whether mouse input should
  remain with this item exclusively.

  \sa setKeepMouseGrab()
 */
bool QDeclarativeItem::keepMouseGrab() const
{
    Q_D(const QDeclarativeItem);
    return d->_keepMouse;
}

/*!
  The flag indicating whether the mouse should remain
  with this item is set to \a keep.

  This is useful for items that wish to grab and keep mouse
  interaction following a predefined gesture.  For example,
  an item that is interested in horizontal mouse movement
  may set keepMouseGrab to true once a threshold has been
  exceeded.  Once keepMouseGrab has been set to true, filtering
  items will not react to mouse events.

  If the item does not indicate that it wishes to retain mouse grab,
  a filtering item may steal the grab. For example, Flickable may attempt
  to steal a mouse grab if it detects that the user has begun to
  move the viewport.

  \sa keepMouseGrab()
 */
void QDeclarativeItem::setKeepMouseGrab(bool keep)
{
    Q_D(QDeclarativeItem);
    d->_keepMouse = keep;
}

/*!
    \qmlmethod object Item::mapFromItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in \a item's coordinate system, to
    this item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped cooordinate.

    If \a item is a \c null value, this maps the point from the coordinate
    system of the root QML view.
*/
QScriptValue QDeclarativeItem::mapFromItem(const QScriptValue &item, qreal x, qreal y) const
{
    QScriptValue sv = QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(this))->newObject();
    QDeclarativeItem *itemObj = qobject_cast<QDeclarativeItem*>(item.toQObject());
    if (!itemObj && !item.isNull()) {
        qmlInfo(this) << "mapFromItem() given argument \"" << item.toString() << "\" which is neither null nor an Item";
        return 0;
    }

    // If QGraphicsItem::mapFromItem() is called with 0, behaves the same as mapFromScene()
    QPointF p = qobject_cast<QGraphicsItem*>(this)->mapFromItem(itemObj, x, y);
    sv.setProperty(QLatin1String("x"), p.x());
    sv.setProperty(QLatin1String("y"), p.y());
    return sv;
}

/*!
    \qmlmethod object Item::mapToItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in this item's coordinate system, to
    \a item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped cooordinate.

    If \a item is a \c null value, this maps \a x and \a y to the coordinate
    system of the root QML view.
*/
QScriptValue QDeclarativeItem::mapToItem(const QScriptValue &item, qreal x, qreal y) const
{
    QScriptValue sv = QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(this))->newObject();
    QDeclarativeItem *itemObj = qobject_cast<QDeclarativeItem*>(item.toQObject());
    if (!itemObj && !item.isNull()) {
        qmlInfo(this) << "mapToItem() given argument \"" << item.toString() << "\" which is neither null nor an Item";
        return 0;
    }

    // If QGraphicsItem::mapToItem() is called with 0, behaves the same as mapToScene()
    QPointF p = qobject_cast<QGraphicsItem*>(this)->mapToItem(itemObj, x, y);
    sv.setProperty(QLatin1String("x"), p.x());
    sv.setProperty(QLatin1String("y"), p.y());
    return sv;
}

/*!
    \qmlmethod Item::forceFocus()

    Force the focus on the item.
    This method sets the focus on the item and makes sure that all the focus scopes higher in the object hierarchy are given focus.
*/
void QDeclarativeItem::forceFocus()
{
    setFocus(true);
    QGraphicsItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
            parent->setFocus(Qt::OtherFocusReason);
        parent = parent->parentItem();
    }
}

void QDeclarativeItemPrivate::focusChanged(bool flag)
{
    Q_Q(QDeclarativeItem);
    emit q->focusChanged(flag);
}

/*! \internal */
QDeclarativeListProperty<QObject> QDeclarativeItem::resources()
{
    return QDeclarativeListProperty<QObject>(this, 0, QDeclarativeItemPrivate::resources_append,
                                             QDeclarativeItemPrivate::resources_count,
                                             QDeclarativeItemPrivate::resources_at);
}

/*!
  \qmlproperty list<State> Item::states
  This property holds a list of states defined by the item.

  \qml
  Item {
    states: [
      State { ... },
      State { ... }
      ...
    ]
  }
  \endqml

  \sa {qmlstate}{States}
*/

/*!
  \property QDeclarativeItem::states
  \internal
*/
/*! \internal */
QDeclarativeListProperty<QDeclarativeState> QDeclarativeItem::states()
{
    Q_D(QDeclarativeItem);
    return d->states()->statesProperty();
}

/*!
  \qmlproperty list<Transition> Item::transitions
  This property holds a list of transitions defined by the item.

  \qml
  Item {
    transitions: [
      Transition { ... },
      Transition { ... }
      ...
    ]
  }
  \endqml

  \sa {state-transitions}{Transitions}
*/

/*!
  \property QDeclarativeItem::transitions
  \internal
*/

/*! \internal */
QDeclarativeListProperty<QDeclarativeTransition> QDeclarativeItem::transitions()
{
    Q_D(QDeclarativeItem);
    return d->states()->transitionsProperty();
}

/*
  \qmlproperty list<Filter> Item::filter
  This property holds a list of graphical filters to be applied to the item.

  \l {Filter}{Filters} include things like \l {Blur}{blurring}
  the item, or giving it a \l Reflection.  Some
  filters may not be available on all canvases; if a filter is not
  available on a certain canvas, it will simply not be applied for
  that canvas (but the QML will still be considered valid).

  \qml
  Item {
    filter: [
      Blur { ... },
      Relection { ... }
      ...
    ]
  }
  \endqml
*/

/*!
  \qmlproperty bool Item::clip
  This property holds whether clipping is enabled.

  if clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \property QDeclarativeItem::clip
  This property holds whether clipping is enabled.

  if clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \qmlproperty string Item::state

  This property holds the name of the current state of the item.

  This property is often used in scripts to change between states. For
  example:

  \qml
    function toggle() {
        if (button.state == 'On')
            button.state = 'Off';
        else
            button.state = 'On';
    }
  \endqml

  If the item is in its base state (i.e. no explicit state has been
  set), \c state will be a blank string. Likewise, you can return an
  item to its base state by setting its current state to \c ''.

  \sa {qmlstates}{States}
*/

/*!
  \property QDeclarativeItem::state
  \internal
*/

/*! \internal */
QString QDeclarativeItem::state() const
{
    Q_D(const QDeclarativeItem);
    if (!d->_stateGroup)
        return QString();
    else
        return d->_stateGroup->state();
}

/*! \internal */
void QDeclarativeItem::setState(const QString &state)
{
    Q_D(QDeclarativeItem);
    d->states()->setState(state);
}

/*!
  \qmlproperty list<Transform> Item::transform
  This property holds the list of transformations to apply.

  For more information see \l Transform.
*/

/*!
  \property QDeclarativeItem::transform
  \internal
*/

/*! \internal */
QDeclarativeListProperty<QGraphicsTransform> QDeclarativeItem::transform()
{
    Q_D(QDeclarativeItem);
    return QDeclarativeListProperty<QGraphicsTransform>(this, 0, d->transform_append, d->transform_count,
                                               d->transform_at, d->transform_clear);
}

/*!
  \internal

  classBegin() is called when the item is constructed, but its
  properties have not yet been set.

  \sa componentComplete(), isComponentComplete()
*/
void QDeclarativeItem::classBegin()
{
    Q_D(QDeclarativeItem);
    d->_componentComplete = false;
    if (d->_stateGroup)
        d->_stateGroup->classBegin();
    if (d->_anchors)
        d->_anchors->classBegin();
}

/*!
  \internal

  componentComplete() is called when all items in the component
  have been constructed.  It is often desireable to delay some
  processing until the component is complete an all bindings in the
  component have been resolved.
*/
void QDeclarativeItem::componentComplete()
{
    Q_D(QDeclarativeItem);
    d->_componentComplete = true;
    if (d->_stateGroup)
        d->_stateGroup->componentComplete();
    if (d->_anchors) {
        d->_anchors->componentComplete();
        d->_anchors->d_func()->updateOnComplete();
    }
    if (d->keyHandler)
        d->keyHandler->componentComplete();
}

QDeclarativeStateGroup *QDeclarativeItemPrivate::states()
{
    Q_Q(QDeclarativeItem);
    if (!_stateGroup) {
        _stateGroup = new QDeclarativeStateGroup;
        if (!_componentComplete)
            _stateGroup->classBegin();
        QObject::connect(_stateGroup, SIGNAL(stateChanged(QString)),
                         q, SIGNAL(stateChanged(QString)));
    }

    return _stateGroup;
}

QDeclarativeItemPrivate::AnchorLines::AnchorLines(QGraphicsObject *q)
{
    left.item = q;
    left.anchorLine = QDeclarativeAnchorLine::Left;
    right.item = q;
    right.anchorLine = QDeclarativeAnchorLine::Right;
    hCenter.item = q;
    hCenter.anchorLine = QDeclarativeAnchorLine::HCenter;
    top.item = q;
    top.anchorLine = QDeclarativeAnchorLine::Top;
    bottom.item = q;
    bottom.anchorLine = QDeclarativeAnchorLine::Bottom;
    vCenter.item = q;
    vCenter.anchorLine = QDeclarativeAnchorLine::VCenter;
    baseline.item = q;
    baseline.anchorLine = QDeclarativeAnchorLine::Baseline;
}

QPointF QDeclarativeItemPrivate::computeTransformOrigin() const
{
    Q_Q(const QDeclarativeItem);

    QRectF br = q->boundingRect();

    switch(origin) {
    default:
    case QDeclarativeItem::TopLeft:
        return QPointF(0, 0);
    case QDeclarativeItem::Top:
        return QPointF(br.width() / 2., 0);
    case QDeclarativeItem::TopRight:
        return QPointF(br.width(), 0);
    case QDeclarativeItem::Left:
        return QPointF(0, br.height() / 2.);
    case QDeclarativeItem::Center:
        return QPointF(br.width() / 2., br.height() / 2.);
    case QDeclarativeItem::Right:
        return QPointF(br.width(), br.height() / 2.);
    case QDeclarativeItem::BottomLeft:
        return QPointF(0, br.height());
    case QDeclarativeItem::Bottom:
        return QPointF(br.width() / 2., br.height());
    case QDeclarativeItem::BottomRight:
        return QPointF(br.width(), br.height());
    }
}

/*! \internal */
bool QDeclarativeItem::sceneEvent(QEvent *event)
{
    Q_D(QDeclarativeItem);
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *k = static_cast<QKeyEvent *>(event);
        if ((k->key() == Qt::Key_Tab || k->key() == Qt::Key_Backtab) &&
            !(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            keyPressEvent(static_cast<QKeyEvent *>(event));
            if (!event->isAccepted())
                return QGraphicsItem::sceneEvent(event);
            else
                return true;
        } else {
            return QGraphicsItem::sceneEvent(event);
        }
    } else {
        bool rv = QGraphicsItem::sceneEvent(event);

        if (event->type() == QEvent::FocusIn ||
            event->type() == QEvent::FocusOut) {
            d->focusChanged(hasFocus());
        }
        return rv;
    }
}

/*! \internal */
QVariant QDeclarativeItem::itemChange(GraphicsItemChange change,
                                       const QVariant &value)
{
    Q_D(QDeclarativeItem);
    switch (change) {
    case ItemParentHasChanged:
        emit parentChanged(parentItem());
        d->parentNotifier.notify();
        break;
    case ItemVisibleHasChanged: {
            for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
                const QDeclarativeItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
                if (change.types & QDeclarativeItemPrivate::Visibility) {
                    change.listener->itemVisibilityChanged(this);
                }
            }
        }
        break;
    case ItemOpacityHasChanged: {
            for(int ii = 0; ii < d->changeListeners.count(); ++ii) {
                const QDeclarativeItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
                if (change.types & QDeclarativeItemPrivate::Opacity) {
                    change.listener->itemOpacityChanged(this);
                }
            }
        }
        break;
    default:
        break;
    }

    return QGraphicsItem::itemChange(change, value);
}

/*! \internal */
QRectF QDeclarativeItem::boundingRect() const
{
    Q_D(const QDeclarativeItem);
    return QRectF(0, 0, d->mWidth, d->mHeight);
}

/*!
    \enum QDeclarativeItem::TransformOrigin

    Controls the point about which simple transforms like scale apply.

    \value TopLeft The top-left corner of the item.
    \value Top The center point of the top of the item.
    \value TopRight The top-right corner of the item.
    \value Left The left most point of the vertical middle.
    \value Center The center of the item.
    \value Right The right most point of the vertical middle.
    \value BottomLeft The bottom-left corner of the item.
    \value Bottom The center point of the bottom of the item.
    \value BottomRight The bottom-right corner of the item.
*/

/*!
    Returns the current transform origin.
*/
QDeclarativeItem::TransformOrigin QDeclarativeItem::transformOrigin() const
{
    Q_D(const QDeclarativeItem);
    return d->origin;
}

/*!
    Set the transform \a origin.
*/
void QDeclarativeItem::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QDeclarativeItem);
    if (origin != d->origin) {
        d->origin = origin;
        if (d->transformData)
            QGraphicsItem::setTransformOriginPoint(d->computeTransformOrigin());
        else
            d->transformOriginDirty = true;
        emit transformOriginChanged(d->origin);
    }
}

void QDeclarativeItemPrivate::transformChanged()
{
    Q_Q(QDeclarativeItem);
    if (transformOriginDirty) {
        q->QGraphicsItem::setTransformOriginPoint(computeTransformOrigin());
        transformOriginDirty = false;
    }
}

/*!
    \property QDeclarativeItem::smooth
    \brief whether the item is smoothly transformed.

    This property is provided purely for the purpose of optimization. Turning
    smooth transforms off is faster, but looks worse; turning smooth
    transformations on is slower, but looks better.

    By default smooth transformations are off.
*/

/*!
    Returns true if the item should be drawn with antialiasing and
    smooth pixmap filtering, false otherwise.

    The default is false.

    \sa setSmooth()
*/
bool QDeclarativeItem::smooth() const
{
    Q_D(const QDeclarativeItem);
    return d->smooth;
}

/*!
    Sets whether the item should be drawn with antialiasing and
    smooth pixmap filtering to \a smooth.

    \sa smooth()
*/
void QDeclarativeItem::setSmooth(bool smooth)
{
    Q_D(QDeclarativeItem);
    if (d->smooth == smooth)
        return;
    d->smooth = smooth;
    emit smoothChanged(smooth);
    update();
}

qreal QDeclarativeItem::width() const
{
    Q_D(const QDeclarativeItem);
    return d->width();
}

void QDeclarativeItem::setWidth(qreal w)
{
    Q_D(QDeclarativeItem);
    d->setWidth(w);
}

void QDeclarativeItem::resetWidth()
{
    Q_D(QDeclarativeItem);
    d->resetWidth();
}

qreal QDeclarativeItemPrivate::width() const
{
    return mWidth;
}

void QDeclarativeItemPrivate::setWidth(qreal w)
{
    Q_Q(QDeclarativeItem);
    if (qIsNaN(w))
        return;

    widthValid = true;
    if (mWidth == w)
        return;

    qreal oldWidth = mWidth;

    q->prepareGeometryChange();
    mWidth = w;

    q->geometryChanged(QRectF(q->x(), q->y(), width(), height()),
                    QRectF(q->x(), q->y(), oldWidth, height()));
}

void QDeclarativeItemPrivate    ::resetWidth()
{
    Q_Q(QDeclarativeItem);
    widthValid = false;
    q->setImplicitWidth(q->implicitWidth());
}

/*!
    Returns the width of the item that is implied by other properties that determine the content.
*/
qreal QDeclarativeItem::implicitWidth() const
{
    Q_D(const QDeclarativeItem);
    return d->implicitWidth;
}

/*!
    Sets the implied width of the item to \a w.
    This is the width implied by other properties that determine the content.
*/
void QDeclarativeItem::setImplicitWidth(qreal w)
{
    Q_D(QDeclarativeItem);
    d->implicitWidth = w;
    if (d->mWidth == w || widthValid())
        return;

    qreal oldWidth = d->mWidth;

    prepareGeometryChange();
    d->mWidth = w;

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, height()));
}

/*!
    Returns whether the width property has been set explicitly.
*/
bool QDeclarativeItem::widthValid() const
{
    Q_D(const QDeclarativeItem);
    return d->widthValid;
}

qreal QDeclarativeItem::height() const
{
    Q_D(const QDeclarativeItem);
    return d->height();
}

void QDeclarativeItem::setHeight(qreal h)
{
    Q_D(QDeclarativeItem);
    d->setHeight(h);
}

void QDeclarativeItem::resetHeight()
{
    Q_D(QDeclarativeItem);
    d->resetHeight();
}

qreal QDeclarativeItemPrivate::height() const
{
    return mHeight;
}

void QDeclarativeItemPrivate::setHeight(qreal h)
{
    Q_Q(QDeclarativeItem);
    if (qIsNaN(h))
        return;

    heightValid = true;
    if (mHeight == h)
        return;

    qreal oldHeight = mHeight;

    q->prepareGeometryChange();
    mHeight = h;

    q->geometryChanged(QRectF(q->x(), q->y(), width(), height()),
                    QRectF(q->x(), q->y(), width(), oldHeight));
}

void QDeclarativeItemPrivate::resetHeight()
{
    Q_Q(QDeclarativeItem);
    heightValid = false;
    q->setImplicitHeight(q->implicitHeight());
}

/*!
    Returns the height of the item that is implied by other properties that determine the content.
*/
qreal QDeclarativeItem::implicitHeight() const
{
    Q_D(const QDeclarativeItem);
    return d->implicitHeight;
}

/*!
    Sets the implied height of the item to \a h.
    This is the height implied by other properties that determine the content.
*/
void QDeclarativeItem::setImplicitHeight(qreal h)
{
    Q_D(QDeclarativeItem);
    d->implicitHeight = h;
    if (d->mHeight == h || heightValid())
        return;

    qreal oldHeight = d->mHeight;

    prepareGeometryChange();
    d->mHeight = h;

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), width(), oldHeight));
}

/*!
    Returns whether the height property has been set explicitly.
*/
bool QDeclarativeItem::heightValid() const
{
    Q_D(const QDeclarativeItem);
    return d->heightValid;
}

/*! \internal */
void QDeclarativeItem::setSize(const QSizeF &size)
{
    Q_D(QDeclarativeItem);
    d->heightValid = true;
    d->widthValid = true;

    if (d->height() == size.height() && d->width() == size.width())
        return;

    qreal oldHeight = d->height();
    qreal oldWidth = d->width();

    prepareGeometryChange();
    d->setHeight(size.height());
    d->setWidth(size.width());

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, oldHeight));
}

/*!
  \qmlproperty bool Item::wantsFocus

  This property indicates whether the item has has an active focus request.

  \sa {qmlfocus}{Keyboard Focus}
*/

/*! \internal */
bool QDeclarativeItem::wantsFocus() const
{
    return focusItem() != 0;
}

/*!
  \qmlproperty bool Item::focus
  This property indicates whether the item has keyboard input focus. Set this
  property to true to request focus.

  \sa {qmlfocus}{Keyboard Focus}
*/

/*! \internal */
bool QDeclarativeItem::hasFocus() const
{
    return QGraphicsItem::hasFocus();
}

/*! \internal */
void QDeclarativeItem::setFocus(bool focus)
{
    if (focus)
        QGraphicsItem::setFocus(Qt::OtherFocusReason);
    else
        QGraphicsItem::clearFocus();
}

/*!
    \reimp
    \internal
*/
void QDeclarativeItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

/*!
    \reimp
    \internal
*/
bool QDeclarativeItem::event(QEvent *ev)
{
    return QGraphicsObject::event(ev);
}

QDebug operator<<(QDebug debug, QDeclarativeItem *item)
{
    if (!item) {
        debug << "QDeclarativeItem(0)";
        return debug;
    }

    debug << item->metaObject()->className() << "(this =" << ((void*)item)
          << ", parent =" << ((void*)item->parentItem())
          << ", geometry =" << QRectF(item->pos(), QSizeF(item->width(), item->height()))
          << ", z =" << item->zValue() << ')';
    return debug;
}

int QDeclarativeItemPrivate::consistentTime = -1;
void QDeclarativeItemPrivate::setConsistentTime(int t)
{
    consistentTime = t;
}

QTime QDeclarativeItemPrivate::currentTime()
{
    if (consistentTime == -1)
        return QTime::currentTime();
    else
        return QTime(0, 0).addMSecs(consistentTime);
}

void QDeclarativeItemPrivate::start(QTime &t)
{
    t = currentTime();
}

int QDeclarativeItemPrivate::elapsed(QTime &t)
{
    int n = t.msecsTo(currentTime());
    if (n < 0)                                // passed midnight
        n += 86400 * 1000;
    return n;
}

int QDeclarativeItemPrivate::restart(QTime &t)
{
    QTime time = currentTime();
    int n = t.msecsTo(time);
    if (n < 0)                                // passed midnight
        n += 86400*1000;
    t = time;
    return n;
}

QT_END_NAMESPACE

#include <moc_qdeclarativeitem.cpp>
