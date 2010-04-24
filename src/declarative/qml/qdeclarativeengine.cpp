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

#include "private/qdeclarativeengine_p.h"
#include "qdeclarativeengine.h"

#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativecompiler_p.h"
#include "private/qdeclarativeglobalscriptclass_p.h"
#include "qdeclarative.h"
#include "qdeclarativecontext.h"
#include "qdeclarativeexpression.h"
#include "qdeclarativecomponent.h"
#include "private/qdeclarativebinding_p_p.h"
#include "private/qdeclarativevme_p.h"
#include "private/qdeclarativeenginedebug_p.h"
#include "private/qdeclarativestringconverters_p.h"
#include "private/qdeclarativexmlhttprequest_p.h"
#include "private/qdeclarativesqldatabase_p.h"
#include "private/qdeclarativetypenamescriptclass_p.h"
#include "private/qdeclarativelistscriptclass_p.h"
#include "qdeclarativescriptstring.h"
#include "private/qdeclarativeglobal_p.h"
#include "private/qdeclarativeworkerscript_p.h"
#include "private/qdeclarativecomponent_p.h"
#include "qdeclarativenetworkaccessmanagerfactory.h"
#include "qdeclarativeimageprovider.h"
#include "private/qdeclarativedirparser_p.h"
#include "qdeclarativeextensioninterface.h"
#include "private/qdeclarativelist_p.h"
#include "private/qdeclarativetypenamecache_p.h"

#include <QtCore/qmetaobject.h>
#include <QScriptClass>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QMetaObject>
#include <QStack>
#include <QMap>
#include <QPluginLoader>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtGui/qcolor.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qsound.h>
#include <QGraphicsObject>
#include <QtCore/qcryptographichash.h>

#include <private/qobject_p.h>
#include <private/qscriptdeclarativeclass_p.h>

#include <private/qdeclarativeitemsmodule_p.h>
#include <private/qdeclarativeutilmodule_p.h>

#ifdef Q_OS_WIN // for %APPDATA%
#include <qt_windows.h>
#include <qlibrary.h>

#define CSIDL_APPDATA		0x001a	// <username>\Application Data
#endif

Q_DECLARE_METATYPE(QDeclarativeProperty)

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlImportTrace, QML_IMPORT_TRACE)

/*!
  \qmlclass QtObject QObject
  \since 4.7
  \brief The QtObject element is the most basic element in QML

  The QtObject element is a non-visual element which contains only the
  objectName property. It is useful for when you need an extremely
  lightweight element to place your own custom properties in.

  It can also be useful for C++ integration, as it is just a plain
  QObject. See the QObject documentation for further details.
*/
/*!
  \qmlproperty string QtObject::objectName
  This property allows you to give a name to this specific object instance.

  See \l{scripting.html#accessing-child-qobjects}{Accessing Child QObjects}
  in the scripting documentation for details how objectName can be used from
  scripts.
*/

struct StaticQtMetaObject : public QObject
{
    static const QMetaObject *get()
        { return &static_cast<StaticQtMetaObject*> (0)->staticQtMetaObject; }
};

static bool qt_QmlQtModule_registered = false;

void QDeclarativeEnginePrivate::defineModule()
{
    qmlRegisterType<QDeclarativeComponent>("Qt",4,7,"Component");
    qmlRegisterType<QObject>("Qt",4,7,"QtObject");
    qmlRegisterType<QDeclarativeWorkerScript>("Qt",4,7,"WorkerScript");

    qmlRegisterType<QDeclarativeBinding>();
}

QDeclarativeEnginePrivate::QDeclarativeEnginePrivate(QDeclarativeEngine *e)
: captureProperties(false), rootContext(0), currentExpression(0), isDebugging(false),
  outputWarningsToStdErr(true), contextClass(0), sharedContext(0), sharedScope(0),
  objectClass(0), valueTypeClass(0), globalClass(0), cleanup(0), erroredBindings(0),
  inProgressCreations(0), scriptEngine(this), workerScriptEngine(0), componentAttached(0),
  inBeginCreate(false), networkAccessManager(0), networkAccessManagerFactory(0),
  typeManager(e), uniqueId(1)
{
    if (!qt_QmlQtModule_registered) {
        qt_QmlQtModule_registered = true;
        QDeclarativeItemModule::defineModule();
        QDeclarativeUtilModule::defineModule();
        QDeclarativeEnginePrivate::defineModule();
        QDeclarativeValueTypeFactory::registerValueTypes();
    }
    globalClass = new QDeclarativeGlobalScriptClass(&scriptEngine);

    // env import paths
    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");
    if (!envImportPath.isEmpty()) {
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
        QLatin1Char pathSep(';');
#else
        QLatin1Char pathSep(':');
#endif
        foreach (const QString &path, QString::fromLatin1(envImportPath).split(pathSep, QString::SkipEmptyParts)) {
            QString canonicalPath = QDir(path).canonicalPath();
            if (!canonicalPath.isEmpty() && !fileImportPath.contains(canonicalPath))
                fileImportPath.append(canonicalPath);
        }
    }
    QString builtinPath = QLibraryInfo::location(QLibraryInfo::ImportsPath);
    if (!builtinPath.isEmpty())
        fileImportPath += builtinPath;

    filePluginPath += QLatin1String(".");

}

QUrl QDeclarativeScriptEngine::resolvedUrl(QScriptContext *context, const QUrl& url)
{
    if (p) {
        QDeclarativeContextData *ctxt = QDeclarativeEnginePrivate::get(this)->getContext(context);
        Q_ASSERT(ctxt);
        return ctxt->resolvedUrl(url);
    }
    return baseUrl.resolved(url);
}

QDeclarativeScriptEngine::QDeclarativeScriptEngine(QDeclarativeEnginePrivate *priv)
: p(priv), sqlQueryClass(0), namedNodeMapClass(0), nodeListClass(0)
{
    // Note that all documentation for stuff put on the global object goes in
    // doc/src/declarative/globalobject.qdoc

    bool mainthread = priv != 0;

    QScriptValue qtObject =
        newQMetaObject(StaticQtMetaObject::get());
    globalObject().setProperty(QLatin1String("Qt"), qtObject);

#ifndef QT_NO_DESKTOPSERVICES
    offlineStoragePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation).replace(QLatin1Char('/'), QDir::separator())
        + QDir::separator() + QLatin1String("QML")
        + QDir::separator() + QLatin1String("OfflineStorage");
#endif

    qt_add_qmlxmlhttprequest(this);
    qt_add_qmlsqldatabase(this);
    // XXX A Multimedia "Qt.Sound" class also needs to be made available,
    // XXX but we don't want a dependency in that cirection.
    // XXX When the above a done some better way, that way should also be
    // XXX used to add Qt.Sound class.

    //types
    qtObject.setProperty(QLatin1String("isQtObject"), newFunction(QDeclarativeEnginePrivate::isQtObject, 1));
    qtObject.setProperty(QLatin1String("rgba"), newFunction(QDeclarativeEnginePrivate::rgba, 4));
    qtObject.setProperty(QLatin1String("hsla"), newFunction(QDeclarativeEnginePrivate::hsla, 4));
    qtObject.setProperty(QLatin1String("rect"), newFunction(QDeclarativeEnginePrivate::rect, 4));
    qtObject.setProperty(QLatin1String("point"), newFunction(QDeclarativeEnginePrivate::point, 2));
    qtObject.setProperty(QLatin1String("size"), newFunction(QDeclarativeEnginePrivate::size, 2));
    qtObject.setProperty(QLatin1String("vector3d"), newFunction(QDeclarativeEnginePrivate::vector, 3));

    if (mainthread) {
        //color helpers
        qtObject.setProperty(QLatin1String("lighter"), newFunction(QDeclarativeEnginePrivate::lighter, 1));
        qtObject.setProperty(QLatin1String("darker"), newFunction(QDeclarativeEnginePrivate::darker, 1));
        qtObject.setProperty(QLatin1String("tint"), newFunction(QDeclarativeEnginePrivate::tint, 2));
    }

    //date/time formatting
    qtObject.setProperty(QLatin1String("formatDate"),newFunction(QDeclarativeEnginePrivate::formatDate, 2));
    qtObject.setProperty(QLatin1String("formatTime"),newFunction(QDeclarativeEnginePrivate::formatTime, 2));
    qtObject.setProperty(QLatin1String("formatDateTime"),newFunction(QDeclarativeEnginePrivate::formatDateTime, 2));

    //misc methods
    qtObject.setProperty(QLatin1String("openUrlExternally"),newFunction(QDeclarativeEnginePrivate::desktopOpenUrl, 1));
    qtObject.setProperty(QLatin1String("md5"),newFunction(QDeclarativeEnginePrivate::md5, 1));
    qtObject.setProperty(QLatin1String("btoa"),newFunction(QDeclarativeEnginePrivate::btoa, 1));
    qtObject.setProperty(QLatin1String("atob"),newFunction(QDeclarativeEnginePrivate::atob, 1));
    qtObject.setProperty(QLatin1String("quit"), newFunction(QDeclarativeEnginePrivate::quit, 0));
    qtObject.setProperty(QLatin1String("resolvedUrl"),newFunction(QDeclarativeScriptEngine::resolvedUrl, 1));

    if (mainthread) {
        qtObject.setProperty(QLatin1String("createQmlObject"),
                newFunction(QDeclarativeEnginePrivate::createQmlObject, 1));
        qtObject.setProperty(QLatin1String("createComponent"),
                newFunction(QDeclarativeEnginePrivate::createComponent, 1));
    }

    //firebug/webkit compat
    QScriptValue consoleObject = newObject();
    consoleObject.setProperty(QLatin1String("log"),newFunction(QDeclarativeEnginePrivate::consoleLog, 1));
    consoleObject.setProperty(QLatin1String("debug"),newFunction(QDeclarativeEnginePrivate::consoleLog, 1));
    globalObject().setProperty(QLatin1String("console"), consoleObject);

    // translation functions need to be installed
    // before the global script class is constructed (QTBUG-6437)
    installTranslatorFunctions();
}

QDeclarativeScriptEngine::~QDeclarativeScriptEngine()
{
    delete sqlQueryClass;
    delete nodeListClass;
    delete namedNodeMapClass;
}

