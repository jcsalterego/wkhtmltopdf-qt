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

#include "private/qdeclarativeanimation_p.h"
#include "private/qdeclarativeanimation_p_p.h"

#include "private/qdeclarativebehavior_p.h"
#include "private/qdeclarativestateoperations_p.h"
#include "private/qdeclarativecontext_p.h"

#include <qdeclarativepropertyvaluesource.h>
#include <qdeclarative.h>
#include <qdeclarativeinfo.h>
#include <qdeclarativeexpression.h>
#include <qdeclarativestringconverters_p.h>
#include <qdeclarativeglobal_p.h>
#include <qdeclarativemetatype_p.h>
#include <qdeclarativevaluetype_p.h>
#include <qdeclarativeproperty_p.h>

#include <qvariant.h>
#include <qcolor.h>
#include <qfile.h>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QtCore/qset.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qmath.h>

#include <private/qvariantanimation_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Animation QDeclarativeAbstractAnimation
    \since 4.7
    \brief The Animation element is the base of all QML animations.

    The Animation element cannot be used directly in a QML file.  It exists
    to provide a set of common properties and methods, available across all the
    other animation types that inherit from it.  Attempting to use the Animation
    element directly will result in an error.
*/

/*!
    \class QDeclarativeAbstractAnimation
    \internal
*/

QDeclarativeAbstractAnimation::QDeclarativeAbstractAnimation(QObject *parent)
: QObject(*(new QDeclarativeAbstractAnimationPrivate), parent)
{
}

QDeclarativeAbstractAnimation::~QDeclarativeAbstractAnimation()
{
}

QDeclarativeAbstractAnimation::QDeclarativeAbstractAnimation(QDeclarativeAbstractAnimationPrivate &dd, QObject *parent)
: QObject(dd, parent)
{
}

/*!
    \qmlproperty bool Animation::running
    This property holds whether the animation is currently running.

    The \c running property can be set to declaratively control whether or not
    an animation is running.  The following example will animate a rectangle
    whenever the \l MouseArea is pressed.

    \code
    Rectangle {
        width: 100; height: 100
        NumberAnimation on x {
            running: myMouse.pressed
            from: 0; to: 100
        }
        MouseArea { id: myMouse }
    }
    \endcode

    Likewise, the \c running property can be read to determine if the animation
    is running.  In the following example the text element will indicate whether
    or not the animation is running.

    \code
    NumberAnimation { id: myAnimation }
    Text { text: myAnimation.running ? "Animation is running" : "Animation is not running" }
    \endcode

    Animations can also be started and stopped imperatively from JavaScript
    using the \c start() and \c stop() methods.

    By default, animations are not running. Though, when the animations are assigned to properties,
    as property value sources using the \e on syntax, they are set to running by default.
*/
bool QDeclarativeAbstractAnimation::isRunning() const
{
    Q_D(const QDeclarativeAbstractAnimation);
    return d->running;
}

//commence is called to start an animation when it is used as a
//simple animation, and not as part of a transition
void QDeclarativeAbstractAnimationPrivate::commence()
{
    Q_Q(QDeclarativeAbstractAnimation);

    QDeclarativeStateActions actions;
    QDeclarativeProperties properties;
    q->transition(actions, properties, QDeclarativeAbstractAnimation::Forward);

    q->qtAnimation()->start();
    if (q->qtAnimation()->state() != QAbstractAnimation::Running) {
        running = false;
        emit q->completed();
    }
}

QDeclarativeProperty QDeclarativeAbstractAnimationPrivate::createProperty(QObject *obj, const QString &str, QObject *infoObj)
{
    QDeclarativeProperty prop(obj, str, qmlContext(infoObj));
    if (!prop.isValid()) {
        qmlInfo(infoObj) << QDeclarativeAbstractAnimation::tr("Cannot animate non-existent property \"%1\"").arg(str);
        return QDeclarativeProperty();
    } else if (!prop.isWritable()) {
        qmlInfo(infoObj) << QDeclarativeAbstractAnimation::tr("Cannot animate read-only property \"%1\"").arg(str);
        return QDeclarativeProperty();
    }
    return prop;
}

void QDeclarativeAbstractAnimation::setRunning(bool r)
{
    Q_D(QDeclarativeAbstractAnimation);
    if (!d->componentComplete) {
        d->running = r;
        if (r == false)
            d->avoidPropertyValueSourceStart = true;
        return;
    }

    if (d->running == r)
        return;

    if (d->group || d->disableUserControl) {
        qmlInfo(this) << "setRunning() cannot be used on non-root animation nodes.";
        return;
    }

    d->running = r;
    if (d->running) {
        if (d->alwaysRunToEnd && d->loopCount != 1
            && qtAnimation()->state() == QAbstractAnimation::Running) {
            //we've restarted before the final loop finished; restore proper loop count
            if (d->loopCount == -1)
                qtAnimation()->setLoopCount(d->loopCount);
            else
                qtAnimation()->setLoopCount(qtAnimation()->currentLoop() + d->loopCount);
        }

        if (!d->connectedTimeLine) {
            QObject::connect(qtAnimation(), SIGNAL(finished()),
                             this, SLOT(timelineComplete()));
            d->connectedTimeLine = true;
        }
        d->commence();
        emit started();
    } else {
        if (d->alwaysRunToEnd) {
            if (d->loopCount != 1)
                qtAnimation()->setLoopCount(qtAnimation()->currentLoop()+1);    //finish the current loop
        } else
            qtAnimation()->stop();

        emit completed();
    }

    emit runningChanged(d->running);
}

/*!
    \qmlproperty bool Animation::paused
    This property holds whether the animation is currently paused.

    The \c paused property can be set to declaratively control whether or not
    an animation is paused.

    Animations can also be paused and resumed imperatively from JavaScript
    using the \c pause() and \c resume() methods.

    By default, animations are not paused.
*/
bool QDeclarativeAbstractAnimation::isPaused() const
{
    Q_D(const QDeclarativeAbstractAnimation);
    return d->paused;
}

void QDeclarativeAbstractAnimation::setPaused(bool p)
{
    Q_D(QDeclarativeAbstractAnimation);
    if (d->paused == p)
        return;

    if (d->group || d->disableUserControl) {
        qmlInfo(this) << "setPaused() cannot be used on non-root animation nodes.";
        return;
    }

    d->paused = p;
    if (d->paused)
        qtAnimation()->pause();
    else
        qtAnimation()->resume();

    emit pausedChanged(d->paused);
}

void QDeclarativeAbstractAnimation::classBegin()
{
    Q_D(QDeclarativeAbstractAnimation);
    d->componentComplete = false;
}

void QDeclarativeAbstractAnimation::componentComplete()
{
    Q_D(QDeclarativeAbstractAnimation);
    d->componentComplete = true;
    if (d->running) {
        d->running = false;
        setRunning(true);
    }
}

/*!
    \qmlproperty bool Animation::alwaysRunToEnd
    This property holds whether the animation should run to completion when it is stopped.

    If this true the animation will complete its current iteration when it
    is stopped - either by setting the \c running property to false, or by
    calling the \c stop() method.  The \c complete() method is not effected
    by this value.

    This behavior is most useful when the \c repeat property is set, as the
    animation will finish playing normally but not restart.

    By default, the alwaysRunToEnd property is not set.
*/
bool QDeclarativeAbstractAnimation::alwaysRunToEnd() const
{
    Q_D(const QDeclarativeAbstractAnimation);
    return d->alwaysRunToEnd;
}

void QDeclarativeAbstractAnimation::setAlwaysRunToEnd(bool f)
{
    Q_D(QDeclarativeAbstractAnimation);
    if (d->alwaysRunToEnd == f)
        return;

    d->alwaysRunToEnd = f;
    emit alwaysRunToEndChanged(f);
}

/*!
    \qmlproperty int Animation::loops
    This property holds the number of times the animation should play.

    By default, \c loops is 1: the animation will play through once and then stop.

    If set to Animation.Infinite, the animation will continuously repeat until it is explicitly
    stopped - either by setting the \c running property to false, or by calling
    the \c stop() method.

    In the following example, the rectangle will spin indefinately.

    \code
    Rectangle {
        width: 100; height: 100; color: "green"
        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
        }
    }
    \endcode
*/
int QDeclarativeAbstractAnimation::loops() const
{
    Q_D(const QDeclarativeAbstractAnimation);
    return d->loopCount;
}

void QDeclarativeAbstractAnimation::setLoops(int loops)
{
    Q_D(QDeclarativeAbstractAnimation);
    if (loops < 0)
        loops = -1;

    if (loops == d->loopCount)
        return;

    d->loopCount = loops;
    qtAnimation()->setLoopCount(loops);
    emit loopCountChanged(loops);
}


int QDeclarativeAbstractAnimation::currentTime()
{
    return qtAnimation()->currentLoopTime();
}

void QDeclarativeAbstractAnimation::setCurrentTime(int time)
{
    qtAnimation()->setCurrentTime(time);
}

QDeclarativeAnimationGroup *QDeclarativeAbstractAnimation::group() const
{
    Q_D(const QDeclarativeAbstractAnimation);
    return d->group;
}

void QDeclarativeAbstractAnimation::setGroup(QDeclarativeAnimationGroup *g)
{
    Q_D(QDeclarativeAbstractAnimation);
    if (d->group == g)
        return;
    if (d->group)
        static_cast<QDeclarativeAnimationGroupPrivate *>(d->group->d_func())->animations.removeAll(this);

    d->group = g;

    if (d->group && !static_cast<QDeclarativeAnimationGroupPrivate *>(d->group->d_func())->animations.contains(this))
        static_cast<QDeclarativeAnimationGroupPrivate *>(d->group->d_func())->animations.append(this);

    //if (g) //if removed from a group, then the group should no longer be the parent
        setParent(g);
}

