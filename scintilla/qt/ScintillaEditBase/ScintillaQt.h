//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// ScintillaQt.h - Qt specific subclass of ScintillaBase
//
// Additions Copyright (c) 2020 Michael Neuroth
// Scintilla platform layer for Qt QML/Quick

#ifndef SCINTILLAQT_H
#define SCINTILLAQT_H

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Scintilla.h"
#include "Platform.h"
#include "ILoader.h"
#include "ILexer.h"
#include "CharacterCategory.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "ScintillaBase.h"
#include "CaseConvert.h"

#include <QObject>
#ifdef PLAT_QT_QML
#include <QQuickPaintedItem>
#else
#include <QAbstractScrollArea>
#endif
#include <QAction>
#include <QClipboard>
#include <QPaintEvent>

class ScintillaEditBase;

namespace Scintilla {

class ScintillaQt : public QObject, public ScintillaBase {
	Q_OBJECT

public:
#ifdef PLAT_QT_QML
    explicit ScintillaQt(QQuickPaintedItem *parent);
    void UpdateInfos(int winId);
#else
    explicit ScintillaQt(QAbstractScrollArea *parent);
#endif
	virtual ~ScintillaQt();

signals:
	void horizontalScrolled(int value);
	void verticalScrolled(int value);
	void horizontalRangeChanged(int max, int page);
	void verticalRangeChanged(int max, int page);

	void notifyParent(SCNotification scn);
	void notifyChange();

	// Clients can use this hook to add additional
	// formats (e.g. rich text) to the MIME data.
	void aboutToCopy(QMimeData *data);

	void command(uptr_t wParam, sptr_t lParam);

private slots:
	void onIdle();
	void execCommand(QAction *action);
	void SelectionChanged();

private:
	void Init();
	void Finalise() override;
	bool DragThreshold(Point ptStart, Point ptNow) override;
	bool ValidCodePage(int codePage) const override;

private:
    //void DebugOutput(int value) override;
	void ScrollText(Sci::Line linesToMove) override;
	void SetVerticalScrollPos() override;
	void SetHorizontalScrollPos() override;
	bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
	void ReconfigureScrollBars() override;
	void CopyToModeClipboard(const SelectionText &selectedText, QClipboard::Mode clipboardMode_);
	void Copy() override;
	void CopyToClipboard(const SelectionText &selectedText) override;
	void PasteFromMode(QClipboard::Mode clipboardMode_);
	void Paste() override;
	void ClaimSelection() override;
	void NotifyChange() override;
	void NotifyFocus(bool focus) override;
	void NotifyParent(SCNotification scn) override;
	void NotifyURIDropped(const char *uri);
	int timers[tickDwell+1];
	bool FineTickerRunning(TickReason reason) override;
	void FineTickerStart(TickReason reason, int millis, int tolerance) override;
	void CancelTimers();
	void FineTickerCancel(TickReason reason) override;
	bool ChangeIdle(bool on);
	bool SetIdle(bool on) override;
	void SetMouseCapture(bool on) override;
	bool HaveMouseCapture() override;
	void StartDrag() override;
	int CharacterSetOfDocument() const;
	const char *CharacterSetIDOfDocument() const;
	QString StringFromDocument(const char *s) const;
	QByteArray BytesForDocument(const QString &text) const;
	CaseFolder *CaseFolderForEncoding() override;
	std::string CaseMapString(const std::string &s, int caseMapping) override;

	void CreateCallTipWindow(PRectangle rc) override;
    void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
public:
	sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;
	sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;
private:
	static sptr_t DirectFunction(sptr_t ptr,
				     unsigned int iMessage, uptr_t wParam, sptr_t lParam);

#ifdef PLAT_QT_QML
    QPainter * GetPainter() { return currentPainter; }
#endif

protected:

    void PartialPaint(const PRectangle &rect);
    void PartialPaintQml(const PRectangle & rect, QPainter *painter);

	void DragEnter(const Point &point);
	void DragMove(const Point &point);
	void DragLeave();
	void Drop(const Point &point, const QMimeData *data, bool move);
	void DropUrls(const QMimeData *data);

	void timerEvent(QTimerEvent *event) override;

private:
#ifdef PLAT_QT_QML
    QQuickPaintedItem *scrollArea;
#else
    QAbstractScrollArea *scrollArea;
#endif

	int vMax, hMax;   // Scroll bar maximums.
	int vPage, hPage; // Scroll bar page sizes.

	bool haveMouseCapture;
	bool dragWasDropped;
	int rectangularSelectionModifier;

#ifdef PLAT_QT_QML
    QPainter *currentPainter;  // temporary variable for paint() handling
#endif

    friend class ::ScintillaEditBase;
};

}

#endif // SCINTILLAQT_H
