//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaEditBase.h - Qt widget that wraps ScintillaQt and provides events and scrolling
//
// Additions Copyright (c) 2020 Michael Neuroth
// Scintilla platform layer for Qt QML/Quick


#ifndef SCINTILLAEDITBASE_H
#define SCINTILLAEDITBASE_H

#include <cstddef>

#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "Debugging.h"
#include "Geometry.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "Platform.h"
#include "Scintilla.h"

#ifdef PLAT_QT_QML
#include <QFont>
#include <QTimer>
#include <QQuickPaintedItem>
#else
#include <QAbstractScrollArea>
#endif
#include <QMimeData>
#include <QElapsedTimer>

namespace Scintilla::Internal {

class ScintillaQt;
class SurfaceImpl;

}

#ifndef EXPORT_IMPORT_API
#ifdef WIN32
#ifdef MAKING_LIBRARY
#define EXPORT_IMPORT_API __declspec(dllexport)
#else
// Defining dllimport upsets moc
#define EXPORT_IMPORT_API __declspec(dllimport)
//#define EXPORT_IMPORT_API
#endif
#else
#define EXPORT_IMPORT_API
#endif
#endif

// REMARK:
// In Qt QML/Quick modus the scrollbar handling should be handled outside
// the scintilla editor control, for example in a ScrolView component.
// This is needed to optimize the user interaction on touch devices.
// In this modus the scintilla editor control runs alway with a (maximal)
// surface area to show the control completely. Updating the implicitSize
// of the scintilla editor control is handled in the overloaded paint(.) method.
class EXPORT_IMPORT_API ScintillaEditBase : public
#ifdef PLAT_QT_QML
	QQuickPaintedItem
#else
	QAbstractScrollArea
#endif
{
	Q_OBJECT

#ifdef PLAT_QT_QML
	friend class SciTEQt;

	Q_PROPERTY(QString text READ getText WRITE setText NOTIFY textChanged)
	Q_PROPERTY(QFont font READ getFont WRITE setFont NOTIFY fontChanged)
	Q_PROPERTY(bool readonly READ getReadonly WRITE setReadonly NOTIFY readonlyChanged)
	Q_PROPERTY(int logicalWidth READ getLogicalWidth NOTIFY logicalWidthChanged)
	Q_PROPERTY(int logicalHeight READ getLogicalHeight NOTIFY logicalHeightChanged)
	Q_PROPERTY(int charHeight READ getCharHeight NOTIFY charHeightChanged)
	Q_PROPERTY(int charWidth READ getCharWidth NOTIFY charWidthChanged)
	Q_PROPERTY(int totalLines READ getTotalLines NOTIFY totalLinesChanged)
	Q_PROPERTY(int totalColumns READ getTotalColumns NOTIFY totalColumnsChanged)
	Q_PROPERTY(int visibleLines READ getVisibleLines NOTIFY visibleLinesChanged)
	Q_PROPERTY(int visibleColumns READ getVisibleColumns NOTIFY visibleColumnsChanged)
	Q_PROPERTY(int firstVisibleLine READ getFirstVisibleLine WRITE setFirstVisisbleLine NOTIFY firstVisibleLineChanged)
	Q_PROPERTY(int firstVisibleColumn READ getFirstVisibleColumn NOTIFY firstVisibleColumnChanged)
	Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged)
	//QML_ELEMENT
#endif

public:
#ifdef PLAT_QT_QML
	explicit ScintillaEditBase(QQuickItem/*QWidget*/ *parent = 0);
#else
	explicit ScintillaEditBase(QWidget *parent = 0);
#endif
	virtual ~ScintillaEditBase();

	virtual sptr_t send(
		unsigned int iMessage,
		uptr_t wParam = 0,
		sptr_t lParam = 0) const;

	virtual sptr_t sends(
		unsigned int iMessage,
		uptr_t wParam = 0,
		const char *s = 0) const;

#ifdef PLAT_QT_QML
	Q_INVOKABLE void scrollRow(int deltaLines);
	Q_INVOKABLE void scrollColumn(int deltaColumns);
	Q_INVOKABLE void enableUpdate(bool enable);
	Q_INVOKABLE void debug();
	Q_INVOKABLE virtual void cmdContextMenu(int menuID);
#endif

