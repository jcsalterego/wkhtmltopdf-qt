/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(3D_CANVAS)

#include "JSCanvasArray.h"

#include "CanvasArray.h"
#include <runtime/Error.h>
#include <runtime/JSNumberCell.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSCanvasArray);

/* Hash table */

static const HashTableValue JSCanvasArrayTableValues[2] =
{
    { "length", DontDelete|ReadOnly, (intptr_t)jsCanvasArrayLength, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static JSC_CONST_HASHTABLE HashTable JSCanvasArrayTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSCanvasArrayTableValues, 0 };
#else
    { 2, 1, JSCanvasArrayTableValues, 0 };
#endif

/* Hash table for prototype */

static const HashTableValue JSCanvasArrayPrototypeTableValues[3] =
{
    { "sizeInBytes", DontDelete|Function, (intptr_t)jsCanvasArrayPrototypeFunctionSizeInBytes, (intptr_t)0 },
    { "alignedSizeInBytes", DontDelete|Function, (intptr_t)jsCanvasArrayPrototypeFunctionAlignedSizeInBytes, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static JSC_CONST_HASHTABLE HashTable JSCanvasArrayPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 3, JSCanvasArrayPrototypeTableValues, 0 };
#else
    { 4, 3, JSCanvasArrayPrototypeTableValues, 0 };
#endif

const ClassInfo JSCanvasArrayPrototype::s_info = { "CanvasArrayPrototype", 0, &JSCanvasArrayPrototypeTable, 0 };

JSObject* JSCanvasArrayPrototype::self(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSCanvasArray>(exec, globalObject);
}

bool JSCanvasArrayPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, &JSCanvasArrayPrototypeTable, this, propertyName, slot);
}

bool JSCanvasArrayPrototype::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSObject>(exec, &JSCanvasArrayPrototypeTable, this, propertyName, descriptor);
}

const ClassInfo JSCanvasArray::s_info = { "CanvasArray", 0, &JSCanvasArrayTable, 0 };

JSCanvasArray::JSCanvasArray(PassRefPtr<Structure> structure, JSDOMGlobalObject* globalObject, PassRefPtr<CanvasArray> impl)
    : DOMObjectWithGlobalPointer(structure, globalObject)
    , m_impl(impl)
{
}

JSCanvasArray::~JSCanvasArray()
{
    forgetDOMObject(*Heap::heap(this)->globalData(), m_impl.get());
}

JSObject* JSCanvasArray::createPrototype(ExecState* exec, JSGlobalObject* globalObject)
{
    return new (exec) JSCanvasArrayPrototype(JSCanvasArrayPrototype::createStructure(globalObject->objectPrototype()));
}

bool JSCanvasArray::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSCanvasArray, Base>(exec, &JSCanvasArrayTable, this, propertyName, slot);
}

bool JSCanvasArray::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSCanvasArray, Base>(exec, &JSCanvasArrayTable, this, propertyName, descriptor);
}

JSValue jsCanvasArrayLength(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    JSCanvasArray* castedThis = static_cast<JSCanvasArray*>(asObject(slot.slotBase()));
    UNUSED_PARAM(exec);
    CanvasArray* imp = static_cast<CanvasArray*>(castedThis->impl());
    return jsNumber(exec, imp->length());
}

JSValue JSC_HOST_CALL jsCanvasArrayPrototypeFunctionSizeInBytes(ExecState* exec, JSObject*, JSValue thisValue, const ArgList& args)
{
    UNUSED_PARAM(args);
    if (!thisValue.inherits(&JSCanvasArray::s_info))
        return throwError(exec, TypeError);
    JSCanvasArray* castedThisObj = static_cast<JSCanvasArray*>(asObject(thisValue));
    CanvasArray* imp = static_cast<CanvasArray*>(castedThisObj->impl());


    JSC::JSValue result = jsNumber(exec, imp->sizeInBytes());
    return result;
}

JSValue JSC_HOST_CALL jsCanvasArrayPrototypeFunctionAlignedSizeInBytes(ExecState* exec, JSObject*, JSValue thisValue, const ArgList& args)
{
    UNUSED_PARAM(args);
    if (!thisValue.inherits(&JSCanvasArray::s_info))
        return throwError(exec, TypeError);
    JSCanvasArray* castedThisObj = static_cast<JSCanvasArray*>(asObject(thisValue));
    CanvasArray* imp = static_cast<CanvasArray*>(castedThisObj->impl());


    JSC::JSValue result = jsNumber(exec, imp->alignedSizeInBytes());
    return result;
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, CanvasArray* object)
{
    return getDOMObjectWrapper<JSCanvasArray>(exec, globalObject, object);
}
CanvasArray* toCanvasArray(JSC::JSValue value)
{
    return value.inherits(&JSCanvasArray::s_info) ? static_cast<JSCanvasArray*>(asObject(value))->impl() : 0;
}

}

#endif // ENABLE(3D_CANVAS)