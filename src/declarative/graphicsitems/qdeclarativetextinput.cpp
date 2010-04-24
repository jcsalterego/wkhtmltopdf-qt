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

#include "private/qdeclarativetextinput_p.h"
#include "private/qdeclarativetextinput_p_p.h"

#include <private/qdeclarativeglobal_p.h>
#include <qdeclarativeinfo.h>

#include <QValidator>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass TextInput QDeclarativeTextInput
  \since 4.7
    The TextInput item allows you to add an editable line of text to a scene.

    TextInput can only display a single line of text, and can only display
    plain text. However it can provide addition input constraints on the text.

    Input constraints include setting a QValidator, an input mask, or a
    maximum input length.
*/
QDeclarativeTextInput::QDeclarativeTextInput(QDeclarativeItem* parent)
    : QDeclarativePaintedItem(*(new QDeclarativeTextInputPrivate), parent)
{
    Q_D(QDeclarativeTextInput);
    d->init();
}

QDeclarativeTextInput::~QDeclarativeTextInput()
{
}

/*!
    \qmlproperty string TextInput::text

    The text in the TextInput.
*/

QString QDeclarativeTextInput::text() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->text();
}

void QDeclarativeTextInput::setText(const QString &s)
{
    Q_D(QDeclarativeTextInput);
    if(s == text())
        return;
    d->control->setText(s);
    //emit textChanged();
}

/*!
    \qmlproperty string TextInput::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool TextInput::font.bold

    Sets the font's weight to bold.
*/

/*!
    \qmlproperty enumeration TextInput::font.weight

    Sets the font's weight.

    The weight can be one of:
    \list
    \o Light
    \o Normal - the default
    \o DemiBold
    \o Bold
    \o Black
    \endlist

    \qml
    TextInput { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool TextInput::font.italic

    Sets the style of the text to italic.
*/

/*!
    \qmlproperty bool TextInput::font.underline

    Set the style of the text to underline.
*/

/*!
    \qmlproperty bool TextInput::font.outline

    Set the style of the text to outline.
*/

/*!
    \qmlproperty bool TextInput::font.strikeout

    Set the style of the text to strikeout.
*/

/*!
    \qmlproperty real TextInput::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int TextInput::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real TextInput::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A value of 100 will keep the spacing unchanged; a value of 200 will enlarge the spacing after a character by
    the width of the character itself.
*/

/*!
    \qmlproperty real TextInput::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration TextInput::font.capitalization

    Sets the capitalization for the text.

    \list
    \o MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \o AllUppercase - This alters the text to be rendered in all uppercase type.
    \o AllLowercase	 - This alters the text to be rendered in all lowercase type.
    \o SmallCaps -	This alters the text to be rendered in small-caps type.
    \o Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    TextInput { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

QFont QDeclarativeTextInput::font() const
{
    Q_D(const QDeclarativeTextInput);
    return d->font;
}

void QDeclarativeTextInput::setFont(const QFont &font)
{
    Q_D(QDeclarativeTextInput);
    if (d->font == font)
        return;

    d->font = font;

    d->control->setFont(d->font);
    if(d->cursorItem){
        d->cursorItem->setHeight(QFontMetrics(d->font).height());
        moveCursor();
    }
    updateSize();
    emit fontChanged(d->font);
}

/*!
    \qmlproperty color TextInput::color

    The text color.
*/
QColor QDeclarativeTextInput::color() const
{
    Q_D(const QDeclarativeTextInput);
    return d->color;
}

void QDeclarativeTextInput::setColor(const QColor &c)
{
    Q_D(QDeclarativeTextInput);
    d->color = c;
}


/*!
    \qmlproperty color TextInput::selectionColor

    The text highlight color, used behind selections.
*/
QColor QDeclarativeTextInput::selectionColor() const
{
    Q_D(const QDeclarativeTextInput);
    return d->selectionColor;
}

