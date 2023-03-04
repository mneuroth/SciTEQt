// SciTE - Scintilla based Text Editor
/** @file DirectorExtension.cxx
 ** Extension for communicating with a director program.
 ** This allows external client programs (and internal extensions) to communicate
 ** with instances of SciTE. The original scheme required you to define the property
 ** ipc.scite.name to be a valid (but _not_ created) pipename, which becomes the
 ** 'request pipe' for sending commands to SciTE. (The set of available commands
 ** is defined in SciTEBase::PerformOne()). One also had to specify a property
 ** ipc.director.name to be an _existing_ pipe which would receive notifications (like
 ** when a file is opened, buffers switched, etc).
 **
 **
 ** This version supports the old protocol, so existing clients such as ScitePM still
 ** work as before. But it is no longer necessary to specify these ipc properties.
 ** If ipc.scite.name is not defined, then a new request pipe is created of the form
 ** /tmp/SciTE.<pid>.in using the current pid. This pipename is put back into
 ** ipc.scite.name (this is useful for internal extensions that want to find _another_
 ** instance of SciTE). This pipe will be removed when SciTE closes normally,
 ** so listing all files with the pattern '/tmp/SciTE.*.in' will give the currently
 ** running SciTE instances.
 **
 ** If a client wants to receive notifications, they must ask using
 ** the register command, i.e. send ':<path to temp file>:register:' to the request
 ** pipe. SciTE will create a new notify pipe (of the form /tmp/SciTE.<pid>.<k>.out)
 ** and write it into the temp file, which the client can read and open.
 **
 ** This version also supports the 'correspondent' concept used by the Win32
 ** version; requests of the form ':<my pipe>:<command>:<args>' make any results
 ** get sent back to the specified, existing pipename <my pipe>. For example,
 ** ':/tmp/mypipe:askproperty:SciteDefaultHome' will make SciTE write the value of
 ** the standard property 'SciteDefaultHome' to the pipe /tmp/mypipe.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cerrno>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "Scintilla.h"

#include "GUI.h"
#include "ScintillaWindow.h"
#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"
#include "Extender.h"
#include "SciTE.h"
#include "JobQueue.h"
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "Searcher.h"
#include "SciTEBase.h"
#include "DirectorExtension.h"

static int fdDirector = 0;
static int fdCorrespondent = 0;
static int fdReceiver = 0;
static bool startedByDirector = false;
static bool shuttingDown = false;

// the number of notify connections this SciTE instance will handle.
const int MAX_PIPES = 20;

#define TMP_FILENAME_LENGTH 1024

static char requestPipeName[TMP_FILENAME_LENGTH];

//#define IF_DEBUG(x) x;
#define IF_DEBUG(x)

IF_DEBUG(static FILE *fdDebug = 0)

// managing a list of notification pipes
// this also allows for proper cleanup of any pipes
// that aren't _directly_ specified by the director.
struct PipeEntry {
	int fd;
	char *name;
	~PipeEntry() {
		free(name);
		name = 0;
	}
};
static PipeEntry s_send_pipes[MAX_PIPES];
static int s_send_cnt = 0;

static bool SendPipeAvailable() {
	return s_send_cnt < MAX_PIPES-1;
}

static void AddSendPipe(int fd, const char *name) {
	if (SendPipeAvailable()) {
		PipeEntry &entry = s_send_pipes[s_send_cnt++];
		entry.fd = fd;
		if (name)
			entry.name = strdup(name);
		else
			entry.name = NULL;
	}
}

static void RemoveSendPipes() noexcept {
	for (int i = 0; i < s_send_cnt; ++i) {
		PipeEntry &entry = s_send_pipes[i];
		close(entry.fd);
		if (entry.name)
			remove(entry.name);
	}
}

static bool MakePipe(const char *pipeName) {
	int res;
	res = mkfifo(pipeName, 0777);
	return res == 0;
}

static int OpenPipe(const char *pipeName) {
	int fd = open(pipeName, O_RDWR | O_NONBLOCK);
	return fd;
}

// we now send notifications to _all_ the notification pipes registered!
static bool SendPipeCommand(const char *pipeCommand) {
	int size;
	if (fdCorrespondent) {
		size = write(fdCorrespondent,pipeCommand,strlen(pipeCommand));
		size += write(fdCorrespondent,"\n",1);
		IF_DEBUG(fprintf(fdDebug, "Send correspondent: %s %d bytes to %d\n", pipeCommand, size,fdCorrespondent))
	} else {
		for (int i = 0; i < s_send_cnt; ++i) {
			int fd = s_send_pipes[i].fd;
			// put a linefeed after the notification!
			size = write(fd, pipeCommand, strlen(pipeCommand));
			size += write(fd,"\n",1);
			IF_DEBUG(fprintf(fdDebug, "Send pipecommand: %s %d bytes to %d\n", pipeCommand, size,fd))
		}
	}
	(void)size; // to keep compiler happy if we aren't debugging...
	return true;
}

static gboolean ReceiverPipeSignal(GIOChannel *source, GIOCondition condition, void *data) {
#ifndef GDK_VERSION_3_6
	gdk_threads_enter();
#endif

	if ((condition & G_IO_IN) == G_IO_IN) {
		std::string pipeString;
		char pipeData[8192];
		gsize readLength;
		GError *error = NULL;
		GIOStatus status = g_io_channel_read_chars(source, pipeData,
		        sizeof(pipeData) - 1, &readLength, &error);
		while ((status != G_IO_STATUS_ERROR) && (readLength > 0)) {
			pipeData[readLength] = '\0';
			pipeString.append(pipeData);
			status = g_io_channel_read_chars(source, pipeData,
			        sizeof(pipeData) - 1, &readLength, &error);
		}
		DirectorExtension *ext = static_cast<DirectorExtension *>(data);
		ext->HandleStringMessage(pipeString.c_str());
	}
#ifndef GDK_VERSION_3_6
	gdk_threads_leave();
#endif
	return TRUE;
}

static void SendDirector(const char *verb, const char *arg = 0) {
	IF_DEBUG(fprintf(fdDebug, "SendDirector:(%s, %s):  fdDirector = %d\n", verb, arg, fdDirector))
	if (s_send_cnt) {
		std::string addressedMessage;
		addressedMessage += verb;
		addressedMessage += ":";
		if (arg)
			addressedMessage += arg;
		//send the message through all the registered pipes
		::SendPipeCommand(addressedMessage.c_str());
	} else {
		IF_DEBUG(fprintf(fdDebug, "SendDirector: no notify pipes\n"))
	}
}

static bool not_empty(const char *s) {
	return s && *s;
}

static void CheckEnvironment(ExtensionAPI *host) {
	if (!host)
		return ;
	if (!fdDirector) {
		std::string director = host->Property("ipc.director.name");
		if (director.length() > 0) {
			startedByDirector = true;
			fdDirector = OpenPipe(director.c_str());
			AddSendPipe(fdDirector,NULL);  // we won't remove this pipe!
		}
	}
}

DirectorExtension &DirectorExtension::Instance() {
	static DirectorExtension singleton;
	return singleton;
}

bool DirectorExtension::Initialise(ExtensionAPI *host_) {
	host = host_;
	IF_DEBUG(fdDebug = fopen("/tmp/SciTE.log", "w"))
	CheckEnvironment(host);
	// always try to create the receive (request) pipe, even if not started by
	// an external director
	CreatePipe();
	IF_DEBUG(fprintf(fdDebug, "Initialise: fdReceiver: %d\n", fdReceiver))
	// but do crash out if we failed and were started by an external director.
	if (!fdReceiver && startedByDirector) {
		exit(3);
	}
	return true;
}

bool DirectorExtension::Finalise() noexcept {
	::SendDirector("closing");
	// close and remove all the notification pipes (except ipc.director.name)
	RemoveSendPipes();
	// close our request pipe
	if (fdReceiver != 0) {
		close(fdReceiver);
	}
	// and remove it if we generated it automatically (i.e. not ipc.scite.name)
	if (not_empty(requestPipeName)) {
		remove(requestPipeName);
	}
	IF_DEBUG(fprintf(fdDebug,"finished\n"))
	IF_DEBUG(fclose(fdDebug))

	g_source_remove(inputWatcher);
	inputWatcher = 0;
	g_io_channel_unref(inputChannel);
	inputChannel = 0;

	return true;
}

bool DirectorExtension::Clear() {
	return false;
}

bool DirectorExtension::Load(const char *) {
	return false;
}

bool DirectorExtension::OnOpen(const char *path) {
	CheckEnvironment(host);
	if (not_empty(path)) {
		::SendDirector("opened", path);
	}
	return false;
}

bool DirectorExtension::OnSwitchFile(const char *path) {
	CheckEnvironment(host);
	if (not_empty(path)) {
		::SendDirector("switched", path);
	}
	return false;
}

bool DirectorExtension::OnSave(const char *path) {
	CheckEnvironment(host);
	if (not_empty(path)) {
		::SendDirector("saved", path);
	}
	return false;
}

bool DirectorExtension::OnClose(const char *path) {
	CheckEnvironment(host);
	if (not_empty(path)) {
		::SendDirector("closed", path);
	}
	return false;
}

bool DirectorExtension::NeedsOnClose() {
	CheckEnvironment(host);
	return s_send_cnt > 0;
}

bool DirectorExtension::OnChar(char) {
	return false;
}

bool DirectorExtension::OnExecute(const char *cmd) {
	CheckEnvironment(host);
	::SendDirector("macro:run", cmd);
	return false;
}

bool DirectorExtension::OnSavePointReached() {
	return false;
}

bool DirectorExtension::OnSavePointLeft() {
	return false;
}

bool DirectorExtension::OnStyle(SA::Position, SA::Position, int, StyleWriter *) {
	return false;
}

// These should probably have arguments

bool DirectorExtension::OnDoubleClick() {
	return false;
}

bool DirectorExtension::OnUpdateUI() {
	return false;
}

bool DirectorExtension::OnMarginClick() {
	return false;
}

bool DirectorExtension::OnMacro(const char *command, const char *params) {
	SendDirector(command, params);
	return false;
}

bool DirectorExtension::SendProperty(const char *prop) {
	CheckEnvironment(host);
	if (not_empty(prop)) {
		::SendDirector("property", prop);
	}
	return false;
}

void DirectorExtension::HandleStringMessage(const char *message) {
	static int kount = 1;
	// Message may contain multiple commands separated by '\n'
	StringList  wlMessage(true);
	wlMessage.Set(message);
	IF_DEBUG(fprintf(fdDebug, "HandleStringMessage: got %s\n", message))
	for (size_t i = 0; i < wlMessage.Length(); i++) {
		// Message format is [:return address:]command:argument
		char *cmd = wlMessage[i];
		char *corresp = NULL;
		if (*cmd == ':') { // see same routine in ../win32/DirectorExtension.cxx!
			// there is a return address
			char *colon = strchr(cmd + 1,':');
			if (colon) {
				*colon = '\0';
				corresp = cmd + 1;
				cmd = colon + 1;
			}
		}
		if (isprefix(cmd, "closing:")) {
			fdDirector = 0;
			if (startedByDirector && host) {
				shuttingDown = true;
				host->ShutDown();
				shuttingDown = false;
			}
		} else if (isprefix(cmd, "register:")) {
			// we handle this verb specially - an extension has asked us for a notify
			// pipe which it can listen to.  We make up a unique name based on our
			// pid and a sequence count.
			// There is an (artificial) limit on the number of notify pipes;
			// if there are no more slots, then the returned pipename is '*'
			char pipeName[TMP_FILENAME_LENGTH];
			if (! SendPipeAvailable()) {
				StringCopy(pipeName,"*");
			} else {
				sprintf(pipeName,"%s/SciTE.%d.%d.out", g_get_tmp_dir(), getpid(), kount++);
			}
			if (corresp == NULL) {
				fprintf(stderr,"SciTE Director: bad request\n");
				return;
			} else {
				// the registering client has passed us a path for receiving the notify pipe name.
				// this has to be a _regular_ file, which may not exist.
				fdCorrespondent = open(corresp,O_WRONLY | O_CREAT, S_IRWXU);
				IF_DEBUG(fprintf(fdDebug,"register '%s' %d\n",corresp,fdCorrespondent))
				if (fdCorrespondent == -1) {
					fdCorrespondent = 0;
					fprintf(stderr,"SciTE Director: cannot open result file '%s'\n",corresp);
					return;
				}
				if (fdCorrespondent != 0) {
					size_t size = write(fdCorrespondent, pipeName, strlen(pipeName));
					size += write(fdCorrespondent, "\n", 1);
					if (size == 0)
						fprintf(stderr,"SciTE Director: cannot write pipe name\n");

				}
			}
			if (SendPipeAvailable()) {
				MakePipe(pipeName);
				int fd = OpenPipe(pipeName);
				AddSendPipe(fd, pipeName);
			}
		} else if (host) {
			if (corresp != NULL) {
				// the client has passed us a pipename to receive the results of this command
				fdCorrespondent = OpenPipe(corresp);
				IF_DEBUG(fprintf(fdDebug,"corresp '%s' %d\n",corresp,fdCorrespondent))
				if (fdCorrespondent == -1) {
					fdCorrespondent = 0;
					fprintf(stderr,"SciTE Director: cannot open correspondent pipe '%s'\n",corresp);
					return;
				}
			}
			host->Perform(cmd);
		}
		if (fdCorrespondent != 0) {
			close(fdCorrespondent);
			fdCorrespondent = 0;
		}
	}
}

void DirectorExtension::CreatePipe(bool) {
	if (!host)
		return;
	bool tryStandardPipeCreation;
	std::string pipeName = host->Property("ipc.scite.name");

	fdReceiver = -1;
	inputWatcher = -1;
	inputChannel = 0;
	requestPipeName[0] = '\0';

	// check we have been given a specific pipe name
	if (pipeName.length() > 0) {
		IF_DEBUG(fprintf(fdDebug, "CreatePipe: if (not_empty(pipeName)): '%s'\n", pipeName.c_str()))
		fdReceiver = OpenPipe(pipeName.c_str());
		// there isn't a pipe - so create one
		if (fdReceiver == -1 && errno == ENOENT) {
			IF_DEBUG(fprintf(fdDebug, "CreatePipe: Non found - making\n"))
			if (MakePipe(pipeName.c_str())) {
				fdReceiver = OpenPipe(pipeName.c_str());
				if (fdReceiver == -1) {
					perror("CreatePipe: could not open newly created pipe");
				}
			} else {
				perror("CreatePipe: could not create ipc.scite.name");
			}
			// We don't need a new pipe, we're supposed to have one
			tryStandardPipeCreation = false;
		} else if (fdReceiver == -1) {
			// there are quite a few errors related to open...
			// maybe the pipe is already owned by another SciTE instance...
			// we'll just try creating a new one
			perror("CreatePipe: opening ipc.scite.name failed");
			tryStandardPipeCreation = true;
		} else {
			// cool - we can open it
			tryStandardPipeCreation = false;
		}
	} else {
		tryStandardPipeCreation = true;
	}

	// We were not given a name or we could'nt open it
	if (tryStandardPipeCreation) {
		sprintf(requestPipeName,"%s/SciTE.%d.in", g_get_tmp_dir(), getpid());
		IF_DEBUG(fprintf(fdDebug, "Creating pipe %s\n", requestPipeName))
		MakePipe(requestPipeName);
		fdReceiver = OpenPipe(requestPipeName);
	}

	// If we were able to open a pipe, listen to it
	if (fdReceiver != -1) {
		// store the inputWatcher so we can remove it.
		inputChannel = g_io_channel_unix_new(fdReceiver);
		inputWatcher = g_io_add_watch(inputChannel, G_IO_IN, (GIOFunc)ReceiverPipeSignal, this);
		//inputWatcher = gdk_input_add(fdReceiver, GDK_INPUT_READ, ReceiverPipeSignal, this);
		// if we were not supplied with an explicit ipc.scite.name, then set this
		// property to be the constructed pipe name.
		if (pipeName.length() > 0) {
			host->SetProperty("ipc.scite.name", requestPipeName);
		}
		return;
	}

	// if we arrive here, we must have failed
	fdReceiver = 0;
}
