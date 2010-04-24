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

#include "qdeclarativecomponent.h"
#include "private/qdeclarativecomponent_p.h"

#include "private/qdeclarativecompiler_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativecompositetypedata_p.h"
#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativevme_p.h"
#include "qdeclarative.h"
#include "qdeclarativeengine.h"
#include "private/qdeclarativebinding_p.h"
#include "private/qdeclarativebinding_p_p.h"
#include "private/qdeclarativeglobal_p.h"
#include "private/qdeclarativescriptparser_p.h"

#include <QStack>
#include <QStringList>
#include <QFileInfo>
#include <QtCore/qdebug.h>
#include <QApplication>

QT_BEGIN_NAMESPACE

class QByteArray;
int statusId = qRegisterMetaType<QDeclarativeComponent::Status>("QDeclarativeComponent::Status");

/*!
    \class QDeclarativeComponent
    \since 4.7
    \brief The QDeclarativeComponent class encapsulates a QML component description.
    \mainclass
*/

/*!
    \qmlclass Component QDeclarativeComponent
    \since 4.7
    \brief The Component element encapsulates a QML component description.

    Components are reusable, encapsulated Qml element with a well-defined interface.
    They are often defined in \l {qdeclarativedocuments.html}{Component Files}.

    The \e Component element allows defining components within a QML file.
    This can be useful for reusing a small component within a single QML
    file, or for defining a component that logically belongs with the
    file containing it.

    \qml
    Item {
        Component {
            id: redSquare
            Rectangle {
                color: "red"
                width: 10
                height: 10
            }
        }
        Loader { sourceComponent: redSquare }
        Loader { sourceComponent: redSquare; x: 20 }
    }
    \endqml
*/

/*!
    \qmlattachedsignal Component::onCompleted()

    Emitted after component "startup" has completed.  This can be used to
    execute script code at startup, once the full QML environment has been
    established.

    The \c {Component::onCompleted} attached property can be applied to
    any element.  The order of running the \c onCompleted scripts is
    undefined.

    \qml
    Rectangle {
        Component.onCompleted: console.log("Completed Running!")
        Rectangle {
            Component.onCompleted: console.log("Nested Completed Running!")
        }
    }
    \endqml
*/

/*!
    \qmlattachedsignal Component::onDestruction()

    Emitted as the component begins destruction.  This can be used to undo
    work done in the onCompleted signal, or other imperative code in your
    application.

    The \c {Component::onDestruction} attached property can be applied to
    any element.  However, it applies to the destruction of the component as
    a whole, and not the destruction of the specific object.  The order of
    running the \c onDestruction scripts is undefined.

    \qml
    Rectangle {
        Component.onDestruction: console.log("Destruction Beginning!")
        Rectangle {
            Component.onDestruction: console.log("Nested Destruction Beginning!")
        }
    }
    \endqml
*/

/*!
    \enum QDeclarativeComponent::Status
    
    Specifies the loading status of the QDeclarativeComponent.

    \value Null This QDeclarativeComponent has no data.  Call loadUrl() or setData() to add QML content.
    \value Ready This QDeclarativeComponent is ready and create() may be called.
    \value Loading This QDeclarativeComponent is loading network data.
    \value Error An error has occured.  Calling errorDescription() to retrieve a description.
*/

void QDeclarativeComponentPrivate::typeDataReady()
{
    Q_Q(QDeclarativeComponent);

    Q_ASSERT(typeData);

    fromTypeData(typeData);
    typeData = 0;

    emit q->statusChanged(q->status());
}

void QDeclarativeComponentPrivate::updateProgress(qreal p)
{
    Q_Q(QDeclarativeComponent);

    progress = p;
    emit q->progressChanged(p);
}

void QDeclarativeComponentPrivate::fromTypeData(QDeclarativeCompositeTypeData *data)
{
    url = data->imports.baseUrl();
    QDeclarativeCompiledData *c = data->toCompiledComponent(engine);

    if (!c) {
        Q_ASSERT(data->status == QDeclarativeCompositeTypeData::Error);

        state.errors = data->errors;

    } else {

        cc = c;

    }

    data->release();
}

