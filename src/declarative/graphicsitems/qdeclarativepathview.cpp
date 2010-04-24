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

#include "private/qdeclarativepathview_p.h"
#include "private/qdeclarativepathview_p_p.h"

#include <qdeclarativestate_p.h>
#include <qdeclarativeopenmetaobject_p.h>
#include <QDebug>
#include <QEvent>
#include <qlistmodelinterface_p.h>
#include <QGraphicsSceneEvent>

#include <math.h>

QT_BEGIN_NAMESPACE

inline qreal qmlMod(qreal x, qreal y)
{
#ifdef QT_USE_MATH_H_FLOATS
    if(sizeof(qreal) == sizeof(float))
        return fmodf(float(x), float(y));
    else
#endif
        return fmod(x, y);
}

static QDeclarativeOpenMetaObjectType *qPathViewAttachedType = 0;

QDeclarativePathViewAttached::QDeclarativePathViewAttached(QObject *parent)
: QObject(parent), m_view(0), m_onPath(false), m_isCurrent(false)
{
    if (qPathViewAttachedType) {
        m_metaobject = new QDeclarativeOpenMetaObject(this, qPathViewAttachedType);
        m_metaobject->setCached(true);
    } else {
        m_metaobject = new QDeclarativeOpenMetaObject(this);
    }
}

QDeclarativePathViewAttached::~QDeclarativePathViewAttached()
{
}

QVariant QDeclarativePathViewAttached::value(const QByteArray &name) const
{
    return m_metaobject->value(name);
}
void QDeclarativePathViewAttached::setValue(const QByteArray &name, const QVariant &val)
{
    m_metaobject->setValue(name, val);
}

QDeclarativeItem *QDeclarativePathViewPrivate::getItem(int modelIndex)
{
    Q_Q(QDeclarativePathView);
    requestedIndex = modelIndex;
    QDeclarativeItem *item = model->item(modelIndex, false);
    if (item) {
        if (!attType) {
            // pre-create one metatype to share with all attached objects
            attType = new QDeclarativeOpenMetaObjectType(&QDeclarativePathViewAttached::staticMetaObject, qmlEngine(q));
            foreach(const QString &attr, path->attributes()) {
                attType->createProperty(attr.toUtf8());
            }
        }
        qPathViewAttachedType = attType;
        QDeclarativePathViewAttached *att = static_cast<QDeclarativePathViewAttached *>(qmlAttachedPropertiesObject<QDeclarativePathView>(item));
        qPathViewAttachedType = 0;
        if (att) {
            att->m_view = q;
            att->setOnPath(true);
        }
        item->setParentItem(q);
    }
    requestedIndex = -1;
    return item;
}

void QDeclarativePathViewPrivate::releaseItem(QDeclarativeItem *item)
{
    if (!item || !model)
        return;
    if (QDeclarativePathViewAttached *att = attached(item))
        att->setOnPath(false);
    model->release(item);
}

QDeclarativePathViewAttached *QDeclarativePathViewPrivate::attached(QDeclarativeItem *item)
{
    return static_cast<QDeclarativePathViewAttached *>(qmlAttachedPropertiesObject<QDeclarativePathView>(item, false));
}

void QDeclarativePathViewPrivate::clear()
{
    for (int i=0; i<items.count(); i++){
        QDeclarativeItem *p = items[i];
        releaseItem(p);
    }
    items.clear();
}

void QDeclarativePathViewPrivate::updateMappedRange()
{
    if (model && pathItems != -1 && pathItems < model->count())
        mappedRange = qreal(pathItems)/model->count();
    else
        mappedRange = 1.0;
}

qreal QDeclarativePathViewPrivate::positionOfIndex(qreal index) const
{
    qreal pos = -1.0;

    if (model && index >= 0 && index < model->count()) {
        qreal start = 0.0;
        if (haveHighlightRange && highlightRangeMode != QDeclarativePathView::NoHighlightRange)
            start = highlightRangeStart;
        qreal globalPos = index + offset;
        globalPos = qmlMod(globalPos, qreal(model->count())) / model->count();
        if (pathItems != -1 && pathItems < model->count()) {
            globalPos += start * mappedRange;
            globalPos = qmlMod(globalPos, 1.0);
            if (globalPos < mappedRange)
                pos = globalPos / mappedRange;
        } else {
            pos = qmlMod(globalPos + start, 1.0);
        }
    }

    return pos;
}

