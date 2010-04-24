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

#include "private/qdeclarativeborderimage_p.h"
#include "private/qdeclarativeborderimage_p_p.h"

#include <qdeclarativeengine.h>
#include <qdeclarativeinfo.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass BorderImage QDeclarativeBorderImage
    \brief The BorderImage element provides an image that can be used as a border.
    \inherits Item
    \since 4.7

    \snippet snippets/declarative/border-image.qml 0

    \image BorderImage.png
 */

/*!
    \class QDeclarativeBorderImage BorderImage
    \internal
    \brief The QDeclarativeBorderImage class provides an image item that you can add to a QDeclarativeView.
*/

QDeclarativeBorderImage::QDeclarativeBorderImage(QDeclarativeItem *parent)
  : QDeclarativeImageBase(*(new QDeclarativeBorderImagePrivate), parent)
{
}

QDeclarativeBorderImage::~QDeclarativeBorderImage()
{
    Q_D(QDeclarativeBorderImage);
    if (d->sciReply)
        d->sciReply->deleteLater();
    if (d->sciPendingPixmapCache)
        QDeclarativePixmapCache::cancel(d->sciurl, this);
}
/*!
    \qmlproperty enumeration BorderImage::status

    This property holds the status of image loading.  It can be one of:
    \list
    \o Null - no image has been set
    \o Ready - the image has been loaded
    \o Loading - the image is currently being loaded
    \o Error - an error occurred while loading the image
    \endlist

    \sa progress
*/

/*!
    \qmlproperty real BorderImage::progress

    This property holds the progress of image loading, from 0.0 (nothing loaded)
    to 1.0 (finished).

    \sa status
*/

/*!
    \qmlproperty bool BorderImage::smooth

    Set this property if you want the image to be smoothly filtered when scaled or
    transformed.  Smooth filtering gives better visual quality, but is slower.  If
    the image is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the image is stationary on
    the screen.  A common pattern when animating an image is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
    \qmlproperty url BorderImage::source

    BorderImage can handle any image format supported by Qt, loaded from any URL scheme supported by Qt.

    It can also handle .sci files, which are a Qml-specific format. A .sci file uses a simple text-based format that specifies
    the borders, the image file and the tile rules.

    The following .sci file sets the borders to 10 on each side for the image \c picture.png:
    \qml
    border.left: 10
    border.top: 10
    border.bottom: 10
    border.right: 10
    source: picture.png
    \endqml

    The URL may be absolute, or relative to the URL of the component.
*/

static QString toLocalFileOrQrc(const QUrl& url)
{
    QString r = url.toLocalFile();
    if (r.isEmpty() && url.scheme() == QLatin1String("qrc"))
        r = QLatin1Char(':') + url.path();
    return r;
}


void QDeclarativeBorderImage::setSource(const QUrl &url)
{
    Q_D(QDeclarativeBorderImage);
    //equality is fairly expensive, so we bypass for simple, common case
    if ((d->url.isEmpty() == url.isEmpty()) && url == d->url)
        return;

    if (d->sciReply) {
        d->sciReply->deleteLater();
        d->sciReply = 0;
    }

    if (d->pendingPixmapCache) {
        QDeclarativePixmapCache::cancel(d->url, this);
        d->pendingPixmapCache = false;
    }
    if (d->sciPendingPixmapCache) {
        QDeclarativePixmapCache::cancel(d->sciurl, this);
        d->sciPendingPixmapCache = false;
    }

    d->url = url;
    d->sciurl = QUrl();
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QDeclarativeBorderImage::load()
{
    Q_D(QDeclarativeBorderImage);
    if (d->progress != 0.0) {
        d->progress = 0.0;
        emit progressChanged(d->progress);
    }

    if (d->url.isEmpty()) {
        d->pix = QPixmap();
        d->status = Null;
        setImplicitWidth(0);
        setImplicitHeight(0);
        emit statusChanged(d->status);
        update();
    } else {
        d->status = Loading;
        if (d->url.path().endsWith(QLatin1String("sci"))) {
#ifndef QT_NO_LOCALFILE_OPTIMIZED_QML
            QString lf = toLocalFileOrQrc(d->url);
            if (!lf.isEmpty()) {
                QFile file(lf);
                file.open(QIODevice::ReadOnly);
                setGridScaledImage(QDeclarativeGridScaledImage(&file));
            } else
#endif
            {
                QNetworkRequest req(d->url);
                d->sciReply = qmlEngine(this)->networkAccessManager()->get(req);

                static int sciReplyFinished = -1;
                static int thisSciRequestFinished = -1;
                if (sciReplyFinished == -1) {
                    sciReplyFinished =
                        QNetworkReply::staticMetaObject.indexOfSignal("finished()");
                    thisSciRequestFinished =
                        QDeclarativeBorderImage::staticMetaObject.indexOfSlot("sciRequestFinished()");
                }

                QMetaObject::connect(d->sciReply, sciReplyFinished, this,
                                     thisSciRequestFinished, Qt::DirectConnection);
            }
        } else {
            QSize impsize;
            QString errorString;
            QDeclarativePixmapReply::Status status = QDeclarativePixmapCache::get(d->url, &d->pix, &errorString, &impsize, d->async);
            if (status != QDeclarativePixmapReply::Ready && status != QDeclarativePixmapReply::Error) {
                QDeclarativePixmapReply *reply = QDeclarativePixmapCache::request(qmlEngine(this), d->url);
                d->pendingPixmapCache = true;
                connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
                connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                        this, SLOT(requestProgress(qint64,qint64)));
            } else {
                //### should be unified with requestFinished
                setImplicitWidth(impsize.width());
                setImplicitHeight(impsize.height());

                if (d->pix.isNull()) {
                    d->status = Error;
                    qmlInfo(this) << errorString;
                }
                if (d->status == Loading)
                    d->status = Ready;
                d->progress = 1.0;
                emit statusChanged(d->status);
                emit progressChanged(d->progress);
                update();
            }
        }
    }

    emit statusChanged(d->status);
}

