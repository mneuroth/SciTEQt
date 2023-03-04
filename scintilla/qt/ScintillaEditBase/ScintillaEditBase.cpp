//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaEditBase.cpp - Qt widget that wraps ScintillaQt and provides events and scrolling
//
// Additions Copyright (c) 2020 Michael Neuroth
// Scintilla platform layer for Qt QML/Quick

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"
#include "PlatQt.h"

#include "Position.h"

#include <QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QInputContext>
#endif
#include <QPainter>
#include <QVarLengthArray>
#include <QScrollBar>
#include <QTextFormat>
#ifdef PLAT_QT_QML
#include <QtGlobal>
#include <QPoint>
#include <QPair>
#include <QList>
#endif

constexpr int IndicatorInput = static_cast<int>(Scintilla::IndicatorNumbers::Ime);
constexpr int IndicatorTarget = IndicatorInput + 1;
constexpr int IndicatorConverted = IndicatorInput + 2;
constexpr int IndicatorUnknown = IndicatorInput + 3;

// Q_WS_MAC and Q_WS_X11 aren't defined in Qt5
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#ifdef Q_OS_MAC
#define Q_WS_MAC 1
#endif

#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
#define Q_WS_X11 1
#endif
#endif // QT_VERSION >= 5.0.0

using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaEditBase::ScintillaEditBase(QQuickItem/*QWidget*/ *parent)
#ifdef PLAT_QT_QML
: QQuickPaintedItem(parent)
#else
: QAbstractScrollArea(parent)
#endif
, enableUpdateFlag(true), logicalWidth(0), logicalHeight(0)
#ifdef PLAT_QT_QML
, dataInputMethodHints(Qt::ImhNone)
//, aLongTouchTimer(this)
, aLastTouchPressTime(-1)
#endif
, sqt(new ScintillaQt(this)), preeditPos(-1), wheelDelta(0)
{
#ifdef PLAT_QT_QML
    setAcceptedMouseButtons(Qt::AllButtons);
    //setAcceptHoverEvents(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    setAcceptTouchEvents(true);
#endif
    //setFiltersChildMouseEvents(false);
    //grabMouse();
    setFlags(QQuickItem::ItemAcceptsInputMethod | QQuickItem::ItemIsFocusScope | QQuickItem::ItemHasContents | QQuickItem::ItemAcceptsDrops | QQuickItem::ItemClipsChildrenToShape);
#endif

	time.start();

#ifndef PLAT_QT_QML
	// Set Qt defaults.
	setAcceptDrops(true);
	setMouseTracking(true);
	setAutoFillBackground(false);
	setFrameStyle(QFrame::NoFrame);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_StaticContents);
	viewport()->setAutoFillBackground(false);
	setAttribute(Qt::WA_KeyCompression);
	setAttribute(Qt::WA_InputMethodEnabled);
#endif

	// All IME indicators drawn in same colour, blue, with different patterns
	const ColourRGBA colourIME(0, 0, UCHAR_MAX);
	sqt->vs.indicators[IndicatorUnknown] = Indicator(IndicatorStyle::Hidden, colourIME);
	sqt->vs.indicators[IndicatorInput] = Indicator(IndicatorStyle::Dots, colourIME);
	sqt->vs.indicators[IndicatorConverted] = Indicator(IndicatorStyle::CompositionThick, colourIME);
	sqt->vs.indicators[IndicatorTarget] = Indicator(IndicatorStyle::StraightBox, colourIME);

	connect(sqt, SIGNAL(notifyParent(Scintilla::NotificationData)),
		this, SLOT(notifyParent(Scintilla::NotificationData)));

	// Connect scroll bars.
#ifndef PLAT_QT_QML
	// scrollbars are handled by the QML ScrollView outside this class
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
	        this, SLOT(scrollVertical(int)));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
	        this, SLOT(scrollHorizontal(int)));
#endif

	// Connect pass-through signals.
	connect(sqt, SIGNAL(horizontalRangeChanged(int,int)),
	        this, SIGNAL(horizontalRangeChanged(int,int)));
	connect(sqt, SIGNAL(verticalRangeChanged(int,int)),
	        this, SIGNAL(verticalRangeChanged(int,int)));
	connect(sqt, SIGNAL(horizontalScrolled(int)),
	        this, SIGNAL(horizontalScrolled(int)));
	connect(sqt, SIGNAL(verticalScrolled(int)),
	        this, SIGNAL(verticalScrolled(int)));

	connect(sqt, SIGNAL(notifyChange()),
	        this, SIGNAL(notifyChange()));

	connect(sqt, SIGNAL(command(Scintilla::uptr_t,Scintilla::sptr_t)),
		this, SLOT(event_command(Scintilla::uptr_t,Scintilla::sptr_t)));

	connect(sqt, SIGNAL(aboutToCopy(QMimeData*)),
		this, SIGNAL(aboutToCopy(QMimeData*)));

#ifdef PLAT_QT_QML
    connect(sqt, SIGNAL(cursorPositionChanged()), this, SIGNAL(cursorPositionChanged()));   // needed to update markers on android platform

    //connect(&aLongTouchTimer, SIGNAL(timeout()), this, SLOT(onLongTouch()));
#endif

	// TODO: performance optimizations... ?
	//setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

ScintillaEditBase::~ScintillaEditBase() = default;

sptr_t ScintillaEditBase::send(
	unsigned int iMessage,
	uptr_t wParam,
	sptr_t lParam) const
{
	return sqt->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

sptr_t ScintillaEditBase::sends(
    unsigned int iMessage,
    uptr_t wParam,
    const char *s) const
{
	return sqt->WndProc(static_cast<Message>(iMessage), wParam, reinterpret_cast<sptr_t>(s));
}

#ifdef PLAT_QT_QML

void ScintillaEditBase::scrollRow(int deltaLines)
{
    int currentLine = sqt->TopLineOfMain();
    sqt->SetTopLine(currentLine + deltaLines);
    //scrollVertical(currentLine + deltaLines);       // also ok !
}

void ScintillaEditBase::scrollColumn(int deltaColumns)
{
    int currentColumnInPixel = send(SCI_GETXOFFSET);
    int newValue = currentColumnInPixel + deltaColumns * getCharWidth();
    if(newValue < 0)
    {
        newValue = 0;
    }
//    send(SCI_SETXOFFSET,newValue);
    scrollHorizontal(newValue);
}

void ScintillaEditBase::debug()
{
    // for debugging...
}

void ScintillaEditBase::cmdContextMenu(int menuID)
{
    sqt->Command(menuID);
}

void ScintillaEditBase::enableUpdate(bool enable)
{
    enableUpdateFlag = enable;
    if( enableUpdateFlag )
    {
        update();
    }
}

/*
void ScintillaEditBase::onLongTouch()
{
#ifndef Q_OS_ANDROID
    //emit showContextMenu(longTouchPoint);
#else
//    if(!sqt->pdoc->IsReadOnly())
//    {
//        // select the word under cursor and show markers and context menu (android)

//        sqt->selectCurrentWord();
//#ifdef PLAT_QT_QML
//        cursorChangedUpdateMarker();
//#endif
//    }
#endif
}
*/
#endif

void ScintillaEditBase::scrollHorizontal(int value)
{
	sqt->HorizontalScrollTo(value);
}

void ScintillaEditBase::scrollVertical(int value)
{
	sqt->ScrollTo(value);
}

bool ScintillaEditBase::event(QEvent *event)
{
	bool result = false;

	if (event->type() == QEvent::KeyPress) {
		// Circumvent the tab focus convention.
		keyPressEvent(static_cast<QKeyEvent *>(event));
		result = event->isAccepted();
	} else if (event->type() == QEvent::Show) {
#ifndef PLAT_QT_QML
		setMouseTracking(true);
		result = QAbstractScrollArea::event(event);
#else
		//grabMouse();
		result = QQuickPaintedItem::event(event);
#endif
	} else if (event->type() == QEvent::Hide) {
#ifndef PLAT_QT_QML
		setMouseTracking(false);
		result = QAbstractScrollArea::event(event);
#else
		//ungrabMouse();
		result = QQuickPaintedItem::event(event);
#endif
	} else {
#ifndef PLAT_QT_QML
		result = QAbstractScrollArea::event(event);
#else
		result = QQuickPaintedItem::event(event);
#endif
	}

	return result;
}

#ifdef PLAT_QT_QML

void ScintillaEditBase::paint(QPainter *painter)
{
	//sqt->PartialPaintQml(PRectFromQRect(/*boundingRect().toRect()*/painter->clipRegion().boundingRect()), painter);
	sqt->PartialPaintQml(PRectFromQRect(boundingRect().toRect()), painter);
}

#else

void ScintillaEditBase::paintEvent(QPaintEvent *event)
{
	sqt->PartialPaint(PRectFromQRect(event->rect()));
}

#endif

namespace {

bool isWheelEventHorizontal(QWheelEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	return event->angleDelta().y() == 0;
#else
	return event->orientation() == Qt::Horizontal;
#endif
}

int wheelEventYDelta(QWheelEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	return event->angleDelta().y();
#else
	return event->delta();
#endif
}

}

void ScintillaEditBase::wheelEvent(QWheelEvent *event)
{
	if (isWheelEventHorizontal(event)) {
#ifndef PLAT_QT_QML
		if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
			event->ignore();
		else
			QAbstractScrollArea::wheelEvent(event);
#else
			QQuickPaintedItem::wheelEvent(event);
#endif
	} else {
		if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
			// Zoom! We play with the font sizes in the styles.
			// Number of steps/line is ignored, we just care if sizing up or down
			if (wheelEventYDelta(event) > 0) {
				sqt->KeyCommand(Message::ZoomIn);
			} else {
				sqt->KeyCommand(Message::ZoomOut);
			}
		} else {
			// Ignore wheel events when the scroll bars are disabled.
#ifndef PLAT_QT_QML
			if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
				event->ignore();
			} else {
#endif
#ifdef PLAT_QT_QML
				// Scroll
				int linesToScroll = 3;
				//QQuickPaintedItem::wheelEvent(event);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                if (event->delta() > 0) {
#else
                if (event->angleDelta().y() > 0) {
#endif
                    sqt->ScrollTo(sqt->topLine-linesToScroll);
				} else {
					sqt->ScrollTo(sqt->topLine+linesToScroll);
				}
				QQuickPaintedItem::wheelEvent(event);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                update();
#endif
#else
				QAbstractScrollArea::wheelEvent(event);
			}
#endif
		}
	}
}

void ScintillaEditBase::focusInEvent(QFocusEvent *event)
{
	sqt->SetFocusState(true);

#ifdef PLAT_QT_QML
	QQuickPaintedItem::focusInEvent(event);
#else
	QAbstractScrollArea::focusInEvent(event);
#endif
}

void ScintillaEditBase::focusOutEvent(QFocusEvent *event)
{
	sqt->SetFocusState(false);

#ifdef PLAT_QT_QML
	QQuickPaintedItem::focusOutEvent(event);
#else
	QAbstractScrollArea::focusOutEvent(event);
#endif
}

#ifdef PLAT_QT_QML

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void ScintillaEditBase::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void ScintillaEditBase::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
	// trigger resize handling only if the size of the control has changed
	// no update is needed for a position change
	if(newGeometry.width() != oldGeometry.width() || newGeometry.height() != oldGeometry.height() )
	{
		sqt->ChangeSize();
		emit resized();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
#else
        QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
#endif
	}
}

void ScintillaEditBase::hoverMoveEvent(QHoverEvent *event)
{
	Q_UNUSED(event);
}

#else

void ScintillaEditBase::resizeEvent(QResizeEvent *)
{
	sqt->ChangeSize();
	emit resized();
}

#endif

