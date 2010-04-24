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

#include "qdeclarativeview.h"

#include <qdeclarative.h>
#include <qdeclarativeitem.h>
#include <qdeclarativeengine.h>
#include <qdeclarativecontext.h>
#include <qdeclarativedebug_p.h>
#include <qdeclarativedebugservice_p.h>
#include <qdeclarativeglobal_p.h>
#include <qdeclarativeguard_p.h>

#include <qscriptvalueiterator.h>
#include <qdebug.h>
#include <qtimer.h>
#include <qevent.h>
#include <qdir.h>
#include <qcoreapplication.h>
#include <qfontdatabase.h>
#include <qicon.h>
#include <qurl.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qgraphicswidget.h>
#include <qbasictimer.h>
#include <QtCore/qabstractanimation.h>
#include <private/qgraphicsview_p.h>
#include <private/qdeclarativeitem_p.h>
#include <private/qdeclarativeitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(frameRateDebug, QML_SHOW_FRAMERATE)

class QDeclarativeViewDebugServer;
class FrameBreakAnimation : public QAbstractAnimation
{
public:
    FrameBreakAnimation(QDeclarativeViewDebugServer *s)
    : QAbstractAnimation((QObject*)s), server(s)
    {
        start();
    }

    virtual int duration() const { return -1; }
    virtual void updateCurrentTime(int msecs);

private:
    QDeclarativeViewDebugServer *server;
};

class QDeclarativeViewDebugServer : public QDeclarativeDebugService
{
public:
    QDeclarativeViewDebugServer(QObject *parent = 0) : QDeclarativeDebugService(QLatin1String("CanvasFrameRate"), parent), breaks(0)
    {
        timer.start();
        new FrameBreakAnimation(this);
    }

    void addTiming(int pe, int tbf)
    {
        if (!isEnabled())
            return;

        bool isFrameBreak = breaks > 1;
        breaks = 0;
        int e = timer.elapsed();
        QByteArray data;
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << (int)pe << (int)tbf << (int)e
           << (bool)isFrameBreak;
        sendMessage(data);
    }

    void frameBreak() { ++breaks; }

private:
    QTime timer;
    int breaks;
};

Q_GLOBAL_STATIC(QDeclarativeViewDebugServer, qfxViewDebugServer);

void FrameBreakAnimation::updateCurrentTime(int msecs)
{
    Q_UNUSED(msecs);
    server->frameBreak();
}

class QDeclarativeViewPrivate : public QDeclarativeItemChangeListener
{
public:
    QDeclarativeViewPrivate(QDeclarativeView *view)
        : q(view), root(0), declarativeItemRoot(0), graphicsWidgetRoot(0), component(0), resizeMode(QDeclarativeView::SizeViewToRootObject) {}
    ~QDeclarativeViewPrivate() { delete root; }
    void execute();
    void itemGeometryChanged(QDeclarativeItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void initResize();
    void updateSize();
    inline QSize rootObjectSize();

    QDeclarativeView *q;

    QDeclarativeGuard<QGraphicsObject> root;
    QDeclarativeGuard<QDeclarativeItem> declarativeItemRoot;
    QDeclarativeGuard<QGraphicsWidget> graphicsWidgetRoot;

    QUrl source;

    QDeclarativeEngine engine;
    QDeclarativeComponent *component;
    QBasicTimer resizetimer;

    QDeclarativeView::ResizeMode resizeMode;
    QTime frameTimer;

    void init();

    QGraphicsScene scene;
};

void QDeclarativeViewPrivate::execute()
{
    if (root) {
        delete root;
        root = 0;
    }
    if (component) {
        delete component;
        component = 0;
    }
    if (!source.isEmpty()) {
        component = new QDeclarativeComponent(&engine, source, q);
        if (!component->isLoading()) {
            q->continueExecute();
        } else {
            QObject::connect(component, SIGNAL(statusChanged(QDeclarativeComponent::Status)), q, SLOT(continueExecute()));
        }
    }
}

void QDeclarativeViewPrivate::itemGeometryChanged(QDeclarativeItem *resizeItem, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (resizeItem == root && resizeMode == QDeclarativeView::SizeViewToRootObject) {
        // wait for both width and height to be changed
        resizetimer.start(0,q);
    }
    QDeclarativeItemChangeListener::itemGeometryChanged(resizeItem, newGeometry, oldGeometry);
}

/*!
    \class QDeclarativeView
  \since 4.7
    \brief The QDeclarativeView class provides a widget for displaying a Qt Declarative user interface.

    Any QGraphicsObject or QDeclarativeItem
    created via QML can be placed on a standard QGraphicsScene and viewed with a standard
    QGraphicsView.

    QDeclarativeView is a QGraphicsView subclass provided as a convenience for displaying QML
    files, and connecting between QML and C++ Qt objects.

    QDeclarativeView performs the following functions:

    \list
    \o Manages QDeclarativeComponent loading and object creation.
    \o Initializes QGraphicsView for optimal performance with QML:
        \list
        \o QGraphicsView::setOptimizationFlags(QGraphicsView::DontSavePainterState);
        \o QGraphicsView::setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
        \o QGraphicsScene::setItemIndexMethod(QGraphicsScene::NoIndex);
        \endlist
    \o Initializes QGraphicsView for QML key handling:
        \list
        \o QGraphicsView::viewport()->setFocusPolicy(Qt::NoFocus);
        \o QGraphicsView::setFocusPolicy(Qt::StrongFocus);
        \o QGraphicsScene::setStickyFocus(true);
        \endlist
    \endlist

    Typical usage:
    \code
    ...
    QDeclarativeView *view = new QDeclarativeView(this);
    vbox->addWidget(view);

    QUrl url = QUrl::fromLocalFile(fileName);
    view->setSource(url);
    view->show();
    \endcode

    To receive errors related to loading and executing QML with QDeclarativeView,
    you can connect to the statusChanged() signal and monitor for QDeclarativeView::Error.
    The errors are available via QDeclarativeView::errors().
*/


/*! \fn void QDeclarativeView::sceneResized(QSize size)
  This signal is emitted when the view is resized to \a size.
*/

/*! \fn void QDeclarativeView::statusChanged(QDeclarativeView::Status status)
    This signal is emitted when the component's current \a status changes.
*/

/*!
  \fn QDeclarativeView::QDeclarativeView(QWidget *parent)
  
  Constructs a QDeclarativeView with the given \a parent.
*/
QDeclarativeView::QDeclarativeView(QWidget *parent)
: QGraphicsView(parent), d(new QDeclarativeViewPrivate(this))
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    d->init();
}