void QDeclarativePathViewPrivate::createHighlight()
{
    Q_Q(QDeclarativePathView);
    if (!q->isComponentComplete())
        return;

    bool changed = false;
    if (highlightItem) {
        delete highlightItem;
        highlightItem = 0;
        changed = true;
    }

    QDeclarativeItem *item = 0;
    if (highlightComponent) {
        QDeclarativeContext *highlightContext = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = highlightComponent->create(highlightContext);
        if (nobj) {
            QDeclarative_setParent_noEvent(highlightContext, nobj);
            item = qobject_cast<QDeclarativeItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete highlightContext;
        }
    } else {
        item = new QDeclarativeItem;
    }
    if (item) {
        QDeclarative_setParent_noEvent(item, q);
        item->setParentItem(q);
        highlightItem = item;
        changed = true;
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QDeclarativePathViewPrivate::updateHighlight()
{
    Q_Q(QDeclarativePathView);
    if (!q->isComponentComplete() || !isValid())
        return;
    if (highlightItem) {
        if (haveHighlightRange && highlightRangeMode == QDeclarativePathView::StrictlyEnforceRange) {
            updateItem(highlightItem, highlightRangeStart);
        } else {
            qreal target = currentIndex;

            tl.reset(moveHighlight);
            moveHighlight.setValue(highlightPosition);

            const int duration = highlightMoveDuration;

            if (target - highlightPosition > model->count()/2) {
                highlightUp = false;
                qreal distance = model->count() - target + highlightPosition;
                tl.move(moveHighlight, 0.0, QEasingCurve(QEasingCurve::InQuad), int(duration * highlightPosition / distance));
                tl.set(moveHighlight, model->count()-0.01);
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::OutQuad), int(duration * (model->count()-target) / distance));
            } else if (target - highlightPosition <= -model->count()/2) {
                highlightUp = true;
                qreal distance = model->count() - highlightPosition + target;
                tl.move(moveHighlight, model->count()-0.01, QEasingCurve(QEasingCurve::InQuad), int(duration * (model->count()-highlightPosition) / distance));
                tl.set(moveHighlight, 0.0);
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::OutQuad), int(duration * target / distance));
            } else {
                highlightUp = highlightPosition - target < 0;
                tl.move(moveHighlight, target, QEasingCurve(QEasingCurve::InOutQuad), duration);
            }
        }
    }
}

void QDeclarativePathViewPrivate::setHighlightPosition(qreal pos)
{
    if (pos != highlightPosition) {
        qreal start = 0.0;
        qreal end = 1.0;
        if (haveHighlightRange && highlightRangeMode != QDeclarativePathView::NoHighlightRange) {
            start = highlightRangeStart;
            end = highlightRangeEnd;
        }

        qreal range = qreal(model->count());
        // calc normalized position of highlight relative to offset
        qreal relativeHighlight = qmlMod(pos + offset, range) / range;

        if (!highlightUp && relativeHighlight > end * mappedRange) {
            qreal diff = 1.0 - relativeHighlight;
            setOffset(offset + diff * range);
        } else if (highlightUp && relativeHighlight >= (end - start) * mappedRange) {
            qreal diff = relativeHighlight - (end - start) * mappedRange;
            setOffset(offset - diff * range - 0.00001);
        }

        highlightPosition = pos;
        qreal pathPos = positionOfIndex(pos);
        updateItem(highlightItem, pathPos);
        if (QDeclarativePathViewAttached *att = attached(highlightItem))
            att->setOnPath(pathPos != -1.0);
    }
}

void QDeclarativePathViewPrivate::updateItem(QDeclarativeItem *item, qreal percent)
{
    if (QDeclarativePathViewAttached *att = attached(item)) {
        foreach(const QString &attr, path->attributes())
            att->setValue(attr.toUtf8(), path->attributeAt(attr, percent));
    }
    QPointF pf = path->pointAt(percent);
    item->setX(pf.x() - item->width()*item->scale()/2);
    item->setY(pf.y() - item->height()*item->scale()/2);
}

void QDeclarativePathViewPrivate::regenerate()
{
    Q_Q(QDeclarativePathView);
    if (!q->isComponentComplete())
        return;

    clear();

    if (!isValid())
        return;

    firstIndex = -1;
    updateMappedRange();
    q->refill();
}

/*!
    \qmlclass PathView QDeclarativePathView
    \since 4.7
    \brief The PathView element lays out model-provided items on a path.
    \inherits Item

    The model is typically provided by a QAbstractListModel "C++ model object", but can also be created directly in QML.

    The items are laid out along a path defined by a \l Path and may be flicked to scroll.

    \snippet doc/src/snippets/declarative/pathview/pathview.qml 0

    \image pathview.gif

    \bold Note that views do not enable \e clip automatically.  If the view
    is not clipped by another item or the screen, it will be necessary
    to set \e {clip: true} in order to have the out of view items clipped
    nicely.

    \sa Path
*/

QDeclarativePathView::QDeclarativePathView(QDeclarativeItem *parent)
  : QDeclarativeItem(*(new QDeclarativePathViewPrivate), parent)
{
    Q_D(QDeclarativePathView);
    d->init();
}

QDeclarativePathView::~QDeclarativePathView()
{
    Q_D(QDeclarativePathView);
    d->clear();
    if (d->attType)
        d->attType->release();
    if (d->ownModel)
        delete d->model;
}

/*!
    \qmlattachedproperty bool PathView::onPath
    This attached property holds whether the item is currently on the path.

    If a pathItemCount has been set, it is possible that some items may
    be instantiated, but not considered to be currently on the path.
    Usually, these items would be set invisible, for example:

    \code
    Component {
        Rectangle {
            visible: PathView.onPath
            ...
        }
    }
    \endcode

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty bool PathView::isCurrentItem
    This attached property is true if this delegate is the current item; otherwise false.

    It is attached to each instance of the delegate.

    This property may be used to adjust the appearance of the current item.
*/

