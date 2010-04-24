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

#include "private/qdeclarativescriptparser_p.h"

#include "private/qdeclarativeparser_p.h"
#include "parser/qdeclarativejsengine_p.h"
#include "parser/qdeclarativejsparser_p.h"
#include "parser/qdeclarativejslexer_p.h"
#include "parser/qdeclarativejsnodepool_p.h"
#include "parser/qdeclarativejsastvisitor_p.h"
#include "parser/qdeclarativejsast_p.h"
#include "private/qdeclarativerewrite_p.h"

#include <QStack>
#include <QCoreApplication>
#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace QDeclarativeJS;
using namespace QDeclarativeParser;

namespace {

class ProcessAST: protected AST::Visitor
{
    struct State {
        State() : object(0), property(0) {}
        State(Object *o) : object(o), property(0) {}
        State(Object *o, Property *p) : object(o), property(p) {}

        Object *object;
        Property *property;
    };

    struct StateStack : public QStack<State>
    {
        void pushObject(Object *obj)
        {
            push(State(obj));
        }

        void pushProperty(const QString &name, const LocationSpan &location)
        {
            const State &state = top();
            if (state.property) {
                State s(state.property->getValue(location),
                        state.property->getValue(location)->getProperty(name.toUtf8()));
                s.property->location = location;
                push(s);
            } else {
                State s(state.object,
                        state.object->getProperty(name.toUtf8()));

                s.property->location = location;
                push(s);
            }
        }
    };

public:
    ProcessAST(QDeclarativeScriptParser *parser);
    virtual ~ProcessAST();

    void operator()(const QString &code, AST::Node *node);

protected:
    Object *defineObjectBinding(AST::UiQualifiedId *propertyName, bool onAssignment,
                                AST::UiQualifiedId *objectTypeName,
                                LocationSpan location,
                                AST::UiObjectInitializer *initializer = 0);

    Object *defineObjectBinding_helper(AST::UiQualifiedId *propertyName, bool onAssignment,
                                       const QString &objectType,
                                       AST::SourceLocation typeLocation,
                                       LocationSpan location,
                                       AST::UiObjectInitializer *initializer = 0);

    QDeclarativeParser::Variant getVariant(AST::ExpressionNode *expr);

    LocationSpan location(AST::SourceLocation start, AST::SourceLocation end);
    LocationSpan location(AST::UiQualifiedId *);

    using AST::Visitor::visit;
    using AST::Visitor::endVisit;

    virtual bool visit(AST::UiProgram *node);
    virtual bool visit(AST::UiImport *node);
    virtual bool visit(AST::UiObjectDefinition *node);
    virtual bool visit(AST::UiPublicMember *node);
    virtual bool visit(AST::UiObjectBinding *node);

    virtual bool visit(AST::UiScriptBinding *node);
    virtual bool visit(AST::UiArrayBinding *node);
    virtual bool visit(AST::UiSourceElement *node);

    void accept(AST::Node *node);

    QString asString(AST::UiQualifiedId *node) const;

    const State state() const;
    Object *currentObject() const;
    Property *currentProperty() const;

    QString qualifiedNameId() const;

    QString textAt(const AST::SourceLocation &loc) const
    { return _contents.mid(loc.offset, loc.length); }


    QString textAt(const AST::SourceLocation &first,
                   const AST::SourceLocation &last) const
    { return _contents.mid(first.offset, last.offset + last.length - first.offset); }

    QString asString(AST::ExpressionNode *expr)
    {
        if (! expr)
            return QString();

        return textAt(expr->firstSourceLocation(), expr->lastSourceLocation());
    }

    QString asString(AST::Statement *stmt)
    {
        if (! stmt)
            return QString();

        QString s = textAt(stmt->firstSourceLocation(), stmt->lastSourceLocation());
        s += QLatin1Char('\n');
        return s;
    }

private:
    QDeclarativeScriptParser *_parser;
    StateStack _stateStack;
    QStringList _scope;
    QString _contents;