/*!
    \qmlmethod Animation::start()
    \brief Starts the animation.

    If the animation is already running, calling this method has no effect.  The
    \c running property will be true following a call to \c start().
*/
void QDeclarativeAbstractAnimation::start()
{
    setRunning(true);
}

/*!
    \qmlmethod Animation::pause()
    \brief Pauses the animation.

    If the animation is already paused, calling this method has no effect.  The
    \c paused property will be true following a call to \c pause().
*/
void QDeclarativeAbstractAnimation::pause()
{
    setPaused(true);
}

/*!
    \qmlmethod Animation::resume()
    \brief Resumes a paused animation.

    If the animation is not paused, calling this method has no effect.  The
    \c paused property will be false following a call to \c resume().
*/
void QDeclarativeAbstractAnimation::resume()
{
    setPaused(false);
}

/*!
    \qmlmethod Animation::stop()
    \brief Stops the animation.

    If the animation is not running, calling this method has no effect.  The
    \c running property will be false following a call to \c stop().

    Normally \c stop() stops the animation immediately, and the animation has
    no further influence on property values.  In this example animation
    \code
    Rectangle {
        NumberAnimation on x { from: 0; to: 100; duration: 500 }
    }
    \endcode
    was stopped at time 250ms, the \c x property will have a value of 50.

    However, if the \c alwaysRunToEnd property is set, the animation will
    continue running until it completes and then stop.  The \c running property
    will still become false immediately.
*/
void QDeclarativeAbstractAnimation::stop()
{
    setRunning(false);
}

/*!
    \qmlmethod Animation::restart()
    \brief Restarts the animation.

    This is a convenience method, and is equivalent to calling \c stop() and
    then \c start().
*/
void QDeclarativeAbstractAnimation::restart()
{
    stop();
    start();
}

/*!
    \qmlmethod Animation::complete()
    \brief Stops the animation, jumping to the final property values.

    If the animation is not running, calling this method has no effect.  The
    \c running property will be false following a call to \c complete().

    Unlike \c stop(), \c complete() immediately fast-forwards the animation to
    its end.  In the following example,
    \code
    Rectangle {
        NumberAnimation on x { from: 0; to: 100; duration: 500 }
    }
    \endcode
    calling \c stop() at time 250ms will result in the \c x property having
    a value of 50, while calling \c complete() will set the \c x property to
    100, exactly as though the animation had played the whole way through.
*/
void QDeclarativeAbstractAnimation::complete()
{
    if (isRunning()) {
         qtAnimation()->setCurrentTime(qtAnimation()->duration());
    }
}

void QDeclarativeAbstractAnimation::setTarget(const QDeclarativeProperty &p)
{
    Q_D(QDeclarativeAbstractAnimation);
    d->defaultProperty = p;

    if (!d->avoidPropertyValueSourceStart)
        setRunning(true);
}

/*
    we rely on setTarget only being called when used as a value source
    so this function allows us to do the same thing as setTarget without
    that assumption
*/
void QDeclarativeAbstractAnimation::setDefaultTarget(const QDeclarativeProperty &p)
{
    Q_D(QDeclarativeAbstractAnimation);
    d->defaultProperty = p;
}

/*
    don't allow start/stop/pause/resume to be manually invoked,
    because something else (like a Behavior) already has control
    over the animation.
*/
void QDeclarativeAbstractAnimation::setDisableUserControl()
{
    Q_D(QDeclarativeAbstractAnimation);
    d->disableUserControl = true;
}

void QDeclarativeAbstractAnimation::transition(QDeclarativeStateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_UNUSED(actions);
    Q_UNUSED(modified);
    Q_UNUSED(direction);
}

void QDeclarativeAbstractAnimation::timelineComplete()
{
    Q_D(QDeclarativeAbstractAnimation);
    setRunning(false);
    if (d->alwaysRunToEnd && d->loopCount != 1) {
        //restore the proper loopCount for the next run
        qtAnimation()->setLoopCount(d->loopCount);
    }
}

/*!
    \qmlclass PauseAnimation QDeclarativePauseAnimation
    \since 4.7
    \inherits Animation
    \brief The PauseAnimation element provides a pause for an animation.

    When used in a SequentialAnimation, PauseAnimation is a step when
    nothing happens, for a specified duration.

    A 500ms animation sequence, with a 100ms pause between two animations:
    \code
    SequentialAnimation {
        NumberAnimation { ... duration: 200 }
        PauseAnimation { duration: 100 }
        NumberAnimation { ... duration: 200 }
    }
    \endcode
*/
/*!
    \internal
    \class QDeclarativePauseAnimation
*/


QDeclarativePauseAnimation::QDeclarativePauseAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QDeclarativePauseAnimationPrivate), parent)
{
    Q_D(QDeclarativePauseAnimation);
    d->init();
}

QDeclarativePauseAnimation::~QDeclarativePauseAnimation()
{
}

void QDeclarativePauseAnimationPrivate::init()
{
    Q_Q(QDeclarativePauseAnimation);
    pa = new QPauseAnimation;
    QDeclarative_setParent_noEvent(pa, q);
}

/*!
    \qmlproperty int PauseAnimation::duration
    This property holds the duration of the pause in milliseconds

    The default value is 250.
*/
int QDeclarativePauseAnimation::duration() const
{
    Q_D(const QDeclarativePauseAnimation);
    return d->pa->duration();
}

void QDeclarativePauseAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarativePauseAnimation);
    if (d->pa->duration() == duration)
        return;
    d->pa->setDuration(duration);
    emit durationChanged(duration);
}

QAbstractAnimation *QDeclarativePauseAnimation::qtAnimation()
{
    Q_D(QDeclarativePauseAnimation);
    return d->pa;
}

/*!
    \qmlclass ColorAnimation QDeclarativeColorAnimation
    \since 4.7
    \inherits PropertyAnimation
    \brief The ColorAnimation element allows you to animate color changes.

    \code
    ColorAnimation { from: "white"; to: "#c0c0c0"; duration: 100 }
    \endcode

    When used in a transition, ColorAnimation will by default animate
    all properties of type color that are changing. If a property or properties
    are explicitly set for the animation, then those will be used instead.
*/
/*!
    \internal
    \class QDeclarativeColorAnimation
*/

QDeclarativeColorAnimation::QDeclarativeColorAnimation(QObject *parent)
: QDeclarativePropertyAnimation(parent)
{
    Q_D(QDeclarativePropertyAnimation);
    d->interpolatorType = QMetaType::QColor;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultToInterpolatorType = true;
}

QDeclarativeColorAnimation::~QDeclarativeColorAnimation()
{
}

/*!
    \qmlproperty color ColorAnimation::from
    This property holds the starting color.
*/
QColor QDeclarativeColorAnimation::from() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->from.value<QColor>();
}

void QDeclarativeColorAnimation::setFrom(const QColor &f)
{
    QDeclarativePropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty color ColorAnimation::to
    This property holds the ending color.
*/
QColor QDeclarativeColorAnimation::to() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->to.value<QColor>();
}

void QDeclarativeColorAnimation::setTo(const QColor &t)
{
    QDeclarativePropertyAnimation::setTo(t);
}



/*!
    \qmlclass ScriptAction QDeclarativeScriptAction
    \since 4.7
    \inherits Animation
    \brief The ScriptAction element allows scripts to be run during an animation.

    ScriptAction can be used to run script at a specific point in an animation.

    \qml
    SequentialAnimation {
        NumberAnimation { ... }
        ScriptAction { script: doSomething(); }
        NumberAnimation { ... }
    }
    \endqml

    When used as part of a Transition, you can also target a specific
    StateChangeScript to run using the \c scriptName property.

    \qml
    State {
        StateChangeScript {
            name: "myScript"
            script: doStateStuff();
        }
    }
    ...
    Transition {
        SequentialAnimation {
            NumberAnimation { ... }
            ScriptAction { scriptName: "myScript" }
            NumberAnimation { ... }
        }
    }
    \endqml

    \sa StateChangeScript
*/
/*!
    \internal
    \class QDeclarativeScriptAction
*/
QDeclarativeScriptAction::QDeclarativeScriptAction(QObject *parent)
    :QDeclarativeAbstractAnimation(*(new QDeclarativeScriptActionPrivate), parent)
{
    Q_D(QDeclarativeScriptAction);
    d->init();
}

QDeclarativeScriptAction::~QDeclarativeScriptAction()
{
}

void QDeclarativeScriptActionPrivate::init()
{
    Q_Q(QDeclarativeScriptAction);
    rsa = new QActionAnimation(&proxy);
    QDeclarative_setParent_noEvent(rsa, q);
}

/*!
    \qmlproperty script ScriptAction::script
    This property holds the script to run.
*/
QDeclarativeScriptString QDeclarativeScriptAction::script() const
{
    Q_D(const QDeclarativeScriptAction);
    return d->script;
}

void QDeclarativeScriptAction::setScript(const QDeclarativeScriptString &script)
{
    Q_D(QDeclarativeScriptAction);
    d->script = script;
}

/*!
    \qmlproperty QString ScriptAction::scriptName
    This property holds the the name of the StateChangeScript to run.

    This property is only valid when ScriptAction is used as part of a transition.
    If both script and scriptName are set, scriptName will be used.

    \note When using scriptName in a reversible transition, the script will only
    be run when the transition is being run forwards.
*/
QString QDeclarativeScriptAction::stateChangeScriptName() const
{
    Q_D(const QDeclarativeScriptAction);
    return d->name;
}

