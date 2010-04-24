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

#include "private/qdeclarativecompiler_p.h"

#include "private/qdeclarativecompositetypedata_p.h"
#include "private/qdeclarativeparser_p.h"
#include "private/qdeclarativescriptparser_p.h"
#include "qdeclarativepropertyvaluesource.h"
#include "qdeclarativecomponent.h"
#include "private/qmetaobjectbuilder_p.h"
#include "private/qdeclarativestringconverters_p.h"
#include "private/qdeclarativeengine_p.h"
#include "qdeclarativeengine.h"
#include "qdeclarativecontext.h"
#include "private/qdeclarativemetatype_p.h"
#include "private/qdeclarativecustomparser_p_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativecomponent_p.h"
#include "parser/qdeclarativejsast_p.h"
#include "private/qdeclarativevmemetaobject_p.h"
#include "private/qdeclarativeexpression_p.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativerewrite_p.h"
#include "qdeclarativescriptstring.h"
#include "private/qdeclarativeglobal_p.h"
#include "private/qdeclarativescriptparser_p.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativecompiledbindings_p.h"
#include "private/qdeclarativeglobalscriptclass_p.h"

#include <QColor>
#include <QDebug>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QAtomicInt>
#include <QtCore/qdebug.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(compilerDump, QML_COMPILER_DUMP);
DEFINE_BOOL_CONFIG_OPTION(compilerStatDump, QML_COMPILER_STATS);
DEFINE_BOOL_CONFIG_OPTION(bindingsDump, QML_BINDINGS_DUMP);

using namespace QDeclarativeParser;

/*!
    Instantiate a new QDeclarativeCompiler.
*/
QDeclarativeCompiler::QDeclarativeCompiler()
: output(0), engine(0), unitRoot(0), unit(0)
{
}

/*!
    Returns true if the last call to compile() caused errors.

    \sa errors()
*/
bool QDeclarativeCompiler::isError() const
{
    return !exceptions.isEmpty();
}

/*!
    Return the list of errors from the last call to compile(), or an empty list
    if there were no errors.
*/
QList<QDeclarativeError> QDeclarativeCompiler::errors() const
{
    return exceptions;
}

/*!
    Returns true if \a name refers to an attached property, false otherwise.

    Attached property names are those that start with a capital letter.
*/
bool QDeclarativeCompiler::isAttachedPropertyName(const QByteArray &name)
{
    return !name.isEmpty() && name.at(0) >= 'A' && name.at(0) <= 'Z';
}

/*!
    Returns true if \a name refers to a signal property, false otherwise.

    Signal property names are those that start with "on", followed by a capital
    letter.
*/
bool QDeclarativeCompiler::isSignalPropertyName(const QByteArray &name)
{
    return name.length() >= 3 && name.startsWith("on") &&
           'A' <= name.at(2) && 'Z' >= name.at(2);
}

/*!
    \macro COMPILE_EXCEPTION
    \internal
    Inserts an error into the QDeclarativeCompiler error list, and returns false
    (failure).

    \a token is used to source the error line and column, and \a desc is the
    error itself.  \a desc can be an expression that can be piped into QDebug.

    For example:

    \code
    COMPILE_EXCEPTION(property, tr("Error for property \"%1\"").arg(QString::fromUtf8(property->name)));
    \endcode
*/
#define COMPILE_EXCEPTION(token, desc) \
    {  \
        QString exceptionDescription; \
        QDeclarativeError error; \
        error.setUrl(output->url); \
        error.setLine((token)->location.start.line); \
        error.setColumn((token)->location.start.column); \
        error.setDescription(desc.trimmed()); \
        exceptions << error; \
        return false; \
    }

/*!
    \macro COMPILE_CHECK
    \internal
    Returns false if \a is false, otherwise does nothing.
*/
#define COMPILE_CHECK(a) \
    { \
        if (!a) return false; \
    }

