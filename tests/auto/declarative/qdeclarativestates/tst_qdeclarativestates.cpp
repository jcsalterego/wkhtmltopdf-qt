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
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <private/qdeclarativeanchors_p_p.h>
#include <private/qdeclarativerectangle_p.h>
#include <private/qdeclarativeimage_p.h>
#include <private/qdeclarativetext_p.h>
#include <private/qdeclarativepropertychanges_p.h>
#include <private/qdeclarativestategroup_p.h>


class MyRect : public QDeclarativeRectangle
{
   Q_OBJECT
   Q_PROPERTY(int propertyWithNotify READ propertyWithNotify WRITE setPropertyWithNotify NOTIFY oddlyNamedNotifySignal)
public:
    MyRect() {}

    void doSomething() { emit didSomething(); }
    
    int propertyWithNotify() const { return m_prop; }
    void setPropertyWithNotify(int i) { m_prop = i; emit oddlyNamedNotifySignal(); }
Q_SIGNALS:
    void didSomething();
    void oddlyNamedNotifySignal();

private:
    int m_prop;
};


class tst_qdeclarativestates : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativestates() {}

private:
    static QByteArray fullDataPath(const QString &path);

private slots:
    void initTestCase();

    void basicChanges();
    void basicExtension();
    void basicBinding();
    void signalOverride();
    void signalOverrideCrash();
    void parentChange();
    void parentChangeErrors();
    void anchorChanges();
    void anchorChanges2();
    void anchorChanges3();
    void anchorChanges4();
    void anchorChanges5();
    void anchorChangesCrash();
    void script();
    void restoreEntryValues();
    void explicitChanges();
    void propertyErrors();
    void incorrectRestoreBug();
    void autoStateAtStartupRestoreBug();
    void deletingChange();
    void deletingState();
    void tempState();
    void illegalTempState();
    void nonExistantProperty();
    void reset();
    void illegalObjectCreation();
    void whenOrdering();
    void urlResolution();
};

void tst_qdeclarativestates::initTestCase()
{
    qmlRegisterType<MyRect>("Qt.test", 1, 0, "MyRectangle");
}

QByteArray tst_qdeclarativestates::fullDataPath(const QString &path)
{
    return QUrl::fromLocalFile(SRCDIR + path).toString().toUtf8();    
}

void tst_qdeclarativestates::basicChanges()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicChanges.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicChanges2.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicChanges3.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2);

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1);
        //### we should be checking that this is an implicit rather than explicit 1 (which currently fails)

        rect->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2);

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1);

    }

    {
        // Test basicChanges4.qml can magically connect to propertyWithNotify's notify
        // signal using 'onPropertyWithNotifyChanged' even though the signal name is
        // actually 'oddlyNamedNotifySignal'

        QDeclarativeComponent component(&engine, SRCDIR "/data/basicChanges4.qml");
        QVERIFY(component.isReady());

        MyRect *rect = qobject_cast<MyRect*>(component.create());
        QVERIFY(rect != 0);

        QMetaProperty prop = rect->metaObject()->property(rect->metaObject()->indexOfProperty("propertyWithNotify"));
        QVERIFY(prop.hasNotifySignal());
        QString notifySignal = QByteArray(prop.notifySignal().signature());
        QVERIFY(!notifySignal.startsWith("propertyWithNotifyChanged("));

        QCOMPARE(rect->color(), QColor(Qt::red));

        rect->setPropertyWithNotify(100);
        QCOMPARE(rect->color(), QColor(Qt::blue));
    }
}

void tst_qdeclarativestates::basicExtension()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicExtension.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2);

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1);

        rect->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2);

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1);
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/fakeExtension.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }
}

void tst_qdeclarativestates::basicBinding()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicBinding.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicBinding2.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicBinding3.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("red"));
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor2", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor2", QColor("green"));
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicBinding4.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("purple"));
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("purple"));

        rect->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }
}

void tst_qdeclarativestates::signalOverride()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/signalOverride.qml");
        MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/signalOverride2.qml");
        MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("white"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("extendedRect"));

        innerRect->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(innerRect->color(),QColor("green"));
        QCOMPARE(innerRect->property("extendedColor").value<QColor>(),QColor("green"));
    }
}

