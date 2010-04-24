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
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>

#include <private/qdeclarativedebugclient_p.h>
#include <private/qdeclarativedebugservice_p.h>

#include "debugutil_p.h"


bool QDeclarativeDebugTest::waitForSignal(QObject *receiver, const char *member, int timeout) {
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QObject::connect(receiver, member, &loop, SLOT(quit()));
    timer.start(timeout);
    loop.exec();
    return timer.isActive();
}

QDeclarativeDebugTestService::QDeclarativeDebugTestService(const QString &s, QObject *parent)
    : QDeclarativeDebugService(s, parent), enabled(false)
{
}

void QDeclarativeDebugTestService::messageReceived(const QByteArray &ba)
{
    sendMessage(ba);
}

void QDeclarativeDebugTestService::enabledChanged(bool e)
{
    enabled = e;
    emit enabledStateChanged();
}


QDeclarativeDebugTestClient::QDeclarativeDebugTestClient(const QString &s, QDeclarativeDebugConnection *c)
    : QDeclarativeDebugClient(s, c)
{
}

QByteArray QDeclarativeDebugTestClient::waitForResponse()
{
    lastMsg.clear();
    QDeclarativeDebugTest::waitForSignal(this, SIGNAL(serverMessage(QByteArray)));
    if (lastMsg.isEmpty()) {
        qWarning() << "tst_QDeclarativeDebugClient: no response from server!";
        return QByteArray();
    }
    return lastMsg;
}

void QDeclarativeDebugTestClient::messageReceived(const QByteArray &ba)
{
    lastMsg = ba;
    emit serverMessage(ba);
}