/*!
    Returns true if literal \a v can be assigned to property \a prop, otherwise
    false.

    This test corresponds to action taken by genLiteralAssignment().  Any change
    made here, must have a corresponding action in genLiteralAssigment().
*/
bool QDeclarativeCompiler::testLiteralAssignment(const QMetaProperty &prop,
                                        QDeclarativeParser::Value *v)
{
    QString string = v->value.asScript();

    if (!prop.isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop.name())));

    if (prop.isEnumType()) {
        int value;
        if (prop.isFlagType()) {
            value = prop.enumerator().keysToValue(string.toUtf8().constData());
        } else
            value = prop.enumerator().keyToValue(string.toUtf8().constData());
        if (value == -1)
            COMPILE_EXCEPTION(v, tr("Invalid property assignment: unknown enumeration"));
        return true;
    }
    int type = prop.userType();
    switch(type) {
        case -1:
            break;
        case QVariant::String:
            if (!v->value.isString()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: string expected"));
            break;
        case QVariant::Url:
            if (!v->value.isString()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: url expected"));
            break;
        case QVariant::UInt:
            {
            bool ok;
            string.toUInt(&ok);
            if (!v->value.isNumber() || !ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: unsigned int expected"));
            }
            break;
        case QVariant::Int:
            {
            bool ok;
            string.toInt(&ok);
            if (!v->value.isNumber() || !ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: int expected"));
            }
            break;
        case QMetaType::Float:
            {
            bool ok;
            string.toFloat(&ok);
            if (!v->value.isNumber() || !ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: float expected"));
            }
            break;
        case QVariant::Double:
            {
            bool ok;
            string.toDouble(&ok);
            if (!v->value.isNumber() || !ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: double expected"));
            }
            break;
        case QVariant::Color:
            {
            bool ok;
            QDeclarativeStringConverters::colorFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: color expected"));
            }
            break;
        case QVariant::Date:
            {
            bool ok;
            QDeclarativeStringConverters::dateFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: date expected"));
            }
            break;
        case QVariant::Time:
            {
            bool ok;
            QDeclarativeStringConverters::timeFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: time expected"));
            }
            break;
        case QVariant::DateTime:
            {
            bool ok;
            QDeclarativeStringConverters::dateTimeFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: datetime expected"));
            }
            break;
        case QVariant::Point:
        case QVariant::PointF:
            {
            bool ok;
            QPointF point = QDeclarativeStringConverters::pointFFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: point expected"));
            }
            break;
        case QVariant::Size:
        case QVariant::SizeF:
            {
            bool ok;
            QSizeF size = QDeclarativeStringConverters::sizeFFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: size expected"));
            }
            break;
        case QVariant::Rect:
        case QVariant::RectF:
            {
            bool ok;
            QRectF rect = QDeclarativeStringConverters::rectFFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: rect expected"));
            }
            break;
        case QVariant::Bool:
            {
            if (!v->value.isBoolean()) COMPILE_EXCEPTION(v, tr("Invalid property assignment: boolean expected"));
            }
            break;
        case QVariant::Vector3D:
            {
            bool ok;
            QDeclarativeStringConverters::vector3DFromString(string, &ok);
            if (!ok) COMPILE_EXCEPTION(v, tr("Invalid property assignment: 3D vector expected"));
            }
            break;
        default:
            {
            int t = prop.userType();
            QDeclarativeMetaType::StringConverter converter =
                QDeclarativeMetaType::customStringConverter(t);
            if (!converter)
                COMPILE_EXCEPTION(v, tr("Invalid property assignment: unsupported type \"%1\"").arg(QString::fromLatin1(QVariant::typeToName(prop.type()))));
            }
            break;
    }
    return true;
}

/*!
    Generate a store instruction for assigning literal \a v to property \a prop.

    Any literal assignment that is approved in testLiteralAssignment() must have
    a corresponding action in this method.
*/
void QDeclarativeCompiler::genLiteralAssignment(const QMetaProperty &prop,
                                       QDeclarativeParser::Value *v)
{
    QString string = v->value.asScript();

    QDeclarativeInstruction instr;
    instr.line = v->location.start.line;
    if (prop.isEnumType()) {
        int value;
        if (prop.isFlagType()) {
            value = prop.enumerator().keysToValue(string.toUtf8().constData());
        } else
            value = prop.enumerator().keyToValue(string.toUtf8().constData());

        instr.type = QDeclarativeInstruction::StoreInteger;
        instr.storeInteger.propertyIndex = prop.propertyIndex();
        instr.storeInteger.value = value;
        output->bytecode << instr;
        return;
    }

    int type = prop.userType();
    switch(type) {
        case -1:
            {
            if (v->value.isNumber()) {
                double n = v->value.asNumber();
                if (double(int(n)) == n) {
                    instr.type = QDeclarativeInstruction::StoreVariantInteger;
                    instr.storeInteger.propertyIndex = prop.propertyIndex();
                    instr.storeInteger.value = int(n);
                } else {
                    instr.type = QDeclarativeInstruction::StoreVariantDouble;
                    instr.storeDouble.propertyIndex = prop.propertyIndex();
                    instr.storeDouble.value = n;
                }
            } else {
                instr.type = QDeclarativeInstruction::StoreVariant;
                instr.storeString.propertyIndex = prop.propertyIndex();
                instr.storeString.value = output->indexForString(string);
            }
            }
            break;
        case QVariant::String:
            {
            instr.type = QDeclarativeInstruction::StoreString;
            instr.storeString.propertyIndex = prop.propertyIndex();
            instr.storeString.value = output->indexForString(string);
            }
            break;
        case QVariant::Url:
            {
            instr.type = QDeclarativeInstruction::StoreUrl;
            QUrl u = string.isEmpty() ? QUrl() : output->url.resolved(QUrl(string));
            instr.storeUrl.propertyIndex = prop.propertyIndex();
            instr.storeUrl.value = output->indexForUrl(u);
            }
            break;
        case QVariant::UInt:
            {
            instr.type = QDeclarativeInstruction::StoreInteger;
            instr.storeInteger.propertyIndex = prop.propertyIndex();
            instr.storeInteger.value = string.toUInt();
            }
            break;
        case QVariant::Int:
            {
            instr.type = QDeclarativeInstruction::StoreInteger;
            instr.storeInteger.propertyIndex = prop.propertyIndex();
            instr.storeInteger.value = string.toInt();
            }
            break;
        case QMetaType::Float:
            {
            instr.type = QDeclarativeInstruction::StoreFloat;
            instr.storeFloat.propertyIndex = prop.propertyIndex();
            instr.storeFloat.value = string.toFloat();
            }
            break;
        case QVariant::Double:
            {
            instr.type = QDeclarativeInstruction::StoreDouble;
            instr.storeDouble.propertyIndex = prop.propertyIndex();
            instr.storeDouble.value = string.toDouble();
            }
            break;
        case QVariant::Color:
            {
            QColor c = QDeclarativeStringConverters::colorFromString(string);
            instr.type = QDeclarativeInstruction::StoreColor;
            instr.storeColor.propertyIndex = prop.propertyIndex();
            instr.storeColor.value = c.rgba();
            }
            break;
        case QVariant::Date:
            {
            QDate d = QDeclarativeStringConverters::dateFromString(string);
            instr.type = QDeclarativeInstruction::StoreDate;
            instr.storeDate.propertyIndex = prop.propertyIndex();
            instr.storeDate.value = d.toJulianDay();
            }
            break;
        case QVariant::Time:
            {
            QTime time = QDeclarativeStringConverters::timeFromString(string);
            int data[] = { time.hour(), time.minute(),
                           time.second(), time.msec() };
            int index = output->indexForInt(data, 4);
            instr.type = QDeclarativeInstruction::StoreTime;
            instr.storeTime.propertyIndex = prop.propertyIndex();
            instr.storeTime.valueIndex = index;
            }
            break;
        case QVariant::DateTime:
            {
            QDateTime dateTime = QDeclarativeStringConverters::dateTimeFromString(string);
            int data[] = { dateTime.date().toJulianDay(),
                           dateTime.time().hour(),
                           dateTime.time().minute(),
                           dateTime.time().second(),
                           dateTime.time().msec() };
            int index = output->indexForInt(data, 5);
            instr.type = QDeclarativeInstruction::StoreDateTime;
            instr.storeDateTime.propertyIndex = prop.propertyIndex();
            instr.storeDateTime.valueIndex = index;
            }
            break;
        case QVariant::Point:
        case QVariant::PointF:
            {
            bool ok;
            QPointF point =
                QDeclarativeStringConverters::pointFFromString(string, &ok);
            float data[] = { float(point.x()), float(point.y()) };
            int index = output->indexForFloat(data, 2);
            if (type == QVariant::PointF)
                instr.type = QDeclarativeInstruction::StorePointF;
            else
                instr.type = QDeclarativeInstruction::StorePoint;
            instr.storeRealPair.propertyIndex = prop.propertyIndex();
            instr.storeRealPair.valueIndex = index;
            }
            break;
        case QVariant::Size:
        case QVariant::SizeF:
            {
            bool ok;
            QSizeF size = QDeclarativeStringConverters::sizeFFromString(string, &ok);
            float data[] = { float(size.width()), float(size.height()) };
            int index = output->indexForFloat(data, 2);
            if (type == QVariant::SizeF)
                instr.type = QDeclarativeInstruction::StoreSizeF;
            else
                instr.type = QDeclarativeInstruction::StoreSize;
            instr.storeRealPair.propertyIndex = prop.propertyIndex();
            instr.storeRealPair.valueIndex = index;
            }
            break;
        case QVariant::Rect:
        case QVariant::RectF:
            {
            bool ok;
            QRectF rect = QDeclarativeStringConverters::rectFFromString(string, &ok);
            float data[] = { float(rect.x()), float(rect.y()),
                             float(rect.width()), float(rect.height()) };
            int index = output->indexForFloat(data, 4);
            if (type == QVariant::RectF)
                instr.type = QDeclarativeInstruction::StoreRectF;
            else
                instr.type = QDeclarativeInstruction::StoreRect;
            instr.storeRect.propertyIndex = prop.propertyIndex();
            instr.storeRect.valueIndex = index;
            }
            break;
        case QVariant::Bool:
            {
            bool b = v->value.asBoolean();
            instr.type = QDeclarativeInstruction::StoreBool;
            instr.storeBool.propertyIndex = prop.propertyIndex();
            instr.storeBool.value = b;
            }
            break;
        case QVariant::Vector3D:
            {
            bool ok;
            QVector3D vector =
                QDeclarativeStringConverters::vector3DFromString(string, &ok);
            float data[] = { float(vector.x()), float(vector.y()), float(vector.z()) };
            int index = output->indexForFloat(data, 3);
            instr.type = QDeclarativeInstruction::StoreVector3D;
            instr.storeRealPair.propertyIndex = prop.propertyIndex();
            instr.storeRealPair.valueIndex = index;
            }
            break;
        default:
            {
            int t = prop.userType();
            int index = output->customTypeData.count();
            instr.type = QDeclarativeInstruction::AssignCustomType;
            instr.assignCustomType.propertyIndex = prop.propertyIndex();
            instr.assignCustomType.valueIndex = index;

            QDeclarativeCompiledData::CustomTypeData data;
            data.index = output->indexForString(string);
            data.type = t;
            output->customTypeData << data;
            }
            break;
    }
    output->bytecode << instr;
}

/*!
    Resets data by clearing the lists that the QDeclarativeCompiler modifies.
*/
void QDeclarativeCompiler::reset(QDeclarativeCompiledData *data)
{
    data->types.clear();
    data->primitives.clear();
    data->floatData.clear();
    data->intData.clear();
    data->customTypeData.clear();
    data->datas.clear();
    data->bytecode.clear();
}

/*!
    Compile \a unit, and store the output in \a out.  \a engine is the QDeclarativeEngine
    with which the QDeclarativeCompiledData will be associated.

    Returns true on success, false on failure.  On failure, the compile errors
    are available from errors().

    If the environment variant QML_COMPILER_DUMP is set
    (eg. QML_COMPILER_DUMP=1) the compiled instructions will be dumped to stderr
    on a successful compiler.
*/
bool QDeclarativeCompiler::compile(QDeclarativeEngine *engine,
                                   QDeclarativeCompositeTypeData *unit,
                                   QDeclarativeCompiledData *out)
{
    exceptions.clear();

    Q_ASSERT(out);
    reset(out);

    output = out;

    // Compile types
    for (int ii = 0; ii < unit->types.count(); ++ii) {
        QDeclarativeCompositeTypeData::TypeReference &tref = unit->types[ii];
        QDeclarativeCompiledData::TypeReference ref;
        QDeclarativeScriptParser::TypeReference *parserRef = unit->data.referencedTypes().at(ii);
        if (tref.type) {
            ref.type = tref.type;
            if (!ref.type->isCreatable()) {
                QString err = ref.type->noCreationReason();
                if (err.isEmpty())
                    err = tr( "Element is not creatable.");
                COMPILE_EXCEPTION(parserRef->refObjects.first(), err);
            }
        } else if (tref.unit) {
            ref.component = tref.unit->toComponent(engine);

            if (ref.component->isError()) {
                QDeclarativeError error;
                error.setUrl(output->url);
                error.setDescription(QLatin1String("Unable to create type ") +
                                     parserRef->name);
                if (!parserRef->refObjects.isEmpty()) {
                    QDeclarativeParser::Object *parserObject = parserRef->refObjects.first();
                    error.setLine(parserObject->location.start.line);
                    error.setColumn(parserObject->location.start.column);
                }

                exceptions << error;
                exceptions << ref.component->errors();
                reset(out);
                return false;
            }
            ref.ref = tref.unit;
            ref.ref->addref();
        }
        ref.className = parserRef->name.toUtf8();
        out->types << ref;
    }

    Object *root = unit->data.tree();
    Q_ASSERT(root);

    this->engine = engine;
    this->unit = unit;
    this->unitRoot = root;
    compileTree(root);

    if (!isError()) {
        if (compilerDump())
            out->dumpInstructions();
        if (compilerStatDump())
            dumpStats();
    } else {
        reset(out);
    }

    compileState = ComponentCompileState();
    savedCompileStates.clear();
    output = 0;
    this->engine = 0;
    this->unit = 0;
    this->unitRoot = 0;

    return !isError();
}

void QDeclarativeCompiler::compileTree(Object *tree)
{
    compileState.root = tree;
    componentStat.lineNumber = tree->location.start.line;

    if (!buildObject(tree, BindingContext()) || !completeComponentBuild())
        return;

    QDeclarativeInstruction init;
    init.type = QDeclarativeInstruction::Init;
    init.line = 0;
    init.init.bindingsSize = compileState.bindings.count();
    init.init.parserStatusSize = compileState.parserStatusCount;
    init.init.contextCache = genContextCache();
    if (compileState.compiledBindingData.isEmpty())
        init.init.compiledBinding = -1;
    else
        init.init.compiledBinding = output->indexForByteArray(compileState.compiledBindingData);
    output->bytecode << init;

    // Build global import scripts
    QHash<QString, Object::ScriptBlock> importedScripts;
    QStringList importedScriptIndexes;

    for (int ii = 0; ii < unit->scripts.count(); ++ii) {
        QString scriptCode = QString::fromUtf8(unit->scripts.at(ii).resource->data);
        Object::ScriptBlock::Pragmas pragmas = QDeclarativeScriptParser::extractPragmas(scriptCode);

        if (!scriptCode.isEmpty()) {
            Object::ScriptBlock &scriptBlock = importedScripts[unit->scripts.at(ii).qualifier];

            scriptBlock.codes.append(scriptCode);
            scriptBlock.lineNumbers.append(1);
            scriptBlock.files.append(unit->scripts.at(ii).resource->url);
            scriptBlock.pragmas.append(pragmas);
        }
    }

    for (QHash<QString, Object::ScriptBlock>::Iterator iter = importedScripts.begin(); 
         iter != importedScripts.end(); ++iter) {

        importedScriptIndexes.append(iter.key());

        QDeclarativeInstruction import;
        import.type = QDeclarativeInstruction::StoreImportedScript;
        import.line = 0;
        import.storeScript.value = output->scripts.count();
        output->scripts << *iter;
        output->bytecode << import;
    }

    genObject(tree);

    QDeclarativeInstruction def;
    init.line = 0;
    def.type = QDeclarativeInstruction::SetDefault;
    output->bytecode << def;

    output->imports = unit->imports;

    output->importCache = new QDeclarativeTypeNameCache(engine);

    for (int ii = 0; ii < importedScriptIndexes.count(); ++ii) 
        output->importCache->add(importedScriptIndexes.at(ii), ii);

    output->imports.cache(output->importCache, engine);

    Q_ASSERT(tree->metatype);

    if (tree->metadata.isEmpty()) {
        output->root = tree->metatype;
    } else {
        static_cast<QMetaObject &>(output->rootData) = *tree->metaObject();
        output->root = &output->rootData;
    }
    if (!tree->metadata.isEmpty()) 
        QDeclarativeEnginePrivate::get(engine)->registerCompositeType(output);
}

static bool ValuePtrLessThan(const Value *t1, const Value *t2) 
{
    return t1->location.start.line < t2->location.start.line ||
           (t1->location.start.line == t2->location.start.line &&
            t1->location.start.column < t2->location.start.column);
}

bool QDeclarativeCompiler::buildObject(Object *obj, const BindingContext &ctxt)
{
    componentStat.objects++;

    Q_ASSERT (obj->type != -1);
    const QDeclarativeCompiledData::TypeReference &tr =
        output->types.at(obj->type);
    obj->metatype = tr.metaObject();

    if (tr.component)
        obj->url = tr.component->url();
    if (tr.type)
        obj->typeName = tr.type->qmlTypeName();
    obj->className = tr.className;

    // This object is a "Component" element
    if (tr.type && obj->metatype == &QDeclarativeComponent::staticMetaObject) {
        COMPILE_CHECK(buildComponent(obj, ctxt));
        return true;
    } 

    // Object instantiations reset the binding context
    BindingContext objCtxt(obj);

    // Create the synthesized meta object, ignoring aliases
    COMPILE_CHECK(checkDynamicMeta(obj)); 
    COMPILE_CHECK(mergeDynamicMetaProperties(obj));
    COMPILE_CHECK(buildDynamicMeta(obj, IgnoreAliases));

    // Find the native type and check for the QDeclarativeParserStatus interface
    QDeclarativeType *type = toQmlType(obj);
    Q_ASSERT(type);
    obj->parserStatusCast = type->parserStatusCast();
    if (obj->parserStatusCast != -1)
        compileState.parserStatusCount++;

    // Check if this is a custom parser type.  Custom parser types allow
    // assignments to non-existent properties.  These assignments are then
    // compiled by the type.
    bool isCustomParser = output->types.at(obj->type).type &&
                          output->types.at(obj->type).type->customParser() != 0;
    QList<QDeclarativeCustomParserProperty> customProps;

    // Fetch the list of deferred properties
    QStringList deferredList = deferredProperties(obj);

    // Must do id property first.  This is to ensure that the id given to any
    // id reference created matches the order in which the objects are
    // instantiated
    foreach(Property *prop, obj->properties) {
        if (prop->name == "id") {
            COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
            break;
        }
    }

    // Merge 
    Property *defaultProperty = 0;
    Property *skipProperty = 0;
    if (obj->defaultProperty) {
        const QMetaObject *metaObject = obj->metaObject();
        Q_ASSERT(metaObject);
        QMetaProperty p = QDeclarativeMetaType::defaultProperty(metaObject);
        if (p.name()) {
            Property *explicitProperty = obj->getProperty(p.name(), false);
            if (explicitProperty && !explicitProperty->value) {
                skipProperty = explicitProperty;

                defaultProperty = new Property;
                defaultProperty->parent = obj;
                defaultProperty->isDefault = true;
                defaultProperty->location = obj->defaultProperty->location;
                defaultProperty->listValueRange = obj->defaultProperty->listValueRange;
                defaultProperty->listCommaPositions = obj->defaultProperty->listCommaPositions;

                defaultProperty->values  = obj->defaultProperty->values;
                defaultProperty->values += explicitProperty->values;
                foreach(Value *value, defaultProperty->values)
                    value->addref();
                qSort(defaultProperty->values.begin(), defaultProperty->values.end(), ValuePtrLessThan);

            } else {
                defaultProperty = obj->defaultProperty;
                defaultProperty->addref();
            }
        } else {
            defaultProperty = obj->defaultProperty;
            defaultProperty->addref();
        }
    }

    // Build all explicit properties specified
    foreach(Property *prop, obj->properties) {

        if (prop == skipProperty)
            continue;
        if (prop->name == "id")
            continue;

        bool canDefer = false;
        if (isCustomParser) {
            if (doesPropertyExist(prop, obj)) {
                int ids = compileState.ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState.ids.count();
            } else {
                customProps << QDeclarativeCustomParserNodePrivate::fromProperty(prop);
            }
        } else {
            if (isSignalPropertyName(prop->name)) {
                COMPILE_CHECK(buildSignal(prop,obj,objCtxt));
            } else {
                int ids = compileState.ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState.ids.count();
            }
        }

        if (canDefer && !deferredList.isEmpty() &&
            deferredList.contains(QString::fromUtf8(prop->name)))
            prop->isDeferred = true;

    }

    // Build the default property
    if (defaultProperty)  {
        Property *prop = defaultProperty;

        bool canDefer = false;
        if (isCustomParser) {
            if (doesPropertyExist(prop, obj)) {
                int ids = compileState.ids.count();
                COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
                canDefer = ids == compileState.ids.count();
            } else {
                customProps << QDeclarativeCustomParserNodePrivate::fromProperty(prop);
            }
        } else {
            int ids = compileState.ids.count();
            COMPILE_CHECK(buildProperty(prop, obj, objCtxt));
            canDefer = ids == compileState.ids.count();
        }

        if (canDefer && !deferredList.isEmpty() &&
            deferredList.contains(QString::fromUtf8(prop->name)))
            prop->isDeferred = true;
    }

    if (defaultProperty) 
        defaultProperty->release();

    // Compile custom parser parts
    if (isCustomParser/* && !customProps.isEmpty()*/) {
        QDeclarativeCustomParser *cp = output->types.at(obj->type).type->customParser();
        cp->clearErrors();
        cp->compiler = this;
        cp->object = obj;
        obj->custom = cp->compile(customProps);
        cp->compiler = 0;
        cp->object = 0;
        foreach (QDeclarativeError err, cp->errors()) {
            err.setUrl(output->url);
            exceptions << err;
        }
    }

    return true;
}

void QDeclarativeCompiler::genObject(QDeclarativeParser::Object *obj)
{
    const QDeclarativeCompiledData::TypeReference &tr =
        output->types.at(obj->type);
    if (tr.type && obj->metatype == &QDeclarativeComponent::staticMetaObject) {
        genComponent(obj);
        return;
    }

    // Create the object
    if (obj->custom.isEmpty() && output->types.at(obj->type).type &&
        !output->types.at(obj->type).type->isExtendedType() && obj != compileState.root) {

        QDeclarativeInstruction create;
        create.type = QDeclarativeInstruction::CreateSimpleObject;
        create.line = obj->location.start.line;
        create.createSimple.create = output->types.at(obj->type).type->createFunction();
        create.createSimple.typeSize = output->types.at(obj->type).type->createSize();
        create.createSimple.column = obj->location.start.column;
        output->bytecode << create;

    } else {

        QDeclarativeInstruction create;
        create.type = QDeclarativeInstruction::CreateObject;
        create.line = obj->location.start.line;
        create.create.column = obj->location.start.column;
        create.create.data = -1;
        if (!obj->custom.isEmpty())
            create.create.data = output->indexForByteArray(obj->custom);
        create.create.type = obj->type;
        if (!output->types.at(create.create.type).type && 
            !obj->bindingBitmask.isEmpty()) {
            Q_ASSERT(obj->bindingBitmask.size() % 4 == 0);
            create.create.bindingBits = 
                output->indexForByteArray(obj->bindingBitmask);
        } else {
            create.create.bindingBits = -1;
        }
        output->bytecode << create;

    }

    // Setup the synthesized meta object if necessary
    if (!obj->metadata.isEmpty()) {
        QDeclarativeInstruction meta;
        meta.type = QDeclarativeInstruction::StoreMetaObject;
        meta.line = 0;
        meta.storeMeta.data = output->indexForByteArray(obj->metadata);
        meta.storeMeta.aliasData = output->indexForByteArray(obj->synthdata);
        meta.storeMeta.propertyCache = output->propertyCaches.count();
        // ### Surely the creation of this property cache could be more efficient
        QDeclarativePropertyCache *propertyCache = 0;
        if (tr.component && QDeclarativeComponentPrivate::get(tr.component)->cc->rootPropertyCache) {
            propertyCache = QDeclarativeComponentPrivate::get(tr.component)->cc->rootPropertyCache->copy();
        } else {
            propertyCache = QDeclarativePropertyCache::create(engine, obj->metaObject()->superClass());
        }
        propertyCache->append(engine, obj->metaObject(), QDeclarativePropertyCache::Data::NoFlags,
                              QDeclarativePropertyCache::Data::IsVMEFunction);
        if (obj == unitRoot) {
            propertyCache->addref();
            output->rootPropertyCache = propertyCache;
        }
        output->propertyCaches << propertyCache;
        output->bytecode << meta;
    }

    // Set the object id
    if (!obj->id.isEmpty()) {
        QDeclarativeInstruction id;
        id.type = QDeclarativeInstruction::SetId;
        id.line = 0;
        id.setId.value = output->indexForString(obj->id);
        id.setId.index = obj->idIndex;
        output->bytecode << id;
    }

    // Begin the class
    if (obj->parserStatusCast != -1) {
        QDeclarativeInstruction begin;
        begin.type = QDeclarativeInstruction::BeginObject;
        begin.begin.castValue = obj->parserStatusCast;
        begin.line = obj->location.start.line;
        output->bytecode << begin;
    }

    genObjectBody(obj);
}

void QDeclarativeCompiler::genObjectBody(QDeclarativeParser::Object *obj)
{
    typedef QPair<Property *, int> PropPair;
    foreach(const PropPair &prop, obj->scriptStringProperties) {
        QDeclarativeInstruction ss;
        ss.type = QDeclarativeInstruction::StoreScriptString;
        ss.storeScriptString.propertyIndex = prop.first->index;
        ss.storeScriptString.value = 
            output->indexForString(prop.first->values.at(0)->value.asScript());
        ss.storeScriptString.scope = prop.second;
        output->bytecode << ss;
    }

    bool seenDefer = false;
    foreach(Property *prop, obj->valueProperties) {
        if (prop->isDeferred) {
            seenDefer = true;
            continue;
        }
        genValueProperty(prop, obj);
    }
    if (seenDefer) {
        QDeclarativeInstruction defer;
        defer.type = QDeclarativeInstruction::Defer;
        defer.line = 0;
        defer.defer.deferCount = 0;
        int deferIdx = output->bytecode.count();
        output->bytecode << defer;

        QDeclarativeInstruction init;
        init.type = QDeclarativeInstruction::Init;
        init.init.bindingsSize = compileState.bindings.count(); // XXX - bigger than necessary
        init.init.parserStatusSize = compileState.parserStatusCount; // XXX - bigger than necessary
        init.init.contextCache = -1;
        init.init.compiledBinding = -1;
        output->bytecode << init;

        foreach(Property *prop, obj->valueProperties) {
            if (!prop->isDeferred)
                continue;
            genValueProperty(prop, obj);
        }

        output->bytecode[deferIdx].defer.deferCount =
            output->bytecode.count() - deferIdx - 1;
    }

    foreach(Property *prop, obj->signalProperties) {

        QDeclarativeParser::Value *v = prop->values.at(0);

        if (v->type == Value::SignalObject) {

            genObject(v->object);

            QDeclarativeInstruction assign;
            assign.type = QDeclarativeInstruction::AssignSignalObject;
            assign.line = v->location.start.line;
            assign.assignSignalObject.signal =
                output->indexForByteArray(prop->name);
            output->bytecode << assign;

        } else if (v->type == Value::SignalExpression) {

            BindingContext ctxt = compileState.signalExpressions.value(v);

            QDeclarativeInstruction store;
            store.type = QDeclarativeInstruction::StoreSignal;
            store.line = v->location.start.line;
            store.storeSignal.signalIndex = prop->index;
            store.storeSignal.value =
                output->indexForString(v->value.asScript().trimmed());
            store.storeSignal.context = ctxt.stack;
            output->bytecode << store;

        }

    }

    foreach(Property *prop, obj->attachedProperties) {
        QDeclarativeInstruction fetch;
        fetch.type = QDeclarativeInstruction::FetchAttached;
        fetch.line = prop->location.start.line;
        fetch.fetchAttached.id = prop->index;
        output->bytecode << fetch;

        genObjectBody(prop->value);

        QDeclarativeInstruction pop;
        pop.type = QDeclarativeInstruction::PopFetchedObject;
        pop.line = prop->location.start.line;
        output->bytecode << pop;
    }

    foreach(Property *prop, obj->groupedProperties) {
        QDeclarativeInstruction fetch;
        fetch.type = QDeclarativeInstruction::FetchObject;
        fetch.fetch.property = prop->index;
        fetch.line = prop->location.start.line;
        output->bytecode << fetch;

        genObjectBody(prop->value);

        QDeclarativeInstruction pop;
        pop.type = QDeclarativeInstruction::PopFetchedObject;
        pop.line = prop->location.start.line;
        output->bytecode << pop;
    }

    foreach(Property *prop, obj->valueTypeProperties) {
        QDeclarativeInstruction fetch;
        fetch.type = QDeclarativeInstruction::FetchValueType;
        fetch.fetchValue.property = prop->index;
        fetch.fetchValue.type = prop->type;
        fetch.line = prop->location.start.line;

        output->bytecode << fetch;

        foreach(Property *vprop, prop->value->valueProperties) {
            genPropertyAssignment(vprop, prop->value, prop);
        }

        QDeclarativeInstruction pop;
        pop.type = QDeclarativeInstruction::PopValueType;
        pop.fetchValue.property = prop->index;
        pop.fetchValue.type = prop->type;
        pop.line = prop->location.start.line;
        output->bytecode << pop;
    }
}

void QDeclarativeCompiler::genComponent(QDeclarativeParser::Object *obj)
{
    QDeclarativeParser::Object *root = obj->defaultProperty->values.at(0)->object;
    Q_ASSERT(root);

    QDeclarativeInstruction create;
    create.type = QDeclarativeInstruction::CreateComponent;
    create.line = root->location.start.line;
    create.createComponent.column = root->location.start.column;
    create.createComponent.endLine = root->location.end.line;
    output->bytecode << create;
    int count = output->bytecode.count();

    ComponentCompileState oldCompileState = compileState;
    compileState = componentState(root);

    QDeclarativeInstruction init;
    init.type = QDeclarativeInstruction::Init;
    init.init.bindingsSize = compileState.bindings.count();
    init.init.parserStatusSize = compileState.parserStatusCount;
    init.init.contextCache = genContextCache();
    if (compileState.compiledBindingData.isEmpty())
        init.init.compiledBinding = -1;
    else
        init.init.compiledBinding = output->indexForByteArray(compileState.compiledBindingData);
    init.line = obj->location.start.line;
    output->bytecode << init;

    genObject(root);

    QDeclarativeInstruction def;
    init.line = 0;
    def.type = QDeclarativeInstruction::SetDefault;
    output->bytecode << def;

    output->bytecode[count - 1].createComponent.count =
        output->bytecode.count() - count;

    compileState = oldCompileState;

    if (!obj->id.isEmpty()) {
        QDeclarativeInstruction id;
        id.type = QDeclarativeInstruction::SetId;
        id.line = 0;
        id.setId.value = output->indexForString(obj->id);
        id.setId.index = obj->idIndex;
        output->bytecode << id;
    }
}

bool QDeclarativeCompiler::buildComponent(QDeclarativeParser::Object *obj,
                                 const BindingContext &ctxt)
{
    // The special "Component" element can only have the id property and a
    // default property, that actually defines the component's tree

    // Find, check and set the "id" property (if any)
    Property *idProp = 0;
    if (obj->properties.count() > 1 ||
       (obj->properties.count() == 1 && obj->properties.begin().key() != "id"))
        COMPILE_EXCEPTION(*obj->properties.begin(), tr("Component elements may not contain properties other than id"));
       
    if (obj->properties.count())
        idProp = *obj->properties.begin();

    if (idProp) {
       if (idProp->value || idProp->values.count() > 1 || idProp->values.at(0)->object) 
           COMPILE_EXCEPTION(idProp, tr("Invalid component id specification"));
       COMPILE_CHECK(checkValidId(idProp->values.first(), idProp->values.first()->primitive()));

        QString idVal = idProp->values.first()->primitive();

        if (compileState.ids.contains(idVal))
            COMPILE_EXCEPTION(idProp, tr("id is not unique"));

        obj->id = idVal;
        addId(idVal, obj);
    }

    // Check the Component tree is well formed
    if (obj->defaultProperty &&
       (obj->defaultProperty->value || obj->defaultProperty->values.count() > 1 ||
        (obj->defaultProperty->values.count() == 1 && !obj->defaultProperty->values.first()->object)))
        COMPILE_EXCEPTION(obj, tr("Invalid component body specification"));

    if (!obj->dynamicProperties.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new properties."));
    if (!obj->dynamicSignals.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new signals."));
    if (!obj->dynamicSlots.isEmpty())
        COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new functions."));

    Object *root = 0;
    if (obj->defaultProperty && obj->defaultProperty->values.count())
        root = obj->defaultProperty->values.first()->object;

    if (!root)
        COMPILE_EXCEPTION(obj, tr("Cannot create empty component specification"));

    // Build the component tree
    COMPILE_CHECK(buildComponentFromRoot(root, ctxt));

    return true;
}

bool QDeclarativeCompiler::buildComponentFromRoot(QDeclarativeParser::Object *obj,
                                         const BindingContext &ctxt)
{
    ComponentCompileState oldComponentCompileState = compileState;
    ComponentStat oldComponentStat = componentStat;

    compileState = ComponentCompileState();
    compileState.root = obj;

    componentStat = ComponentStat();
    componentStat.lineNumber = obj->location.start.line;

    if (obj)
        COMPILE_CHECK(buildObject(obj, ctxt));

    COMPILE_CHECK(completeComponentBuild());

    compileState = oldComponentCompileState;
    componentStat = oldComponentStat;

    return true;
}


// Build a sub-object.  A sub-object is one that was not created directly by
// QML - such as a grouped property object, or an attached object.  Sub-object's
// can't have an id, involve a custom parser, have attached properties etc.
bool QDeclarativeCompiler::buildSubObject(Object *obj, const BindingContext &ctxt)
{
    Q_ASSERT(obj->metatype);
    Q_ASSERT(!obj->defaultProperty);
    Q_ASSERT(ctxt.isSubContext()); // sub-objects must always be in a binding
                                   // sub-context

    foreach(Property *prop, obj->properties) {
        if (isSignalPropertyName(prop->name)) {
            COMPILE_CHECK(buildSignal(prop, obj, ctxt));
        } else {
            COMPILE_CHECK(buildProperty(prop, obj, ctxt));
        }
    }

    return true;
}

int QDeclarativeCompiler::componentTypeRef()
{
    QDeclarativeType *t = QDeclarativeMetaType::qmlType("Qt/Component",4,7);
    for (int ii = output->types.count() - 1; ii >= 0; --ii) {
        if (output->types.at(ii).type == t)
            return ii;
    }
    QDeclarativeCompiledData::TypeReference ref;
    ref.className = "Component";
    ref.type = t;
    output->types << ref;
    return output->types.count() - 1;
}

bool QDeclarativeCompiler::buildSignal(QDeclarativeParser::Property *prop, QDeclarativeParser::Object *obj,
                                       const BindingContext &ctxt)
{
    Q_ASSERT(obj->metaObject());

    QByteArray name = prop->name;
    Q_ASSERT(name.startsWith("on"));
    name = name.mid(2);
    if(name[0] >= 'A' && name[0] <= 'Z')
        name[0] = name[0] - 'A' + 'a';

    int sigIdx = QDeclarativePropertyPrivate::findSignalByName(obj->metaObject(), name).methodIndex();

    if (sigIdx == -1) {

        // If the "on<Signal>" name doesn't resolve into a signal, try it as a
        // property.
        COMPILE_CHECK(buildProperty(prop, obj, ctxt));

    }  else {

        if (prop->value || prop->values.count() != 1)
            COMPILE_EXCEPTION(prop, tr("Incorrectly specified signal assignment"));

        prop->index = sigIdx;
        obj->addSignalProperty(prop);

        if (prop->values.at(0)->object) {
            COMPILE_CHECK(buildObject(prop->values.at(0)->object, ctxt));
            prop->values.at(0)->type = Value::SignalObject;
        } else {
            prop->values.at(0)->type = Value::SignalExpression;

            QString script = prop->values.at(0)->value.asScript().trimmed();
            if (script.isEmpty())
                COMPILE_EXCEPTION(prop, tr("Empty signal assignment"));

            compileState.signalExpressions.insert(prop->values.at(0), ctxt);
        }
    }

    return true;
}


/*!
    Returns true if (value) property \a prop exists on obj, false otherwise.
*/
bool QDeclarativeCompiler::doesPropertyExist(QDeclarativeParser::Property *prop,
                                    QDeclarativeParser::Object *obj)
{
    if(isAttachedPropertyName(prop->name) || prop->name == "id")
        return true;

    const QMetaObject *mo = obj->metaObject();
    if (mo) {
        if (prop->isDefault) {
            QMetaProperty p = QDeclarativeMetaType::defaultProperty(mo);
            return p.name() != 0;
        } else {
            int idx = mo->indexOfProperty(prop->name.constData());
            return idx != -1;
        }
    }

    return false;
}

bool QDeclarativeCompiler::buildProperty(QDeclarativeParser::Property *prop,
                                QDeclarativeParser::Object *obj,
                                const BindingContext &ctxt)
{
    if (prop->isEmpty())
        COMPILE_EXCEPTION(prop, tr("Empty property assignment"));

    const QMetaObject *metaObject = obj->metaObject();
    Q_ASSERT(metaObject);

    if (isAttachedPropertyName(prop->name)) {
        // Setup attached property data

        if (ctxt.isSubContext()) {
            // Attached properties cannot be used on sub-objects.  Sub-objects
            // always exist in a binding sub-context, which is what we test
            // for here.
            COMPILE_EXCEPTION(prop, tr("Attached properties cannot be used here"));
        }

        QDeclarativeType *type = 0;
        QDeclarativeEnginePrivate::ImportedNamespace *typeNamespace = 0;
        QDeclarativeEnginePrivate::get(engine)->resolveType(unit->imports, prop->name, 
                                                   &type, 0, 0, 0, &typeNamespace);

        if (typeNamespace) {
            // ### We might need to indicate that this property is a namespace 
            // for the DOM API
            COMPILE_CHECK(buildPropertyInNamespace(typeNamespace, prop, obj, 
                                                   ctxt));
            return true;
        } else if (!type || !type->attachedPropertiesType())  {
            COMPILE_EXCEPTION(prop, tr("Non-existent attached object"));
        }

        if (!prop->value)
            COMPILE_EXCEPTION(prop, tr("Invalid attached object assignment"));

        Q_ASSERT(type->attachedPropertiesFunction());
        prop->index = type->index();
        prop->value->metatype = type->attachedPropertiesType();
    } else {
        // Setup regular property data
        QMetaProperty p;

        if (prop->isDefault) {
            p = QDeclarativeMetaType::defaultProperty(metaObject);

            if (p.name()) {
                prop->index = p.propertyIndex();
                prop->name = p.name();
            }

        } else {
            prop->index = metaObject->indexOfProperty(prop->name.constData());

            if (prop->index != -1) {
                p = metaObject->property(prop->index);
                Q_ASSERT(p.name());
            }
        }

        // We can't error here as the "id" property does not require a
        // successful index resolution
        if (p.name()) {
            prop->type = p.userType();
        }
    }

    if (prop->index != -1) 
        prop->parent->setBindingBit(prop->index);

    if (!prop->isDefault && prop->name == "id" && !ctxt.isSubContext()) {

        // The magic "id" behavior doesn't apply when "id" is resolved as a
        // default property or to sub-objects (which are always in binding
        // sub-contexts)
        COMPILE_CHECK(buildIdProperty(prop, obj));
        if (prop->type == QVariant::String &&
            prop->values.at(0)->value.isString())
            COMPILE_CHECK(buildPropertyAssignment(prop, obj, ctxt));

    } else if (isAttachedPropertyName(prop->name)) {

        COMPILE_CHECK(buildAttachedProperty(prop, obj, ctxt));

    } else if (prop->index == -1) {

        if (prop->isDefault) {
            COMPILE_EXCEPTION(prop->values.first(), tr("Cannot assign to non-existent default property"));
        } else {
            COMPILE_EXCEPTION(prop, tr("Cannot assign to non-existent property \"%1\"").arg(QString::fromUtf8(prop->name)));
        }

    } else if (prop->value) {

        COMPILE_CHECK(buildGroupedProperty(prop, obj, ctxt));

    } else if (QDeclarativeEnginePrivate::get(engine)->isList(prop->type)) {

        COMPILE_CHECK(buildListProperty(prop, obj, ctxt));

    } else if (prop->type == qMetaTypeId<QDeclarativeScriptString>()) {

        COMPILE_CHECK(buildScriptStringProperty(prop, obj, ctxt));

    } else {

        COMPILE_CHECK(buildPropertyAssignment(prop, obj, ctxt));

    }

    return true;
}

bool 
QDeclarativeCompiler::buildPropertyInNamespace(QDeclarativeEnginePrivate::ImportedNamespace *ns,
                                      QDeclarativeParser::Property *nsProp, 
                                      QDeclarativeParser::Object *obj, 
                                      const BindingContext &ctxt)
{
    if (!nsProp->value)
        COMPILE_EXCEPTION(nsProp, tr("Invalid use of namespace"));

    foreach (Property *prop, nsProp->value->properties) {

        if (!isAttachedPropertyName(prop->name))
            COMPILE_EXCEPTION(prop, tr("Not an attached property name"));

        // Setup attached property data

        QDeclarativeType *type = 0;
        QDeclarativeEnginePrivate::get(engine)->resolveTypeInNamespace(ns, prop->name,
                                                              &type, 0, 0, 0);

        if (!type || !type->attachedPropertiesType()) 
            COMPILE_EXCEPTION(prop, tr("Non-existent attached object"));

        if (!prop->value)
            COMPILE_EXCEPTION(prop, tr("Invalid attached object assignment"));

        Q_ASSERT(type->attachedPropertiesFunction());
        prop->index = type->index();
        prop->value->metatype = type->attachedPropertiesType();

        COMPILE_CHECK(buildAttachedProperty(prop, obj, ctxt));
    }

    return true;
}

void QDeclarativeCompiler::genValueProperty(QDeclarativeParser::Property *prop,
                                   QDeclarativeParser::Object *obj)
{
    if (QDeclarativeEnginePrivate::get(engine)->isList(prop->type)) {
        genListProperty(prop, obj);
    } else {
        genPropertyAssignment(prop, obj);
    }
}

void QDeclarativeCompiler::genListProperty(QDeclarativeParser::Property *prop,
                                  QDeclarativeParser::Object *obj)
{
    int listType = QDeclarativeEnginePrivate::get(engine)->listType(prop->type);

    QDeclarativeInstruction fetch;
    fetch.type = QDeclarativeInstruction::FetchQList;
    fetch.line = prop->location.start.line;
    fetch.fetchQmlList.property = prop->index;
    bool listTypeIsInterface = QDeclarativeMetaType::isInterface(listType);
    fetch.fetchQmlList.type = listType;
    output->bytecode << fetch;

    for (int ii = 0; ii < prop->values.count(); ++ii) {
        Value *v = prop->values.at(ii);

        if (v->type == Value::CreatedObject) {

            genObject(v->object);
            if (listTypeIsInterface) {
                QDeclarativeInstruction assign;
                assign.type = QDeclarativeInstruction::AssignObjectList;
                assign.line = prop->location.start.line;
                output->bytecode << assign;
            } else {
                QDeclarativeInstruction store;
                store.type = QDeclarativeInstruction::StoreObjectQList;
                store.line = prop->location.start.line;
                output->bytecode << store;
            }

        } else if (v->type == Value::PropertyBinding) {

            genBindingAssignment(v, prop, obj);

        }

    }

    QDeclarativeInstruction pop;
    pop.type = QDeclarativeInstruction::PopQList;
    pop.line = prop->location.start.line;
    output->bytecode << pop;
}

void QDeclarativeCompiler::genPropertyAssignment(QDeclarativeParser::Property *prop,
                                        QDeclarativeParser::Object *obj,
                                        QDeclarativeParser::Property *valueTypeProperty)
{
    for (int ii = 0; ii < prop->values.count(); ++ii) {
        QDeclarativeParser::Value *v = prop->values.at(ii);

        Q_ASSERT(v->type == Value::CreatedObject ||
                 v->type == Value::PropertyBinding ||
                 v->type == Value::Literal);

        if (v->type == Value::CreatedObject) {

            genObject(v->object);

            if (QDeclarativeMetaType::isInterface(prop->type)) {

                QDeclarativeInstruction store;
                store.type = QDeclarativeInstruction::StoreInterface;
                store.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->bytecode << store;

            } else if (prop->type == -1) {

                QDeclarativeInstruction store;
                store.type = QDeclarativeInstruction::StoreVariantObject;
                store.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->bytecode << store;

            } else {

                QDeclarativeInstruction store;
                store.type = QDeclarativeInstruction::StoreObject;
                store.line = v->object->location.start.line;
                store.storeObject.propertyIndex = prop->index;
                output->bytecode << store;

            }
        } else if (v->type == Value::PropertyBinding) {

            genBindingAssignment(v, prop, obj, valueTypeProperty);

        } else if (v->type == Value::Literal) {

            QMetaProperty mp = obj->metaObject()->property(prop->index);
            genLiteralAssignment(mp, v);

        }

    }

    for (int ii = 0; ii < prop->onValues.count(); ++ii) {

        QDeclarativeParser::Value *v = prop->onValues.at(ii);

        Q_ASSERT(v->type == Value::ValueSource ||
                 v->type == Value::ValueInterceptor);

        if (v->type == Value::ValueSource) {
            genObject(v->object);

            QDeclarativeInstruction store;
            store.type = QDeclarativeInstruction::StoreValueSource;
            store.line = v->object->location.start.line;
            if (valueTypeProperty) {
                store.assignValueSource.property = genValueTypeData(prop, valueTypeProperty);
                store.assignValueSource.owner = 1;
            } else {
                store.assignValueSource.property = genPropertyData(prop);
                store.assignValueSource.owner = 0;
            }
            QDeclarativeType *valueType = toQmlType(v->object);
            store.assignValueSource.castValue = valueType->propertyValueSourceCast();
            output->bytecode << store;

        } else if (v->type == Value::ValueInterceptor) {
            genObject(v->object);

            QDeclarativeInstruction store;
            store.type = QDeclarativeInstruction::StoreValueInterceptor;
            store.line = v->object->location.start.line;
            if (valueTypeProperty) {
                store.assignValueInterceptor.property = genValueTypeData(prop, valueTypeProperty);
                store.assignValueInterceptor.owner = 1;
            } else {
                store.assignValueInterceptor.property = genPropertyData(prop);
                store.assignValueInterceptor.owner = 0;
            }
            QDeclarativeType *valueType = toQmlType(v->object);
            store.assignValueInterceptor.castValue = valueType->propertyValueInterceptorCast();
            output->bytecode << store;
        }

    }
}

bool QDeclarativeCompiler::buildIdProperty(QDeclarativeParser::Property *prop,
                                  QDeclarativeParser::Object *obj)
{
    if (prop->value ||
        prop->values.count() > 1 ||
        prop->values.at(0)->object)
        COMPILE_EXCEPTION(prop, tr("Invalid use of id property"));

    QDeclarativeParser::Value *idValue = prop->values.at(0);
    QString val = idValue->primitive();

    COMPILE_CHECK(checkValidId(idValue, val));

    if (compileState.ids.contains(val))
        COMPILE_EXCEPTION(prop, tr("id is not unique"));

    prop->values.at(0)->type = Value::Id;

    obj->id = val;
    addId(val, obj);

    return true;
}

void QDeclarativeCompiler::addId(const QString &id, QDeclarativeParser::Object *obj)
{
    Q_ASSERT(!compileState.ids.contains(id));
    Q_ASSERT(obj->id == id);
    obj->idIndex = compileState.ids.count();
    compileState.ids.insert(id, obj);
    compileState.idIndexes.insert(obj->idIndex, obj);
}

void QDeclarativeCompiler::addBindingReference(const BindingReference &ref)
{
    Q_ASSERT(ref.value && !compileState.bindings.contains(ref.value));
    compileState.bindings.insert(ref.value, ref);
}

void QDeclarativeCompiler::saveComponentState()
{
    Q_ASSERT(compileState.root);
    Q_ASSERT(!savedCompileStates.contains(compileState.root));

    savedCompileStates.insert(compileState.root, compileState);
    savedComponentStats.append(componentStat);
}

QDeclarativeCompiler::ComponentCompileState
QDeclarativeCompiler::componentState(QDeclarativeParser::Object *obj)
{
    Q_ASSERT(savedCompileStates.contains(obj));
    return savedCompileStates.value(obj);
}

// Build attached property object.  In this example,
// Text {
//    GridView.row: 10
// }
// GridView is an attached property object.
bool QDeclarativeCompiler::buildAttachedProperty(QDeclarativeParser::Property *prop,
                                        QDeclarativeParser::Object *obj,
                                        const BindingContext &ctxt)
{
    Q_ASSERT(prop->value);
    Q_ASSERT(prop->index != -1); // This is set in buildProperty()

    obj->addAttachedProperty(prop);

    COMPILE_CHECK(buildSubObject(prop->value, ctxt.incr()));

    return true;
}


// Build "grouped" properties. In this example:
// Text {
//     font.pointSize: 12
//     font.family: "Helvetica"
// }
// font is a nested property.  pointSize and family are not.
bool QDeclarativeCompiler::buildGroupedProperty(QDeclarativeParser::Property *prop,
                                                QDeclarativeParser::Object *obj,
                                                const BindingContext &ctxt)
{
    Q_ASSERT(prop->type != 0);
    Q_ASSERT(prop->index != -1);

    if (QDeclarativeValueTypeFactory::isValueType(prop->type)) {
        QDeclarativeEnginePrivate *ep =
            static_cast<QDeclarativeEnginePrivate *>(QObjectPrivate::get(engine));
        if (prop->type >= 0 /* QVariant == -1 */ && ep->valueTypes[prop->type]) {

            if (prop->values.count()) {
                if (prop->values.at(0)->location < prop->value->location) {
                    COMPILE_EXCEPTION(prop->value, tr( "Property has already been assigned a value"));
                } else {
                    COMPILE_EXCEPTION(prop->values.at(0), tr( "Property has already been assigned a value"));
                }
            }

            if (!obj->metaObject()->property(prop->index).isWritable()) {
                COMPILE_EXCEPTION(prop, tr( "Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop->name)));
            }

            COMPILE_CHECK(buildValueTypeProperty(ep->valueTypes[prop->type],
                                                 prop->value, obj, ctxt.incr()));
            obj->addValueTypeProperty(prop);
        } else {
            COMPILE_EXCEPTION(prop, tr("Invalid grouped property access"));
        }

    } else {
        // Load the nested property's meta type
        prop->value->metatype = 
            QDeclarativeEnginePrivate::get(engine)->metaObjectForType(prop->type);
        if (!prop->value->metatype)
            COMPILE_EXCEPTION(prop, tr("Invalid grouped property access"));

        if (prop->values.count()) 
            COMPILE_EXCEPTION(prop->values.at(0), tr( "Cannot assign a value directly to a grouped property"));

        obj->addGroupedProperty(prop);

        COMPILE_CHECK(buildSubObject(prop->value, ctxt.incr()));
    }

    return true;
}

bool QDeclarativeCompiler::buildValueTypeProperty(QObject *type,
                                                  QDeclarativeParser::Object *obj,
                                                  QDeclarativeParser::Object *baseObj,
                                                  const BindingContext &ctxt)
{
    if (obj->defaultProperty)
        COMPILE_EXCEPTION(obj, tr("Invalid property use"));
    obj->metatype = type->metaObject();

    foreach (Property *prop, obj->properties) {
        int idx = type->metaObject()->indexOfProperty(prop->name.constData());
        if (idx == -1)
            COMPILE_EXCEPTION(prop, tr("Cannot assign to non-existent property \"%1\"").arg(QString::fromUtf8(prop->name)));
        QMetaProperty p = type->metaObject()->property(idx);
        prop->index = idx;
        prop->type = p.userType();
        prop->isValueTypeSubProperty = true;

        if (prop->value)
            COMPILE_EXCEPTION(prop, tr("Property assignment expected"));

        if (prop->values.count() > 1) {
            COMPILE_EXCEPTION(prop, tr("Single property assignment expected"));
        } else if (prop->values.count()) {
            Value *value = prop->values.at(0);

            if (value->object) {
                COMPILE_EXCEPTION(prop, tr("Unexpected object assignment"));
            } else if (value->value.isScript()) {
                // ### Check for writability
                BindingReference reference;
                reference.expression = value->value;
                reference.property = prop;
                reference.value = value;
                reference.bindingContext = ctxt;
                reference.bindingContext.owner++;
                addBindingReference(reference);
                value->type = Value::PropertyBinding;
            } else  {
                COMPILE_CHECK(testLiteralAssignment(p, value));
                value->type = Value::Literal;
            }
        }

        for (int ii = 0; ii < prop->onValues.count(); ++ii) {
            Value *v = prop->onValues.at(ii);
            Q_ASSERT(v->object);

            COMPILE_CHECK(buildPropertyOnAssignment(prop, obj, baseObj, v, ctxt)); 
        }

        obj->addValueProperty(prop);
    }

    return true;
}

// Build assignments to QML lists.  QML lists are properties of type
// QDeclarativeListProperty<T>.  List properties can accept a list of 
// objects, or a single binding.
bool QDeclarativeCompiler::buildListProperty(QDeclarativeParser::Property *prop,
                                             QDeclarativeParser::Object *obj,
                                             const BindingContext &ctxt)
{
    Q_ASSERT(QDeclarativeEnginePrivate::get(engine)->isList(prop->type));

    int t = prop->type;

    obj->addValueProperty(prop);

    int listType = QDeclarativeEnginePrivate::get(engine)->listType(t);
    bool listTypeIsInterface = QDeclarativeMetaType::isInterface(listType);

    bool assignedBinding = false;
    for (int ii = 0; ii < prop->values.count(); ++ii) {
        Value *v = prop->values.at(ii);
        if (v->object) {
            v->type = Value::CreatedObject;
            COMPILE_CHECK(buildObject(v->object, ctxt));

            // We check object coercian here.  We check interface assignment
            // at runtime.
            if (!listTypeIsInterface) {
                if (!canCoerce(listType, v->object)) {
                    COMPILE_EXCEPTION(v, tr("Cannot assign object to list"));
                }
            }

        } else if (v->value.isScript()) {
            if (assignedBinding)
                COMPILE_EXCEPTION(v, tr("Can only assign one binding to lists"));

            assignedBinding = true;
            COMPILE_CHECK(buildBinding(v, prop, ctxt));
            v->type = Value::PropertyBinding;
        } else {
            COMPILE_EXCEPTION(v, tr("Cannot assign primitives to lists"));
        }
    }

    return true;
}

// Compiles an assignment to a QDeclarativeScriptString property
bool QDeclarativeCompiler::buildScriptStringProperty(QDeclarativeParser::Property *prop,
                                            QDeclarativeParser::Object *obj,
                                            const BindingContext &ctxt)
{
    if (prop->values.count() > 1) 
        COMPILE_EXCEPTION(prop->values.at(1), tr( "Cannot assign multiple values to a script property"));

    if (prop->values.at(0)->object || !prop->values.at(0)->value.isScript())
        COMPILE_EXCEPTION(prop->values.at(0), tr( "Invalid property assignment: script expected"));

    obj->addScriptStringProperty(prop, ctxt.stack);

    return true;
}

// Compile regular property assignments of the form "property: <value>"
bool QDeclarativeCompiler::buildPropertyAssignment(QDeclarativeParser::Property *prop,
                                          QDeclarativeParser::Object *obj,
                                          const BindingContext &ctxt)
{
    obj->addValueProperty(prop);

    for (int ii = 0; ii < prop->values.count(); ++ii) {
        Value *v = prop->values.at(ii);
        if (v->object) {

            COMPILE_CHECK(buildPropertyObjectAssignment(prop, obj, v, ctxt));

        } else {

            COMPILE_CHECK(buildPropertyLiteralAssignment(prop, obj, v, ctxt));

        }
    }

    for (int ii = 0; ii < prop->onValues.count(); ++ii) {
        Value *v = prop->onValues.at(ii);

        Q_ASSERT(v->object);
        COMPILE_CHECK(buildPropertyOnAssignment(prop, obj, obj, v, ctxt));
    }

    return true;
}

// Compile assigning a single object instance to a regular property
bool QDeclarativeCompiler::buildPropertyObjectAssignment(QDeclarativeParser::Property *prop,
                                                         QDeclarativeParser::Object *obj,
                                                         QDeclarativeParser::Value *v,
                                                         const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(v->object->type != -1);

    if (!obj->metaObject()->property(prop->index).isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop->name)));

    if (QDeclarativeMetaType::isInterface(prop->type)) {

        // Assigning an object to an interface ptr property
        COMPILE_CHECK(buildObject(v->object, ctxt));

        v->type = Value::CreatedObject;

    } else if (prop->type == -1) {

        // Assigning an object to a QVariant
        COMPILE_CHECK(buildObject(v->object, ctxt));

        v->type = Value::CreatedObject;
    } else {
        // Normally buildObject() will set this up, but we need the static
        // meta object earlier to test for assignability.  It doesn't matter
        // that there may still be outstanding synthesized meta object changes
        // on this type, as they are not relevant for assignability testing
        v->object->metatype = output->types.at(v->object->type).metaObject();
        Q_ASSERT(v->object->metaObject());

        // We want to raw metaObject here as the raw metaobject is the
        // actual property type before we applied any extensions that might
        // effect the properties on the type, but don't effect assignability
        const QMetaObject *propertyMetaObject =
            QDeclarativeEnginePrivate::get(engine)->rawMetaObjectForType(prop->type);

        // Will be true if the assgned type inherits propertyMetaObject
        bool isAssignable = false;
        // Determine isAssignable value
        if (propertyMetaObject) {
            const QMetaObject *c = v->object->metatype;
            while(c) {
                isAssignable |= (QDeclarativePropertyPrivate::equal(c, propertyMetaObject));
                c = c->superClass();
            }
        }

        if (isAssignable) {
            // Simple assignment
            COMPILE_CHECK(buildObject(v->object, ctxt));

            v->type = Value::CreatedObject;
        } else if (propertyMetaObject == &QDeclarativeComponent::staticMetaObject) {
            // Automatic "Component" insertion
            QDeclarativeParser::Object *root = v->object;
            QDeclarativeParser::Object *component = new QDeclarativeParser::Object;
            component->type = componentTypeRef();
            component->typeName = "Qt/Component";
            component->metatype = &QDeclarativeComponent::staticMetaObject;
            component->location = root->location;
            QDeclarativeParser::Value *componentValue = new QDeclarativeParser::Value;
            componentValue->object = root;
            component->getDefaultProperty()->addValue(componentValue);
            v->object = component;
            COMPILE_CHECK(buildPropertyObjectAssignment(prop, obj, v, ctxt));
        } else {
            COMPILE_EXCEPTION(v->object, tr("Cannot assign object to property"));
        }
    }

    return true;
}

// Compile assigning a single object instance to a regular property using the "on" syntax.
//
// For example:
//     Item {
//         NumberAnimation on x { }
//     }
bool QDeclarativeCompiler::buildPropertyOnAssignment(QDeclarativeParser::Property *prop,
                                                     QDeclarativeParser::Object *obj,
                                                     QDeclarativeParser::Object *baseObj,
                                                     QDeclarativeParser::Value *v,
                                                     const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(v->object->type != -1);

    if (!obj->metaObject()->property(prop->index).isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop->name)));


    // Normally buildObject() will set this up, but we need the static
    // meta object earlier to test for assignability.  It doesn't matter
    // that there may still be outstanding synthesized meta object changes
    // on this type, as they are not relevant for assignability testing
    v->object->metatype = output->types.at(v->object->type).metaObject();
    Q_ASSERT(v->object->metaObject());

    // Will be true if the assigned type inherits QDeclarativePropertyValueSource
    bool isPropertyValue = false;
    // Will be true if the assigned type inherits QDeclarativePropertyValueInterceptor
    bool isPropertyInterceptor = false;
    if (QDeclarativeType *valueType = toQmlType(v->object)) {
        isPropertyValue = valueType->propertyValueSourceCast() != -1;
        isPropertyInterceptor = valueType->propertyValueInterceptorCast() != -1;
    }

    if (isPropertyValue || isPropertyInterceptor) {
        // Assign as a property value source
        COMPILE_CHECK(buildObject(v->object, ctxt));

        if (isPropertyInterceptor && prop->parent->synthdata.isEmpty())
            buildDynamicMeta(baseObj, ForceCreation);
        v->type = isPropertyValue ? Value::ValueSource : Value::ValueInterceptor;
    } else {
        COMPILE_EXCEPTION(v, tr("\"%1\" cannot operate on \"%2\"").arg(QString::fromUtf8(v->object->typeName)).arg(QString::fromUtf8(prop->name.constData())));
    }

    return true;
}

// Compile assigning a literal or binding to a regular property
bool QDeclarativeCompiler::buildPropertyLiteralAssignment(QDeclarativeParser::Property *prop,
                                                 QDeclarativeParser::Object *obj,
                                                 QDeclarativeParser::Value *v,
                                                 const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);

    if (v->value.isScript()) {

        //optimization for <Type>.<EnumValue> enum assignments
        bool isEnumAssignment = false;
        COMPILE_CHECK(testQualifiedEnumAssignment(obj->metaObject()->property(prop->index), obj, v, &isEnumAssignment));
        if (isEnumAssignment) {
            v->type = Value::Literal;
            return true;
        }

        COMPILE_CHECK(buildBinding(v, prop, ctxt));

        v->type = Value::PropertyBinding;

    } else {

        COMPILE_CHECK(testLiteralAssignment(obj->metaObject()->property(prop->index), v));

        v->type = Value::Literal;
    }

    return true;
}

bool QDeclarativeCompiler::testQualifiedEnumAssignment(const QMetaProperty &prop,
                                              QDeclarativeParser::Object *obj,
                                              QDeclarativeParser::Value *v,
                                              bool *isAssignment)
{
    *isAssignment = false;
    if (!prop.isEnumType())
        return true;

    if (!prop.isWritable())
        COMPILE_EXCEPTION(v, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop.name())));

    QString string = v->value.asString();
    if (!string.at(0).isUpper())
        return true;

    QStringList parts = string.split(QLatin1Char('.'));
    if (parts.count() != 2)
        return true;

    QString typeName = parts.at(0);
    QDeclarativeType *type = 0;
    QDeclarativeEnginePrivate::get(engine)->resolveType(unit->imports, typeName.toUtf8(),
                                               &type, 0, 0, 0, 0);

    if (!type || obj->typeName != type->qmlTypeName())
        return true;

    QString enumValue = parts.at(1);
    int value;
    if (prop.isFlagType()) {
        value = prop.enumerator().keysToValue(enumValue.toUtf8().constData());
    } else
        value = prop.enumerator().keyToValue(enumValue.toUtf8().constData());
    if (value == -1)
        return true;

    v->type = Value::Literal;
    v->value = QDeclarativeParser::Variant(enumValue);
    *isAssignment = true;

    return true;
}

// Similar logic to above, but not knowing target property.
int QDeclarativeCompiler::evaluateEnum(const QByteArray& script) const
{
    int dot = script.indexOf('.');
    if (dot > 0) {
        QDeclarativeType *type = 0;
        QDeclarativeEnginePrivate::get(engine)->resolveType(unit->imports, script.left(dot), &type, 0, 0, 0, 0);
        if (!type)
            return -1;
        const QMetaObject *mo = type->metaObject();
        const char *key = script.constData() + dot+1;
        int i = mo->enumeratorCount();
        while (i--) {
            int v = mo->enumerator(i).keyToValue(key);
            if (v >= 0)
                return v;
        }
    }
    return -1;
}

// Ensures that the dynamic meta specification on obj is valid
bool QDeclarativeCompiler::checkDynamicMeta(QDeclarativeParser::Object *obj)
{
    QSet<QByteArray> propNames;
    QSet<QByteArray> methodNames;
    bool seenDefaultProperty = false;

    // Check properties
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const QDeclarativeParser::Object::DynamicProperty &prop =
            obj->dynamicProperties.at(ii);

        if (prop.isDefaultProperty) {
            if (seenDefaultProperty)
                COMPILE_EXCEPTION(&prop, tr("Duplicate default property"));
            seenDefaultProperty = true;
        }

        if (propNames.contains(prop.name))
            COMPILE_EXCEPTION(&prop, tr("Duplicate property name"));

        if (QString::fromUtf8(prop.name).at(0).isUpper()) 
            COMPILE_EXCEPTION(&prop, tr("Property names cannot begin with an upper case letter"));
        propNames.insert(prop.name);
    }

    for (int ii = 0; ii < obj->dynamicSignals.count(); ++ii) {
        QByteArray name = obj->dynamicSignals.at(ii).name;
        if (methodNames.contains(name))
            COMPILE_EXCEPTION(obj, tr("Duplicate signal name"));
        if (QString::fromUtf8(name).at(0).isUpper()) 
            COMPILE_EXCEPTION(obj, tr("Signal names cannot begin with an upper case letter"));
        methodNames.insert(name);
    }
    for (int ii = 0; ii < obj->dynamicSlots.count(); ++ii) {
        QByteArray name = obj->dynamicSlots.at(ii).name;
        if (methodNames.contains(name))
            COMPILE_EXCEPTION(obj, tr("Duplicate method name"));
        if (QString::fromUtf8(name).at(0).isUpper()) 
            COMPILE_EXCEPTION(obj, tr("Method names cannot begin with an upper case letter"));
        methodNames.insert(name);
    }

    return true;
}

bool QDeclarativeCompiler::mergeDynamicMetaProperties(QDeclarativeParser::Object *obj)
{
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const Object::DynamicProperty &p = obj->dynamicProperties.at(ii);

        if (!p.defaultValue || p.type == Object::DynamicProperty::Alias)
            continue;

        Property *property = 0;
        if (p.isDefaultProperty) {
            property = obj->getDefaultProperty();
        } else {
            property = obj->getProperty(p.name);
            if (!property->values.isEmpty()) 
                COMPILE_EXCEPTION(property, tr("Property value set multiple times"));
        }

        if (property->value)
            COMPILE_EXCEPTION(property, tr("Invalid property nesting"));

        for (int ii = 0; ii < p.defaultValue->values.count(); ++ii) {
            Value *v = p.defaultValue->values.at(ii);
            v->addref();
            property->values.append(v);
        }
    }
    return true;
}