void ScintillaEditBase::keyPressEvent(QKeyEvent *event)
{
	// All keystrokes containing the meta modifier are
	// assumed to be shortcuts not handled by scintilla.
	if (QApplication::keyboardModifiers() & Qt::MetaModifier) {
#ifdef PLAT_QT_QML
		QQuickPaintedItem::keyPressEvent(event);
#else
		QAbstractScrollArea::keyPressEvent(event);
#endif
		emit keyPressed(event);
		return;
	}

	int key = 0;
	switch (event->key()) {
		case Qt::Key_Down:          key = SCK_DOWN;     break;
		case Qt::Key_Up:            key = SCK_UP;       break;
		case Qt::Key_Left:          key = SCK_LEFT;     break;
		case Qt::Key_Right:         key = SCK_RIGHT;    break;
		case Qt::Key_Home:          key = SCK_HOME;     break;
		case Qt::Key_End:           key = SCK_END;      break;
		case Qt::Key_PageUp:        key = SCK_PRIOR;    break;
		case Qt::Key_PageDown:      key = SCK_NEXT;     break;
		case Qt::Key_Delete:        key = SCK_DELETE;   break;
		case Qt::Key_Insert:        key = SCK_INSERT;   break;
		case Qt::Key_Escape:        key = SCK_ESCAPE;   break;
		case Qt::Key_Backspace:     key = SCK_BACK;     break;
		case Qt::Key_Plus:          key = SCK_ADD;      break;
		case Qt::Key_Minus:         key = SCK_SUBTRACT; break;
		case Qt::Key_Backtab:       // fall through
		case Qt::Key_Tab:           key = SCK_TAB;      break;
		case Qt::Key_Enter:         // fall through
		case Qt::Key_Return:        key = SCK_RETURN;   break;
		case Qt::Key_Control:       key = 0;            break;
		case Qt::Key_Alt:           key = 0;            break;
		case Qt::Key_Shift:         key = 0;            break;
		case Qt::Key_Meta:          key = 0;            break;
		default:                    key = event->key(); break;
	}

#ifdef Q_OS_ANDROID
    // do not use the current state, because a key event could also be triggered by the input context menu as meta keyboard event ! --> needed for edit context menu on android platform
    bool shift = event->modifiers() & Qt::ShiftModifier;
    bool ctrl  = event->modifiers() & Qt::ControlModifier;
    bool alt   = event->modifiers() & Qt::AltModifier;
#else
	bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

	bool consumed = false;
	bool added = sqt->KeyDownWithModifiers(static_cast<Keys>(key),
                           ModifierFlags(shift, ctrl, alt),
					       &consumed) != 0;
	if (!consumed)
		consumed = added;

	if (!consumed) {
		// Don't insert text if the control key was pressed unless
		// it was pressed in conjunction with alt for AltGr emulation.
		bool input = (!ctrl || alt);

		// Additionally, on non-mac platforms, don't insert text
		// if the alt key was pressed unless control is also present.
		// On mac alt can be used to insert special characters.
#ifndef Q_WS_MAC
		input &= (!alt || ctrl);
#endif

		QString text = event->text();
		if (input && !text.isEmpty() && text[0].isPrint()) {
			QByteArray utext = sqt->BytesForDocument(text);
			sqt->InsertCharacter(std::string_view(utext.data(), utext.size()), CharacterSource::DirectInput);
		} else {
			event->ignore();
		}
	}

	emit keyPressed(event);
}

#ifdef Q_WS_X11
static int modifierTranslated(int sciModifier)
{
	switch (sciModifier) {
		case SCMOD_SHIFT:
			return Qt::ShiftModifier;
		case SCMOD_CTRL:
			return Qt::ControlModifier;
		case SCMOD_ALT:
			return Qt::AltModifier;
		case SCMOD_SUPER:
			return Qt::MetaModifier;
		default:
			return 0;
	}
}
#endif

void ScintillaEditBase::mousePressEvent(QMouseEvent *event)
{
	Point pos = PointFromQPoint(event->pos());

	emit buttonPressed(event);

	if (event->button() == Qt::MiddleButton &&
	    QApplication::clipboard()->supportsSelection()) {
		SelectionPosition selPos = sqt->SPositionFromLocation(
					pos, false, false, sqt->UserVirtualSpace());
		sqt->sel.Clear();
		sqt->SetSelection(selPos, selPos);
		sqt->PasteFromMode(QClipboard::Selection);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
		bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
#ifdef Q_WS_X11
		// On X allow choice of rectangular modifier since most window
		// managers grab alt + click for moving windows.
		bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);
#else
		bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

        sqt->ButtonDownWithModifiers(pos, time.elapsed(), ModifierFlags(shift, ctrl, alt));

#ifdef PLAT_QT_QML
        cursorChangedUpdateMarker();
#endif	
	}

	if (event->button() == Qt::RightButton) {
		sqt->RightButtonDownWithModifiers(pos, time.elapsed(), ModifiersOfKeyboard());

#ifdef PLAT_QT_QML
		Point pos = PointFromQPoint(event->globalPos());
		Point pt = PointFromQPoint(event->pos());
		if (!sqt->PointInSelection(pt)) {
			sqt->SetEmptySelection(sqt->PositionFromLocation(pt));
		}
// TODO: call context menu callback if set otherwise use default context menu...
		if (sqt->ShouldDisplayPopup(pt)) {
			sqt->ContextMenu(pos); 
		}
#endif
	}

#ifdef PLAT_QT_QML
//    setFocus(true);
    forceActiveFocus();

    emit enableScrollViewInteraction(false);

	event->setAccepted(true);
#endif
}


#ifdef PLAT_QT_QML
void ProcessScintillaContextMenu(Point pt, Scintilla::Internal::Window & w, const QList<QPair<QString, QPair<int, bool>>> & menu)
{
	ScintillaEditBase * pQtObj = static_cast<ScintillaEditBase *>(w.GetID());

	emit pQtObj->clearContextMenu();
	for (QPair<QString, QPair<int, bool>> item: menu)
	{
		if (item.first.size() > 0)
		{
			emit pQtObj->addToContextMenu(item.second.first, item.first, item.second.second);
		}	
	}

	QPoint aPoint(pt.x, pt.y);
	emit pQtObj->showContextMenu(aPoint);
}
#endif