    inline bool isSignalProperty(const QByteArray &propertyName) const {
        return (propertyName.length() >= 3 && propertyName.startsWith("on") &&
                ('A' <= propertyName.at(2) && 'Z' >= propertyName.at(2)));
    }

};

ProcessAST::ProcessAST(QDeclarativeScriptParser *parser)
    : _parser(parser)
{
}

ProcessAST::~ProcessAST()
{
}

void ProcessAST::operator()(const QString &code, AST::Node *node)
{
    _contents = code;
    accept(node);
}

void ProcessAST::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

const ProcessAST::State ProcessAST::state() const
{
    if (_stateStack.isEmpty())
        return State();

    return _stateStack.back();
}

Object *ProcessAST::currentObject() const
{
    return state().object;
}

Property *ProcessAST::currentProperty() const
{
    return state().property;
}

QString ProcessAST::qualifiedNameId() const
{
    return _scope.join(QLatin1String("/"));
}

QString ProcessAST::asString(AST::UiQualifiedId *node) const
{
    QString s;

    for (AST::UiQualifiedId *it = node; it; it = it->next) {
        s.append(it->name->asString());

        if (it->next)
            s.append(QLatin1Char('.'));
    }

    return s;
}

Object *
ProcessAST::defineObjectBinding_helper(AST::UiQualifiedId *propertyName,
                                       bool onAssignment,
                                       const QString &objectType,
                                       AST::SourceLocation typeLocation,
                                       LocationSpan location,
                                       AST::UiObjectInitializer *initializer)
{
    int lastTypeDot = objectType.lastIndexOf(QLatin1Char('.'));
    bool isType = !objectType.isEmpty() &&
                    (objectType.at(0).isUpper() ||
                        (lastTypeDot >= 0 && objectType.at(lastTypeDot+1).isUpper()));

    int propertyCount = 0;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name->asString(),
                                 this->location(name));
    }

    if (!onAssignment && propertyCount && currentProperty() && currentProperty()->values.count()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    if (!isType) {

        if(propertyCount || !currentObject()) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected type name"));
            error.setLine(typeLocation.startLine);
            error.setColumn(typeLocation.startColumn);
            _parser->_errors << error;
            return 0;
        }

        LocationSpan loc = ProcessAST::location(typeLocation, typeLocation);
        if (propertyName)
            loc = ProcessAST::location(propertyName);

        _stateStack.pushProperty(objectType, loc);
       accept(initializer);
        _stateStack.pop();

        return 0;

    } else {
        // Class

        QString resolvableObjectType = objectType;
        if (lastTypeDot >= 0)
            resolvableObjectType.replace(QLatin1Char('.'),QLatin1Char('/'));

        Object *obj = new Object;

        QDeclarativeScriptParser::TypeReference *typeRef = _parser->findOrCreateType(resolvableObjectType);
        obj->type = typeRef->id;

        typeRef->refObjects.append(obj);

        // XXX this doesn't do anything (_scope never builds up)
        _scope.append(resolvableObjectType);
        obj->typeName = qualifiedNameId().toUtf8();
        _scope.removeLast();

        obj->location = location;

        if (propertyCount) {

            Property *prop = currentProperty();
            Value *v = new Value;
            v->object = obj;
            v->location = obj->location;
            if (onAssignment)
                prop->addOnValue(v);
            else
                prop->addValue(v);

            while (propertyCount--)
                _stateStack.pop();

        } else {

            if (! _parser->tree()) {
                _parser->setTree(obj);
            } else {
                const State state = _stateStack.top();
                Value *v = new Value;
                v->object = obj;
                v->location = obj->location;
                if (state.property) {
                    state.property->addValue(v);
                } else {
                    Property *defaultProp = state.object->getDefaultProperty();
                    if (defaultProp->location.start.line == -1) {
                        defaultProp->location = v->location;
                        defaultProp->location.end = defaultProp->location.start;
                        defaultProp->location.range.length = 0;
                    }
                    defaultProp->addValue(v);
                }
            }
        }

        _stateStack.pushObject(obj);
        accept(initializer);
        _stateStack.pop();

        return obj;
    }
}