/*!
    \qmlproperty model PathView::model
    This property holds the model providing data for the view.

    The model provides a set of data that is used to create the items for the view.
    For large or dynamic datasets the model is usually provided by a C++ model object.
    Models can also be created directly in XML, using the ListModel element.

    \sa {qmlmodels}{Data Models}
*/
QVariant QDeclarativePathView::model() const
{
    Q_D(const QDeclarativePathView);
    return d->modelVariant;
}

void QDeclarativePathView::setModel(const QVariant &model)
{
    Q_D(QDeclarativePathView);
    if (d->modelVariant == model)
        return;

    if (d->model) {
        disconnect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        disconnect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        disconnect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        disconnect(d->model, SIGNAL(createdItem(int, QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
        for (int i=0; i<d->items.count(); i++){
            QDeclarativeItem *p = d->items[i];
            d->model->release(p);
        }
        d->items.clear();
    }

    d->modelVariant = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QDeclarativeVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QDeclarativeVisualModel *>(object))) {
        if (d->ownModel) {
            delete d->model;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QDeclarativeVisualDataModel(qmlContext(this));
            d->ownModel = true;
        }
        if (QDeclarativeVisualDataModel *dataModel = qobject_cast<QDeclarativeVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    if (d->model) {
        connect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        connect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        connect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->model, SIGNAL(createdItem(int, QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
    }
    d->offset = qmlMod(d->offset, qreal(d->model->count()));
    if (d->offset < 0)
        d->offset = d->model->count() + d->offset;
    d->regenerate();
    d->fixOffset();
    emit countChanged();
    emit modelChanged();
}

/*!
    \qmlproperty int PathView::count
    This property holds the number of items in the model.
*/
int QDeclarativePathView::count() const
{
    Q_D(const QDeclarativePathView);
    return d->model ? d->model->count() : 0;
}

/*!
    \qmlproperty Path PathView::path
    \default
    This property holds the path used to lay out the items.
    For more information see the \l Path documentation.
*/
QDeclarativePath *QDeclarativePathView::path() const
{
    Q_D(const QDeclarativePathView);
    return d->path;
}

void QDeclarativePathView::setPath(QDeclarativePath *path)
{
    Q_D(QDeclarativePathView);
    if (d->path == path)
        return;
    if (d->path)
        disconnect(d->path, SIGNAL(changed()), this, SLOT(refill()));
    d->path = path;
    connect(d->path, SIGNAL(changed()), this, SLOT(refill()));
    d->clear();
    if (d->attType) {
        d->attType->release();
        d->attType = 0;
    }
    d->regenerate();
    emit pathChanged();
}

/*!
    \qmlproperty int PathView::currentIndex
    This property holds the index of the current item.
*/
int QDeclarativePathView::currentIndex() const
{
    Q_D(const QDeclarativePathView);
    return d->currentIndex;
}

void QDeclarativePathView::setCurrentIndex(int idx)
{
    Q_D(QDeclarativePathView);
    if (d->model && d->model->count())
        idx = qAbs(idx % d->model->count());
    if (d->model && idx != d->currentIndex) {
        if (d->model->count()) {
            int itemIndex = (d->currentIndex - d->firstIndex + d->model->count()) % d->model->count();
            if (itemIndex < d->items.count()) {
                if (QDeclarativeItem *item = d->items.at(itemIndex)) {
                    if (QDeclarativePathViewAttached *att = d->attached(item))
                        att->setIsCurrentItem(false);
                }
            }
        }
        d->currentItem = 0;
        d->moveReason = QDeclarativePathViewPrivate::SetIndex;
        d->currentIndex = idx;
        if (d->model->count()) {
            if (d->haveHighlightRange && d->highlightRangeMode == QDeclarativePathView::StrictlyEnforceRange)
                d->snapToCurrent();
            int itemIndex = (idx - d->firstIndex + d->model->count()) % d->model->count();
            if (itemIndex < d->items.count()) {
                d->currentItem = d->items.at(itemIndex);
                d->currentItem->setFocus(true);
                if (QDeclarativePathViewAttached *att = d->attached(d->currentItem))
                    att->setIsCurrentItem(true);
            }
            d->currentItemOffset = d->positionOfIndex(d->currentIndex);
            d->updateHighlight();
        }
        emit currentIndexChanged();
    }
}

/*!
    \qmlproperty real PathView::offset

    The offset specifies how far along the path the items are from their initial positions.
    This is a real number that ranges from 0.0 to the count of items in the model.
*/
qreal QDeclarativePathView::offset() const
{
    Q_D(const QDeclarativePathView);
    return d->offset;
}

void QDeclarativePathView::setOffset(qreal offset)
{
    Q_D(QDeclarativePathView);
    d->setOffset(offset);
    d->updateCurrent();
}

void QDeclarativePathViewPrivate::setOffset(qreal o)
{
    Q_Q(QDeclarativePathView);
    if (offset != o) {
        if (isValid() && q->isComponentComplete()) {
            offset = qmlMod(o, qreal(model->count()));
            if (offset < 0)
                offset += qreal(model->count());
            q->refill();
        } else {
            offset = o;
        }
        emit q->offsetChanged();
    }
}

/*!
    \qmlproperty component PathView::highlight
    This property holds the component to use as the highlight.

    An instance of the highlight component will be created for each view.
    The geometry of the resultant component instance will be managed by the view
    so as to stay with the current item.

    The below example demonstrates how to make a simple highlight.  Note the use
    of the PathView.onPath property to ensure that the highlight is hidden
    when flicked off of the path.

    \code
    Component {
        Rectangle {
            visible: PathView.onPath
            ...
        }
    }
    \endcode

    \sa highlightItem, highlightRangeMode
*/

QDeclarativeComponent *QDeclarativePathView::highlight() const
{
    Q_D(const QDeclarativePathView);
    return d->highlightComponent;
}

void QDeclarativePathView::setHighlight(QDeclarativeComponent *highlight)
{
    Q_D(QDeclarativePathView);
    if (highlight != d->highlightComponent) {
        d->highlightComponent = highlight;
        d->createHighlight();
        d->updateHighlight();
        emit highlightChanged();
    }
}

/*!
  \qmlproperty Item PathView::highlightItem

  \c highlightItem holds the highlight item, which was created
  from the \l highlight component.

  \sa highlight
*/
QDeclarativeItem *QDeclarativePathView::highlightItem()
{
    Q_D(const QDeclarativePathView);
    return d->highlightItem;
}
/*!
    \qmlproperty real PathView::preferredHighlightBegin
    \qmlproperty real PathView::preferredHighlightEnd
    \qmlproperty enumeration PathView::highlightRangeMode

    These properties set the preferred range of the highlight (current item)
    within the view.  The preferred values must be in the range 0.0-1.0.

    If highlightRangeMode is set to \e ApplyRange the view will
    attempt to maintain the highlight within the range, however
    the highlight can move outside of the range at the ends of the path
    or due to a mouse interaction.

    If highlightRangeMode is set to \e StrictlyEnforceRange the highlight will never
    move outside of the range.  This means that the current item will change
    if a keyboard or mouse action would cause the highlight to move
    outside of the range.

    Note that this is the correct way to influence where the
    current item ends up when the view moves. For example, if you want the
    currently selected item to be in the middle of the path, then set the
    highlight range to be 0.5,0.5 and highlightRangeMode to StrictlyEnforceRange.
    Then, when the path scrolls,
    the currently selected item will be the item at that position. This also applies to
    when the currently selected item changes - it will scroll to within the preferred
    highlight range. Furthermore, the behaviour of the current item index will occur
    whether or not a highlight exists.

    The default value is \e StrictlyEnforceRange.

    Note that a valid range requires preferredHighlightEnd to be greater
    than or equal to preferredHighlightBegin.
*/
qreal QDeclarativePathView::preferredHighlightBegin() const
{
    Q_D(const QDeclarativePathView);
    return d->highlightRangeStart;
}

void QDeclarativePathView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QDeclarativePathView);
    if (d->highlightRangeStart == start || start < 0 || start > 1.0)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightBeginChanged();
}

qreal QDeclarativePathView::preferredHighlightEnd() const
{
    Q_D(const QDeclarativePathView);
    return d->highlightRangeEnd;
}

void QDeclarativePathView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QDeclarativePathView);
    if (d->highlightRangeEnd == end || end < 0 || end > 1.0)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightEndChanged();
}

QDeclarativePathView::HighlightRangeMode QDeclarativePathView::highlightRangeMode() const
{
    Q_D(const QDeclarativePathView);
    return d->highlightRangeMode;
}

void QDeclarativePathView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QDeclarativePathView);
    if (d->highlightRangeMode == mode)
        return;
    d->highlightRangeMode = mode;
    d->haveHighlightRange = d->highlightRangeMode != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}


