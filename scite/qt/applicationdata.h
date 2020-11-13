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

#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QObject>
#include <QQmlApplicationEngine>

#ifdef Q_OS_WIN
#define LOG_NAME "c:\\tmp\\sciteqt_quick_qdebug.log"
#else
#define LOG_NAME "/sdcard/Texte/sciteqt_quick_qdebug.log"
#endif

#define ASSETS_DIR                  "assets:/files/"
#define FILES_DIR                   "/data/data/org.scintilla.sciteqt/files/"
#define LOCALISATIONS_DIR           "/data/data/org.scintilla.sciteqt/files/localisations/"
#define SCRIPTS_DIR                 "/data/data/org.scintilla.sciteqt/files/scripts/"
#define SDCARD_DIRECTORY            "/sdcard"

#define SCITE_PROPERTIES            "SciTE.properties"
#define SCITE_GLOBAL_PROPERTIES     "SciTEGlobal.properties"
#define SCITE_ABBREV_PROPERTIES     "abbrev.properties"
#define SCITE_SCITE_USER_PROPERTIES "SciTEUser.properties"
#define SCITE_SCITE_DOC_HTML        "SciTEDoc.html"
#define SCITE_LOCALE_DE_PROPERTIES  "locale.de.properties"
#define SCITE_LOCALE_NL_PROPERTIES  "locale.nl.properties"
#define SCITE_LOCALE_FR_PROPERTIES  "locale.fr.properties"
#define SCITE_LOCALE_ES_PROPERTIES  "locale.es.properties"
#define SCITE_LOCALE_IT_PROPERTIES  "locale.it.properties"
#define SCITE_LOCALE_PT_PROPERTIES  "locale.pt_PT.properties"

// **************************************************************************

class SciTEQt;
class Extension;
class ShareUtils;
class StorageAccess;

// class for this app to communicate between C++ and QML
class ApplicationData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filesPath READ getFilesPath)
    Q_PROPERTY(QString homePath READ getHomePath)
    Q_PROPERTY(QString sdCardPath READ getSDCardPath)

    Q_PROPERTY(bool isAppStoreSupported READ isAppStoreSupported NOTIFY isAppStoreSupportedChanged)
    Q_PROPERTY(bool isShareSupported READ isShareSupported NOTIFY isShareSupportedChanged)

public:
    explicit ApplicationData(QObject *parent, ShareUtils * pShareUtils, StorageAccess & aStorageAccess, Extension * pExtension, QQmlApplicationEngine & aEngine);
    ~ApplicationData();

    Q_INVOKABLE QString getNormalizedPath(const QString & path) const;

    Q_INVOKABLE bool hasAccessToSDCardPath() const;
    Q_INVOKABLE bool grantAccessToSDCardPath();

    Q_INVOKABLE QString readFileContent(const QString & fileName) const;
    Q_INVOKABLE bool writeFileContent(const QString & fileName, const QString & content);

    Q_INVOKABLE bool deleteFile(const QString & fileName);

    Q_INVOKABLE QString readLog() const;

    Q_INVOKABLE QStringList getSDCardPaths() const;
    QString getFilesPath() const;
    QString getHomePath() const;
    QString getSDCardPath() const;

    bool isAppStoreSupported() const;
    bool isShareSupported() const;

    static QString simpleReadFileContent(const QString & fileName);
    static bool simpleWriteFileContent(const QString & fileName, const QString & content);

    QQmlApplicationEngine & GetQmlApplicationEngine();
    Extension * GetExtension();

    bool shareSimpleText(const QString & text);
    bool shareText(const QString & tempFileName, const QString & text);

    bool writeAndSendSharedFile(const QString & fileName, const QString & fileExtension, const QString & fileMimeType, std::function<bool(QString)> saveFunc, bool bSendFile = true);
    void removeAllFilesForShare();

    bool loadAndShowFileContent(const QString & sFileName);
    bool loadTextFile(const QString & sFileName, QString & sText);
    bool saveTextFile(const QString & sFileName, const QString & sText);

public slots:
#if defined(Q_OS_ANDROID)
     void sltApplicationStateChanged(Qt::ApplicationState applicationState);
#endif
    void sltFileUrlReceived(const QString & sUrl);
    void sltFileReceivedAndSaved(const QString & sUrl);
    void sltTextReceived(const QString &sContent);
    void sltShareError(int requestCode, const QString & message);
    void sltShareEditDone(int requestCode, const QString & urlTxt);
    void sltShareFinished(int requestCode);
    void sltShareNoAppAvailable(int requestCode);

    void sltErrorText(const QString & msg);

signals:
    void isAppStoreSupportedChanged();
    void isShareSupportedChanged();
    void sendErrorText(const QString & msg);
    void fileLoaded(const QString & sFileUri, const QString & sDecodedFileUri, const QString & sContent, bool bNewCreated, bool bSaveACopyModus);

private:
#if defined(Q_OS_ANDROID)
    QStringList                 m_aSharedFilesList;
#endif

    StorageAccess &             m_aStorageAccess;
    ShareUtils *                m_pShareUtils;      // not an owner !

    QQmlApplicationEngine &     m_aEngine;
    Extension *                 m_pExtension;   // not an owner !
};

void UnpackFiles();

#endif // APPLICATIONDATA_H