Object *ProcessAST::defineObjectBinding(AST::UiQualifiedId *qualifiedId, bool onAssignment,
                                        AST::UiQualifiedId *objectTypeName,
                                        LocationSpan location,
                                        AST::UiObjectInitializer *initializer)
{
    const QString objectType = asString(objectTypeName);
    const AST::SourceLocation typeLocation = objectTypeName->identifierToken;

    if (objectType == QLatin1String("Script")) {

        AST::UiObjectMemberList *it = initializer->members;
        for (; it; it = it->next) {
            AST::UiScriptBinding *scriptBinding = AST::cast<AST::UiScriptBinding *>(it->member);
            if (! scriptBinding)
                continue;

            QString propertyName = asString(scriptBinding->qualifiedId);
            if (propertyName == QLatin1String("source")) {
                if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(scriptBinding->statement)) {
                    QDeclarativeParser::Variant string = getVariant(stmt->expression);
                    if (string.isStringList()) {
                        QStringList urls = string.asStringList();
                        // We need to add this as a resource
                        for (int ii = 0; ii < urls.count(); ++ii)
                            _parser->_refUrls << QUrl(urls.at(ii));
                    }
                }
            }
        }

    }

    return defineObjectBinding_helper(qualifiedId, onAssignment, objectType, typeLocation, location, initializer);
}

LocationSpan ProcessAST::location(AST::UiQualifiedId *id)
{
    return location(id->identifierToken, id->identifierToken);
}

LocationSpan ProcessAST::location(AST::SourceLocation start, AST::SourceLocation end)
{
    LocationSpan rv;
    rv.start.line = start.startLine;
    rv.start.column = start.startColumn;
    rv.end.line = end.startLine;
    rv.end.column = end.startColumn + end.length - 1;
    rv.range.offset = start.offset;
    rv.range.length = end.offset + end.length - start.offset;
    return rv;
}

// UiProgram: UiImportListOpt UiObjectMemberList ;
bool ProcessAST::visit(AST::UiProgram *node)
{
    accept(node->imports);
    accept(node->members->member);
    return false;
}

// UiImport: T_IMPORT T_STRING_LITERAL ;
bool ProcessAST::visit(AST::UiImport *node)
{
    QString uri;
    QDeclarativeScriptParser::Import import;

    if (node->fileName) {
        uri = node->fileName->asString();

        if (uri.endsWith(QLatin1String(".js"))) {
            import.type = QDeclarativeScriptParser::Import::Script;
            _parser->_refUrls << QUrl(uri);
        } else {
            import.type = QDeclarativeScriptParser::Import::File;
        }
    } else {
        import.type = QDeclarativeScriptParser::Import::Library;
        uri = asString(node->importUri);
    }

    AST::SourceLocation startLoc = node->importToken;
    AST::SourceLocation endLoc = node->semicolonToken;

    // Qualifier
    if (node->importId) {
        import.qualifier = node->importId->asString();
        if (!import.qualifier.at(0).isUpper()) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Invalid import qualifier ID"));
            error.setLine(node->importIdToken.startLine);
            error.setColumn(node->importIdToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        // Check for script qualifier clashes
        bool isScript = import.type == QDeclarativeScriptParser::Import::Script;
        for (int ii = 0; ii < _parser->_imports.count(); ++ii) {
            const QDeclarativeScriptParser::Import &other = _parser->_imports.at(ii);
            bool otherIsScript = other.type == QDeclarativeScriptParser::Import::Script;

            if ((isScript || otherIsScript) && import.qualifier == other.qualifier) {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Script import qualifiers must be unique."));
                error.setLine(node->importIdToken.startLine);
                error.setColumn(node->importIdToken.startColumn);
                _parser->_errors << error;
                return false;
            }
        }

    } else if (import.type == QDeclarativeScriptParser::Import::Script) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Script import requires a qualifier"));
        error.setLine(node->fileNameToken.startLine);
        error.setColumn(node->fileNameToken.startColumn);
        _parser->_errors << error;
        return false;
    }

    if (node->versionToken.isValid()) {
        import.version = textAt(node->versionToken);
    } else if (import.type == QDeclarativeScriptParser::Import::Library) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Library import requires a version"));
        error.setLine(node->importIdToken.startLine);
        error.setColumn(node->importIdToken.startColumn);
        _parser->_errors << error;
        return false;
    }


    import.location = location(startLoc, endLoc);
    import.uri = uri;

    _parser->_imports << import;

    return false;
}