/*!
    \qmlproperty int PathView::highlightMoveDuration
    This property holds the move animation duration of the highlight delegate.

    If the highlightRangeMode is StrictlyEnforceRange then this property
    determines the speed that the items move along the path.

    The default value for the duration is 300ms.
*/
int QDeclarativePathView::highlightMoveDuration() const
{
    Q_D(const QDeclarativePathView);
    return d->highlightMoveDuration;
}

void QDeclarativePathView::setHighlightMoveDuration(int duration)
{
    Q_D(QDeclarativePathView);
    if (d->highlightMoveDuration == duration)
        return;
    d->highlightMoveDuration = duration;
    emit highlightMoveDurationChanged();
}

/*!
    \qmlproperty real PathView::dragMargin
    This property holds the maximum distance from the path that initiate mouse dragging.

    By default the path can only be dragged by clicking on an item.  If
    dragMargin is greater than zero, a drag can be initiated by clicking
    within dragMargin pixels of the path.
*/
qreal QDeclarativePathView::dragMargin() const
{
    Q_D(const QDeclarativePathView);
    return d->dragMargin;
}

void QDeclarativePathView::setDragMargin(qreal dragMargin)
{
    Q_D(QDeclarativePathView);
    if (d->dragMargin == dragMargin)
        return;
    d->dragMargin = dragMargin;
    emit dragMarginChanged();
}