QScriptValue QDeclarativeScriptEngine::resolvedUrl(QScriptContext *ctxt, QScriptEngine *engine)
{
    QString arg = ctxt->argument(0).toString();
    QUrl r = QDeclarativeScriptEngine::get(engine)->resolvedUrl(ctxt,QUrl(arg));
    return QScriptValue(r.toString());
}

QNetworkAccessManager *QDeclarativeScriptEngine::networkAccessManager()
{
    return p->getNetworkAccessManager();
}

QDeclarativeEnginePrivate::~QDeclarativeEnginePrivate()
{
    while (cleanup) {
        QDeclarativeCleanup *c = cleanup;
        cleanup = c->next;
        if (cleanup) cleanup->prev = &cleanup;
        c->next = 0;
        c->prev = 0;
        c->clear();
    }

    delete rootContext;
    rootContext = 0;
    delete contextClass;
    contextClass = 0;
    delete objectClass;
    objectClass = 0;
    delete valueTypeClass;
    valueTypeClass = 0;
    delete typeNameClass;
    typeNameClass = 0;
    delete listClass;
    listClass = 0;
    delete globalClass;
    globalClass = 0;

    for(int ii = 0; ii < bindValues.count(); ++ii)
        clear(bindValues[ii]);
    for(int ii = 0; ii < parserStatus.count(); ++ii)
        clear(parserStatus[ii]);
    for(QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.constBegin(); iter != m_compositeTypes.constEnd(); ++iter)
        (*iter)->release();
    for(QHash<const QMetaObject *, QDeclarativePropertyCache *>::Iterator iter = propertyCache.begin(); iter != propertyCache.end(); ++iter)
        (*iter)->release();

}

void QDeclarativeEnginePrivate::clear(SimpleList<QDeclarativeAbstractBinding> &bvs)
{
    bvs.clear();
}

void QDeclarativeEnginePrivate::clear(SimpleList<QDeclarativeParserStatus> &pss)
{
    for (int ii = 0; ii < pss.count; ++ii) {
        QDeclarativeParserStatus *ps = pss.at(ii);
        if(ps)
            ps->d = 0;
    }
    pss.clear();
}

Q_GLOBAL_STATIC(QDeclarativeEngineDebugServer, qmlEngineDebugServer);
typedef QMap<QString, QString> StringStringMap;
Q_GLOBAL_STATIC(StringStringMap, qmlEnginePluginsWithRegisteredTypes); // stores the uri


void QDeclarativePrivate::qdeclarativeelement_destructor(QObject *o)
{
    QObjectPrivate *p = QObjectPrivate::get(o);
    Q_ASSERT(p->declarativeData);
    QDeclarativeData *d = static_cast<QDeclarativeData*>(p->declarativeData);
    if (d->ownContext)
        d->context->destroy();
}

void QDeclarativeData::destroyed(QAbstractDeclarativeData *d, QObject *o)
{
    static_cast<QDeclarativeData *>(d)->destroyed(o);
}

void QDeclarativeData::parentChanged(QAbstractDeclarativeData *d, QObject *o, QObject *p)
{
    static_cast<QDeclarativeData *>(d)->parentChanged(o, p);
}

void QDeclarativeEnginePrivate::init()
{
    Q_Q(QDeclarativeEngine);
    qRegisterMetaType<QVariant>("QVariant");
    qRegisterMetaType<QDeclarativeScriptString>("QDeclarativeScriptString");
    qRegisterMetaType<QScriptValue>("QScriptValue");

    QDeclarativeData::init();

    contextClass = new QDeclarativeContextScriptClass(q);
    objectClass = new QDeclarativeObjectScriptClass(q);
    valueTypeClass = new QDeclarativeValueTypeScriptClass(q);
    typeNameClass = new QDeclarativeTypeNameScriptClass(q);
    listClass = new QDeclarativeListScriptClass(q);
    rootContext = new QDeclarativeContext(q,true);

    if (QCoreApplication::instance()->thread() == q->thread() &&
        QDeclarativeEngineDebugServer::isDebuggingEnabled()) {
        qmlEngineDebugServer();
        isDebugging = true;
        QDeclarativeEngineDebugServer::addEngine(q);
    }
}

QDeclarativeWorkerScriptEngine *QDeclarativeEnginePrivate::getWorkerScriptEngine()
{
    Q_Q(QDeclarativeEngine);
    if (!workerScriptEngine)
        workerScriptEngine = new QDeclarativeWorkerScriptEngine(q);
    return workerScriptEngine;
}

/*!
  \class QDeclarativeEngine
  \since 4.7
  \brief The QDeclarativeEngine class provides an environment for instantiating QML components.
  \mainclass

  Each QML component is instantiated in a QDeclarativeContext.
  QDeclarativeContext's are essential for passing data to QML
  components.  In QML, contexts are arranged hierarchically and this
  hierarchy is managed by the QDeclarativeEngine.

  Prior to creating any QML components, an application must have
  created a QDeclarativeEngine to gain access to a QML context.  The
  following example shows how to create a simple Text item.

  \code
  QDeclarativeEngine engine;
  QDeclarativeComponent component(&engine);
  component.setData("import Qt 4.7\nText { text: \"Hello world!\" }", QUrl());
  QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(component.create());

  //add item to view, etc
  ...
  \endcode

  In this case, the Text item will be created in the engine's
  \l {QDeclarativeEngine::rootContext()}{root context}.

  \sa QDeclarativeComponent QDeclarativeContext
*/

/*!
  Create a new QDeclarativeEngine with the given \a parent.
*/
QDeclarativeEngine::QDeclarativeEngine(QObject *parent)
: QObject(*new QDeclarativeEnginePrivate(this), parent)
{
    Q_D(QDeclarativeEngine);
    d->init();
}

/*!
  Destroys the QDeclarativeEngine.

  Any QDeclarativeContext's created on this engine will be
  invalidated, but not destroyed (unless they are parented to the
  QDeclarativeEngine object).
*/
QDeclarativeEngine::~QDeclarativeEngine()
{
    Q_D(QDeclarativeEngine);
    if (d->isDebugging)
        QDeclarativeEngineDebugServer::remEngine(this);
}

/*! \fn void QDeclarativeEngine::quit()
  This signal is emitted when the QDeclarativeEngine quits.
 */

/*!
  Clears the engine's internal component cache.

  Normally the QDeclarativeEngine caches components loaded from qml
  files.  This method clears this cache and forces the component to be
  reloaded.
 */
void QDeclarativeEngine::clearComponentCache()
{
    Q_D(QDeclarativeEngine);
    d->typeManager.clearCache();
}

/*!
  Returns the engine's root context.

  The root context is automatically created by the QDeclarativeEngine.
  Data that should be available to all QML component instances
  instantiated by the engine should be put in the root context.

  Additional data that should only be available to a subset of
  component instances should be added to sub-contexts parented to the
  root context.
*/
QDeclarativeContext *QDeclarativeEngine::rootContext()
{
    Q_D(QDeclarativeEngine);
    return d->rootContext;
}

/*!
  Sets the \a factory to use for creating QNetworkAccessManager(s).

  QNetworkAccessManager is used for all network access by QML.  By
  implementing a factory it is possible to create custom
  QNetworkAccessManager with specialized caching, proxy and cookie
  support.

  The factory must be set before exceuting the engine.
*/
void QDeclarativeEngine::setNetworkAccessManagerFactory(QDeclarativeNetworkAccessManagerFactory *factory)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    d->networkAccessManagerFactory = factory;
}

/*!
  Returns the current QDeclarativeNetworkAccessManagerFactory.

  \sa setNetworkAccessManagerFactory()
*/
QDeclarativeNetworkAccessManagerFactory *QDeclarativeEngine::networkAccessManagerFactory() const
{
    Q_D(const QDeclarativeEngine);
    return d->networkAccessManagerFactory;
}

QNetworkAccessManager *QDeclarativeEnginePrivate::createNetworkAccessManager(QObject *parent) const
{
    QMutexLocker locker(&mutex);
    QNetworkAccessManager *nam;
    if (networkAccessManagerFactory) {
        nam = networkAccessManagerFactory->create(parent);
    } else {
        nam = new QNetworkAccessManager(parent);
    }

    return nam;
}

QNetworkAccessManager *QDeclarativeEnginePrivate::getNetworkAccessManager() const
{
    Q_Q(const QDeclarativeEngine);
    if (!networkAccessManager)
        networkAccessManager = createNetworkAccessManager(const_cast<QDeclarativeEngine*>(q));
    return networkAccessManager;
}

/*!
  Returns a common QNetworkAccessManager which can be used by any QML
  element instantiated by this engine.

  If a QDeclarativeNetworkAccessManagerFactory has been set and a
  QNetworkAccessManager has not yet been created, the
  QDeclarativeNetworkAccessManagerFactory will be used to create the
  QNetworkAccessManager; otherwise the returned QNetworkAccessManager
  will have no proxy or cache set.

  \sa setNetworkAccessManagerFactory()
*/
QNetworkAccessManager *QDeclarativeEngine::networkAccessManager() const
{
    Q_D(const QDeclarativeEngine);
    return d->getNetworkAccessManager();
}

/*!

  Sets the \a provider to use for images requested via the \e
  image: url scheme, with host \a providerId.

  QDeclarativeImageProvider allows images to be provided to QML
  asynchronously.  The image request will be run in a low priority
  thread.  This allows potentially costly image loading to be done in
  the background, without affecting the performance of the UI.

  Note that images loaded from a QDeclarativeImageProvider are cached
  by QPixmapCache, similar to any image loaded by QML.

  The QDeclarativeEngine assumes ownership of the provider.

  This example creates a provider with id \e colors:

  \snippet examples/declarative/imageprovider/imageprovider.cpp 0

  \snippet examples/declarative/imageprovider/imageprovider-example.qml 0

  \sa removeImageProvider()
*/
void QDeclarativeEngine::addImageProvider(const QString &providerId, QDeclarativeImageProvider *provider)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.insert(providerId, provider);
}

/*!
  Returns the QDeclarativeImageProvider set for \a providerId.
*/
QDeclarativeImageProvider *QDeclarativeEngine::imageProvider(const QString &providerId) const
{
    Q_D(const QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    return d->imageProviders.value(providerId);
}

/*!
  Removes the QDeclarativeImageProvider for \a providerId.

  Returns the provider if it was found; otherwise returns 0.

  \sa addImageProvider()
*/
void QDeclarativeEngine::removeImageProvider(const QString &providerId)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    delete d->imageProviders.take(providerId);
}

