/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSTATE_H
#define QSTATE_H

#include <QtCore/qabstractstate.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_STATEMACHINE

class QAbstractTransition;
class QSignalTransition;

class QStatePrivate;
class Q_CORE_EXPORT QState : public QAbstractState
{
    Q_OBJECT
    Q_PROPERTY(QAbstractState* initialState READ initialState WRITE setInitialState)
    Q_PROPERTY(QAbstractState* errorState READ errorState WRITE setErrorState)
    Q_PROPERTY(ChildMode childMode READ childMode WRITE setChildMode)
    Q_ENUMS(ChildMode)
public:
    enum ChildMode {
        ExclusiveStates,
        ParallelStates
    };

    QState(QState *parent = 0);
    QState(ChildMode childMode, QState *parent = 0);
    ~QState();

    QAbstractState *errorState() const;
    void setErrorState(QAbstractState *state);

    QAbstractTransition *addTransition(QAbstractTransition *transition);
    QSignalTransition *addTransition(QObject *sender, const char *signal, QAbstractState *target);
    QAbstractTransition *addTransition(QAbstractState *target);
    void removeTransition(QAbstractTransition *transition);

    QAbstractState *initialState() const;
    void setInitialState(QAbstractState *state);

    ChildMode childMode() const;
    void setChildMode(ChildMode mode);

#ifndef QT_NO_PROPERTIES
    void assignProperty(QObject *object, const char *name,
                        const QVariant &value);
#endif

Q_SIGNALS:
    void finished();
    void polished();

protected:
    void onEntry(QEvent *event);
    void onExit(QEvent *event);

    bool event(QEvent *e);

protected:
    QState(QStatePrivate &dd, QState *parent);

private:
    Q_DISABLE_COPY(QState)
    Q_DECLARE_PRIVATE(QState)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif