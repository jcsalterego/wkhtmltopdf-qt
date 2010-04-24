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

#ifndef QDECLARATIVEITEM_P_H
#define QDECLARATIVEITEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qdeclarativeitem.h"

#include "private/qdeclarativeanchors_p.h"
#include "private/qdeclarativeanchors_p_p.h"
#include "private/qdeclarativeitemchangelistener_p.h"
#include <private/qpodvector_p.h>

#include <private/qdeclarativestate_p.h>
#include <private/qdeclarativenullablevalue_p_p.h>
#include <private/qdeclarativenotifier_p.h>
#include <private/qdeclarativeglobal_p.h>

#include <qdeclarative.h>
#include <qdeclarativecontext.h>

#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>

#include <private/qgraphicsitem_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class QDeclarativeItemKeyFilter;

//### merge into private?
class QDeclarativeContents : public QObject
{
    Q_OBJECT
public:
    QDeclarativeContents();

    QRectF rectF() const;

    void setItem(QDeclarativeItem *item);

public Q_SLOTS:
    void calcHeight();
    void calcWidth();

Q_SIGNALS:
    void rectChanged(QRectF);

private:
    QDeclarativeItem *m_item;
    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;
};

class Q_DECLARATIVE_EXPORT QDeclarativeItemPrivate : public QGraphicsItemPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeItem)