QImage QDeclarativeEnginePrivate::getImageFromProvider(const QUrl &url, QSize *size, const QSize& req_size)
{
    QMutexLocker locker(&mutex);
    QImage image;
    QDeclarativeImageProvider *provider = imageProviders.value(url.host());
    if (provider)
        image = provider->request(url.path().mid(1), size, req_size);
    return image;
}

/*!
  Return the base URL for this engine.  The base URL is only used to
  resolve components when a relative URL is passed to the
  QDeclarativeComponent constructor.

  If a base URL has not been explicitly set, this method returns the
  application's current working directory.

  \sa setBaseUrl()
*/
QUrl QDeclarativeEngine::baseUrl() const
{
    Q_D(const QDeclarativeEngine);
    if (d->baseUrl.isEmpty()) {
        return QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    } else {
        return d->baseUrl;
    }
}

/*!
  Set the  base URL for this engine to \a url.

  \sa baseUrl()
*/
void QDeclarativeEngine::setBaseUrl(const QUrl &url)
{
    Q_D(QDeclarativeEngine);
    d->baseUrl = url;
}

/*!
  Returns true if warning messages will be output to stderr in addition
  to being emitted by the warnings() signal, otherwise false.

  The default value is true.
*/
bool QDeclarativeEngine::outputWarningsToStandardError() const
{
    Q_D(const QDeclarativeEngine);
    return d->outputWarningsToStdErr;
}

/*!
  Set whether warning messages will be output to stderr to \a enabled.

  If \a enabled is true, any warning messages generated by QML will be
  output to stderr and emitted by the warnings() signal.  If \a enabled
  is false, on the warnings() signal will be emitted.  This allows
  applications to handle warning output themselves.

  The default value is true.
*/
void QDeclarativeEngine::setOutputWarningsToStandardError(bool enabled)
{
    Q_D(QDeclarativeEngine);
    d->outputWarningsToStdErr = enabled;
}

/*!
  Returns the QDeclarativeContext for the \a object, or 0 if no
  context has been set.

  When the QDeclarativeEngine instantiates a QObject, the context is
  set automatically.
  */
QDeclarativeContext *QDeclarativeEngine::contextForObject(const QObject *object)
{
    if(!object)
        return 0;

    QObjectPrivate *priv = QObjectPrivate::get(const_cast<QObject *>(object));

    QDeclarativeData *data =
        static_cast<QDeclarativeData *>(priv->declarativeData);

    if (!data)
        return 0;
    else if (data->outerContext)
        return data->outerContext->asQDeclarativeContext();
    else
        return 0;
}

/*!
  Sets the QDeclarativeContext for the \a object to \a context.
  If the \a object already has a context, a warning is
  output, but the context is not changed.

  When the QDeclarativeEngine instantiates a QObject, the context is
  set automatically.
 */
void QDeclarativeEngine::setContextForObject(QObject *object, QDeclarativeContext *context)
{
    if (!object || !context)
        return;

    QDeclarativeData *data = QDeclarativeData::get(object, true);
    if (data->context) {
        qWarning("QDeclarativeEngine::setContextForObject(): Object already has a QDeclarativeContext");
        return;
    }

    QDeclarativeContextData *contextData = QDeclarativeContextData::get(context);
    contextData->addObject(object);
}

/*!
  \enum QDeclarativeEngine::ObjectOwnership

  Ownership controls whether or not QML automatically destroys the
  QObject when the object is garbage collected by the JavaScript
  engine.  The two ownership options are:

  \value CppOwnership The object is owned by C++ code, and will
  never be deleted by QML.  The JavaScript destroy() method cannot be
  used on objects with CppOwnership.  This option is similar to
  QScriptEngine::QtOwnership.

  \value JavaScriptOwnership The object is owned by JavaScript.
  When the object is returned to QML as the return value of a method
  call or property access, QML will delete the object if there are no
  remaining JavaScript references to it and it has no
  QObject::parent().  This option is similar to
  QScriptEngine::ScriptOwnership.

  Generally an application doesn't need to set an object's ownership
  explicitly.  QML uses a heuristic to set the default object
  ownership.  By default, an object that is created by QML has
  JavaScriptOwnership.  The exception to this are the root objects
  created by calling QDeclarativeCompnent::create() or
  QDeclarativeComponent::beginCreate() which have CppOwnership by
  default.  The ownership of these root-level objects is considered to
  have been transfered to the C++ caller.

  Objects not-created by QML have CppOwnership by default.  The
  exception to this is objects returned from a C++ method call.  The
  ownership of these objects is passed to JavaScript.

  Calling setObjectOwnership() overrides the default ownership
  heuristic used by QML.
*/

/*!
  Sets the \a ownership of \a object.
*/
void QDeclarativeEngine::setObjectOwnership(QObject *object, ObjectOwnership ownership)
{
    if (!object)
        return;

    QDeclarativeData *ddata = QDeclarativeData::get(object, true);
    if (!ddata)
        return;

    ddata->indestructible = (ownership == CppOwnership)?true:false;
    ddata->explicitIndestructibleSet = true;
}

/*!
  Returns the ownership of \a object.
*/
QDeclarativeEngine::ObjectOwnership QDeclarativeEngine::objectOwnership(QObject *object)
{
    if (!object)
        return CppOwnership;

    QDeclarativeData *ddata = QDeclarativeData::get(object, false);
    if (!ddata)
        return CppOwnership;
    else
        return ddata->indestructible?CppOwnership:JavaScriptOwnership;
}

void qmlExecuteDeferred(QObject *object)
{
    QDeclarativeData *data = QDeclarativeData::get(object);

    if (data && data->deferredComponent) {

        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(data->context->engine);

        QDeclarativeComponentPrivate::ConstructionState state;
        QDeclarativeComponentPrivate::beginDeferred(ep, object, &state);

        data->deferredComponent->release();
        data->deferredComponent = 0;

        QDeclarativeComponentPrivate::complete(ep, &state);

        if (!state.errors.isEmpty())
            ep->warning(state.errors);
    }
}

QDeclarativeContext *qmlContext(const QObject *obj)
{
    return QDeclarativeEngine::contextForObject(obj);
}

QDeclarativeEngine *qmlEngine(const QObject *obj)
{
    QDeclarativeContext *context = QDeclarativeEngine::contextForObject(obj);
    return context?context->engine():0;
}

QObject *qmlAttachedPropertiesObjectById(int id, const QObject *object, bool create)
{
    QDeclarativeData *data = QDeclarativeData::get(object);
    if (!data)
        return 0; // Attached properties are only on objects created by QML

    QObject *rv = data->attachedProperties?data->attachedProperties->value(id):0;
    if (rv || !create)
        return rv;

    QDeclarativeAttachedPropertiesFunc pf = QDeclarativeMetaType::attachedPropertiesFuncById(id);
    if (!pf)
        return 0;

    rv = pf(const_cast<QObject *>(object));

    if (rv) {
        if (!data->attachedProperties)
            data->attachedProperties = new QHash<int, QObject *>();
        data->attachedProperties->insert(id, rv);
    }

    return rv;
}

QObject *qmlAttachedPropertiesObject(int *idCache, const QObject *object,
                                     const QMetaObject *attachedMetaObject, bool create)
{
    if (*idCache == -1)
        *idCache = QDeclarativeMetaType::attachedPropertiesFuncId(attachedMetaObject);

    if (*idCache == -1 || !object)
        return 0;

    return qmlAttachedPropertiesObjectById(*idCache, object, create);
}

void QDeclarativeData::destroyed(QObject *object)
{
    if (deferredComponent)
        deferredComponent->release();
    if (attachedProperties)
        delete attachedProperties;

    if (nextContextObject)
        nextContextObject->prevContextObject = prevContextObject;
    if (prevContextObject)
        *prevContextObject = nextContextObject;

    QDeclarativeAbstractBinding *binding = bindings;
    while (binding) {
        QDeclarativeAbstractBinding *next = binding->m_nextBinding;
        binding->m_prevBinding = 0;
        binding->m_nextBinding = 0;
        binding->destroy();
        binding = next;
    }

    if (bindingBits)
        free(bindingBits);

    if (propertyCache)
        propertyCache->release();

    if (ownContext && context)
        context->destroy();

    QDeclarativeGuard<QObject> *guard = guards;
    while (guard) {
        QDeclarativeGuard<QObject> *g = guard;
        guard = guard->next;
        g->o = 0;
        g->prev = 0;
        g->next = 0;
        g->objectDestroyed(object);
    }

    if (scriptValue)
        delete scriptValue;

    if (ownMemory)
        delete this;
}

void QDeclarativeData::parentChanged(QObject *, QObject *parent)
{
    if (!parent && scriptValue) { delete scriptValue; scriptValue = 0; }
}

bool QDeclarativeData::hasBindingBit(int bit) const
{
    if (bindingBitsSize > bit)
        return bindingBits[bit / 32] & (1 << (bit % 32));
    else
        return false;
}

void QDeclarativeData::clearBindingBit(int bit)
{
    if (bindingBitsSize > bit)
        bindingBits[bit / 32] &= ~(1 << (bit % 32));
}

void QDeclarativeData::setBindingBit(QObject *obj, int bit)
{
    if (bindingBitsSize <= bit) {
        int props = obj->metaObject()->propertyCount();
        Q_ASSERT(bit < props);

        int arraySize = (props + 31) / 32;
        int oldArraySize = bindingBitsSize / 32;

        bindingBits = (quint32 *)realloc(bindingBits,
                                         arraySize * sizeof(quint32));

        memset(bindingBits + oldArraySize,
               0x00,
               sizeof(quint32) * (arraySize - oldArraySize));

        bindingBitsSize = arraySize * 32;
    }

    bindingBits[bit / 32] |= (1 << (bit % 32));
}

/*!
    Creates a QScriptValue allowing you to use \a object in QML script.
    \a engine is the QDeclarativeEngine it is to be created in.

    The QScriptValue returned is a QtScript Object, not a QtScript QObject, due
    to the special needs of QML requiring more functionality than a standard
    QtScript QObject.
*/
QScriptValue QDeclarativeEnginePrivate::qmlScriptObject(QObject* object,
                                               QDeclarativeEngine* engine)
{
    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(engine);
    return enginePriv->objectClass->newQObject(object);
}

