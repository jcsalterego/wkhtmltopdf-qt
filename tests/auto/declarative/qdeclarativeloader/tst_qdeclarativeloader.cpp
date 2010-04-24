/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qtest.h>
#include <QtGui/QGraphicsWidget>
#include <QtGui/QGraphicsScene>

#include <QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <private/qdeclarativeloader_p.h>
#include "testhttpserver.h"

#define SERVER_PORT 14450

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

#define TRY_WAIT(expr) \
    do { \
        for (int ii = 0; ii < 6; ++ii) { \
            if ((expr)) break; \
            QTest::qWait(50); \
        } \
        QVERIFY((expr)); \
    } while (false)

class tst_QDeclarativeLoader : public QObject

{
    Q_OBJECT
public:
    tst_QDeclarativeLoader();

private slots:
    void url();
    void invalidUrl();
    void component();
    void clear();
    void urlToComponent();
    void componentToUrl();
    void sizeLoaderToItem();
    void sizeItemToLoader();
    void noResize();
    void sizeLoaderToGraphicsWidget();
    void sizeGraphicsWidgetToLoader();
    void noResizeGraphicsWidget();
    void networkRequestUrl();
    void failNetworkRequest();
//    void networkComponent();

    void deleteComponentCrash();
    void nonItem();
    void vmeErrors();

private:
    QDeclarativeEngine engine;
};


tst_QDeclarativeLoader::tst_QDeclarativeLoader()
{
}

void tst_QDeclarativeLoader::url()
{
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import Qt 4.7\nLoader { source: \"Rect120x60.qml\" }"), TEST_FILE(""));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);
    QVERIFY(loader->item());
    QVERIFY(loader->source() == QUrl::fromLocalFile(SRCDIR "/data/Rect120x60.qml"));
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QDeclarativeLoader::Ready);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

    delete loader;
}

void tst_QDeclarativeLoader::component()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SetSourceComponent.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item);

    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(1)); 
    QVERIFY(loader);
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QDeclarativeLoader::Ready);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

    QDeclarativeComponent *c = qobject_cast<QDeclarativeComponent*>(item->QGraphicsObject::children().at(0));
    QVERIFY(c);
    QCOMPARE(loader->sourceComponent(), c);

    delete item;
}

void tst_QDeclarativeLoader::invalidUrl()
{
    QTest::ignoreMessage(QtWarningMsg, QString("<Unknown File>: File error for URL " + QUrl::fromLocalFile(SRCDIR "/data/IDontExist.qml").toString()).toUtf8().constData());

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import Qt 4.7\nLoader { source: \"IDontExist.qml\" }"), TEST_FILE(""));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);
    QVERIFY(loader->item() == 0);
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QDeclarativeLoader::Error);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 0);

    delete loader;
}

void tst_QDeclarativeLoader::clear()
{
    {
        QDeclarativeComponent component(&engine);
        component.setData(QByteArray(
                    "import Qt 4.7\n"
                    " Loader { id: loader\n"
                    "  source: 'Rect120x60.qml'\n"
                    "  Timer { interval: 200; running: true; onTriggered: loader.source = '' }\n"
                    " }")
                , TEST_FILE(""));
        QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
        QVERIFY(loader != 0);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

        QTest::qWait(500);

        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QDeclarativeLoader::Null);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 0);

        delete loader;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("/SetSourceComponent.qml"));
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
        QVERIFY(item);

        QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(1)); 
        QVERIFY(loader);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

        loader->setSourceComponent(0);

        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QDeclarativeLoader::Null);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 0);

        delete item;
    }
    {
        QDeclarativeComponent component(&engine, TEST_FILE("/SetSourceComponent.qml"));
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
        QVERIFY(item);

        QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(1)); 
        QVERIFY(loader);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

        QMetaObject::invokeMethod(item, "clear");

        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QDeclarativeLoader::Null);
        QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 0);

        delete item;
    }
}

void tst_QDeclarativeLoader::urlToComponent()
{
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import Qt 4.7\n"
                "Loader {\n"
                " id: loader\n"
                " Component { id: myComp; Rectangle { width: 10; height: 10 } }\n"
                " source: \"Rect120x60.qml\"\n"
                " Timer { interval: 100; running: true; onTriggered: loader.sourceComponent = myComp }\n"
                "}" )
            , TEST_FILE(""));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QTest::qWait(500);
    QVERIFY(loader != 0);
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);
    QCOMPARE(loader->width(), 10.0);
    QCOMPARE(loader->height(), 10.0);

    delete loader;
}