void QDeclarativeComponentPrivate::clear()
{
    if (typeData) {
        typeData->remWaiter(this);
        typeData->release();
        typeData = 0;
    }
        
    if (cc) { 
        cc->release();
        cc = 0;
    }
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
}

/*!
    Destruct the QDeclarativeComponent.
*/
QDeclarativeComponent::~QDeclarativeComponent()
{
    Q_D(QDeclarativeComponent);

    if (d->state.completePending) {
        qWarning("QDeclarativeComponent: Component destroyed while completion pending");
        d->completeCreate();
    }

    if (d->typeData) {
        d->typeData->remWaiter(d);
        d->typeData->release();
    }
    if (d->cc)
        d->cc->release();
}

/*!
    \property QDeclarativeComponent::status
    The component's current \l{QDeclarativeComponent::Status} {status}.
 */
QDeclarativeComponent::Status QDeclarativeComponent::status() const
{
    Q_D(const QDeclarativeComponent);

    if (d->typeData)
        return Loading;
    else if (!d->state.errors.isEmpty())
        return Error;
    else if (d->engine && d->cc)
        return Ready;
    else
        return Null;
}

/*!
    \property QDeclarativeComponent::isNull

    Is true if the component is in the Null state, false otherwise.

    Equivalent to status() == QDeclarativeComponent::Null.
*/
bool QDeclarativeComponent::isNull() const
{
    return status() == Null;
}

/*!
    \property QDeclarativeComponent::isReady

    Is true if the component is in the Ready state, false otherwise.

    Equivalent to status() == QDeclarativeComponent::Ready.
*/
bool QDeclarativeComponent::isReady() const
{
    return status() == Ready;
}

/*!
    \property QDeclarativeComponent::isError

    Is true if the component is in the Error state, false otherwise.

    Equivalent to status() == QDeclarativeComponent::Error.
*/
bool QDeclarativeComponent::isError() const
{
    return status() == Error;
}

/*!
    \property QDeclarativeComponent::isLoading

    Is true if the component is in the Loading state, false otherwise.

    Equivalent to status() == QDeclarativeComponent::Loading.
*/
bool QDeclarativeComponent::isLoading() const
{
    return status() == Loading;
}

/*!
    \property QDeclarativeComponent::progress
    The progress of loading the component, from 0.0 (nothing loaded)
    to 1.0 (finished).
*/
qreal QDeclarativeComponent::progress() const
{
    Q_D(const QDeclarativeComponent);
    return d->progress;
}

/*!
    \fn void QDeclarativeComponent::progressChanged(qreal progress)

    Emitted whenever the component's loading progress changes.  \a progress will be the
    current progress between 0.0 (nothing loaded) and 1.0 (finished).
*/

/*!
    \fn void QDeclarativeComponent::statusChanged(QDeclarativeComponent::Status status)

    Emitted whenever the component's status changes.  \a status will be the
    new status.
*/

/*!
    Create a QDeclarativeComponent with no data and give it the specified
    \a engine and \a parent. Set the data with setData().
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
}

/*!
    Create a QDeclarativeComponent from the given \a url and give it the
    specified \a parent and \a engine.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    \sa loadUrl()
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, const QUrl &url, QObject *parent)
: QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    loadUrl(url);
}

/*!
    Create a QDeclarativeComponent from the given \a fileName and give it the specified 
    \a parent and \a engine.

    \sa loadUrl()
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, const QString &fileName, 
                           QObject *parent)
: QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    loadUrl(QUrl::fromLocalFile(fileName));
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, QDeclarativeCompiledData *cc, int start, int count, QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    d->cc = cc;
    cc->addref();
    d->start = start;
    d->count = count;
    d->url = cc->url;
    d->progress = 1.0;
}

/*!
    Sets the QDeclarativeComponent to use the given QML \a data.  If \a url
    is provided, it is used to set the component name and to provide
    a base path for items resolved by this component.
*/
void QDeclarativeComponent::setData(const QByteArray &data, const QUrl &url)
{
    Q_D(QDeclarativeComponent);

    d->clear();

    d->url = url;

    QDeclarativeCompositeTypeData *typeData = 
        QDeclarativeEnginePrivate::get(d->engine)->typeManager.getImmediate(data, url);
    
    if (typeData->status == QDeclarativeCompositeTypeData::Waiting
     || typeData->status == QDeclarativeCompositeTypeData::WaitingResources)
    {
        d->typeData = typeData;
        d->typeData->addWaiter(d);

    } else {

        d->fromTypeData(typeData);

    }

    d->progress = 1.0;
    emit statusChanged(status());
    emit progressChanged(d->progress);
}