Q_GLOBAL_STATIC(QAtomicInt, classIndexCounter)

bool QDeclarativeCompiler::buildDynamicMeta(QDeclarativeParser::Object *obj, DynamicMetaMode mode)
{
    Q_ASSERT(obj);
    Q_ASSERT(obj->metatype);

    if (mode != ForceCreation &&
        obj->dynamicProperties.isEmpty() &&
        obj->dynamicSignals.isEmpty() &&
        obj->dynamicSlots.isEmpty())
        return true;

    QByteArray dynamicData(sizeof(QDeclarativeVMEMetaData), (char)0);

    QByteArray newClassName = obj->metatype->className();
    newClassName.append("_QML_");
    int idx = classIndexCounter()->fetchAndAddRelaxed(1);
    newClassName.append(QByteArray::number(idx));
    if (compileState.root == obj) {
        QString path = output->url.path();
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        if (lastSlash > -1) {
            QString nameBase = path.mid(lastSlash + 1, path.length()-lastSlash-5);
            if (!nameBase.isEmpty() && nameBase.at(0).isUpper())
                newClassName = nameBase.toUtf8() + "_QMLTYPE_" + QByteArray::number(idx);
        }
    }

    QMetaObjectBuilder builder;
    builder.setClassName(newClassName);
    builder.setFlags(QMetaObjectBuilder::DynamicMetaObject);

    bool hasAlias = false;
    for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
        const Object::DynamicProperty &p = obj->dynamicProperties.at(ii);

        int propIdx =
            obj->metaObject()->indexOfProperty(p.name.constData());
        if (-1 != propIdx) {
            QMetaProperty prop = obj->metaObject()->property(propIdx);
            if (prop.isFinal())
                COMPILE_EXCEPTION(&p, tr("Cannot override FINAL property"));
        }

        if (p.isDefaultProperty &&
            (p.type != Object::DynamicProperty::Alias ||
             mode == ResolveAliases))
            builder.addClassInfo("DefaultProperty", p.name);

        QByteArray type;
        int propertyType = 0;
        bool readonly = false;
        switch(p.type) {
        case Object::DynamicProperty::Alias:
            hasAlias = true;
            continue;
            break;
        case Object::DynamicProperty::CustomList:
        case Object::DynamicProperty::Custom:
            {
                QByteArray customTypeName;
                QDeclarativeType *qmltype = 0;
                QUrl url;
                QDeclarativeEnginePrivate *priv = QDeclarativeEnginePrivate::get(engine);
                if (!priv->resolveType(unit->imports, p.customType, &qmltype, 
                                       &url, 0, 0, 0)) 
                    COMPILE_EXCEPTION(&p, tr("Invalid property type"));

                if (!qmltype) {
                    QDeclarativeCompositeTypeData *tdata = priv->typeManager.get(url);
                    Q_ASSERT(tdata);
                    Q_ASSERT(tdata->status == QDeclarativeCompositeTypeData::Complete);

                    QDeclarativeCompiledData *data = tdata->toCompiledComponent(engine);
                    customTypeName = data->root->className();
                } else {
                    customTypeName = qmltype->typeName();
                }

                if (p.type == Object::DynamicProperty::Custom) {
                    type = customTypeName + '*';
                    propertyType = QMetaType::QObjectStar;
                } else {
                    readonly = true;
                    type = "QDeclarativeListProperty<";
                    type.append(customTypeName);
                    type.append(">");
                    propertyType = qMetaTypeId<QDeclarativeListProperty<QObject> >();
                }
            }
            break;
        case Object::DynamicProperty::Variant:
            propertyType = -1;
            type = "QVariant";
            break;
        case Object::DynamicProperty::Int:
            propertyType = QVariant::Int;
            type = "int";
            break;
        case Object::DynamicProperty::Bool:
            propertyType = QVariant::Bool;
            type = "bool";
            break;
        case Object::DynamicProperty::Real:
            propertyType = QVariant::Double;
            type = "double";
            break;
        case Object::DynamicProperty::String:
            propertyType = QVariant::String;
            type = "QString";
            break;
        case Object::DynamicProperty::Url:
            propertyType = QVariant::Url;
            type = "QUrl";
            break;
        case Object::DynamicProperty::Color:
            propertyType = QVariant::Color;
            type = "QColor";
            break;
        case Object::DynamicProperty::Time:
            propertyType = QVariant::Time;
            type = "QTime";
            break;
        case Object::DynamicProperty::Date:
            propertyType = QVariant::Date;
            type = "QDate";
            break;
        case Object::DynamicProperty::DateTime:
            propertyType = QVariant::DateTime;
            type = "QDateTime";
            break;
        }

        ((QDeclarativeVMEMetaData *)dynamicData.data())->propertyCount++;
        QDeclarativeVMEMetaData::PropertyData propertyData = { propertyType };
        dynamicData.append((char *)&propertyData, sizeof(propertyData));

        builder.addSignal(p.name + "Changed()");
        QMetaPropertyBuilder propBuilder = 
            builder.addProperty(p.name, type, builder.methodCount() - 1);
        propBuilder.setScriptable(true);
        propBuilder.setWritable(!readonly);
    }

    if (mode == ResolveAliases) {
        for (int ii = 0; ii < obj->dynamicProperties.count(); ++ii) {
            const Object::DynamicProperty &p = obj->dynamicProperties.at(ii);

            if (p.type == Object::DynamicProperty::Alias) {
                ((QDeclarativeVMEMetaData *)dynamicData.data())->aliasCount++;
                compileAlias(builder, dynamicData, obj, p);
            }
        }
    }

    for (int ii = 0; ii < obj->dynamicSignals.count(); ++ii) {
        const Object::DynamicSignal &s = obj->dynamicSignals.at(ii);
        QByteArray sig(s.name + '(');
        for (int jj = 0; jj < s.parameterTypes.count(); ++jj) {
            if (jj) sig.append(',');
            sig.append(s.parameterTypes.at(jj));
        }
        sig.append(')');
        QMetaMethodBuilder b = builder.addSignal(sig);
        b.setParameterNames(s.parameterNames);
        ((QDeclarativeVMEMetaData *)dynamicData.data())->signalCount++;
    }

    QStringList funcScripts;

    for (int ii = 0; ii < obj->dynamicSlots.count(); ++ii) {
        Object::DynamicSlot &s = obj->dynamicSlots[ii];
        QByteArray sig(s.name + '(');
        QString funcScript(QLatin1String("(function("));

        for (int jj = 0; jj < s.parameterNames.count(); ++jj) {
            if (jj) { 
                sig.append(',');
                funcScript.append(QLatin1Char(','));
            }
            funcScript.append(QLatin1String(s.parameterNames.at(jj)));
            sig.append("QVariant");
        }
        sig.append(')');
        funcScript.append(QLatin1Char(')'));
        funcScript.append(s.body);
        funcScript.append(QLatin1Char(')'));
        funcScripts << funcScript;

        QMetaMethodBuilder b = builder.addSlot(sig);
        b.setReturnType("QVariant");
        b.setParameterNames(s.parameterNames);

        ((QDeclarativeVMEMetaData *)dynamicData.data())->methodCount++;
        QDeclarativeVMEMetaData::MethodData methodData =
             { s.parameterNames.count(), 0, funcScript.length(), s.location.start.line };

        dynamicData.append((char *)&methodData, sizeof(methodData));
    }

    for (int ii = 0; ii < obj->dynamicSlots.count(); ++ii) {
        const QString &funcScript = funcScripts.at(ii);
        QDeclarativeVMEMetaData::MethodData *data =
            ((QDeclarativeVMEMetaData *)dynamicData.data())->methodData() + ii;

        data->bodyOffset = dynamicData.size();

        dynamicData.append((const char *)funcScript.constData(),
                           (funcScript.length() * sizeof(QChar)));
    }

    obj->metadata = builder.toRelocatableData();
    builder.fromRelocatableData(&obj->extObject, obj->metatype, obj->metadata);

    if (mode == IgnoreAliases && hasAlias)
        compileState.aliasingObjects << obj;

    obj->synthdata = dynamicData;

    return true;
}

