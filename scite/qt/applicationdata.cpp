#include "applicationdata.h"

#include <QDir>
#include <QUrl>
#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QTextStream>

#include <QDebug>

// see: https://stackoverflow.com/questions/14791360/qt5-syntax-highlighting-in-qml
template <class T> T childObject(QQmlApplicationEngine& engine,
                                 const QString& objectName,
                                 const QString& propertyName,
                                 bool bGetRoot = true)
{
    QList<QObject*> rootObjects = engine.rootObjects();
    foreach (QObject* object, rootObjects)
    {
        QObject* child = object->findChild<QObject*>(objectName);
        if (child != 0)
        {
            if( propertyName.length()==0 )
            {
                if(bGetRoot)
                {
                    return dynamic_cast<T>(object);
                }
                else
                {
                    return dynamic_cast<T>(child);
                }
            }
            else
            {
                std::string s = propertyName.toStdString();
                QObject* object = child->property(s.c_str()).value<QObject*>();
                Q_ASSERT(object != 0);
                T prop = dynamic_cast<T>(object);
                Q_ASSERT(prop != 0);
                return prop;
            }
        }
    }
    return (T) 0;
}


ApplicationData::ApplicationData(QObject *parent, QQmlApplicationEngine & aEngine)
    : QObject(parent),
      m_aEngine(aEngine)
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

void ApplicationData::startFileDialog(const QString & sDirectory, const QString & sFilter, bool bAsOpenDialog)
{
    QObject * appWin = childObject<QObject*>(m_aEngine, "fileDialog", "");
    if( appWin != 0 )
    {
        QMetaObject::invokeMethod(appWin, "startFileDialog",
                QGenericReturnArgument(),
                Q_ARG(QVariant, sDirectory),
                Q_ARG(QVariant, sFilter),
                Q_ARG(QVariant, bAsOpenDialog));
    }
}

QObject * ApplicationData::showInfoDialog(const QString & sInfoText)
{
    QVariant result;
    QObject * appWin = childObject<QObject*>(m_aEngine, "infoDialog", "");
    if( appWin != 0 )
    {
        QMetaObject::invokeMethod(appWin, "showInfoDialog",
                QGenericReturnArgument(),
                Q_ARG(QVariant, sInfoText));
    }
    QObject * infoDlg = childObject<QObject*>(m_aEngine, "infoDialog", "", false);
    return infoDlg;
}
