// (c) 2017 Ekkehard Gentz (ekke) @ekkescorner
// my blog about Qt for mobile: http://j.mp/qt-x
// see also /COPYRIGHT and /LICENSE

/*
 * Code taken from MobileGnuplotViewer(Quick) project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

#include "androidshareutils.hpp"

#include <QUrl>
#include <QFileInfo>
#include <QDateTime>

#include <QtAndroidExtras/QAndroidJniObject>
#include <jni.h>

const static int RESULT_OK = -1;
const static int RESULT_CANCELED = 0;

AndroidShareUtils* AndroidShareUtils::mInstance = NULL;

AndroidShareUtils::AndroidShareUtils(QObject* parent) : PlatformShareUtils(parent)
{
    // we need the instance for JNI Call
    mInstance = this;
}

AndroidShareUtils* AndroidShareUtils::getInstance()
{
    if (!mInstance) {
        mInstance = new AndroidShareUtils;
    }

    return mInstance;
}

bool AndroidShareUtils::isSciteQtInstalled()
{
    jboolean value = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                              "isSciteQtInstalled",
                                              "()Z");
    return value;
}

bool AndroidShareUtils::isAppInstalled(const QString &packageName)
{
    QAndroidJniObject jsPackageName = QAndroidJniObject::fromString(packageName);
    jboolean value = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                              "isAppInstalled",
                                              "(Ljava/lang/String;)Z",
                                              jsPackageName.object<jstring>());
    return value;
}

bool AndroidShareUtils::checkMimeTypeView(const QString &mimeType)
{
    QAndroidJniObject jsMime = QAndroidJniObject::fromString(mimeType);
    jboolean verified = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                              "checkMimeTypeView",
                                              "(Ljava/lang/String;)Z",
                                              jsMime.object<jstring>());
    return verified;
}

bool AndroidShareUtils::checkMimeTypeEdit(const QString &mimeType)
{
    QAndroidJniObject jsMime = QAndroidJniObject::fromString(mimeType);
    jboolean verified = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                                  "checkMimeTypeEdit",
                                                  "(Ljava/lang/String;)Z",
                                                  jsMime.object<jstring>());
    return verified;
}

void AndroidShareUtils::share(const QString &text, const QUrl &url)
{
    QAndroidJniObject jsText = QAndroidJniObject::fromString(text);
    QAndroidJniObject jsUrl = QAndroidJniObject::fromString(url.toString());
    jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                              "share",
                                              "(Ljava/lang/String;Ljava/lang/String;)Z",
                                              jsText.object<jstring>(), jsUrl.object<jstring>());

    if(!ok) {
        emit shareNoAppAvailable(0);
    }
}

/*
 * As default we're going the Java - way with one simple JNI call (recommended)
 * if altImpl is true we're going the pure JNI way
 *
 * If a requestId was set we want to get the Activity Result back (recommended)
 * We need the Request Id and Result Id to control our workflow
*/
void AndroidShareUtils::sendFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl)
{
    mIsEditMode = false;

    if(!altImpl) {
        QAndroidJniObject jsPath = QAndroidJniObject::fromString(filePath);
        QAndroidJniObject jsTitle = QAndroidJniObject::fromString(title);
        QAndroidJniObject jsMimeType = QAndroidJniObject::fromString(mimeType);
        jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                                  "sendFile",
                                                  "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Z",
                                                  jsPath.object<jstring>(), jsTitle.object<jstring>(), jsMimeType.object<jstring>(), requestId);
        if(!ok) {
            emit shareNoAppAvailable(requestId);
        }
        return;
    }

    // THE FILE PATH
    // to get a valid Path we must prefix file://
    // attention file must be inside Users Documents folder !
    // trying to send a file from APP DATA will fail
    QAndroidJniObject jniPath = QAndroidJniObject::fromString("file://"+filePath);
    if(!jniPath.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nFilePath not valid"));
        return;
    }
    // next step: convert filePath Java String into Java Uri
    QAndroidJniObject jniUri = QAndroidJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jniPath.object<jstring>());
    if(!jniUri.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nURI not valid"));
        return;
    }

    // THE INTENT ACTION
    // create a Java String for the ACTION
    QAndroidJniObject jniAction = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_SEND");
    if(!jniAction.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }
    // then create the Intent Object for this Action
    QAndroidJniObject jniIntent("android/content/Intent","(Ljava/lang/String;)V",jniAction.object<jstring>());
    if(!jniIntent.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    // THE MIME TYPE
    if(mimeType.isEmpty()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType is empty"));
        return;
    }
    // create a Java String for the File Type (Mime Type)
    QAndroidJniObject jniMimeType = QAndroidJniObject::fromString(mimeType);
    if(!jniMimeType.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType not valid"));
        return;
    }
    // set Type (MimeType)
    QAndroidJniObject jniType = jniIntent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", jniMimeType.object<jstring>());
    if(!jniType.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    // THE EXTRA STREAM
    // create a Java String for the EXTRA
    QAndroidJniObject jniExtra = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "EXTRA_STREAM");
    if(!jniExtra.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }
    // put Extra (EXTRA_STREAM and URI)
    QAndroidJniObject jniExtraStreamUri = jniIntent.callObjectMethod("putExtra", "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;", jniExtra.object<jstring>(), jniUri.object<jobject>());
    //QAndroidJniObject jniExtraStreamUri = jniIntent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", jniExtra.object<jstring>(), jniExtra.object<jstring>());
    if(!jniExtraStreamUri.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject packageManager = activity.callObjectMethod("getPackageManager",
                                                                 "()Landroid/content/pm/PackageManager;");
    QAndroidJniObject componentName = jniIntent.callObjectMethod("resolveActivity",
                                                              "(Landroid/content/pm/PackageManager;)Landroid/content/ComponentName;",
                                                              packageManager.object());
    if (!componentName.isValid()) {
        emit shareNoAppAvailable(requestId);
        return;
    }

    if(requestId <= 0) {
        // we dont need a result if there's no requestId
        QtAndroid::startActivity(jniIntent, requestId);
    } else {
        // we have the JNI Object, know the requestId
        // and want the Result back into 'this' handleActivityResult(...)
        // attention: to test JNI with QAndroidActivityResultReceiver you must comment or rename
        // onActivityResult()  method in QShareActivity.java - otherwise you'll get wrong request or result codes
        QtAndroid::startActivity(jniIntent, requestId, this);
    }
}

/*
 * As default we're going the Java - way with one simple JNI call (recommended)
 * if altImpl is true we're going the pure JNI way
 *
 * If a requestId was set we want to get the Activity Result back (recommended)
 * We need the Request Id and Result Id to control our workflow
*/
void AndroidShareUtils::viewFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl)
{
    mIsEditMode = false;

    if(!altImpl) {
        QAndroidJniObject jsPath = QAndroidJniObject::fromString(filePath);
        QAndroidJniObject jsTitle = QAndroidJniObject::fromString(title);
        QAndroidJniObject jsMimeType = QAndroidJniObject::fromString(mimeType);
        jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                                  "viewFile",
                                                  "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Z",
                                                  jsPath.object<jstring>(), jsTitle.object<jstring>(), jsMimeType.object<jstring>(), requestId);
        if(!ok) {
            emit shareNoAppAvailable(requestId);
        }
        return;
    }

    // THE FILE PATH
    // to get a valid Path we must prefix file://
    // attention file must be inside Users Documents folder !
    // trying to view or edit a file from APP DATA will fail
    QAndroidJniObject jniPath = QAndroidJniObject::fromString("file://"+filePath);
    if(!jniPath.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nFilePath not valid"));
        return;
    }
    // next step: convert filePath Java String into Java Uri
    QAndroidJniObject jniUri = QAndroidJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jniPath.object<jstring>());
    if(!jniUri.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nURI not valid"));
        return;
    }

    // THE INTENT ACTION
    // create a Java String for the ACTION
    QAndroidJniObject jniParam = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_VIEW");
    if(!jniParam.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }
    // then create the Intent Object for this Action
    QAndroidJniObject jniIntent("android/content/Intent","(Ljava/lang/String;)V",jniParam.object<jstring>());
    if(!jniIntent.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    // THE FILE TYPE
    if(mimeType.isEmpty()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType is empty"));
        return;
    }
    // create a Java String for the File Type (Mime Type)
    QAndroidJniObject jniType = QAndroidJniObject::fromString(mimeType);
    if(!jniType.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType not valid"));
        return;
    }
    // set Data (the URI) and Type (MimeType)
    QAndroidJniObject jniResult = jniIntent.callObjectMethod("setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;", jniUri.object<jobject>(), jniType.object<jstring>());
    if(!jniResult.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject packageManager = activity.callObjectMethod("getPackageManager",
                                                                 "()Landroid/content/pm/PackageManager;");
    QAndroidJniObject componentName = jniIntent.callObjectMethod("resolveActivity",
                                                              "(Landroid/content/pm/PackageManager;)Landroid/content/ComponentName;",
                                                              packageManager.object());
    if (!componentName.isValid()) {
        emit shareNoAppAvailable(requestId);
        return;
    }

    if(requestId <= 0) {
        // we dont need a result if there's no requestId
        QtAndroid::startActivity(jniIntent, requestId);
    } else {
        // we have the JNI Object, know the requestId
        // and want the Result back into 'this' handleActivityResult(...)
        // attention: to test JNI with QAndroidActivityResultReceiver you must comment or rename
        // onActivityResult()  method in QShareActivity.java - otherwise you'll get wrong request or result codes
        QtAndroid::startActivity(jniIntent, requestId, this);
    }
}

/*
 * As default we're going the Java - way with one simple JNI call (recommended)
 * if altImpl is true we're going the pure JNI way
 *
 * If a requestId was set we want to get the Activity Result back (recommended)
 * We need the Request Id and Result Id to control our workflow
*/
void AndroidShareUtils::editFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl)
{
    mIsEditMode = true;
    mCurrentFilePath = filePath;
    QFileInfo fileInfo = QFileInfo(mCurrentFilePath);
    mLastModified = fileInfo.lastModified().toSecsSinceEpoch();

    if(!altImpl) {
        QAndroidJniObject jsPath = QAndroidJniObject::fromString(filePath);
        QAndroidJniObject jsTitle = QAndroidJniObject::fromString(title);
        QAndroidJniObject jsMimeType = QAndroidJniObject::fromString(mimeType);

        jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QShareUtils",
                                                  "editFile",
                                                  "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Z",
                                                  jsPath.object<jstring>(), jsTitle.object<jstring>(), jsMimeType.object<jstring>(), requestId);

        if(!ok) {
            emit shareNoAppAvailable(requestId);
        }
        return;
    }

    // THE FILE PATH
    // to get a valid Path we must prefix file://
    // attention file must be inside Users Documents folder !
    // trying to view or edit a file from APP DATA will fail
    QAndroidJniObject jniPath = QAndroidJniObject::fromString("file://"+filePath);
    if(!jniPath.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nFilePath not valid"));
        return;
    }
    // next step: convert filePath Java String into Java Uri
    QAndroidJniObject jniUri = QAndroidJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jniPath.object<jstring>());
    if(!jniUri.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nURI not valid"));
        return;
    }

    // THE INTENT ACTION
    // create a Java String for the ACTION
    QAndroidJniObject jniParam = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_EDIT");
    if(!jniParam.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }
    // then create the Intent Object for this Action
    QAndroidJniObject jniIntent("android/content/Intent","(Ljava/lang/String;)V",jniParam.object<jstring>());
    if(!jniIntent.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    // THE FILE TYPE
    if(mimeType.isEmpty()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType is empty"));
        return;
    }
    // create a Java String for the File Type (Mime Type)
    QAndroidJniObject jniType = QAndroidJniObject::fromString(mimeType);
    if(!jniType.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured\nMimeType not valid"));
        return;
    }
    // set Data (the URI) and Type (MimeType)
    QAndroidJniObject jniResult = jniIntent.callObjectMethod("setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;", jniUri.object<jobject>(), jniType.object<jstring>());
    if(!jniResult.isValid()) {
        emit shareError(requestId, tr("Share: an Error occured"));
        return;
    }

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject packageManager = activity.callObjectMethod("getPackageManager",
                                                                 "()Landroid/content/pm/PackageManager;");
    QAndroidJniObject componentName = jniIntent.callObjectMethod("resolveActivity",
                                                              "(Landroid/content/pm/PackageManager;)Landroid/content/ComponentName;",
                                                              packageManager.object());
    if (!componentName.isValid()) {
        emit shareNoAppAvailable(requestId);
        return;
    }

    // now all is ready to start the Activity:
    if(requestId <= 0) {
        // we dont need a result if there's no requestId
        QtAndroid::startActivity(jniIntent, requestId);
    } else {
        // we have the JNI Object, know the requestId
        // and want the Result back into 'this' handleActivityResult(...)
        // attention: to test JNI with QAndroidActivityResultReceiver you must comment or rename
        // onActivityResult()  method in QShareActivity.java - otherwise you'll get wrong request or result codes
        QtAndroid::startActivity(jniIntent, requestId, this);
    }
}