void tst_QDeclarativeLoader::componentToUrl()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SetSourceComponent.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item);

    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(1)); 
    QVERIFY(loader);
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

    loader->setSource(TEST_FILE("/Rect120x60.qml"));
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);
    QCOMPARE(loader->width(), 120.0);
    QCOMPARE(loader->height(), 60.0);

    delete item;
}

void tst_QDeclarativeLoader::sizeLoaderToItem()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SizeToItem.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);
    QVERIFY(loader->resizeMode() == QDeclarativeLoader::SizeLoaderToItem);
    QCOMPARE(loader->width(), 120.0);
    QCOMPARE(loader->height(), 60.0);

    // Check resize
    QDeclarativeItem *rect = qobject_cast<QDeclarativeItem*>(loader->item());
    QVERIFY(rect);
    rect->setWidth(150);
    rect->setHeight(45);
    QCOMPARE(loader->width(), 150.0);
    QCOMPARE(loader->height(), 45.0);

    // Switch mode
    loader->setResizeMode(QDeclarativeLoader::SizeItemToLoader);
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(rect->width(), 180.0);
    QCOMPARE(rect->height(), 30.0);

    // notify
    QSignalSpy spy(loader, SIGNAL(resizeModeChanged()));
    loader->setResizeMode(QDeclarativeLoader::NoResize);
    QCOMPARE(spy.count(),1);
    loader->setResizeMode(QDeclarativeLoader::NoResize);
    QCOMPARE(spy.count(),1);

    delete loader;
}

void tst_QDeclarativeLoader::sizeItemToLoader()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SizeToLoader.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);
    QVERIFY(loader->resizeMode() == QDeclarativeLoader::SizeItemToLoader);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(loader->height(), 80.0);

    QDeclarativeItem *rect = qobject_cast<QDeclarativeItem*>(loader->item());
    QVERIFY(rect);
    QCOMPARE(rect->width(), 200.0);
    QCOMPARE(rect->height(), 80.0);

    // Check resize
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(rect->width(), 180.0);
    QCOMPARE(rect->height(), 30.0);

    // Switch mode
    loader->setResizeMode(QDeclarativeLoader::SizeLoaderToItem);
    rect->setWidth(160);
    rect->setHeight(45);
    QCOMPARE(loader->width(), 160.0);
    QCOMPARE(loader->height(), 45.0);

    delete loader;
}

void tst_QDeclarativeLoader::noResize()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/NoResize.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(loader->height(), 80.0);

    QDeclarativeItem *rect = qobject_cast<QDeclarativeItem*>(loader->item());
    QVERIFY(rect);
    QCOMPARE(rect->width(), 120.0);
    QCOMPARE(rect->height(), 60.0);

    delete loader;
}

void tst_QDeclarativeLoader::sizeLoaderToGraphicsWidget()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SizeLoaderToGraphicsWidget.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QGraphicsScene scene;
    scene.addItem(loader);

    QVERIFY(loader != 0);
    QVERIFY(loader->resizeMode() == QDeclarativeLoader::SizeLoaderToItem);
    QCOMPARE(loader->width(), 250.0);
    QCOMPARE(loader->height(), 250.0);

    // Check resize
    QGraphicsWidget *widget = qobject_cast<QGraphicsWidget*>(loader->item());
    QVERIFY(widget);
    widget->resize(QSizeF(150,45));
    QCOMPARE(loader->width(), 150.0);
    QCOMPARE(loader->height(), 45.0);

    // Switch mode
    loader->setResizeMode(QDeclarativeLoader::SizeItemToLoader);
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(widget->size().width(), 180.0);
    QCOMPARE(widget->size().height(), 30.0);

    delete loader;
}

void tst_QDeclarativeLoader::sizeGraphicsWidgetToLoader()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/SizeGraphicsWidgetToLoader.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QGraphicsScene scene;
    scene.addItem(loader);

    QVERIFY(loader != 0);
    QVERIFY(loader->resizeMode() == QDeclarativeLoader::SizeItemToLoader);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(loader->height(), 80.0);

    QGraphicsWidget *widget = qobject_cast<QGraphicsWidget*>(loader->item());
    QVERIFY(widget);
    QCOMPARE(widget->size().width(), 200.0);
    QCOMPARE(widget->size().height(), 80.0);

    // Check resize
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(widget->size().width(), 180.0);
    QCOMPARE(widget->size().height(), 30.0);

    // Switch mode
    loader->setResizeMode(QDeclarativeLoader::SizeLoaderToItem);
    widget->resize(QSizeF(160,45));
    QCOMPARE(loader->width(), 160.0);
    QCOMPARE(loader->height(), 45.0);

    delete loader;
}