bool QDeclarativeCompiler::checkValidId(QDeclarativeParser::Value *v, const QString &val)
{
    if (val.isEmpty()) 
        COMPILE_EXCEPTION(v, tr( "Invalid empty ID"));

    if (val.at(0).isLetter() && !val.at(0).isLower()) 
        COMPILE_EXCEPTION(v, tr( "IDs cannot start with an uppercase letter"));

    QChar u(QLatin1Char('_'));
    for (int ii = 0; ii < val.count(); ++ii) {

        if (ii == 0 && !val.at(ii).isLetter() && val.at(ii) != u) {
            COMPILE_EXCEPTION(v, tr( "IDs must start with a letter or underscore"));
        } else if (ii != 0 && !val.at(ii).isLetterOrNumber() && val.at(ii) != u)  {
            COMPILE_EXCEPTION(v, tr( "IDs must contain only letters, numbers, and underscores"));
        }

    }

    if (QDeclarativeEnginePrivate::get(engine)->globalClass->illegalNames().contains(val))
        COMPILE_EXCEPTION(v, tr( "ID illegally masks global JavaScript property"));

    return true;
}

#include <qdeclarativejsparser_p.h>

static QStringList astNodeToStringList(QDeclarativeJS::AST::Node *node)
{
    if (node->kind == QDeclarativeJS::AST::Node::Kind_IdentifierExpression) {
        QString name =
            static_cast<QDeclarativeJS::AST::IdentifierExpression *>(node)->name->asString();
        return QStringList() << name;
    } else if (node->kind == QDeclarativeJS::AST::Node::Kind_FieldMemberExpression) {
        QDeclarativeJS::AST::FieldMemberExpression *expr = static_cast<QDeclarativeJS::AST::FieldMemberExpression *>(node);

        QStringList rv = astNodeToStringList(expr->base);
        if (rv.isEmpty())
            return rv;
        rv.append(expr->name->asString());
        return rv;
    }
    return QStringList();
}

