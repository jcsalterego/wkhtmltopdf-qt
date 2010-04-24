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

#ifndef QDECLARATIVETEXT_H
#define QDECLARATIVETEXT_H

#include <QtGui/qtextoption.h>
#include "qdeclarativeitem.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QDeclarativeTextPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeText : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(HAlignment)
    Q_ENUMS(VAlignment)
    Q_ENUMS(TextStyle)
    Q_ENUMS(TextFormat)
    Q_ENUMS(TextElideMode)
    Q_ENUMS(WrapMode)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(TextStyle style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(QColor styleColor READ styleColor WRITE setStyleColor NOTIFY styleColorChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(TextElideMode elide READ elideMode WRITE setElideMode NOTIFY elideModeChanged) //### elideMode?

public:
    QDeclarativeText(QDeclarativeItem *parent=0);
    ~QDeclarativeText();

    enum HAlignment { AlignLeft = Qt::AlignLeft,
                       AlignRight = Qt::AlignRight,
                       AlignHCenter = Qt::AlignHCenter };
    enum VAlignment { AlignTop = Qt::AlignTop,
                       AlignBottom = Qt::AlignBottom,
                       AlignVCenter = Qt::AlignVCenter };
    enum TextStyle { Normal,
                      Outline,
                      Raised,
                      Sunken };
    enum TextFormat { PlainText = Qt::PlainText,
                       RichText = Qt::RichText,
                       AutoText = Qt::AutoText,
                       StyledText = 4 };
    enum TextElideMode { ElideLeft = Qt::ElideLeft,
                          ElideRight = Qt::ElideRight,
                          ElideMiddle = Qt::ElideMiddle,
                          ElideNone = Qt::ElideNone };

    enum WrapMode { NoWrap = QTextOption::NoWrap,
                    WordWrap = QTextOption::WordWrap,
                    WrapAnywhere = QTextOption::WrapAnywhere,
                    WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere
                  };

    QString text() const;
    void setText(const QString &);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    TextStyle style() const;
    void setStyle(TextStyle style);

    QColor styleColor() const;
    void setStyleColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    TextFormat textFormat() const;
    void setTextFormat(TextFormat format);

    TextElideMode elideMode() const;
    void setElideMode(TextElideMode);

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    virtual void componentComplete();

    int resourcesLoading() const; // mainly for testing

Q_SIGNALS:
    void textChanged(const QString &text);
    void linkActivated(const QString &link);
    void fontChanged(const QFont &font);
    void colorChanged(const QColor &color);
    void styleChanged(TextStyle style);
    void styleColorChanged(const QColor &color);
    void horizontalAlignmentChanged(HAlignment alignment);
    void verticalAlignmentChanged(VAlignment alignment);
    void wrapModeChanged();
    void textFormatChanged(TextFormat textFormat);
    void elideModeChanged(TextElideMode mode);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

private:
    Q_DISABLE_COPY(QDeclarativeText)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeText)

    friend class QTextDocumentWithImageResources;
    void reloadWithResources();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeText)

QT_END_HEADER

#endif
