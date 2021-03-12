/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
/*
 * Code taken from MobileGnuplotViewer(Quick) project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

#include "storageaccess.h"

//#include <QDebug>

#if defined(Q_OS_ANDROID)
#include <QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include <jni.h>
#endif

const static int RESULT_OK = -1;
const static int RESULT_CANCELED = 0;

StorageAccess * StorageAccess::m_pInstance = 0;

StorageAccess::StorageAccess(QObject *parent) : QObject(parent)
{
    m_pInstance = this;
}

StorageAccess * StorageAccess::getInstance()
{
    if (!m_pInstance)
    {
        m_pInstance = new StorageAccess();
    }

    return m_pInstance;
}

void StorageAccess::onFileOpenActivityResult(int resultCode, const QString & fileUri, const QString & decodedFileUri, const QByteArray & fileContent)
{
    //qDebug() << "*** onFileOpenActivityResult() " << resultCode << " " << fileUri << " decodedUri=" << decodedFileUri << " len=" << fileContent.size() << endl;

    if(resultCode == RESULT_OK)
    {
        emit openFileContentReceived(fileUri, decodedFileUri, fileContent);
    }
    else if(resultCode == RESULT_CANCELED)
    {
        emit openFileCanceled();
    }
    else
    {
        emit openFileError(QString("unexpected result code %1").arg(resultCode));
    }
}

void StorageAccess::onFileCreateActivityResult(int resultCode, const QString & fileUri, const QString & decodedFileUri)
{
    if(resultCode == RESULT_OK)
    {
        emit createFileReceived(fileUri, decodedFileUri);
    }
    else if(resultCode == RESULT_CANCELED)
    {
        emit openFileCanceled();
    }
    else
    {
        emit openFileError(QString("unexpected result code %1").arg(resultCode));
    }
}

void StorageAccess::openFile()
{
#if defined(Q_OS_ANDROID)
    jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QStorageAccess",
                                              "openFile",
                                              "()Z");
    if(!ok)
    {
        emit openFileError(tr("Error: can not call java method openFile()"));
    }
#endif
}

#if defined(Q_OS_ANDROID)
// http://pavelk.ru/qt-android-jni-preobrazovanie-qbytearray-v-jbytearray
jbyteArray QByteArray2jbyteArray(const QByteArray & buf)
{
    QAndroidJniEnvironment env;
    // Bugfix 12.3.2021: copy terminating 0 to buffer
    jbyteArray array = env->NewByteArray(buf.size()+1);
    env->SetByteArrayRegion(array, 0, buf.size()+1, reinterpret_cast<jbyte*>((char *)buf.data()));
    return array;
}

/*
inline QByteArray jbyteArray2QByteArray(jbyteArray buf)
{
    QByteArray array;

    QAndroidJniEnvironment env;
    jclass jclass_of_bytearray = env->FindClass("[B");
    if( env->IsInstanceOf(buf,jclass_of_bytearray) )
    {
        int len = env->GetArrayLength(buf);     // see code: https://android.googlesource.com/platform/art/+/kitkat-dev/runtime/jni_internal.cc
        array.resize(len);
        env->GetByteArrayRegion(buf, 0, len, reinterpret_cast<jbyte*>(array.data()));
    }
    return array;
}
*/
/*
QJNIObjectPrivate QJNIObjectPrivate::fromString(const QString &string)
{
    QJNIEnvironmentPrivate env;
    jstring res = env->NewString(reinterpret_cast<const jchar*>(string.constData()),
                                        string.length());
    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}
*/
#endif

bool StorageAccess::updateFile(const QString & fileUri, const QByteArray & fileContent)
{
#if defined(Q_OS_ANDROID)
    QAndroidJniObject jniFileUri = QAndroidJniObject::fromString(fileUri);
    jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QStorageAccess",
                                              "updateFile",
                                              "(Ljava/lang/String;[B)Z",
                                              jniFileUri.object<jstring>(),
                                              QByteArray2jbyteArray(fileContent));
    return ok;
#else
    Q_UNUSED(fileUri)
    Q_UNUSED(fileContent)
#endif
    return false;
}

bool StorageAccess::deleteFile(const QString & fileUri)
{
#if defined(Q_OS_ANDROID)
    QAndroidJniObject jniFileUri = QAndroidJniObject::fromString(fileUri);
    jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QStorageAccess",
                                              "deleteFile",
                                              "(Ljava/lang/String;)Z",
                                              jniFileUri.object<jstring>());
    return ok;
#else
    Q_UNUSED(fileUri)
#endif
    return false;
}

