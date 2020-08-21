#include "applicationdata.h"

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

#include <QDebug>



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
    // extract the gnuplot binary and help file to call gnuplot as external process
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
}

// **************************************************************************

ApplicationData::ApplicationData(QObject *parent, Extension * pExtension, QQmlApplicationEngine & aEngine)
    : QObject(parent),
      m_aEngine(aEngine),
      m_pExtension(pExtension)
{
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
        QUrl url(fileName);
        if(url.isValid() && url.isLocalFile())
        {
            translatedFileName = url.toLocalFile();
        }
    }
    return translatedFileName;
}

QString ApplicationData::readFileContent(const QString & fileName) const
{
    QString translatedFileName = GetTranslatedFileName(fileName);

    QFile file(translatedFileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString(tr("Error reading ") + fileName);
    }

    QTextStream stream(&file);
    auto text = stream.readAll();

    file.close();

    return text;
}

bool ApplicationData::writeFileContent(const QString & fileName, const QString & content)
{
    QString translatedFileName = GetTranslatedFileName(fileName);

    QFile file(translatedFileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    stream << content;

    file.close();

    return true;
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
