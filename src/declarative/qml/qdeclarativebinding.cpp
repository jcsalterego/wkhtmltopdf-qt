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

#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativebinding_p_p.h"

#include "qdeclarative.h"
#include "qdeclarativecontext.h"
#include "qdeclarativeinfo.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativedata_p.h"
#include "private/qdeclarativestringconverters_p.h"

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QDeclarativeBindingData::QDeclarativeBindingData()
: updating(false), enabled(false)
{
}

QDeclarativeBindingData::~QDeclarativeBindingData()
{
    removeError();
}

void QDeclarativeBindingData::refresh()
{
    if (enabled && !updating && q) {
        QDeclarativeBinding *b = static_cast<QDeclarativeBinding *>(QDeclarativeExpressionPrivate::get(q));
        b->update();
    }
}

QDeclarativeBindingPrivate::QDeclarativeBindingPrivate()
: QDeclarativeExpressionPrivate(new QDeclarativeBindingData)
{
}

QDeclarativeBinding::QDeclarativeBinding(void *data, QDeclarativeRefCount *rc, QObject *obj, 
                                         QDeclarativeContextData *ctxt, const QString &url, int lineNumber, 
                                         QObject *parent)
: QDeclarativeExpression(ctxt, data, rc, obj, url, lineNumber, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::QDeclarativeBinding(const QString &str, QObject *obj, QDeclarativeContext *ctxt, 
                                         QObject *parent)
: QDeclarativeExpression(QDeclarativeContextData::get(ctxt), str, obj, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::QDeclarativeBinding(const QString &str, QObject *obj, QDeclarativeContextData *ctxt, 
                                         QObject *parent)
: QDeclarativeExpression(ctxt, str, obj, *new QDeclarativeBindingPrivate)
{
    setParent(parent);
    setNotifyOnValueChanged(true);
}

QDeclarativeBinding::~QDeclarativeBinding()
{
}

void QDeclarativeBinding::setTarget(const QDeclarativeProperty &prop)
{
    Q_D(QDeclarativeBinding);
    d->bindingData()->property = prop;

    update();
}

QDeclarativeProperty QDeclarativeBinding::property() const 
{
   Q_D(const QDeclarativeBinding);
   return d->bindingData()->property; 
}

void QDeclarativeBinding::update(QDeclarativePropertyPrivate::WriteFlags flags)
{
    Q_D(QDeclarativeBinding);

    QDeclarativeBindingData *data = d->bindingData();

    if (!data->enabled || !data->context() || !data->context()->isValid())
        return;

    data->addref();

    if (!data->updating) {
        data->updating = true;

        if (data->property.propertyType() == qMetaTypeId<QDeclarativeBinding *>()) {

            int idx = data->property.index();
            Q_ASSERT(idx != -1);


            QDeclarativeBinding *t = this;
            int status = -1;
            void *a[] = { &t, 0, &status, &flags };
            QMetaObject::metacall(data->property.object(),
                                  QMetaObject::WriteProperty,
                                  idx, a);

        } else {
            QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(data->context()->engine);

            bool isUndefined = false;
            QVariant value;

            QScriptValue scriptValue = d->scriptValue(0, &isUndefined);
            if (data->property.propertyTypeCategory() == QDeclarativeProperty::List) {
                value = ep->scriptValueToVariant(scriptValue, qMetaTypeId<QList<QObject *> >());
            } else if (scriptValue.isNull() && 
                       data->property.propertyTypeCategory() == QDeclarativeProperty::Object) {
                value = QVariant::fromValue((QObject *)0);
            } else {
                value = ep->scriptValueToVariant(scriptValue, data->property.propertyType());
                if (value.userType() == QMetaType::QObjectStar && !qvariant_cast<QObject*>(value)) {
                    // If the object is null, we extract the predicted type.  While this isn't
                    // 100% reliable, in many cases it gives us better error messages if we
                    // assign this null-object to an incompatible property
                    int type = ep->objectClass->objectType(scriptValue);
                    QObject *o = 0;
                    value = QVariant(type, (void *)&o);
                }
            }


            if (data->error.isValid()) {

            } else if (isUndefined && data->property.isResettable()) {

                data->property.reset();

            } else if (isUndefined && data->property.propertyType() == qMetaTypeId<QVariant>()) {

                QDeclarativePropertyPrivate::write(data->property, QVariant(), flags);

            } else if (isUndefined) {

                QUrl url = QUrl(data->url);
                int line = data->line;
                if (url.isEmpty()) url = QUrl(QLatin1String("<Unknown File>"));

                data->error.setUrl(url);
                data->error.setLine(line);
                data->error.setColumn(-1);
                data->error.setDescription(QLatin1String("Unable to assign [undefined] to ") + QLatin1String(QMetaType::typeName(data->property.propertyType())));

            } else if (data->property.object() && 
                       !QDeclarativePropertyPrivate::write(data->property, value, flags)) {

                QUrl url = QUrl(data->url);
                int line = data->line;
                if (url.isEmpty()) url = QUrl(QLatin1String("<Unknown File>"));

                const char *valueType = 0;
                if (value.userType() == QVariant::Invalid) valueType = "null";
                else valueType = QMetaType::typeName(value.userType());

                data->error.setUrl(url);
                data->error.setLine(line);
                data->error.setColumn(-1);
                data->error.setDescription(QLatin1String("Unable to assign ") +
                                           QLatin1String(valueType) +
                                           QLatin1String(" to ") +
                                           QLatin1String(QMetaType::typeName(data->property.propertyType())));
            }

            if (data->error.isValid()) {
               if (!data->addError(ep)) ep->warning(this->error());
            } else {
                data->removeError();
            }
        }

        data->updating = false;
    } else {
        qmlInfo(data->property.object()) << tr("Binding loop detected for property \"%1\"").arg(data->property.name());
    }

    data->release();
}

void QDeclarativeBindingPrivate::emitValueChanged()
{
    Q_Q(QDeclarativeBinding);
    q->update();
}

void QDeclarativeBinding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    Q_D(QDeclarativeBinding);
    d->bindingData()->enabled = e;
    setNotifyOnValueChanged(e);

    QDeclarativeAbstractBinding::setEnabled(e, flags);

    if (e) {
        addToObject(d->bindingData()->property.object());
        update(flags);
    } else {
        removeFromObject();
    }
}

int QDeclarativeBinding::propertyIndex()
{
    Q_D(QDeclarativeBinding);
    return QDeclarativePropertyPrivate::bindingIndex(d->bindingData()->property);
}

bool QDeclarativeBinding::enabled() const
{
    Q_D(const QDeclarativeBinding);

    return d->bindingData()->enabled;
}

QString QDeclarativeBinding::expression() const
{
    return QDeclarativeExpression::expression();
}

QDeclarativeAbstractBinding::QDeclarativeAbstractBinding()
: m_object(0), m_mePtr(0), m_prevBinding(0), m_nextBinding(0)
{
}

QDeclarativeAbstractBinding::~QDeclarativeAbstractBinding()
{
    removeFromObject();
    if (m_mePtr)
        *m_mePtr = 0;
}

void QDeclarativeAbstractBinding::destroy()
{
    delete this;
}

void QDeclarativeAbstractBinding::addToObject(QObject *object)
{
    Q_ASSERT(object);

    if (m_object == object)
        return;

    int index = propertyIndex();

    removeFromObject();

    Q_ASSERT(!m_prevBinding);

    m_object = object;
    QDeclarativeData *data = QDeclarativeData::get(object, true);

    if (index & 0xFF000000) {
        // Value type

        int coreIndex = index & 0xFFFFFF;

        // Find the value type proxy (if there is one)
        QDeclarativeValueTypeProxyBinding *proxy = 0;
        if (data->hasBindingBit(coreIndex)) {
            QDeclarativeAbstractBinding *b = data->bindings;
            while (b && b->propertyIndex() != coreIndex)
                b = b->m_nextBinding;
            Q_ASSERT(b && b->bindingType() == QDeclarativeAbstractBinding::ValueTypeProxy);
            proxy = static_cast<QDeclarativeValueTypeProxyBinding *>(b);
        }

        if (!proxy) 
            proxy = new QDeclarativeValueTypeProxyBinding(object, coreIndex);
        proxy->addToObject(object);

        m_nextBinding = proxy->m_bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &proxy->m_bindings;
        proxy->m_bindings = this;

    } else {
        m_nextBinding = data->bindings;
        if (m_nextBinding) m_nextBinding->m_prevBinding = &m_nextBinding;
        m_prevBinding = &data->bindings;
        data->bindings = this;

        data->setBindingBit(m_object, index);
    }
}

void QDeclarativeAbstractBinding::removeFromObject()
{
    if (m_prevBinding) {
        int index = propertyIndex();

        Q_ASSERT(m_object);

        *m_prevBinding = m_nextBinding;
        if (m_nextBinding) m_nextBinding->m_prevBinding = m_prevBinding;
        m_prevBinding = 0;
        m_nextBinding = 0;

        if (index & 0xFF000000) {
            // Value type - we don't remove the proxy from the object.  It will sit their happily
            // doing nothing for ever more.
        } else {
            QDeclarativeData *data = QDeclarativeData::get(m_object, false);
            if (data) data->clearBindingBit(index);
        }

        m_object = 0;
    }
}

void QDeclarativeAbstractBinding::clear()
{
    if (m_mePtr) {
        *m_mePtr = 0;
        m_mePtr = 0;
    }
}

QString QDeclarativeAbstractBinding::expression() const
{
    return QLatin1String("<Unknown>");
}

void QDeclarativeAbstractBinding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags)
{
    if (e) m_mePtr = 0;
}

