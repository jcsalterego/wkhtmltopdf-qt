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

#ifndef QDECLARATIVEMETATYPE_P_H
#define QDECLARATIVEMETATYPE_P_H

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

#include "qdeclarative.h"

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qbitarray.h>

QT_BEGIN_NAMESPACE

class QDeclarativeType;
class QDeclarativeCustomParser;
class Q_DECLARATIVE_EXPORT QDeclarativeMetaType
{
public:
    static bool copy(int type, void *data, const void *copy = 0);

    static QList<QByteArray> qmlTypeNames();
    static QList<QDeclarativeType*> qmlTypes();

    static QDeclarativeType *qmlType(const QByteArray &, int, int);
    static QDeclarativeType *qmlType(const QMetaObject *);
    static QDeclarativeType *qmlType(int);

    static QMetaProperty defaultProperty(const QMetaObject *);
    static QMetaProperty defaultProperty(QObject *);
    static QMetaMethod defaultMethod(const QMetaObject *);
    static QMetaMethod defaultMethod(QObject *);

    static bool isQObject(int);
    static QObject *toQObject(const QVariant &, bool *ok = 0);

    static int listType(int);
    static int attachedPropertiesFuncId(const QMetaObject *);
    static QDeclarativeAttachedPropertiesFunc attachedPropertiesFuncById(int);

    enum TypeCategory { Unknown, Object, List };
    static TypeCategory typeCategory(int);
        
    static bool isInterface(int);
    static const char *interfaceIId(int);
    static bool isList(int);

    typedef QVariant (*StringConverter)(const QString &);
    static void registerCustomStringConverter(int, StringConverter);
    static StringConverter customStringConverter(int);

    static bool isModule(const QByteArray &module, int versionMajor, int versionMinor);
};

class QDeclarativeTypePrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeType
{
public:
    QByteArray typeName() const;
    QByteArray qmlTypeName() const;

    int majorVersion() const;
    int minorVersion() const;
    bool availableInVersion(int vmajor, int vminor) const;

    QObject *create() const;
    void create(QObject **, void **, size_t) const;

    typedef void (*CreateFunc)(void *);
    CreateFunc createFunction() const;
    int createSize() const;

    QDeclarativeCustomParser *customParser() const;

    bool isCreatable() const;
    bool isExtendedType() const;
    QString noCreationReason() const;

    bool isInterface() const;
    int typeId() const;
    int qListTypeId() const;

    const QMetaObject *metaObject() const;
    const QMetaObject *baseMetaObject() const;

    QDeclarativeAttachedPropertiesFunc attachedPropertiesFunction() const;
    const QMetaObject *attachedPropertiesType() const;

    int parserStatusCast() const;
    QVariant fromObject(QObject *) const;
    const char *interfaceIId() const;
    int propertyValueSourceCast() const;
    int propertyValueInterceptorCast() const;

    int index() const;
private:
    friend class QDeclarativeTypePrivate;
    friend struct QDeclarativeMetaTypeData;
    friend int QDeclarativePrivate::registerType(const QDeclarativePrivate::RegisterInterface &);
    friend int QDeclarativePrivate::registerType(const QDeclarativePrivate::RegisterType &);
    QDeclarativeType(int, const QDeclarativePrivate::RegisterInterface &);
    QDeclarativeType(int, const QDeclarativePrivate::RegisterType &);
    ~QDeclarativeType();

    QDeclarativeTypePrivate *d;
};

QT_END_NAMESPACE

#endif // QDECLARATIVEMETATYPE_P_H