bool QDeclarativeCompiler::compileAlias(QMetaObjectBuilder &builder,
                               QByteArray &data,
                               Object *obj,
                               const Object::DynamicProperty &prop)
{
    if (!prop.defaultValue)
        COMPILE_EXCEPTION(obj, tr("No property alias location"));

    if (prop.defaultValue->values.count() != 1 ||
        prop.defaultValue->values.at(0)->object ||
        !prop.defaultValue->values.at(0)->value.isScript())
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));

    QDeclarativeJS::AST::Node *node = prop.defaultValue->values.at(0)->value.asAST();
    if (!node)
        COMPILE_EXCEPTION(obj, tr("No property alias location")); // ### Can this happen?

    QStringList alias = astNodeToStringList(node);

    if (alias.count() != 1 && alias.count() != 2)
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias reference. An alias reference must be specified as <id> or <id>.<property>"));

    if (!compileState.ids.contains(alias.at(0)))
        COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias reference. Unable to find id \"%1\"").arg(alias.at(0)));

    Object *idObject = compileState.ids[alias.at(0)];

    QByteArray typeName;

    int propIdx = -1;
    int flags = 0;
    bool writable = false;
    if (alias.count() == 2) {
        propIdx = idObject->metaObject()->indexOfProperty(alias.at(1).toUtf8().constData());

        if (-1 == propIdx)
            COMPILE_EXCEPTION(prop.defaultValue, tr("Invalid alias location"));

        QMetaProperty aliasProperty = idObject->metaObject()->property(propIdx);
        writable = aliasProperty.isWritable();

        if (aliasProperty.isEnumType()) 
            typeName = "int";  // Avoid introducing a dependency on the aliased metaobject
        else
            typeName = aliasProperty.typeName();
    } else {
        typeName = idObject->metaObject()->className();

        //use the base type since it has been registered with metatype system
        int index = typeName.indexOf("_QML_");
        if (index != -1) {
            typeName = typeName.left(index);
        } else {
            index = typeName.indexOf("_QMLTYPE_");
            const QMetaObject *mo = idObject->metaObject();
            while (index != -1 && mo) {
                typeName = mo->superClass()->className();
                index = typeName.indexOf("_QMLTYPE_");
                mo = mo->superClass();
            }
        }

        typeName += '*';
    }

    if (typeName.endsWith('*'))
        flags |= QML_ALIAS_FLAG_PTR;

    data.append((const char *)&idObject->idIndex, sizeof(idObject->idIndex));
    data.append((const char *)&propIdx, sizeof(propIdx));
    data.append((const char *)&flags, sizeof(flags));

    builder.addSignal(prop.name + "Changed()");
    QMetaPropertyBuilder propBuilder = 
        builder.addProperty(prop.name, typeName.constData(), builder.methodCount() - 1);
    propBuilder.setScriptable(true);
    propBuilder.setWritable(writable);
    return true;
}