/*!
  \fn QDeclarativeView::QDeclarativeView(const QUrl &source, QWidget *parent)

  Constructs a QDeclarativeView with the given QML \a source and \a parent.
*/
QDeclarativeView::QDeclarativeView(const QUrl &source, QWidget *parent)
: QGraphicsView(parent), d(new QDeclarativeViewPrivate(this))
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    d->init();
    setSource(source);
}

void QDeclarativeViewPrivate::init()
{
    q->setScene(&scene);

    q->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    q->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    q->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    q->setFrameStyle(0);

    // These seem to give the best performance
    q->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    q->viewport()->setFocusPolicy(Qt::NoFocus);
    q->setFocusPolicy(Qt::StrongFocus);

    scene.setStickyFocus(true);  //### needed for correct focus handling
}

/*!
  The destructor clears the view's \l {QGraphicsObject} {items} and
  deletes the internal representation.
 */
QDeclarativeView::~QDeclarativeView()
{
    delete d;
}

/*! \property QDeclarativeView::source
  \brief The URL of the source of the QML component.

  Changing this property causes the QML component to be reloaded.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.
 */

/*!
    Sets the source to the \a url, loads the QML component and instantiates it.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    Calling this methods multiple times with the same url will result
    in the QML being reloaded.
 */
void QDeclarativeView::setSource(const QUrl& url)
{
    d->source = url;
    d->execute();
}

/*!
  Returns the source URL, if set.

  \sa setSource()
 */
QUrl QDeclarativeView::source() const
{
    return d->source;
}

/*!
  Returns a pointer to the QDeclarativeEngine used for instantiating
  QML Components.
 */
QDeclarativeEngine* QDeclarativeView::engine()
{
    return &d->engine;
}

/*!
  This function returns the root of the context hierarchy.  Each QML
  component is instantiated in a QDeclarativeContext.  QDeclarativeContext's are
  essential for passing data to QML components.  In QML, contexts are
  arranged hierarchically and this hierarchy is managed by the
  QDeclarativeEngine.
 */
QDeclarativeContext* QDeclarativeView::rootContext()
{
    return d->engine.rootContext();
}

/*!
  \enum QDeclarativeView::Status
    Specifies the loading status of the QDeclarativeView.

    \value Null This QDeclarativeView has no source set.
    \value Ready This QDeclarativeView has loaded and created the QML component.
    \value Loading This QDeclarativeView is loading network data.
    \value Error An error has occured.  Call errorDescription() to retrieve a description.
*/

/*! \enum QDeclarativeView::ResizeMode

  This enum specifies how to resize the view.

  \value SizeViewToRootObject The view resizes with the root item in the QML.
  \value SizeRootObjectToView The view will automatically resize the root item to the size of the view.
*/

/*!
    \property QDeclarativeView::status
    The component's current \l{QDeclarativeView::Status} {status}.
*/

