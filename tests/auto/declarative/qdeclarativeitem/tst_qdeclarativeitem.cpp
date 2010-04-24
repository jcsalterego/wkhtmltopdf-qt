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
#include <QtTest/QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeview.h>
#include <QtDeclarative/qdeclarativeitem.h>

class tst_QDeclarativeItem : public QObject

{
    Q_OBJECT
public:
    tst_QDeclarativeItem();

private slots:
    void keys();
    void keyNavigation();
    void smooth();
    void clip();
    void mapCoordinates();
    void mapCoordinates_data();
    void propertyChanges();
    void transforms();
    void transforms_data();

    void childrenProperty();
    void resourcesProperty();

private:
    template<typename T>
    T *findItem(QGraphicsObject *parent, const QString &objectName);
    QDeclarativeEngine engine;
};

class KeysTestObject : public QObject
{
    Q_OBJECT
public:
    KeysTestObject() : mKey(0), mModifiers(0), mForwardedKey(0) {}

    void reset() {
        mKey = 0;
        mText = QString();
        mModifiers = 0;
        mForwardedKey = 0;
    }

public slots:
    void keyPress(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void keyRelease(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void forwardedKey(int key) {
        mForwardedKey = key;
    }

public:
    int mKey;
    QString mText;
    int mModifiers;
    int mForwardedKey;

private:
};


tst_QDeclarativeItem::tst_QDeclarativeItem()
{
}

void tst_QDeclarativeItem::keys()
{
    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    KeysTestObject *testObject = new KeysTestObject;
    canvas->rootContext()->setContextProperty("keysTestObject", testObject);

    canvas->rootContext()->setContextProperty("enableKeyHanding", QVariant(true));

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/keystest.qml"));
    canvas->show();
    qApp->processEvents();

    QEvent wa(QEvent::WindowActivate);
    QApplication::sendEvent(canvas, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QApplication::sendEvent(canvas, &fe);

    QKeyEvent key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyRelease, Qt::Key_A, Qt::ShiftModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::ShiftModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Return));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Return));
    QCOMPARE(testObject->mText, QLatin1String("Return"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_0, Qt::NoModifier, "0", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_0));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_0));
    QCOMPARE(testObject->mText, QLatin1String("0"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_9, Qt::NoModifier, "9", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_9));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_9));
    QCOMPARE(testObject->mText, QLatin1String("9"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Tab));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Tab));
    QCOMPARE(testObject->mText, QLatin1String("Tab"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Backtab));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Backtab));
    QCOMPARE(testObject->mText, QLatin1String("Backtab"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    canvas->rootContext()->setContextProperty("enableKeyHanding", QVariant(false));

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, 0);
    QVERIFY(!key.isAccepted());

    delete canvas;
    delete testObject;
}

void tst_QDeclarativeItem::keyNavigation()
{
    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);

    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/keynavigationtest.qml"));
    canvas->show();
    qApp->processEvents();

    QEvent wa(QEvent::WindowActivate);
    QApplication::sendEvent(canvas, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QApplication::sendEvent(canvas, &fe);

    QDeclarativeItem *item = findItem<QDeclarativeItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // right
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // down
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // left
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // up
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // tab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    // backtab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QDeclarativeItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasFocus());

    delete canvas;
}

void tst_QDeclarativeItem::smooth()
{
    QDeclarativeComponent component(&engine);
    component.setData("import Qt 4.7; Item { smooth: false; }", QUrl::fromLocalFile(""));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(smoothChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->smooth());

    item->setSmooth(true);
    QVERIFY(item->smooth());
    QCOMPARE(spy.count(),1);
    QList<QVariant> arguments = spy.first();
    QVERIFY(arguments.count() == 1);
    QVERIFY(arguments.at(0).toBool() == true);

    item->setSmooth(true);
    QCOMPARE(spy.count(),1);

    item->setSmooth(false);
    QVERIFY(!item->smooth());
    QCOMPARE(spy.count(),2);
    item->setSmooth(false);
    QCOMPARE(spy.count(),2);

    delete item;
}