bool ProcessAST::visit(AST::UiPublicMember *node)
{
    const struct TypeNameToType {
        const char *name;
        Object::DynamicProperty::Type type;
        const char *qtName;
    } propTypeNameToTypes[] = {
        { "int", Object::DynamicProperty::Int, "int" },
        { "bool", Object::DynamicProperty::Bool, "bool" },
        { "double", Object::DynamicProperty::Real, "double" },
        { "real", Object::DynamicProperty::Real, "qreal" },
        { "string", Object::DynamicProperty::String, "QString" },
        { "url", Object::DynamicProperty::Url, "QUrl" },
        { "color", Object::DynamicProperty::Color, "QColor" },
        // Internally QTime, QDate and QDateTime are all supported.
        // To be more consistent with JavaScript we expose only
        // QDateTime as it matches closely with the Date JS type.
        // We also call it "date" to match.
        // { "time", Object::DynamicProperty::Time, "QTime" },
        // { "date", Object::DynamicProperty::Date, "QDate" },
        { "date", Object::DynamicProperty::DateTime, "QDateTime" },
        { "variant", Object::DynamicProperty::Variant, "QVariant" }
    };
    const int propTypeNameToTypesCount = sizeof(propTypeNameToTypes) /
                                         sizeof(propTypeNameToTypes[0]);

    if(node->type == AST::UiPublicMember::Signal) {
        const QString name = node->name->asString();

        Object::DynamicSignal signal;
        signal.name = name.toUtf8();

        AST::UiParameterList *p = node->parameters;
        while (p) {
            const QString memberType = p->type->asString();
            const char *qtType = 0;
            for(int ii = 0; !qtType && ii < propTypeNameToTypesCount; ++ii) {
                if(QLatin1String(propTypeNameToTypes[ii].name) == memberType)
                    qtType = propTypeNameToTypes[ii].qtName;
            }

            if (!qtType) {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected parameter type"));
                error.setLine(node->typeToken.startLine);
                error.setColumn(node->typeToken.startColumn);
                _parser->_errors << error;
                return false;
            }
            
            signal.parameterTypes << qtType;
            signal.parameterNames << p->name->asString().toUtf8();
            p = p->finish();
        }

        _stateStack.top().object->dynamicSignals << signal;
    } else {
        const QString memberType = node->memberType->asString();
        const QString name = node->name->asString();

        bool typeFound = false;
        Object::DynamicProperty::Type type;

        if (memberType == QLatin1String("alias")) {
            type = Object::DynamicProperty::Alias;
            typeFound = true;
        }

        for(int ii = 0; !typeFound && ii < propTypeNameToTypesCount; ++ii) {
            if(QLatin1String(propTypeNameToTypes[ii].name) == memberType) {
                type = propTypeNameToTypes[ii].type;
                typeFound = true;
            }
        }

        if (!typeFound && memberType.at(0).isUpper()) {
            QString typemodifier;
            if(node->typeModifier)
                typemodifier = node->typeModifier->asString();
            if (typemodifier == QString()) {
                type = Object::DynamicProperty::Custom;
            } else if(typemodifier == QLatin1String("list")) {
                type = Object::DynamicProperty::CustomList;
            } else {
                QDeclarativeError error;
                error.setDescription(QCoreApplication::translate("QDeclarativeParser","Invalid property type modifier"));
                error.setLine(node->typeModifierToken.startLine);
                error.setColumn(node->typeModifierToken.startColumn);
                _parser->_errors << error;
                return false;
            }
            typeFound = true;
        } else if (node->typeModifier) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Unexpected property type modifier"));
            error.setLine(node->typeModifierToken.startLine);
            error.setColumn(node->typeModifierToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        if(!typeFound) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Expected property type"));
            error.setLine(node->typeToken.startLine);
            error.setColumn(node->typeToken.startColumn);
            _parser->_errors << error;
            return false;
        }

        if (node->isReadonlyMember) {
            QDeclarativeError error;
            error.setDescription(QCoreApplication::translate("QDeclarativeParser","Readonly not yet supported"));
            error.setLine(node->readonlyToken.startLine);
            error.setColumn(node->readonlyToken.startColumn);
            _parser->_errors << error;
            return false;

        }
        Object::DynamicProperty property;
        property.isDefaultProperty = node->isDefaultMember;
        property.type = type;
        if (type >= Object::DynamicProperty::Custom) {
            QDeclarativeScriptParser::TypeReference *typeRef =
                _parser->findOrCreateType(memberType);
            typeRef->refObjects.append(_stateStack.top().object);
        }
        property.customType = memberType.toUtf8();
        property.name = name.toUtf8();
        property.location = location(node->firstSourceLocation(),
                                     node->lastSourceLocation());

        if (node->expression) { // default value
            property.defaultValue = new Property;
            property.defaultValue->parent = _stateStack.top().object;
            property.defaultValue->location =
                    location(node->expression->firstSourceLocation(),
                             node->expression->lastSourceLocation());
            Value *value = new Value;
            value->location = location(node->expression->firstSourceLocation(),
                                       node->expression->lastSourceLocation());
            value->value = getVariant(node->expression);
            property.defaultValue->values << value;
        }

        _stateStack.top().object->dynamicProperties << property;

        // process QML-like initializers (e.g. property Object o: Object {})
        accept(node->binding);
    }

    return false;
}