void ScintillaEditBase::mouseReleaseEvent(QMouseEvent *event)
{
	const QPoint point = event->pos();
	if (event->button() == Qt::LeftButton)
		sqt->ButtonUpWithModifiers(PointFromQPoint(point), time.elapsed(), ModifiersOfKeyboard());

	const sptr_t pos = send(SCI_POSITIONFROMPOINT, point.x(), point.y());
	const sptr_t line = send(SCI_LINEFROMPOSITION, pos);
    int modifiers = QApplication::keyboardModifiers();

	emit textAreaClicked(line, modifiers);
	emit buttonReleased(event);

#ifdef PLAT_QT_QML
    emit enableScrollViewInteraction(true);

    event->setAccepted(true);
#endif
}

void ScintillaEditBase::mouseDoubleClickEvent(QMouseEvent *event)
{
	// Scintilla does its own double-click detection.
#ifndef PLAT_QT_QML
	mousePressEvent(event);
#else	
	Q_UNUSED(event);
#endif
}

void ScintillaEditBase::mouseMoveEvent(QMouseEvent *event)
{
	Point pos = PointFromQPoint(event->pos());

	bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
#ifdef Q_WS_X11
	// On X allow choice of rectangular modifier since most window
	// managers grab alt + click for moving windows.
	bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);
#else
	bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

    const KeyMod modifiers = ModifierFlags(shift, ctrl, alt);

	sqt->ButtonMoveWithModifiers(pos, time.elapsed(), modifiers);

#ifdef PLAT_QT_QML
    cursorChangedUpdateMarker();

    event->setAccepted(true);
#endif
}

#ifndef PLAT_QT_QML

void ScintillaEditBase::contextMenuEvent(QContextMenuEvent *event)
{
	Point pos = PointFromQPoint(event->globalPos());
	Point pt = PointFromQPoint(event->pos());
	if (!sqt->PointInSelection(pt)) {
		sqt->SetEmptySelection(sqt->PositionFromLocation(pt));
	}
	if (sqt->ShouldDisplayPopup(pt)) {
		sqt->ContextMenu(pos);
	}
}

#endif

void ScintillaEditBase::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		sqt->DragEnter(point);
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dragLeaveEvent(QDragLeaveEvent * /* event */)
{
	sqt->DragLeave();
}

void ScintillaEditBase::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		sqt->DragMove(point);
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
		sqt->DropUrls(event->mimeData());
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		bool move = (event->source() == this &&
                 event->proposedAction() == Qt::MoveAction);
		sqt->Drop(point, event->mimeData(), move);
	} else {
		event->ignore();
	}
}

bool ScintillaEditBase::IsHangul(const QChar qchar)
{
	unsigned int unicode = qchar.unicode();
	// Korean character ranges used for preedit chars.
	// http://www.programminginkorean.com/programming/hangul-in-unicode/
	const bool HangulJamo = (0x1100 <= unicode && unicode <= 0x11FF);
	const bool HangulCompatibleJamo = (0x3130 <= unicode && unicode <= 0x318F);
	const bool HangulJamoExtendedA = (0xA960 <= unicode && unicode <= 0xA97F);
	const bool HangulJamoExtendedB = (0xD7B0 <= unicode && unicode <= 0xD7FF);
	const bool HangulSyllable = (0xAC00 <= unicode && unicode <= 0xD7A3);
	return HangulJamo || HangulCompatibleJamo  || HangulSyllable ||
				HangulJamoExtendedA || HangulJamoExtendedB;
}

void ScintillaEditBase::MoveImeCarets(Scintilla::Position offset)
{
	// Move carets relatively by bytes
	for (size_t r=0; r < sqt->sel.Count(); r++) {
		const Sci::Position positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->sel.Range(r).caret.SetPosition(positionInsert + offset);
		sqt->sel.Range(r).anchor.SetPosition(positionInsert + offset);
 	}
}