void QDeclarativeTextInput::setSelectionColor(const QColor &color)
{
    Q_D(QDeclarativeTextInput);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::Highlight, d->selectionColor);
    d->control->setPalette(p);
    emit selectionColorChanged(color);
}

/*!
    \qmlproperty color TextInput::selectedTextColor

    The highlighted text color, used in selections.
*/
QColor QDeclarativeTextInput::selectedTextColor() const
{
    Q_D(const QDeclarativeTextInput);
    return d->selectedTextColor;
}

void QDeclarativeTextInput::setSelectedTextColor(const QColor &color)
{
    Q_D(QDeclarativeTextInput);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::HighlightedText, d->selectedTextColor);
    d->control->setPalette(p);
    emit selectedTextColorChanged(color);
}

/*!
    \qmlproperty enumeration TextInput::horizontalAlignment

    Sets the horizontal alignment of the text within the TextInput item's
    width and height.  By default, the text is left aligned.

    TextInput does not have vertical alignment, as the natural height is
    exactly the height of the single line of text. If you set the height
    manually to something larger, TextInput will always be top aligned
    vertically. You can use anchors to align it however you want within
    another item.

    The valid values for \c horizontalAlignment are \c AlignLeft, \c AlignRight and
    \c AlignHCenter.
*/
QDeclarativeTextInput::HAlignment QDeclarativeTextInput::hAlign() const
{
    Q_D(const QDeclarativeTextInput);
    return d->hAlign;
}

void QDeclarativeTextInput::setHAlign(HAlignment align)
{
    Q_D(QDeclarativeTextInput);
    if(align == d->hAlign)
        return;
    d->hAlign = align;
    updateRect();
    emit horizontalAlignmentChanged(d->hAlign);
}

bool QDeclarativeTextInput::isReadOnly() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->isReadOnly();
}

void QDeclarativeTextInput::setReadOnly(bool ro)
{
    Q_D(QDeclarativeTextInput);
    if (d->control->isReadOnly() == ro)
        return;

    d->control->setReadOnly(ro);

    emit readOnlyChanged(ro);
}

int QDeclarativeTextInput::maxLength() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->maxLength();
}

void QDeclarativeTextInput::setMaxLength(int ml)
{
    Q_D(QDeclarativeTextInput);
    if (d->control->maxLength() == ml)
        return;

    d->control->setMaxLength(ml);

    emit maximumLengthChanged(ml);
}

/*!
    \qmlproperty bool TextInput::cursorVisible
    Set to true when the TextInput shows a cursor.

    This property is set and unset when the TextInput gets focus, so that other
    properties can be bound to whether the cursor is currently showing. As it
    gets set and unset automatically, when you set the value yourself you must
    keep in mind that your value may be overwritten.

    It can be set directly in script, for example if a KeyProxy might
    forward keys to it and you desire it to look active when this happens
    (but without actually giving it the focus).

    It should not be set directly on the element, like in the below QML,
    as the specified value will be overridden an lost on focus changes.

    \code
    TextInput {
        text: "Text"
        cursorVisible: false
    }
    \endcode

    In the above snippet the cursor will still become visible when the
    TextInput gains focus.
*/
bool QDeclarativeTextInput::isCursorVisible() const
{
    Q_D(const QDeclarativeTextInput);
    return d->cursorVisible;
}

void QDeclarativeTextInput::setCursorVisible(bool on)
{
    Q_D(QDeclarativeTextInput);
    if (d->cursorVisible == on)
        return;
    d->cursorVisible = on;
    d->control->setCursorBlinkPeriod(on?QApplication::cursorFlashTime():0);
    //d->control should emit the cursor update regions
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int TextInput::cursorPosition
    The position of the cursor in the TextInput.
*/
int QDeclarativeTextInput::cursorPosition() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->cursor();
}
void QDeclarativeTextInput::setCursorPosition(int cp)
{
    Q_D(QDeclarativeTextInput);
    d->control->moveCursor(cp);
}

/*!
  \internal

  Returns a Rect which encompasses the cursor, but which may be larger than is
  required. Ignores custom cursor delegates.
*/
QRect QDeclarativeTextInput::cursorRect() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->cursorRect();
}