public:
    QDeclarativeItemPrivate()
    : _anchors(0), _contents(0),
      _baselineOffset(0),
      _anchorLines(0),
      _stateGroup(0), origin(QDeclarativeItem::Center),
      widthValid(false), heightValid(false),
      _componentComplete(true), _keepMouse(false),
      smooth(false), transformOriginDirty(true), keyHandler(0),
      mWidth(0), mHeight(0), implicitWidth(0), implicitHeight(0)
    {
        QGraphicsItemPrivate::acceptedMouseButtons = 0;
        isDeclarativeItem = 1;
        QGraphicsItemPrivate::flags = QGraphicsItem::GraphicsItemFlags(
                                      QGraphicsItem::ItemHasNoContents
                                      | QGraphicsItem::ItemIsFocusable
                                      | QGraphicsItem::ItemNegativeZStacksBehindParent);

    }

    void init(QDeclarativeItem *parent)
    {
        Q_Q(QDeclarativeItem);
        if (parent) {
            QDeclarative_setParent_noEvent(q, parent);
            q->setParentItem(parent);
        }
        _baselineOffset.invalidate();
        mouseSetsFocus = false;
    }

    QString _id;

    // Private Properties
    qreal width() const;
    void setWidth(qreal);
    void resetWidth();

    qreal height() const;
    void setHeight(qreal);
    void resetHeight();


    // data property
    static void data_append(QDeclarativeListProperty<QObject> *, QObject *);

    // resources property
    static QObject *resources_at(QDeclarativeListProperty<QObject> *, int);
    static void resources_append(QDeclarativeListProperty<QObject> *, QObject *);
    static int resources_count(QDeclarativeListProperty<QObject> *);

    // transform property
    static int transform_count(QDeclarativeListProperty<QGraphicsTransform> *list);
    static void transform_append(QDeclarativeListProperty<QGraphicsTransform> *list, QGraphicsTransform *);
    static QGraphicsTransform *transform_at(QDeclarativeListProperty<QGraphicsTransform> *list, int);
    static void transform_clear(QDeclarativeListProperty<QGraphicsTransform> *list);

    // Accelerated property accessors
    QDeclarativeNotifier parentNotifier;
    static void parentProperty(QObject *o, void *rv, QDeclarativeNotifierEndpoint *e);

    QDeclarativeAnchors *anchors() {
        if (!_anchors) {
            Q_Q(QDeclarativeItem);
            _anchors = new QDeclarativeAnchors(q);
            if (!_componentComplete)
                _anchors->classBegin();
        }
        return _anchors;
    }
    QDeclarativeAnchors *_anchors;
    QDeclarativeContents *_contents;

    QDeclarativeNullableValue<qreal> _baselineOffset;

    struct AnchorLines {
        AnchorLines(QGraphicsObject *);
        QDeclarativeAnchorLine left;
        QDeclarativeAnchorLine right;
        QDeclarativeAnchorLine hCenter;
        QDeclarativeAnchorLine top;
        QDeclarativeAnchorLine bottom;
        QDeclarativeAnchorLine vCenter;
        QDeclarativeAnchorLine baseline;
    };
    mutable AnchorLines *_anchorLines;
    AnchorLines *anchorLines() const {
        Q_Q(const QDeclarativeItem);
        if (!_anchorLines) _anchorLines =
            new AnchorLines(const_cast<QDeclarativeItem *>(q));
        return _anchorLines;
    }

    enum ChangeType {
        Geometry = 0x01,
        SiblingOrder = 0x02,
        Visibility = 0x04,
        Opacity = 0x08,
        Destroyed = 0x10
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    struct ChangeListener {
        ChangeListener(QDeclarativeItemChangeListener *l, QDeclarativeItemPrivate::ChangeTypes t) : listener(l), types(t) {}
        QDeclarativeItemChangeListener *listener;
        QDeclarativeItemPrivate::ChangeTypes types;
        bool operator==(const ChangeListener &other) const { return listener == other.listener && types == other.types; }
    };

    void addItemChangeListener(QDeclarativeItemChangeListener *listener, ChangeTypes types) {
        changeListeners.append(ChangeListener(listener, types));
    }
    void removeItemChangeListener(QDeclarativeItemChangeListener *, ChangeTypes types);
    QPODVector<ChangeListener,4> changeListeners;

    QDeclarativeStateGroup *states();
    QDeclarativeStateGroup *_stateGroup;

    QDeclarativeItem::TransformOrigin origin:4;
    bool widthValid:1;
    bool heightValid:1;
    bool _componentComplete:1;
    bool _keepMouse:1;
    bool smooth:1;
    bool transformOriginDirty : 1;

    QDeclarativeItemKeyFilter *keyHandler;

    qreal mWidth;
    qreal mHeight;
    qreal implicitWidth;
    qreal implicitHeight;

    QPointF computeTransformOrigin() const;

    virtual void setPosHelper(const QPointF &pos)
    {
        Q_Q(QDeclarativeItem);
        QRectF oldGeometry(this->pos.x(), this->pos.y(), mWidth, mHeight);
        QGraphicsItemPrivate::setPosHelper(pos);
        q->geometryChanged(QRectF(this->pos.x(), this->pos.y(), mWidth, mHeight), oldGeometry);
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void subFocusItemChange()
    {
        emit q_func()->wantsFocusChanged(subFocusItem != 0);
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void siblingOrderChange()
    {
        Q_Q(QDeclarativeItem);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QDeclarativeItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QDeclarativeItemPrivate::SiblingOrder) {
                change.listener->itemSiblingOrderChanged(q);
            }
        }
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void transformChanged();

    virtual void focusChanged(bool);

    static int consistentTime;
    static QTime currentTime();
    static void setConsistentTime(int t);
    static void start(QTime &);
    static int elapsed(QTime &);
    static int restart(QTime &);
};

/*
    Key filters can be installed on a QDeclarativeItem, but not removed.  Currently they
    are only used by attached objects (which are only destroyed on Item
    destruction), so this isn't a problem.  If in future this becomes any form
    of public API, they will have to support removal too.
*/
class QDeclarativeItemKeyFilter
{
public:
    QDeclarativeItemKeyFilter(QDeclarativeItem * = 0);
    virtual ~QDeclarativeItemKeyFilter();

    virtual void keyPressed(QKeyEvent *event);
    virtual void keyReleased(QKeyEvent *event);
    virtual void inputMethodEvent(QInputMethodEvent *event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    virtual void componentComplete();

private:
    QDeclarativeItemKeyFilter *m_next;
};

class QDeclarativeKeyNavigationAttachedPrivate : public QObjectPrivate
{
public:
    QDeclarativeKeyNavigationAttachedPrivate()
        : QObjectPrivate(), left(0), right(0), up(0), down(0), tab(0), backtab(0) {}

    QDeclarativeItem *left;
    QDeclarativeItem *right;
    QDeclarativeItem *up;
    QDeclarativeItem *down;
    QDeclarativeItem *tab;
    QDeclarativeItem *backtab;
};

class QDeclarativeKeyNavigationAttached : public QObject, public QDeclarativeItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeKeyNavigationAttached)

    Q_PROPERTY(QDeclarativeItem *left READ left WRITE setLeft NOTIFY changed)
    Q_PROPERTY(QDeclarativeItem *right READ right WRITE setRight NOTIFY changed)
    Q_PROPERTY(QDeclarativeItem *up READ up WRITE setUp NOTIFY changed)
    Q_PROPERTY(QDeclarativeItem *down READ down WRITE setDown NOTIFY changed)
    Q_PROPERTY(QDeclarativeItem *tab READ tab WRITE setTab NOTIFY changed)
    Q_PROPERTY(QDeclarativeItem *backtab READ backtab WRITE setBacktab NOTIFY changed)

public:
    QDeclarativeKeyNavigationAttached(QObject * = 0);

    QDeclarativeItem *left() const;
    void setLeft(QDeclarativeItem *);
    QDeclarativeItem *right() const;
    void setRight(QDeclarativeItem *);
    QDeclarativeItem *up() const;
    void setUp(QDeclarativeItem *);
    QDeclarativeItem *down() const;
    void setDown(QDeclarativeItem *);
    QDeclarativeItem *tab() const;
    void setTab(QDeclarativeItem *);
    QDeclarativeItem *backtab() const;
    void setBacktab(QDeclarativeItem *);

    static QDeclarativeKeyNavigationAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void changed();

private:
    virtual void keyPressed(QKeyEvent *event);
    virtual void keyReleased(QKeyEvent *event);
};

class QDeclarativeKeysAttachedPrivate : public QObjectPrivate
{
public:
    QDeclarativeKeysAttachedPrivate()
        : QObjectPrivate(), inPress(false), inRelease(false)
        , inIM(false), enabled(true), imeItem(0), item(0)
    {}