// used from QAndroidActivityResultReceiver
void AndroidShareUtils::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data)
{
    Q_UNUSED(data);
    processActivityResult(receiverRequestCode, resultCode, "");
}

// used from Activity.java onActivityResult()
void AndroidShareUtils::onActivityResult(int requestCode, int resultCode, const QString & urlTxt)
{
    processActivityResult(requestCode, resultCode, urlTxt);
}

void AndroidShareUtils::processActivityResult(int requestCode, int resultCode, const QString & urlTxt)
{
    // we're getting RESULT_OK only if edit is done
    if(resultCode == RESULT_OK) {
        emit shareEditDone(requestCode, urlTxt);
    } else if(resultCode == RESULT_CANCELED) {
        if(mIsEditMode) {
            // Attention: not all Apps will give you the correct ResultCode:
            // Google Fotos will send OK if saved and CANCELED if canceled
            // Some Apps always sends CANCELED even if you modified and Saved the File
            // so you should check the modified Timestamp of the File to know if
            // you should emit shareEditDone() or shareFinished() !!!
            QFileInfo fileInfo = QFileInfo(mCurrentFilePath);
            qint64 currentModified = fileInfo.lastModified().toSecsSinceEpoch();
            if(currentModified > mLastModified) {
                emit shareEditDone(requestCode, urlTxt);
                return;
            }
        }
        emit shareFinished(requestCode);
    } else {
        emit shareError(requestCode, tr("Share: an Error occured"));
    }
}