// UiObjectMember: UiQualifiedId UiObjectInitializer ;
bool ProcessAST::visit(AST::UiObjectDefinition *node)
{
    LocationSpan l = location(node->firstSourceLocation(),
                              node->lastSourceLocation());

    defineObjectBinding(/*propertyName = */ 0, false,
                        node->qualifiedTypeNameId,
                        l,
                        node->initializer);

    return false;
}


// UiObjectMember: UiQualifiedId T_COLON UiQualifiedId UiObjectInitializer ;
bool ProcessAST::visit(AST::UiObjectBinding *node)
{
    LocationSpan l = location(node->qualifiedTypeNameId->identifierToken,
                              node->initializer->rbraceToken);

    defineObjectBinding(node->qualifiedId, node->hasOnToken,
                        node->qualifiedTypeNameId,
                        l,
                        node->initializer);

    return false;
}

QDeclarativeParser::Variant ProcessAST::getVariant(AST::ExpressionNode *expr)
{
    if (AST::StringLiteral *lit = AST::cast<AST::StringLiteral *>(expr)) {
        return QDeclarativeParser::Variant(lit->value->asString());
    } else if (expr->kind == AST::Node::Kind_TrueLiteral) {
        return QDeclarativeParser::Variant(true);
    } else if (expr->kind == AST::Node::Kind_FalseLiteral) {
        return QDeclarativeParser::Variant(false);
    } else if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(expr)) {
        return QDeclarativeParser::Variant(lit->value, asString(expr));
    } else {

        if (AST::UnaryMinusExpression *unaryMinus = AST::cast<AST::UnaryMinusExpression *>(expr)) {
           if (AST::NumericLiteral *lit = AST::cast<AST::NumericLiteral *>(unaryMinus->expression)) {
               return QDeclarativeParser::Variant(-lit->value, asString(expr));
           }
        }

        return  QDeclarativeParser::Variant(asString(expr), expr);
    }
}


// UiObjectMember: UiQualifiedId T_COLON Statement ;
bool ProcessAST::visit(AST::UiScriptBinding *node)
{
    int propertyCount = 0;
    AST::UiQualifiedId *propertyName = node->qualifiedId;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name->asString(),
                                 location(name));
    }

    Property *prop = currentProperty();

    if (prop->values.count()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    QDeclarativeParser::Variant primitive;

    if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement *>(node->statement)) {
        primitive = getVariant(stmt->expression);
    } else { // do binding
        primitive = QDeclarativeParser::Variant(asString(node->statement),
                                       node->statement);
    }

    prop->location.range.length = prop->location.range.offset + prop->location.range.length - node->qualifiedId->identifierToken.offset;
    prop->location.range.offset = node->qualifiedId->identifierToken.offset;
    Value *v = new Value;
    v->value = primitive;
    v->location = location(node->statement->firstSourceLocation(),
                           node->statement->lastSourceLocation());

    prop->addValue(v);

    while (propertyCount--)
        _stateStack.pop();

    return true;
}

