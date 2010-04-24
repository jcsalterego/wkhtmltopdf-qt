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

#ifndef QDECLARATIVEITEM_H
#define QDECLARATIVEITEM_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativecomponent.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtGui/qgraphicsitem.h>
#include <QtGui/qgraphicstransform.h>
#include <QtGui/qfont.h>
#include <QtGui/qaction.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeState;
class QDeclarativeAnchorLine;
class QDeclarativeTransition;
class QDeclarativeKeyEvent;
class QDeclarativeAnchors;
class QDeclarativeItemPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeItem : public QGraphicsObject, public QDeclarativeParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativeParserStatus)

    Q_PROPERTY(QDeclarativeItem * parent READ parentItem WRITE setParentItem NOTIFY parentChanged DESIGNABLE false FINAL)
    Q_PROPERTY(QDeclarativeListProperty<QObject> data READ data DESIGNABLE false)
    Q_PROPERTY(QDeclarativeListProperty<QObject> resources READ resources DESIGNABLE false)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeState> states READ states DESIGNABLE false)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeTransition> transitions READ transitions DESIGNABLE false)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QRectF childrenRect READ childrenRect NOTIFY childrenRectChanged DESIGNABLE false FINAL)
    Q_PROPERTY(QDeclarativeAnchors * anchors READ anchors DESIGNABLE false CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine left READ left CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine right READ right CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine horizontalCenter READ horizontalCenter CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine top READ top CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine bottom READ bottom CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine verticalCenter READ verticalCenter CONSTANT FINAL)
    Q_PROPERTY(QDeclarativeAnchorLine baseline READ baseline CONSTANT FINAL)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged)
    Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged) // ### move to QGI/QGO, NOTIFY
    Q_PROPERTY(bool focus READ hasFocus WRITE setFocus NOTIFY focusChanged FINAL)
    Q_PROPERTY(bool wantsFocus READ wantsFocus NOTIFY wantsFocusChanged)
    Q_PROPERTY(QDeclarativeListProperty<QGraphicsTransform> transform READ transform DESIGNABLE false FINAL)
    Q_PROPERTY(TransformOrigin transformOrigin READ transformOrigin WRITE setTransformOrigin NOTIFY transformOriginChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_ENUMS(TransformOrigin)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    enum TransformOrigin {
        TopLeft, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight
    };

    QDeclarativeItem(QDeclarativeItem *parent = 0);
    virtual ~QDeclarativeItem();

    QDeclarativeItem *parentItem() const;
    void setParentItem(QDeclarativeItem *parent);

    QDeclarativeListProperty<QObject> data();
    QDeclarativeListProperty<QObject> resources();

    QDeclarativeAnchors *anchors();
    QRectF childrenRect();

    bool clip() const;
    void setClip(bool);

    QDeclarativeListProperty<QDeclarativeState> states();
    QDeclarativeListProperty<QDeclarativeTransition> transitions();

    QString state() const;
    void setState(const QString &);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);

    QDeclarativeListProperty<QGraphicsTransform> transform();

    qreal width() const;
    void setWidth(qreal);
    void resetWidth();
    qreal implicitWidth() const;

    qreal height() const;
    void setHeight(qreal);
    void resetHeight();
    qreal implicitHeight() const;

    void setSize(const QSizeF &size);

    TransformOrigin transformOrigin() const;
    void setTransformOrigin(TransformOrigin);

    bool smooth() const;
    void setSmooth(bool);

    QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    bool wantsFocus() const;
    bool hasFocus() const;
    void setFocus(bool);

    bool keepMouseGrab() const;
    void setKeepMouseGrab(bool);

    Q_INVOKABLE QScriptValue mapFromItem(const QScriptValue &item, qreal x, qreal y) const;
    Q_INVOKABLE QScriptValue mapToItem(const QScriptValue &item, qreal x, qreal y) const;
    Q_INVOKABLE void forceFocus();

    QDeclarativeAnchorLine left() const;
    QDeclarativeAnchorLine right() const;
    QDeclarativeAnchorLine horizontalCenter() const;
    QDeclarativeAnchorLine top() const;
    QDeclarativeAnchorLine bottom() const;
    QDeclarativeAnchorLine verticalCenter() const;
    QDeclarativeAnchorLine baseline() const;

Q_SIGNALS:
    void childrenChanged();
    void childrenRectChanged(const QRectF &);
    void baselineOffsetChanged(qreal);
    void stateChanged(const QString &);
    void focusChanged(bool);
    void wantsFocusChanged(bool);
    void parentChanged(QDeclarativeItem *);
    void transformOriginChanged(TransformOrigin);
    void smoothChanged(bool);
    void clipChanged(bool);

protected:
    bool isComponentComplete() const;
    virtual bool sceneEvent(QEvent *);
    virtual bool event(QEvent *);
    virtual QVariant itemChange(GraphicsItemChange, const QVariant &);

    void setImplicitWidth(qreal);
    bool widthValid() const; // ### better name?
    void setImplicitHeight(qreal);
    bool heightValid() const; // ### better name?

    virtual void classBegin();
    virtual void componentComplete();
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void inputMethodEvent(QInputMethodEvent *);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

protected:
    QDeclarativeItem(QDeclarativeItemPrivate &dd, QDeclarativeItem *parent = 0);

private:
    Q_DISABLE_COPY(QDeclarativeItem)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeItem)
};

template<typename T>
        T qobject_cast(QGraphicsObject *o)
{
    QObject *obj = o;
    return qobject_cast<T>(obj);
}

// ### move to QGO
template<typename T>
T qobject_cast(QGraphicsItem *item)
{
    if (!item) return 0;
    QObject *o = item->toGraphicsObject();
    return qobject_cast<T>(o);
}

QDebug Q_DECLARATIVE_EXPORT operator<<(QDebug debug, QDeclarativeItem *item);

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeItem)
QML_DECLARE_TYPE(QGraphicsObject)
QML_DECLARE_TYPE(QGraphicsTransform)
QML_DECLARE_TYPE(QGraphicsScale)
QML_DECLARE_TYPE(QGraphicsRotation)
QML_DECLARE_TYPE(QGraphicsWidget)
QML_DECLARE_TYPE(QAction)

QT_END_HEADER

#endif // QDECLARATIVEITEM_H