/*!
    \qmlproperty int BorderImage::border.left
    \qmlproperty int BorderImage::border.right
    \qmlproperty int BorderImage::border.top
    \qmlproperty int BorderImage::border.bottom

    \target ImagexmlpropertiesscaleGrid

    The 4 border lines (2 horizontal and 2 vertical) break an image into 9 sections, as shown below:

    \image declarative-scalegrid.png

    When the image is scaled:
    \list
    \i the corners (sections 1, 3, 7, and 9) are not scaled at all
    \i sections 2 and 8 are scaled according to \l{BorderImage::horizontalTileMode}{horizontalTileMode}
    \i sections 4 and 6 are scaled according to \l{BorderImage::verticalTileMode}{verticalTileMode}
    \i the middle (section 5) is scaled according to both \l{BorderImage::horizontalTileMode}{horizontalTileMode} and \l{BorderImage::verticalTileMode}{verticalTileMode}
    \endlist

    Each border line (left, right, top, and bottom) specifies an offset from the respective side. For example, \c{border.bottom: 10} sets the bottom line 10 pixels up from the bottom of the image.

    The border lines can also be specified using a
    \l {BorderImage::source}{.sci file}.
*/

QDeclarativeScaleGrid *QDeclarativeBorderImage::border()
{
    Q_D(QDeclarativeBorderImage);
    return d->getScaleGrid();
}

/*!
    \qmlproperty enumeration BorderImage::horizontalTileMode
    \qmlproperty enumeration BorderImage::verticalTileMode

    This property describes how to repeat or stretch the middle parts of the border image.

    \list
    \o Stretch - Scale the image to fit to the available area.
    \o Repeat - Tile the image until there is no more space. May crop the last image.
    \o Round - Like Repeat, but scales the images down to ensure that the last image is not cropped.
    \endlist
*/
QDeclarativeBorderImage::TileMode QDeclarativeBorderImage::horizontalTileMode() const
{
    Q_D(const QDeclarativeBorderImage);
    return d->horizontalTileMode;
}

void QDeclarativeBorderImage::setHorizontalTileMode(TileMode t)
{
    Q_D(QDeclarativeBorderImage);
    if (t != d->horizontalTileMode) {
        d->horizontalTileMode = t;
        emit horizontalTileModeChanged();
        update();
    }
}

QDeclarativeBorderImage::TileMode QDeclarativeBorderImage::verticalTileMode() const
{
    Q_D(const QDeclarativeBorderImage);
    return d->verticalTileMode;
}

void QDeclarativeBorderImage::setVerticalTileMode(TileMode t)
{
    Q_D(QDeclarativeBorderImage);
    if (t != d->verticalTileMode) {
        d->verticalTileMode = t;
        emit verticalTileModeChanged();
        update();
    }
}