void ScintillaEditBase::DrawImeIndicator(int indicator, int len)
{
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	if (indicator < INDICATOR_CONTAINER || indicator > INDICATOR_MAX) {
		return;
	}
	sqt->pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r=0; r< sqt-> sel.Count(); r++) {
		const Sci::Position positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

static int GetImeCaretPos(QInputMethodEvent *event)
{
	foreach (QInputMethodEvent::Attribute attr, event->attributes()) {
		if (attr.type == QInputMethodEvent::Cursor)
			return attr.start;
	}
	return 0;
}

static std::vector<int> MapImeIndicators(QInputMethodEvent *event)
{
	std::vector<int> imeIndicator(event->preeditString().size(), IndicatorUnknown);
	foreach (QInputMethodEvent::Attribute attr, event->attributes()) {
		if (attr.type == QInputMethodEvent::TextFormat) {
			QTextFormat format = attr.value.value<QTextFormat>();
			QTextCharFormat charFormat = format.toCharFormat();

			int indicator = IndicatorUnknown;
			switch (charFormat.underlineStyle()) {
				case QTextCharFormat::NoUnderline: // win32, linux
					indicator = IndicatorTarget;
					break;
				case QTextCharFormat::SingleUnderline: // osx
				case QTextCharFormat::DashUnderline: // win32, linux
					indicator = IndicatorInput;
					break;
				case QTextCharFormat::DotLine:
				case QTextCharFormat::DashDotLine:
				case QTextCharFormat::WaveUnderline:
				case QTextCharFormat::SpellCheckUnderline:
					indicator = IndicatorConverted;
					break;

				default:
					indicator = IndicatorUnknown;
			}

			if (format.hasProperty(QTextFormat::BackgroundBrush)) // win32, linux
				indicator = IndicatorTarget;

#ifdef Q_OS_OSX
			if (charFormat.underlineStyle() == QTextCharFormat::SingleUnderline) {
				QColor uc = charFormat.underlineColor();
				if (uc.lightness() < 2) { // osx
					indicator = IndicatorTarget;
				}
			}
#endif

			for (int i = attr.start; i < attr.start+attr.length; i++) {
				imeIndicator[i] = indicator;
			}
		}
	}
	return imeIndicator;
}

void ScintillaEditBase::inputMethodEvent(QInputMethodEvent *event)
{
	// Copy & paste by johnsonj with a lot of helps of Neil
	// Great thanks for my forerunners, jiniya and BLUEnLIVE

	if (sqt->pdoc->IsReadOnly() || sqt->SelectionContainsProtected()) {
		// Here, a canceling and/or completing composition function is needed.
		return;
	}

    // used from QQuickTextControlPrivate::inputMethodEvent()
    for (int i = 0; i < event->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = event->attributes().at(i);
        if (a.type == QInputMethodEvent::Selection) {
            Sci::Position curPos = sqt->CurrentPosition();
            SelectionPosition selStart = sqt->SelectionStart();
            SelectionPosition selEnd = sqt->SelectionEnd();

            int paraStart = sqt->pdoc->ParaUp(curPos);

            //selStart.Add(a.length);
            SelectionPosition newStart(paraStart + a.start);
            SelectionPosition newEnd(paraStart + a.start + a.length);
            if(newStart>newEnd)
            {
                sqt->SetSelection(newEnd, newStart);
            }
            else
            {
                sqt->SetSelection(newStart, newEnd);
            }
            curPos = sqt->CurrentPosition();

            // update markers by triggering QtAndroidInputContext::updateSelectionHandles()
#ifdef PLAT_QT_QML
            cursorChangedUpdateMarker();
#endif
        }
        if (a.type == QInputMethodEvent::Cursor) {
            //hasImState = true;
            //preeditCursor = a.start;
            //cursorVisible = a.length != 0;
        }
    }

	bool initialCompose = false;
	if (sqt->pdoc->TentativeActive()) {
		sqt->pdoc->TentativeUndo();
	} else {
		// No tentative undo means start of this composition so
		// Fill in any virtual spaces.
		initialCompose = true;
	}

	sqt->view.imeCaretBlockOverride = false;

	if (!event->commitString().isEmpty()) {
		const QString &commitStr = event->commitString();
		const int commitStrLen = commitStr.length();

		for (int i = 0; i < commitStrLen;) {
			const int ucWidth = commitStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = commitStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);

			sqt->InsertCharacter(std::string_view(oneChar.data(), oneChar.length()), CharacterSource::DirectInput);
			i += ucWidth;
		}

	} else if (!event->preeditString().isEmpty()) {
		const QString preeditStr = event->preeditString();
		const int preeditStrLen = preeditStr.length();
		if (preeditStrLen == 0) {
			sqt->ShowCaretAtCurrentPosition();
			return;
		}

		if (initialCompose)
			sqt->ClearBeforeTentativeStart();
		sqt->pdoc->TentativeStart(); // TentativeActive() from now on.

		std::vector<int> imeIndicator = MapImeIndicators(event);

		for (int i = 0; i < preeditStrLen;) {
			const int ucWidth = preeditStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = preeditStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);
			const int oneCharLen = oneChar.length();

			sqt->InsertCharacter(std::string_view(oneChar.data(), oneCharLen), CharacterSource::TentativeInput);

			DrawImeIndicator(imeIndicator[i], oneCharLen);
			i += ucWidth;
		}

		// Move IME carets.
		int imeCaretPos = GetImeCaretPos(event);
		int imeEndToImeCaretU16 = imeCaretPos - preeditStrLen;
		const Sci::Position imeCaretPosDoc = sqt->pdoc->GetRelativePositionUTF16(sqt->CurrentPosition(), imeEndToImeCaretU16);

		MoveImeCarets(- sqt->CurrentPosition() + imeCaretPosDoc);

		if (IsHangul(preeditStr.at(0))) {
#ifndef Q_OS_WIN
			if (imeCaretPos > 0) {
				int oneCharBefore = sqt->pdoc->GetRelativePosition(sqt->CurrentPosition(), -1);
				MoveImeCarets(- sqt->CurrentPosition() + oneCharBefore);
			}
#endif
			sqt->view.imeCaretBlockOverride = true;
		}

		// Set candidate box position for Qt::ImMicroFocus.
		preeditPos = sqt->CurrentPosition();
		sqt->EnsureCaretVisible();
#ifndef PLAT_QT_QML		
		updateMicroFocus();
#endif
	}
	sqt->ShowCaretAtCurrentPosition();
}

qreal xoff = 5.0;
qreal yoff = 5.0;

QVariant ScintillaEditBase::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    // see: QQuickTextEdit::inputMethodQuery(...)

    if (property == Qt::ImCursorPosition && !argument.isNull()) {
        argument = QVariant(argument.toPointF() - QPointF(xoff, yoff));
        const QPointF pt = argument.toPointF();
        if (!pt.isNull()) {
            Point scintillaPoint = PointFromQPointF(pt);
            Sci::Position ptPos = sqt->PositionFromLocation(scintillaPoint);
            int pos = send(SCI_GETCURRENTPOS);
            int paraStart = sqt->pdoc->ParaUp(pos);
            return QVariant((int)ptPos - paraStart);
        }
        return inputMethodQuery(property);
    }

    auto v = inputMethodQuery(property);
    if (property == Qt::ImCursorRectangle || property == Qt::ImAnchorRectangle)
    {
        v = QVariant(v.toRectF().translated(xoff, yoff));
    }

    return v;
}