/*!
    \qmlproperty int TextInput::selectionStart

    The cursor position before the first character in the current selection.
    Setting this and selectionEnd allows you to specify a selection in the
    text edit.

    Note that if selectionStart == selectionEnd then there is no current
    selection. If you attempt to set selectionStart to a value outside of
    the current text, selectionStart will not be changed.

    \sa selectionEnd, cursorPosition, selectedText
*/
int QDeclarativeTextInput::selectionStart() const
{
    Q_D(const QDeclarativeTextInput);
    return d->lastSelectionStart;
}

void QDeclarativeTextInput::setSelectionStart(int s)
{
    Q_D(QDeclarativeTextInput);
    if(d->lastSelectionStart == s || s < 0 || s > text().length())
        return;
    d->lastSelectionStart = s;
    d->control->setSelection(s, d->lastSelectionEnd - s);
}

/*!
    \qmlproperty int TextInput::selectionEnd

    The cursor position after the last character in the current selection.
    Setting this and selectionStart allows you to specify a selection in the
    text edit.

    Note that if selectionStart == selectionEnd then there is no current
    selection. If you attempt to set selectionEnd to a value outside of
    the current text, selectionEnd will not be changed.

    \sa selectionStart, cursorPosition, selectedText
*/
int QDeclarativeTextInput::selectionEnd() const
{
    Q_D(const QDeclarativeTextInput);
    return d->lastSelectionEnd;
}

void QDeclarativeTextInput::setSelectionEnd(int s)
{
    Q_D(QDeclarativeTextInput);
    if(d->lastSelectionEnd == s || s < 0 || s > text().length())
        return;
    d->lastSelectionEnd = s;
    d->control->setSelection(d->lastSelectionStart, s - d->lastSelectionStart);
}

/*!
    \qmlproperty string TextInput::selectedText

    This read-only property provides the text currently selected in the
    text input.

    It is equivalent to the following snippet, but is faster and easier
    to use.

    \qml
    myTextInput.text.toString().substring(myTextInput.selectionStart,
        myTextInput.selectionEnd);
    \endqml
*/
QString QDeclarativeTextInput::selectedText() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->selectedText();
}

/*!
    \qmlproperty bool TextInput::focusOnPress

    Whether the TextInput should gain focus on a mouse press. By default this is
    set to true.
*/
bool QDeclarativeTextInput::focusOnPress() const
{
    Q_D(const QDeclarativeTextInput);
    return d->focusOnPress;
}

void QDeclarativeTextInput::setFocusOnPress(bool b)
{
    Q_D(QDeclarativeTextInput);
    if (d->focusOnPress == b)
        return;

    d->focusOnPress = b;

    emit focusOnPressChanged(d->focusOnPress);
}

/*!
    \qmlproperty bool TextInput::autoScroll

    Whether the TextInput should scroll when the text is longer than the width. By default this is
    set to true.
*/
bool QDeclarativeTextInput::autoScroll() const
{
    Q_D(const QDeclarativeTextInput);
    return d->autoScroll;
}

void QDeclarativeTextInput::setAutoScroll(bool b)
{
    Q_D(QDeclarativeTextInput);
    if (d->autoScroll == b)
        return;

    d->autoScroll = b;

    emit autoScrollChanged(d->autoScroll);
}

/*!
    \qmlclass IntValidator QIntValidator

    This element provides a validator for integer values
*/
/*!
    \qmlproperty int IntValidator::top

    This property holds the validator's highest acceptable value.
    By default, this property's value is derived from the highest signed integer available (typically 2147483647).
*/
/*!
    \qmlproperty int IntValidator::bottom

    This property holds the validator's lowest acceptable value.
    By default, this property's value is derived from the lowest signed integer available (typically -2147483647).
*/

/*!
    \qmlclass DoubleValidator QDoubleValidator

    This element provides a validator for non-integer numbers.
*/

