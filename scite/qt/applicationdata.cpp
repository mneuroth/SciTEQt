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

#include "applicationdata.h"
#include "storageaccess.h"

#include "sciteqt.h"

#include <QDir>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QBuffer>
#include <QByteArray>
#include <QTextStream>
#include <QQmlApplicationEngine>

#if defined(Q_OS_ANDROID)
#include <QtAndroid>
#include <QtAndroidExtras>
#include "android/androidshareutils.hpp"
#endif

bool extractAssetFile(const QString & sAssetFileName, const QString & sOutputFileName, bool bExecuteFlags, QDateTime * pDateForReplace = 0)
{
    bool bForce = false;

    if( pDateForReplace!=0 )
    {
        if( QFile::exists(sOutputFileName) )
        {
            QFileInfo aOutputFile(sOutputFileName);

            // force replace of file if last modification date of existing file is older than the given date
            bForce = aOutputFile.lastModified() < *pDateForReplace;
        }
    }
    if( bForce || !QFile::exists(sOutputFileName) )
    {
        QFile aFile(sAssetFileName);
        if( aFile.open(QIODevice::ReadOnly) )
        {
            QByteArray aContent = aFile.readAll();
            aFile.close();

            QFileInfo aFileInfo(sOutputFileName);
            QString sPath = aFileInfo.absoluteDir().absolutePath();
            QDir aDir(sPath);
            aDir.mkpath(sPath);

            QFile aFileOut(sOutputFileName);
            aFileOut.open(QIODevice::WriteOnly);
            aFileOut.write(aContent);
            if( bExecuteFlags )
            {
                aFileOut.setPermissions(QFile::ExeGroup|QFile::ExeOther|QFile::ExeOwner|QFile::ExeUser|aFileOut.permissions());
            }
            aFileOut.close();

            return true;
        }
        return false;
    }
    return true;    // file already existed !
}

void UnpackFiles()
{
#ifdef Q_OS_ANDROID
    // extract the sciteqt properties and help file
    QString sAsset,sOutput;
    //QString sCpuArchitecture(QSysInfo::buildCpuArchitecture());
    sAsset = QString(ASSETS_DIR)+QString(SCITE_PROPERTIES);
    sOutput = QString(FILES_DIR)+QString(SCITE_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_GLOBAL_PROPERTIES);
    sOutput = QString(FILES_DIR)+QString(SCITE_GLOBAL_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_ABBREV_PROPERTIES);
    sOutput = QString(FILES_DIR)+QString(SCITE_ABBREV_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_SCITE_USER_PROPERTIES);
    sOutput = QString(FILES_DIR)+QString(SCITE_SCITE_USER_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_SCITE_DOC_HTML);
    sOutput = QString(FILES_DIR)+QString(SCITE_SCITE_DOC_HTML);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_DE_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_DE_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_NL_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_NL_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_FR_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_FR_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_ES_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_ES_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_PT_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_PT_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_IT_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_IT_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_RU_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_RU_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_JA_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_JA_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_KO_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_KO_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_ZH_T_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_ZH_T_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_AR_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_AR_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_PL_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_PL_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_CS_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_CS_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_DA_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_DA_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_TR_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_TR_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_ID_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_ID_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_EL_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_EL_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_FI_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_FI_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_NB_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_NB_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_HU_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_HU_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_SV_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_SV_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_SL_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_SL_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_RO_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_RO_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_BG_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_BG_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_TH_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_TH_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_UK_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_UK_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_ET_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_ET_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_SR_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_SR_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString(SCITE_LOCALE_MS_PROPERTIES);
    sOutput = QString(LOCALISATIONS_DIR)+QString(SCITE_LOCALE_MS_PROPERTIES);
    extractAssetFile(sAsset,sOutput,false);
/* --> use direct from asset
    sAsset = QString(ASSETS_DIR)+QString("qt_de.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_de.qm");
    extractAssetFile(sAsset,sOutput,false);
// nl missing
    sAsset = QString(ASSETS_DIR)+QString("qt_fr.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_fr.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_es.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_es.qm");
    extractAssetFile(sAsset,sOutput,false);
// pd missing
    sAsset = QString(ASSETS_DIR)+QString("qt_it.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_it.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_ru.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_ru.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_ja.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_ja.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_ko.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_ko.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_ja.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_zh_tw.qm");
    extractAssetFile(sAsset,sOutput,false);
    sAsset = QString(ASSETS_DIR)+QString("qt_ja.qm");
    sOutput = QString(QT_TRANSLATIONS_DIR)+QString("qt_ar.qm");
    extractAssetFile(sAsset,sOutput,false);
*/
#endif
}