/*!
Returns the QDeclarativeContext the component was created in.  This is only
valid for components created directly from QML.
*/
QDeclarativeContext *QDeclarativeComponent::creationContext() const
{
    Q_D(const QDeclarativeComponent);
    if(d->creationContext)
        return d->creationContext->asQDeclarativeContext();

    return qmlContext(this);
}

/*!
    Load the QDeclarativeComponent from the provided \a url.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.
*/
void QDeclarativeComponent::loadUrl(const QUrl &url)
{
    Q_D(QDeclarativeComponent);

    d->clear();

    if (url.isRelative() && !url.isEmpty())
        d->url = d->engine->baseUrl().resolved(url);
    else
        d->url = url;

    if (url.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(tr("Invalid empty URL"));
        d->state.errors << error;
        return;
    }

    QDeclarativeCompositeTypeData *data = 
        QDeclarativeEnginePrivate::get(d->engine)->typeManager.get(d->url);

    if (data->status == QDeclarativeCompositeTypeData::Waiting
     || data->status == QDeclarativeCompositeTypeData::WaitingResources)
    {
        d->typeData = data;
        d->typeData->addWaiter(d);
        d->progress = data->progress;
    } else {
        d->fromTypeData(data);
        d->progress = 1.0;
    }

    emit statusChanged(status());
    emit progressChanged(d->progress);
}

/*!
    Return the list of errors that occured during the last compile or create
    operation.  An empty list is returned if isError() is not set.
*/
QList<QDeclarativeError> QDeclarativeComponent::errors() const
{
    Q_D(const QDeclarativeComponent);
    if (isError())
        return d->state.errors;
    else
        return QList<QDeclarativeError>();
}

/*!
    \internal
    errorsString is only meant as a way to get the errors in script
*/
QString QDeclarativeComponent::errorsString() const
{
    Q_D(const QDeclarativeComponent);
    QString ret;
    if(!isError())
        return ret;
    foreach(const QDeclarativeError &e, d->state.errors) {
        ret += e.url().toString() + QLatin1Char(':') +
               QString::number(e.line()) + QLatin1Char(' ') +
               e.description() + QLatin1Char('\n');
    }
    return ret;
}

