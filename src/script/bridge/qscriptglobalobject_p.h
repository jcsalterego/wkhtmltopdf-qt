/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
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

#ifndef QSCRIPTGLOBALOBJECT_P_H
#define QSCRIPTGLOBALOBJECT_P_H

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

#include <QtCore/qobjectdefs.h>

#include "JSGlobalObject.h"

QT_BEGIN_NAMESPACE

namespace QScript
{

class GlobalObject : public JSC::JSGlobalObject
{
public:
    GlobalObject();
    virtual ~GlobalObject();
    virtual JSC::UString className() const { return "global"; }
    virtual void mark();
    virtual bool getOwnPropertySlot(JSC::ExecState*,
                                    const JSC::Identifier& propertyName,
                                    JSC::PropertySlot&);
    virtual void put(JSC::ExecState* exec, const JSC::Identifier& propertyName,
                     JSC::JSValue, JSC::PutPropertySlot&);
    virtual void putWithAttributes(JSC::ExecState* exec, const JSC::Identifier& propertyName,
                                   JSC::JSValue value, unsigned attributes);
    virtual bool deleteProperty(JSC::ExecState*,
                                const JSC::Identifier& propertyName,
                                bool checkDontDelete = true);
    virtual bool getPropertyAttributes(JSC::ExecState*, const JSC::Identifier&,
                                       unsigned&) const;
    virtual void getPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&,
                                  unsigned listedAttributes = JSC::Structure::Prototype);
    virtual void defineGetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* getterFunction);
    virtual void defineSetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* setterFunction);
    virtual JSC::JSValue lookupGetter(JSC::ExecState*, const JSC::Identifier& propertyName);
    virtual JSC::JSValue lookupSetter(JSC::ExecState*, const JSC::Identifier& propertyName);

public:
    JSC::JSObject *customGlobalObject;
};

class OriginalGlobalObjectProxy : public JSC::JSObject
{
public:
    explicit OriginalGlobalObjectProxy(WTF::PassRefPtr<JSC::Structure> sid,
                                       JSC::JSGlobalObject *object)
        : JSC::JSObject(sid), originalGlobalObject(object)
    {}
    virtual ~OriginalGlobalObjectProxy()
    {}
    virtual JSC::UString className() const
    { return originalGlobalObject->className(); }
    virtual void mark()
    {
        Q_ASSERT(!marked());
        if (!originalGlobalObject->marked())
            originalGlobalObject->JSC::JSGlobalObject::mark();
        JSC::JSObject::mark();
    }
    virtual bool getOwnPropertySlot(JSC::ExecState* exec,
                                    const JSC::Identifier& propertyName,
                                    JSC::PropertySlot& slot)
    { return originalGlobalObject->JSC::JSGlobalObject::getOwnPropertySlot(exec, propertyName, slot); }
    virtual void put(JSC::ExecState* exec, const JSC::Identifier& propertyName,
                     JSC::JSValue value, JSC::PutPropertySlot& slot)
    { originalGlobalObject->JSC::JSGlobalObject::put(exec, propertyName, value, slot); }
    virtual void putWithAttributes(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::JSValue value, unsigned attributes)
    { originalGlobalObject->JSC::JSGlobalObject::putWithAttributes(exec, propertyName, value, attributes); }
    virtual bool deleteProperty(JSC::ExecState* exec,
                                const JSC::Identifier& propertyName, bool checkDontDelete = true)
    { return originalGlobalObject->JSC::JSGlobalObject::deleteProperty(exec, propertyName, checkDontDelete); }
    virtual bool getPropertyAttributes(JSC::ExecState* exec, const JSC::Identifier& propertyName,
                                       unsigned& attributes) const
    { return originalGlobalObject->JSC::JSGlobalObject::getPropertyAttributes(exec, propertyName, attributes); }
    virtual void getPropertyNames(JSC::ExecState* exec, JSC::PropertyNameArray& propertyNames,
                                  unsigned listedAttributes = JSC::Structure::Prototype)
    { originalGlobalObject->JSC::JSGlobalObject::getPropertyNames(exec, propertyNames, listedAttributes); }
    virtual void defineGetter(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::JSObject* getterFunction)
    { originalGlobalObject->JSC::JSGlobalObject::defineGetter(exec, propertyName, getterFunction); }
    virtual void defineSetter(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::JSObject* setterFunction)
    { originalGlobalObject->JSC::JSGlobalObject::defineSetter(exec, propertyName, setterFunction); }
    virtual JSC::JSValue lookupGetter(JSC::ExecState* exec, const JSC::Identifier& propertyName)
    { return originalGlobalObject->JSC::JSGlobalObject::lookupGetter(exec, propertyName); }
    virtual JSC::JSValue lookupSetter(JSC::ExecState* exec, const JSC::Identifier& propertyName)
    { return originalGlobalObject->JSC::JSGlobalObject::lookupSetter(exec, propertyName); }
private:
    JSC::JSGlobalObject *originalGlobalObject;
};

} // namespace QScript

QT_END_NAMESPACE

#endif