    bool isConnected(const char *signalName);

    QGraphicsItem *finalFocusProxy(QGraphicsItem *item) const
    {
        QGraphicsItem *fp;
        while ((fp = item->focusProxy()))
            item = fp;
        return item;
    }

    //loop detection
    bool inPress:1;
    bool inRelease:1;
    bool inIM:1;

    bool enabled : 1;

    QGraphicsItem *imeItem;
    QList<QDeclarativeItem *> targets;
    QDeclarativeItem *item;
};

class QDeclarativeKeysAttached : public QObject, public QDeclarativeItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeKeysAttached)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeItem> forwardTo READ forwardTo)

public:
    QDeclarativeKeysAttached(QObject *parent=0);
    ~QDeclarativeKeysAttached();

    bool enabled() const { Q_D(const QDeclarativeKeysAttached); return d->enabled; }
    void setEnabled(bool enabled) {
        Q_D(QDeclarativeKeysAttached);
        if (enabled != d->enabled) {
            d->enabled = enabled;
            emit enabledChanged();
        }
    }

    QDeclarativeListProperty<QDeclarativeItem> forwardTo() {
        Q_D(QDeclarativeKeysAttached);
        return QDeclarativeListProperty<QDeclarativeItem>(this, d->targets);
    }

    virtual void componentComplete();

    static QDeclarativeKeysAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void enabledChanged();
    void pressed(QDeclarativeKeyEvent *event);
    void released(QDeclarativeKeyEvent *event);
    void digit0Pressed(QDeclarativeKeyEvent *event);
    void digit1Pressed(QDeclarativeKeyEvent *event);
    void digit2Pressed(QDeclarativeKeyEvent *event);
    void digit3Pressed(QDeclarativeKeyEvent *event);
    void digit4Pressed(QDeclarativeKeyEvent *event);
    void digit5Pressed(QDeclarativeKeyEvent *event);
    void digit6Pressed(QDeclarativeKeyEvent *event);
    void digit7Pressed(QDeclarativeKeyEvent *event);
    void digit8Pressed(QDeclarativeKeyEvent *event);
    void digit9Pressed(QDeclarativeKeyEvent *event);

    void leftPressed(QDeclarativeKeyEvent *event);
    void rightPressed(QDeclarativeKeyEvent *event);
    void upPressed(QDeclarativeKeyEvent *event);
    void downPressed(QDeclarativeKeyEvent *event);
    void tabPressed(QDeclarativeKeyEvent *event);
    void backtabPressed(QDeclarativeKeyEvent *event);

    void asteriskPressed(QDeclarativeKeyEvent *event);
    void numberSignPressed(QDeclarativeKeyEvent *event);
    void escapePressed(QDeclarativeKeyEvent *event);
    void returnPressed(QDeclarativeKeyEvent *event);
    void enterPressed(QDeclarativeKeyEvent *event);
    void deletePressed(QDeclarativeKeyEvent *event);
    void spacePressed(QDeclarativeKeyEvent *event);
    void backPressed(QDeclarativeKeyEvent *event);
    void cancelPressed(QDeclarativeKeyEvent *event);
    void selectPressed(QDeclarativeKeyEvent *event);
    void yesPressed(QDeclarativeKeyEvent *event);
    void noPressed(QDeclarativeKeyEvent *event);
    void context1Pressed(QDeclarativeKeyEvent *event);
    void context2Pressed(QDeclarativeKeyEvent *event);
    void context3Pressed(QDeclarativeKeyEvent *event);
    void context4Pressed(QDeclarativeKeyEvent *event);
    void callPressed(QDeclarativeKeyEvent *event);
    void hangupPressed(QDeclarativeKeyEvent *event);
    void flipPressed(QDeclarativeKeyEvent *event);
    void menuPressed(QDeclarativeKeyEvent *event);
    void volumeUpPressed(QDeclarativeKeyEvent *event);
    void volumeDownPressed(QDeclarativeKeyEvent *event);

private:
    virtual void keyPressed(QKeyEvent *event);
    virtual void keyReleased(QKeyEvent *event);
    virtual void inputMethodEvent(QInputMethodEvent *);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    const QByteArray keyToSignal(int key) {
        QByteArray keySignal;
        if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            keySignal = "digit0Pressed";
            keySignal[5] = '0' + (key - Qt::Key_0);
        } else {
            int i = 0;
            while (sigMap[i].key && sigMap[i].key != key)
                ++i;
            keySignal = sigMap[i].sig;
        }
        return keySignal;
    }

    struct SigMap {
        int key;
        const char *sig;
    };

    static const SigMap sigMap[];
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativeItemPrivate::ChangeTypes);

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeKeysAttached)
QML_DECLARE_TYPEINFO(QDeclarativeKeysAttached, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QDeclarativeKeyNavigationAttached)
QML_DECLARE_TYPEINFO(QDeclarativeKeyNavigationAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QDECLARATIVEITEM_P_H
