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

#ifndef QDECLARATIVELAYOUTS_H
#define QDECLARATIVELAYOUTS_H

#include "qdeclarativeitem.h"

#include <private/qdeclarativestate_p.h>
#include <private/qpodvector_p.h>

#include <QtCore/QObject>
#include <QtCore/QString>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QDeclarativeBasePositionerPrivate;

class Q_DECLARATIVE_EXPORT QDeclarativeBasePositioner : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(QDeclarativeTransition *move READ move WRITE setMove NOTIFY moveChanged)
    Q_PROPERTY(QDeclarativeTransition *add READ add WRITE setAdd NOTIFY addChanged)
public:
    enum PositionerType { None = 0x0, Horizontal = 0x1, Vertical = 0x2, Both = 0x3 };
    QDeclarativeBasePositioner(PositionerType, QDeclarativeItem *parent);
    ~QDeclarativeBasePositioner();

    int spacing() const;
    void setSpacing(int);

    QDeclarativeTransition *move() const;
    void setMove(QDeclarativeTransition *);

    QDeclarativeTransition *add() const;
    void setAdd(QDeclarativeTransition *);

protected:
    QDeclarativeBasePositioner(QDeclarativeBasePositionerPrivate &dd, PositionerType at, QDeclarativeItem *parent);
    virtual void componentComplete();
    virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
    void finishApplyTransitions();

Q_SIGNALS:
    void spacingChanged();
    void moveChanged();
    void addChanged();

protected Q_SLOTS:
    void prePositioning();

protected:
    virtual void doPositioning(QSizeF *contentSize)=0;
    struct PositionedItem {
        PositionedItem(QDeclarativeItem *i) : item(i), isNew(false), isVisible(true) {}
        bool operator==(const PositionedItem &other) const { return other.item == item; }
        QDeclarativeItem *item;
        bool isNew;
        bool isVisible;
    };

    QPODVector<PositionedItem,8> positionedItems;
    void positionX(int,const PositionedItem &target);
    void positionY(int,const PositionedItem &target);

private:
    Q_DISABLE_COPY(QDeclarativeBasePositioner)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeBasePositioner)
};

class Q_DECLARATIVE_EXPORT QDeclarativeColumn : public QDeclarativeBasePositioner
{
    Q_OBJECT
public:
    QDeclarativeColumn(QDeclarativeItem *parent=0);
protected:
    virtual void doPositioning(QSizeF *contentSize);
private:
    Q_DISABLE_COPY(QDeclarativeColumn)
};

class Q_DECLARATIVE_EXPORT QDeclarativeRow: public QDeclarativeBasePositioner
{
    Q_OBJECT
public:
    QDeclarativeRow(QDeclarativeItem *parent=0);
protected:
    virtual void doPositioning(QSizeF *contentSize);
private:
    Q_DISABLE_COPY(QDeclarativeRow)
};

class Q_DECLARATIVE_EXPORT QDeclarativeGrid : public QDeclarativeBasePositioner
{
    Q_OBJECT
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)

public:
    QDeclarativeGrid(QDeclarativeItem *parent=0);

    int rows() const {return m_rows;}
    void setRows(const int rows);

    int columns() const {return m_columns;}
    void setColumns(const int columns);

    Q_ENUMS(Flow)
    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

Q_SIGNALS:
    void rowsChanged();
    void columnsChanged();
    void flowChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize);

private:
    int m_rows;
    int m_columns;
    Flow m_flow;
    Q_DISABLE_COPY(QDeclarativeGrid)
};

class QDeclarativeFlowPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeFlow: public QDeclarativeBasePositioner
{
    Q_OBJECT
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
public:
    QDeclarativeFlow(QDeclarativeItem *parent=0);

    Q_ENUMS(Flow)
    enum Flow { LeftToRight, TopToBottom };
    Flow flow() const;
    void setFlow(Flow);

Q_SIGNALS:
    void flowChanged();

protected:
    virtual void doPositioning(QSizeF *contentSize);

protected:
    QDeclarativeFlow(QDeclarativeFlowPrivate &dd, QDeclarativeItem *parent);
private:
    Q_DISABLE_COPY(QDeclarativeFlow)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeFlow)
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeColumn)
QML_DECLARE_TYPE(QDeclarativeRow)
QML_DECLARE_TYPE(QDeclarativeGrid)
QML_DECLARE_TYPE(QDeclarativeFlow)

QT_END_HEADER

#endif
