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

#ifndef QDECLARATIVEGRIDVIEW_H
#define QDECLARATIVEGRIDVIEW_H

#include "private/qdeclarativeflickable_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QDeclarativeVisualModel;
class QDeclarativeGridViewAttached;
class QDeclarativeGridViewPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeGridView : public QDeclarativeFlickable
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeGridView)

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QDeclarativeItem *currentItem READ currentItem NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    Q_PROPERTY(QDeclarativeComponent *highlight READ highlight WRITE setHighlight NOTIFY highlightChanged)
    Q_PROPERTY(QDeclarativeItem *highlightItem READ highlightItem NOTIFY highlightItemChanged)
    Q_PROPERTY(bool highlightFollowsCurrentItem READ highlightFollowsCurrentItem WRITE setHighlightFollowsCurrentItem)
    Q_PROPERTY(int highlightMoveDuration READ highlightMoveDuration WRITE setHighlightMoveDuration NOTIFY highlightMoveDurationChanged)

    Q_PROPERTY(qreal preferredHighlightBegin READ preferredHighlightBegin WRITE setPreferredHighlightBegin NOTIFY preferredHighlightBeginChanged)
    Q_PROPERTY(qreal preferredHighlightEnd READ preferredHighlightEnd WRITE setPreferredHighlightEnd NOTIFY preferredHighlightEndChanged)
    Q_PROPERTY(HighlightRangeMode highlightRangeMode READ highlightRangeMode WRITE setHighlightRangeMode NOTIFY highlightRangeModeChanged)

    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(bool keyNavigationWraps READ isWrapEnabled WRITE setWrapEnabled NOTIFY keyNavigationWrapsChanged)
    Q_PROPERTY(int cacheBuffer READ cacheBuffer WRITE setCacheBuffer NOTIFY cacheBufferChanged)
    Q_PROPERTY(int cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(int cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)

    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged)

    Q_ENUMS(HighlightRangeMode)
    Q_ENUMS(SnapMode)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    QDeclarativeGridView(QDeclarativeItem *parent=0);
    ~QDeclarativeGridView();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    int currentIndex() const;
    void setCurrentIndex(int idx);

    QDeclarativeItem *currentItem();
    QDeclarativeItem *highlightItem();
    int count() const;

    QDeclarativeComponent *highlight() const;
    void setHighlight(QDeclarativeComponent *highlight);

    bool highlightFollowsCurrentItem() const;
    void setHighlightFollowsCurrentItem(bool);

    int highlightMoveDuration() const;
    void setHighlightMoveDuration(int);

    enum HighlightRangeMode { NoHighlightRange, ApplyRange, StrictlyEnforceRange };
    HighlightRangeMode highlightRangeMode() const;
    void setHighlightRangeMode(HighlightRangeMode mode);

    qreal preferredHighlightBegin() const;
    void setPreferredHighlightBegin(qreal);

    qreal preferredHighlightEnd() const;
    void setPreferredHighlightEnd(qreal);

    Q_ENUMS(Flow)
    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

    bool isWrapEnabled() const;
    void setWrapEnabled(bool);

    int cacheBuffer() const;
    void setCacheBuffer(int);

    int cellWidth() const;
    void setCellWidth(int);

    int cellHeight() const;
    void setCellHeight(int);

    enum SnapMode { NoSnap, SnapToRow, SnapOneRow };
    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    enum PositionMode { Beginning, Center, End, Visible, Contain };
    Q_ENUMS(PositionMode);

    Q_INVOKABLE void positionViewAtIndex(int index, int mode);
    Q_INVOKABLE int indexAt(int x, int y) const;

    static QDeclarativeGridViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void moveCurrentIndexUp();
    void moveCurrentIndexDown();
    void moveCurrentIndexLeft();
    void moveCurrentIndexRight();

Q_SIGNALS:
    void countChanged();
    void currentIndexChanged();
    void cellWidthChanged();
    void cellHeightChanged();
    void highlightChanged();
    void highlightItemChanged();
    void preferredHighlightBeginChanged();
    void preferredHighlightEndChanged();
    void highlightRangeModeChanged();
    void highlightMoveDurationChanged();
    void modelChanged();
    void delegateChanged();
    void flowChanged();
    void keyNavigationWrapsChanged();
    void cacheBufferChanged();
    void snapModeChanged();

protected:
    virtual bool event(QEvent *event);
    virtual void viewportMoved();
    virtual qreal minYExtent() const;
    virtual qreal maxYExtent() const;
    virtual qreal minXExtent() const;
    virtual qreal maxXExtent() const;
    virtual void keyPressEvent(QKeyEvent *);
    virtual void componentComplete();

private Q_SLOTS:
    void trackedPositionChanged();
    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int from, int to, int count);
    void modelReset();
    void destroyRemoved();
    void createdItem(int index, QDeclarativeItem *item);
    void destroyingItem(QDeclarativeItem *item);
    void animStopped();

private:
    void refill();
};

class QDeclarativeGridViewAttached : public QObject
{
    Q_OBJECT
public:
    QDeclarativeGridViewAttached(QObject *parent)
        : QObject(parent), m_view(0), m_isCurrent(false), m_delayRemove(false) {}
    ~QDeclarativeGridViewAttached() {}

    Q_PROPERTY(QDeclarativeGridView *view READ view CONSTANT)
    QDeclarativeGridView *view() { return m_view; }

    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY currentItemChanged)
    bool isCurrentItem() const { return m_isCurrent; }
    void setIsCurrentItem(bool c) {
        if (m_isCurrent != c) {
            m_isCurrent = c;
            emit currentItemChanged();
        }
    }

    Q_PROPERTY(bool delayRemove READ delayRemove WRITE setDelayRemove NOTIFY delayRemoveChanged)
    bool delayRemove() const { return m_delayRemove; }
    void setDelayRemove(bool delay) {
        if (m_delayRemove != delay) {
            m_delayRemove = delay;
            emit delayRemoveChanged();
        }
    }

    void emitAdd() { emit add(); }
    void emitRemove() { emit remove(); }

Q_SIGNALS:
    void currentItemChanged();
    void delayRemoveChanged();
    void add();
    void remove();

public:
    QDeclarativeGridView *m_view;
    bool m_isCurrent : 1;
    bool m_delayRemove : 1;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeGridView)
QML_DECLARE_TYPEINFO(QDeclarativeGridView, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif
