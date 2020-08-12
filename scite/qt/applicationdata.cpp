#include "applicationdata.h"

#include "sciteqt.h"

#include <QDir>
#include <QUrl>
#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QTextStream>
#include <QQmlApplicationEngine>

#include <QDebug>

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
