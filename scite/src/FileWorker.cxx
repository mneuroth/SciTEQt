// SciTE - Scintilla based Text Editor
/** @file FileWorker.cxx
 ** Implementation of classes to perform background file tasks as threads.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include "ILoader.h"

#include "GUI.h"

#include "FilePath.h"
#include "Cookie.h"
#include "Worker.h"
#include "FileWorker.h"
#include "Utf8_16.h"

constexpr double timeBetweenProgress = 0.4;

FileWorker::FileWorker(WorkerListener *pListener_, const FilePath &path_, size_t size_, FILE *fp_) :
	pListener(pListener_), path(path_), size(size_), err(0), fp(fp_), sleepTime(0), nextProgress(timeBetweenProgress) {
}

FileWorker::~FileWorker() {
}

double FileWorker::Duration() noexcept {
	return et.Duration();
}

FileLoader::FileLoader(WorkerListener *pListener_, ILoader *pLoader_, const FilePath &path_, size_t size_, FILE *fp_) :
	FileWorker(pListener_, path_, size_, fp_), pLoader(pLoader_), readSoFar(0), unicodeMode(uni8Bit) {
	SetSizeJob(size);
}

FileLoader::~FileLoader() {
}

void FileLoader::Execute() {
	if (fp) {
		Utf8_16_Read convert;
		std::vector<char> data(blockSize);
		size_t lenFile = fread(&data[0], 1, blockSize, fp);
		const UniMode umCodingCookie = CodingCookieValue(std::string_view(data.data(), lenFile));
		while ((lenFile > 0) && (err == 0) && (!Cancelling())) {
			GUI::SleepMilliseconds(sleepTime);
			lenFile = convert.convert(&data[0], lenFile);
			const char *dataBlock = convert.getNewBuf();
			err = pLoader->AddData(dataBlock, lenFile);
			IncrementProgress(lenFile);
			if (et.Duration() > nextProgress) {
				nextProgress = et.Duration() + timeBetweenProgress;
				pListener->PostOnMainThread(WORK_FILEPROGRESS, this);
			}
			lenFile = fread(&data[0], 1, blockSize, fp);
			if ((lenFile == 0) && (err == 0)) {
				// Handle case where convert is holding a lead surrogate but no more data
				const size_t lenFileTrail = convert.convert(nullptr, lenFile);
				if (lenFileTrail) {
					const char *dataTrail = convert.getNewBuf();
					err = pLoader->AddData(dataTrail, lenFileTrail);
				}
			}
		}
		fclose(fp);
		fp = nullptr;
		unicodeMode = static_cast<UniMode>(
				      static_cast<int>(convert.getEncoding()));
		// Check the first two lines for coding cookies
		if (unicodeMode == uni8Bit) {
			unicodeMode = umCodingCookie;
		}
	}
	SetCompleted();
	pListener->PostOnMainThread(WORK_FILEREAD, this);
}

void FileLoader::Cancel() {
	FileWorker::Cancel();
	pLoader->Release();
	pLoader = nullptr;
}

FileStorer::FileStorer(WorkerListener *pListener_, const char *documentBytes_, const FilePath &path_,
		       size_t size_, FILE *fp_, UniMode unicodeMode_, bool visibleProgress_) :
	FileWorker(pListener_, path_, size_, fp_), documentBytes(documentBytes_), writtenSoFar(0),
	unicodeMode(unicodeMode_), visibleProgress(visibleProgress_) {
	SetSizeJob(size);
}

FileStorer::~FileStorer() {
}

static bool IsUTF8TrailByte(int ch) noexcept {
	return (ch >= 0x80) && (ch < (0x80 + 0x40));
}

void FileStorer::Execute() {
	if (fp) {
		Utf8_16_Write convert;
		if (unicodeMode != uniCookie) {	// Save file with cookie without BOM.
			convert.setEncoding(static_cast<Utf8_16::encodingType>(
						    static_cast<int>(unicodeMode)));
		}
		convert.setfile(fp);
		std::vector<char> data(blockSize + 1);
		const size_t lengthDoc = size;
		size_t grabSize;
		for (size_t i = 0; i < lengthDoc && (!Cancelling()); i += grabSize) {
			GUI::SleepMilliseconds(sleepTime);
			grabSize = lengthDoc - i;
			if (grabSize > blockSize)
				grabSize = blockSize;
			if ((unicodeMode != uni8Bit) && (i + grabSize < lengthDoc)) {
				// Round down so only whole characters retrieved.
				size_t startLast = grabSize;
				while ((startLast > 0) && ((grabSize - startLast) < 6) && IsUTF8TrailByte(static_cast<unsigned char>(documentBytes[i + startLast])))
					startLast--;
				if ((grabSize - startLast) < 5)
					grabSize = startLast;
			}
			memcpy(&data[0], documentBytes+i, grabSize);
			const size_t written = convert.fwrite(&data[0], grabSize);
			IncrementProgress(grabSize);
			if (et.Duration() > nextProgress) {
				nextProgress = et.Duration() + timeBetweenProgress;
				pListener->PostOnMainThread(WORK_FILEPROGRESS, this);
			}
			if (written == 0) {
				err = 1;
				break;
			}
		}
		if (convert.fclose() != 0) {
			err = 1;
		}
	}
	SetCompleted();
	pListener->PostOnMainThread(WORK_FILEWRITTEN, this);
}

void FileStorer::Cancel() {
	FileWorker::Cancel();
}