/*!
    \qmlproperty real PathView::flickDeceleration
    This property holds the rate at which a flick will decelerate.

    The default is 100.
*/
qreal QDeclarativePathView::flickDeceleration() const
{
    Q_D(const QDeclarativePathView);
    return d->deceleration;
}

void QDeclarativePathView::setFlickDeceleration(qreal dec)
{
    Q_D(QDeclarativePathView);
    if (d->deceleration == dec)
        return;
    d->deceleration = dec;
    emit flickDecelerationChanged();
}

/*!
    \qmlproperty bool PathView::interactive

    A user cannot drag or flick a PathView that is not interactive.

    This property is useful for temporarily disabling flicking. This allows
    special interaction with PathView's children.
*/
bool QDeclarativePathView::isInteractive() const
{
    Q_D(const QDeclarativePathView);
    return d->interactive;
}

void QDeclarativePathView::setInteractive(bool interactive)
{
    Q_D(QDeclarativePathView);
    if (interactive != d->interactive) {
        d->interactive = interactive;
        if (!interactive)
            d->tl.clear();
        emit interactiveChanged();
    }
}

/*!
    \qmlproperty component PathView::delegate

    The delegate provides a template defining each item instantiated by the view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.

    Note that the PathView will layout the items based on the size of the root
    item in the delegate.

    Here is an example delegate:
    \snippet doc/src/snippets/declarative/pathview/pathview.qml 1
*/
QDeclarativeComponent *QDeclarativePathView::delegate() const
{
    Q_D(const QDeclarativePathView);
     if (d->model) {
        if (QDeclarativeVisualDataModel *dataModel = qobject_cast<QDeclarativeVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QDeclarativePathView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QDeclarativePathView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QDeclarativeVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QDeclarativeVisualDataModel *dataModel = qobject_cast<QDeclarativeVisualDataModel*>(d->model)) {
        dataModel->setDelegate(delegate);
        d->regenerate();
        emit delegateChanged();
    }
}

/*!
  \qmlproperty int PathView::pathItemCount
  This property holds the number of items visible on the path at any one time.
*/
int QDeclarativePathView::pathItemCount() const
{
    Q_D(const QDeclarativePathView);
    return d->pathItems;
}

void QDeclarativePathView::setPathItemCount(int i)
{
    Q_D(QDeclarativePathView);
    if (i == d->pathItems)
        return;
    if (i < 1)
        i = 1;
    d->pathItems = i;
    if (d->isValid() && isComponentComplete()) {
        d->regenerate();
    }
    emit pathItemCountChanged();
}

QPointF QDeclarativePathViewPrivate::pointNear(const QPointF &point, qreal *nearPercent) const
{
    //XXX maybe do recursively at increasing resolution.
    qreal mindist = 1e10; // big number
    QPointF nearPoint = path->pointAt(0);
    qreal nearPc = 0;
    for (qreal i=1; i < 1000; i++) {
        QPointF pt = path->pointAt(i/1000.0);
        QPointF diff = pt - point;
        qreal dist = diff.x()*diff.x() + diff.y()*diff.y();
        if (dist < mindist) {
            nearPoint = pt;
            nearPc = i;
            mindist = dist;
        }
    }

    if (nearPercent)
        *nearPercent = nearPc / 1000.0;

    return nearPoint;
}


void QDeclarativePathView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativePathView);
    if (!d->interactive || !d->items.count())
        return;
    QPointF scenePoint = mapToScene(event->pos());
    int idx = 0;
    for (; idx < d->items.count(); ++idx) {
        QRectF rect = d->items.at(idx)->boundingRect();
        rect = d->items.at(idx)->mapToScene(rect).boundingRect();
        if (rect.contains(scenePoint))
            break;
    }
    if (idx == d->items.count() && d->dragMargin == 0.)  // didn't click on an item
        return;

    d->startPoint = d->pointNear(event->pos(), &d->startPc);
    if (idx == d->items.count()) {
        qreal distance = qAbs(event->pos().x() - d->startPoint.x()) + qAbs(event->pos().y() - d->startPoint.y());
        if (distance > d->dragMargin)
            return;
    }

    d->stealMouse = false;
    d->lastElapsed = 0;
    d->lastDist = 0;
    QDeclarativeItemPrivate::start(d->lastPosTime);
    d->tl.clear();
}