QDeclarativeView::Status QDeclarativeView::status() const
{
    if (!d->component)
        return QDeclarativeView::Null;

    return QDeclarativeView::Status(d->component->status());
}

/*!
    Return the list of errors that occured during the last compile or create
    operation.  An empty list is returned if isError() is not set.
*/
QList<QDeclarativeError> QDeclarativeView::errors() const
{
    if (d->component)
        return d->component->errors();
    return QList<QDeclarativeError>();
}

/*!
    \property QDeclarativeView::resizeMode
    \brief whether the view should resize the canvas contents

    If this property is set to SizeViewToRootObject (the default), the view
    resizes with the root item in the QML.

    If this property is set to SizeRootObjectToView, the view will
    automatically resize the root item.

    Regardless of this property, the sizeHint of the view
    is the initial size of the root item. Note though that
    since QML may load dynamically, that size may change.
*/

void QDeclarativeView::setResizeMode(ResizeMode mode)
{
    if (d->resizeMode == mode)
        return;

    if (d->declarativeItemRoot) {
        if (d->resizeMode == SizeViewToRootObject) {
            QDeclarativeItemPrivate *p =
                static_cast<QDeclarativeItemPrivate *>(QGraphicsItemPrivate::get(d->declarativeItemRoot));
            p->removeItemChangeListener(d, QDeclarativeItemPrivate::Geometry);
        }
    } else if (d->graphicsWidgetRoot) {
         if (d->resizeMode == QDeclarativeView::SizeViewToRootObject) {
             d->graphicsWidgetRoot->removeEventFilter(this);
         }
    }

    d->resizeMode = mode;
    if (d->root) {
        d->initResize();
    }
}

void QDeclarativeViewPrivate::initResize()
{
    if (declarativeItemRoot) {
        if (resizeMode == QDeclarativeView::SizeViewToRootObject) {
            QDeclarativeItemPrivate *p =
                static_cast<QDeclarativeItemPrivate *>(QGraphicsItemPrivate::get(declarativeItemRoot));
            p->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
        }
    } else if (graphicsWidgetRoot) {
        if (resizeMode == QDeclarativeView::SizeViewToRootObject) {
            graphicsWidgetRoot->installEventFilter(q);
        }
    }
    updateSize();
}

void QDeclarativeViewPrivate::updateSize()
{
    if (!root)
        return;
    if (declarativeItemRoot) {
        if (resizeMode == QDeclarativeView::SizeViewToRootObject) {
            QSize newSize = QSize(declarativeItemRoot->width(), declarativeItemRoot->height());
            if (newSize.isValid() && newSize != q->size()) {
                q->resize(newSize);
            }
        } else if (resizeMode == QDeclarativeView::SizeRootObjectToView) {
            if (!qFuzzyCompare(q->width(), declarativeItemRoot->width()))
                declarativeItemRoot->setWidth(q->width());
            if (!qFuzzyCompare(q->height(), declarativeItemRoot->height()))
                declarativeItemRoot->setHeight(q->height());
        }
    } else if (graphicsWidgetRoot) {
        if (resizeMode == QDeclarativeView::SizeViewToRootObject) {
            QSize newSize = QSize(graphicsWidgetRoot->size().width(), graphicsWidgetRoot->size().height());
            if (newSize.isValid() && newSize != q->size()) {
                q->resize(newSize);
            }
        } else if (resizeMode == QDeclarativeView::SizeRootObjectToView) {
            QSizeF newSize = QSize(q->size().width(), q->size().height());
            if (newSize.isValid() && newSize != graphicsWidgetRoot->size()) {
                graphicsWidgetRoot->resize(newSize);
            }
        }
    }
    q->updateGeometry();
}

QSize QDeclarativeViewPrivate::rootObjectSize()
{
    QSize rootObjectSize(0,0);
    int widthCandidate = -1;
    int heightCandidate = -1;
    if (declarativeItemRoot) {
        widthCandidate = declarativeItemRoot->width();
        heightCandidate = declarativeItemRoot->height();
    } else if (root) {
        QSizeF size = root->boundingRect().size();
        widthCandidate = size.width();
        heightCandidate = size.height();
    }
    if (widthCandidate > 0) {
        rootObjectSize.setWidth(widthCandidate);
    }
    if (heightCandidate > 0) {
        rootObjectSize.setHeight(heightCandidate);
    }
    return rootObjectSize;
}

QDeclarativeView::ResizeMode QDeclarativeView::resizeMode() const
{
    return d->resizeMode;
}

/*!
  \internal
 */