void tst_qdeclarativestates::signalOverrideCrash()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/signalOverrideCrash.qml");
    MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
    QVERIFY(rect != 0);

    rect->setState("overridden");
    rect->doSomething();
}

void tst_qdeclarativestates::parentChange()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/parentChange1.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QDeclarativeListReference list(rect, "states");
        QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
        QVERIFY(state != 0);

        qmlExecuteDeferred(state);
        QDeclarativeParentChange *pChange = qobject_cast<QDeclarativeParentChange*>(state->operationAt(0));
        QVERIFY(pChange != 0);
        QDeclarativeItem *nParent = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("NewParent"));
        QVERIFY(nParent != 0);

        QCOMPARE(pChange->parent(), nParent);

        rect->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(-133));
        QCOMPARE(innerRect->y(), qreal(-300));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/parentChange2.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        rect->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(15));
        QCOMPARE(innerRect->scale(), qreal(.5));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-19.9075));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-8.73433));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/parentChange3.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        rect->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(-37));
        QCOMPARE(innerRect->scale(), qreal(.25));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-217.305));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-164.413));

        rect->setState("");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        //do a non-qFuzzyCompare fuzzy compare
        QVERIFY(innerRect->y() < qreal(0.00001) && innerRect->y() > qreal(-0.00001));
    }
}

void tst_qdeclarativestates::parentChangeErrors()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/parentChange4.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("/data/parentChange4.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under non-uniform scale");
        rect->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/parentChange5.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("/data/parentChange5.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under complex transform");
        rect->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }
}

void tst_qdeclarativestates::anchorChanges()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChanges1.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativeAnchorChanges *aChanges = qobject_cast<QDeclarativeAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rect->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));
    QCOMPARE(aChanges->object(), qobject_cast<QDeclarativeItem*>(innerRect));
    QCOMPARE(aChanges->object()->anchors()->left().anchorLine, QDeclarativeAnchorLine::Invalid);  //### was reset (how do we distinguish from not set at all)
    QCOMPARE(aChanges->object()->anchors()->right().item, rect->right().item);
    QCOMPARE(aChanges->object()->anchors()->right().anchorLine, rect->right().anchorLine);

    rect->setState("");
    QCOMPARE(innerRect->x(), qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges2()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChanges2.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    rect->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));

    rect->setState("");
    QCOMPARE(innerRect->x(), qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges3()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChanges3.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QDeclarativeItem *leftGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QDeclarativeItem *bottomGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativeAnchorChanges *aChanges = qobject_cast<QDeclarativeAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rect->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QDeclarativeItem*>(innerRect));
    QCOMPARE(aChanges->object()->anchors()->left().item, leftGuideline->left().item);
    QCOMPARE(aChanges->object()->anchors()->left().anchorLine, leftGuideline->left().anchorLine);
    QCOMPARE(aChanges->object()->anchors()->right().item, rect->right().item);
    QCOMPARE(aChanges->object()->anchors()->right().anchorLine, rect->right().anchorLine);
    QCOMPARE(aChanges->object()->anchors()->top().item, rect->top().item);
    QCOMPARE(aChanges->object()->anchors()->top().anchorLine, rect->top().anchorLine);
    QCOMPARE(aChanges->object()->anchors()->bottom().item, bottomGuideline->bottom().item);
    QCOMPARE(aChanges->object()->anchors()->bottom().anchorLine, bottomGuideline->bottom().anchorLine);

    QCOMPARE(innerRect->x(), qreal(10));
    QCOMPARE(innerRect->y(), qreal(0));
    QCOMPARE(innerRect->width(), qreal(190));
    QCOMPARE(innerRect->height(), qreal(150));

    rect->setState("");
    QCOMPARE(innerRect->x(), qreal(0));
    QCOMPARE(innerRect->y(), qreal(10));
    QCOMPARE(innerRect->width(), qreal(150));
    QCOMPARE(innerRect->height(), qreal(190));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges4()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChanges4.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QDeclarativeItem *leftGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QDeclarativeItem *bottomGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativeAnchorChanges *aChanges = qobject_cast<QDeclarativeAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rect->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QDeclarativeItem*>(innerRect));
    QCOMPARE(aChanges->object()->anchors()->horizontalCenter().item, bottomGuideline->horizontalCenter().item);
    QCOMPARE(aChanges->object()->anchors()->horizontalCenter().anchorLine, bottomGuideline->horizontalCenter().anchorLine);
    QCOMPARE(aChanges->object()->anchors()->verticalCenter().item, leftGuideline->verticalCenter().item);
    QCOMPARE(aChanges->object()->anchors()->verticalCenter().anchorLine, leftGuideline->verticalCenter().anchorLine);

    delete rect;
}