void QDeclarativePathView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativePathView);
    if (!d->interactive || d->lastPosTime.isNull())
        return;

    if (!d->stealMouse) {
        QPointF delta = event->pos() - d->startPoint;
        if (qAbs(delta.x()) > QApplication::startDragDistance() && qAbs(delta.y()) > QApplication::startDragDistance())
            d->stealMouse = true;
    }

    if (d->stealMouse) {
        d->moveReason = QDeclarativePathViewPrivate::Mouse;
        qreal newPc;
        d->pointNear(event->pos(), &newPc);
        qreal diff = (newPc - d->startPc)*d->model->count()*d->mappedRange;
        if (diff) {
            setOffset(d->offset + diff);

            if (diff > d->model->count()/2)
                diff -= d->model->count();
            else if (diff < -d->model->count()/2)
                diff += d->model->count();

            d->lastElapsed = QDeclarativeItemPrivate::restart(d->lastPosTime);
            d->lastDist = diff;
            d->startPc = newPc;
        }
    }
}

void QDeclarativePathView::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    Q_D(QDeclarativePathView);
    d->stealMouse = false;
    setKeepMouseGrab(false);
    if (!d->interactive || d->lastPosTime.isNull())
        return;

    qreal elapsed = qreal(d->lastElapsed + QDeclarativeItemPrivate::elapsed(d->lastPosTime)) / 1000.;
    qreal velocity = elapsed > 0. ? d->lastDist / elapsed : 0;
    if (d->model && d->model->count() && qAbs(velocity) > 1.) {
        qreal count = d->pathItems == -1 ? d->model->count() : d->pathItems;
        if (qAbs(velocity) > count * 2) // limit velocity
            velocity = (velocity > 0 ? count : -count) * 2;
        // Calculate the distance to be travelled
        qreal v2 = velocity*velocity;
        qreal accel = d->deceleration/10;
        // + 0.25 to encourage moving at least one item in the flick direction
        qreal dist = qMin(qreal(d->model->count()-1), qreal(v2 / (accel * 2.0) + 0.25));
        if (d->haveHighlightRange && d->highlightRangeMode == QDeclarativePathView::StrictlyEnforceRange) {
            // round to nearest item.
            if (velocity > 0.)
                dist = qRound(dist + d->offset) - d->offset;
            else
                dist = qRound(dist - d->offset) + d->offset;
            // Calculate accel required to stop on item boundary
            accel = v2 / (2.0f * qAbs(dist));
        }
        d->moveOffset.setValue(d->offset);
        d->tl.accel(d->moveOffset, velocity, accel, dist);
        d->tl.callback(QDeclarativeTimeLineCallback(&d->moveOffset, d->fixOffsetCallback, d));
    } else {
        d->fixOffset();
    }

    d->lastPosTime = QTime();
    ungrabMouse();
}

bool QDeclarativePathView::sendMouseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativePathView);
    QGraphicsSceneMouseEvent mouseEvent(event->type());
    QRectF myRect = mapToScene(QRectF(0, 0, width(), height())).boundingRect();
    QGraphicsScene *s = scene();
    QDeclarativeItem *grabber = s ? qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem()) : 0;
    if ((d->stealMouse || myRect.contains(event->scenePos().toPoint())) && (!grabber || !grabber->keepMouseGrab())) {
        mouseEvent.setAccepted(false);
        for (int i = 0x1; i <= 0x10; i <<= 1) {
            if (event->buttons() & i) {
                Qt::MouseButton button = Qt::MouseButton(i);
                mouseEvent.setButtonDownPos(button, mapFromScene(event->buttonDownPos(button)));
            }
        }
        mouseEvent.setScenePos(event->scenePos());
        mouseEvent.setLastScenePos(event->lastScenePos());
        mouseEvent.setPos(mapFromScene(event->scenePos()));
        mouseEvent.setLastPos(mapFromScene(event->lastScenePos()));

        switch(mouseEvent.type()) {
        case QEvent::GraphicsSceneMouseMove:
            mouseMoveEvent(&mouseEvent);
            break;
        case QEvent::GraphicsSceneMousePress:
            mousePressEvent(&mouseEvent);
            break;
        case QEvent::GraphicsSceneMouseRelease:
            mouseReleaseEvent(&mouseEvent);
            break;
        default:
            break;
        }
        grabber = qobject_cast<QDeclarativeItem*>(s->mouseGrabberItem());
        if (grabber && d->stealMouse && !grabber->keepMouseGrab() && grabber != this)
            grabMouse();

        return d->stealMouse;
    } else if (!d->lastPosTime.isNull()) {
        d->lastPosTime = QTime();
    }
    return false;
}

bool QDeclarativePathView::sceneEventFilter(QGraphicsItem *i, QEvent *e)
{
    Q_D(QDeclarativePathView);
    if (!isVisible() || !d->interactive)
        return QDeclarativeItem::sceneEventFilter(i, e);

    switch (e->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
        {
            bool ret = sendMouseEvent(static_cast<QGraphicsSceneMouseEvent *>(e));
            if (e->type() == QEvent::GraphicsSceneMouseRelease)
                return ret;
            break;
        }
    default:
        break;
    }

    return QDeclarativeItem::sceneEventFilter(i, e);
}

void QDeclarativePathView::componentComplete()
{
    Q_D(QDeclarativePathView);
    QDeclarativeItem::componentComplete();
    d->createHighlight();
    d->regenerate();
    d->updateHighlight();
}

