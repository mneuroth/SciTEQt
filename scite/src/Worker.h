// SciTE - Scintilla based Text Editor
/** @file Worker.h
 ** Definition of classes to perform background tasks as threads.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WORKER_H
#define WORKER_H

struct Worker {
private:
	std::atomic_bool completed;
	std::atomic_bool cancelling;
	std::atomic_size_t jobSize;
	std::atomic_size_t jobProgress;
public:
	Worker() : completed(false), cancelling(false), jobSize(1), jobProgress(0) {
	}
	// Deleted so Worker objects can not be copied.
	Worker(const Worker &) = delete;
	Worker(Worker &&) = delete;
	void operator=(const Worker &) = delete;
	void operator=(Worker &&) = delete;
	virtual ~Worker() {
	}
	virtual void Execute() {}
	bool FinishedJob() const noexcept {
		return completed;
	}
	void SetCompleted() noexcept {
		completed = true;
	}
	bool Cancelling() const noexcept {
		return cancelling;
	}
	size_t SizeJob() const noexcept {
		return jobSize;
	}
	void SetSizeJob(size_t size) noexcept {
		jobSize = size;
	}
	size_t ProgressMade() const noexcept {
		return jobProgress;
	}
	void IncrementProgress(size_t increment) noexcept {
		jobProgress += increment;
	}
	virtual void Cancel() {
		cancelling = true;
		// Wait for writing thread to finish
		for (;;) {
			if (completed)
				return;
		}
	}
};

struct WorkerListener {
	virtual void PostOnMainThread(int cmd, Worker *pWorker) = 0;
};

#endif
