// SciTE - Scintilla based Text Editor
/** @file FileWorker.h
 ** Definition of classes to perform background file tasks as threads.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef FILEWORKER_H
#define FILEWORKER_H

/// Base size of file I/O operations.
constexpr size_t blockSize = 128 * 1024;

struct FileWorker : public Worker {
	WorkerListener *pListener;
	FilePath path;
	size_t size;
	int err;
	FILE *fp;
	GUI::ElapsedTime et;
	int sleepTime;
	double nextProgress;

	FileWorker(WorkerListener *pListener_, const FilePath &path_, size_t size_, FILE *fp_);
	~FileWorker() override;
	virtual double Duration() noexcept;
	void Cancel() override {
		Worker::Cancel();
	}
	virtual bool IsLoading() const noexcept = 0;
};

class FileLoader : public FileWorker {
public:
	ILoader *pLoader;
	size_t readSoFar;
	UniMode unicodeMode;

	FileLoader(WorkerListener *pListener_, ILoader *pLoader_, const FilePath &path_, size_t size_, FILE *fp_);
	~FileLoader() override;
	void Execute() override;
	void Cancel() override;
	bool IsLoading() const noexcept override {
		return true;
	}
};

class FileStorer : public FileWorker {
public:
	const char *documentBytes;
	size_t writtenSoFar;
	UniMode unicodeMode;
	bool visibleProgress;

	FileStorer(WorkerListener *pListener_, const char *documentBytes_, const FilePath &path_,
		   size_t size_, FILE *fp_, UniMode unicodeMode_, bool visibleProgress_);
	~FileStorer() override;
	void Execute() override;
	void Cancel() override;
	bool IsLoading() const noexcept override {
		return false;
	}
};

enum {
	WORK_FILEREAD = 1,
	WORK_FILEWRITTEN = 2,
	WORK_FILEPROGRESS = 3,
	WORK_PLATFORM = 100
};

#endif