/*!
    Returns the QDeclarativeContext for the executing QScript \a ctxt.
*/
QDeclarativeContextData *QDeclarativeEnginePrivate::getContext(QScriptContext *ctxt)
{
    QScriptValue scopeNode = QScriptDeclarativeClass::scopeChainValue(ctxt, -3);
    Q_ASSERT(scopeNode.isValid());
    Q_ASSERT(QScriptDeclarativeClass::scriptClass(scopeNode) == contextClass);
    return contextClass->contextFromValue(scopeNode);
}

QScriptValue QDeclarativeEnginePrivate::createComponent(QScriptContext *ctxt, QScriptEngine *engine)
{
    QDeclarativeEnginePrivate *activeEnginePriv =
        static_cast<QDeclarativeScriptEngine*>(engine)->p;
    QDeclarativeEngine* activeEngine = activeEnginePriv->q_func();

    QDeclarativeContextData* context = activeEnginePriv->getContext(ctxt);
    Q_ASSERT(context);

    if(ctxt->argumentCount() != 1) {
        return ctxt->throwError("Qt.createComponent(): Invalid arguments");
    }else{
        QString arg = ctxt->argument(0).toString();
        if (arg.isEmpty())
            return engine->nullValue();
        QUrl url = QUrl(context->resolvedUrl(QUrl(arg)));
        QDeclarativeComponent *c = new QDeclarativeComponent(activeEngine, url, activeEngine);
        QDeclarativeComponentPrivate::get(c)->creationContext = context;
        QDeclarativeData::get(c, true)->setImplicitDestructible();
        return activeEnginePriv->objectClass->newQObject(c, qMetaTypeId<QDeclarativeComponent*>());
    }
}

QScriptValue QDeclarativeEnginePrivate::createQmlObject(QScriptContext *ctxt, QScriptEngine *engine)
{
    QDeclarativeEnginePrivate *activeEnginePriv =
        static_cast<QDeclarativeScriptEngine*>(engine)->p;
    QDeclarativeEngine* activeEngine = activeEnginePriv->q_func();

    if(ctxt->argumentCount() < 2 || ctxt->argumentCount() > 3)
        return ctxt->throwError("Qt.createQmlObject(): Invalid arguments");

    QDeclarativeContextData* context = activeEnginePriv->getContext(ctxt);
    Q_ASSERT(context);

    QString qml = ctxt->argument(0).toString();
    if (qml.isEmpty())
        return engine->nullValue();

    QUrl url;
    if(ctxt->argumentCount() > 2)
        url = QUrl(ctxt->argument(2).toString());
    else
        url = QUrl(QLatin1String("inline"));

    if (url.isValid() && url.isRelative())
        url = context->resolvedUrl(url);

    QObject *parentArg = activeEnginePriv->objectClass->toQObject(ctxt->argument(1));
    if(!parentArg)
        return ctxt->throwError("Qt.createQmlObject(): Missing parent object");

    QDeclarativeComponent component(activeEngine);
    component.setData(qml.toUtf8(), url);

    if(component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        QString errstr = QLatin1String("Qt.createQmlObject() failed to create object: ");
        QScriptValue arr = ctxt->engine()->newArray(errors.length());
        int i = 0;
        foreach (const QDeclarativeError &error, errors){
            errstr += QLatin1String("    ") + error.toString() + QLatin1String("\n");
            QScriptValue qmlErrObject = ctxt->engine()->newObject();
            qmlErrObject.setProperty("lineNumber", QScriptValue(error.line()));
            qmlErrObject.setProperty("columnNumber", QScriptValue(error.column()));
            qmlErrObject.setProperty("fileName", QScriptValue(error.url().toString()));
            qmlErrObject.setProperty("message", QScriptValue(error.description()));
            arr.setProperty(i++, qmlErrObject);
        }
        QScriptValue err = ctxt->throwError(errstr);
        err.setProperty("qmlErrors",arr);
        return err;
    }

    if (!component.isReady())
        return ctxt->throwError("Qt.createQmlObject(): Component is not ready");

    QObject *obj = component.create(context->asQDeclarativeContext());

    if(component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        QString errstr = QLatin1String("Qt.createQmlObject() failed to create object: ");
        QScriptValue arr = ctxt->engine()->newArray(errors.length());
        int i = 0;
        foreach (const QDeclarativeError &error, errors){
            errstr += QLatin1String("    ") + error.toString() + QLatin1String("\n");
            QScriptValue qmlErrObject = ctxt->engine()->newObject();
            qmlErrObject.setProperty("lineNumber", QScriptValue(error.line()));
            qmlErrObject.setProperty("columnNumber", QScriptValue(error.column()));
            qmlErrObject.setProperty("fileName", QScriptValue(error.url().toString()));
            qmlErrObject.setProperty("message", QScriptValue(error.description()));
            arr.setProperty(i++, qmlErrObject);
        }
        QScriptValue err = ctxt->throwError(errstr);
        err.setProperty("qmlErrors",arr);
        return err;
    }

    Q_ASSERT(obj);

    obj->setParent(parentArg);
    QGraphicsObject* gobj = qobject_cast<QGraphicsObject*>(obj);
    QGraphicsObject* gparent = qobject_cast<QGraphicsObject*>(parentArg);
    if(gobj && gparent)
        gobj->setParentItem(gparent);

    QDeclarativeData::get(obj, true)->setImplicitDestructible();
    return activeEnginePriv->objectClass->newQObject(obj, QMetaType::QObjectStar);
}

QScriptValue QDeclarativeEnginePrivate::isQtObject(QScriptContext *ctxt, QScriptEngine *engine)
{
    if (ctxt->argumentCount() == 0)
        return QScriptValue(engine, false);

    return QScriptValue(engine, 0 != ctxt->argument(0).toQObject());
}

QScriptValue QDeclarativeEnginePrivate::vector(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 3)
        return ctxt->throwError("Qt.vector(): Invalid arguments");
    qsreal x = ctxt->argument(0).toNumber();
    qsreal y = ctxt->argument(1).toNumber();
    qsreal z = ctxt->argument(2).toNumber();
    return engine->newVariant(qVariantFromValue(QVector3D(x, y, z)));
}

QScriptValue QDeclarativeEnginePrivate::formatDate(QScriptContext*ctxt, QScriptEngine*engine)
{
    int argCount = ctxt->argumentCount();
    if(argCount == 0 || argCount > 2)
        return ctxt->throwError("Qt.formatDate(): Invalid arguments");

    QDate date = ctxt->argument(0).toDateTime().date();
    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    if (argCount == 2) {
        if (ctxt->argument(1).isString()) {
            QString format = ctxt->argument(1).toString();
            return engine->newVariant(qVariantFromValue(date.toString(format)));
        } else if (ctxt->argument(1).isNumber()) {
            enumFormat = Qt::DateFormat(ctxt->argument(1).toUInt32());
        } else {
            return ctxt->throwError("Qt.formatDate(): Invalid date format");
        }
    }
    return engine->newVariant(qVariantFromValue(date.toString(enumFormat)));
}

QScriptValue QDeclarativeEnginePrivate::formatTime(QScriptContext*ctxt, QScriptEngine*engine)
{
    int argCount = ctxt->argumentCount();
    if(argCount == 0 || argCount > 2)
        return ctxt->throwError("Qt.formatTime(): Invalid arguments");

    QTime date = ctxt->argument(0).toDateTime().time();
    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    if (argCount == 2) {
        if (ctxt->argument(1).isString()) {
            QString format = ctxt->argument(1).toString();
            return engine->newVariant(qVariantFromValue(date.toString(format)));
        } else if (ctxt->argument(1).isNumber()) {
            enumFormat = Qt::DateFormat(ctxt->argument(1).toUInt32());
        } else {
            return ctxt->throwError("Qt.formatTime(): Invalid time format");
        }
    }
    return engine->newVariant(qVariantFromValue(date.toString(enumFormat)));
}

QScriptValue QDeclarativeEnginePrivate::formatDateTime(QScriptContext*ctxt, QScriptEngine*engine)
{
    int argCount = ctxt->argumentCount();
    if(argCount == 0 || argCount > 2)
        return ctxt->throwError("Qt.formatDateTime(): Invalid arguments");

    QDateTime date = ctxt->argument(0).toDateTime();
    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    if (argCount == 2) {
        if (ctxt->argument(1).isString()) {
            QString format = ctxt->argument(1).toString();
            return engine->newVariant(qVariantFromValue(date.toString(format)));
        } else if (ctxt->argument(1).isNumber()) {
            enumFormat = Qt::DateFormat(ctxt->argument(1).toUInt32());
        } else { 
            return ctxt->throwError("Qt.formatDateTime(): Invalid datetime formate");
        }
    }
    return engine->newVariant(qVariantFromValue(date.toString(enumFormat)));
}

QScriptValue QDeclarativeEnginePrivate::rgba(QScriptContext *ctxt, QScriptEngine *engine)
{
    int argCount = ctxt->argumentCount();
    if(argCount < 3 || argCount > 4)
        return ctxt->throwError("Qt.rgba(): Invalid arguments");
    qsreal r = ctxt->argument(0).toNumber();
    qsreal g = ctxt->argument(1).toNumber();
    qsreal b = ctxt->argument(2).toNumber();
    qsreal a = (argCount == 4) ? ctxt->argument(3).toNumber() : 1;

    if (r < 0.0) r=0.0;
    if (r > 1.0) r=1.0;
    if (g < 0.0) g=0.0;
    if (g > 1.0) g=1.0;
    if (b < 0.0) b=0.0;
    if (b > 1.0) b=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return qScriptValueFromValue(engine, qVariantFromValue(QColor::fromRgbF(r, g, b, a)));
}