void QDeclarativeScriptAction::setStateChangeScriptName(const QString &name)
{
    Q_D(QDeclarativeScriptAction);
    d->name = name;
}

void QDeclarativeScriptActionPrivate::execute()
{
    Q_Q(QDeclarativeScriptAction);
    if (hasRunScriptScript && reversing)
        return;

    QDeclarativeScriptString scriptStr = hasRunScriptScript ? runScriptScript : script;

    const QString &str = scriptStr.script();
    if (!str.isEmpty()) {
        QDeclarativeExpression expr(scriptStr.context(), str, scriptStr.scopeObject());
        QDeclarativeData *ddata = QDeclarativeData::get(q);
        if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
            expr.setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber);
        expr.evaluate();
        if (expr.hasError()) 
            qmlInfo(q) << expr.error();
    }
}

void QDeclarativeScriptAction::transition(QDeclarativeStateActions &actions,
                                    QDeclarativeProperties &modified,
                                    TransitionDirection direction)
{
    Q_D(QDeclarativeScriptAction);
    Q_UNUSED(modified);

    d->hasRunScriptScript = false;
    d->reversing = (direction == Backward);
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];

        if (action.event && action.event->typeName() == QLatin1String("StateChangeScript")
            && static_cast<QDeclarativeStateChangeScript*>(action.event)->name() == d->name) {
            d->runScriptScript = static_cast<QDeclarativeStateChangeScript*>(action.event)->script();
            d->hasRunScriptScript = true;
            action.actionDone = true;
            break;  //only match one (names should be unique)
        }
    }
}

QAbstractAnimation *QDeclarativeScriptAction::qtAnimation()
{
    Q_D(QDeclarativeScriptAction);
    return d->rsa;
}



/*!
    \qmlclass PropertyAction QDeclarativePropertyAction
    \since 4.7
    \inherits Animation
    \brief The PropertyAction element allows immediate property changes during animation.

    Explicitly set \c theimage.smooth=true during a transition:
    \code
    PropertyAction { target: theimage; property: "smooth"; value: true }
    \endcode

    Set \c thewebview.url to the value set for the destination state:
    \code
    PropertyAction { target: thewebview; property: "url" }
    \endcode

    The PropertyAction is immediate -
    the target property is not animated to the selected value in any way.
*/
/*!
    \internal
    \class QDeclarativePropertyAction
*/
QDeclarativePropertyAction::QDeclarativePropertyAction(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QDeclarativePropertyActionPrivate), parent)
{
    Q_D(QDeclarativePropertyAction);
    d->init();
}

QDeclarativePropertyAction::~QDeclarativePropertyAction()
{
}

void QDeclarativePropertyActionPrivate::init()
{
    Q_Q(QDeclarativePropertyAction);
    spa = new QActionAnimation;
    QDeclarative_setParent_noEvent(spa, q);
}

/*!
    \qmlproperty Object PropertyAction::target
    This property holds an explicit target object to animate.

    The exact effect of the \c target property depends on how the animation
    is being used.  Refer to the \l animation documentation for details.
*/

QObject *QDeclarativePropertyAction::target() const
{
    Q_D(const QDeclarativePropertyAction);
    return d->target;
}

void QDeclarativePropertyAction::setTarget(QObject *o)
{
    Q_D(QDeclarativePropertyAction);
    if (d->target == o)
        return;
    d->target = o;
    emit targetChanged(d->target, d->propertyName);
}

QString QDeclarativePropertyAction::property() const
{
    Q_D(const QDeclarativePropertyAction);
    return d->propertyName;
}

void QDeclarativePropertyAction::setProperty(const QString &n)
{
    Q_D(QDeclarativePropertyAction);
    if (d->propertyName == n)
        return;
    d->propertyName = n;
    emit targetChanged(d->target, d->propertyName);
}

/*!
    \qmlproperty list<Object> PropertyAction::targets
    \qmlproperty string PropertyAction::property
    \qmlproperty string PropertyAction::properties
    \qmlproperty Object PropertyAction::target

    These properties are used as a set to determine which properties should be
    affected by this action.

    The details of how these properties are interpreted in different situations
    is covered in the \l{PropertyAnimation::properties}{corresponding} PropertyAnimation
    documentation.

    \sa exclude
*/
QString QDeclarativePropertyAction::properties() const
{
    Q_D(const QDeclarativePropertyAction);
    return d->properties;
}

void QDeclarativePropertyAction::setProperties(const QString &p)
{
    Q_D(QDeclarativePropertyAction);
    if (d->properties == p)
        return;
    d->properties = p;
    emit propertiesChanged(p);
}

QDeclarativeListProperty<QObject> QDeclarativePropertyAction::targets()
{
    Q_D(QDeclarativePropertyAction);
    return QDeclarativeListProperty<QObject>(this, d->targets);
}

/*!
    \qmlproperty list<Object> PropertyAction::exclude
    This property holds the objects not to be affected by this animation.

    \sa targets
*/
QDeclarativeListProperty<QObject> QDeclarativePropertyAction::exclude()
{
    Q_D(QDeclarativePropertyAction);
    return QDeclarativeListProperty<QObject>(this, d->exclude);
}

/*!
    \qmlproperty any PropertyAction::value
    This property holds the value to be set on the property.
    If not set, then the value defined for the end state of the transition.
*/
QVariant QDeclarativePropertyAction::value() const
{
    Q_D(const QDeclarativePropertyAction);
    return d->value;
}

void QDeclarativePropertyAction::setValue(const QVariant &v)
{
    Q_D(QDeclarativePropertyAction);
    if (d->value.isNull || d->value != v) {
        d->value = v;
        emit valueChanged(v);
    }
}

QAbstractAnimation *QDeclarativePropertyAction::qtAnimation()
{
    Q_D(QDeclarativePropertyAction);
    return d->spa;
}

void QDeclarativePropertyAction::transition(QDeclarativeStateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_D(QDeclarativePropertyAction);
    Q_UNUSED(direction);

    struct QDeclarativeSetPropertyAnimationAction : public QAbstractAnimationAction
    {
        QDeclarativeStateActions actions;
        virtual void doAction()
        {
            for (int ii = 0; ii < actions.count(); ++ii) {
                const QDeclarativeAction &action = actions.at(ii);
                QDeclarativePropertyPrivate::write(action.property, action.toValue, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
            }
        }
    };

    QStringList props = d->properties.isEmpty() ? QStringList() : d->properties.split(QLatin1Char(','));
    for (int ii = 0; ii < props.count(); ++ii)
        props[ii] = props.at(ii).trimmed();
    if (!d->propertyName.isEmpty())
        props << d->propertyName;

    QList<QObject*> targets = d->targets;
    if (d->target)
        targets.append(d->target);

    bool hasSelectors = !props.isEmpty() || !targets.isEmpty() || !d->exclude.isEmpty();

    if (d->defaultProperty.isValid() && !hasSelectors) {
        props << d->defaultProperty.name();
        targets << d->defaultProperty.object();
    }

    QDeclarativeSetPropertyAnimationAction *data = new QDeclarativeSetPropertyAnimationAction;

    bool hasExplicit = false;
    //an explicit animation has been specified
    if (d->value.isValid()) {
        for (int i = 0; i < props.count(); ++i) {
            for (int j = 0; j < targets.count(); ++j) {
                QDeclarativeAction myAction;
                myAction.property = d->createProperty(targets.at(j), props.at(i), this);
                if (myAction.property.isValid()) {
                    myAction.toValue = d->value;
                    QDeclarativePropertyAnimationPrivate::convertVariant(myAction.toValue, myAction.property.propertyType());
                    data->actions << myAction;
                    hasExplicit = true;
                    for (int ii = 0; ii < actions.count(); ++ii) {
                        QDeclarativeAction &action = actions[ii];
                        if (action.property.object() == myAction.property.object() &&
                            myAction.property.name() == action.property.name()) {
                            modified << action.property;
                            break;  //### any chance there could be multiples?
                        }
                    }
                }
            }
        }
    }

    if (!hasExplicit)
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];

        QObject *obj = action.property.object();
        QString propertyName = action.property.name();
        QObject *sObj = action.specifiedObject;
        QString sPropertyName = action.specifiedProperty;
        bool same = (obj == sObj);

        if ((targets.isEmpty() || targets.contains(obj) || (!same && targets.contains(sObj))) &&
           (!d->exclude.contains(obj)) && (same || (!d->exclude.contains(sObj))) &&
           (props.contains(propertyName) || (!same && props.contains(sPropertyName)))) {
            QDeclarativeAction myAction = action;

            if (d->value.isValid())
                myAction.toValue = d->value;
            QDeclarativePropertyAnimationPrivate::convertVariant(myAction.toValue, myAction.property.propertyType());

            modified << action.property;
            data->actions << myAction;
            action.fromValue = myAction.toValue;
        }
    }

    if (data->actions.count()) {
        d->spa->setAnimAction(data, QAbstractAnimation::DeleteWhenStopped);
    } else {
        delete data;
    }
}

/*!
    \qmlclass NumberAnimation QDeclarativeNumberAnimation
    \since 4.7
    \inherits PropertyAnimation
    \brief The NumberAnimation element allows you to animate changes in properties of type qreal.

    Animate a set of properties over 200ms, from their values in the start state to
    their values in the end state of the transition:
    \code
    NumberAnimation { properties: "x,y,scale"; duration: 200 }
    \endcode
*/

/*!
    \internal
    \class QDeclarativeNumberAnimation
*/

QDeclarativeNumberAnimation::QDeclarativeNumberAnimation(QObject *parent)
: QDeclarativePropertyAnimation(parent)
{
    init();
}