void tst_qdeclarativestates::anchorChanges5()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChanges5.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeRectangle *innerRect = qobject_cast<QDeclarativeRectangle*>(rect->findChild<QDeclarativeRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QDeclarativeItem *leftGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QDeclarativeItem *bottomGuideline = qobject_cast<QDeclarativeItem*>(rect->findChild<QDeclarativeItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativeAnchorChanges *aChanges = qobject_cast<QDeclarativeAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rect->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QDeclarativeItem*>(innerRect));
    //QCOMPARE(aChanges->anchors()->horizontalCenter().item, bottomGuideline->horizontalCenter().item);
    //QCOMPARE(aChanges->anchors()->horizontalCenter().anchorLine, bottomGuideline->horizontalCenter().anchorLine);
    //QCOMPARE(aChanges->anchors()->baseline().item, leftGuideline->baseline().item);
    //QCOMPARE(aChanges->anchors()->baseline().anchorLine, leftGuideline->baseline().anchorLine);

    delete rect;
}

//QTBUG-9609
void tst_qdeclarativestates::anchorChangesCrash()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/anchorChangesCrash.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    rect->setState("reanchored");

    delete rect;
}

void tst_qdeclarativestates::script()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/script.qml");
        QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rect->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rect->setState("");
        QCOMPARE(rect->color(),QColor("blue")); // a script isn't reverted
    }
}

void tst_qdeclarativestates::restoreEntryValues()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/restoreEntryValues.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->color(),QColor("red"));

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setState("");
    QCOMPARE(rect->color(),QColor("blue"));
}

void tst_qdeclarativestates::explicitChanges()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/explicit.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativePropertyChanges *changes = qobject_cast<QDeclarativePropertyChanges*>(rect->findChild<QDeclarativePropertyChanges*>("changes"));
    QVERIFY(changes != 0);
    QVERIFY(changes->isExplicit());

    QCOMPARE(rect->color(),QColor("red"));

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setProperty("sourceColor", QColor("green"));
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    rect->setProperty("sourceColor", QColor("yellow"));
    QCOMPARE(rect->color(),QColor("red"));

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("yellow"));
}

void tst_qdeclarativestates::propertyErrors()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/propertyErrors.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->color(),QColor("red"));

    QTest::ignoreMessage(QtWarningMsg, fullDataPath("/data/propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("/data/propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to read-only property \"wantsFocus\"");
    rect->setState("blue");
}

void tst_qdeclarativestates::incorrectRestoreBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/basicChanges.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->color(),QColor("red"));

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    // make sure if we change the base state value, we then restore to it correctly
    rect->setColor(QColor("green"));

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setState("");
    QCOMPARE(rect->color(),QColor("green"));
}

void tst_qdeclarativestates::autoStateAtStartupRestoreBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent component(&engine, SRCDIR "/data/autoStateAtStartupRestoreBug.qml");
    QObject *obj = component.create();

    QVERIFY(obj != 0);
    QCOMPARE(obj->property("test").toInt(), 3);

    obj->setProperty("input", 2);

    QCOMPARE(obj->property("test").toInt(), 9);

    delete obj;
}

void tst_qdeclarativestates::deletingChange()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/deleting.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));
    QCOMPARE(rect->radius(),qreal(5));

    rect->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(0));

    QDeclarativePropertyChanges *pc = rect->findChild<QDeclarativePropertyChanges*>("pc1");
    QVERIFY(pc != 0);
    delete pc;

    QDeclarativeState *state = rect->findChild<QDeclarativeState*>();
    QVERIFY(state != 0);
    qmlExecuteDeferred(state);
    QCOMPARE(state->operationCount(), 1);

    rect->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(5));

    delete rect;
}

