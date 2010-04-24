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
#include <private/qdeclarativeconnections_p.h>
#include <private/qdeclarativeitem_p.h>
#include "../../../shared/util.h"
#include <QtDeclarative/qdeclarativescriptstring.h>

class tst_qdeclarativeconnection : public QObject

{
    Q_OBJECT
public:
    tst_qdeclarativeconnection();

private slots:
    void defaultValues();
    void properties();
    void connection();
    void trimming();
    void targetChanged();

private:
    QDeclarativeEngine engine;
};

tst_qdeclarativeconnection::tst_qdeclarativeconnection()
{
}

void tst_qdeclarativeconnection::defaultValues()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/test-connection3.qml"));
    QDeclarativeConnections *item = qobject_cast<QDeclarativeConnections*>(c.create());

    QVERIFY(item != 0);
    QVERIFY(item->target() == 0);

    delete item;
}

void tst_qdeclarativeconnection::properties()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/test-connection2.qml"));
    QDeclarativeConnections *item = qobject_cast<QDeclarativeConnections*>(c.create());

    QVERIFY(item != 0);

    QVERIFY(item != 0);
    QVERIFY(item->target() == item);

    delete item;
}

void tst_qdeclarativeconnection::connection()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/test-connection.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(c.create());

    QVERIFY(item != 0);

    QCOMPARE(item->property("tested").toBool(), false);
    QCOMPARE(item->width(), 50.);
    emit item->setWidth(100.);
    QCOMPARE(item->width(), 100.);
    QCOMPARE(item->property("tested").toBool(), true);

    delete item;
}

void tst_qdeclarativeconnection::trimming()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/trimming.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(c.create());

    QVERIFY(item != 0);

    QCOMPARE(item->property("tested").toString(), QString(""));
    int index = item->metaObject()->indexOfSignal("testMe(int,QString)");
    QMetaMethod method = item->metaObject()->method(index);
    method.invoke(item,
                  Qt::DirectConnection,
                  Q_ARG(int, 5),
                  Q_ARG(QString, "worked"));
    QCOMPARE(item->property("tested").toString(), QString("worked5"));

    delete item;
}

// Confirm that target can be changed by one of our signal handlers
void tst_qdeclarativeconnection::targetChanged()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/connection-targetchange.qml"));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(c.create());
    QVERIFY(item != 0);

    QDeclarativeConnections *connections = item->findChild<QDeclarativeConnections*>("connections");
    QVERIFY(connections);

    QDeclarativeItem *item1 = item->findChild<QDeclarativeItem*>("item1");
    QVERIFY(item1);

    item1->setWidth(200);

    QDeclarativeItem *item2 = item->findChild<QDeclarativeItem*>("item2");
    QVERIFY(item2);
    QVERIFY(connections->target() == item2);

    // If we don't crash then we're OK

    delete item;
}

QTEST_MAIN(tst_qdeclarativeconnection)

#include "tst_qdeclarativeconnection.moc"