QScriptValue QDeclarativeEnginePrivate::hsla(QScriptContext *ctxt, QScriptEngine *engine)
{
    int argCount = ctxt->argumentCount();
    if(argCount < 3 || argCount > 4)
        return ctxt->throwError("Qt.hsla(): Invalid arguments");
    qsreal h = ctxt->argument(0).toNumber();
    qsreal s = ctxt->argument(1).toNumber();
    qsreal l = ctxt->argument(2).toNumber();
    qsreal a = (argCount == 4) ? ctxt->argument(3).toNumber() : 1;

    if (h < 0.0) h=0.0;
    if (h > 1.0) h=1.0;
    if (s < 0.0) s=0.0;
    if (s > 1.0) s=1.0;
    if (l < 0.0) l=0.0;
    if (l > 1.0) l=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return qScriptValueFromValue(engine, qVariantFromValue(QColor::fromHslF(h, s, l, a)));
}

QScriptValue QDeclarativeEnginePrivate::rect(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 4)
        return ctxt->throwError("Qt.rect(): Invalid arguments");

    qsreal x = ctxt->argument(0).toNumber();
    qsreal y = ctxt->argument(1).toNumber();
    qsreal w = ctxt->argument(2).toNumber();
    qsreal h = ctxt->argument(3).toNumber();

    if (w < 0 || h < 0)
        return engine->nullValue();

    return qScriptValueFromValue(engine, qVariantFromValue(QRectF(x, y, w, h)));
}

QScriptValue QDeclarativeEnginePrivate::point(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 2)
        return ctxt->throwError("Qt.point(): Invalid arguments");
    qsreal x = ctxt->argument(0).toNumber();
    qsreal y = ctxt->argument(1).toNumber();
    return qScriptValueFromValue(engine, qVariantFromValue(QPointF(x, y)));
}

QScriptValue QDeclarativeEnginePrivate::size(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 2)
        return ctxt->throwError("Qt.size(): Invalid arguments");
    qsreal w = ctxt->argument(0).toNumber();
    qsreal h = ctxt->argument(1).toNumber();
    return qScriptValueFromValue(engine, qVariantFromValue(QSizeF(w, h)));
}

QScriptValue QDeclarativeEnginePrivate::lighter(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 1)
        return ctxt->throwError("Qt.lighter(): Invalid arguments");
    QVariant v = ctxt->argument(0).toVariant();
    QColor color;
    if (v.userType() == QVariant::Color)
        color = v.value<QColor>();
    else if (v.userType() == QVariant::String) {
        bool ok;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok)
            return engine->nullValue();
    } else
        return engine->nullValue();
    color = color.lighter();
    return qScriptValueFromValue(engine, qVariantFromValue(color));
}

QScriptValue QDeclarativeEnginePrivate::darker(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 1)
        return ctxt->throwError("Qt.darker(): Invalid arguments");
    QVariant v = ctxt->argument(0).toVariant();
    QColor color;
    if (v.userType() == QVariant::Color)
        color = v.value<QColor>();
    else if (v.userType() == QVariant::String) {
        bool ok;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok)
            return engine->nullValue();
    } else
        return engine->nullValue();
    color = color.darker();
    return qScriptValueFromValue(engine, qVariantFromValue(color));
}

QScriptValue QDeclarativeEnginePrivate::desktopOpenUrl(QScriptContext *ctxt, QScriptEngine *e)
{
    if(ctxt->argumentCount() < 1)
        return QScriptValue(e, false);
    bool ret = false;
#ifndef QT_NO_DESKTOPSERVICES
    ret = QDesktopServices::openUrl(QUrl(ctxt->argument(0).toString()));
#endif
    return QScriptValue(e, ret);
}

QScriptValue QDeclarativeEnginePrivate::md5(QScriptContext *ctxt, QScriptEngine *)
{
    if (ctxt->argumentCount() != 1)
        return ctxt->throwError("Qt.md5(): Invalid arguments");

    QByteArray data = ctxt->argument(0).toString().toUtf8();
    QByteArray result = QCryptographicHash::hash(data, QCryptographicHash::Md5);

    return QScriptValue(QLatin1String(result.toHex()));
}

QScriptValue QDeclarativeEnginePrivate::btoa(QScriptContext *ctxt, QScriptEngine *)
{
    if (ctxt->argumentCount() != 1) 
        return ctxt->throwError("Qt.btoa(): Invalid arguments");

    QByteArray data = ctxt->argument(0).toString().toUtf8();

    return QScriptValue(QLatin1String(data.toBase64()));
}

QScriptValue QDeclarativeEnginePrivate::atob(QScriptContext *ctxt, QScriptEngine *)
{
    if (ctxt->argumentCount() != 1) 
        return ctxt->throwError("Qt.atob(): Invalid arguments");

    QByteArray data = ctxt->argument(0).toString().toUtf8();

    return QScriptValue(QLatin1String(QByteArray::fromBase64(data)));
}

QScriptValue QDeclarativeEnginePrivate::consoleLog(QScriptContext *ctxt, QScriptEngine *e)
{
    if(ctxt->argumentCount() < 1)
        return e->newVariant(QVariant(false));

    QByteArray msg;

    for (int i=0; i<ctxt->argumentCount(); ++i) {
        if (!msg.isEmpty()) msg += ' ';
        msg += ctxt->argument(i).toString().toLocal8Bit();
        // does not support firebug "%[a-z]" formatting, since firebug really
        // does just ignore the format letter, which makes it pointless.
    }

    qDebug("%s",msg.constData());

    return e->newVariant(QVariant(true));
}

void QDeclarativeEnginePrivate::sendQuit()
{
    Q_Q(QDeclarativeEngine);
    emit q->quit();
}

static void dumpwarning(const QDeclarativeError &error)
{
    qWarning().nospace() << qPrintable(error.toString());
}

static void dumpwarning(const QList<QDeclarativeError> &errors)
{
    for (int ii = 0; ii < errors.count(); ++ii)
        dumpwarning(errors.at(ii));
}

