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

#ifndef QDECLARATIVECUSTOMPARSER_H
#define QDECLARATIVECUSTOMPARSER_H

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

#include "private/qdeclarativemetatype_p.h"
#include "qdeclarativeerror.h"
#include "private/qdeclarativeparser_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qxmlstream.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeCompiler;

class QDeclarativeCustomParserPropertyPrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeCustomParserProperty
{
public:
    QDeclarativeCustomParserProperty();
    QDeclarativeCustomParserProperty(const QDeclarativeCustomParserProperty &);
    QDeclarativeCustomParserProperty &operator=(const QDeclarativeCustomParserProperty &);
    ~QDeclarativeCustomParserProperty();

    QByteArray name() const;
    QDeclarativeParser::Location location() const;

    bool isList() const;
    // Will be one of QDeclarativeParser::Variant, QDeclarativeCustomParserProperty or 
    // QDeclarativeCustomParserNode
    QList<QVariant> assignedValues() const;

private:
    friend class QDeclarativeCustomParserNodePrivate;
    friend class QDeclarativeCustomParserPropertyPrivate;
    QDeclarativeCustomParserPropertyPrivate *d;
};

class QDeclarativeCustomParserNodePrivate;
class Q_DECLARATIVE_EXPORT QDeclarativeCustomParserNode
{
public:
    QDeclarativeCustomParserNode();
    QDeclarativeCustomParserNode(const QDeclarativeCustomParserNode &);
    QDeclarativeCustomParserNode &operator=(const QDeclarativeCustomParserNode &);
    ~QDeclarativeCustomParserNode();

    QByteArray name() const;
    QDeclarativeParser::Location location() const;

    QList<QDeclarativeCustomParserProperty> properties() const;

private:
    friend class QDeclarativeCustomParserNodePrivate;
    QDeclarativeCustomParserNodePrivate *d;
};

class Q_DECLARATIVE_EXPORT QDeclarativeCustomParser
{
public:
    QDeclarativeCustomParser() : compiler(0), object(0) {}
    virtual ~QDeclarativeCustomParser() {}

    void clearErrors();

    virtual QByteArray compile(const QList<QDeclarativeCustomParserProperty> &)=0;
    virtual void setCustomData(QObject *, const QByteArray &)=0;

    QList<QDeclarativeError> errors() const { return exceptions; }

protected:
    void error(const QString& description);
    void error(const QDeclarativeCustomParserProperty&, const QString& description);
    void error(const QDeclarativeCustomParserNode&, const QString& description);

    int evaluateEnum(const QByteArray&) const;

private:
    QList<QDeclarativeError> exceptions;
    QDeclarativeCompiler *compiler;
    QDeclarativeParser::Object *object;
    friend class QDeclarativeCompiler;
};

#if 0
#define QML_REGISTER_CUSTOM_TYPE(URI, VERSION_MAJ, VERSION_MIN, NAME, TYPE, CUSTOMTYPE) \
            qmlRegisterCustomType<TYPE>(#URI, VERSION_MAJ, VERSION_MIN, #NAME, #TYPE, new CUSTOMTYPE)
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeclarativeCustomParserProperty)
Q_DECLARE_METATYPE(QDeclarativeCustomParserNode)

QT_END_HEADER

#endif