void QDeclarativePathView::refill()
{
    Q_D(QDeclarativePathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    bool currentVisible = false;

    // first move existing items and remove items off path
    int idx = d->firstIndex;
    QList<QDeclarativeItem*>::iterator it = d->items.begin();
    while (it != d->items.end()) {
        qreal pos = d->positionOfIndex(idx);
        QDeclarativeItem *item = *it;
        if (pos >= 0.0) {
            d->updateItem(item, pos);
            if (idx == d->currentIndex) {
                currentVisible = true;
                d->currentItemOffset = pos;
            }
            ++it;
        } else {
//            qDebug() << "release";
            d->updateItem(item, 1.0);
            d->releaseItem(item);
            if (it == d->items.begin()) {
                if (++d->firstIndex >= d->model->count())
                    d->firstIndex = 0;
            }
            it = d->items.erase(it);
        }
        ++idx;
        if (idx >= d->model->count())
            idx = 0;
    }

    // add items to beginning and end
    int count = d->pathItems == -1 ? d->model->count() : qMin(d->pathItems, d->model->count());
    if (d->items.count() < count) {
        int idx = qRound(d->model->count() - d->offset) % d->model->count();
        qreal startPos = 0.0;
        if (d->haveHighlightRange && d->highlightRangeMode != QDeclarativePathView::NoHighlightRange)
            startPos = d->highlightRangeStart;
        if (d->firstIndex >= 0) {
            startPos = d->positionOfIndex(d->firstIndex);
            idx = (d->firstIndex + d->items.count()) % d->model->count();
        }
        qreal pos = d->positionOfIndex(idx);
        while ((pos > startPos || !d->items.count()) && d->items.count() < count) {
//            qDebug() << "append" << idx;
            QDeclarativeItem *item = d->getItem(idx);
            item->setZValue(idx+1);
            if (d->currentIndex == idx) {
                item->setFocus(true);
                if (QDeclarativePathViewAttached *att = d->attached(item))
                    att->setIsCurrentItem(true);
                currentVisible = true;
                d->currentItemOffset = pos;
                d->currentItem = item;
            }
            if (d->items.count() == 0)
                d->firstIndex = idx;
            d->items.append(item);
            d->updateItem(item, pos);
            d->model->completeItem();
            ++idx;
            if (idx >= d->model->count())
                idx = 0;
            pos = d->positionOfIndex(idx);
        }

        idx = d->firstIndex - 1;
        if (idx < 0)
            idx = d->model->count() - 1;
        pos = d->positionOfIndex(idx);
        while (pos >= 0.0 && pos < startPos) {
//            qDebug() << "prepend" << idx;
            QDeclarativeItem *item = d->getItem(idx);
            item->setZValue(idx+1);
            if (d->currentIndex == idx) {
                item->setFocus(true);
                if (QDeclarativePathViewAttached *att = d->attached(item))
                    att->setIsCurrentItem(true);
                currentVisible = true;
                d->currentItemOffset = pos;
                d->currentItem = item;
            }
            d->items.prepend(item);
            d->updateItem(item, pos);
            d->model->completeItem();
            d->firstIndex = idx;
            idx = d->firstIndex - 1;
            if (idx < 0)
                idx = d->model->count() - 1;
            pos = d->positionOfIndex(idx);
        }
    }

    if (!currentVisible)
        d->currentItemOffset = 1.0;

    if (d->highlightItem && d->haveHighlightRange && d->highlightRangeMode == QDeclarativePathView::StrictlyEnforceRange) {
        d->updateItem(d->highlightItem, d->highlightRangeStart);
        if (QDeclarativePathViewAttached *att = d->attached(d->highlightItem))
            att->setOnPath(true);
    } else if (d->highlightItem && d->moveReason != QDeclarativePathViewPrivate::SetIndex) {
        d->updateItem(d->highlightItem, d->currentItemOffset);
        if (QDeclarativePathViewAttached *att = d->attached(d->highlightItem))
            att->setOnPath(currentVisible);
    }
}

void QDeclarativePathView::itemsInserted(int modelIndex, int count)
{
    //XXX support animated insertion
    Q_D(QDeclarativePathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    QList<QDeclarativeItem *> removedItems = d->items;
    d->items.clear();
    if (modelIndex <= d->currentIndex) {
        d->currentIndex += count;
        emit currentIndexChanged();
    }
    d->regenerate();
    while (removedItems.count())
        d->releaseItem(removedItems.takeLast());
    d->updateCurrent();
    emit countChanged();
}

void QDeclarativePathView::itemsRemoved(int modelIndex, int count)
{
    //XXX support animated removal
    Q_D(QDeclarativePathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    // fix current
    bool currentChanged = false;
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        currentChanged = true;
    } else if (d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count) {
        // current item has been removed.
        d->currentIndex = qMin(modelIndex, d->model->count()-1);
        if (d->currentItem) {
            if (QDeclarativePathViewAttached *att = d->attached(d->currentItem))
                att->setIsCurrentItem(true);
        }
        currentChanged = true;
    }

    QList<QDeclarativeItem *> removedItems = d->items;
    d->items.clear();
    if (d->offset >= d->model->count())
        d->offset = d->model->count() - 1;

    d->regenerate();
    while (removedItems.count())
        d->releaseItem(removedItems.takeLast());
    d->updateCurrent();
    if (currentChanged)
        emit currentIndexChanged();
    emit countChanged();
}

void QDeclarativePathView::itemsMoved(int /*from*/, int /*to*/, int /*count*/)
{
    Q_D(QDeclarativePathView);
    if (!d->isValid() || !isComponentComplete())
        return;

    QList<QDeclarativeItem *> removedItems = d->items;
    d->items.clear();
    d->regenerate();
    while (removedItems.count())
        d->releaseItem(removedItems.takeLast());

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem, this);
        if (oldCurrent != d->currentIndex)
            emit currentIndexChanged();
    }
    d->updateCurrent();
}

