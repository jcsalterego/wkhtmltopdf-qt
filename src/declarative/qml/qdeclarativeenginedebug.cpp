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

#include "private/qdeclarativeenginedebug_p.h"

#include "private/qdeclarativeboundsignal_p.h"
#include "qdeclarativeengine.h"
#include "private/qdeclarativemetatype_p.h"
#include "qdeclarativeproperty.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativewatcher_p.h"
#include "private/qdeclarativevaluetype_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

QList<QDeclarativeEngine *> QDeclarativeEngineDebugServer::m_engines;
QDeclarativeEngineDebugServer::QDeclarativeEngineDebugServer(QObject *parent)
: QDeclarativeDebugService(QLatin1String("QDeclarativeEngine"), parent),
  m_watch(new QDeclarativeWatcher(this))
{
    QObject::connect(m_watch, SIGNAL(propertyChanged(int,int,QMetaProperty,QVariant)),
                     this, SLOT(propertyChanged(int,int,QMetaProperty,QVariant)));
}

QDataStream &operator<<(QDataStream &ds, 
                        const QDeclarativeEngineDebugServer::QDeclarativeObjectData &data)
{
    ds << data.url << data.lineNumber << data.columnNumber << data.idString
       << data.objectName << data.objectType << data.objectId << data.contextId;
    return ds;
}

QDataStream &operator>>(QDataStream &ds, 
                        QDeclarativeEngineDebugServer::QDeclarativeObjectData &data)
{
    ds >> data.url >> data.lineNumber >> data.columnNumber >> data.idString
       >> data.objectName >> data.objectType >> data.objectId >> data.contextId;
    return ds;
}

QDataStream &operator<<(QDataStream &ds, 
                        const QDeclarativeEngineDebugServer::QDeclarativeObjectProperty &data)
{
    ds << (int)data.type << data.name << data.value << data.valueTypeName
       << data.binding << data.hasNotifySignal;
    return ds;
}

QDataStream &operator>>(QDataStream &ds,  
                        QDeclarativeEngineDebugServer::QDeclarativeObjectProperty &data)
{
    int type;
    ds >> type >> data.name >> data.value >> data.valueTypeName
       >> data.binding >> data.hasNotifySignal;
    data.type = (QDeclarativeEngineDebugServer::QDeclarativeObjectProperty::Type)type;
    return ds;
}

QDeclarativeEngineDebugServer::QDeclarativeObjectProperty 
QDeclarativeEngineDebugServer::propertyData(QObject *obj, int propIdx)
{
    QDeclarativeObjectProperty rv;

    QMetaProperty prop = obj->metaObject()->property(propIdx);

    rv.type = QDeclarativeObjectProperty::Unknown;
    rv.valueTypeName = QString::fromUtf8(prop.typeName());
    rv.name = QString::fromUtf8(prop.name());
    rv.hasNotifySignal = prop.hasNotifySignal();
    QDeclarativeAbstractBinding *binding = 
        QDeclarativePropertyPrivate::binding(QDeclarativeProperty(obj, rv.name));
    if (binding)
        rv.binding = binding->expression();

    QVariant value = prop.read(obj);
    rv.value = valueContents(value);

    if (QDeclarativeValueTypeFactory::isValueType(prop.userType())) {
        rv.type = QDeclarativeObjectProperty::Basic;
    } else if (QDeclarativeMetaType::isQObject(prop.userType()))  {
        rv.type = QDeclarativeObjectProperty::Object;
    } else if (QDeclarativeMetaType::isList(prop.userType())) {
        rv.type = QDeclarativeObjectProperty::List;
    }

    return rv;
}

QVariant QDeclarativeEngineDebugServer::valueContents(const QVariant &value) const
{
    int userType = value.userType();
    if (QDeclarativeValueTypeFactory::isValueType(userType))
        return value;

    /*
    if (QDeclarativeMetaType::isList(userType)) {
        int count = QDeclarativeMetaType::listCount(value);
        QVariantList contents;
        for (int i=0; i<count; i++)
            contents << valueContents(QDeclarativeMetaType::listAt(value, i));
        return contents;
    } else */
    if (QDeclarativeMetaType::isQObject(userType)) {
        QObject *o = QDeclarativeMetaType::toQObject(value);
        if (o) {
            QString name = o->objectName();
            if (name.isEmpty())
                name = QLatin1String("<unnamed object>");
            return name;
        }
    }

    return QLatin1String("<unknown value>");
}

void QDeclarativeEngineDebugServer::buildObjectDump(QDataStream &message, 
                                           QObject *object, bool recur)
{
    message << objectData(object);

    // Some children aren't added to an object until particular properties are read
    // - e.g. child state objects aren't added until the 'states' property is read -
    // but this should only affect internal objects that aren't shown by the
    // debugger anyway.

    QObjectList children = object->children();
    
    int childrenCount = children.count();
    for (int ii = 0; ii < children.count(); ++ii) {
        if (qobject_cast<QDeclarativeContext*>(children[ii]) || QDeclarativeBoundSignal::cast(children[ii]))
            --childrenCount;
    }

    message << childrenCount << recur;

    QList<QDeclarativeObjectProperty> fakeProperties;

    for (int ii = 0; ii < children.count(); ++ii) {
        QObject *child = children.at(ii);
        if (qobject_cast<QDeclarativeContext*>(child))
            continue;
        QDeclarativeBoundSignal *signal = QDeclarativeBoundSignal::cast(child);
        if (signal) {
            QDeclarativeObjectProperty prop;
            prop.type = QDeclarativeObjectProperty::SignalProperty;
            prop.hasNotifySignal = false;
            QDeclarativeExpression *expr = signal->expression();
            if (expr) {
                prop.value = expr->expression();
                QObject *scope = expr->scopeObject();
                if (scope) {
                    QString sig = QLatin1String(scope->metaObject()->method(signal->index()).signature());
                    int lparen = sig.indexOf(QLatin1Char('('));
                    if (lparen >= 0) {
                        QString methodName = sig.mid(0, lparen);
                        prop.name = QLatin1String("on") + methodName[0].toUpper()
                                + methodName.mid(1);
                    }
                }
            }
            fakeProperties << prop;
        } else {
            if (recur)
                buildObjectDump(message, child, recur);
            else
                message << objectData(child);
        }
    }

    message << (object->metaObject()->propertyCount() + fakeProperties.count());

    for (int ii = 0; ii < object->metaObject()->propertyCount(); ++ii) 
        message << propertyData(object, ii);

    for (int ii = 0; ii < fakeProperties.count(); ++ii)
        message << fakeProperties[ii];
}

void QDeclarativeEngineDebugServer::buildObjectList(QDataStream &message, QDeclarativeContext *ctxt)
{
    QDeclarativeContextData *p = QDeclarativeContextData::get(ctxt);

    QString ctxtName = ctxt->objectName();
    int ctxtId = QDeclarativeDebugService::idForObject(ctxt);

    message << ctxtName << ctxtId; 

    int count = 0;

    QDeclarativeContextData *child = p->childContexts;
    while (child) {
        if (!child->isInternal)
            ++count;
        child = child->nextChild;
    }

    message << count;

    child = p->childContexts;
    while (child) {
        if (!child->isInternal) 
            buildObjectList(message, child->asQDeclarativeContext());
        child = child->nextChild;
    }

    // Clean deleted objects
    QDeclarativeContextPrivate *ctxtPriv = QDeclarativeContextPrivate::get(ctxt);
    for (int ii = 0; ii < ctxtPriv->instances.count(); ++ii) {
        if (!ctxtPriv->instances.at(ii)) {
            ctxtPriv->instances.removeAt(ii);
            --ii;
        }
    }

    message << ctxtPriv->instances.count();
    for (int ii = 0; ii < ctxtPriv->instances.count(); ++ii) {
        message << objectData(ctxtPriv->instances.at(ii));
    }
}

QDeclarativeEngineDebugServer::QDeclarativeObjectData 
QDeclarativeEngineDebugServer::objectData(QObject *object)
{
    QDeclarativeData *ddata = QDeclarativeData::get(object);
    QDeclarativeObjectData rv;
    if (ddata && ddata->outerContext) {
        rv.url = ddata->outerContext->url;
        rv.lineNumber = ddata->lineNumber;
        rv.columnNumber = ddata->columnNumber;
    } else {
        rv.lineNumber = -1;
        rv.columnNumber = -1;
    }

    QDeclarativeContext *context = qmlContext(object);
    if (context) {
        QDeclarativeContextData *cdata = QDeclarativeContextData::get(context);
        if (cdata)
            rv.idString = cdata->findObjectId(object);
    }

    rv.objectName = object->objectName();
    rv.objectId = QDeclarativeDebugService::idForObject(object);
    rv.contextId = QDeclarativeDebugService::idForObject(qmlContext(object));

    QDeclarativeType *type = QDeclarativeMetaType::qmlType(object->metaObject());
    if (type) {
        QString typeName = QLatin1String(type->qmlTypeName());
        int lastSlash = typeName.lastIndexOf(QLatin1Char('/'));
        rv.objectType = lastSlash < 0 ? typeName : typeName.mid(lastSlash+1);
    } else {
        rv.objectType = QString::fromUtf8(object->metaObject()->className());
        int marker = rv.objectType.indexOf(QLatin1String("_QMLTYPE_"));
        if (marker != -1)
            rv.objectType = rv.objectType.left(marker);
    }

    return rv;
}

