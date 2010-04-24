/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QtDeclarative/QDeclarativeExtensionPlugin>
#include <QtDeclarative/qdeclarative.h>
#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QPushButton>
#include <QDebug>

class MyPushButton : public QGraphicsProxyWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    MyPushButton(QGraphicsItem* parent = 0)
        : QGraphicsProxyWidget(parent)
    {
        widget = new QPushButton("MyPushButton");
        widget->setAttribute(Qt::WA_NoSystemBackground);
        setWidget(widget);

        QObject::connect(widget, SIGNAL(clicked(bool)), this, SIGNAL(clicked(bool)));
    }

    QString text() const
    {
        return widget->text();
    }

    void setText(const QString& text)
    {
        if (text != widget->text()) {
            widget->setText(text);
            emit textChanged();
        }
    }

Q_SIGNALS:
    void clicked(bool);
    void textChanged();

private:
    QPushButton *widget;
};

class ProxyWidgetsPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT
public:
    void registerTypes(const char *uri)
    {
        qmlRegisterType<MyPushButton>(uri, 1, 0, "MyPushButton");
    }
};

#include "proxywidgets.moc"

Q_EXPORT_PLUGIN2(proxywidgetsplugin, ProxyWidgetsPlugin);