void QDeclarativeView::continueExecute()
{

    disconnect(d->component, SIGNAL(statusChanged(QDeclarativeComponent::Status)), this, SLOT(continueExecute()));

    if (d->component->isError()) {
        QList<QDeclarativeError> errorList = d->component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    QObject *obj = d->component->create();

    if(d->component->isError()) {
        QList<QDeclarativeError> errorList = d->component->errors();
        foreach (const QDeclarativeError &error, errorList) {
            qWarning() << error;
        }
        emit statusChanged(status());
        return;
    }

    setRootObject(obj);
    emit statusChanged(status());
}


/*!
  \internal
*/
void QDeclarativeView::setRootObject(QObject *obj)
{
    if (d->root == obj)
        return;
    if (QDeclarativeItem *declarativeItem = qobject_cast<QDeclarativeItem *>(obj)) {
        d->scene.addItem(declarativeItem);
        d->root = declarativeItem;
        d->declarativeItemRoot = declarativeItem;
    } else if (QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject *>(obj)) {
        d->scene.addItem(graphicsObject);
        d->root = graphicsObject;
        if (graphicsObject->isWidget()) {
            d->graphicsWidgetRoot = static_cast<QGraphicsWidget*>(graphicsObject);
        } else {
            qWarning() << "QDeclarativeView::resizeMode is not honored for components of type QGraphicsObject";
        }
    } else if (obj) {
        qWarning() << "QDeclarativeView only supports loading of root objects that derive from QGraphicsObject";
        if (QWidget* widget  = qobject_cast<QWidget *>(obj)) {
            window()->setAttribute(Qt::WA_OpaquePaintEvent, false);
            window()->setAttribute(Qt::WA_NoSystemBackground, false);
            if (layout() && layout()->count()) {
                // Hide the QGraphicsView in GV mode.
                QLayoutItem *item = layout()->itemAt(0);
                if (item->widget())
                    item->widget()->hide();
            }
            widget->setParent(this);
            if (isVisible()) {
                widget->setVisible(true);
            }
            resize(widget->size());
        }
    }

    if (d->root) {
        QSize initialSize = d->rootObjectSize();
        if (initialSize != size()) {
            resize(initialSize);
        }
        d->initResize();
    }
}

/*!
  \internal
  If the \l {QTimerEvent} {timer event} \a e is this
  view's resize timer, sceneResized() is emitted.
 */
void QDeclarativeView::timerEvent(QTimerEvent* e)
{
    if (!e || e->timerId() == d->resizetimer.timerId()) {
        d->updateSize();
        d->resizetimer.stop();
    }
}

/*! \reimp */
bool QDeclarativeView::eventFilter(QObject *watched, QEvent *e)
{
    if (watched == d->root && d->resizeMode == SizeViewToRootObject) {
        if (d->graphicsWidgetRoot) {
            if (e->type() == QEvent::GraphicsSceneResize) {
                d->updateSize();
            }
        }
    }
    return QGraphicsView::eventFilter(watched, e);
}

/*!
    \internal
    Preferred size follows the root object in
    resize mode SizeViewToRootObject and
    the view in resize mode SizeRootObjectToView.
*/
QSize QDeclarativeView::sizeHint() const
{
    if (d->resizeMode == SizeRootObjectToView) {
        return size();
    } else { // d->resizeMode == SizeViewToRootObject
        return d->rootObjectSize();
    }
}

/*!
  Returns the view's root \l {QGraphicsObject} {item}.
 */
QGraphicsObject *QDeclarativeView::rootObject() const
{
    return d->root;
}

/*!
  \internal
  This function handles the \l {QResizeEvent} {resize event}
  \a e.
 */
void QDeclarativeView::resizeEvent(QResizeEvent *e)
{
    if (d->resizeMode == SizeRootObjectToView) {
        d->updateSize();
    }
    if (d->declarativeItemRoot) {
        setSceneRect(QRectF(0, 0, d->declarativeItemRoot->width(), d->declarativeItemRoot->height()));
    } else if (d->root) {
        setSceneRect(d->root->boundingRect());
    } else {
        setSceneRect(rect());
    }
    emit sceneResized(e->size());
    QGraphicsView::resizeEvent(e);
}

/*!
    \internal
*/
void QDeclarativeView::paintEvent(QPaintEvent *event)
{
    int time = 0;
    if (frameRateDebug() || QDeclarativeViewDebugServer::isDebuggingEnabled())
        time = d->frameTimer.restart();
    QGraphicsView::paintEvent(event);
    if (QDeclarativeViewDebugServer::isDebuggingEnabled())
        qfxViewDebugServer()->addTiming(d->frameTimer.elapsed(), time);
    if (frameRateDebug())
        qDebug() << "paintEvent:" << d->frameTimer.elapsed() << "time since last frame:" << time;
}

QT_END_NAMESPACE