QDeclarativeNumberAnimation::QDeclarativeNumberAnimation(QDeclarativePropertyAnimationPrivate &dd, QObject *parent)
: QDeclarativePropertyAnimation(dd, parent)
{
    init();
}

QDeclarativeNumberAnimation::~QDeclarativeNumberAnimation()
{
}

void QDeclarativeNumberAnimation::init()
{
    Q_D(QDeclarativePropertyAnimation);
    d->interpolatorType = QMetaType::QReal;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
}

/*!
    \qmlproperty real NumberAnimation::from
    This property holds the starting value.
    If not set, then the value defined in the start state of the transition.
*/
qreal QDeclarativeNumberAnimation::from() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->from.toReal();
}

void QDeclarativeNumberAnimation::setFrom(qreal f)
{
    QDeclarativePropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real NumberAnimation::to
    This property holds the ending value.
    If not set, then the value defined in the end state of the transition or Behavior.
*/
qreal QDeclarativeNumberAnimation::to() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->to.toReal();
}

void QDeclarativeNumberAnimation::setTo(qreal t)
{
    QDeclarativePropertyAnimation::setTo(t);
}



/*!
    \qmlclass Vector3dAnimation QDeclarativeVector3dAnimation
    \since 4.7
    \inherits PropertyAnimation
    \brief The Vector3dAnimation element allows you to animate changes in properties of type QVector3d.
*/

/*!
    \internal
    \class QDeclarativeVector3dAnimation
*/

QDeclarativeVector3dAnimation::QDeclarativeVector3dAnimation(QObject *parent)
: QDeclarativePropertyAnimation(parent)
{
    Q_D(QDeclarativePropertyAnimation);
    d->interpolatorType = QMetaType::QVector3D;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultToInterpolatorType = true;
}

QDeclarativeVector3dAnimation::~QDeclarativeVector3dAnimation()
{
}

/*!
    \qmlproperty real Vector3dAnimation::from
    This property holds the starting value.
    If not set, then the value defined in the start state of the transition.
*/
QVector3D QDeclarativeVector3dAnimation::from() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->from.value<QVector3D>();
}

void QDeclarativeVector3dAnimation::setFrom(QVector3D f)
{
    QDeclarativePropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real Vector3dAnimation::to
    This property holds the ending value.
    If not set, then the value defined in the end state of the transition or Behavior.
*/
QVector3D QDeclarativeVector3dAnimation::to() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->to.value<QVector3D>();
}

void QDeclarativeVector3dAnimation::setTo(QVector3D t)
{
    QDeclarativePropertyAnimation::setTo(t);
}



/*!
    \qmlclass RotationAnimation QDeclarativeRotationAnimation
    \since 4.7
    \inherits PropertyAnimation
    \brief The RotationAnimation element allows you to animate rotations.

    RotationAnimation is a specialized PropertyAnimation that gives control
    over the direction of rotation. By default, it will rotate in the direction
    of the numerical change; a rotation from 0 to 240 will rotate 220 degrees
    clockwise, while a rotation from 240 to 0 will rotate 220 degrees
    counterclockwise.

    When used in a transition RotationAnimation will rotate all
    properties named "rotation" or "angle". You can override this by providing
    your own properties via \c properties or \c property.

    In the following example we use RotationAnimation to animate the rotation
    between states via the shortest path.
    \qml
    states: {
        State { name: "180"; PropertyChanges { target: myItem; rotation: 180 } }
        State { name: "90"; PropertyChanges { target: myItem; rotation: 90 } }
        State { name: "-90"; PropertyChanges { target: myItem; rotation: -90 } }
    }
    transition: Transition {
        RotationAnimation { direction: RotationAnimation.Shortest }
    }
    \endqml
*/

/*!
    \internal
    \class QDeclarativeRotationAnimation
*/

QVariant _q_interpolateShortestRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff > 180.0){
        newt -= 360.0;
        diff -= 360.0;
    }
    while(diff < -180.0){
        newt += 360.0;
        diff += 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QVariant _q_interpolateClockwiseRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff < 0.0){
        newt += 360.0;
        diff += 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QVariant _q_interpolateCounterclockwiseRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff > 0.0){
        newt -= 360.0;
        diff -= 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QDeclarativeRotationAnimation::QDeclarativeRotationAnimation(QObject *parent)
: QDeclarativePropertyAnimation(*(new QDeclarativeRotationAnimationPrivate), parent)
{
    Q_D(QDeclarativeRotationAnimation);
    d->interpolatorType = QMetaType::QReal;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultProperties = QLatin1String("rotation,angle");
}

QDeclarativeRotationAnimation::~QDeclarativeRotationAnimation()
{
}

/*!
    \qmlproperty real RotationAnimation::from
    This property holds the starting value.
    If not set, then the value defined in the start state of the transition.
*/
qreal QDeclarativeRotationAnimation::from() const
{
    Q_D(const QDeclarativeRotationAnimation);
    return d->from.toReal();
}

void QDeclarativeRotationAnimation::setFrom(qreal f)
{
    QDeclarativePropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real RotationAnimation::to
    This property holds the ending value.
    If not set, then the value defined in the end state of the transition or Behavior.
*/
qreal QDeclarativeRotationAnimation::to() const
{
    Q_D(const QDeclarativeRotationAnimation);
    return d->to.toReal();
}

void QDeclarativeRotationAnimation::setTo(qreal t)
{
    QDeclarativePropertyAnimation::setTo(t);
}

/*!
    \qmlproperty enumeration RotationAnimation::direction
    The direction in which to rotate.
    Possible values are Numerical, Clockwise, Counterclockwise,
    or Shortest.

    \table
    \row
        \o Numerical
        \o Rotate by linearly interpolating between the two numbers.
           A rotation from 10 to 350 will rotate 340 degrees clockwise.
    \row
        \o Clockwise
        \o Rotate clockwise between the two values
    \row
        \o Counterclockwise
        \o Rotate counterclockwise between the two values
    \row
        \o Shortest
        \o Rotate in the direction that produces the shortest animation path.
           A rotation from 10 to 350 will rotate 20 degrees counterclockwise.
    \endtable

    The default direction is Numerical.
*/
QDeclarativeRotationAnimation::RotationDirection QDeclarativeRotationAnimation::direction() const
{
    Q_D(const QDeclarativeRotationAnimation);
    return d->direction;
}

void QDeclarativeRotationAnimation::setDirection(QDeclarativeRotationAnimation::RotationDirection direction)
{
    Q_D(QDeclarativeRotationAnimation);
    if (d->direction == direction)
        return;

    d->direction = direction;
    switch(d->direction) {
    case Clockwise:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateClockwiseRotation);
        break;
    case Counterclockwise:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateCounterclockwiseRotation);
        break;
    case Shortest:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateShortestRotation);
        break;
    default:
        d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
        break;
    }

    emit directionChanged();
}



QDeclarativeAnimationGroup::QDeclarativeAnimationGroup(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QDeclarativeAnimationGroupPrivate), parent)
{
}

QDeclarativeAnimationGroup::QDeclarativeAnimationGroup(QDeclarativeAnimationGroupPrivate &dd, QObject *parent)
    : QDeclarativeAbstractAnimation(dd, parent)
{
}

void QDeclarativeAnimationGroupPrivate::append_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, QDeclarativeAbstractAnimation *a)
{
    QDeclarativeAnimationGroup *q = qobject_cast<QDeclarativeAnimationGroup *>(list->object);
    if (q) {
        a->setGroup(q);
        QDeclarative_setParent_noEvent(a->qtAnimation(), q->d_func()->ag);
        q->d_func()->ag->addAnimation(a->qtAnimation());
    }
}

void QDeclarativeAnimationGroupPrivate::clear_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list)
{
    QDeclarativeAnimationGroup *q = qobject_cast<QDeclarativeAnimationGroup *>(list->object);
    if (q) {
        for (int i = 0; i < q->d_func()->animations.count(); ++i)
            q->d_func()->animations.at(i)->setGroup(0);
        q->d_func()->animations.clear();
    }
}

QDeclarativeAnimationGroup::~QDeclarativeAnimationGroup()
{
}

QDeclarativeListProperty<QDeclarativeAbstractAnimation> QDeclarativeAnimationGroup::animations()
{
    Q_D(QDeclarativeAnimationGroup);
    QDeclarativeListProperty<QDeclarativeAbstractAnimation> list(this, d->animations);
    list.append = &QDeclarativeAnimationGroupPrivate::append_animation;
    list.clear = &QDeclarativeAnimationGroupPrivate::clear_animation;
    return list;
}

/*!
    \qmlclass SequentialAnimation QDeclarativeSequentialAnimation
    \since 4.7
    \inherits Animation
    \brief The SequentialAnimation element allows you to run animations sequentially.

    Animations controlled in SequentialAnimation will be run one after the other.

    The following example chains two numeric animations together.  The \c MyItem
    object will animate from its current x position to 100, and then back to 0.

    \code
    SequentialAnimation {
        NumberAnimation { target: MyItem; property: "x"; to: 100 }
        NumberAnimation { target: MyItem; property: "x"; to: 0 }
    }
    \endcode

    \sa ParallelAnimation
*/

QDeclarativeSequentialAnimation::QDeclarativeSequentialAnimation(QObject *parent) :
    QDeclarativeAnimationGroup(parent)
{
    Q_D(QDeclarativeAnimationGroup);
    d->ag = new QSequentialAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, this);
}

QDeclarativeSequentialAnimation::~QDeclarativeSequentialAnimation()
{
}