void QDeclarativeEnginePrivate::warning(const QDeclarativeError &error)
{
    Q_Q(QDeclarativeEngine);
    q->warnings(QList<QDeclarativeError>() << error);
    if (outputWarningsToStdErr)
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(const QList<QDeclarativeError> &errors)
{
    Q_Q(QDeclarativeEngine);
    q->warnings(errors);
    if (outputWarningsToStdErr)
        dumpwarning(errors);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEngine *engine, const QDeclarativeError &error)
{
    if (engine)
        QDeclarativeEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEngine *engine, const QList<QDeclarativeError> &error)
{
    if (engine)
        QDeclarativeEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEnginePrivate *engine, const QDeclarativeError &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEnginePrivate *engine, const QList<QDeclarativeError> &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

QScriptValue QDeclarativeEnginePrivate::quit(QScriptContext * /*ctxt*/, QScriptEngine *e)
{
    QDeclarativeEnginePrivate *qe = get (e);
    qe->sendQuit();
    return QScriptValue();
}

QScriptValue QDeclarativeEnginePrivate::tint(QScriptContext *ctxt, QScriptEngine *engine)
{
    if(ctxt->argumentCount() != 2)
        return ctxt->throwError("Qt.tint(): Invalid arguments");
    //get color
    QVariant v = ctxt->argument(0).toVariant();
    QColor color;
    if (v.userType() == QVariant::Color)
        color = v.value<QColor>();
    else if (v.userType() == QVariant::String) {
        bool ok;
        color = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok)
            return engine->nullValue();
    } else
        return engine->nullValue();

    //get tint color
    v = ctxt->argument(1).toVariant();
    QColor tintColor;
    if (v.userType() == QVariant::Color)
        tintColor = v.value<QColor>();
    else if (v.userType() == QVariant::String) {
        bool ok;
        tintColor = QDeclarativeStringConverters::colorFromString(v.toString(), &ok);
        if (!ok)
            return engine->nullValue();
    } else
        return engine->nullValue();

    //tint
    QColor finalColor;
    int a = tintColor.alpha();
    if (a == 0xFF)
        finalColor = tintColor;
    else if (a == 0x00)
        finalColor = color;
    else {
        qreal a = tintColor.alphaF();
        qreal inv_a = 1.0 - a;

        finalColor.setRgbF(tintColor.redF() * a + color.redF() * inv_a,
                           tintColor.greenF() * a + color.greenF() * inv_a,
                           tintColor.blueF() * a + color.blueF() * inv_a,
                           a + inv_a * color.alphaF());
    }

    return qScriptValueFromValue(engine, qVariantFromValue(finalColor));
}

QScriptValue QDeclarativeEnginePrivate::scriptValueFromVariant(const QVariant &val)
{
    if (val.userType() == qMetaTypeId<QDeclarativeListReference>()) {
        QDeclarativeListReferencePrivate *p =
            QDeclarativeListReferencePrivate::get((QDeclarativeListReference*)val.constData());
        if (p->object) {
            return listClass->newList(p->property, p->propertyType);
        } else {
            return scriptEngine.nullValue();
        }
    } else if (val.userType() == qMetaTypeId<QList<QObject *> >()) {
        const QList<QObject *> &list = *(QList<QObject *>*)val.constData();
        QScriptValue rv = scriptEngine.newArray(list.count());
        for (int ii = 0; ii < list.count(); ++ii) {
            QObject *object = list.at(ii);
            rv.setProperty(ii, objectClass->newQObject(object));
        }
        return rv;
    } else if (QDeclarativeValueType *vt = valueTypes[val.userType()]) {
        return valueTypeClass->newObject(val, vt);
    }

    bool objOk;
    QObject *obj = QDeclarativeMetaType::toQObject(val, &objOk);
    if (objOk) {
        return objectClass->newQObject(obj);
    } else {
        return qScriptValueFromValue(&scriptEngine, val);
    }
}

QVariant QDeclarativeEnginePrivate::scriptValueToVariant(const QScriptValue &val, int hint)
{
    QScriptDeclarativeClass *dc = QScriptDeclarativeClass::scriptClass(val);
    if (dc == objectClass)
        return QVariant::fromValue(objectClass->toQObject(val));
    else if (dc == valueTypeClass)
        return valueTypeClass->toVariant(val);
    else if (dc == contextClass)
        return QVariant();

    // Convert to a QList<QObject*> only if val is an array and we were explicitly hinted
    if (hint == qMetaTypeId<QList<QObject *> >() && val.isArray()) {
        QList<QObject *> list;
        int length = val.property(QLatin1String("length")).toInt32();
        for (int ii = 0; ii < length; ++ii) {
            QScriptValue arrayItem = val.property(ii);
            QObject *d = arrayItem.toQObject();
            list << d;
        }
        return QVariant::fromValue(list);
    }

    return val.toVariant();
}

// XXX this beyonds in QUrl::toLocalFile()
// WARNING, there is a copy of this function in qdeclarativecompositetypemanager.cpp
static QString toLocalFileOrQrc(const QUrl& url)
{
    if (url.scheme() == QLatin1String("qrc")) {
        if (url.authority().isEmpty())
            return QLatin1Char(':') + url.path();
        return QString();
    }
    return url.toLocalFile();
}

/////////////////////////////////////////////////////////////
struct QDeclarativeEnginePrivate::ImportedNamespace {
    QStringList uris;
    QStringList urls;
    QList<int> majversions;
    QList<int> minversions;
    QList<bool> isLibrary;
    QList<QDeclarativeDirComponents> qmlDirComponents;

    bool find(const QByteArray& type, int *vmajor, int *vminor, QDeclarativeType** type_return, QUrl* url_return,
              QUrl *base = 0)
    {
        for (int i=0; i<urls.count(); ++i) {
            int vmaj = majversions.at(i);
            int vmin = minversions.at(i);

            QByteArray qt = uris.at(i).toUtf8();
            qt += '/';
            qt += type;

            QDeclarativeType *t = QDeclarativeMetaType::qmlType(qt,vmaj,vmin);
            if (t) {
                if (vmajor) *vmajor = vmaj;
                if (vminor) *vminor = vmin;
                if (type_return)
                    *type_return = t;
                return true;
            }

            QUrl url = QUrl(urls.at(i) + QLatin1Char('/') + QString::fromUtf8(type) + QLatin1String(".qml"));
            QDeclarativeDirComponents qmldircomponents = qmlDirComponents.at(i);

            bool typeWasDeclaredInQmldir = false;
            if (!qmldircomponents.isEmpty()) {
                const QString typeName = QString::fromUtf8(type);
                foreach (const QDeclarativeDirParser::Component &c, qmldircomponents) {
                    if (c.typeName == typeName) {
                        typeWasDeclaredInQmldir = true;

                        // importing version -1 means import ALL versions
                        if ((vmaj == -1) || (c.majorVersion < vmaj || (c.majorVersion == vmaj && vmin >= c.minorVersion))) {
                            QUrl candidate = url.resolved(QUrl(c.fileName));
                            if (c.internal && base) {
                                if (base->resolved(QUrl(c.fileName)) != candidate)
                                    continue; // failed attempt to access an internal type
                            }
                            if (url_return)
                                *url_return = candidate;
                            return true;
                        }
                    }
                }
            }

            if (!typeWasDeclaredInQmldir  && !isLibrary.at(i)) {
                // XXX search non-files too! (eg. zip files, see QT-524)
                QFileInfo f(toLocalFileOrQrc(url));
                if (f.exists()) {
                    if (url_return)
                        *url_return = url;
                    return true;
                }
            }
        }
        return false;
    }
};

static bool greaterThan(const QString &s1, const QString &s2)
{
    return s1 > s2;
}

class QDeclarativeImportsPrivate {
public:
    QDeclarativeImportsPrivate() : ref(1)
    {
    }

    ~QDeclarativeImportsPrivate()
    {
        foreach (QDeclarativeEnginePrivate::ImportedNamespace* s, set.values())
            delete s;
    }

    QSet<QString> qmlDirFilesForWhichPluginsHaveBeenLoaded;

    bool importExtension(const QString &absoluteFilePath, const QString &uri, QDeclarativeEngine *engine, QDeclarativeDirComponents* components, QString *errorString) {
        QFile file(absoluteFilePath);
        QString filecontent;
        if (file.open(QFile::ReadOnly)) {
            filecontent = QString::fromUtf8(file.readAll());
            if (qmlImportTrace())
                qDebug() << "QDeclarativeEngine::add: loaded" << absoluteFilePath;
        } else {
            if (errorString)
                *errorString = QDeclarativeEngine::tr("module \"%1\" definition \"%2\" not readable").arg(uri).arg(absoluteFilePath);
            return false;
        }
        QDir dir = QFileInfo(file).dir();

        QDeclarativeDirParser qmldirParser;
        qmldirParser.setSource(filecontent);
        qmldirParser.parse();

        if (! qmlDirFilesForWhichPluginsHaveBeenLoaded.contains(absoluteFilePath)) {
            qmlDirFilesForWhichPluginsHaveBeenLoaded.insert(absoluteFilePath);


            foreach (const QDeclarativeDirParser::Plugin &plugin, qmldirParser.plugins()) {

                QString resolvedFilePath =
                        QDeclarativeEnginePrivate::get(engine)
                        ->resolvePlugin(dir, plugin.path,
                                        plugin.name);

                if (!resolvedFilePath.isEmpty()) {
                    if (!engine->importPlugin(resolvedFilePath, uri, errorString)) {
                        if (errorString)
                            *errorString = QDeclarativeEngine::tr("plugin cannot be loaded for module \"%1\": %2").arg(uri).arg(*errorString);
                        return false;
                    }
                } else {
                    if (errorString)
                        *errorString = QDeclarativeEngine::tr("module \"%1\" plugin \"%2\" not found").arg(uri).arg(plugin.name);
                    return false;
                }
            }
        }

        if (components)
            *components = qmldirParser.components();

        return true;
    }

    QString resolvedUri(const QString &dir_arg, QDeclarativeEngine *engine)
    {
        QString dir = dir_arg;
        if (dir.endsWith(QLatin1Char('/')) || dir.endsWith(QLatin1Char('\\')))
            dir.chop(1);

        QStringList paths = QDeclarativeEnginePrivate::get(engine)->fileImportPath;
        qSort(paths.begin(), paths.end(), greaterThan); // Ensure subdirs preceed their parents.

        QString stableRelativePath = dir;
        foreach( QString path, paths) {
            if (dir.startsWith(path)) {
                stableRelativePath = dir.mid(path.length()+1);
                break;
            }
        }
        stableRelativePath.replace(QLatin1Char('/'), QLatin1Char('.'));
        stableRelativePath.replace(QLatin1Char('\\'), QLatin1Char('.'));
        return stableRelativePath;
    }




    bool add(const QUrl& base, const QDeclarativeDirComponents &qmldircomponentsnetwork, const QString& uri_arg, const QString& prefix, int vmaj, int vmin, QDeclarativeScriptParser::Import::Type importType, QDeclarativeEngine *engine, QString *errorString)
    {
        QDeclarativeDirComponents qmldircomponents = qmldircomponentsnetwork;
        QString uri = uri_arg;
        QDeclarativeEnginePrivate::ImportedNamespace *s;
        if (prefix.isEmpty()) {
            s = &unqualifiedset;
        } else {
            s = set.value(prefix);
            if (!s)
                set.insert(prefix,(s=new QDeclarativeEnginePrivate::ImportedNamespace));
        }



        QString url = uri;
        if (importType == QDeclarativeScriptParser::Import::Library) {
            url.replace(QLatin1Char('.'), QLatin1Char('/'));
            bool found = false;
            QString dir;


            foreach (const QString &p,
                     QDeclarativeEnginePrivate::get(engine)->fileImportPath) {
                dir = p+QLatin1Char('/')+url;

                QFileInfo fi(dir+QLatin1String("/qmldir"));
                const QString absoluteFilePath = fi.absoluteFilePath();

                if (fi.isFile()) {
                    found = true;

                    url = QUrl::fromLocalFile(fi.absolutePath()).toString();
                    uri = resolvedUri(dir, engine);
                    if (!importExtension(absoluteFilePath, uri, engine, &qmldircomponents, errorString))
                        return false;
                    break;
                }
            }

            if (!found) {
                found = QDeclarativeMetaType::isModule(uri.toUtf8(), vmaj, vmin);
                if (!found) {
                    if (errorString) {
                        bool anyversion = QDeclarativeMetaType::isModule(uri.toUtf8(), -1, -1);
                        if (anyversion)
                            *errorString = QDeclarativeEngine::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin);
                        else
                            *errorString = QDeclarativeEngine::tr("module \"%1\" is not installed").arg(uri_arg);
                    }
                    return false;
                }
            }
        } else {

            if (importType == QDeclarativeScriptParser::Import::File && qmldircomponents.isEmpty()) {
                QUrl importUrl = base.resolved(QUrl(uri + QLatin1String("/qmldir")));
                QString localFileOrQrc = toLocalFileOrQrc(importUrl);
                if (!localFileOrQrc.isEmpty()) {
                    QString dir = toLocalFileOrQrc(base.resolved(QUrl(uri)));
                    if (dir.isEmpty() || !QDir().exists(dir)) {
                        if (errorString)
                            *errorString = QDeclarativeEngine::tr("\"%1\": no such directory").arg(uri_arg);
                        return false; // local import dirs must exist
                    }
                    uri = resolvedUri(toLocalFileOrQrc(base.resolved(QUrl(uri))), engine);
                    if (uri.endsWith(QLatin1Char('/')))
                        uri.chop(1);
                    if (QFile::exists(localFileOrQrc)) {
                        if (!importExtension(localFileOrQrc,uri,engine,&qmldircomponents,errorString))
                            return false;
                    }
                } else {
                    if (prefix.isEmpty()) {
                        // directory must at least exist for valid import
                        QString localFileOrQrc = toLocalFileOrQrc(base.resolved(QUrl(uri)));
                        if (localFileOrQrc.isEmpty() || !QDir().exists(localFileOrQrc)) {
                            if (errorString) {
                                if (localFileOrQrc.isEmpty())
                                    *errorString = QDeclarativeEngine::tr("import \"%1\" has no qmldir and no namespace").arg(uri);
                                else
                                    *errorString = QDeclarativeEngine::tr("\"%1\": no such directory").arg(uri);
                            }
                            return false;
                        }
                    }
                }
            }

            url = base.resolved(QUrl(url)).toString();
            if (url.endsWith(QLatin1Char('/')))
                url.chop(1);
        }

        if (vmaj > -1 && vmin > -1 && !qmldircomponents.isEmpty()) {
            QList<QDeclarativeDirParser::Component>::ConstIterator it = qmldircomponents.begin();
            for (; it != qmldircomponents.end(); ++it) {
                if (it->majorVersion > vmaj || (it->majorVersion == vmaj && it->minorVersion >= vmin))
                    break;
            }
            if (it == qmldircomponents.end()) {
                *errorString = QDeclarativeEngine::tr("module \"%1\" version %2.%3 is not installed").arg(uri_arg).arg(vmaj).arg(vmin);
                return false;
            }
        }

        s->uris.prepend(uri);
        s->urls.prepend(url);
        s->majversions.prepend(vmaj);
        s->minversions.prepend(vmin);
        s->isLibrary.prepend(importType == QDeclarativeScriptParser::Import::Library);
        s->qmlDirComponents.prepend(qmldircomponents);
        return true;
    }

    bool find(const QByteArray& type, int *vmajor, int *vminor, QDeclarativeType** type_return, QUrl* url_return)
    {
        QDeclarativeEnginePrivate::ImportedNamespace *s = 0;
        int slash = type.indexOf('/');
        if (slash >= 0) {
            s = set.value(QString::fromUtf8(type.left(slash)));
            if (!s)
                return false; // qualifier must be a namespace
            int nslash = type.indexOf('/',slash+1);
            if (nslash > 0)
                return false; // only single qualification allowed
        } else {
            s = &unqualifiedset;
        }
        QByteArray unqualifiedtype = slash < 0 ? type : type.mid(slash+1); // common-case opt (QString::mid works fine, but slower)
        if (s) {
            if (s->find(unqualifiedtype,vmajor,vminor,type_return,url_return, &base))
                return true;
            if (s->urls.count() == 1 && !s->isLibrary[0] && url_return && s != &unqualifiedset) {
                // qualified, and only 1 url
                *url_return = QUrl(s->urls[0]+QLatin1Char('/')).resolved(QUrl(QString::fromUtf8(unqualifiedtype) + QLatin1String(".qml")));
                return true;
            }

        }

        return false;
    }

    QDeclarativeEnginePrivate::ImportedNamespace *findNamespace(const QString& type)
    {
        return set.value(type);
    }

    QUrl base;
    int ref;

private:
    friend struct QDeclarativeEnginePrivate::Imports;
    QDeclarativeEnginePrivate::ImportedNamespace unqualifiedset;
    QHash<QString,QDeclarativeEnginePrivate::ImportedNamespace* > set;
};