QDeclarativeValueTypeProxyBinding::QDeclarativeValueTypeProxyBinding(QObject *o, int index)
: m_object(o), m_index(index), m_bindings(0)
{
}

QDeclarativeValueTypeProxyBinding::~QDeclarativeValueTypeProxyBinding()
{
    while (m_bindings) {
        QDeclarativeAbstractBinding *binding = m_bindings;
        binding->setEnabled(false, 0);
        binding->destroy();
    }
}

void QDeclarativeValueTypeProxyBinding::setEnabled(bool e, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (e) {
        addToObject(m_object);

        QDeclarativeAbstractBinding *bindings = m_bindings;
        m_bindings = 0;
        recursiveEnable(bindings, flags);
    } else {
        removeFromObject();

        QDeclarativeAbstractBinding *bindings = m_bindings;
        m_bindings = 0;
        recursiveDisable(bindings);
    }
}

void QDeclarativeValueTypeProxyBinding::recursiveEnable(QDeclarativeAbstractBinding *b, QDeclarativePropertyPrivate::WriteFlags flags)
{
    if (!b)
        return;

    QDeclarativeAbstractBinding *next = b->m_nextBinding;
    b->m_prevBinding = 0;
    b->m_nextBinding = 0;
    Q_ASSERT(b->m_mePtr == 0);
    b->m_mePtr = &b;

    recursiveEnable(next, flags);

    if (b)
        b->setEnabled(true, flags);
}

void QDeclarativeValueTypeProxyBinding::recursiveDisable(QDeclarativeAbstractBinding *b)
{
    if (!b)
        return;

    recursiveDisable(b->m_nextBinding);

    b->setEnabled(false, 0);

    Q_ASSERT(b->m_prevBinding == 0);
    Q_ASSERT(b->m_nextBinding == 0);
    b->m_nextBinding = m_bindings;
    if (b->m_nextBinding) b->m_nextBinding->m_prevBinding = &b->m_nextBinding;
    b->m_prevBinding = &m_bindings;
    m_bindings = b;
}

int QDeclarativeValueTypeProxyBinding::propertyIndex()
{
    return m_index;
}

void QDeclarativeValueTypeProxyBinding::update(QDeclarativePropertyPrivate::WriteFlags)
{
}

QDeclarativeAbstractBinding *QDeclarativeValueTypeProxyBinding::binding(int propertyIndex)
{
    QDeclarativeAbstractBinding *binding = m_bindings;
    
    while (binding && binding->propertyIndex() != propertyIndex) 
        binding = binding->m_nextBinding;

    return binding;
}

QT_END_NAMESPACE
