#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QObject>
#include <QQmlApplicationEngine>

#ifdef Q_OS_WIN
#define LOG_NAME "c:\\tmp\\mgv_quick_qdebug.log"
#else
#define LOG_NAME "/sdcard/Texte/sciteqt_quick_qdebug.log"
//#define LOG_NAME "mgv_quick_qdebug.log"
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

    QQmlApplicationEngine & GetQmlApplicationEngine();
    Extension * GetExtension();

public slots:
#if defined(Q_OS_ANDROID)
     void sltApplicationStateChanged(Qt::ApplicationState applicationState);
#endif

signals:
    void isAppStoreSupportedChanged();
    void isShareSupportedChanged();

private:
    StorageAccess &             m_aStorageAccess;
    ShareUtils *                m_pShareUtils;      // not an owner !

    QQmlApplicationEngine &     m_aEngine;
    Extension *                 m_pExtension;   // not an owner !
};

void UnpackFiles();

#endif // APPLICATIONDATA_H
