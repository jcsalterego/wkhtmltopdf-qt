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

#include "private/qdeclarativerectangle_p.h"
#include "private/qdeclarativerectangle_p_p.h"

#include <QPainter>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QDeclarativePen
    \brief The QDeclarativePen class provides a pen used for drawing rectangle borders on a QDeclarativeView.

    By default, the pen is invalid and nothing is drawn. You must either set a color (then the default
    width is 1) or a width (then the default color is black).

    A width of 1 indicates is a single-pixel line on the border of the item being painted.

    Example:
    \qml
    Rectangle { border.width: 2; border.color: "red" ... }
    \endqml
*/

void QDeclarativePen::setColor(const QColor &c)
{
    _color = c;
    _valid = _color.alpha() ? true : false;
    emit penChanged();
}

void QDeclarativePen::setWidth(int w)
{
    if (_width == w && _valid)
        return;

    _width = w;
    _valid = (_width < 1) ? false : true;
    emit penChanged();
}


/*!
    \qmlclass GradientStop QDeclarativeGradientStop
  \since 4.7
    \brief The GradientStop item defines the color at a position in a Gradient

    \sa Gradient
*/

/*!
    \qmlproperty real GradientStop::position
    \qmlproperty color GradientStop::color

    Sets a \e color at a \e position in a gradient.
*/

void QDeclarativeGradientStop::updateGradient()
{
    if (QDeclarativeGradient *grad = qobject_cast<QDeclarativeGradient*>(parent()))
        grad->doUpdate();
}

/*!
    \qmlclass Gradient QDeclarativeGradient
  \since 4.7
    \brief The Gradient item defines a gradient fill.

    A gradient is defined by two or more colors, which will be blended seemlessly.  The
    colors are specified at their position in the range 0.0 - 1.0 via
    the GradientStop item.  For example, the following code paints a
    rectangle with a gradient starting with red, blending to yellow at 1/3 of the
    size of the rectangle, and ending with Green:

    \table
    \row
    \o \image gradient.png
    \o \quotefile doc/src/snippets/declarative/gradient.qml
    \endtable

    \sa GradientStop
*/

/*!
    \qmlproperty list<GradientStop> Gradient::stops
    This property holds the gradient stops describing the gradient.
*/

const QGradient *QDeclarativeGradient::gradient() const
{
    if (!m_gradient && !m_stops.isEmpty()) {
        m_gradient = new QLinearGradient(0,0,0,1.0);
        for (int i = 0; i < m_stops.count(); ++i) {
            const QDeclarativeGradientStop *stop = m_stops.at(i);
            m_gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
            m_gradient->setColorAt(stop->position(), stop->color());
        }
    }

    return m_gradient;
}

void QDeclarativeGradient::doUpdate()
{
    delete m_gradient;
    m_gradient = 0;
    emit updated();
}


/*!
    \qmlclass Rectangle QDeclarativeRectangle
  \since 4.7
    \brief The Rectangle item allows you to add rectangles to a scene.
    \inherits Item

    A Rectangle is painted having a solid fill (color) and an optional border.
    You can also create rounded rectangles using the radius property.

    \qml
    Rectangle {
        width: 100
        height: 100
        color: "red"
        border.color: "black"
        border.width: 5
        radius: 10
    }
    \endqml

    \image declarative-rect.png
*/

int QDeclarativeRectanglePrivate::doUpdateSlotIdx = -1;

/*!
    \internal
    \class QDeclarativeRectangle
    \brief The QDeclarativeRectangle class provides a rectangle item that you can add to a QDeclarativeView.
*/
QDeclarativeRectangle::QDeclarativeRectangle(QDeclarativeItem *parent)
  : QDeclarativeItem(*(new QDeclarativeRectanglePrivate), parent)
{
}

void QDeclarativeRectangle::doUpdate()
{
    Q_D(QDeclarativeRectangle);
    d->rectImage = QPixmap();
    const int pw = d->pen && d->pen->isValid() ? d->pen->width() : 0;
    d->setPaintMargin((pw+1)/2);
    update();
}

/*!
    \qmlproperty int Rectangle::border.width
    \qmlproperty color Rectangle::border.color

    The width and color used to draw the border of the rectangle.

    A width of 1 creates a thin line. For no line, use a width of 0 or a transparent color.

    To keep the border smooth (rather than blurry), odd widths cause the rectangle to be painted at
    a half-pixel offset;
*/
QDeclarativePen *QDeclarativeRectangle::border()
{
    Q_D(QDeclarativeRectangle);
    return d->getPen();
}