bool QDeclarativeCompiler::buildBinding(QDeclarativeParser::Value *value,
                               QDeclarativeParser::Property *prop,
                               const BindingContext &ctxt)
{
    Q_ASSERT(prop->index != -1);
    Q_ASSERT(prop->parent);
    Q_ASSERT(prop->parent->metaObject());

    QMetaProperty mp = prop->parent->metaObject()->property(prop->index);
    if (!mp.isWritable() && !QDeclarativeMetaType::isList(prop->type))
        COMPILE_EXCEPTION(prop, tr("Invalid property assignment: \"%1\" is a read-only property").arg(QString::fromUtf8(prop->name)));

    BindingReference reference;
    reference.expression = value->value;
    reference.property = prop;
    reference.value = value;
    reference.bindingContext = ctxt;
    addBindingReference(reference);

    return true;
}

void QDeclarativeCompiler::genBindingAssignment(QDeclarativeParser::Value *binding,
                                                QDeclarativeParser::Property *prop,
                                                QDeclarativeParser::Object *obj,
                                                QDeclarativeParser::Property *valueTypeProperty)
{
    Q_UNUSED(obj);
    Q_ASSERT(compileState.bindings.contains(binding));

    const BindingReference &ref = compileState.bindings.value(binding);
    if (ref.dataType == BindingReference::Experimental) {
        QDeclarativeInstruction store;
        store.type = QDeclarativeInstruction::StoreCompiledBinding;
        store.assignBinding.value = ref.compiledIndex;
        store.assignBinding.context = ref.bindingContext.stack;
        store.assignBinding.owner = ref.bindingContext.owner;
        if (valueTypeProperty) 
            store.assignBinding.property = (valueTypeProperty->index & 0xFFFF) |
                                           ((valueTypeProperty->type & 0xFF)) << 16 |
                                           ((prop->index & 0xFF) << 24);
        else 
            store.assignBinding.property = prop->index;
        store.line = binding->location.start.line;
        output->bytecode << store;
        return;
    }

    QDeclarativeInstruction store;
    store.type = QDeclarativeInstruction::StoreBinding;
    store.assignBinding.value = output->indexForByteArray(ref.compiledData);
    store.assignBinding.context = ref.bindingContext.stack;
    store.assignBinding.owner = ref.bindingContext.owner;
    store.line = binding->location.start.line;

    Q_ASSERT(ref.bindingContext.owner == 0 ||
             (ref.bindingContext.owner != 0 && valueTypeProperty));
    if (ref.bindingContext.owner) {
        store.assignBinding.property = genValueTypeData(prop, valueTypeProperty);
    } else {
        store.assignBinding.property = genPropertyData(prop);
    }

    output->bytecode << store;
}

