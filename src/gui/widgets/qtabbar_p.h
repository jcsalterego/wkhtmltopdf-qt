/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTABBAR_P_H
#define QTABBAR_P_H

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

#include "qtabbar.h"
#include "private/qwidget_p.h"

#include <qicon.h>
#include <qtoolbutton.h>
#include <qdebug.h>
#include <qvariantanimation.h>

#ifndef QT_NO_TABBAR

#define ANIMATION_DURATION 250

#include <qstyleoption.h>

QT_BEGIN_NAMESPACE

class QTabBarPrivate  : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QTabBar)
public:
    QTabBarPrivate()
        :currentIndex(-1), pressedIndex(-1), shape(QTabBar::RoundedNorth), layoutDirty(false),
        drawBase(true), scrollOffset(0), elideModeSetByUser(false), useScrollButtonsSetByUser(false), expanding(true), closeButtonOnTabs(false),
        selectionBehaviorOnRemove(QTabBar::SelectRightTab), paintWithOffsets(true), movable(false),
        dragInProgress(false), documentMode(false), movingTab(0)
#ifdef Q_WS_MAC
        , previousPressedIndex(-1)
#endif
        {}

    int currentIndex;
    int pressedIndex;
    QTabBar::Shape shape;
    bool layoutDirty;
    bool drawBase;
    int scrollOffset;

    struct Tab {
        inline Tab(const QIcon &ico, const QString &txt)
            : enabled(true) , shortcutId(0), text(txt), icon(ico),
            leftWidget(0), rightWidget(0), lastTab(-1), dragOffset(0)
#ifndef QT_NO_ANIMATION
            , animation(0)
#endif //QT_NO_ANIMATION
        {}
        bool operator==(const Tab &other) const { return &other == this; }
        bool enabled;
        int shortcutId;
        QString text;
#ifndef QT_NO_TOOLTIP
        QString toolTip;
#endif
#ifndef QT_NO_WHATSTHIS
        QString whatsThis;
#endif
        QIcon icon;
        QRect rect;
        QRect minRect;
        QRect maxRect;

        QColor textColor;
        QVariant data;
        QWidget *leftWidget;
        QWidget *rightWidget;
        int lastTab;
        int dragOffset;

#ifndef QT_NO_ANIMATION
        ~Tab() { delete animation; }
        struct TabBarAnimation : public QVariantAnimation {
            TabBarAnimation(Tab *t, QTabBarPrivate *_priv) : tab(t), priv(_priv)
            { setEasingCurve(QEasingCurve::InOutQuad); }

            void updateCurrentValue(const QVariant &current)
            { priv->moveTab(priv->tabList.indexOf(*tab), current.toInt()); }

            void updateState(State, State newState)
            { if (newState == Stopped) priv->moveTabFinished(priv->tabList.indexOf(*tab)); }
        private:
            //these are needed for the callbacks
            Tab *tab;
            QTabBarPrivate *priv;
        } *animation;

        void startAnimation(QTabBarPrivate *priv, int duration) {
            if (!animation)
                animation = new TabBarAnimation(this, priv);
            animation->setStartValue(dragOffset);
            animation->setEndValue(0);
            animation->setDuration(duration);
            animation->start();
        }
#else
        void startAnimation(QTabBarPrivate *priv, int duration)
        { Q_UNUSED(duration); priv->moveTabFinished(priv->tabList.indexOf(*this)); }
#endif //QT_NO_ANIMATION
    };
    QList<Tab> tabList;

    int calculateNewPosition(int from, int to, int index) const;
    void slide(int from, int to);
    void init();
    int extraWidth() const;

    Tab *at(int index);
    const Tab *at(int index) const;

    int indexAtPos(const QPoint &p) const;

    inline bool validIndex(int index) const { return index >= 0 && index < tabList.count(); }
    void setCurrentNextEnabledIndex(int offset);

    QSize minimumTabSizeHint(int index);

    QToolButton* rightB; // right or bottom
    QToolButton* leftB; // left or top

    void _q_scrollTabs();
    void _q_closeTab();
    void moveTab(int index, int offset);
    void moveTabFinished(int index);
    QRect hoverRect;

    void refresh();
    void layoutTabs();
    void layoutWidgets(int start = 0);
    void layoutTab(int index);
    void updateMacBorderMetrics();
    void setupMovableTab();

    void makeVisible(int index);
    QSize iconSize;
    Qt::TextElideMode elideMode;
    bool elideModeSetByUser;
    bool useScrollButtons;
    bool useScrollButtonsSetByUser;

    bool expanding;
    bool closeButtonOnTabs;
    QTabBar::SelectionBehavior selectionBehaviorOnRemove;

    QPoint dragStartPosition;
    bool paintWithOffsets;
    bool movable;
    bool dragInProgress;
    bool documentMode;

    QWidget *movingTab;
#ifdef Q_WS_MAC
    int previousPressedIndex;
#endif
    // shared by tabwidget and qtabbar
    static void initStyleBaseOption(QStyleOptionTabBarBaseV2 *optTabBase, QTabBar *tabbar, QSize size)
    {
        QStyleOptionTab tabOverlap;
        tabOverlap.shape = tabbar->shape();
        int overlap = tabbar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, tabbar);
        QWidget *theParent = tabbar->parentWidget();
        optTabBase->init(tabbar);
        optTabBase->shape = tabbar->shape();
        optTabBase->documentMode = tabbar->documentMode();
        if (theParent && overlap > 0) {
            QRect rect;
            switch (tabOverlap.shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                rect.setRect(0, size.height()-overlap, size.width(), overlap);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                rect.setRect(0, 0, size.width(), overlap);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                rect.setRect(0, 0, overlap, size.height());
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                rect.setRect(size.width() - overlap, 0, overlap, size.height());
                break;
            }
            optTabBase->rect = rect;
        }
    }

};

class CloseButton : public QAbstractButton
{
    Q_OBJECT

public:
    CloseButton(QWidget *parent = 0);

    QSize sizeHint() const;
    inline QSize minimumSizeHint() const
        { return sizeHint(); }
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
};


QT_END_NAMESPACE

#endif

#endif