QVariant ScintillaEditBase::inputMethodQuery(Qt::InputMethodQuery query) const
{
	const Scintilla::Position pos = send(SCI_GETCURRENTPOS);
	const Scintilla::Position line = send(SCI_LINEFROMPOSITION, pos);

	switch (query) {
#ifdef PLAT_QT_QML
        case Qt::ImEnabled:
            {
                return QVariant((bool)(flags() & ItemAcceptsInputMethod));
            }
        case Qt::ImHints:
			return QVariant((int)inputMethodHints());
        case Qt::ImInputItemClipRectangle:
			return QQuickItem::inputMethodQuery(query);
        // see: QQuickTextControl::inputMethodQuery()
        case Qt::ImMaximumTextLength:
            return QVariant(); // No limit.
        case Qt::ImAnchorRectangle:
            {
                SelectionPosition selStart = sqt->SelectionStart();
                SelectionPosition selEnd = sqt->SelectionEnd();
                //Sci::Position ptStart = selStart.Position();
                //Sci::Position ptEnd = selEnd.Position();

                Point ptStart = sqt->LocationFromPosition(selStart);
                Point ptEnd = sqt->LocationFromPosition(selEnd);

                int width = send(SCI_GETCARETWIDTH);
                int height = send(SCI_TEXTHEIGHT, line);
                return QRect(ptEnd.x, ptEnd.y, width, height);
            }
        // selection == Position <--> AnchorPosition
        case Qt::ImAnchorPosition:
            {
                //Sci::Position curPos = sqt->CurrentPosition();
                SelectionPosition selStart = sqt->SelectionStart();
                SelectionPosition selEnd = sqt->SelectionEnd();
                ////Point ptEnd = selEnd.Position();
                //return QVariant(/*ptEnd*/10);

                int paraStart = sqt->pdoc->ParaUp(pos);
                return (int)selEnd.Position() - paraStart;
            }
        case Qt::ImAbsolutePosition:
            {
                //Sci::Position curPos = sqt->CurrentPosition();
                //return QVariant((int)curPos);
                return QVariant((int)pos);
            }
        case Qt::ImTextAfterCursor:
        {
            // from Qt::ImSurroundingText:
            int paraStart = sqt->pdoc->ParaUp(pos);
            int paraEnd = sqt->pdoc->ParaDown(pos);
            QVarLengthArray<char,1024> buffer(paraEnd - paraStart + 1);

            Sci_CharacterRange charRange;
            charRange.cpMin = pos;
            charRange.cpMax = paraEnd;

            Sci_TextRange textRange;
            textRange.chrg = charRange;
            textRange.lpstrText = buffer.data();

            send(SCI_GETTEXTRANGE, 0, (sptr_t)&textRange);

            return sqt->StringFromDocument(buffer.constData());
        }
        case Qt::ImTextBeforeCursor:
        {
            // from Qt::ImSurroundingText:
            int paraStart = sqt->pdoc->ParaUp(pos);
            int paraEnd = sqt->pdoc->ParaDown(pos);
            QVarLengthArray<char,1024> buffer(paraEnd - paraStart + 1);

            Sci_CharacterRange charRange;
            charRange.cpMin = paraStart;
            charRange.cpMax = pos;

            Sci_TextRange textRange;
            textRange.chrg = charRange;
            textRange.lpstrText = buffer.data();

            send(SCI_GETTEXTRANGE, 0, (sptr_t)&textRange);

            return sqt->StringFromDocument(buffer.constData());
        }
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		// Qt 5 renamed ImMicroFocus to ImCursorRectangle then deprecated
		// ImMicroFocus. Its the same value (2) and same description.
		case Qt::ImCursorRectangle:
		{
			const Scintilla::Position startPos = (preeditPos >= 0) ? preeditPos : pos;
			const Point pt = sqt->LocationFromPosition(startPos);
			const int width = static_cast<int>(send(SCI_GETCARETWIDTH));
			const int height = static_cast<int>(send(SCI_TEXTHEIGHT, line));
			return QRectF(pt.x, pt.y, width, height).toRect();
		}
#else
		case Qt::ImMicroFocus:
		{
			const Scintilla::Position startPos = (preeditPos >= 0) ? preeditPos : pos;
			const Point pt = sqt->LocationFromPosition(startPos);
			const int width = static_cast<int>(send(SCI_GETCARETWIDTH));
			const int height = static_cast<int>(send(SCI_TEXTHEIGHT, line));
			return QRect(pt.x, pt.y, width, height);
		}
#endif

		case Qt::ImFont:
		{
			char fontName[64];
			const sptr_t style = send(SCI_GETSTYLEAT, pos);
			const int len = static_cast<int>(sends(SCI_STYLEGETFONT, style, fontName));
			const int size = static_cast<int>(send(SCI_STYLEGETSIZE, style));
			const bool italic = send(SCI_STYLEGETITALIC, style);
			const int weight = send(SCI_STYLEGETBOLD, style) ? QFont::Bold : -1;
			return QFont(QString::fromUtf8(fontName, len), size, weight, italic);
		}

		case Qt::ImCursorPosition:
		{
			const Scintilla::Position paraStart = sqt->pdoc->ParaUp(pos);
			return QVariant(static_cast<int>(pos - paraStart));
		}

		case Qt::ImSurroundingText:
		{
			const Scintilla::Position paraStart = sqt->pdoc->ParaUp(pos);
			const Scintilla::Position paraEnd = sqt->pdoc->ParaDown(pos);
			const std::string buffer = sqt->RangeText(paraStart, paraEnd);
			return sqt->StringFromDocument(buffer.c_str());
		}

		case Qt::ImCurrentSelection:
		{
			QVarLengthArray<char,1024> buffer(send(SCI_GETSELTEXT));
			sends(SCI_GETSELTEXT, 0, buffer.data());

			return sqt->StringFromDocument(buffer.constData());
		}

		default:
			return QVariant();
	}
}

#ifdef PLAT_QT_QML