void UnpackFilesWasm()
{
#if defined(Q_OS_WASM)
    QDir aDir("/");
    aDir.mkpath("/localisations");
    //QFile::copy(":/SciTEGlobal.properties", "/SciTEGlobal.properties");
    //QFile::copy(":/SciTEGlobal.properties", "/home/web_user/SciTEGlobal.properties");
    //QFile::copy(":/SciTEUser.properties", "/SciTEUser.properties");
    //QFile::copy(":/SciTEUser.properties", "/home/web_user/SciTEUser.properties");
    QFile::copy(":/locale.de.properties", "/localisations/locale.de.properties");
    QFile::copy(":/locale.nl.properties", "/localisations/locale.nl.properties");
    QFile::copy(":/locale.fr.properties", "/localisations/locale.fr.properties");
    QFile::copy(":/locale.es.properties", "/localisations/locale.es.properties");
    QFile::copy(":/locale.it.properties", "/localisations/locale.it.properties");
    QFile::copy(":/locale.ru.properties", "/localisations/locale.ru.properties");
    QFile::copy(":/locale.ja.properties", "/localisations/locale.ja.properties");
    QFile::copy(":/locale.ar.properties", "/localisations/locale.ar.properties");
    QFile::copy(":/locale.ko_KR.properties", "/localisations/locale.ko_KR.properties");
    QFile::copy(":/locale.pt_PT.properties", "/localisations/locale.pt_PT.properties");
    QFile::copy(":/locale.zh_t.properties", "/localisations/locale.zh_t.properties");
    QFile::copy(":/locale.pl.properties", "/localisations/locale.pl.properties");
    QFile::copy(":/locale.cz.properties", "/localisations/locale.cz.properties");
    QFile::copy(":/locale.da.properties", "/localisations/locale.da.properties");
    QFile::copy(":/locale.tr.properties", "/localisations/locale.tr.properties");
    QFile::copy(":/locale.id.properties", "/localisations/locale.id.properties");
    QFile::copy(":/locale.el.properties", "/localisations/locale.el.properties");
    QFile::copy(":/locale.fi.properties", "/localisations/locale.fi.properties");
    QFile::copy(":/locale.nb.properties", "/localisations/locale.nb.properties");
    QFile::copy(":/locale.hu.properties", "/localisations/locale.hu.properties");
    QFile::copy(":/locale.sv.properties", "/localisations/locale.sv.properties");
    QFile::copy(":/locale.sl.properties", "/localisations/locale.sl.properties");
    QFile::copy(":/locale.ro.properties", "/localisations/locale.ro.properties");
    QFile::copy(":/locale.bg.properties", "/localisations/locale.bg.properties");
    QFile::copy(":/locale.th.properties", "/localisations/locale.th.properties");
    QFile::copy(":/locale.uk.properties", "/localisations/locale.uk.properties");
    QFile::copy(":/locale.et.properties", "/localisations/locale.et.properties");
    QFile::copy(":/locale.sr.properties", "/localisations/locale.sr.properties");
    QFile::copy(":/locale.ms.properties", "/localisations/locale.ms.properties");
#endif
}

bool HasAccessToSDCardPath()
{
#if defined(Q_OS_ANDROID)
    QtAndroid::PermissionResult result = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
    return result == QtAndroid::PermissionResult::Granted;
#else
    return true;
#endif
}