void tst_QDeclarativeItem::clip()
{
    QDeclarativeComponent component(&engine);
    component.setData("import Qt 4.7\nItem { clip: false\n }", QUrl::fromLocalFile(""));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(clipChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->clip());

    item->setClip(true);
    QVERIFY(item->clip());

    QList<QVariant> arguments = spy.first();
    QVERIFY(arguments.count() == 1);
    QVERIFY(arguments.at(0).toBool() == true);

    QCOMPARE(spy.count(),1);
    item->setClip(true);
    QCOMPARE(spy.count(),1);

    item->setClip(false);
    QVERIFY(!item->clip());
    QCOMPARE(spy.count(),2);
    item->setClip(false);
    QCOMPARE(spy.count(),2);

    delete item;
}

void tst_QDeclarativeItem::mapCoordinates()
{
    QFETCH(int, x);
    QFETCH(int, y);

    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(300, 300);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/mapCoordinates.qml"));
    canvas->show();
    qApp->processEvents();

    QDeclarativeItem *root = qobject_cast<QDeclarativeItem*>(canvas->rootObject());
    QVERIFY(root != 0);
    QDeclarativeItem *a = findItem<QDeclarativeItem>(canvas->rootObject(), "itemA");
    QVERIFY(a != 0);
    QDeclarativeItem *b = findItem<QDeclarativeItem>(canvas->rootObject(), "itemB");
    QVERIFY(b != 0);

    QVariant result;

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QGraphicsItem*>(a)->mapToItem(b, x, y));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QGraphicsItem*>(a)->mapFromItem(b, x, y));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QGraphicsItem*>(a)->mapToScene(x, y));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QGraphicsItem*>(a)->mapFromScene(x, y));

    QString warning1 = QUrl::fromLocalFile(SRCDIR "/data/mapCoordinates.qml").toString() + ":7:5: QML Item: mapToItem() given argument \"1122\" which is neither null nor an Item";
    QString warning2 = QUrl::fromLocalFile(SRCDIR "/data/mapCoordinates.qml").toString() + ":7:5: QML Item: mapFromItem() given argument \"1122\" which is neither null nor an Item";

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAToInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAFromInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    delete canvas;
}

void tst_QDeclarativeItem::mapCoordinates_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");

    for (int i=-20; i<=20; i+=10)
        QTest::newRow(QTest::toString(i)) << i << i;
}

void tst_QDeclarativeItem::transforms_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<QMatrix>("matrix");
    QTest::newRow("translate") << QByteArray("Translate { x: 10; y: 20 }")
        << QMatrix(1,0,0,1,10,20);
    QTest::newRow("rotation") << QByteArray("Rotation { angle: 90 }")
        << QMatrix(0,1,-1,0,0,0);
    QTest::newRow("scale") << QByteArray("Scale { xScale: 1.5; yScale: -2  }")
        << QMatrix(1.5,0,0,-2,0,0);
    QTest::newRow("sequence") << QByteArray("[ Translate { x: 10; y: 20 }, Scale { xScale: 1.5; yScale: -2  } ]")
        << QMatrix(1,0,0,1,10,20) * QMatrix(1.5,0,0,-2,0,0);
}

void tst_QDeclarativeItem::transforms()
{
    QFETCH(QByteArray, qml);
    QFETCH(QMatrix, matrix);
    QDeclarativeComponent component(&engine);
    component.setData("import Qt 4.7\nItem { transform: "+qml+"}", QUrl::fromLocalFile(""));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item);
    QCOMPARE(item->sceneMatrix(), matrix);
}

void tst_QDeclarativeItem::childrenProperty()
{
    QDeclarativeComponent component(&engine, SRCDIR "/data/childrenProperty.qml");
    
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    delete o;
}

void tst_QDeclarativeItem::resourcesProperty()
{
    QDeclarativeComponent component(&engine, SRCDIR "/data/resourcesProperty.qml");
    
    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    delete o;
}