/*!
    \qmlproperty real DoubleValidator::top

    This property holds the validator's maximum acceptable value.
    By default, this property contains a value of infinity.
*/
/*!
    \qmlproperty real DoubleValidator::bottom

    This property holds the validator's minimum acceptable value.
    By default, this property contains a value of -infinity.
*/
/*!
    \qmlproperty int DoubleValidator::decimals

    This property holds the validator's maximum number of digits after the decimal point.
    By default, this property contains a value of 1000.
*/
/*!
    \qmlproperty enumeration DoubleValidator::notation
    This property holds the notation of how a string can describe a number.

    The values for this property are DoubleValidator.StandardNotation or DoubleValidator.ScientificNotation.
    If this property is set to ScientificNotation, the written number may have an exponent part(i.e. 1.5E-2).

    By default, this property is set to ScientificNotation.
*/

/*!
    \qmlclass RegExpValidator QRegExpValidator

    This element provides a validator, which counts as valid any string which
    matches a specified regular expression.
*/
/*!
   \qmlproperty regExp RegExpValidator::regExp

   This property holds the regular expression used for validation.

   Note that this property should be a regular expression in JS syntax, e.g /a/ for the regular expression
   matching "a".

   By default, this property contains a regular expression with the pattern .* that matches any string.
*/

/*!
    \qmlproperty Validator TextInput::validator

    Allows you to set a validator on the TextInput. When a validator is set
    the TextInput will only accept input which leaves the text property in
    an acceptable or intermediate state. The accepted signal will only be sent
    if the text is in an acceptable state when enter is pressed.

    Currently supported validators are IntValidator, DoubleValidator and
    RegExpValidator. An example of using validators is shown below, which allows
    input of integers between 11 and 31 into the text input:

    \code
    import Qt 4.7
    TextInput{
        validator: IntValidator{bottom: 11; top: 31;}
        focus: true
    }
    \endcode

    \sa acceptableInput, inputMask
*/
QValidator* QDeclarativeTextInput::validator() const
{
    Q_D(const QDeclarativeTextInput);
    //###const cast isn't good, but needed for property system?
    return const_cast<QValidator*>(d->control->validator());
}

void QDeclarativeTextInput::setValidator(QValidator* v)
{
    Q_D(QDeclarativeTextInput);
    if (d->control->validator() == v)
        return;

    d->control->setValidator(v);
    if(!d->control->hasAcceptableInput()){
        d->oldValidity = false;
        emit acceptableInputChanged();
    }

    emit validatorChanged();
}

/*!
    \qmlproperty string TextInput::inputMask

    Allows you to set an input mask on the TextInput, restricting the allowable
    text inputs. See QLineEdit::inputMask for further details, as the exact
    same mask strings are used by TextInput.

    \sa acceptableInput, validator
*/
QString QDeclarativeTextInput::inputMask() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->inputMask();
}

void QDeclarativeTextInput::setInputMask(const QString &im)
{
    Q_D(QDeclarativeTextInput);
    if (d->control->inputMask() == im)
        return;

    d->control->setInputMask(im);
    emit inputMaskChanged(d->control->inputMask());
}

/*!
    \qmlproperty bool TextInput::acceptableInput

    This property is always true unless a validator or input mask has been set.
    If a validator or input mask has been set, this property will only be true
    if the current text is acceptable to the validator or input mask as a final
    string (not as an intermediate string).
*/
bool QDeclarativeTextInput::hasAcceptableInput() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->hasAcceptableInput();
}

/*!
    \qmlproperty TextInput.EchoMode TextInput::echoMode

    Specifies how the text should be displayed in the TextInput.
    The default is Normal, which displays the text as it is. Other values
    are Password, which displays asterixes instead of characters, NoEcho,
    which displays nothing, and PasswordEchoOnEdit, which displays all but the
    current character as asterixes.

*/
QDeclarativeTextInput::EchoMode QDeclarativeTextInput::echoMode() const
{
    Q_D(const QDeclarativeTextInput);
    return (QDeclarativeTextInput::EchoMode)d->control->echoMode();
}