/*!
    \qmlproperty Gradient Rectangle::gradient

    The gradient to use to fill the rectangle.

    This property allows for the construction of simple vertical gradients.
    Other gradients may by formed by adding rotation to the rectangle.

    \table
    \row
    \o \image declarative-rect_gradient.png
    \o
    \qml
    Rectangle { y: 0; width: 80; height: 80; color: "lightsteelblue" }
    Rectangle { y: 100; width: 80; height: 80
        gradient: Gradient {
            GradientStop { position: 0.0; color: "lightsteelblue" }
            GradientStop { position: 1.0; color: "blue" }
        }
    }
    Rectangle { rotation: 90; y: 200; width: 80; height: 80
        gradient: Gradient {
            GradientStop { position: 0.0; color: "lightsteelblue" }
            GradientStop { position: 1.0; color: "blue" }
        }
    }
    \endqml
    \endtable

    If both a gradient and a color are specified, the gradient will be used.

    \sa Gradient, color
*/
QDeclarativeGradient *QDeclarativeRectangle::gradient() const
{
    Q_D(const QDeclarativeRectangle);
    return d->gradient;
}

void QDeclarativeRectangle::setGradient(QDeclarativeGradient *gradient)
{
    Q_D(QDeclarativeRectangle);
    if (d->gradient == gradient)
        return;
    static int updatedSignalIdx = -1;
    if (updatedSignalIdx < 0)
        updatedSignalIdx = QDeclarativeGradient::staticMetaObject.indexOfSignal("updated()");
    if (d->doUpdateSlotIdx < 0)
        d->doUpdateSlotIdx = QDeclarativeRectangle::staticMetaObject.indexOfSlot("doUpdate()");
    if (d->gradient)
        QMetaObject::disconnect(d->gradient, updatedSignalIdx, this, d->doUpdateSlotIdx);
    d->gradient = gradient;
    if (d->gradient)
        QMetaObject::connect(d->gradient, updatedSignalIdx, this, d->doUpdateSlotIdx);
    update();
}


/*!
    \qmlproperty real Rectangle::radius
    This property holds the corner radius used to draw a rounded rectangle.

    If radius is non-zero, the rectangle will be painted as a rounded rectangle, otherwise it will be
    painted as a normal rectangle. The same radius is used by all 4 corners; there is currently
    no way to specify different radii for different corners.
*/
qreal QDeclarativeRectangle::radius() const
{
    Q_D(const QDeclarativeRectangle);
    return d->radius;
}

void QDeclarativeRectangle::setRadius(qreal radius)
{
    Q_D(QDeclarativeRectangle);
    if (d->radius == radius)
        return;

    d->radius = radius;
    d->rectImage = QPixmap();
    update();
    emit radiusChanged();
}

/*!
    \qmlproperty color Rectangle::color
    This property holds the color used to fill the rectangle.

    \qml
    // green rectangle using hexidecimal notation
    Rectangle { color: "#00FF00" }

    // steelblue rectangle using SVG color name
    Rectangle { color: "steelblue" }
    \endqml

    The default color is white.

    If both a gradient and a color are specified, the gradient will be used.
*/
QColor QDeclarativeRectangle::color() const
{
    Q_D(const QDeclarativeRectangle);
    return d->color;
}

void QDeclarativeRectangle::setColor(const QColor &c)
{
    Q_D(QDeclarativeRectangle);
    if (d->color == c)
        return;

    d->color = c;
    d->rectImage = QPixmap();
    update();
    emit colorChanged();
}

void QDeclarativeRectangle::generateRoundedRect()
{
    Q_D(QDeclarativeRectangle);
    if (d->rectImage.isNull()) {
        const int pw = d->pen && d->pen->isValid() ? d->pen->width() : 0;
        const int radius = qCeil(d->radius);    //ensure odd numbered width/height so we get 1-pixel center
        d->rectImage = QPixmap(radius*2 + 3 + pw*2, radius*2 + 3 + pw*2);
        d->rectImage.fill(Qt::transparent);
        QPainter p(&(d->rectImage));
        p.setRenderHint(QPainter::Antialiasing);
        if (d->pen && d->pen->isValid()) {
            QPen pn(QColor(d->pen->color()), d->pen->width());
            p.setPen(pn);
        } else {
            p.setPen(Qt::NoPen);
        }
        p.setBrush(d->color);
        if (pw%2)
            p.drawRoundedRect(QRectF(qreal(pw)/2+1, qreal(pw)/2+1, d->rectImage.width()-(pw+1), d->rectImage.height()-(pw+1)), d->radius, d->radius);
        else
            p.drawRoundedRect(QRectF(qreal(pw)/2, qreal(pw)/2, d->rectImage.width()-pw, d->rectImage.height()-pw), d->radius, d->radius);
    }
}