/*!
    \property QDeclarativeComponent::url
    The component URL.  This is the URL passed to either the constructor,
    or the loadUrl() or setData() methods.
*/
QUrl QDeclarativeComponent::url() const
{
    Q_D(const QDeclarativeComponent);
    return d->url;
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeComponentPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    \internal
    A version of create which returns a scriptObject, for use in script
*/
QScriptValue QDeclarativeComponent::createObject()
{
    Q_D(QDeclarativeComponent);
    QDeclarativeContext* ctxt = creationContext();
    if(!ctxt)
        return QScriptValue();
    QObject* ret = create(ctxt);
    if (!ret)
        return QScriptValue();
    QDeclarativeEnginePrivate *priv = QDeclarativeEnginePrivate::get(d->engine);
    QDeclarativeData::get(ret, true)->setImplicitDestructible();
    return priv->objectClass->newQObject(ret, QMetaType::QObjectStar);
}

/*!
    Create an object instance from this component.  Returns 0 if creation
    failed.  \a context specifies the context within which to create the object
    instance.  

    If \a context is 0 (the default), it will create the instance in the
    engine' s \l {QDeclarativeEngine::rootContext()}{root context}.
*/
QObject *QDeclarativeComponent::create(QDeclarativeContext *context)
{
    Q_D(QDeclarativeComponent);

    if (!context)
        context = d->engine->rootContext();

    QObject *rv = beginCreate(context);
    completeCreate();
    return rv;
}

QObject *QDeclarativeComponentPrivate::create(QDeclarativeContextData *context, 
                                              const QBitField &bindings)
{
    if (!context)
        context = QDeclarativeContextData::get(engine->rootContext());

    QObject *rv = beginCreate(context, bindings);
    completeCreate();
    return rv;
}

/*!
    This method provides more advanced control over component instance creation.
    In general, programmers should use QDeclarativeComponent::create() to create a 
    component.

    Create an object instance from this component.  Returns 0 if creation
    failed.  \a context specifies the context within which to create the object
    instance.  

    When QDeclarativeComponent constructs an instance, it occurs in three steps:
    \list 1
    \i The object hierarchy is created, and constant values are assigned.
    \i Property bindings are evaluated for the the first time.
    \i If applicable, QDeclarativeParserStatus::componentComplete() is called on objects.
    \endlist 
    QDeclarativeComponent::beginCreate() differs from QDeclarativeComponent::create() in that it
    only performs step 1.  QDeclarativeComponent::completeCreate() must be called to 
    complete steps 2 and 3.

    This breaking point is sometimes useful when using attached properties to
    communicate information to an instantiated component, as it allows their
    initial values to be configured before property bindings take effect.
*/
QObject *QDeclarativeComponent::beginCreate(QDeclarativeContext *context)
{
    Q_D(QDeclarativeComponent);
    QObject *rv = d->beginCreate(context?QDeclarativeContextData::get(context):0, QBitField());
    if (rv) {
        QDeclarativeData *ddata = QDeclarativeData::get(rv);
        Q_ASSERT(ddata);
        ddata->indestructible = true;
    }
    return rv;
}

QObject *
QDeclarativeComponentPrivate::beginCreate(QDeclarativeContextData *context, const QBitField &bindings)
{
    Q_Q(QDeclarativeComponent);
    if (!context) {
        qWarning("QDeclarativeComponent: Cannot create a component in a null context");
        return 0;
    }

    if (!context->isValid()) {
        qWarning("QDeclarativeComponent: Cannot create a component in an invalid context");
        return 0;
    }

    if (context->engine != engine) {
        qWarning("QDeclarativeComponent: Must create component in context from the same QDeclarativeEngine");
        return 0;
    }

    if (state.completePending) {
        qWarning("QDeclarativeComponent: Cannot create new component instance before completing the previous");
        return 0;
    }

    if (!q->isReady()) {
        qWarning("QDeclarativeComponent: Component is not ready");
        return 0;
    }

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

    QDeclarativeContextData *ctxt = new QDeclarativeContextData;
    ctxt->isInternal = true;
    ctxt->url = cc->url;
    ctxt->imports = cc->importCache;

    // Nested global imports
    if (creationContext && start != -1) 
        ctxt->importedScripts = creationContext->importedScripts;

    cc->importCache->addref();
    ctxt->setParent(context);

    QObject *rv = begin(ctxt, ep, cc, start, count, &state, bindings);

    if (rv && !context->isInternal && ep->isDebugging)
        context->asQDeclarativeContextPrivate()->instances.append(rv);

    return rv;
}

QObject * QDeclarativeComponentPrivate::begin(QDeclarativeContextData *ctxt, QDeclarativeEnginePrivate *enginePriv,
                                              QDeclarativeCompiledData *component, int start, int count,
                                              ConstructionState *state, const QBitField &bindings)
{
    bool isRoot = !enginePriv->inBeginCreate;
    enginePriv->inBeginCreate = true;

    QDeclarativeVME vme;
    QObject *rv = vme.run(ctxt, component, start, count, bindings);

    if (vme.isError()) 
        state->errors = vme.errors();

    if (isRoot) {
        enginePriv->inBeginCreate = false;

        state->bindValues = enginePriv->bindValues;
        state->parserStatus = enginePriv->parserStatus;
        state->componentAttached = enginePriv->componentAttached;
        if (state->componentAttached)
            state->componentAttached->prev = &state->componentAttached;

        enginePriv->componentAttached = 0;
        enginePriv->bindValues.clear();
        enginePriv->parserStatus.clear();
        state->completePending = true;
        enginePriv->inProgressCreations++;
    }

    return rv;
}

void QDeclarativeComponentPrivate::beginDeferred(QDeclarativeEnginePrivate *enginePriv,
                                                 QObject *object, ConstructionState *state)
{
    bool isRoot = !enginePriv->inBeginCreate;
    enginePriv->inBeginCreate = true;

    QDeclarativeVME vme;
    vme.runDeferred(object);

    if (vme.isError()) 
        state->errors = vme.errors();

    if (isRoot) {
        enginePriv->inBeginCreate = false;

        state->bindValues = enginePriv->bindValues;
        state->parserStatus = enginePriv->parserStatus;
        state->componentAttached = enginePriv->componentAttached;
        if (state->componentAttached)
            state->componentAttached->prev = &state->componentAttached;

        enginePriv->componentAttached = 0;
        enginePriv->bindValues.clear();
        enginePriv->parserStatus.clear();
        state->completePending = true;
        enginePriv->inProgressCreations++;
    }
}

void QDeclarativeComponentPrivate::complete(QDeclarativeEnginePrivate *enginePriv, ConstructionState *state)
{
    if (state->completePending) {

        for (int ii = 0; ii < state->bindValues.count(); ++ii) {
            QDeclarativeEnginePrivate::SimpleList<QDeclarativeAbstractBinding> bv = 
                state->bindValues.at(ii);
            for (int jj = 0; jj < bv.count; ++jj) {
                if(bv.at(jj)) 
                    bv.at(jj)->setEnabled(true, QDeclarativePropertyPrivate::BypassInterceptor | 
                                                QDeclarativePropertyPrivate::DontRemoveBinding);
            }
            QDeclarativeEnginePrivate::clear(bv);
        }

        for (int ii = 0; ii < state->parserStatus.count(); ++ii) {
            QDeclarativeEnginePrivate::SimpleList<QDeclarativeParserStatus> ps = 
                state->parserStatus.at(ii);

            for (int jj = ps.count - 1; jj >= 0; --jj) {
                QDeclarativeParserStatus *status = ps.at(jj);
                if (status && status->d) {
                    status->d = 0;
                    status->componentComplete();
                }
            }
            QDeclarativeEnginePrivate::clear(ps);
        }

        while (state->componentAttached) {
            QDeclarativeComponentAttached *a = state->componentAttached;
            a->rem();
            QDeclarativeData *d = QDeclarativeData::get(a->parent());
            Q_ASSERT(d);
            Q_ASSERT(d->context);
            a->add(&d->context->componentAttached);
            emit a->completed();
        }

        state->bindValues.clear();
        state->parserStatus.clear();
        state->completePending = false;

        enginePriv->inProgressCreations--;
        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings) {
                enginePriv->warning(enginePriv->erroredBindings->error);
                enginePriv->erroredBindings->removeError();
            }
        }
    }
}