void QDeclarativeTextInput::setEchoMode(QDeclarativeTextInput::EchoMode echo)
{
    Q_D(QDeclarativeTextInput);
    if (echoMode() == echo)
        return;

    d->control->setEchoMode((uint)echo);
    emit echoModeChanged(echoMode());
}

/*!
    \qmlproperty Component TextInput::cursorDelegate
    The delegate for the cursor in the TextInput.

    If you set a cursorDelegate for a TextInput, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the TextInput when a cursor is
    needed, and the x property of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QDeclarativeItem or
    QDeclarativeItem derived item.
*/
QDeclarativeComponent* QDeclarativeTextInput::cursorDelegate() const
{
    Q_D(const QDeclarativeTextInput);
    return d->cursorComponent;
}

void QDeclarativeTextInput::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QDeclarativeTextInput);
    if (d->cursorComponent == c)
        return;

    d->cursorComponent = c;
    if(!c){
        //note that the components are owned by something else
        disconnect(d->control, SIGNAL(cursorPositionChanged(int, int)),
                this, SLOT(moveCursor()));
        delete d->cursorItem;
    }else{
        d->startCreatingCursor();
    }

    emit cursorDelegateChanged();
}

void QDeclarativeTextInputPrivate::startCreatingCursor()
{
    Q_Q(QDeclarativeTextInput);
    q->connect(control, SIGNAL(cursorPositionChanged(int, int)),
            q, SLOT(moveCursor()));
    if(cursorComponent->isReady()){
        q->createCursor();
    }else if(cursorComponent->isLoading()){
        q->connect(cursorComponent, SIGNAL(statusChanged(int)),
                q, SLOT(createCursor()));
    }else {//isError
        qmlInfo(q, cursorComponent->errors()) << QDeclarativeTextInput::tr("Could not load cursor delegate");
    }
}

void QDeclarativeTextInput::createCursor()
{
    Q_D(QDeclarativeTextInput);
    if(d->cursorComponent->isError()){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not load cursor delegate");
        return;
    }

    if(!d->cursorComponent->isReady())
        return;

    if(d->cursorItem)
        delete d->cursorItem;
    d->cursorItem = qobject_cast<QDeclarativeItem*>(d->cursorComponent->create());
    if(!d->cursorItem){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not instantiate cursor delegate");
        return;
    }

    QDeclarative_setParent_noEvent(d->cursorItem, this);
    d->cursorItem->setParentItem(this);
    d->cursorItem->setX(d->control->cursorToX());
    d->cursorItem->setHeight(d->control->height());
}

void QDeclarativeTextInput::moveCursor()
{
    Q_D(QDeclarativeTextInput);
    if(!d->cursorItem)
        return;
    d->cursorItem->setX(d->control->cursorToX() - d->hscroll);
}

/*
    \qmlmethod int xToPosition(int x)

    This function returns the character position at
    x pixels from the left of the textInput. Position 0 is before the
    first character, position 1 is after the first character but before the second,
    and so on until position text.length, which is after all characters.

    This means that for all x values before the first character this function returns 0,
    and for all x values after the last character this function returns text.length.
*/
int QDeclarativeTextInput::xToPosition(int x)
{
    Q_D(const QDeclarativeTextInput);
    return d->control->xToPos(x - d->hscroll);
}

void QDeclarativeTextInputPrivate::focusChanged(bool hasFocus)
{
    Q_Q(QDeclarativeTextInput);
    focused = hasFocus;
    q->setCursorVisible(hasFocus);
    if(q->echoMode() == QDeclarativeTextInput::PasswordEchoOnEdit && !hasFocus)
        control->updatePasswordEchoEditing(false);//QLineControl sets it on key events, but doesn't deal with focus events
    if (!hasFocus)
        control->deselect();
    QDeclarativeItemPrivate::focusChanged(hasFocus);
}

