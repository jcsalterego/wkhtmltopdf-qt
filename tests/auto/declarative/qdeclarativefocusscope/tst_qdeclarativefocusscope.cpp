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
#include <QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeview.h>
#include <private/qdeclarativerectangle_p.h>
#include <private/qdeclarativetextedit_p.h>
#include <private/qdeclarativetext_p.h>
#include <QtDeclarative/private/qdeclarativefocusscope_p.h>


class tst_qdeclarativefocusscope : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativefocusscope() {}

    template<typename T>
    T *findItem(QGraphicsObject *parent, const QString &id);

private slots:
    void basic();
    void nested();
    void noFocus();
    void textEdit();
    void forceFocus();
};

/*
   Find an item with the specified id.
*/
template<typename T>
T *tst_qdeclarativefocusscope::findItem(QGraphicsObject *parent, const QString &objectName)
{
    const QMetaObject &mo = T::staticMetaObject;
    QList<QGraphicsItem *> children = parent->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(children.at(i)->toGraphicsObject());
        if (item) {
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
                return static_cast<T*>(item);
            }
            item = findItem<T>(item, objectName);
            if (item)
                return static_cast<T*>(item);
        }
    }
    return 0;
}

void tst_qdeclarativefocusscope::basic()
{
    QDeclarativeView *view = new QDeclarativeView;
    view->setSource(QUrl::fromLocalFile(SRCDIR "/data/test.qml"));

    QDeclarativeRectangle *item0 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item0"));
    QDeclarativeRectangle *item1 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item1"));
    QDeclarativeRectangle *item2 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item2"));
    QDeclarativeRectangle *item3 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != 0);
    QVERIFY(item1 != 0);
    QVERIFY(item2 != 0);
    QVERIFY(item3 != 0);

    view->show();
    qApp->setActiveWindow(view);
    qApp->processEvents();

#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(view);
#endif

    QVERIFY(view->hasFocus());
    QVERIFY(view->scene()->hasFocus());
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == true);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Down);
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == true);

    delete view;
}

void tst_qdeclarativefocusscope::nested()
{
    QDeclarativeView *view = new QDeclarativeView;
    view->setSource(QUrl::fromLocalFile(SRCDIR "/data/test2.qml"));

    QDeclarativeFocusScope *item1 = findItem<QDeclarativeFocusScope>(view->rootObject(), QLatin1String("item1"));
    QDeclarativeFocusScope *item2 = findItem<QDeclarativeFocusScope>(view->rootObject(), QLatin1String("item2"));
    QDeclarativeFocusScope *item3 = findItem<QDeclarativeFocusScope>(view->rootObject(), QLatin1String("item3"));
    QDeclarativeFocusScope *item4 = findItem<QDeclarativeFocusScope>(view->rootObject(), QLatin1String("item4"));
    QDeclarativeFocusScope *item5 = findItem<QDeclarativeFocusScope>(view->rootObject(), QLatin1String("item5"));
    QVERIFY(item1 != 0);
    QVERIFY(item2 != 0);
    QVERIFY(item3 != 0);
    QVERIFY(item4 != 0);
    QVERIFY(item5 != 0);

    view->show();
    qApp->setActiveWindow(view);
    qApp->processEvents();

#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(view);
#endif

    QVERIFY(view->hasFocus());
    QVERIFY(view->scene()->hasFocus());

    QVERIFY(item1->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->wantsFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->wantsFocus() == true);
    QVERIFY(item3->hasFocus() == false);
    QVERIFY(item4->wantsFocus() == true);
    QVERIFY(item4->hasFocus() == false);
    QVERIFY(item5->wantsFocus() == true);
    QVERIFY(item5->hasFocus() == true);
    delete view;
}

void tst_qdeclarativefocusscope::noFocus()
{
    QDeclarativeView *view = new QDeclarativeView;
    view->setSource(QUrl::fromLocalFile(SRCDIR "/data/test4.qml"));

    QDeclarativeRectangle *item0 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item0"));
    QDeclarativeRectangle *item1 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item1"));
    QDeclarativeRectangle *item2 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item2"));
    QDeclarativeRectangle *item3 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != 0);
    QVERIFY(item1 != 0);
    QVERIFY(item2 != 0);
    QVERIFY(item3 != 0);

    view->show();
    qApp->setActiveWindow(view);
    qApp->processEvents();

#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(view);
#endif

    QVERIFY(view->hasFocus());
    QVERIFY(view->scene()->hasFocus());
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Down);
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    delete view;
}

void tst_qdeclarativefocusscope::textEdit()
{
    QDeclarativeView *view = new QDeclarativeView;
    view->setSource(QUrl::fromLocalFile(SRCDIR "/data/test5.qml"));

    QDeclarativeRectangle *item0 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item0"));
    QDeclarativeTextEdit *item1 = findItem<QDeclarativeTextEdit>(view->rootObject(), QLatin1String("item1"));
    QDeclarativeRectangle *item2 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item2"));
    QDeclarativeTextEdit *item3 = findItem<QDeclarativeTextEdit>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != 0);
    QVERIFY(item1 != 0);
    QVERIFY(item2 != 0);
    QVERIFY(item3 != 0);

    view->show();
    qApp->setActiveWindow(view);
    qApp->processEvents();

#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(view);
#endif

    QVERIFY(view->hasFocus());
    QVERIFY(view->scene()->hasFocus());
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == true);
    QVERIFY(item3->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_Down);
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->hasFocus() == true);

    delete view;
}

void tst_qdeclarativefocusscope::forceFocus()
{
    QDeclarativeView *view = new QDeclarativeView;
    view->setSource(QUrl::fromLocalFile(SRCDIR "/data/forcefocus.qml"));

    QDeclarativeRectangle *item0 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item0"));
    QDeclarativeRectangle *item1 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item1"));
    QDeclarativeRectangle *item2 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item2"));
    QDeclarativeRectangle *item3 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item3"));
    QDeclarativeRectangle *item4 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item4"));
    QDeclarativeRectangle *item5 = findItem<QDeclarativeRectangle>(view->rootObject(), QLatin1String("item5"));
    QVERIFY(item0 != 0);
    QVERIFY(item1 != 0);
    QVERIFY(item2 != 0);
    QVERIFY(item3 != 0);
    QVERIFY(item4 != 0);
    QVERIFY(item5 != 0);

    view->show();
    qApp->setActiveWindow(view);
    qApp->processEvents();

#ifdef Q_WS_X11
    // to be safe and avoid failing setFocus with window managers
    qt_x11_wait_for_window_manager(view);
#endif

    QVERIFY(view->hasFocus());
    QVERIFY(view->scene()->hasFocus());
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->wantsFocus() == false);
    QVERIFY(item4->hasFocus() == false);
    QVERIFY(item5->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_4);
    QVERIFY(item0->wantsFocus() == true);
    QVERIFY(item1->hasFocus() == true);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->wantsFocus() == false);
    QVERIFY(item4->hasFocus() == false);
    QVERIFY(item5->hasFocus() == false);

    QTest::keyClick(view, Qt::Key_5);
    QVERIFY(item0->wantsFocus() == false);
    QVERIFY(item1->hasFocus() == false);
    QVERIFY(item2->hasFocus() == false);
    QVERIFY(item3->wantsFocus() == true);
    QVERIFY(item4->hasFocus() == false);
    QVERIFY(item5->hasFocus() == true);

    delete view;
}


QTEST_MAIN(tst_qdeclarativefocusscope)

#include "tst_qdeclarativefocusscope.moc"