QAbstractAnimation *QDeclarativeSequentialAnimation::qtAnimation()
{
    Q_D(QDeclarativeAnimationGroup);
    return d->ag;
}

void QDeclarativeSequentialAnimation::transition(QDeclarativeStateActions &actions,
                                    QDeclarativeProperties &modified,
                                    TransitionDirection direction)
{
    Q_D(QDeclarativeAnimationGroup);

    int inc = 1;
    int from = 0;
    if (direction == Backward) {
        inc = -1;
        from = d->animations.count() - 1;
    }

    bool valid = d->defaultProperty.isValid();
    for (int ii = from; ii < d->animations.count() && ii >= 0; ii += inc) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }
}



/*!
    \qmlclass ParallelAnimation QDeclarativeParallelAnimation
    \since 4.7
    \inherits Animation
    \brief The ParallelAnimation element allows you to run animations in parallel.

    Animations contained in ParallelAnimation will be run at the same time.

    The following animation demonstrates animating the \c MyItem item
    to (100,100) by animating the x and y properties in parallel.

    \code
    ParallelAnimation {
        NumberAnimation { target: MyItem; property: "x"; to: 100 }
        NumberAnimation { target: MyItem; property: "y"; to: 100 }
    }
    \endcode

    \sa SequentialAnimation
*/
/*!
    \internal
    \class QDeclarativeParallelAnimation
*/

QDeclarativeParallelAnimation::QDeclarativeParallelAnimation(QObject *parent) :
    QDeclarativeAnimationGroup(parent)
{
    Q_D(QDeclarativeAnimationGroup);
    d->ag = new QParallelAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, this);
}

QDeclarativeParallelAnimation::~QDeclarativeParallelAnimation()
{
}

QAbstractAnimation *QDeclarativeParallelAnimation::qtAnimation()
{
    Q_D(QDeclarativeAnimationGroup);
    return d->ag;
}

void QDeclarativeParallelAnimation::transition(QDeclarativeStateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_D(QDeclarativeAnimationGroup);
    bool valid = d->defaultProperty.isValid();
    for (int ii = 0; ii < d->animations.count(); ++ii) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }
}



//convert a variant from string type to another animatable type
void QDeclarativePropertyAnimationPrivate::convertVariant(QVariant &variant, int type)
{
    if (variant.userType() != QVariant::String) {
        variant.convert((QVariant::Type)type);
        return;
    }

    switch (type) {
    case QVariant::Rect: {
        variant.setValue(QDeclarativeStringConverters::rectFFromString(variant.toString()).toRect());
        break;
    }
    case QVariant::RectF: {
        variant.setValue(QDeclarativeStringConverters::rectFFromString(variant.toString()));
        break;
    }
    case QVariant::Point: {
        variant.setValue(QDeclarativeStringConverters::pointFFromString(variant.toString()).toPoint());
        break;
    }
    case QVariant::PointF: {
        variant.setValue(QDeclarativeStringConverters::pointFFromString(variant.toString()));
        break;
    }
    case QVariant::Size: {
        variant.setValue(QDeclarativeStringConverters::sizeFFromString(variant.toString()).toSize());
        break;
    }
    case QVariant::SizeF: {
        variant.setValue(QDeclarativeStringConverters::sizeFFromString(variant.toString()));
        break;
    }
    case QVariant::Color: {
        variant.setValue(QDeclarativeStringConverters::colorFromString(variant.toString()));
        break;
    }
    case QVariant::Vector3D: {
        variant.setValue(QDeclarativeStringConverters::vector3DFromString(variant.toString()));
        break;
    }
    default:
        if (QDeclarativeValueTypeFactory::isValueType((uint)type)) {
            variant.convert((QVariant::Type)type);
        } else {
            QDeclarativeMetaType::StringConverter converter = QDeclarativeMetaType::customStringConverter(type);
            if (converter)
                variant = converter(variant.toString());
        }
        break;
    }
}

/*!
    \qmlclass PropertyAnimation QDeclarativePropertyAnimation
    \since 4.7
    \inherits Animation
    \brief The PropertyAnimation element allows you to animate property changes.

    PropertyAnimation provides a way to animate changes to a property's value. It can
    be used in many different situations:
    \list
    \o In a Transition

    Animate any objects that have changed their x or y properties in the target state using
    an InOutQuad easing curve:
    \qml
    Transition { PropertyAnimation { properties: "x,y"; easing.type: "InOutQuad" } }
    \endqml
    \o In a Behavior

    Animate all changes to a rectangle's x property.
    \qml
    Rectangle {
        Behavior on x { PropertyAnimation {} }
    }
    \endqml
    \o As a property value source

    Repeatedly animate the rectangle's x property.
    \qml
    Rectangle {
        SequentialAnimation on x {
            loops: Animation.Infinite
            PropertyAnimation { to: 50 }
            PropertyAnimation { to: 0 }
        }
    }
    \endqml
    \o In a signal handler

    Fade out \c theObject when clicked:
    \qml
    MouseArea {
        anchors.fill: theObject
        onClicked: PropertyAnimation { target: theObject; property: "opacity"; to: 0 }
    }
    \endqml
    \o Standalone

    Animate \c theObject's size property over 200ms, from its current size to 20-by-20:
    \qml
    PropertyAnimation { target: theObject; property: "size"; to: "20x20"; duration: 200 }
    \endqml
    \endlist

    Depending on how the animation is used, the set of properties normally used will be
    different. For more information see the individual property documentation, as well
    as the \l{QML Animation} introduction.
*/

QDeclarativePropertyAnimation::QDeclarativePropertyAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QDeclarativePropertyAnimationPrivate), parent)
{
    Q_D(QDeclarativePropertyAnimation);
    d->init();
}

QDeclarativePropertyAnimation::QDeclarativePropertyAnimation(QDeclarativePropertyAnimationPrivate &dd, QObject *parent)
: QDeclarativeAbstractAnimation(dd, parent)
{
    Q_D(QDeclarativePropertyAnimation);
    d->init();
}

QDeclarativePropertyAnimation::~QDeclarativePropertyAnimation()
{
}

void QDeclarativePropertyAnimationPrivate::init()
{
    Q_Q(QDeclarativePropertyAnimation);
    va = new QDeclarativeBulkValueAnimator;
    QDeclarative_setParent_noEvent(va, q);
}

/*!
    \qmlproperty int PropertyAnimation::duration
    This property holds the duration of the animation, in milliseconds.

    The default value is 250.
*/
int QDeclarativePropertyAnimation::duration() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->va->duration();
}

void QDeclarativePropertyAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarativePropertyAnimation);
    if (d->va->duration() == duration)
        return;
    d->va->setDuration(duration);
    emit durationChanged(duration);
}

/*!
    \qmlproperty real PropertyAnimation::from
    This property holds the starting value.
    If not set, then the value defined in the start state of the transition.
*/
QVariant QDeclarativePropertyAnimation::from() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->from;
}

void QDeclarativePropertyAnimation::setFrom(const QVariant &f)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->fromIsDefined && f == d->from)
        return;
    d->from = f;
    d->fromIsDefined = f.isValid();
    emit fromChanged(f);
}

/*!
    \qmlproperty real PropertyAnimation::to
    This property holds the ending value.
    If not set, then the value defined in the end state of the transition or Behavior.
*/
QVariant QDeclarativePropertyAnimation::to() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->to;
}

void QDeclarativePropertyAnimation::setTo(const QVariant &t)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->toIsDefined && t == d->to)
        return;
    d->to = t;
    d->toIsDefined = t.isValid();
    emit toChanged(t);
}