void QDeclarativeTextInput::keyPressEvent(QKeyEvent* ev)
{
    Q_D(QDeclarativeTextInput);
    if(((d->control->cursor() == 0 && ev->key() == Qt::Key_Left)
            || (d->control->cursor() == d->control->text().length()
                && ev->key() == Qt::Key_Right))
            && (d->lastSelectionStart == d->lastSelectionEnd)){
        //ignore when moving off the end
        //unless there is a selection, because then moving will do something (deselect)
        ev->ignore();
    }else{
        d->control->processKeyEvent(ev);
    }
    if (!ev->isAccepted())
        QDeclarativePaintedItem::keyPressEvent(ev);
}

void QDeclarativeTextInput::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativeTextInput);
    if(d->focusOnPress){
        QGraphicsItem *p = parentItem();//###Is there a better way to find my focus scope?
        while(p) {
            if(p->flags() & QGraphicsItem::ItemIsFocusScope){
                p->setFocus();
                break;
            }
            p = p->parentItem();
        }
        setFocus(true);
    }
    bool mark = event->modifiers() & Qt::ShiftModifier;
    int cursor = d->xToPos(event->pos().x());
    d->control->moveCursor(cursor, mark);
}

void QDeclarativeTextInput::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativeTextInput);
    d->control->moveCursor(d->xToPos(event->pos().x()), true);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarativeTextInput::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarativeTextInput);
    QWidget *widget = event->widget();
    if (widget && !d->control->isReadOnly() && boundingRect().contains(event->pos()))
        qt_widget_private(widget)->handleSoftwareInputPanel(event->button(), d->focusOnPress);
    d->control->processEvent(event);
}

bool QDeclarativeTextInput::event(QEvent* ev)
{
    Q_D(QDeclarativeTextInput);
    //Anything we don't deal with ourselves, pass to the control
    bool handled = false;
    switch(ev->type()){
        case QEvent::KeyPress:
        case QEvent::KeyRelease://###Should the control be doing anything with release?
        case QEvent::GraphicsSceneMousePress:
        case QEvent::GraphicsSceneMouseMove:
        case QEvent::GraphicsSceneMouseRelease:
            break;
        default:
            handled = d->control->processEvent(ev);
            if (ev->type() == QEvent::InputMethod)
                updateSize();
    }
    if(!handled)
        return QDeclarativePaintedItem::event(ev);
    return true;
}

void QDeclarativeTextInput::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    if (newGeometry.width() != oldGeometry.width())
        updateSize();
    QDeclarativePaintedItem::geometryChanged(newGeometry, oldGeometry);
}

void QDeclarativeTextInput::drawContents(QPainter *p, const QRect &r)
{
    Q_D(QDeclarativeTextInput);
    p->setRenderHint(QPainter::TextAntialiasing, true);
    p->save();
    p->setPen(QPen(d->color));
    int flags = QLineControl::DrawText;
    if(!isReadOnly() && d->cursorVisible && !d->cursorItem)
        flags |= QLineControl::DrawCursor;
    if (d->control->hasSelectedText())
            flags |= QLineControl::DrawSelections;
    QPoint offset = QPoint(0,0);
    QFontMetrics fm = QFontMetrics(d->font);
    int cix = qRound(d->control->cursorToX());
    QRect br(boundingRect().toRect());
    //###Is this using bearing appropriately?
    int minLB = qMax(0, -fm.minLeftBearing());
    int minRB = qMax(0, -fm.minRightBearing());
    int widthUsed = qRound(d->control->naturalTextWidth()) + 1 + minRB;
    if (d->autoScroll) {
        if ((minLB + widthUsed) <=  br.width()) {
            // text fits in br; use hscroll for alignment
            switch (d->hAlign & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
            case Qt::AlignRight:
                d->hscroll = widthUsed - br.width() + 1;
                break;
            case Qt::AlignHCenter:
                d->hscroll = (widthUsed - br.width()) / 2;
                break;
            default:
                // Left
                d->hscroll = 0;
                break;
            }
            d->hscroll -= minLB;
        } else if (cix - d->hscroll >= br.width()) {
            // text doesn't fit, cursor is to the right of br (scroll right)
            d->hscroll = cix - br.width() + 1;
        } else if (cix - d->hscroll < 0 && d->hscroll < widthUsed) {
            // text doesn't fit, cursor is to the left of br (scroll left)
            d->hscroll = cix;
        } else if (widthUsed - d->hscroll < br.width()) {
            // text doesn't fit, text document is to the left of br; align
            // right
            d->hscroll = widthUsed - br.width() + 1;
        }
        // the y offset is there to keep the baseline constant in case we have script changes in the text.
        offset = br.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());
    } else {
        if(d->hAlign == AlignRight){
            d->hscroll = width() - widthUsed;
        }else if(d->hAlign == AlignHCenter){
            d->hscroll = (width() - widthUsed) / 2;
        }
        d->hscroll -= minLB;
        offset = QPoint(d->hscroll, 0);
    }

    d->control->draw(p, offset, r, flags);

    p->restore();
}