void QDeclarativePathView::modelReset()
{
    Q_D(QDeclarativePathView);
    d->regenerate();
    emit countChanged();
}

void QDeclarativePathView::createdItem(int index, QDeclarativeItem *item)
{
    Q_D(QDeclarativePathView);
    if (d->requestedIndex != index) {
        item->setParentItem(this);
        d->updateItem(item, index < d->firstIndex ? 0.0 : 1.0);
    }
}

void QDeclarativePathView::destroyingItem(QDeclarativeItem *item)
{
    Q_UNUSED(item);
}

void QDeclarativePathView::ticked()
{
    Q_D(QDeclarativePathView);
    d->updateCurrent();
}

// find the item closest to the snap position
int QDeclarativePathViewPrivate::calcCurrentIndex()
{
    int current = -1;
    if (model && items.count()) {
        offset = qmlMod(offset, model->count());
        if (offset < 0)
            offset += model->count();
        current = qRound(qAbs(qmlMod(model->count() - offset, model->count())));
    }

    return current;
}

void QDeclarativePathViewPrivate::updateCurrent()
{
    Q_Q(QDeclarativePathView);
    if (moveReason != Mouse)
        return;
    if (!haveHighlightRange || highlightRangeMode != QDeclarativePathView::StrictlyEnforceRange)
        return;

    int idx = calcCurrentIndex();
    if (model && idx != currentIndex) {
        int itemIndex = (currentIndex - firstIndex + model->count()) % model->count();
        if (itemIndex < items.count()) {
            if (QDeclarativeItem *item = items.at(itemIndex)) {
                if (QDeclarativePathViewAttached *att = attached(item))
                    att->setIsCurrentItem(false);
            }
        }
        currentIndex = idx;
        currentItem = 0;
        itemIndex = (idx - firstIndex + model->count()) % model->count();
        if (itemIndex < items.count()) {
            currentItem = items.at(itemIndex);
            currentItem->setFocus(true);
            if (QDeclarativePathViewAttached *att = attached(currentItem))
                att->setIsCurrentItem(true);
        }
        emit q->currentIndexChanged();
    }
}

void QDeclarativePathViewPrivate::fixOffsetCallback(void *d)
{
    ((QDeclarativePathViewPrivate *)d)->fixOffset();
}

void QDeclarativePathViewPrivate::fixOffset()
{
    Q_Q(QDeclarativePathView);
    if (model && items.count()) {
        if (haveHighlightRange && highlightRangeMode == QDeclarativePathView::StrictlyEnforceRange) {
            int curr = calcCurrentIndex();
            if (curr != currentIndex)
                q->setCurrentIndex(curr);
            else
                snapToCurrent();
        }
    }
}

void QDeclarativePathViewPrivate::snapToCurrent()
{
    if (!model || model->count() <= 0)
        return;

    qreal targetOffset = model->count() - currentIndex;

    moveReason = Other;
    tl.reset(moveOffset);
    moveOffset.setValue(offset);

    const int duration = highlightMoveDuration;

    if (targetOffset - offset > model->count()/2) {
        qreal distance = model->count() - targetOffset + offset;
        tl.move(moveOffset, 0.0, QEasingCurve(QEasingCurve::InQuad), int(duration * offset / distance));
        tl.set(moveOffset, model->count());
        tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::OutQuad), int(duration * (model->count()-targetOffset) / distance));
    } else if (targetOffset - offset <= -model->count()/2) {
        qreal distance = model->count() - offset + targetOffset;
        tl.move(moveOffset, model->count(), QEasingCurve(QEasingCurve::InQuad), int(duration * (model->count()-offset) / distance));
        tl.set(moveOffset, 0.0);
        tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::OutQuad), int(duration * targetOffset / distance));
    } else {
        tl.move(moveOffset, targetOffset, QEasingCurve(QEasingCurve::InOutQuad), duration);
    }
}

QDeclarativePathViewAttached *QDeclarativePathView::qmlAttachedProperties(QObject *obj)
{
    return new QDeclarativePathViewAttached(obj);
}

QT_END_NAMESPACE