/*!
    \qmlproperty enumeration PropertyAnimation::easing.type
    \qmlproperty real PropertyAnimation::easing.amplitude
    \qmlproperty real PropertyAnimation::easing.overshoot
    \qmlproperty real PropertyAnimation::easing.period
    \brief the easing curve used for the animation.

    To specify an easing curve you need to specify at least the type. For some curves you can also specify
    amplitude, period and/or overshoot (more details provided after the table). The default easing curve is
    Linear.

    \qml
    PropertyAnimation { properties: "y"; easing.type: "InOutElastic"; easing.amplitude: 2.0; easing.period: 1.5 }
    \endqml

    Available types are:

    \table
    \row
        \o \c Linear
        \o Easing curve for a linear (t) function: velocity is constant.
        \o \inlineimage qeasingcurve-linear.png
    \row
        \o \c InQuad
        \o Easing curve for a quadratic (t^2) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquad.png
    \row
        \o \c OutQuad
        \o Easing curve for a quadratic (t^2) function: decelerating to zero velocity.
        \o \inlineimage qeasingcurve-outquad.png
    \row
        \o \c InOutQuad
        \o Easing curve for a quadratic (t^2) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquad.png
    \row
        \o \c OutInQuad
        \o Easing curve for a quadratic (t^2) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquad.png
    \row
        \o \c InCubic
        \o Easing curve for a cubic (t^3) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-incubic.png
    \row
        \o \c OutCubic
        \o Easing curve for a cubic (t^3) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outcubic.png
    \row
        \o \c InOutCubic
        \o Easing curve for a cubic (t^3) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutcubic.png
    \row
        \o \c OutInCubic
        \o Easing curve for a cubic (t^3) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outincubic.png
    \row
        \o \c InQuart
        \o Easing curve for a quartic (t^4) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquart.png
    \row
        \o \c OutQuart
        \o Easing curve for a cubic (t^4) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outquart.png
    \row
        \o \c InOutQuart
        \o Easing curve for a cubic (t^4) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquart.png
    \row
        \o \c OutInQuart
        \o Easing curve for a cubic (t^4) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquart.png
    \row
        \o \c InQuint
        \o Easing curve for a quintic (t^5) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquint.png
    \row
        \o \c OutQuint
        \o Easing curve for a cubic (t^5) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outquint.png
    \row
        \o \c InOutQuint
        \o Easing curve for a cubic (t^5) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquint.png
    \row
        \o \c OutInQuint
        \o Easing curve for a cubic (t^5) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquint.png
    \row
        \o \c InSine
        \o Easing curve for a sinusoidal (sin(t)) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-insine.png
    \row
        \o \c OutSine
        \o Easing curve for a sinusoidal (sin(t)) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outsine.png
    \row
        \o \c InOutSine
        \o Easing curve for a sinusoidal (sin(t)) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutsine.png
    \row
        \o \c OutInSine
        \o Easing curve for a sinusoidal (sin(t)) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinsine.png
    \row
        \o \c InExpo
        \o Easing curve for an exponential (2^t) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inexpo.png
    \row
        \o \c OutExpo
        \o Easing curve for an exponential (2^t) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outexpo.png
    \row
        \o \c InOutExpo
        \o Easing curve for an exponential (2^t) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutexpo.png
    \row
        \o \c OutInExpo
        \o Easing curve for an exponential (2^t) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinexpo.png
    \row
        \o \c InCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-incirc.png
    \row
        \o \c OutCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outcirc.png
    \row
        \o \c InOutCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutcirc.png
    \row
        \o \c OutInCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outincirc.png
    \row
        \o \c InElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: accelerating from zero velocity.
        \br The peak amplitude can be set with the \e amplitude parameter, and the period of decay by the \e period parameter.
        \o \inlineimage qeasingcurve-inelastic.png
    \row
        \o \c OutElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: decelerating from zero velocity.
        \br The peak amplitude can be set with the \e amplitude parameter, and the period of decay by the \e period parameter.
        \o \inlineimage qeasingcurve-outelastic.png
    \row
        \o \c InOutElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutelastic.png
    \row
        \o \c OutInElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinelastic.png
    \row
        \o \c InBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing in: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inback.png
    \row
        \o \c OutBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing out: decelerating to zero velocity.
        \o \inlineimage qeasingcurve-outback.png
    \row
        \o \c InOutBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing in/out: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutback.png
    \row
        \o \c OutInBack
        \o Easing curve for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing out/in: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinback.png
    \row
        \o \c InBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inbounce.png
    \row
        \o \c OutBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outbounce.png
    \row
        \o \c InOutBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function easing in/out: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutbounce.png
    \row
        \o \c OutInBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function easing out/in: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinbounce.png
    \endtable

    easing.amplitude is not applicable for all curve types. It is only applicable for bounce and elastic curves (curves of type
    QEasingCurve::InBounce, QEasingCurve::OutBounce, QEasingCurve::InOutBounce, QEasingCurve::OutInBounce, QEasingCurve::InElastic,
    QEasingCurve::OutElastic, QEasingCurve::InOutElastic or QEasingCurve::OutInElastic).

    easing.overshoot is not applicable for all curve types. It is only applicable if type is: QEasingCurve::InBack, QEasingCurve::OutBack,
    QEasingCurve::InOutBack or QEasingCurve::OutInBack.

    easing.period is not applicable for all curve types. It is only applicable if type is: QEasingCurve::InElastic, QEasingCurve::OutElastic,
    QEasingCurve::InOutElastic or QEasingCurve::OutInElastic.
*/
QEasingCurve QDeclarativePropertyAnimation::easing() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->easing;
}

void QDeclarativePropertyAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->easing == e)
        return;

    d->easing = e;
    d->va->setEasingCurve(d->easing);
    emit easingChanged(e);
}

QObject *QDeclarativePropertyAnimation::target() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->target;
}

void QDeclarativePropertyAnimation::setTarget(QObject *o)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->target == o)
        return;
    d->target = o;
    emit targetChanged(d->target, d->propertyName);
}

QString QDeclarativePropertyAnimation::property() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->propertyName;
}

void QDeclarativePropertyAnimation::setProperty(const QString &n)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->propertyName == n)
        return;
    d->propertyName = n;
    emit targetChanged(d->target, d->propertyName);
}

QString QDeclarativePropertyAnimation::properties() const
{
    Q_D(const QDeclarativePropertyAnimation);
    return d->properties;
}

void QDeclarativePropertyAnimation::setProperties(const QString &prop)
{
    Q_D(QDeclarativePropertyAnimation);
    if (d->properties == prop)
        return;

    d->properties = prop;
    emit propertiesChanged(prop);
}

/*!
    \qmlproperty string PropertyAnimation::properties
    \qmlproperty list<Object> PropertyAnimation::targets
    \qmlproperty string PropertyAnimation::property
    \qmlproperty Object PropertyAnimation::target

    These properties are used as a set to determine which properties should be animated.
    The singular and plural forms are functionally identical, e.g.
    \qml
    NumberAnimation { target: theItem; property: "x"; to: 500 }
    \endqml
    has the same meaning as
    \qml
    NumberAnimation { targets: theItem; properties: "x"; to: 500 }
    \endqml
    The singular forms are slightly optimized, so if you do have only a single target/property
    to animate you should try to use them.

    In many cases these properties do not need to be explicitly specified -- they can be
    inferred from the animation framework.
    \table 80%
    \row
    \o Value Source / Behavior
    \o When an animation is used as a value source or in a Behavior, the default target and property
       name to be animated can both be inferred.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           NumberAnimation on x { to: 500; loops: Animation.Infinite } //animate theRect's x property
           Behavior on y { NumberAnimation {} } //animate theRect's y property
       }
       \endqml
    \row
    \o Transition
    \o When used in a transition, a property animation is assumed to match \e all targets
       but \e no properties. In practice, that means you need to specify at least the properties
       in order for the animation to do anything.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           Item { id: uselessItem }
           states: State {
               name: "state1"
               PropertyChanges { target: theRect; x: 200; y: 200; z: 4 }
               PropertyChanges { target: uselessItem; x: 10; y: 10; z: 2 }
           }
           transitions: Transition {
               //animate both theRect's and uselessItem's x and y to their final values
               NumberAnimation { properties: "x,y" }

               //animate theRect's z to its final value
               NumberAnimation { target: theRect; property: "z" }
           }
       }
       \endqml
    \row
    \o Standalone
    \o When an animation is used standalone, both the target and property need to be
       explicitly specified.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           //need to explicitly specify target and property
           NumberAnimation { id: theAnim; target: theRect; property: "x" to: 500 }
           MouseArea {
               anchors.fill: parent
               onClicked: theAnim.start()
           }
       }
       \endqml
    \endtable

    As seen in the above example, properties is specified as a comma-separated string of property names to animate.

    \sa exclude
*/
QDeclarativeListProperty<QObject> QDeclarativePropertyAnimation::targets()
{
    Q_D(QDeclarativePropertyAnimation);
    return QDeclarativeListProperty<QObject>(this, d->targets);
}

/*!
    \qmlproperty list<Object> PropertyAnimation::exclude
    This property holds the items not to be affected by this animation.
    \sa PropertyAnimation::targets
*/
QDeclarativeListProperty<QObject> QDeclarativePropertyAnimation::exclude()
{
    Q_D(QDeclarativePropertyAnimation);
    return QDeclarativeListProperty<QObject>(this, d->exclude);
}

QAbstractAnimation *QDeclarativePropertyAnimation::qtAnimation()
{
    Q_D(QDeclarativePropertyAnimation);
    return d->va;
}

struct PropertyUpdater : public QDeclarativeBulkValueUpdater
{
    QDeclarativeStateActions actions;
    int interpolatorType;       //for Number/ColorAnimation
    int prevInterpolatorType;   //for generic
    QVariantAnimation::Interpolator interpolator;
    bool reverse;
    bool fromSourced;
    bool fromDefined;
    bool *wasDeleted;
    PropertyUpdater() : prevInterpolatorType(0), wasDeleted(0) {}
    ~PropertyUpdater() { if (wasDeleted) *wasDeleted = true; }
    void setValue(qreal v)
    {
        bool deleted = false;
        wasDeleted = &deleted;
        if (reverse)    //QVariantAnimation sends us 1->0 when reversed, but we are expecting 0->1
            v = 1 - v;
        for (int ii = 0; ii < actions.count(); ++ii) {
            QDeclarativeAction &action = actions[ii];

            if (v == 1.)
                QDeclarativePropertyPrivate::write(action.property, action.toValue, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
            else {
                if (!fromSourced && !fromDefined) {
                    action.fromValue = action.property.read();
                    if (interpolatorType)
                        QDeclarativePropertyAnimationPrivate::convertVariant(action.fromValue, interpolatorType);
                }
                if (!interpolatorType) {
                    int propType = action.property.propertyType();
                    if (!prevInterpolatorType || prevInterpolatorType != propType) {
                        prevInterpolatorType = propType;
                        interpolator = QVariantAnimationPrivate::getInterpolator(prevInterpolatorType);
                    }
                }
                if (interpolator)
                    QDeclarativePropertyPrivate::write(action.property, interpolator(action.fromValue.constData(), action.toValue.constData(), v), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
            }
            if (deleted)
                return;
        }
        wasDeleted = 0;
        fromSourced = true;
    }
};

void QDeclarativePropertyAnimation::transition(QDeclarativeStateActions &actions,
                                     QDeclarativeProperties &modified,
                                     TransitionDirection direction)
{
    Q_D(QDeclarativePropertyAnimation);

    QStringList props = d->properties.isEmpty() ? QStringList() : d->properties.split(QLatin1Char(','));
    for (int ii = 0; ii < props.count(); ++ii)
        props[ii] = props.at(ii).trimmed();
    if (!d->propertyName.isEmpty())
        props << d->propertyName;

    QList<QObject*> targets = d->targets;
    if (d->target)
        targets.append(d->target);

    bool hasSelectors = !props.isEmpty() || !targets.isEmpty() || !d->exclude.isEmpty();
    bool useType = (props.isEmpty() && d->defaultToInterpolatorType) ? true : false;

    if (d->defaultProperty.isValid() && !hasSelectors) {
        props << d->defaultProperty.name();
        targets << d->defaultProperty.object();
    }

    if (props.isEmpty() && !d->defaultProperties.isEmpty()) {
        props << d->defaultProperties.split(QLatin1Char(','));
    }

    PropertyUpdater *data = new PropertyUpdater;
    data->interpolatorType = d->interpolatorType;
    data->interpolator = d->interpolator;
    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = d->fromIsDefined;

    bool hasExplicit = false;
    //an explicit animation has been specified
    if (d->toIsDefined) {
        for (int i = 0; i < props.count(); ++i) {
            for (int j = 0; j < targets.count(); ++j) {
                QDeclarativeAction myAction;
                myAction.property = d->createProperty(targets.at(j), props.at(i), this);
                if (myAction.property.isValid()) {
                    if (d->fromIsDefined) {
                        myAction.fromValue = d->from;
                        d->convertVariant(myAction.fromValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
                    }
                    myAction.toValue = d->to;
                    d->convertVariant(myAction.toValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
                    data->actions << myAction;
                    hasExplicit = true;
                    for (int ii = 0; ii < actions.count(); ++ii) {
                        QDeclarativeAction &action = actions[ii];
                        if (action.property.object() == myAction.property.object() &&
                            myAction.property.name() == action.property.name()) {
                            modified << action.property;
                            break;  //### any chance there could be multiples?
                        }
                    }
                }
            }
        }
    }

    if (!hasExplicit)
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];

        QObject *obj = action.property.object();
        QString propertyName = action.property.name();
        QObject *sObj = action.specifiedObject;
        QString sPropertyName = action.specifiedProperty;
        bool same = (obj == sObj);

        if ((targets.isEmpty() || targets.contains(obj) || (!same && targets.contains(sObj))) &&
           (!d->exclude.contains(obj)) && (same || (!d->exclude.contains(sObj))) &&
           (props.contains(propertyName) || (!same && props.contains(sPropertyName))
               || (useType && action.property.propertyType() == d->interpolatorType))) {
            QDeclarativeAction myAction = action;

            if (d->fromIsDefined)
                myAction.fromValue = d->from;
            else
                myAction.fromValue = QVariant();
            if (d->toIsDefined)
                myAction.toValue = d->to;

            d->convertVariant(myAction.fromValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
            d->convertVariant(myAction.toValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());

            modified << action.property;

            data->actions << myAction;
            action.fromValue = myAction.toValue;
        }
    }

    if (data->actions.count()) {
        if (!d->rangeIsSet) {
            d->va->setStartValue(qreal(0));
            d->va->setEndValue(qreal(1));
            d->rangeIsSet = true;
        }
        d->va->setAnimValue(data, QAbstractAnimation::DeleteWhenStopped);
        d->va->setFromSourcedValue(&data->fromSourced);
        d->actions = &data->actions;
    } else {
        delete data;
        d->actions = 0;
    }
}

/*!
    \qmlclass ParentAnimation QDeclarativeParentAnimation
    \since 4.7
    \inherits Animation
    \brief The ParentAnimation element allows you to animate parent changes.

    ParentAnimation is used in conjunction with NumberAnimation to smoothly
    animate changing an item's parent. In the following example,
    ParentAnimation wraps a NumberAnimation which animates from the
    current position in the old parent to the new position in the new
    parent.

    \qml
    ...
    State {
        //reparent myItem to newParent. myItem's final location
        //should be 10,10 in newParent.
        ParentChange {
            target: myItem
            parent: newParent
            x: 10; y: 10
        }
    }
    ...
    Transition {
        //smoothly reparent myItem and move into new position
        ParentAnimation {
            target: theItem
            NumberAnimation { properties: "x,y" }
        }
    }
    \endqml

    ParentAnimation can wrap any number of animations -- those animations will
    be run in parallel (like those in a ParallelAnimation group).

    In some cases, such as reparenting between items with clipping, it's useful
    to animate the parent change via another item with no clipping.

    When used in a transition, ParentAnimation will by default animate
    all ParentChanges.
*/

/*!
    \internal
    \class QDeclarativeParentAnimation
*/
QDeclarativeParentAnimation::QDeclarativeParentAnimation(QObject *parent)
    : QDeclarativeAnimationGroup(*(new QDeclarativeParentAnimationPrivate), parent)
{
    Q_D(QDeclarativeParentAnimation);
    d->topLevelGroup = new QSequentialAnimationGroup;
    QDeclarative_setParent_noEvent(d->topLevelGroup, this);

    d->startAction = new QActionAnimation;
    QDeclarative_setParent_noEvent(d->startAction, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->startAction);

    d->ag = new QParallelAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->ag);

    d->endAction = new QActionAnimation;
    QDeclarative_setParent_noEvent(d->endAction, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->endAction);
}

QDeclarativeParentAnimation::~QDeclarativeParentAnimation()
{
}

/*!
    \qmlproperty item ParentAnimation::target
    The item to reparent.

    When used in a transition, if no target is specified all
    ParentChanges will be animated by the ParentAnimation.
*/
QDeclarativeItem *QDeclarativeParentAnimation::target() const
{
    Q_D(const QDeclarativeParentAnimation);
    return d->target;
}

void QDeclarativeParentAnimation::setTarget(QDeclarativeItem *target)
{
    Q_D(QDeclarativeParentAnimation);
    if (target == d->target)
        return;

    d->target = target;
    emit targetChanged();
}

/*!
    \qmlproperty item ParentAnimation::newParent
    The new parent to animate to.

    If not set, then the parent defined in the end state of the transition.
*/
QDeclarativeItem *QDeclarativeParentAnimation::newParent() const
{
    Q_D(const QDeclarativeParentAnimation);
    return d->newParent;
}

void QDeclarativeParentAnimation::setNewParent(QDeclarativeItem *newParent)
{
    Q_D(QDeclarativeParentAnimation);
    if (newParent == d->newParent)
        return;

    d->newParent = newParent;
    emit newParentChanged();
}

/*!
    \qmlproperty item ParentAnimation::via
    The item to reparent via. This provides a way to do an unclipped animation
    when both the old parent and new parent are clipped

    \qml
    ParentAnimation {
        target: myItem
        via: topLevelItem
        ...
    }
    \endqml
*/
QDeclarativeItem *QDeclarativeParentAnimation::via() const
{
    Q_D(const QDeclarativeParentAnimation);
    return d->via;
}

void QDeclarativeParentAnimation::setVia(QDeclarativeItem *via)
{
    Q_D(QDeclarativeParentAnimation);
    if (via == d->via)
        return;

    d->via = via;
    emit viaChanged();
}

//### mirrors same-named function in QDeclarativeItem
QPointF QDeclarativeParentAnimationPrivate::computeTransformOrigin(QDeclarativeItem::TransformOrigin origin, qreal width, qreal height) const
{
    switch(origin) {
    default:
    case QDeclarativeItem::TopLeft:
        return QPointF(0, 0);
    case QDeclarativeItem::Top:
        return QPointF(width / 2., 0);
    case QDeclarativeItem::TopRight:
        return QPointF(width, 0);
    case QDeclarativeItem::Left:
        return QPointF(0, height / 2.);
    case QDeclarativeItem::Center:
        return QPointF(width / 2., height / 2.);
    case QDeclarativeItem::Right:
        return QPointF(width, height / 2.);
    case QDeclarativeItem::BottomLeft:
        return QPointF(0, height);
    case QDeclarativeItem::Bottom:
        return QPointF(width / 2., height);
    case QDeclarativeItem::BottomRight:
        return QPointF(width, height);
    }
}

void QDeclarativeParentAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_D(QDeclarativeParentAnimation);

    struct QDeclarativeParentAnimationData : public QAbstractAnimationAction
    {
        QDeclarativeParentAnimationData() {}
        ~QDeclarativeParentAnimationData() { qDeleteAll(pc); }

        QDeclarativeStateActions actions;
        //### reverse should probably apply on a per-action basis
        bool reverse;
        QList<QDeclarativeParentChange *> pc;
        virtual void doAction()
        {
            for (int ii = 0; ii < actions.count(); ++ii) {
                const QDeclarativeAction &action = actions.at(ii);
                if (reverse)
                    action.event->reverse();
                else
                    action.event->execute();
            }
        }
    };

    QDeclarativeParentAnimationData *data = new QDeclarativeParentAnimationData;
    QDeclarativeParentAnimationData *viaData = new QDeclarativeParentAnimationData;

    bool hasExplicit = false;
    if (d->target && d->newParent) {
        data->reverse = false;
        QDeclarativeAction myAction;
        QDeclarativeParentChange *pc = new QDeclarativeParentChange;
        pc->setObject(d->target);
        pc->setParent(d->newParent);
        myAction.event = pc;
        data->pc << pc;
        data->actions << myAction;
        hasExplicit = true;
        if (d->via) {
            viaData->reverse = false;
            QDeclarativeAction myVAction;
            QDeclarativeParentChange *vpc = new QDeclarativeParentChange;
            vpc->setObject(d->target);
            vpc->setParent(d->via);
            myVAction.event = vpc;
            viaData->pc << vpc;
            viaData->actions << myVAction;
        }
        //### once actions have concept of modified,
        //    loop to match appropriate ParentChanges and mark as modified
    }

    if (!hasExplicit)
    for (int i = 0; i < actions.size(); ++i) {
        QDeclarativeAction &action = actions[i];
        if (action.event && action.event->typeName() == QLatin1String("ParentChange")
            && (!d->target || static_cast<QDeclarativeParentChange*>(action.event)->object() == d->target)) {

            QDeclarativeParentChange *pc = static_cast<QDeclarativeParentChange*>(action.event);
            QDeclarativeAction myAction = action;
            data->reverse = action.reverseEvent;

            //### this logic differs from PropertyAnimation
            //    (probably a result of modified vs. done)
            if (d->newParent) {
                QDeclarativeParentChange *epc = new QDeclarativeParentChange;
                epc->setObject(static_cast<QDeclarativeParentChange*>(action.event)->object());
                epc->setParent(d->newParent);
                myAction.event = epc;
                data->pc << epc;
                data->actions << myAction;
                pc = epc;
            } else {
                action.actionDone = true;
                data->actions << myAction;
            }

            if (d->via) {
                viaData->reverse = false;
                QDeclarativeAction myAction;
                QDeclarativeParentChange *vpc = new QDeclarativeParentChange;
                vpc->setObject(pc->object());
                vpc->setParent(d->via);
                myAction.event = vpc;
                viaData->pc << vpc;
                viaData->actions << myAction;
                QDeclarativeAction dummyAction;
                QDeclarativeAction &xAction = pc->xIsSet() ? actions[++i] : dummyAction;
                QDeclarativeAction &yAction = pc->yIsSet() ? actions[++i] : dummyAction;
                QDeclarativeAction &sAction = pc->scaleIsSet() ? actions[++i] : dummyAction;
                QDeclarativeAction &rAction = pc->rotationIsSet() ? actions[++i] : dummyAction;
                bool forward = (direction == QDeclarativeAbstractAnimation::Forward);
                QDeclarativeItem *target = pc->object();
                QDeclarativeItem *targetParent = forward ? pc->parent() : pc->originalParent();

                //### this mirrors the logic in QDeclarativeParentChange.
                bool ok;
                const QTransform &transform = targetParent->itemTransform(d->via, &ok);
                if (transform.type() >= QTransform::TxShear || !ok) {
                    qmlInfo(this) << QDeclarativeParentAnimation::tr("Unable to preserve appearance under complex transform");
                    ok = false;
                }

                qreal scale = 1;
                qreal rotation = 0;
                if (ok && transform.type() != QTransform::TxRotate) {
                    if (transform.m11() == transform.m22())
                        scale = transform.m11();
                    else {
                        qmlInfo(this) << QDeclarativeParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }
                } else if (ok && transform.type() == QTransform::TxRotate) {
                    if (transform.m11() == transform.m22())
                        scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
                    else {
                        qmlInfo(this) << QDeclarativeParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }

                    if (scale != 0)
                        rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
                    else {
                        qmlInfo(this) << QDeclarativeParentAnimation::tr("Unable to preserve appearance under scale of 0");
                        ok = false;
                    }
                }

                const QPointF &point = transform.map(QPointF(xAction.toValue.toReal(),yAction.toValue.toReal()));
                qreal x = point.x();
                qreal y = point.y();
                if (ok && target->transformOrigin() != QDeclarativeItem::TopLeft) {
                    qreal w = target->width();
                    qreal h = target->height();
                    if (pc->widthIsSet())
                        w = actions[++i].toValue.toReal();
                    if (pc->heightIsSet())
                        h = actions[++i].toValue.toReal();
                    const QPointF &transformOrigin
                            = d->computeTransformOrigin(target->transformOrigin(), w,h);
                    qreal tempxt = transformOrigin.x();
                    qreal tempyt = transformOrigin.y();
                    QTransform t;
                    t.translate(-tempxt, -tempyt);
                    t.rotate(rotation);
                    t.scale(scale, scale);
                    t.translate(tempxt, tempyt);
                    const QPointF &offset = t.map(QPointF(0,0));
                    x += offset.x();
                    y += offset.y();
                }

                if (ok) {
                    //qDebug() << x << y << rotation << scale;
                    xAction.toValue = x;
                    yAction.toValue = y;
                    sAction.toValue = sAction.toValue.toReal() * scale;
                    rAction.toValue = rAction.toValue.toReal() + rotation;
                }
            }
        }
    }

    if (data->actions.count()) {
        if (direction == QDeclarativeAbstractAnimation::Forward) {
            d->startAction->setAnimAction(d->via ? viaData : data, QActionAnimation::DeleteWhenStopped);
            d->endAction->setAnimAction(d->via ? data : 0, QActionAnimation::DeleteWhenStopped);
        } else {
            d->endAction->setAnimAction(d->via ? viaData : data, QActionAnimation::DeleteWhenStopped);
            d->startAction->setAnimAction(d->via ? data : 0, QActionAnimation::DeleteWhenStopped);
        }
    } else {
        delete data;
        delete viaData;
    }

    //take care of any child animations
    bool valid = d->defaultProperty.isValid();
    for (int ii = 0; ii < d->animations.count(); ++ii) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }

}

QAbstractAnimation *QDeclarativeParentAnimation::qtAnimation()
{
    Q_D(QDeclarativeParentAnimation);
    return d->topLevelGroup;
}

/*!
    \qmlclass AnchorAnimation QDeclarativeAnchorAnimation
    \since 4.7
    \inherits Animation
    \brief The AnchorAnimation element allows you to animate anchor changes.

    AnchorAnimation will animated any changes specified by a state's AnchorChanges.
    In the following snippet we animate the addition of a right anchor to our item.
    \qml
    Item {
        id: myItem
        width: 100
    }
    ...
    State {
        AnchorChanges {
            target: myItem
            anchors.right: container.right
        }
    }
    ...
    Transition {
        //smoothly reanchor myItem and move into new position
        AnchorAnimation {}
    }
    \endqml

    \sa AnchorChanges
*/

QDeclarativeAnchorAnimation::QDeclarativeAnchorAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QDeclarativeAnchorAnimationPrivate), parent)
{
    Q_D(QDeclarativeAnchorAnimation);
    d->va = new QDeclarativeBulkValueAnimator;
    QDeclarative_setParent_noEvent(d->va, this);
}

QDeclarativeAnchorAnimation::~QDeclarativeAnchorAnimation()
{
}

QAbstractAnimation *QDeclarativeAnchorAnimation::qtAnimation()
{
    Q_D(QDeclarativeAnchorAnimation);
    return d->va;
}

/*!
    \qmlproperty list<Item> AnchorAnimation::targets
    The items to reanchor.

    If no targets are specified all AnchorChanges will be
    animated by the AnchorAnimation.
*/
QDeclarativeListProperty<QDeclarativeItem> QDeclarativeAnchorAnimation::targets()
{
    Q_D(QDeclarativeAnchorAnimation);
    return QDeclarativeListProperty<QDeclarativeItem>(this, d->targets);
}

/*!
    \qmlproperty int AnchorAnimation::duration
    This property holds the duration of the animation, in milliseconds.

    The default value is 250.
*/
int QDeclarativeAnchorAnimation::duration() const
{
    Q_D(const QDeclarativeAnchorAnimation);
    return d->va->duration();
}

void QDeclarativeAnchorAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarativeAnchorAnimation);
    if (d->va->duration() == duration)
        return;
    d->va->setDuration(duration);
    emit durationChanged(duration);
}

/*!
    \qmlproperty enumeration AnchorAnimation::easing.type
    \qmlproperty real AnchorAnimation::easing.amplitude
    \qmlproperty real AnchorAnimation::easing.overshoot
    \qmlproperty real AnchorAnimation::easing.period
    \brief the easing curve used for the animation.

    To specify an easing curve you need to specify at least the type. For some curves you can also specify
    amplitude, period and/or overshoot. The default easing curve is
    Linear.

    \qml
    AnchorAnimation { easing.type: "InOutQuad" }
    \endqml

    See the \l{PropertyAnimation::easing.type} documentation for information
    about the different types of easing curves.
*/

QEasingCurve QDeclarativeAnchorAnimation::easing() const
{
    Q_D(const QDeclarativeAnchorAnimation);
    return d->va->easingCurve();
}

void QDeclarativeAnchorAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QDeclarativeAnchorAnimation);
    if (d->va->easingCurve() == e)
        return;

    d->va->setEasingCurve(e);
    emit easingChanged(e);
}

void QDeclarativeAnchorAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_UNUSED(modified);
    Q_D(QDeclarativeAnchorAnimation);
    PropertyUpdater *data = new PropertyUpdater;
    data->interpolatorType = QMetaType::QReal;
    data->interpolator = d->interpolator;

    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = false;

    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];
        if (action.event && action.event->typeName() == QLatin1String("AnchorChanges")
            && (d->targets.isEmpty() || d->targets.contains(static_cast<QDeclarativeAnchorChanges*>(action.event)->object()))) {
            data->actions << static_cast<QDeclarativeAnchorChanges*>(action.event)->additionalActions();
        }
    }

    if (data->actions.count()) {
        if (!d->rangeIsSet) {
            d->va->setStartValue(qreal(0));
            d->va->setEndValue(qreal(1));
            d->rangeIsSet = true;
        }
        d->va->setAnimValue(data, QAbstractAnimation::DeleteWhenStopped);
        d->va->setFromSourcedValue(&data->fromSourced);
    } else {
        delete data;
    }
}

QT_END_NAMESPACE