/*!
\overload
Returns the value of the given \a property.
*/
QVariant QDeclarativeTextInput::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QDeclarativeTextInput);
    switch(property) {
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        return QVariant(d->control->cursor());
    case Qt::ImSurroundingText:
        return QVariant(text());
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->control->selectionStart() == d->control->selectionEnd())
            return QVariant(d->control->cursor());
        else if (d->control->selectionStart() == d->control->cursor())
            return QVariant(d->control->selectionEnd());
        else
            return QVariant(d->control->selectionStart());
    default:
        return QVariant();
    }
}

void QDeclarativeTextInput::selectAll()
{
    Q_D(QDeclarativeTextInput);
    d->control->setSelection(0, d->control->text().length());
}


/*!
    \qmlproperty bool TextInput::smooth

    Set this property if you want the text to be smoothly scaled or
    transformed.  Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*
   \qmlproperty string TextInput::passwordCharacter

   This is the character displayed when echoMode is set to Password or
   PasswordEchoOnEdit. By default it is an asterisk.

   Attempting to set this to more than one character will set it to
   the first character in the string. Attempting to set this to less
   than one character will fail.
*/
QString QDeclarativeTextInput::passwordCharacter() const
{
    Q_D(const QDeclarativeTextInput);
    return QString(d->control->passwordCharacter());
}

void QDeclarativeTextInput::setPasswordCharacter(const QString &str)
{
    Q_D(QDeclarativeTextInput);
    if(str.length() < 1)
        return;
    emit passwordCharacterChanged();
    d->control->setPasswordCharacter(str.constData()[0]);
}

/*
   \qmlproperty string TextInput::displayText

   This is the actual text displayed in the TextInput. When
   echoMode is set to TextInput::Normal this will be exactly
   the same as the TextInput::text property. When echoMode
   is set to something else, this property will contain the text
   the user sees, while the text property will contain the
   entered text.
*/
QString QDeclarativeTextInput::displayText() const
{
    Q_D(const QDeclarativeTextInput);
    return d->control->displayText();
}

/*
    \qmlmethod void moveCursorSelection(int pos)

    This method allows you to move the cursor while modifying the selection accordingly.
    To simply move the cursor, set the cursorPosition property.

    When this method is called it additionally sets either the
    selectionStart or the selectionEnd (whichever was at the previous cursor position)
    to the specified position. This allows you to easily extend and contract the selected
    text range.

    Example: The sequence of calls
        cursorPosition = 5
        moveCursorSelection(9)
        moveCursorSelection(7)
    would move the cursor to position 5, extend the selection end from 5 to 9
    and then retract the selection end from 9 to 7, leaving the text from position 5 to 7
    selected (the 6th and 7th characters).
*/
void QDeclarativeTextInput::moveCursorSelection(int pos)
{
    Q_D(QDeclarativeTextInput);
    d->control->moveCursor(pos, true);
}