/*!
    This method provides more advanced control over component instance creation.
    In general, programmers should use QDeclarativeComponent::create() to create a 
    component.

    Complete a component creation begin with QDeclarativeComponent::beginCreate().
*/
void QDeclarativeComponent::completeCreate()
{
    Q_D(QDeclarativeComponent);
    d->completeCreate();
}

void QDeclarativeComponentPrivate::completeCreate()
{
    if (state.completePending) {
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
        complete(ep, &state);
    }
}

QDeclarativeComponentAttached::QDeclarativeComponentAttached(QObject *parent)
: QObject(parent), prev(0), next(0)
{
}

QDeclarativeComponentAttached::~QDeclarativeComponentAttached()
{
    if (prev) *prev = next;
    if (next) next->prev = prev;
    prev = 0;
    next = 0;
}

/*!
    \internal
*/
QDeclarativeComponentAttached *QDeclarativeComponent::qmlAttachedProperties(QObject *obj)
{
    QDeclarativeComponentAttached *a = new QDeclarativeComponentAttached(obj);

    QDeclarativeEngine *engine = qmlEngine(obj);
    if (!engine)
        return a;

    if (QDeclarativeEnginePrivate::get(engine)->inBeginCreate) {
        QDeclarativeEnginePrivate *p = QDeclarativeEnginePrivate::get(engine);
        a->add(&p->componentAttached);
    } else {
        QDeclarativeData *d = QDeclarativeData::get(obj);
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
    }

    return a;
}

QT_END_NAMESPACE