void tst_QDeclarativeLoader::noResizeGraphicsWidget()
{
    QDeclarativeComponent component(&engine, TEST_FILE("/NoResizeGraphicsWidget.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QGraphicsScene scene;
    scene.addItem(loader);

    QVERIFY(loader != 0);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(loader->height(), 80.0);

    QGraphicsWidget *widget = qobject_cast<QGraphicsWidget*>(loader->item());
    QVERIFY(widget);
    QCOMPARE(widget->size().width(), 250.0);
    QCOMPARE(widget->size().height(), 250.0);

    delete loader;
}

void tst_QDeclarativeLoader::networkRequestUrl()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/data");

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import Qt 4.7\nLoader { source: \"http://127.0.0.1:14450/Rect120x60.qml\" }"), QUrl::fromLocalFile(SRCDIR "/dummy.qml"));
    if (component.isError())
        qDebug() << component.errors();
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);

    TRY_WAIT(loader->status() == QDeclarativeLoader::Ready);

    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

    delete loader;
}

/* XXX Component waits until all dependencies are loaded.  Is this actually possible?
void tst_QDeclarativeLoader::networkComponent()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory("slowdata", TestHTTPServer::Delay);

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray(
                "import Qt 4.7\n"
                "import \"http://127.0.0.1:14450/\" as NW\n"
                "Item {\n"
                " Component { id: comp; NW.SlowRect {} }\n"
                " Loader { sourceComponent: comp } }")
            , TEST_FILE(""));

    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item);

    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(1)); 
    QVERIFY(loader);
    TRY_WAIT(loader->status() == QDeclarativeLoader::Ready);

    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QDeclarativeLoader::Ready);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);

    delete loader;
}
*/

void tst_QDeclarativeLoader::failNetworkRequest()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(SRCDIR "/data");

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Network error for URL http://127.0.0.1:14450/IDontExist.qml");

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import Qt 4.7\nLoader { source: \"http://127.0.0.1:14450/IDontExist.qml\" }"), QUrl::fromLocalFile("http://127.0.0.1:14450/dummy.qml"));
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader != 0);

    TRY_WAIT(loader->status() == QDeclarativeLoader::Error);

    QVERIFY(loader->item() == 0);
    QCOMPARE(loader->progress(), 0.0);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 0);

    delete loader;
}

// QTBUG-9241
void tst_QDeclarativeLoader::deleteComponentCrash()
{
    QDeclarativeComponent component(&engine, TEST_FILE("crash.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item);

    item->metaObject()->invokeMethod(item, "setLoaderSource");

    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(item->QGraphicsObject::children().at(0));
    QVERIFY(loader);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QLatin1String("blue"));
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QDeclarativeLoader::Ready);
    QCOMPARE(static_cast<QGraphicsItem*>(loader)->children().count(), 1);
    QVERIFY(loader->source() == QUrl::fromLocalFile(SRCDIR "/data/BlueRect.qml"));

    delete item;
}

void tst_QDeclarativeLoader::nonItem()
{
    QDeclarativeComponent component(&engine, TEST_FILE("nonItem.qml"));
    QString err = QUrl::fromLocalFile(SRCDIR).toString() + "/data/nonItem.qml:3:1: QML Loader: Loader does not support loading non-visual elements.";

    QTest::ignoreMessage(QtWarningMsg, err.toLatin1().constData());
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader);
    QVERIFY(loader->item() == 0);

    delete loader;
}

void tst_QDeclarativeLoader::vmeErrors()
{
    QDeclarativeComponent component(&engine, TEST_FILE("vmeErrors.qml"));
    QString err = QUrl::fromLocalFile(SRCDIR).toString() + "/data/VmeError.qml:6: Cannot assign object type QObject with no default method";
    QTest::ignoreMessage(QtWarningMsg, err.toLatin1().constData());
    QDeclarativeLoader *loader = qobject_cast<QDeclarativeLoader*>(component.create());
    QVERIFY(loader);
    QVERIFY(loader->item() == 0);

    delete loader;
}

QTEST_MAIN(tst_QDeclarativeLoader)

#include "tst_qdeclarativeloader.moc"