bool GrantAccessToSDCardPath()
{
#if defined(Q_OS_ANDROID)
    QStringList permissions;
    permissions.append("android.permission.WRITE_EXTERNAL_STORAGE");
    QtAndroid::PermissionResultMap result = QtAndroid::requestPermissionsSync(permissions);
    if( result.count()!=1 && result["android.permission.WRITE_EXTERNAL_STORAGE"]!=QtAndroid::PermissionResult::Granted )
    {
        return false;
    }
#else
#endif
    return true;
}

// **************************************************************************

ApplicationData::ApplicationData(QObject *parent, ShareUtils * pShareUtils, StorageAccess & aStorageAccess, Extension * pExtension, QQmlApplicationEngine & aEngine)
    : QObject(parent),
      m_aStorageAccess(aStorageAccess),
      m_pShareUtils(pShareUtils),
      m_aEngine(aEngine),
      m_pExtension(pExtension)
{
#if defined(Q_OS_ANDROID)
    QMetaObject::Connection result;
    result = connect(m_pShareUtils, SIGNAL(fileUrlReceived(QString)), this, SLOT(sltFileUrlReceived(QString)));
    result = connect(m_pShareUtils, SIGNAL(fileReceivedAndSaved(QString)), this, SLOT(sltFileReceivedAndSaved(QString)));
    result = connect(m_pShareUtils, SIGNAL(textReceived(QString)), this, SLOT(sltTextReceived(QString)));
    result = connect(m_pShareUtils, SIGNAL(shareError(int, QString)), this, SLOT(sltShareError(int, QString)));
    connect(m_pShareUtils, SIGNAL(shareFinished(int)), this, SLOT(sltShareFinished(int)));
    connect(m_pShareUtils, SIGNAL(shareEditDone(int, QString)), this, SLOT(sltShareEditDone(int, QString)));
    connect(m_pShareUtils, SIGNAL(shareNoAppAvailable(int)), this, SLOT(sltShareNoAppAvailable(int)));
#endif
}

ApplicationData::~ApplicationData()
{
}

bool IsAndroidStorageFileUrl(const QString & url)
{
    return url.startsWith("content:/");
}

QString GetTranslatedFileName(const QString & fileName)
{
    QString translatedFileName = fileName;
    if( IsAndroidStorageFileUrl(fileName) )
    {
        // handle android storage urls --> forward content://... to QFile directly
        translatedFileName = fileName;
    }
    else
    {
        if(!fileName.startsWith(":"))       // files with : should be resolved in the resources ...
        {
            QUrl url(fileName);
            if(url.isValid() && url.isLocalFile())
            {
                translatedFileName = url.toLocalFile();
            }
        }
    }
    return translatedFileName;
}

QString ApplicationData::getNormalizedPath(const QString & path) const
{
    QDir aInfo(path);
    return aInfo.canonicalPath();
}


QString ApplicationData::simpleReadFileContent(const QString & fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString(tr("Error reading ") + fileName);
    }

    QTextStream stream(&file);
    auto text = stream.readAll();

    file.close();

    return text;
}

bool ApplicationData::simpleWriteFileContent(const QString & fileName, const QString & content)
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    stream << content;

    file.close();

    return true;
}

QString ApplicationData::readFileContent(const QString & fileName) const
{
    QString translatedFileName = GetTranslatedFileName(fileName);

    if( IsAndroidStorageFileUrl(translatedFileName) )
    {
        QByteArray data;
        bool ok = m_aStorageAccess.readFile(translatedFileName, data);
        if( ok )
        {
            return QString(data);
        }
        else
        {
            return QString(tr("Error reading ") + fileName);
        }
    }
    else
    {
        return simpleReadFileContent(translatedFileName);
    }
}

bool ApplicationData::writeFileContent(const QString & fileName, const QString & content)
{
    QString translatedFileName = GetTranslatedFileName(fileName);

    if( IsAndroidStorageFileUrl(translatedFileName) )
    {
        bool ok = m_aStorageAccess.updateFile(translatedFileName, content.toUtf8());
        return ok;
    }
    else
    {
        return simpleWriteFileContent(translatedFileName, content);
    }
}