int QDeclarativeCompiler::genContextCache()
{
    if (compileState.ids.count() == 0)
        return -1;

    QDeclarativeIntegerCache *cache = new QDeclarativeIntegerCache(engine);

    for (QHash<QString, QDeclarativeParser::Object *>::ConstIterator iter = compileState.ids.begin();
         iter != compileState.ids.end();
         ++iter) 
        cache->add(iter.key(), (*iter)->idIndex);

    output->contextCaches.append(cache);
    return output->contextCaches.count() - 1;
}

int QDeclarativeCompiler::genValueTypeData(QDeclarativeParser::Property *valueTypeProp, 
                                  QDeclarativeParser::Property *prop)
{
    QByteArray data =
        QDeclarativePropertyPrivate::saveValueType(prop->parent->metaObject(), prop->index, 
                                              QDeclarativeEnginePrivate::get(engine)->valueTypes[prop->type]->metaObject(), 
                                              valueTypeProp->index);
//                valueTypeProp->index, valueTypeProp->type);

    return output->indexForByteArray(data);
}

int QDeclarativeCompiler::genPropertyData(QDeclarativeParser::Property *prop)
{
    return output->indexForByteArray(QDeclarativePropertyPrivate::saveProperty(prop->parent->metaObject(), prop->index));
}