void QDeclarativeTextInputPrivate::init()
{
    Q_Q(QDeclarativeTextInput);
    control->setCursorWidth(1);
    control->setPasswordCharacter(QLatin1Char('*'));
    control->setLayoutDirection(Qt::LeftToRight);
    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
    q->setFlag(QGraphicsItem::ItemAcceptsInputMethod);
    q->connect(control, SIGNAL(cursorPositionChanged(int,int)),
               q, SLOT(cursorPosChanged()));
    q->connect(control, SIGNAL(selectionChanged()),
               q, SLOT(selectionChanged()));
    q->connect(control, SIGNAL(textChanged(const QString &)),
               q, SIGNAL(displayTextChanged(const QString &)));
    q->connect(control, SIGNAL(textChanged(const QString &)),
               q, SLOT(q_textChanged()));
    q->connect(control, SIGNAL(accepted()),
               q, SIGNAL(accepted()));
    q->connect(control, SIGNAL(updateNeeded(QRect)),
               q, SLOT(updateRect(QRect)));
    q->connect(control, SIGNAL(cursorPositionChanged(int,int)),
               q, SLOT(updateRect()));//TODO: Only update rect between pos's
    q->connect(control, SIGNAL(selectionChanged()),
               q, SLOT(updateRect()));//TODO: Only update rect in selection
    //Note that above TODOs probably aren't that big a savings
    q->updateSize();
    oldValidity = control->hasAcceptableInput();
    lastSelectionStart = 0;
    lastSelectionEnd = 0;
    QPalette p = control->palette();
    selectedTextColor = p.color(QPalette::HighlightedText);
    selectionColor = p.color(QPalette::Highlight);
}

void QDeclarativeTextInput::cursorPosChanged()
{
    Q_D(QDeclarativeTextInput);
    emit cursorPositionChanged();

    if(!d->control->hasSelectedText()){
        if(d->lastSelectionStart != d->control->cursor()){
            d->lastSelectionStart = d->control->cursor();
            emit selectionStartChanged();
        }
        if(d->lastSelectionEnd != d->control->cursor()){
            d->lastSelectionEnd = d->control->cursor();
            emit selectionEndChanged();
        }
    }
}

void QDeclarativeTextInput::selectionChanged()
{
    Q_D(QDeclarativeTextInput);
    emit selectedTextChanged();

    if(d->lastSelectionStart != d->control->selectionStart()){
        d->lastSelectionStart = d->control->selectionStart();
        if(d->lastSelectionStart == -1)
            d->lastSelectionStart = d->control->cursor();
        emit selectionStartChanged();
    }
    if(d->lastSelectionEnd != d->control->selectionEnd()){
        d->lastSelectionEnd = d->control->selectionEnd();
        if(d->lastSelectionEnd == -1)
            d->lastSelectionEnd = d->control->cursor();
        emit selectionEndChanged();
    }
}

void QDeclarativeTextInput::q_textChanged()
{
    Q_D(QDeclarativeTextInput);
    updateSize();
    emit textChanged();
    if(hasAcceptableInput() != d->oldValidity){
        d->oldValidity = hasAcceptableInput();
        emit acceptableInputChanged();
    }
}

void QDeclarativeTextInput::updateRect(const QRect &r)
{
    Q_D(QDeclarativeTextInput);
    if(r == QRect())
        clearCache();
    else
        dirtyCache(QRect(r.x() - d->hscroll, r.y(), r.width(), r.height()));
    update();
}

void QDeclarativeTextInput::updateSize(bool needsRedraw)
{
    Q_D(QDeclarativeTextInput);
    int w = width();
    int h = height();
    setImplicitHeight(d->control->height());
    int cursorWidth = d->control->cursorWidth();
    if(d->cursorItem)
        cursorWidth = d->cursorItem->width();
    //### Is QFontMetrics too slow?
    QFontMetricsF fm(d->font);
    setImplicitWidth(fm.width(d->control->displayText())+cursorWidth);
    setContentsSize(QSize(width(), height()));//Repaints if changed
    if(w==width() && h==height() && needsRedraw){
        clearCache();
        update();
    }
}

QT_END_NAMESPACE