bool ApplicationData::deleteFile(const QString & fileName)
{
    QFile aDir(fileName);
    bool ok = aDir.remove();
    return ok;
}

QString ApplicationData::readLog() const
{
    return readFileContent(LOG_NAME);
}

QQmlApplicationEngine & ApplicationData::GetQmlApplicationEngine()
{
    return m_aEngine;
}

Extension * ApplicationData::GetExtension()
{
    return m_pExtension;
}

QString ApplicationData::getFilesPath() const
{
#if defined(Q_OS_ANDROID)
    return FILES_DIR;
#else
    return ".";
#endif
}

QString ApplicationData::getHomePath() const
{
#if defined(Q_OS_ANDROID)
    return FILES_DIR;
#else
    return ".";
#endif
}

QString ApplicationData::getSDCardPath() const
{
#if defined(Q_OS_ANDROID)
// TODO: list of sdcards returning: internal & external SD Card
    return "/sdcard"; // FILES_DIR;
#elif defined(Q_OS_WIN)
    return "d:\\";
#else
    return "/sdcard";
#endif
}

static QString GetSDCardPathOrg()
{
#if defined(Q_OS_ANDROID)
    // TODO --> this code does not work for sd cards on Android 6.0 and above, waiting for Qt 5.8 or 5.9
    // see: https://qt-project.org/forums/viewthread/35519
    QAndroidJniObject aMediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
        // maybe better: getExternalFilesDir(s)() or getExternalCacheDirs()
    QAndroidJniObject aMediaPath = aMediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    QString sSdCardAbsPath = aMediaPath.toString();
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck())
    {
        // Handle exception here.
        env->ExceptionClear();
        sSdCardAbsPath = SDCARD_DIRECTORY;
    }
    // other option may be: getenv("EXTERNAL_STORAGE")
    return sSdCardAbsPath;
#else
    return SDCARD_DIRECTORY;
#endif
}

#if defined(Q_OS_ANDROID)
static inline QString GetAbsolutePath(const QAndroidJniObject &file)
{
    QAndroidJniObject path = file.callObjectMethod("getAbsolutePath",
                                                   "()Ljava/lang/String;");
    if (!path.isValid())
        return QString();

    return path.toString();
}
#endif

static QStringList GetOriginalExternalFilesDirs(/*const char *directoryField = 0*/)
{
    QStringList result;

#if defined(Q_OS_ANDROID)
    QAndroidJniObject appCtx = QtAndroid::androidContext();
    if (!appCtx.isValid())
        return QStringList();

    QAndroidJniObject dirField = QAndroidJniObject::fromString(QLatin1String(""));
/*
    if (directoryField) {
        dirField = QJNIObjectPrivate::getStaticObjectField("android/os/Environment",
                                                           directoryField,
                                                           "Ljava/lang/String;");
        if (!dirField.isValid())
            return QStringList();
    }
*/
    QAndroidJniObject files = appCtx.callObjectMethod("getExternalFilesDirs",
                                                     "(Ljava/lang/String;)[Ljava/io/File;",
                                                     dirField.object());

    if (!files.isValid())
        return QStringList();

    QAndroidJniEnvironment env;

    // Converts the QAndroidJniObject into a jobjectArray
    jobjectArray arr = files.object<jobjectArray>();
    int size = env->GetArrayLength(arr);

    /* Loop that converts all the elements in the jobjectArray
     * into QStrings and puts them in a QStringList*/
    for (int i = 0; i < size; i++)
    {
        jobject file = env->GetObjectArrayElement(arr, i);

        QAndroidJniObject afile(file);
        result.append(GetAbsolutePath(afile));

        env->DeleteLocalRef(file);
    }

    //env->DeleteLocalRef(arr);
#else
    // nothing to do...
#endif

    return result;
}

static QString RemoveAppPath(const QString & item)
{
    int iFound = item.indexOf("/Android/data");
    if(iFound>=0)
    {
        return item.left(iFound);
    }
    return item;
}

