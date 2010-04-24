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

#ifndef QDECLARATIVEPRIVATE_H
#define QDECLARATIVEPRIVATE_H

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#ifndef Q_OS_WIN
#include <stdint.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

typedef QObject *(*QDeclarativeAttachedPropertiesFunc)(QObject *);

template <typename TYPE>
class QDeclarativeTypeInfo
{
public:
    enum {
        hasAttachedProperties = 0
    };
};


class QDeclarativeCustomParser;
namespace QDeclarativePrivate
{
    void Q_DECLARATIVE_EXPORT qdeclarativeelement_destructor(QObject *);
    template<typename T>
    class QDeclarativeElement : public T
    {
    public:
        virtual ~QDeclarativeElement() { 
            QDeclarativePrivate::qdeclarativeelement_destructor(this); 
        }
    };

    template<typename T>
    void createInto(void *memory) { new (memory) QDeclarativeElement<T>; }

    template<typename T>
    QObject *createParent(QObject *p) { return new T(p); }

    template<class From, class To, int N>
    struct StaticCastSelectorClass
    {
        static inline int cast() { return -1; }
    };

    template<class From, class To>
    struct StaticCastSelectorClass<From, To, sizeof(int)>
    {
        static inline int cast() { return (int)((intptr_t)static_cast<To *>((From *)0x10000000)) - 0x10000000; }
    };

    template<class From, class To>
    struct StaticCastSelector
    {
        typedef int yes_type;
        typedef char no_type;

        static yes_type check(To *);
        static no_type check(...);

        static inline int cast()
        {
            return StaticCastSelectorClass<From, To, sizeof(check((From *)0))>::cast();
        }
    };

    template <typename T>
    struct has_attachedPropertiesMember
    {
        static bool const value = QDeclarativeTypeInfo<T>::hasAttachedProperties;
    };

    template <typename T, bool hasMember>
    class has_attachedPropertiesMethod 
    {
        typedef int yes_type;
        typedef char no_type;

        template<typename ReturnType>
        static yes_type check(ReturnType *(*)(QObject *));
        static no_type check(...);

    public:
        static bool const value = sizeof(check(&T::qmlAttachedProperties)) == sizeof(yes_type);
    }; 

    template <typename T>
    class has_attachedPropertiesMethod<T, false>
    {
    public:
        static bool const value = false;
    };

    template<typename T, int N>
    class AttachedPropertySelector
    {
    public:
        static inline QDeclarativeAttachedPropertiesFunc func() { return 0; }
        static inline const QMetaObject *metaObject() { return 0; }
    };
    template<typename T>
    class AttachedPropertySelector<T, 1>
    {
        static inline QObject *attachedProperties(QObject *obj) {
            return T::qmlAttachedProperties(obj);
        }
        template<typename ReturnType>
        static inline const QMetaObject *attachedPropertiesMetaObject(ReturnType *(*)(QObject *)) {
            return &ReturnType::staticMetaObject;
        }
    public:
        static inline QDeclarativeAttachedPropertiesFunc func() {
            return &attachedProperties;
        }
        static inline const QMetaObject *metaObject() {
            return attachedPropertiesMetaObject(&T::qmlAttachedProperties);
        }
    };

    template<typename T>
    inline QDeclarativeAttachedPropertiesFunc attachedPropertiesFunc()
    {
        return AttachedPropertySelector<T, has_attachedPropertiesMethod<T, has_attachedPropertiesMember<T>::value>::value>::func();
    }

    template<typename T>
    inline const QMetaObject *attachedPropertiesMetaObject()
    {
        return AttachedPropertySelector<T, has_attachedPropertiesMethod<T, has_attachedPropertiesMember<T>::value>::value>::metaObject();
    }

    struct RegisterType {
        int version;

        int typeId;
        int listId;
        int objectSize;
        void (*create)(void *);
        QString noCreationReason;

        const char *uri;
        int versionMajor;
        int versionMinor;
        const char *elementName;
        const QMetaObject *metaObject;

        QDeclarativeAttachedPropertiesFunc attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QDeclarativeCustomParser *customParser;
    };

    struct RegisterInterface {
        int version;

        int typeId;
        int listId;

        const char *iid;
    };

    int Q_DECLARATIVE_EXPORT registerType(const RegisterType &);
    int Q_DECLARATIVE_EXPORT registerType(const RegisterInterface &);

}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEPRIVATE_H
