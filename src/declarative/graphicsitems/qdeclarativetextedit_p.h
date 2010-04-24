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

#ifndef QDECLARATIVETEXTEDIT_H
#define QDECLARATIVETEXTEDIT_H

#include "private/qdeclarativetext_p.h"
#include "private/qdeclarativepainteditem_p.h"

#include <QtGui/qtextdocument.h>
#include <QtGui/qtextoption.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qtextformat.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QDeclarativeTextEditPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeTextEdit : public QDeclarativePaintedItem
{
    Q_OBJECT
    Q_ENUMS(VAlignment)
    Q_ENUMS(HAlignment)
    Q_ENUMS(TextFormat)
    Q_ENUMS(WrapMode)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool cursorVisible READ isCursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(QDeclarativeComponent* cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY cursorDelegateChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(bool focusOnPress READ focusOnPress WRITE setFocusOnPress NOTIFY focusOnPressChanged)
    Q_PROPERTY(bool persistentSelection READ persistentSelection WRITE setPersistentSelection NOTIFY persistentSelectionChanged)
    Q_PROPERTY(qreal textMargin READ textMargin WRITE setTextMargin NOTIFY textMarginChanged)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints)

public:
    QDeclarativeTextEdit(QDeclarativeItem *parent=0);

    enum HAlignment {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter
    };

    enum VAlignment {
        AlignTop = Qt::AlignTop,
        AlignBottom = Qt::AlignBottom,
        AlignVCenter = Qt::AlignVCenter
    };

    enum TextFormat {
        PlainText = Qt::PlainText,
        RichText = Qt::RichText,
        AutoText = Qt::AutoText
    };

    enum WrapMode { NoWrap = QTextOption::NoWrap,
                    WordWrap = QTextOption::WordWrap,
                    WrapAnywhere = QTextOption::WrapAnywhere,
                    WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere
                  };

    QString text() const;
    void setText(const QString &);

    TextFormat textFormat() const;
    void setTextFormat(TextFormat format);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    QColor selectionColor() const;
    void setSelectionColor(const QColor &c);

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    bool isCursorVisible() const;
    void setCursorVisible(bool on);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    QDeclarativeComponent* cursorDelegate() const;
    void setCursorDelegate(QDeclarativeComponent*);

    int selectionStart() const;
    void setSelectionStart(int);

    int selectionEnd() const;
    void setSelectionEnd(int);

    QString selectedText() const;

    bool focusOnPress() const;
    void setFocusOnPress(bool on);

    bool persistentSelection() const;
    void setPersistentSelection(bool on);

    qreal textMargin() const;
    void setTextMargin(qreal margin);

    virtual void componentComplete();

    /* FROM EDIT */
    void setReadOnly(bool);
    bool isReadOnly() const;

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    QRect cursorRect() const;

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

Q_SIGNALS:
    void textChanged(const QString &);
    void cursorPositionChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectionChanged();
    void colorChanged(const QColor &color);
    void selectionColorChanged(const QColor &color);
    void selectedTextColorChanged(const QColor &color);
    void fontChanged(const QFont &font);
    void horizontalAlignmentChanged(HAlignment alignment);
    void verticalAlignmentChanged(VAlignment alignment);
    void wrapModeChanged();
    void textFormatChanged(TextFormat textFormat);
    void readOnlyChanged(bool isReadOnly);
    void cursorVisibleChanged(bool isCursorVisible);
    void cursorDelegateChanged();
    void focusOnPressChanged(bool focusIsPressed);
    void persistentSelectionChanged(bool isPersistentSelection);
    void textMarginChanged(qreal textMargin);

public Q_SLOTS:
    void selectAll();

private Q_SLOTS:
    void updateImgCache(const QRectF &rect);
    void q_textChanged();
    void updateSelectionMarkers();
    void moveCursorDelegate();
    void loadCursorDelegate();

private:
    void updateSize();

protected:
    virtual void geometryChanged(const QRectF &newGeometry, 
                                 const QRectF &oldGeometry);

    bool event(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

    // mouse filter?
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void inputMethodEvent(QInputMethodEvent *e);

    void drawContents(QPainter *, const QRect &);
private:
    Q_DISABLE_COPY(QDeclarativeTextEdit)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarativeTextEdit)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeTextEdit)

QT_END_HEADER

#endif