void ScintillaEditBase::touchEvent(QTouchEvent *event)
{
    if(sqt->pdoc->IsReadOnly())
    {
        //event->ignore();        // --> transformiert touchEvents in mouseEvents !!!
        return;
    }

    forceActiveFocus();

    if( event->touchPointStates() == Qt::TouchPointPressed && event->touchPoints().count()>0 )
    {
        aLastTouchPressTime = time.elapsed();
        QTouchEvent::TouchPoint point = event->touchPoints().first();
        QPoint mousePressedPoint = point.pos().toPoint();
        //mouseMoved = false;
        //mouseDeltaLineMove = 0;

        // moves the cursor...
        //Point scintillaPoint = PointFromQPoint(mousePressedPoint);
        //Sci::Position pos = sqt->PositionFromLocation(scintillaPoint);
        //sqt->MovePositionTo(pos);

        longTouchPoint = point.pos().toPoint();

//#ifndef Q_OS_ANDROID
//        aLongTouchTimer.start(500);
//#endif

        cursorChangedUpdateMarker();
    }
//    if( event->touchPointStates() == Qt::TouchPointStationary && event->touchPoints().count()>0 )
//    {
//        QTouchEvent::TouchPoint point = event->touchPoints().first();
//        emit showContextMenu(point.pos().toPoint());
//    }
    else if(event->touchPointStates() == Qt::TouchPointStationary && event->touchPoints().count()>0)
    {
    }
    else if(event->touchPointStates() == Qt::TouchPointMoved && event->touchPoints().count()>0)
    {
//#ifndef Q_OS_ANDROID
//        aLongTouchTimer.stop();
//#endif
        }
    else if( event->touchPointStates() == Qt::TouchPointReleased && event->touchPoints().count()>0 )
    {
//#ifndef Q_OS_ANDROID
//        aLongTouchTimer.stop();
//#endif
        // is ths a short touch (time between press and release < 100ms) ?
        if(aLastTouchPressTime>=0 && (time.elapsed()-aLastTouchPressTime)<100)
        {
            QTouchEvent::TouchPoint point = event->touchPoints().first();
            QPoint mousePressedPoint = point.pos().toPoint();
            Point scintillaPoint = PointFromQPoint(mousePressedPoint);

            Sci::Position pos = sqt->PositionFromLocation(scintillaPoint);
            sqt->MovePositionTo(pos);

#ifdef Q_OS_ANDROID
            // Android: trigger software keyboard for inputs:
            // https://stackoverflow.com/questions/39436518/how-to-get-the-android-keyboard-to-appear-when-using-qt-for-android
            // https://stackoverflow.com/questions/5724811/how-to-show-the-keyboard-on-qt

            // Check if not in readonly modus --> pdoc->IsReadOnly()
            if( hasActiveFocus() && !sqt->pdoc->IsReadOnly() )
            {
                // TODO working: QGuiApplication::inputMethod()->commit();

                // QML: Qt.inputMethod.show();
                QInputMethod *keyboard = qGuiApp->inputMethod();
                //QInputMethod *keyboard = QGuiApplication::inputMethod();
                if(!keyboard->isVisible())
                {
                    keyboard->show();
                }
            }
#endif

            cursorChangedUpdateMarker();
        }
    }
    else
    {
        QQuickPaintedItem::touchEvent(event);
        return;
    }

    event->accept();
}

#endif

void ScintillaEditBase::notifyParent(NotificationData scn)
{
	emit notify(&scn);
	switch (scn.nmhdr.code) {
		case Notification::StyleNeeded:
			emit styleNeeded(scn.position);
			break;

		case Notification::CharAdded:
			emit charAdded(scn.ch);
			break;

		case Notification::SavePointReached:
			emit savePointChanged(false);
			break;

		case Notification::SavePointLeft:
			emit savePointChanged(true);
			break;

		case Notification::ModifyAttemptRO:
			emit modifyAttemptReadOnly();
			break;

		case Notification::Key:
			emit key(scn.ch);
			break;

		case Notification::DoubleClick:
			emit doubleClick(scn.position, scn.line);
			break;

		case Notification::UpdateUI:
#ifdef PLAT_QT_QML
			UpdateQuickView();
#endif			
			emit updateUi(scn.updated);
			break;

		case Notification::Modified:
		{
			const bool added = FlagSet(scn.modificationType, ModificationFlags::InsertText);
			const bool deleted = FlagSet(scn.modificationType, ModificationFlags::DeleteText);

			const Scintilla::Position length = send(SCI_GETTEXTLENGTH);
			bool firstLineAdded = (added && length == 1) ||
			                      (deleted && length == 0);

			if (scn.linesAdded != 0) {
				emit linesAdded(scn.linesAdded);
			} else if (firstLineAdded) {
				emit linesAdded(added ? 1 : -1);
			}

			const QByteArray bytes = QByteArray::fromRawData(scn.text, scn.text ? scn.length : 0);
			emit modified(scn.modificationType, scn.position, scn.length,
			              scn.linesAdded, bytes, scn.line,
			              scn.foldLevelNow, scn.foldLevelPrev);
			break;
		}

		case Notification::MacroRecord:
			emit macroRecord(scn.message, scn.wParam, scn.lParam);
			break;

		case Notification::MarginClick:
			emit marginClicked(scn.position, scn.modifiers, scn.margin);
			break;

		case Notification::NeedShown:
			emit needShown(scn.position, scn.length);
			break;

		case Notification::Painted:
			emit painted();
			break;

		case Notification::UserListSelection:
			emit userListSelection();
			break;

		case Notification::URIDropped:
			emit uriDropped(QString::fromUtf8(scn.text));
			break;

		case Notification::DwellStart:
			emit dwellStart(scn.x, scn.y);
			break;

		case Notification::DwellEnd:
			emit dwellEnd(scn.x, scn.y);
			break;

		case Notification::Zoom:
			emit zoom(send(SCI_GETZOOM));
			break;

		case Notification::HotSpotClick:
			emit hotSpotClick(scn.position, scn.modifiers);
			break;

		case Notification::HotSpotDoubleClick:
			emit hotSpotDoubleClick(scn.position, scn.modifiers);
			break;

		case Notification::CallTipClick:
			emit callTipClick();
			break;

		case Notification::AutoCSelection:
			emit autoCompleteSelection(scn.lParam, QString::fromUtf8(scn.text));
			break;

		case Notification::AutoCCancelled:
			emit autoCompleteCancelled();
			break;

		case Notification::FocusIn:
			emit focusChanged(true);
			break;

		case Notification::FocusOut:
			emit focusChanged(false);
			break;

		default:
			return;
	}
}

