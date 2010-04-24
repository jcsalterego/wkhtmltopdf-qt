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

#include "private/qdeclarativescalegrid_p_p.h"

#include <qdeclarative.h>

#include <QBuffer>
#include <QDebug>

QT_BEGIN_NAMESPACE
/*!
    \internal
    \class QDeclarativeScaleGrid
    \brief The QDeclarativeScaleGrid class allows you to specify a 3x3 grid to use in scaling an image.
*/

QDeclarativeScaleGrid::QDeclarativeScaleGrid(QObject *parent) : QObject(parent), _left(0), _top(0), _right(0), _bottom(0)
{
}

QDeclarativeScaleGrid::~QDeclarativeScaleGrid()
{
}

bool QDeclarativeScaleGrid::isNull() const
{
    return !_left && !_top && !_right && !_bottom;
}

void QDeclarativeScaleGrid::setLeft(int pos)
{
    if (_left != pos) {
        _left = pos;
        emit borderChanged();
    }
}

void QDeclarativeScaleGrid::setTop(int pos)
{
    if (_top != pos) {
        _top = pos;
        emit borderChanged();
    }
}

void QDeclarativeScaleGrid::setRight(int pos)
{
    if (_right != pos) {
        _right = pos;
        emit borderChanged();
    }
}

void QDeclarativeScaleGrid::setBottom(int pos)
{
    if (_bottom != pos) {
        _bottom = pos;
        emit borderChanged();
    }
}

QDeclarativeGridScaledImage::QDeclarativeGridScaledImage()
: _l(-1), _r(-1), _t(-1), _b(-1),
  _h(QDeclarativeBorderImage::Stretch), _v(QDeclarativeBorderImage::Stretch)
{
}

QDeclarativeGridScaledImage::QDeclarativeGridScaledImage(const QDeclarativeGridScaledImage &o)
: _l(o._l), _r(o._r), _t(o._t), _b(o._b), _h(o._h), _v(o._v), _pix(o._pix)
{
}

QDeclarativeGridScaledImage &QDeclarativeGridScaledImage::operator=(const QDeclarativeGridScaledImage &o)
{
    _l = o._l;
    _r = o._r;
    _t = o._t;
    _b = o._b;
    _h = o._h;
    _v = o._v;
    _pix = o._pix;
    return *this;
}

QDeclarativeGridScaledImage::QDeclarativeGridScaledImage(QIODevice *data)
: _l(-1), _r(-1), _t(-1), _b(-1), _h(QDeclarativeBorderImage::Stretch), _v(QDeclarativeBorderImage::Stretch)
{
    int l = -1;
    int r = -1;
    int t = -1;
    int b = -1;
    QString imgFile;

    while(!data->atEnd()) {
        QString line = QString::fromUtf8(data->readLine().trimmed());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        QStringList list = line.split(QLatin1Char(':'));
        if (list.count() != 2)
            return;

        list[0] = list[0].trimmed();
        list[1] = list[1].trimmed();

        if (list[0] == QLatin1String("border.left"))
            l = list[1].toInt();
        else if (list[0] == QLatin1String("border.right"))
            r = list[1].toInt();
        else if (list[0] == QLatin1String("border.top"))
            t = list[1].toInt();
        else if (list[0] == QLatin1String("border.bottom"))
            b = list[1].toInt();
        else if (list[0] == QLatin1String("source"))
            imgFile = list[1];
        else if (list[0] == QLatin1String("horizontalTileRule"))
            _h = stringToRule(list[1]);
        else if (list[0] == QLatin1String("verticalTileRule"))
            _v = stringToRule(list[1]);
    }

    if (l < 0 || r < 0 || t < 0 || b < 0 || imgFile.isEmpty())
        return;

    _l = l; _r = r; _t = t; _b = b;

    _pix = imgFile;
}

QDeclarativeBorderImage::TileMode QDeclarativeGridScaledImage::stringToRule(const QString &s)
{
    if (s == QLatin1String("Stretch"))
        return QDeclarativeBorderImage::Stretch;
    if (s == QLatin1String("Repeat"))
        return QDeclarativeBorderImage::Repeat;
    if (s == QLatin1String("Round"))
        return QDeclarativeBorderImage::Round;

    qWarning("QDeclarativeGridScaledImage: Invalid tile rule specified. Using Stretch.");
    return QDeclarativeBorderImage::Stretch;
}

bool QDeclarativeGridScaledImage::isValid() const
{
    return _l >= 0;
}

int QDeclarativeGridScaledImage::gridLeft() const
{
    return _l;
}

int QDeclarativeGridScaledImage::gridRight() const
{
    return _r;
}

int QDeclarativeGridScaledImage::gridTop() const
{
    return _t;
}

int QDeclarativeGridScaledImage::gridBottom() const
{
    return _b;
}

QString QDeclarativeGridScaledImage::pixmapUrl() const
{
    return _pix;
}

QT_END_NAMESPACE