static QStringList GetExternalFilesDirs(/*const char *directoryField = 0*/)
{
    QStringList result = GetOriginalExternalFilesDirs();
    QStringList newResult;

    foreach(const QString & item, result)
    {
        newResult.append(RemoveAppPath(item));
    }

    return newResult;
}

QStringList ApplicationData::getSDCardPaths() const
{
    QStringList allPaths;
    allPaths.append(SDCARD_DIRECTORY);
    allPaths.append(GetExternalFilesDirs());
#if defined(Q_OS_WIN)
    // for testing...
    allPaths.append("c:\\tmp");
#endif
    QString path = GetSDCardPathOrg();
    if(!allPaths.contains(path))
    {
        allPaths.append(path);
    }
    return allPaths;
}

bool ApplicationData::isAppStoreSupported() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

bool ApplicationData::isShareSupported() const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

bool ApplicationData::hasAccessToSDCardPath() const
{
    return ::HasAccessToSDCardPath();
}

bool ApplicationData::grantAccessToSDCardPath()
{
    return ::GrantAccessToSDCardPath();
}

#if defined(Q_OS_ANDROID)
void ApplicationData::sltApplicationStateChanged(Qt::ApplicationState applicationState)
{
    if( applicationState == Qt::ApplicationState::ApplicationSuspended )
    {
/* TODO
        QObject* homePage = childObject<QObject*>(m_aEngine, "homePage", "");
        if( homePage!=0 )
        {
            QMetaObject::invokeMethod(homePage, "checkForModified", QGenericReturnArgument());
        }
*/
    }
}
#endif

bool ApplicationData::writeAndSendSharedFile(const QString & fileName, const QString & fileExtension, const QString & fileMimeType, std::function<bool(QString)> saveFunc, bool bSendFile)
{
#if defined(Q_OS_ANDROID)
    QString fileNameIn = fileName;
    if(fileNameIn.isNull() || fileNameIn.isEmpty())
    {
        fileNameIn = "default.txt";
    }
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    QString targetPath = paths[0]+QDir::separator()+"temp_shared_files";
    QString targetPathX = paths[0]+QDir::separator()+"sciteqt_shared_files";
    QString tempTarget = targetPath+QDir::separator()+fileNameIn+fileExtension;
    QString tempTargetX = targetPathX+QDir::separator()+fileNameIn+fileExtension;
    if( !QDir(targetPath).exists() )
    {
        if( !QDir("").mkpath(targetPath) )
        {
            return false;
        }
    }
    QFile::remove(tempTarget);
    // write temporary file with current script content
    if( !saveFunc(tempTarget) )
    {
        return false;
    }

    QFile::remove(tempTargetX);
    if( !QFile::copy(tempTarget, tempTargetX) )
    {
        return false;
    }

    m_aSharedFilesList.append(tempTarget);
    m_aSharedFilesList.append(tempTargetX);

    /*bool permissionsSet =*/ QFile(tempTargetX).setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
    int requestId = 24;
    bool altImpl = false;
    if( bSendFile )
    {
        m_pShareUtils->sendFile(tempTargetX, tr("Send file"), fileMimeType, requestId, altImpl);
    }
    else
    {
        m_pShareUtils->viewFile(tempTargetX, tr("View file"), fileMimeType, requestId, altImpl);
    }

    // remark: remove temporary files in slot:  sltShareFinished() / sltShareError()
    return true;
#else
    Q_UNUSED(fileName)
    Q_UNUSED(fileExtension)
    Q_UNUSED(fileMimeType)
    Q_UNUSED(saveFunc)
    Q_UNUSED(bSendFile)
    return false;
#endif
}

bool ApplicationData::shareSimpleText(const QString & text)
{
#if defined(Q_OS_ANDROID)
	if( m_pShareUtils != 0 )
    {
        m_pShareUtils->share(text, QUrl());
        return true;
    }
#else
    Q_UNUSED(text);
#endif
    return false;
}

bool ApplicationData::shareText(const QString & tempFileName, const QString & text)
{
    return writeAndSendSharedFile(tempFileName, "", "text/plain", [this, text](QString name) -> bool { return this->saveTextFile(name, text); });
}