void ScintillaEditBase::event_command(uptr_t wParam, sptr_t lParam)
{
	emit command(wParam, lParam);
}

KeyMod ScintillaEditBase::ModifiersOfKeyboard()
{
	const bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	const bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	const bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;

    return ModifierFlags(shift, ctrl, alt);
}


#ifdef PLAT_QT_QML

QString ScintillaEditBase::getText() const
{
	int textLength = send(SCI_GETTEXTLENGTH);
	char * buffer = new char[textLength+1];
	send(SCI_GETTEXT,textLength+1,(sptr_t)buffer);
	QString ret(buffer);
	delete [] buffer;
	return ret;
}

void ScintillaEditBase::setText(const QString & txt)
{
	send(SCI_SETTEXT, 0, (sptr_t)txt.toUtf8().constData());
	send(SCI_EMPTYUNDOBUFFER);
	send(SCI_COLOURISE, 0, -1);
	setFocus(true);
}

void ScintillaEditBase::setFont(const QFont & newFont)
{
//TODO: maybe    QApplication::setFont(newFont);
	aFont = newFont;

	// Set the font for a style.
	setStylesFont(newFont, 0);
}

int ScintillaEditBase::getLogicalWidth() const
{
	return logicalWidth;
}

int ScintillaEditBase::getLogicalHeight() const
{
	return logicalHeight;
}

int ScintillaEditBase::getCharHeight() const
{
	int charHeight = send(SCI_TEXTHEIGHT);
	return charHeight;
}

int ScintillaEditBase::getCharWidth() const
{
	char buf[2];
	strcpy(buf,"X");
	int charWidth = send(SCI_TEXTWIDTH,0,(sptr_t)buf);
	return charWidth;
}

int ScintillaEditBase::getFirstVisibleLine() const
{
	int firstLine = send(SCI_GETFIRSTVISIBLELINE);
	return firstLine;
}

void ScintillaEditBase::setFirstVisisbleLine(int lineNo)
{
	send(SCI_SETFIRSTVISIBLELINE, lineNo);
}

int ScintillaEditBase::getTotalLines() const
{
	int lineCount = send(SCI_GETLINECOUNT);
	return lineCount;
}

int ScintillaEditBase::getFirstVisibleColumn() const
{
	int offset = send(SCI_GETXOFFSET) / getCharWidth();
	return offset;
}

int ScintillaEditBase::getTotalColumns() const
{
	int columnCount = send(SCI_GETSCROLLWIDTH)/getCharWidth();
	return columnCount;
}

int ScintillaEditBase::getVisibleLines() const
{
	int count = send(SCI_LINESONSCREEN);
	return count;
}

int ScintillaEditBase::getVisibleColumns() const
{
	int marginWidth = send(SCI_GETMARGINWIDTHN);
	int visibleWidth = sqt->GetTextRectangle().Width() - marginWidth;
	int count = visibleWidth / getCharWidth();
	return count;
}

Qt::InputMethodHints ScintillaEditBase::inputMethodHints() const
{
	return dataInputMethodHints | Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhMultiLine;
}

void ScintillaEditBase::setInputMethodHints(Qt::InputMethodHints hints)
{
	if (hints == dataInputMethodHints)
		return;

	dataInputMethodHints = hints;
	updateInputMethod(Qt::ImHints);
	emit inputMethodHintsChanged();
}

bool ScintillaEditBase::getReadonly() const
{
	bool flag = (bool)send(SCI_GETREADONLY);
	return flag;
}

void ScintillaEditBase::setReadonly(bool value)
{
    if(value != getReadonly())
    {
        send(SCI_SETREADONLY, value);

        emit readonlyChanged();
    }
}

void ScintillaEditBase::UpdateQuickView()
{
	int lineCount = send(SCI_GETLINECOUNT);
	int textWidth = send(SCI_GETSCROLLWIDTH);
	int lineHeight = send(SCI_TEXTHEIGHT);
	int textHeight = lineCount * lineHeight;

	bool widthChanged = logicalWidth != textWidth;
	bool heightChanged = logicalHeight != textHeight;
	if( widthChanged)
	{
		logicalWidth = textWidth;
		emit logicalWidthChanged();
	}
	if( heightChanged )
	{
		logicalHeight = textHeight;
		emit logicalHeightChanged();
	}

// TODO: hier nur signal senden, wenn wirklich notwendig, d. h. falls gescrollt wurde ...
	emit firstVisibleLineChanged();
	emit firstVisibleColumnChanged();

#ifdef PLAT_QT_QML
    cursorChangedUpdateMarker();
#endif
}

// taken from QScintilla
void ScintillaEditBase::setStylesFont(const QFont &f, int style)
{
	send(SCI_STYLESETFONT, style, (sptr_t)f.family().toLatin1().data());
	send(SCI_STYLESETSIZEFRACTIONAL, style,
            long(f.pointSizeF() * SC_FONT_SIZE_MULTIPLIER));

	// Pass the Qt weight via the back door.
	send(SCI_STYLESETWEIGHT, style, -f.weight());

	send(SCI_STYLESETITALIC, style, f.italic());
	send(SCI_STYLESETUNDERLINE, style, f.underline());

	// Tie the font settings of the default style to that of style 0 (the style
	// conventionally used for whitespace by lexers).  This is needed so that
	// fold marks, indentations, edge columns etc are set properly.
	if (style == 0)
		setStylesFont(f, STYLE_DEFAULT);
}

void ScintillaEditBase::cursorChangedUpdateMarker()
{
    if(!sqt->pdoc->IsReadOnly())
    {
        emit qApp->inputMethod()->cursorRectangleChanged();   // IMPORTANT: this moves the handle !!! see: QQuickTextControl::updateCursorRectangle()
        emit qApp->inputMethod()->anchorRectangleChanged();
        emit cursorPositionChanged();
    }
}

void RegisterScintillaType()
{
    qmlRegisterType<ScintillaEditBase>("org.scintilla.scintilla", 1, 0, "ScintillaEditBase");
}

#endif