void AndroidShareUtils::checkPendingIntents(const QString workingDirPath)
{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if(activity.isValid()) {
        // create a Java String for the Working Dir Path
        QAndroidJniObject jniWorkingDir = QAndroidJniObject::fromString(workingDirPath);
        if(!jniWorkingDir.isValid()) {
            emit shareError(0, tr("Share: an Error occured\nWorkingDir not valid"));
            return;
        }
        activity.callMethod<void>("checkPendingIntents","(Ljava/lang/String;)V", jniWorkingDir.object<jstring>());
        return;
    }
}

void AndroidShareUtils::setFileUrlReceived(const QString &url)
{
    if(url.isEmpty()) {
        emit shareError(0, tr("Empty URL received"));
        return;
    }
    QString myUrl;
    if(url.startsWith("file://")) {
        myUrl= url.right(url.length()-7);
    } else {
        myUrl= url;
    }

    // check if File exists
    QFileInfo fileInfo = QFileInfo(myUrl);
    if(fileInfo.exists()) {
        emit fileUrlReceived(myUrl);
    } else {
        emit shareError(0, tr("File does not exist: %1").arg(myUrl));
    }
}

void AndroidShareUtils::setFileReceivedAndSaved(const QString &url)
{
    if(url.isEmpty()) {
        emit shareError(0, tr("Empty URL received"));
        return;
    }
    QString myUrl;
    if(url.startsWith("file://")) {
        myUrl= url.right(url.length()-7);
    } else {
        myUrl= url;
    }

    // check if File exists
    QFileInfo fileInfo = QFileInfo(myUrl);
    if(fileInfo.exists()) {
        emit fileReceivedAndSaved(myUrl);
    } else {
        emit shareError(0, tr("File does not exist: %1").arg(myUrl));
    }
}