void ApplicationData::removeAllFilesForShare()
{
#if defined(Q_OS_ANDROID)
    // remove temporary copied file for sendFile ==> maybe erase the whole directory for data exchange ?
    foreach(const QString & name, m_aSharedFilesList)
    {
        QFile::remove(name);
    }

    m_aSharedFilesList.clear();
#endif
}

bool ApplicationData::loadAndShowFileContent(const QString & sFileName)
{
    bool ok = false;

    if( !sFileName.isEmpty() )
    {
        // load script and show it
        QFileInfo aFileInfo(sFileName);
        QString sScript;
        ok = loadTextFile(sFileName, sScript);
        if( !ok )
        {
            sltErrorText(tr("Can not load file %1").arg(sFileName));
        }
        else
        {
            emit fileLoaded(sFileName, sFileName, sScript, false, false);
        }
    }
    else
    {
        sltErrorText(tr("File name is empty!"));
    }

    return ok;
}

bool ApplicationData::saveTextFile(const QString & sFileName, const QString & sText)
{
    bool ok = false;
    QFile aFile(sFileName.toUtf8());            // workaround
    if( aFile.open(QIODevice::WriteOnly) )
    {
        qint64 iCount = aFile.write(sText.toUtf8());    // write text as utf8 encoded text
        aFile.close();
        ok = iCount>=0;
    }
    if( !ok )
    {
        sltErrorText(tr("Error writing file: ")+sFileName);
    }
    return ok;
}

bool ApplicationData::loadTextFile(const QString & sFileName, QString & sText)
{
    // TODO Maybe: hier muss ggf. der Zugriff auf die Datei sichergestellt werden !!! --> Intent.FLAG_GRANT_READ_URI_PERMISSION
    // WORKAROUND: einmalig auf SD-Karten-Speicher / Externen-Speicher zugreifen
    bool ok = false;
    QFile aFile(sFileName);
    if( aFile.open(QIODevice::ReadOnly) )
    {
        QByteArray aContent = aFile.readAll();
        aFile.close();
        sText = QString::fromUtf8(aContent);    // interpret file as utf8 encoded text
        ok = sText.length()>0;
    }
    return ok;
}

void ApplicationData::sltFileUrlReceived(const QString & sUrl)
{
    // <== share from file manager
    // --> /storage/0000-0000/Texte/xyz.txt     --> SD-Card
    // --> /storage/emulated/0/Texte/xyz.txt    --> internal Memory

    // output:
    // /data/user/0/org.scintilla.sciteqt/files
    // /storage/emulated/0/Android/data/org.scintilla.sciteqt/files

    /*bool ok =*/ loadAndShowFileContent(sUrl);
}

void ApplicationData::sltFileReceivedAndSaved(const QString & sUrl)
{
    // <== share from google documents
    // --> /data/user/0/org.scintilla.sciteqt/files/sciteqt_shared_files/Test.txt.txt

    /*bool ok =*/ loadAndShowFileContent(sUrl);
}

void ApplicationData::sltTextReceived(const QString &sContent)
{
    QString sTempFileName = "./shared_text.txt";
    emit fileLoaded(sTempFileName, sTempFileName, sContent, false, false);
}

void ApplicationData::sltShareError(int requestCode, const QString & message)
{
    Q_UNUSED(requestCode);

    sltErrorText("Error sharing: received "+message);

    removeAllFilesForShare();
}

void ApplicationData::sltShareNoAppAvailable(int requestCode)
{
    Q_UNUSED(requestCode);

    sltErrorText("Error sharing: no app available");

    removeAllFilesForShare();
}

void ApplicationData::sltShareEditDone(int requestCode, const QString & urlTxt)
{
    Q_UNUSED(requestCode);
    Q_UNUSED(urlTxt);

    removeAllFilesForShare();
}

void ApplicationData::sltShareFinished(int requestCode)
{
    Q_UNUSED(requestCode);

    removeAllFilesForShare();
}

void ApplicationData::sltErrorText(const QString & msg)
{
    emit sendErrorText(msg);
}