void tst_qdeclarativestates::deletingState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/deletingState.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeStateGroup *sg = rect->findChild<QDeclarativeStateGroup*>();
    QVERIFY(sg != 0);
    QVERIFY(sg->findState("blue") != 0);

    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    sg->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    QDeclarativeState *state = rect->findChild<QDeclarativeState*>();
    QVERIFY(state != 0);
    delete state;

    QVERIFY(sg->findState("blue") == 0);

    //### should we warn that state doesn't exist
    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));

    delete rect;
}

void tst_qdeclarativestates::tempState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/legalTempState.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QTest::ignoreMessage(QtDebugMsg, "entering placed");
    QTest::ignoreMessage(QtDebugMsg, "entering idle");
    rect->setState("placed");
    QCOMPARE(rect->state(), QLatin1String("idle"));
}

void tst_qdeclarativestates::illegalTempState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/illegalTempState.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML StateGroup: Can't apply a state change as part of a state definition.");
    rect->setState("placed");
    QCOMPARE(rect->state(), QLatin1String("placed"));
}

void tst_qdeclarativestates::nonExistantProperty()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, SRCDIR "/data/nonExistantProp.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QTest::ignoreMessage(QtWarningMsg, fullDataPath("/data/nonExistantProp.qml") + ":9:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    rect->setState("blue");
    QCOMPARE(rect->state(), QLatin1String("blue"));
}

void tst_qdeclarativestates::reset()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, SRCDIR "/data/reset.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(c.create());
    QVERIFY(rect != 0);

    QDeclarativeText *text = rect->findChild<QDeclarativeText*>();
    QVERIFY(text != 0);
    QCOMPARE(text->width(), qreal(40.));
    QVERIFY(text->width() < text->height());

    rect->setState("state1");

    QVERIFY(text->width() > 41);
    QVERIFY(text->width() > text->height());
}

void tst_qdeclarativestates::illegalObjectCreation()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent component(&engine, SRCDIR "/data/illegalObj.qml");
    QList<QDeclarativeError> errors = component.errors();
    QVERIFY(errors.count() == 1);
    const QDeclarativeError &error = errors.at(0);
    QCOMPARE(error.line(), 9);
    QCOMPARE(error.column(), 23);
    QCOMPARE(error.description().toUtf8().constData(), "PropertyChanges does not support creating state-specific objects.");
}

void tst_qdeclarativestates::whenOrdering()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, SRCDIR "/data/whenOrdering.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(c.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->state(), QLatin1String(""));
    rect->setProperty("condition2", true);
    QCOMPARE(rect->state(), QLatin1String("state2"));
    rect->setProperty("condition1", true);
    QCOMPARE(rect->state(), QLatin1String("state1"));
    rect->setProperty("condition2", false);
    QCOMPARE(rect->state(), QLatin1String("state1"));
    rect->setProperty("condition2", true);
    QCOMPARE(rect->state(), QLatin1String("state1"));
    rect->setProperty("condition1", false);
    rect->setProperty("condition2", false);
    QCOMPARE(rect->state(), QLatin1String(""));
}

void tst_qdeclarativestates::urlResolution()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, SRCDIR "/data/urlResolution.qml");
    QDeclarativeRectangle *rect = qobject_cast<QDeclarativeRectangle*>(c.create());
    QVERIFY(rect != 0);

    QDeclarativeItem *myType = rect->findChild<QDeclarativeItem*>("MyType");
    QDeclarativeImage *image1 = rect->findChild<QDeclarativeImage*>("image1");
    QDeclarativeImage *image2 = rect->findChild<QDeclarativeImage*>("image2");
    QDeclarativeImage *image3 = rect->findChild<QDeclarativeImage*>("image3");
    QVERIFY(myType != 0 && image1 != 0 && image2 != 0 && image3 != 0);

    myType->setState("SetImageState");
    QUrl resolved = QUrl::fromLocalFile(SRCDIR "/data/Implementation/images/qt-logo.png");
    QCOMPARE(image1->source(), resolved);
    QCOMPARE(image2->source(), resolved);
    QCOMPARE(image3->source(), resolved);
}

QTEST_MAIN(tst_qdeclarativestates)

#include "tst_qdeclarativestates.moc"