void AndroidShareUtils::setTextContentReceived(const QString &text)
{
    emit textReceived(text);
}

void AndroidShareUtils::setUnknownContentReceived(const QString &errMsg)
{
    emit shareError(0, errMsg);
}

// to be safe we check if a File Url from java really exists for Qt
// if not on the Java side we'll try to read the content as Stream
bool AndroidShareUtils::checkFileExits(const QString &url)
{
    if(url.isEmpty()) {
        emit shareError(0, tr("Empty URL received"));
        return false;
    }
    QString myUrl;
    if(url.startsWith("file://")) {
        myUrl= url.right(url.length()-7);
    } else {
        myUrl= url;
    }

    // check if File exists
    QFileInfo fileInfo = QFileInfo(myUrl);
    if(fileInfo.exists()) {
        return true;
    } else {
        return false;
    }
}

void AndroidShareUtils::qDebugOutput(const QString &txt)
{
    Q_UNUSED(txt);
    AddToLog("$$$ "+txt);
}

// instead of defining all JNICALL as demonstrated below
// there's another way, making it easier to manage all the methods
// see https://www.kdab.com/qt-android-episode-5/

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_setFileUrlReceived(JNIEnv *env,
                                        jobject obj,
                                        jstring url)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->setFileUrlReceived(urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    return;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_setFileReceivedAndSaved(JNIEnv *env,
                                        jobject obj,
                                        jstring url)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->setFileReceivedAndSaved(urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    return;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_setTextContentReceived(JNIEnv *env,
                                        jobject obj,
                                        jstring text)
{
    const char *textStr = env->GetStringUTFChars(text, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->setTextContentReceived(textStr);
    env->ReleaseStringUTFChars(text, textStr);
    return;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_setUnknownContentReceived(JNIEnv *env,
                                        jobject obj,
                                        jstring errMsg)
{
    const char *errMsgStr = env->GetStringUTFChars(errMsg, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->setUnknownContentReceived(errMsgStr);
    env->ReleaseStringUTFChars(errMsg, errMsgStr);
    return;
}

JNIEXPORT bool JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_checkFileExits(JNIEnv *env,
                                        jobject obj,
                                        jstring url)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    Q_UNUSED (obj)
    bool exists = AndroidShareUtils::getInstance()->checkFileExits(urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    return exists;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_qDebugOutput(JNIEnv *env,
                                        jobject obj,
                                        jstring txt)
{
    const char *txtStr = env->GetStringUTFChars(txt, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->qDebugOutput(txtStr);
    env->ReleaseStringUTFChars(txt, txtStr);
    return;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_fireActivityResult(JNIEnv *env,
                                        jobject obj,
                                        jint requestCode,
                                        jint resultCode,
                                        jstring url)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    Q_UNUSED (obj)
    AndroidShareUtils::getInstance()->onActivityResult(requestCode, resultCode, urlStr);
    env->ReleaseStringUTFChars(url, urlStr);
    return;
}

#ifdef __cplusplus
}
#endif
