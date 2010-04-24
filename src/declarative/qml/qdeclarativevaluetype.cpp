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

#include "private/qdeclarativevaluetype_p.h"

#include "private/qdeclarativemetatype_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

template<typename T>
int qmlRegisterValueTypeEnums(const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');

    QDeclarativePrivate::RegisterType type = {
        0,

        qRegisterMetaType<T *>(pointerName.constData()), 0, 0, 0,

        QString(),

        "Qt", 4, 7, qmlName, &T::staticMetaObject,

        0, 0,

        0, 0, 0,

        0, 0,

        0
    };

    return QDeclarativePrivate::registerType(type);
}

QDeclarativeValueTypeFactory::QDeclarativeValueTypeFactory()
{
    // ### Optimize
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        valueTypes[ii] = valueType(ii);
}

QDeclarativeValueTypeFactory::~QDeclarativeValueTypeFactory()
{
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        delete valueTypes[ii];
}

bool QDeclarativeValueTypeFactory::isValueType(int idx)
{
    if ((uint)idx < QVariant::UserType)
        return true;
    return false;
}

void QDeclarativeValueTypeFactory::registerValueTypes()
{
    qmlRegisterValueTypeEnums<QDeclarativeEasingValueType>("Easing");
    qmlRegisterValueTypeEnums<QDeclarativeFontValueType>("Font");
}

QDeclarativeValueType *QDeclarativeValueTypeFactory::valueType(int t)
{
    switch (t) {
    case QVariant::Point:
        return new QDeclarativePointValueType;
    case QVariant::PointF:
        return new QDeclarativePointFValueType;
    case QVariant::Size:
        return new QDeclarativeSizeValueType;
    case QVariant::SizeF:
        return new QDeclarativeSizeFValueType;
    case QVariant::Rect:
        return new QDeclarativeRectValueType;
    case QVariant::RectF:
        return new QDeclarativeRectFValueType;
    case QVariant::Vector3D:
        return new QDeclarativeVector3DValueType;
    case QVariant::EasingCurve:
        return new QDeclarativeEasingValueType;
    case QVariant::Font:
        return new QDeclarativeFontValueType;
    default:
        return 0;
    }
}

QDeclarativeValueType::QDeclarativeValueType(QObject *parent)
: QObject(parent)
{
}

QDeclarativePointFValueType::QDeclarativePointFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativePointFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &point, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativePointFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &point, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativePointFValueType::value()
{
    return QVariant(point);
}

void QDeclarativePointFValueType::setValue(QVariant value)
{
    point = qvariant_cast<QPointF>(value);
}

qreal QDeclarativePointFValueType::x() const
{
    return point.x();
}

qreal QDeclarativePointFValueType::y() const
{
    return point.y();
}

void QDeclarativePointFValueType::setX(qreal x)
{
    point.setX(x);
}

void QDeclarativePointFValueType::setY(qreal y)
{
    point.setY(y);
}