bool QDeclarativeCompiler::completeComponentBuild()
{
    componentStat.ids = compileState.ids.count();

    for (int ii = 0; ii < compileState.aliasingObjects.count(); ++ii) {
        Object *aliasObject = compileState.aliasingObjects.at(ii);
        COMPILE_CHECK(buildDynamicMeta(aliasObject, ResolveAliases));
    }

    QDeclarativeBindingCompiler::Expression expr;
    expr.component = compileState.root;
    expr.ids = compileState.ids;

    QDeclarativeBindingCompiler bindingCompiler;

    for (QHash<QDeclarativeParser::Value*,BindingReference>::Iterator iter = compileState.bindings.begin(); 
         iter != compileState.bindings.end(); ++iter) {

        BindingReference &binding = *iter;

        expr.context = binding.bindingContext.object;
        expr.property = binding.property;
        expr.expression = binding.expression;
        expr.imports = unit->imports;

        int index = bindingCompiler.compile(expr, QDeclarativeEnginePrivate::get(engine));
        if (index != -1) {
            binding.dataType = BindingReference::Experimental;
            binding.compiledIndex = index;
            componentStat.optimizedBindings.append(iter.key()->location);
            continue;
        } 

        binding.dataType = BindingReference::QtScript;

        // Pre-rewrite the expression
        QString expression = binding.expression.asScript();

        // ### Optimize
        QDeclarativeRewrite::SharedBindingTester sharableTest;
        bool isSharable = sharableTest.isSharable(expression);
        
        QDeclarativeRewrite::RewriteBinding rewriteBinding;
        expression = rewriteBinding(expression);

        quint32 length = expression.length();
        quint32 pc; 
        
        if (isSharable) {
            pc = output->cachedClosures.count();
            pc |= 0x80000000;
            output->cachedClosures.append(0);
        } else {
            pc = output->cachedPrograms.length();
            output->cachedPrograms.append(0);
        }

        binding.compiledData =
            QByteArray((const char *)&pc, sizeof(quint32)) +
            QByteArray((const char *)&length, sizeof(quint32)) +
            QByteArray((const char *)expression.constData(), 
                       expression.length() * sizeof(QChar));

        componentStat.scriptBindings.append(iter.key()->location);
    }

    if (bindingCompiler.isValid()) {
        compileState.compiledBindingData = bindingCompiler.program();
        if (bindingsDump()) 
            QDeclarativeBindingCompiler::dump(compileState.compiledBindingData);
    }

    saveComponentState();

    return true;
}

void QDeclarativeCompiler::dumpStats()
{
    qWarning().nospace() << "QML Document: " << output->url.toString();
    for (int ii = 0; ii < savedComponentStats.count(); ++ii) {
        const ComponentStat &stat = savedComponentStats.at(ii);
        qWarning().nospace() << "    Component Line " << stat.lineNumber;
        qWarning().nospace() << "        Total Objects:      " << stat.objects;
        qWarning().nospace() << "        IDs Used:           " << stat.ids;
        qWarning().nospace() << "        Optimized Bindings: " << stat.optimizedBindings.count();

        {
        QByteArray output;
        for (int ii = 0; ii < stat.optimizedBindings.count(); ++ii) {
            if (0 == (ii % 10)) {
                if (ii) output.append("\n");
                output.append("            ");
            }

            output.append("(");
            output.append(QByteArray::number(stat.optimizedBindings.at(ii).start.line));
            output.append(":");
            output.append(QByteArray::number(stat.optimizedBindings.at(ii).start.column));
            output.append(") ");
        }
        if (!output.isEmpty())
            qWarning().nospace() << output.constData();
        }

        qWarning().nospace() << "        QScript Bindings:   " << stat.scriptBindings.count();
        {
        QByteArray output;
        for (int ii = 0; ii < stat.scriptBindings.count(); ++ii) {
            if (0 == (ii % 10)) {
                if (ii) output.append("\n");
                output.append("            ");
            }

            output.append("(");
            output.append(QByteArray::number(stat.scriptBindings.at(ii).start.line));
            output.append(":");
            output.append(QByteArray::number(stat.scriptBindings.at(ii).start.column));
            output.append(") ");
        }
        if (!output.isEmpty())
            qWarning().nospace() << output.constData();
        }
    }
}

/*!
    Returns true if from can be assigned to a (QObject) property of type
    to.
*/
bool QDeclarativeCompiler::canCoerce(int to, QDeclarativeParser::Object *from)
{
    const QMetaObject *toMo = 
        QDeclarativeEnginePrivate::get(engine)->rawMetaObjectForType(to);
    const QMetaObject *fromMo = from->metaObject();

    while (fromMo) {
        if (QDeclarativePropertyPrivate::equal(fromMo, toMo))
            return true;
        fromMo = fromMo->superClass();
    }
    return false;
}

QDeclarativeType *QDeclarativeCompiler::toQmlType(QDeclarativeParser::Object *from)
{
    // ### Optimize
    const QMetaObject *mo = from->metatype;
    QDeclarativeType *type = 0;
    while (!type && mo) {
        type = QDeclarativeMetaType::qmlType(mo);
        mo = mo->superClass();
    }
   return type;
}

QStringList QDeclarativeCompiler::deferredProperties(QDeclarativeParser::Object *obj)
{
    const QMetaObject *mo = obj->metatype;

    int idx = mo->indexOfClassInfo("DeferredPropertyNames");
    if (idx == -1)
        return QStringList();

    QMetaClassInfo classInfo = mo->classInfo(idx);
    QStringList rv = QString::fromUtf8(classInfo.value()).split(QLatin1Char(','));
    return rv;
}

QT_END_NAMESPACE