static QList<int> collectCommas(AST::UiArrayMemberList *members)
{
    QList<int> commas;

    if (members) {
        for (AST::UiArrayMemberList *it = members->next; it; it = it->next) {
            commas.append(it->commaToken.offset);
        }
    }

    return commas;
}

// UiObjectMember: UiQualifiedId T_COLON T_LBRACKET UiArrayMemberList T_RBRACKET ;
bool ProcessAST::visit(AST::UiArrayBinding *node)
{
    int propertyCount = 0;
    AST::UiQualifiedId *propertyName = node->qualifiedId;
    for (AST::UiQualifiedId *name = propertyName; name; name = name->next){
        ++propertyCount;
        _stateStack.pushProperty(name->name->asString(),
                                 location(name));
    }

    Property* prop = currentProperty();

    if (prop->values.count()) {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","Property value set multiple times"));
        error.setLine(this->location(propertyName).start.line);
        error.setColumn(this->location(propertyName).start.column);
        _parser->_errors << error;
        return 0;
    }

    accept(node->members);

    // For the DOM, store the position of the T_LBRACKET upto the T_RBRACKET as the range:
    prop->listValueRange.offset = node->lbracketToken.offset;
    prop->listValueRange.length = node->rbracketToken.offset + node->rbracketToken.length - node->lbracketToken.offset;

    // Store the positions of the comma token too, again for the DOM to be able to retreive it.
    prop->listCommaPositions = collectCommas(node->members);

    while (propertyCount--)
        _stateStack.pop();

    return false;
}

bool ProcessAST::visit(AST::UiSourceElement *node)
{
    QDeclarativeParser::Object *obj = currentObject();

    if (AST::FunctionDeclaration *funDecl = AST::cast<AST::FunctionDeclaration *>(node->sourceElement)) {

        Object::DynamicSlot slot;
        slot.location = location(funDecl->firstSourceLocation(), funDecl->lastSourceLocation());

        AST::FormalParameterList *f = funDecl->formals;
        while (f) {
            slot.parameterNames << f->name->asString().toUtf8();
            f = f->finish();
        }

        QString body = textAt(funDecl->lbraceToken, funDecl->rbraceToken);
        slot.name = funDecl->name->asString().toUtf8();
        slot.body = body;
        obj->dynamicSlots << slot;

    } else {
        QDeclarativeError error;
        error.setDescription(QCoreApplication::translate("QDeclarativeParser","JavaScript declaration outside Script element"));
        error.setLine(node->firstSourceLocation().startLine);
        error.setColumn(node->firstSourceLocation().startColumn);
        _parser->_errors << error;
    }
    return false;
}

} // end of anonymous namespace


QDeclarativeScriptParser::QDeclarativeScriptParser()
: root(0), data(0)
{

}

QDeclarativeScriptParser::~QDeclarativeScriptParser()
{
    clear();
}

class QDeclarativeScriptParserJsASTData
{
public:
    QDeclarativeScriptParserJsASTData(const QString &filename)
        : nodePool(filename, &engine) {}

    Engine engine;
    NodePool nodePool;
};

bool QDeclarativeScriptParser::parse(const QByteArray &qmldata, const QUrl &url)
{
    clear();

    const QString fileName = url.toString();
    _scriptFile = fileName;

    QTextStream stream(qmldata, QIODevice::ReadOnly);
    stream.setCodec("UTF-8");
    const QString code = stream.readAll();

    data = new QDeclarativeScriptParserJsASTData(fileName);

    Lexer lexer(&data->engine);
    lexer.setCode(code, /*line = */ 1);

    Parser parser(&data->engine);

    if (! parser.parse() || !_errors.isEmpty()) {

        // Extract errors from the parser
        foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {

            if (m.isWarning())
                continue;

            QDeclarativeError error;
            error.setUrl(url);
            error.setDescription(m.message);
            error.setLine(m.loc.startLine);
            error.setColumn(m.loc.startColumn);
            _errors << error;

        }
    }

    if (_errors.isEmpty()) {
        ProcessAST process(this);
        process(code, parser.ast());

        // Set the url for process errors
        for(int ii = 0; ii < _errors.count(); ++ii)
            _errors[ii].setUrl(url);
    }

    return _errors.isEmpty();
}