QDeclarativePointValueType::QDeclarativePointValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativePointValueType::read(QObject *obj, int idx)
{
    void *a[] = { &point, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativePointValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &point, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativePointValueType::value()
{
    return QVariant(point);
}

void QDeclarativePointValueType::setValue(QVariant value)
{
    point = qvariant_cast<QPoint>(value);
}

int QDeclarativePointValueType::x() const
{
    return point.x();
}

int QDeclarativePointValueType::y() const
{
    return point.y();
}

void QDeclarativePointValueType::setX(int x)
{
    point.setX(x);
}

void QDeclarativePointValueType::setY(int y)
{
    point.setY(y);
}

QDeclarativeSizeFValueType::QDeclarativeSizeFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeSizeFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &size, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeSizeFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &size, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeSizeFValueType::value()
{
    return QVariant(size);
}

void QDeclarativeSizeFValueType::setValue(QVariant value)
{
    size = qvariant_cast<QSizeF>(value);
}

qreal QDeclarativeSizeFValueType::width() const
{
    return size.width();
}

qreal QDeclarativeSizeFValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeFValueType::setWidth(qreal w)
{
    size.setWidth(w);
}

void QDeclarativeSizeFValueType::setHeight(qreal h)
{
    size.setHeight(h);
}

QDeclarativeSizeValueType::QDeclarativeSizeValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeSizeValueType::read(QObject *obj, int idx)
{
    void *a[] = { &size, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeSizeValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &size, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeSizeValueType::value()
{
    return QVariant(size);
}

void QDeclarativeSizeValueType::setValue(QVariant value)
{
    size = qvariant_cast<QSize>(value);
}

int QDeclarativeSizeValueType::width() const
{
    return size.width();
}

int QDeclarativeSizeValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeValueType::setWidth(int w)
{
    size.setWidth(w);
}

void QDeclarativeSizeValueType::setHeight(int h)
{
    size.setHeight(h);
}

QDeclarativeRectFValueType::QDeclarativeRectFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeRectFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &rect, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeRectFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &rect, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeRectFValueType::value()
{
    return QVariant(rect);
}

void QDeclarativeRectFValueType::setValue(QVariant value)
{
    rect = qvariant_cast<QRectF>(value);
}

qreal QDeclarativeRectFValueType::x() const
{
    return rect.x();
}

qreal QDeclarativeRectFValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectFValueType::setX(qreal x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectFValueType::setY(qreal y)
{
    rect.moveTop(y);
}

qreal QDeclarativeRectFValueType::width() const
{
    return rect.width();
}

qreal QDeclarativeRectFValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectFValueType::setWidth(qreal w)
{
    rect.setWidth(w);
}

void QDeclarativeRectFValueType::setHeight(qreal h)
{
    rect.setHeight(h);
}

QDeclarativeRectValueType::QDeclarativeRectValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeRectValueType::read(QObject *obj, int idx)
{
    void *a[] = { &rect, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeRectValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &rect, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeRectValueType::value()
{
    return QVariant(rect);
}

void QDeclarativeRectValueType::setValue(QVariant value)
{
    rect = qvariant_cast<QRect>(value);
}

int QDeclarativeRectValueType::x() const
{
    return rect.x();
}

int QDeclarativeRectValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectValueType::setX(int x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectValueType::setY(int y)
{
    rect.moveTop(y);
}

int QDeclarativeRectValueType::width() const
{
    return rect.width();
}

int QDeclarativeRectValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectValueType::setWidth(int w)
{
    rect.setWidth(w);
}

void QDeclarativeRectValueType::setHeight(int h)
{
    rect.setHeight(h);
}

QDeclarativeVector3DValueType::QDeclarativeVector3DValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeVector3DValueType::read(QObject *obj, int idx)
{
    void *a[] = { &vector, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeVector3DValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &vector, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeVector3DValueType::value()
{
    return QVariant(vector);
}

void QDeclarativeVector3DValueType::setValue(QVariant value)
{
    vector = qvariant_cast<QVector3D>(value);
}

qreal QDeclarativeVector3DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector3DValueType::y() const
{
    return vector.y();
}

qreal QDeclarativeVector3DValueType::z() const
{
    return vector.z();
}

void QDeclarativeVector3DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector3DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QDeclarativeVector3DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

QDeclarativeEasingValueType::QDeclarativeEasingValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeEasingValueType::read(QObject *obj, int idx)
{
    void *a[] = { &easing, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeEasingValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &easing, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeEasingValueType::value()
{
    return QVariant(easing);
}

void QDeclarativeEasingValueType::setValue(QVariant value)
{
    easing = qvariant_cast<QEasingCurve>(value);
}

QDeclarativeEasingValueType::Type QDeclarativeEasingValueType::type() const
{
    return (QDeclarativeEasingValueType::Type)easing.type();
}

qreal QDeclarativeEasingValueType::amplitude() const
{
    return easing.amplitude();
}

qreal QDeclarativeEasingValueType::overshoot() const
{
    return easing.overshoot();
}

qreal QDeclarativeEasingValueType::period() const
{
    return easing.period();
}

void QDeclarativeEasingValueType::setType(QDeclarativeEasingValueType::Type type)
{
    easing.setType((QEasingCurve::Type)type);
}

void QDeclarativeEasingValueType::setAmplitude(qreal amplitude)
{
    easing.setAmplitude(amplitude);
}

void QDeclarativeEasingValueType::setOvershoot(qreal overshoot)
{
    easing.setOvershoot(overshoot);
}

void QDeclarativeEasingValueType::setPeriod(qreal period)
{
    easing.setPeriod(period);
}

QDeclarativeFontValueType::QDeclarativeFontValueType(QObject *parent)
: QDeclarativeValueType(parent), pixelSizeSet(false), pointSizeSet(false)
{
}

void QDeclarativeFontValueType::read(QObject *obj, int idx)
{
    void *a[] = { &font, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
    pixelSizeSet = false;
    pointSizeSet = false;
}

void QDeclarativeFontValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &font, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeFontValueType::value()
{
    return QVariant(font);
}

void QDeclarativeFontValueType::setValue(QVariant value)
{
    font = qvariant_cast<QFont>(value);
}


QString QDeclarativeFontValueType::family() const
{
    return font.family();
}

void QDeclarativeFontValueType::setFamily(const QString &family)
{
    font.setFamily(family);
}

bool QDeclarativeFontValueType::bold() const
{
    return font.bold();
}

void QDeclarativeFontValueType::setBold(bool b)
{
    font.setBold(b);
}

QDeclarativeFontValueType::FontWeight QDeclarativeFontValueType::weight() const
{
    return (QDeclarativeFontValueType::FontWeight)font.weight();
}

void QDeclarativeFontValueType::setWeight(QDeclarativeFontValueType::FontWeight w)
{
    font.setWeight((QFont::Weight)w);
}

bool QDeclarativeFontValueType::italic() const
{
    return font.italic();
}

void QDeclarativeFontValueType::setItalic(bool b)
{
    font.setItalic(b);
}

bool QDeclarativeFontValueType::underline() const
{
    return font.underline();
}

void QDeclarativeFontValueType::setUnderline(bool b)
{
    font.setUnderline(b);
}

bool QDeclarativeFontValueType::overline() const
{
    return font.overline();
}

void QDeclarativeFontValueType::setOverline(bool b)
{
    font.setOverline(b);
}

bool QDeclarativeFontValueType::strikeout() const
{
    return font.strikeOut();
}

void QDeclarativeFontValueType::setStrikeout(bool b)
{
    font.setStrikeOut(b);
}

qreal QDeclarativeFontValueType::pointSize() const
{
    return font.pointSizeF();
}

void QDeclarativeFontValueType::setPointSize(qreal size)
{
    if (pixelSizeSet) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        pointSizeSet = true;
        font.setPointSizeF(size);
    } else {
        pointSizeSet = false;
    }
}

int QDeclarativeFontValueType::pixelSize() const
{
    return font.pixelSize();
}

void QDeclarativeFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if (pointSizeSet)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        font.setPixelSize(size);
        pixelSizeSet = true;
    } else {
        pixelSizeSet = false;
    }
}

QDeclarativeFontValueType::Capitalization QDeclarativeFontValueType::capitalization() const
{
    return (QDeclarativeFontValueType::Capitalization)font.capitalization();
}

void QDeclarativeFontValueType::setCapitalization(QDeclarativeFontValueType::Capitalization c)
{
    font.setCapitalization((QFont::Capitalization)c);
}

qreal QDeclarativeFontValueType::letterSpacing() const
{
    return font.letterSpacing();
}

void QDeclarativeFontValueType::setLetterSpacing(qreal size)
{
    font.setLetterSpacing(QFont::PercentageSpacing, size);
}

qreal QDeclarativeFontValueType::wordSpacing() const
{
    return font.wordSpacing();
}

void QDeclarativeFontValueType::setWordSpacing(qreal size)
{
    font.setWordSpacing(size);
}

QT_END_NAMESPACE