bool StorageAccess::readFile(const QString & fileUri, QByteArray & fileContent)
{
#if defined(Q_OS_ANDROID)
    QAndroidJniObject jniFileUri = QAndroidJniObject::fromString(fileUri);

    // see: https://forum.qt.io/topic/42497/solved-android-how-to-get-a-byte-array-from-java/4

#ifndef OLD_STORAGE_ACCESS_HANDLING
    QAndroidJniObject contentExt = QAndroidJniObject::callStaticObjectMethod("org/scintilla/utils/QStorageAccess",
                                              "readFileExt",
                                              "(Ljava/lang/String;)Lorg/scintilla/utils/Tuple;",
                                              jniFileUri.object<jstring>());

    // improved robustness of access to storage framework, because of exceptions reported via google play --> check data before using it
    if( contentExt.isValid() )
    {
        jboolean ok = contentExt.callMethod<jboolean>("GetSuccessFlag");
        QAndroidJniObject contentArrayTemp = contentExt.callObjectMethod<jbyteArray>("GetContent");
        jbyteArray contentArray = contentArrayTemp.object<jbyteArray>();

        if( ok )
        {
            QByteArray qContent;
            QAndroidJniEnvironment env;
            jclass jclass_of_bytearray = env->FindClass("[B");
            if( env->IsInstanceOf(contentArray,jclass_of_bytearray) )
            {
                int len = env->GetArrayLength(contentArray);     // see code: https://android.googlesource.com/platform/art/+/kitkat-dev/runtime/jni_internal.cc
                qContent.resize(len);
                env->GetByteArrayRegion(contentArray, 0, len, reinterpret_cast<jbyte*>(qContent.data()));
            }

            fileContent = qContent;

            return ok;
        }

        return false;
    }

    return false;

#else
    QAndroidJniObject content = QAndroidJniObject::callStaticObjectMethod("org/scintilla/utils/QStorageAccess",
                                              "readFile",
                                              "(Ljava/lang/String;)[B",
                                              jniFileUri.object<jstring>());

    if( content.isValid() )
    {
        jbyteArray contentArray = content.object<jbyteArray>();

        //QByteArray qContent = jbyteArray2QByteArray(contentArray);

        // inline the method above
        QByteArray qContent;
        QAndroidJniEnvironment env;
        jclass jclass_of_bytearray = env->FindClass("[B");
        if( env->IsInstanceOf(contentArray,jclass_of_bytearray) )
        {
            int len = env->GetArrayLength(contentArray);     // PROBLEM ! buf==byte[0] ? // Code: https://android.googlesource.com/platform/art/+/kitkat-dev/runtime/jni_internal.cc
            qContent.resize(len);
            env->GetByteArrayRegion(contentArray, 0, len, reinterpret_cast<jbyte*>(qContent.data()));
        }


        fileContent = qContent;
    //TODO working: check if release is needed:       env->ReleaseByteArrayElements(contentArray, icon, JNI_ABORT);
    // WARNING: this makes problems with files of length 0 !!!! --> use new implementation above
        return qContent.length()>0;
    }

    return false;
#endif
#else
    Q_UNUSED(fileUri)
    Q_UNUSED(fileContent)
    return false;
#endif
}

void StorageAccess::createFile(const QString & fileName, const QString & mimeType)
{
#if defined(Q_OS_ANDROID)
    QAndroidJniObject jniFileName = QAndroidJniObject::fromString(fileName);
    QAndroidJniObject jniMimeType = QAndroidJniObject::fromString(mimeType);
    jboolean ok = QAndroidJniObject::callStaticMethod<jboolean>("org/scintilla/utils/QStorageAccess",
                                              "createFile",
                                              "(Ljava/lang/String;Ljava/lang/String;)Z",
                                              jniFileName.object<jstring>(),
                                              jniMimeType.object<jstring>());
    if(!ok)
    {
        emit openFileError(tr("Error: can not call java method createFile()"));
    }
#else
    Q_UNUSED(fileName)
    Q_UNUSED(mimeType)
#endif
}

#if defined(Q_OS_ANDROID)

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_fireFileOpenActivityResult(JNIEnv *env,
                                        jobject obj,
                                        jint resultCode,
                                        jstring url,
                                        jstring decodedUrl,
                                        jbyteArray fileContent)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    const char *urlDecodedStr = env->GetStringUTFChars(decodedUrl, NULL);
    jsize contentSize = env->GetArrayLength(fileContent);
    QByteArray * pArray = 0;
    if( contentSize > 0 )
    {
        jboolean isCopy = false;
        jbyte * content = env->GetByteArrayElements(fileContent, &isCopy);
        pArray = new QByteArray((char *)content, contentSize);
        env->ReleaseByteArrayElements(fileContent, content, JNI_ABORT);
    }
    else
    {
        pArray = new QByteArray();
    }
    Q_UNUSED (obj)
    StorageAccess::getInstance()->onFileOpenActivityResult(resultCode, urlStr, urlDecodedStr, *pArray);
    env->ReleaseStringUTFChars(url, urlStr);
    env->ReleaseStringUTFChars(url, urlDecodedStr);
    delete pArray;
    return;
}

JNIEXPORT void JNICALL
  Java_org_scintilla_activity_sharex_QShareActivity_fireFileCreateActivityResult(JNIEnv *env,
                                        jobject obj,
                                        jint resultCode,
                                        jstring url,
                                        jstring decodedUrl)
{
    const char *urlStr = env->GetStringUTFChars(url, NULL);
    const char *urlDecodedStr = env->GetStringUTFChars(decodedUrl, NULL);
    Q_UNUSED (obj)
    StorageAccess::getInstance()->onFileCreateActivityResult(resultCode, urlStr, urlDecodedStr);
    env->ReleaseStringUTFChars(url, urlStr);
    env->ReleaseStringUTFChars(url, urlDecodedStr);
    return;
}

#ifdef __cplusplus
}
#endif

#endif