void QDeclarativeRectangle::generateBorderedRect()
{
    Q_D(QDeclarativeRectangle);
    if (d->rectImage.isNull()) {
        const int pw = d->pen && d->pen->isValid() ? d->pen->width() : 0;
        d->rectImage = QPixmap(pw*2 + 3, pw*2 + 3);
        d->rectImage.fill(Qt::transparent);
        QPainter p(&(d->rectImage));
        p.setRenderHint(QPainter::Antialiasing);
        if (d->pen && d->pen->isValid()) {
            QPen pn(QColor(d->pen->color()), d->pen->width());
            pn.setJoinStyle(Qt::MiterJoin);
            p.setPen(pn);
        } else {
            p.setPen(Qt::NoPen);
        }
        p.setBrush(d->color);
        if (pw%2)
            p.drawRect(QRectF(qreal(pw)/2+1, qreal(pw)/2+1, d->rectImage.width()-(pw+1), d->rectImage.height()-(pw+1)));
        else
            p.drawRect(QRectF(qreal(pw)/2, qreal(pw)/2, d->rectImage.width()-pw, d->rectImage.height()-pw));
    }
}

void QDeclarativeRectangle::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    Q_D(QDeclarativeRectangle);
    if (d->radius > 0 || (d->pen && d->pen->isValid())
        || (d->gradient && d->gradient->gradient()) ) {
        drawRect(*p);
    }
    else {
        bool oldAA = p->testRenderHint(QPainter::Antialiasing);
        if (d->smooth)
            p->setRenderHints(QPainter::Antialiasing, true);
        p->fillRect(QRectF(0, 0, width(), height()), d->color);
        if (d->smooth)
            p->setRenderHint(QPainter::Antialiasing, oldAA);
    }
}

void QDeclarativeRectangle::drawRect(QPainter &p)
{
    Q_D(QDeclarativeRectangle);
    if ((d->gradient && d->gradient->gradient())
        || d->radius > width()/2 || d->radius > height()/2) {
        // XXX This path is still slower than the image path
        // Image path won't work for gradients or invalid radius though
        bool oldAA = p.testRenderHint(QPainter::Antialiasing);
        if (d->smooth)
            p.setRenderHint(QPainter::Antialiasing);
        if (d->pen && d->pen->isValid()) {
            QPen pn(QColor(d->pen->color()), d->pen->width());
            p.setPen(pn);
        } else {
            p.setPen(Qt::NoPen);
        }
        if (d->gradient && d->gradient->gradient())
            p.setBrush(*d->gradient->gradient());
        else
            p.setBrush(d->color);
        const int pw = d->pen && d->pen->isValid() ? d->pen->width() : 0;
        QRectF rect;
        if (pw%2)
            rect = QRectF(0.5, 0.5, width()-1, height()-1);
        else
            rect = QRectF(0, 0, width(), height());
        qreal radius = d->radius;
        if (radius > width()/2 || radius > height()/2)
            radius = qMin(width()/2, height()/2);
        if (radius > 0.)
            p.drawRoundedRect(rect, radius, radius);
        else
            p.drawRect(rect);
        if (d->smooth)
            p.setRenderHint(QPainter::Antialiasing, oldAA);
    } else {
        bool oldAA = p.testRenderHint(QPainter::Antialiasing);
        bool oldSmooth = p.testRenderHint(QPainter::SmoothPixmapTransform);
        if (d->smooth)
            p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, d->smooth);

        const int pw = d->pen && d->pen->isValid() ? (d->pen->width()+1)/2*2 : 0;

        if (d->radius > 0)
            generateRoundedRect();
        else
            generateBorderedRect();

        int xOffset = (d->rectImage.width()-1)/2;
        int yOffset = (d->rectImage.height()-1)/2;
        Q_ASSERT(d->rectImage.width() == 2*xOffset + 1);
        Q_ASSERT(d->rectImage.height() == 2*yOffset + 1);

        QMargins margins(xOffset, yOffset, xOffset, yOffset);
        QTileRules rules(Qt::StretchTile, Qt::StretchTile);
        //NOTE: even though our item may have qreal-based width and height, qDrawBorderPixmap only supports QRects
        qDrawBorderPixmap(&p, QRect(-pw/2, -pw/2, width()+pw, height()+pw), margins, d->rectImage, d->rectImage.rect(), margins, rules);

        if (d->smooth) {
            p.setRenderHint(QPainter::Antialiasing, oldAA);
            p.setRenderHint(QPainter::SmoothPixmapTransform, oldSmooth);
        }
    }
}

/*!
    \qmlproperty bool Rectangle::smooth

    Set this property if you want the item to be smoothly scaled or
    transformed.  Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.

    \image rect-smooth.png
    On this image, smooth is turned off on the top half and on on the bottom half.
*/

QRectF QDeclarativeRectangle::boundingRect() const
{
    Q_D(const QDeclarativeRectangle);
    return QRectF(-d->paintmargin, -d->paintmargin, d->width()+d->paintmargin*2, d->height()+d->paintmargin*2);
}

QT_END_NAMESPACE