public slots:
	// Scroll events coming from GUI to be sent to Scintilla.
	void scrollHorizontal(int value);
	void scrollVertical(int value);

	// Emit Scintilla notifications as signals.
	void notifyParent(Scintilla::NotificationData scn);
	void event_command(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

#ifdef PLAT_QT_QML
    //void onLongTouch();
#endif

signals:
    void cursorPositionChanged();
	void horizontalScrolled(int value);
	void verticalScrolled(int value);
	void horizontalRangeChanged(int max, int page);
	void verticalRangeChanged(int max, int page);
	void notifyChange();
	void linesAdded(Scintilla::Position linesAdded);

	// Clients can use this hook to add additional
	// formats (e.g. rich text) to the MIME data.
	void aboutToCopy(QMimeData *data);

	// Scintilla Notifications
	void styleNeeded(Scintilla::Position position);
	void charAdded(int ch);
	void savePointChanged(bool dirty);
	void modifyAttemptReadOnly();
	void key(int key);
	void doubleClick(Scintilla::Position position, Scintilla::Position line);
	void updateUi(Scintilla::Update updated);
	void modified(Scintilla::ModificationFlags type, Scintilla::Position position, Scintilla::Position length, Scintilla::Position linesAdded,
		      const QByteArray &text, Scintilla::Position line, Scintilla::FoldLevel foldNow, Scintilla::FoldLevel foldPrev);
	void macroRecord(Scintilla::Message message, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void marginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin);
	void textAreaClicked(Scintilla::Position line, int modifiers);
	void needShown(Scintilla::Position position, Scintilla::Position length);
	void painted();
	void userListSelection(); // Wants some args.
	void uriDropped(const QString &uri);
	void dwellStart(int x, int y);
	void dwellEnd(int x, int y);
	void zoom(int zoom);
	void hotSpotClick(Scintilla::Position position, Scintilla::KeyMod modifiers);
	void hotSpotDoubleClick(Scintilla::Position position, Scintilla::KeyMod modifiers);
	void callTipClick();
	void autoCompleteSelection(Scintilla::Position position, const QString &text);
	void autoCompleteCancelled();
	void focusChanged(bool focused);

	// Base notifications for compatibility with other Scintilla implementations
	void notify(Scintilla::NotificationData *pscn);
	void command(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

	// GUI event notifications needed under Qt
	void buttonPressed(QMouseEvent *event);
	void buttonReleased(QMouseEvent *event);
	void keyPressed(QKeyEvent *event);
	void resized();
#ifdef PLAT_QT_QML
	void textChanged();
	void fontChanged();
	void readonlyChanged();
	void logicalWidthChanged();
	void logicalHeightChanged();
	void charHeightChanged();
	void charWidthChanged();
	void totalLinesChanged();
	void firstVisibleLineChanged();
	void firstVisibleColumnChanged();
	void totalColumnsChanged();
	void visibleLinesChanged();
	void visibleColumnsChanged();
	void inputMethodHintsChanged();
	void enableScrollViewInteraction(bool value);
	void showContextMenu(const QPoint & pos);
	void addToContextMenu(int menuId, const QString & txt, bool enabled);
	void clearContextMenu();
	//void readonlyChanged();
#endif

protected:
	bool event(QEvent *event) override;
#ifdef PLAT_QT_QML
	void paint(QPainter *painter) override;
#else
	void paintEvent(QPaintEvent *event) override;
#endif
	void wheelEvent(QWheelEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
#ifdef PLAT_QT_QML
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif
	void hoverMoveEvent(QHoverEvent *event) override;
#else
	void resizeEvent(QResizeEvent *event) override;
#endif
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
#ifndef PLAT_QT_QML
	// TODO: implement context menu for Qt QML/Quick
	void contextMenuEvent(QContextMenuEvent *event) override;
#endif
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void inputMethodEvent(QInputMethodEvent *event) override;
	QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const;
#ifndef PLAT_QT_QML
	void scrollContentsBy(int, int) override {}
#else
    void touchEvent(QTouchEvent *event) override;
#endif

private:
#ifdef PLAT_QT_QML
	QString getText() const;
	void setText(const QString & txt);
	QFont getFont() const { return aFont; }
	void setFont(const QFont & newFont);
	void setStylesFont(const QFont &f, int style);
	int getLogicalWidth() const;
	int getLogicalHeight() const;
	int getCharHeight() const;
	int getCharWidth() const;
	int getFirstVisibleLine() const;
	void setFirstVisisbleLine(int lineNo);
	int getTotalLines() const;
	int getFirstVisibleColumn() const;
	int getTotalColumns() const;
	int getVisibleLines() const;
	int getVisibleColumns() const;
	Qt::InputMethodHints inputMethodHints() const;
	void setInputMethodHints(Qt::InputMethodHints hints);
	bool getReadonly() const;
	void setReadonly(bool value);

    void cursorChangedUpdateMarker();

    void UpdateQuickView();

// TODO: set context menu callback... to add more menu items to context menu...
    //QPoint mousePressedPoint;
	bool enableUpdateFlag;
    //bool mouseMoved;
    //int mouseDeltaLineMove;
	int logicalWidth;
	int logicalHeight;
	QFont aFont;
	QPoint longTouchPoint;
	Qt::InputMethodHints dataInputMethodHints;
    //QTimer aLongTouchTimer;
    qint64 aLastTouchPressTime;
#endif

	Scintilla::Internal::ScintillaQt *sqt;

	QElapsedTimer time;

	Scintilla::Position preeditPos;
	QString preeditString;

	int wheelDelta;

	static bool IsHangul(const QChar qchar);
	void MoveImeCarets(Scintilla::Position offset);
	void DrawImeIndicator(int indicator, int len);
	static Scintilla::KeyMod ModifiersOfKeyboard();
};

void RegisterScintillaType();

#endif /* SCINTILLAEDITBASE_H */