void QDeclarativeBorderImage::setGridScaledImage(const QDeclarativeGridScaledImage& sci)
{
    Q_D(QDeclarativeBorderImage);
    if (!sci.isValid()) {
        d->status = Error;
        emit statusChanged(d->status);
    } else {
        QDeclarativeScaleGrid *sg = border();
        sg->setTop(sci.gridTop());
        sg->setBottom(sci.gridBottom());
        sg->setLeft(sci.gridLeft());
        sg->setRight(sci.gridRight());
        d->horizontalTileMode = sci.horizontalTileRule();
        d->verticalTileMode = sci.verticalTileRule();

        d->sciurl = d->url.resolved(QUrl(sci.pixmapUrl()));
        QSize impsize;
        QString errorString;
        QDeclarativePixmapReply::Status status = QDeclarativePixmapCache::get(d->sciurl, &d->pix, &errorString, &impsize, d->async);
        if (status != QDeclarativePixmapReply::Ready && status != QDeclarativePixmapReply::Error) {
            QDeclarativePixmapReply *reply = QDeclarativePixmapCache::request(qmlEngine(this), d->sciurl);
            d->sciPendingPixmapCache = true;

            static int replyDownloadProgress = -1;
            static int replyFinished = -1;
            static int thisRequestProgress = -1;
            static int thisRequestFinished = -1;
            if (replyDownloadProgress == -1) {
                replyDownloadProgress =
                    QDeclarativePixmapReply::staticMetaObject.indexOfSignal("downloadProgress(qint64,qint64)");
                replyFinished =
                    QDeclarativePixmapReply::staticMetaObject.indexOfSignal("finished()");
                thisRequestProgress =
                    QDeclarativeBorderImage::staticMetaObject.indexOfSlot("requestProgress(qint64,qint64)");
                thisRequestFinished =
                    QDeclarativeBorderImage::staticMetaObject.indexOfSlot("requestFinished()");
            }

            QMetaObject::connect(reply, replyFinished, this,
                                 thisRequestFinished, Qt::DirectConnection);
            QMetaObject::connect(reply, replyDownloadProgress, this,
                                 thisRequestProgress, Qt::DirectConnection);
        } else {
            //### should be unified with requestFinished
            setImplicitWidth(impsize.width());
            setImplicitHeight(impsize.height());

            if (d->pix.isNull()) {
                d->status = Error;
                qmlInfo(this) << errorString;
            }
            if (d->status == Loading)
                d->status = Ready;
            d->progress = 1.0;
            emit statusChanged(d->status);
            emit progressChanged(1.0);
            update();
        }
    }
}

void QDeclarativeBorderImage::requestFinished()
{
    Q_D(QDeclarativeBorderImage);

    QSize impsize;
    if (d->url.path().endsWith(QLatin1String(".sci"))) {
        d->sciPendingPixmapCache = false;
        QString errorString;
        if (QDeclarativePixmapCache::get(d->sciurl, &d->pix, &errorString, &impsize, d->async) != QDeclarativePixmapReply::Ready) {
            d->status = Error;
            qmlInfo(this) << errorString;
        }
    } else {
        d->pendingPixmapCache = false;
        QString errorString;
        if (QDeclarativePixmapCache::get(d->url, &d->pix, &errorString, &impsize, d->async) != QDeclarativePixmapReply::Ready) {
            d->status = Error;
            qmlInfo(this) << errorString;
        }
    }
    setImplicitWidth(impsize.width());
    setImplicitHeight(impsize.height());

    if (d->status == Loading)
        d->status = Ready;
    d->progress = 1.0;
    emit statusChanged(d->status);
    emit progressChanged(1.0);
    update();
}

#define BORDERIMAGE_MAX_REDIRECT 16

void QDeclarativeBorderImage::sciRequestFinished()
{
    Q_D(QDeclarativeBorderImage);

    d->redirectCount++;
    if (d->redirectCount < BORDERIMAGE_MAX_REDIRECT) {
        QVariant redirect = d->sciReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = d->sciReply->url().resolved(redirect.toUrl());
            setSource(url);
            return;
        }
    }
    d->redirectCount=0;

    if (d->sciReply->error() != QNetworkReply::NoError) {
        d->status = Error;
        d->sciReply->deleteLater();
        d->sciReply = 0;
        emit statusChanged(d->status);
    } else {
        QDeclarativeGridScaledImage sci(d->sciReply);
        d->sciReply->deleteLater();
        d->sciReply = 0;
        setGridScaledImage(sci);
    }
}

void QDeclarativeBorderImage::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    Q_D(QDeclarativeBorderImage);
    if (d->pix.isNull())
        return;

    bool oldAA = p->testRenderHint(QPainter::Antialiasing);
    bool oldSmooth = p->testRenderHint(QPainter::SmoothPixmapTransform);
    if (d->smooth)
        p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, d->smooth);

    const QDeclarativeScaleGrid *border = d->getScaleGrid();
    QMargins margins(border->left(), border->top(), border->right(), border->bottom());
    QTileRules rules((Qt::TileRule)d->horizontalTileMode, (Qt::TileRule)d->verticalTileMode);
    qDrawBorderPixmap(p, QRect(0, 0, (int)d->width(), (int)d->height()), margins, d->pix, d->pix.rect(), margins, rules);
    if (d->smooth) {
        p->setRenderHint(QPainter::Antialiasing, oldAA);
        p->setRenderHint(QPainter::SmoothPixmapTransform, oldSmooth);
    }
}

QT_END_NAMESPACE
