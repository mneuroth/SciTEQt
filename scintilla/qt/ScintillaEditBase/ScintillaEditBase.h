//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// ScintillaWidget.h - Qt widget that wraps ScintillaQt and provides events and scrolling
//
// Additions Copyright (c) 2020 Michael Neuroth
// Scintilla platform layer for Qt QML/Quick


#ifndef SCINTILLAEDITBASE_H
#define SCINTILLAEDITBASE_H

#include <cstddef>

#include <string_view>
#include <vector>
#include <memory>

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

namespace Scintilla {

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

	Q_PROPERTY(QString text READ getText WRITE setText)
	Q_PROPERTY(QFont font READ getFont WRITE setFont)
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
	void notifyParent(SCNotification scn);
	void event_command(uptr_t wParam, sptr_t lParam);

#ifdef PLAT_QT_QML
	void onLongTouch();
#endif

signals:
	void horizontalScrolled(int value);
	void verticalScrolled(int value);
	void horizontalRangeChanged(int max, int page);
	void verticalRangeChanged(int max, int page);
	void notifyChange();
	void linesAdded(int linesAdded);

	// Clients can use this hook to add additional
	// formats (e.g. rich text) to the MIME data.
	void aboutToCopy(QMimeData *data);

	// Scintilla Notifications
	void styleNeeded(int position);
	void charAdded(int ch);
	void savePointChanged(bool dirty);
	void modifyAttemptReadOnly();
	void key(int key);
	void doubleClick(int position, int line);
	void updateUi(int updated);
	void modified(int type, int position, int length, int linesAdded,
	              const QByteArray &text, int line, int foldNow, int foldPrev);
	void macroRecord(int message, uptr_t wParam, sptr_t lParam);
	void marginClicked(int position, int modifiers, int margin);
	void textAreaClicked(int line, int modifiers);
	void needShown(int position, int length);
	void painted();
	void userListSelection(); // Wants some args.
	void uriDropped(const QString &uri);
	void dwellStart(int x, int y);
	void dwellEnd(int x, int y);
	void zoom(int zoom);
	void hotSpotClick(int position, int modifiers);
	void hotSpotDoubleClick(int position, int modifiers);
	void callTipClick();
	void autoCompleteSelection(int position, const QString &text);
	void autoCompleteCancelled();
	void focusChanged(bool focused);

	// Base notifications for compatibility with other Scintilla implementations
	void notify(SCNotification *pscn);
	void command(uptr_t wParam, sptr_t lParam);

	// GUI event notifications needed under Qt
	void buttonPressed(QMouseEvent *event);
	void buttonReleased(QMouseEvent *event);
	void keyPressed(QKeyEvent *event);
	void resized();
#ifdef PLAT_QT_QML
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
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
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

	void UpdateQuickView();

// TODO: set context menu callback... to add more menu items to context menu...
	QPoint mousePressedPoint;
	bool enableUpdateFlag;
	bool mouseMoved;
	int mouseDeltaLineMove;
	int logicalWidth;
	int logicalHeight;
	QFont aFont;
	QPoint longTouchPoint;
	Qt::InputMethodHints dataInputMethodHints;
	QTimer aLongTouchTimer;
#endif

	Scintilla::ScintillaQt *sqt;

	QElapsedTimer time;

	int preeditPos;
	QString preeditString;

	int wheelDelta;

	static bool IsHangul(const QChar qchar);
	void MoveImeCarets(int offset);
	void DrawImeIndicator(int indicator, int len);
	int ModifiersOfKeyboard() const;
};

#ifdef PLAT_QT_QML
void RegisterScintillaType();
#endif

#endif /* SCINTILLAEDITBASE_H */