QDeclarativeEnginePrivate::Imports::Imports(const Imports &copy) :
    d(copy.d)
{
    ++d->ref;
}

QDeclarativeEnginePrivate::Imports &QDeclarativeEnginePrivate::Imports::operator =(const Imports &copy)
{
    ++copy.d->ref;
    if (--d->ref == 0)
        delete d;
    d = copy.d;
    return *this;
}

QDeclarativeEnginePrivate::Imports::Imports() :
    d(new QDeclarativeImportsPrivate)
{
}

QDeclarativeEnginePrivate::Imports::~Imports()
{
    if (--d->ref == 0)
        delete d;
}

static QDeclarativeTypeNameCache *cacheForNamespace(QDeclarativeEngine *engine, const QDeclarativeEnginePrivate::ImportedNamespace &set, QDeclarativeTypeNameCache *cache)
{
    if (!cache)
        cache = new QDeclarativeTypeNameCache(engine);

    QList<QDeclarativeType *> types = QDeclarativeMetaType::qmlTypes();

    for (int ii = 0; ii < set.uris.count(); ++ii) {
        QByteArray base = set.uris.at(ii).toUtf8() + '/';
        int major = set.majversions.at(ii);
        int minor = set.minversions.at(ii);

        foreach (QDeclarativeType *type, types) {
            if (type->qmlTypeName().startsWith(base) &&
                type->qmlTypeName().lastIndexOf('/') == (base.length() - 1) &&
                type->availableInVersion(major,minor))
            {
                QString name = QString::fromUtf8(type->qmlTypeName().mid(base.length()));

                cache->add(name, type);
            }
        }
    }

    return cache;
}

void QDeclarativeEnginePrivate::Imports::cache(QDeclarativeTypeNameCache *cache, QDeclarativeEngine *engine) const
{
    const QDeclarativeEnginePrivate::ImportedNamespace &set = d->unqualifiedset;

    for (QHash<QString,QDeclarativeEnginePrivate::ImportedNamespace* >::ConstIterator iter = d->set.begin();
         iter != d->set.end(); ++iter) {

        QDeclarativeTypeNameCache::Data *d = cache->data(iter.key());
        if (d) {
            if (!d->typeNamespace)
                cacheForNamespace(engine, *(*iter), d->typeNamespace);
        } else {
            QDeclarativeTypeNameCache *nc = cacheForNamespace(engine, *(*iter), 0);
            cache->add(iter.key(), nc);
            nc->release();
        }
    }

    cacheForNamespace(engine, set, cache);
}

/*
QStringList QDeclarativeEnginePrivate::Imports::unqualifiedSet() const
{
    QStringList rv;

    const QDeclarativeEnginePrivate::ImportedNamespace &set = d->unqualifiedset;

    for (int ii = 0; ii < set.urls.count(); ++ii) {
        if (set.isBuiltin.at(ii))
            rv << set.urls.at(ii);
    }

    return rv;
}
*/

/*!
  Sets the base URL to be used for all relative file imports added.
*/
void QDeclarativeEnginePrivate::Imports::setBaseUrl(const QUrl& url)
{
    d->base = url;
}

/*!
  Returns the base URL to be used for all relative file imports added.
*/
QUrl QDeclarativeEnginePrivate::Imports::baseUrl() const
{
    return d->base;
}

/*!
  Adds \a path as a directory where the engine searches for
  installed modules in a URL-based directory structure.

  The newly added \a path will be first in the importPathList().

  \sa setImportPathList()
*/
void QDeclarativeEngine::addImportPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug() << "QDeclarativeEngine::addImportPath" << path;
    Q_D(QDeclarativeEngine);
    QUrl url = QUrl(path);
    if (url.isRelative() || url.scheme() == QString::fromLocal8Bit("file")) {
        QDir dir = QDir(path);
        d->fileImportPath.prepend(dir.canonicalPath());
    } else {
        d->fileImportPath.prepend(path);
    }
}


/*!
  Returns the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  For example, if \c /opt/MyApp/lib/imports is in the path, then QML that
  imports \c com.mycompany.Feature will cause the QDeclarativeEngine to look
  in \c /opt/MyApp/lib/imports/com/mycompany/Feature/ for the components
  provided by that module. A \c qmldir file is required for defining the
  type version mapping and possibly declarative extensions plugins.

  By default, the list contains the paths specified in the \c QML_IMPORT_PATH environment
  variable, then the builtin \c ImportsPath from QLibraryInfo.

  \sa addImportPath() setImportPathList()
*/
QStringList QDeclarativeEngine::importPathList() const
{
    Q_D(const QDeclarativeEngine);
    return d->fileImportPath;
}

/*!
  Sets the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  By default, the list contains the paths specified in the \c QML_IMPORT_PATH environment
  variable, then the builtin \c ImportsPath from QLibraryInfo.

  \sa importPathList() addImportPath()
  */
void QDeclarativeEngine::setImportPathList(const QStringList &paths)
{
    Q_D(QDeclarativeEngine);
    d->fileImportPath = paths;
}


/*!
  Adds \a path as a directory where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  The newly added \a path will be first in the pluginPathList().

  \sa setPluginPathList()
*/
void QDeclarativeEngine::addPluginPath(const QString& path)
{
    if (qmlImportTrace())
        qDebug() << "QDeclarativeEngine::addPluginPath" << path;
    Q_D(QDeclarativeEngine);
    QUrl url = QUrl(path);
    if (url.isRelative() || url.scheme() == QString::fromLocal8Bit("file")) {
        QDir dir = QDir(path);
        d->filePluginPath.prepend(dir.canonicalPath());
    } else {
        d->filePluginPath.prepend(path);
    }
}


/*!
  Returns the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa addPluginPath() setPluginPathList()
*/
QStringList QDeclarativeEngine::pluginPathList() const
{
    Q_D(const QDeclarativeEngine);
    return d->filePluginPath;
}

/*!
  Sets the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file)
  to \a paths.

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa pluginPathList() addPluginPath()
  */
void QDeclarativeEngine::setPluginPathList(const QStringList &paths)
{
    Q_D(QDeclarativeEngine);
    d->filePluginPath = paths;
}


/*!
  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, *\a errorString will be set to a message describing the failure.

  The plugin has to be a Qt plugin which implements the QDeclarativeExtensionPlugin interface.
*/
bool QDeclarativeEngine::importPlugin(const QString &filePath, const QString &uri, QString *errorString)
{
    if (qmlImportTrace())
        qDebug() << "QDeclarativeEngine::importPlugin" << uri << "from" << filePath;
    QFileInfo fileInfo(filePath);
    const QString absoluteFilePath = fileInfo.absoluteFilePath();

    QDeclarativeEnginePrivate *d = QDeclarativeEnginePrivate::get(this);
    bool engineInitialized = d->initializedPlugins.contains(absoluteFilePath);
    bool typesRegistered = qmlEnginePluginsWithRegisteredTypes()->contains(absoluteFilePath);

    if (typesRegistered) {
        Q_ASSERT_X(qmlEnginePluginsWithRegisteredTypes()->value(absoluteFilePath) == uri,
                   "QDeclarativeEngine::importExtension",
                   "Internal error: Plugin imported previously with different uri");
    }

    if (!engineInitialized || !typesRegistered) {
        QPluginLoader loader(absoluteFilePath);

        if (!loader.load()) {
            if (errorString)
                *errorString = loader.errorString();
            return false;
        }

        if (QDeclarativeExtensionInterface *iface = qobject_cast<QDeclarativeExtensionInterface *>(loader.instance())) {

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            if (!typesRegistered) {

                // ### this code should probably be protected with a mutex.
                qmlEnginePluginsWithRegisteredTypes()->insert(absoluteFilePath, uri);
                iface->registerTypes(moduleId);
            }
            if (!engineInitialized) {
                // things on the engine (eg. adding new global objects) have to be done for every engine.

                // protect against double initialization
                d->initializedPlugins.insert(absoluteFilePath);
                iface->initializeEngine(this, moduleId);
            }
        } else {
            if (errorString)
                *errorString = loader.errorString();
            return false;
        }
    }

    return true;
}