void tst_QDeclarativeItem::propertyChanges()
{
    QDeclarativeView *canvas = new QDeclarativeView(0);
    canvas->setFixedSize(240,320);
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/propertychanges.qml"));
    canvas->show();

    QEvent wa(QEvent::WindowActivate);
    QApplication::sendEvent(canvas, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QApplication::sendEvent(canvas, &fe);

    QDeclarativeItem *item = findItem<QDeclarativeItem>(canvas->rootObject(), "item");
    QDeclarativeItem *parentItem = findItem<QDeclarativeItem>(canvas->rootObject(), "parentItem");

    QVERIFY(item);
    QVERIFY(parentItem);

    QSignalSpy parentSpy(item, SIGNAL(parentChanged(QDeclarativeItem *)));
    QSignalSpy widthSpy(item, SIGNAL(widthChanged()));
    QSignalSpy heightSpy(item, SIGNAL(heightChanged()));
    QSignalSpy baselineOffsetSpy(item, SIGNAL(baselineOffsetChanged(qreal)));
    QSignalSpy childrenRectSpy(parentItem, SIGNAL(childrenRectChanged(QRectF)));
    QSignalSpy focusSpy(item, SIGNAL(focusChanged(bool)));
    QSignalSpy wantsFocusSpy(parentItem, SIGNAL(wantsFocusChanged(bool)));

    item->setParentItem(parentItem);
    item->setWidth(100.0);
    item->setHeight(200.0);
    item->setFocus(true);
    item->setBaselineOffset(10.0);

    QCOMPARE(item->parentItem(), parentItem);
    QCOMPARE(parentSpy.count(),1);
    QList<QVariant> parentArguments = parentSpy.first();
    QVERIFY(parentArguments.count() == 1);
    QCOMPARE(item->parentItem(), qvariant_cast<QDeclarativeItem *>(parentArguments.at(0)));

    QCOMPARE(item->width(), 100.0);
    QCOMPARE(widthSpy.count(),1);

    QCOMPARE(item->height(), 200.0);
    QCOMPARE(heightSpy.count(),1);

    QCOMPARE(item->baselineOffset(), 10.0);
    QCOMPARE(baselineOffsetSpy.count(),1);
    QList<QVariant> baselineOffsetArguments = baselineOffsetSpy.first();
    QVERIFY(baselineOffsetArguments.count() == 1);
    QCOMPARE(item->baselineOffset(), baselineOffsetArguments.at(0).toReal());

    QCOMPARE(parentItem->childrenRect(), QRectF(0.0,0.0,100.0,200.0));
    QCOMPARE(childrenRectSpy.count(),2);
    QList<QVariant> childrenRectArguments = childrenRectSpy.at(1);
    QVERIFY(childrenRectArguments.count() == 1);
    QCOMPARE(parentItem->childrenRect(), childrenRectArguments.at(0).toRectF());

    QCOMPARE(item->hasFocus(), true);
    QCOMPARE(focusSpy.count(),1);
    QList<QVariant> focusArguments = focusSpy.first();
    QVERIFY(focusArguments.count() == 1);
    QCOMPARE(focusArguments.at(0).toBool(), true);

    QCOMPARE(parentItem->hasFocus(), false);
    QCOMPARE(parentItem->wantsFocus(), true);
    QCOMPARE(wantsFocusSpy.count(),1);
    QList<QVariant> wantsFocusArguments = wantsFocusSpy.first();
    QVERIFY(wantsFocusArguments.count() == 1);
    QCOMPARE(wantsFocusArguments.at(0).toBool(), true);

    delete canvas;
}

template<typename T>
T *tst_QDeclarativeItem::findItem(QGraphicsObject *parent, const QString &objectName)
{
    if (!parent)
        return 0;

    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->QGraphicsObject::children().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(parent->childItems().at(i));
        if(!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
            return static_cast<T*>(item);
        item = findItem<T>(item, objectName);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}



QTEST_MAIN(tst_QDeclarativeItem)

#include "tst_qdeclarativeitem.moc"