void QDeclarativeEngineDebugServer::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    QByteArray type;
    ds >> type;

    if (type == "LIST_ENGINES") {
        int queryId;
        ds >> queryId;

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LIST_ENGINES_R");
        rs << queryId << m_engines.count();

        for (int ii = 0; ii < m_engines.count(); ++ii) {
            QDeclarativeEngine *engine = m_engines.at(ii);

            QString engineName = engine->objectName();
            int engineId = QDeclarativeDebugService::idForObject(engine);

            rs << engineName << engineId;
        }

        sendMessage(reply);
    } else if (type == "LIST_OBJECTS") {
        int queryId;
        int engineId = -1;
        ds >> queryId >> engineId;

        QDeclarativeEngine *engine = 
            qobject_cast<QDeclarativeEngine *>(QDeclarativeDebugService::objectForId(engineId));

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("LIST_OBJECTS_R") << queryId;

        if (engine)
            buildObjectList(rs, engine->rootContext());

        sendMessage(reply);
    } else if (type == "FETCH_OBJECT") {
        int queryId;
        int objectId;
        bool recurse;

        ds >> queryId >> objectId >> recurse;

        QObject *object = QDeclarativeDebugService::objectForId(objectId);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("FETCH_OBJECT_R") << queryId;

        if (object) 
            buildObjectDump(rs, object, recurse);

        sendMessage(reply);
    } else if (type == "WATCH_OBJECT") {
        int queryId;
        int objectId;

        ds >> queryId >> objectId;
        bool ok = m_watch->addWatch(queryId, objectId);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_OBJECT_R") << queryId << ok;

        sendMessage(reply);
    } else if (type == "WATCH_PROPERTY") {
        int queryId;
        int objectId;
        QByteArray property;

        ds >> queryId >> objectId >> property;
        bool ok = m_watch->addWatch(queryId, objectId, property);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_PROPERTY_R") << queryId << ok;

        sendMessage(reply);
    } else if (type == "WATCH_EXPR_OBJECT") {
        int queryId;
        int debugId;
        QString expr;

        ds >> queryId >> debugId >> expr;
        bool ok = m_watch->addWatch(queryId, debugId, expr);

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("WATCH_EXPR_OBJECT_R") << queryId << ok;

        sendMessage(reply);
    } else if (type == "NO_WATCH") {
        int queryId;

        ds >> queryId;
        m_watch->removeWatch(queryId);
    } else if (type == "EVAL_EXPRESSION") {
        int queryId;
        int objectId;
        QString expr;

        ds >> queryId >> objectId >> expr;

        QObject *object = QDeclarativeDebugService::objectForId(objectId);
        QDeclarativeContext *context = qmlContext(object);
        QVariant result;
        if (object && context) {
            QDeclarativeExpression exprObj(context, expr, object);
            bool undefined = false;
            QVariant value = exprObj.evaluate(&undefined);
            if (undefined)
                result = QLatin1String("<undefined>");
            else
                result = valueContents(value);
        } else {
            result = QLatin1String("<unknown context>");
        }

        QByteArray reply;
        QDataStream rs(&reply, QIODevice::WriteOnly);
        rs << QByteArray("EVAL_EXPRESSION_R") << queryId << result;

        sendMessage(reply);
    }
}

void QDeclarativeEngineDebugServer::propertyChanged(int id, int objectId, const QMetaProperty &property, const QVariant &value)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);

    rs << QByteArray("UPDATE_WATCH") << id << objectId << QByteArray(property.name()) << valueContents(value);

    sendMessage(reply);
}

void QDeclarativeEngineDebugServer::addEngine(QDeclarativeEngine *engine)
{
    Q_ASSERT(engine);
    Q_ASSERT(!m_engines.contains(engine));

    m_engines.append(engine);
}

void QDeclarativeEngineDebugServer::remEngine(QDeclarativeEngine *engine)
{
    Q_ASSERT(engine);
    Q_ASSERT(m_engines.contains(engine));

    m_engines.removeAll(engine);
}

QT_END_NAMESPACE