/*!
  \property QDeclarativeEngine::offlineStoragePath
  \brief the directory for storing offline user data

  Returns the directory where SQL and other offline
  storage is placed.

  QDeclarativeWebView and the SQL databases created with openDatabase()
  are stored here.

  The default is QML/OfflineStorage in the platform-standard
  user application data directory.

  Note that the path may not currently exist on the filesystem, so
  callers wanting to \e create new files at this location should create
  it first - see QDir::mkpath().
*/
void QDeclarativeEngine::setOfflineStoragePath(const QString& dir)
{
    Q_D(QDeclarativeEngine);
    d->scriptEngine.offlineStoragePath = dir;
}

QString QDeclarativeEngine::offlineStoragePath() const
{
    Q_D(const QDeclarativeEngine);
    return d->scriptEngine.offlineStoragePath;
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a path, \a suffixes, and \a prefix.
  The \a prefix must contain the dot.

  \a qmldirPath is the location of the qmldir file.
 */
QString QDeclarativeEnginePrivate::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath, const QString &baseName,
                                        const QStringList &suffixes,
                                        const QString &prefix)
{
    QStringList searchPaths = filePluginPath;
    bool qmldirPluginPathIsRelative = QDir::isRelativePath(qmldirPluginPath);
    if (!qmldirPluginPathIsRelative)
        searchPaths.prepend(qmldirPluginPath);

    foreach (const QString &pluginPath, searchPaths) {

        QString resolvedPath;

        if (pluginPath == QLatin1String(".")) {
            if (qmldirPluginPathIsRelative)
                resolvedPath = qmldirPath.absoluteFilePath(qmldirPluginPath);
            else
                resolvedPath = qmldirPath.absolutePath();
        } else {
            resolvedPath = pluginPath;
        }

        // hack for resources, should probably go away
        if (resolvedPath.startsWith(QLatin1Char(':')))
            resolvedPath = QCoreApplication::applicationDirPath();

        QDir dir(resolvedPath);
        foreach (const QString &suffix, suffixes) {
            QString pluginFileName = prefix;

            pluginFileName += baseName;
            pluginFileName += suffix;

            QFileInfo fileInfo(dir, pluginFileName);

            if (fileInfo.exists())
                return fileInfo.absoluteFilePath();
        }
    }

    if (qmlImportTrace())
        qDebug() << "QDeclarativeEngine::resolvePlugin: Could not resolve plugin" << baseName << "in" << qmldirPath.absolutePath();
    return QString();
}

/*!
  \internal

  Returns the result of the merge of \a baseName with \a dir and the platform suffix.

  \table
  \header \i Platform \i Valid suffixes
  \row \i Windows     \i \c .dll
  \row \i Unix/Linux  \i \c .so
  \row \i AIX  \i \c .a
  \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
  \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
  \row \i Symbian     \i \c .dll
  \endtable

  Version number on unix are ignored.
*/
QString QDeclarativeEnginePrivate::resolvePlugin(const QDir &qmldirPath, const QString &qmldirPluginPath, const QString &baseName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
# ifdef QT_DEBUG
                         << QLatin1String("d.dll") // try a qmake-style debug build first
# endif
                         << QLatin1String(".dll"));
#elif defined(Q_OS_SYMBIAN)
    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
                         << QLatin1String(".dll")
                         << QLatin1String(".qtplugin"));
#else

# if defined(Q_OS_DARWIN)

    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName,
                         QStringList()
# ifdef QT_DEBUG
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build first
                         << QLatin1String(".dylib")
# else
                         << QLatin1String(".dylib")
                         << QLatin1String("_debug.dylib") // try a qmake-style debug build after
# endif
                         << QLatin1String(".so")
                         << QLatin1String(".bundle"),
                         QLatin1String("lib"));
# else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String(".sl");
#   if defined __ia64
    validSuffixList << QLatin1String(".so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String(".a") << QLatin1String(".so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String(".so");
#  endif

    // Examples of valid library names:
    //  libfoo.so

    return resolvePlugin(qmldirPath, qmldirPluginPath, baseName, validSuffixList, QLatin1String("lib"));
# endif

#endif
}

/*!
  \internal

  Adds information to \a imports such that subsequent calls to resolveType()
  will resolve types qualified by \a prefix by considering types found at the given \a uri.

  The uri is either a directory (if importType is FileImport), or a URI resolved using paths
  added via addImportPath() (if importType is LibraryImport).

  The \a prefix may be empty, in which case the import location is considered for
  unqualified types.

  The base URL must already have been set with Import::setBaseUrl().
*/
bool QDeclarativeEnginePrivate::addToImport(Imports* imports, const QDeclarativeDirComponents &qmldircomponentsnetwork, const QString& uri, const QString& prefix, int vmaj, int vmin, QDeclarativeScriptParser::Import::Type importType, QString *errorString) const
{
    QDeclarativeEngine *engine = QDeclarativeEnginePrivate::get(const_cast<QDeclarativeEnginePrivate *>(this));
    if (qmlImportTrace())
        qDebug().nospace() << "QDeclarativeEngine::addToImport " << imports << " " << uri << " " << vmaj << '.' << vmin << " " << (importType==QDeclarativeScriptParser::Import::Library? "Library" : "File") << " as " << prefix;
    bool ok = imports->d->add(imports->d->base,qmldircomponentsnetwork, uri,prefix,vmaj,vmin,importType, engine, errorString);
    return ok;
}

/*!
  \internal

  Using the given \a imports, the given (namespace qualified) \a type is resolved to either
  an ImportedNamespace stored at \a ns_return,
  a QDeclarativeType stored at \a type_return, or
  a component located at \a url_return.

  If any return pointer is 0, the corresponding search is not done.

  \sa addToImport()
*/
bool QDeclarativeEnginePrivate::resolveType(const Imports& imports, const QByteArray& type, QDeclarativeType** type_return, QUrl* url_return, int *vmaj, int *vmin, ImportedNamespace** ns_return) const
{
    ImportedNamespace* ns = imports.d->findNamespace(QString::fromUtf8(type));
    if (ns) {
        if (ns_return)
            *ns_return = ns;
        return true;
    }
    if (type_return || url_return) {
        if (imports.d->find(type,vmaj,vmin,type_return,url_return)) {
            if (qmlImportTrace()) {
                if (type_return && *type_return && url_return && !url_return->isEmpty())
                    qDebug() << "QDeclarativeEngine::resolveType" << type << '=' << (*type_return)->typeName() << *url_return;
                if (type_return && *type_return)
                    qDebug() << "QDeclarativeEngine::resolveType" << type << '=' << (*type_return)->typeName();
                if (url_return && !url_return->isEmpty())
                    qDebug() << "QDeclarativeEngine::resolveType" << type << '=' << *url_return;
            }
            return true;
        }
    }
    return false;
}

/*!
  \internal

  Searching \e only in the namespace \a ns (previously returned in a call to
  resolveType(), \a type is found and returned to either
  a QDeclarativeType stored at \a type_return, or
  a component located at \a url_return.

  If either return pointer is 0, the corresponding search is not done.
*/
void QDeclarativeEnginePrivate::resolveTypeInNamespace(ImportedNamespace* ns, const QByteArray& type, QDeclarativeType** type_return, QUrl* url_return, int *vmaj, int *vmin ) const
{
    ns->find(type,vmaj,vmin,type_return,url_return);
}

static void voidptr_destructor(void *v)
{
    void **ptr = (void **)v;
    delete ptr;
}

static void *voidptr_constructor(const void *v)
{
    if (!v) {
        return new void*;
    } else {
        return new void*(*(void **)v);
    }
}

void QDeclarativeEnginePrivate::registerCompositeType(QDeclarativeCompiledData *data)
{
    QByteArray name = data->root->className();

    QByteArray ptr = name + '*';
    QByteArray lst = "QDeclarativeListProperty<" + name + ">";

    int ptr_type = QMetaType::registerType(ptr.constData(), voidptr_destructor,
                                           voidptr_constructor);
    int lst_type = QMetaType::registerType(lst.constData(), voidptr_destructor,
                                           voidptr_constructor);

    m_qmlLists.insert(lst_type, ptr_type);
    m_compositeTypes.insert(ptr_type, data);
    data->addref();
}

bool QDeclarativeEnginePrivate::isList(int t) const
{
    return m_qmlLists.contains(t) || QDeclarativeMetaType::isList(t);
}

int QDeclarativeEnginePrivate::listType(int t) const
{
    QHash<int, int>::ConstIterator iter = m_qmlLists.find(t);
    if (iter != m_qmlLists.end())
        return *iter;
    else
        return QDeclarativeMetaType::listType(t);
}

bool QDeclarativeEnginePrivate::isQObject(int t)
{
    return m_compositeTypes.contains(t) || QDeclarativeMetaType::isQObject(t);
}

QObject *QDeclarativeEnginePrivate::toQObject(const QVariant &v, bool *ok) const
{
    int t = v.userType();
    if (m_compositeTypes.contains(t)) {
        if (ok) *ok = true;
        return *(QObject **)(v.constData());
    } else {
        return QDeclarativeMetaType::toQObject(v, ok);
    }
}

QDeclarativeMetaType::TypeCategory QDeclarativeEnginePrivate::typeCategory(int t) const
{
    if (m_compositeTypes.contains(t))
        return QDeclarativeMetaType::Object;
    else if (m_qmlLists.contains(t))
        return QDeclarativeMetaType::List;
    else
        return QDeclarativeMetaType::typeCategory(t);
}

const QMetaObject *QDeclarativeEnginePrivate::rawMetaObjectForType(int t) const
{
    QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.find(t);
    if (iter != m_compositeTypes.end()) {
        return (*iter)->root;
    } else {
        QDeclarativeType *type = QDeclarativeMetaType::qmlType(t);
        return type?type->baseMetaObject():0;
    }
}

const QMetaObject *QDeclarativeEnginePrivate::metaObjectForType(int t) const
{
    QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.find(t);
    if (iter != m_compositeTypes.end()) {
        return (*iter)->root;
    } else {
        QDeclarativeType *type = QDeclarativeMetaType::qmlType(t);
        return type?type->metaObject():0;
    }
}

QT_END_NAMESPACE