QList<QDeclarativeScriptParser::TypeReference*> QDeclarativeScriptParser::referencedTypes() const
{
    return _refTypes;
}

QList<QUrl> QDeclarativeScriptParser::referencedResources() const
{
    return _refUrls;
}

Object *QDeclarativeScriptParser::tree() const
{
    return root;
}

QList<QDeclarativeScriptParser::Import> QDeclarativeScriptParser::imports() const
{
    return _imports;
}

QList<QDeclarativeError> QDeclarativeScriptParser::errors() const
{
    return _errors;
}

/*
Searches for ".pragma <value>" declarations within \a script.  Currently supported pragmas
are:
    library
*/
QDeclarativeParser::Object::ScriptBlock::Pragmas QDeclarativeScriptParser::extractPragmas(QString &script)
{
    QDeclarativeParser::Object::ScriptBlock::Pragmas rv = QDeclarativeParser::Object::ScriptBlock::None;

    const QChar forwardSlash(QLatin1Char('/'));
    const QChar star(QLatin1Char('*'));
    const QChar newline(QLatin1Char('\n'));
    const QChar dot(QLatin1Char('.'));
    const QChar semicolon(QLatin1Char(';'));
    const QChar space(QLatin1Char(' '));
    const QString pragma(QLatin1String(".pragma "));

    const QChar *pragmaData = pragma.constData();

    const QChar *data = script.constData();
    const int length = script.count();
    for (int ii = 0; ii < length; ++ii) {
        const QChar &c = data[ii];

        if (c.isSpace())
            continue;

        if (c == forwardSlash) {
            ++ii;
            if (ii >= length)
                return rv;

            const QChar &c = data[ii];
            if (c == forwardSlash) {
                // Find next newline
                while (ii < length && data[++ii] != newline) {};
            } else if (c == star) {
                // Find next star
                while (true) {
                    while (ii < length && data[++ii] != star) {};
                    if (ii + 1 >= length)
                        return rv;

                    if (data[ii + 1] == forwardSlash) {
                        ++ii;
                        break;
                    }
                }
            } else {
                return rv;
            }
        } else if (c == dot) {
            // Could be a pragma!
            if (ii + pragma.length() >= length ||
                0 != ::memcmp(data + ii, pragmaData, sizeof(QChar) * pragma.length()))
                return rv;

            int pragmaStatementIdx = ii;

            ii += pragma.length();

            while (ii < length && data[ii].isSpace()) { ++ii; }

            int startIdx = ii;

            while (ii < length && data[ii].isLetter()) { ++ii; }

            int endIdx = ii;

            if (ii != length && data[ii] != forwardSlash && !data[ii].isSpace() && data[ii] != semicolon)
                return rv;

            QString p(data + startIdx, endIdx - startIdx);

            if (p == QLatin1String("library"))
                rv |= QDeclarativeParser::Object::ScriptBlock::Shared;
            else
                return rv;

            for (int jj = pragmaStatementIdx; jj < endIdx; ++jj) script[jj] = space;

        } else {
            return rv;
        }
    }

    return rv;
}

void QDeclarativeScriptParser::clear()
{
    if (root) {
        root->release();
        root = 0;
    }
    _imports.clear();
    qDeleteAll(_refTypes);
    _refTypes.clear();
    _errors.clear();

    if (data) {
        delete data;
        data = 0;
    }
}

QDeclarativeScriptParser::TypeReference *QDeclarativeScriptParser::findOrCreateType(const QString &name)
{
    TypeReference *type = 0;
    int i = 0;
    for (; i < _refTypes.size(); ++i) {
        if (_refTypes.at(i)->name == name) {
            type = _refTypes.at(i);
            break;
        }
    }
    if (!type) {
        type = new TypeReference(i, name);
        _refTypes.append(type);
    }

    return type;
}

void QDeclarativeScriptParser::setTree(Object *tree)
{
    Q_ASSERT(! root);

    root = tree;
}

QT_END_NAMESPACE
