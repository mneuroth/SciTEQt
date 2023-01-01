#include "qhtmlfileaccess.h"
#include "qhtml5file.h"
#include <Qt>
#include <QtCore/QFile>
#include <QtCore/QDebug>

void QHtmlFileAccess::loadFile(const QString &accept)
{
    QHtml5File::load(accept, [this](const QByteArray &data, const QString &name) {
        emit fileDataReady(data, name);
    });   
}

void QHtmlFileAccess::saveFile(const QByteArray &contents, const QString &fileNameHint)
{
    QHtml5File::save(contents, fileNameHint);
}

Q_INVOKABLE void QHtmlFileAccess::loadFsFile(const QString &accept, const QString &tmpPath)
{
    QHtml5File::load(accept, [this, tmpPath](const QByteArray &data, const QString &name) {
        // The application has requested that the file data should be loaded
        // into the emscripten file system. Save the data and pass the
        // tmp file path and real file anme to the app.
        QString tmpFilePath = tmpPath + QString("/") + name;
        QFile tmpFile(tmpFilePath);
        tmpFile.open(QIODevice::WriteOnly);
        tmpFile.write(data);
        tmpFile.close();
        emit fsFileReady(tmpFilePath, name);
    });   
}

Q_INVOKABLE void QHtmlFileAccess::saveFsFile(const QString &tmpFilePath, const QString &fileNameHint)
{
    // The application has saved to the emscripten file system. Read
    // the file and download/save to the local (OS) file system.
    qDebug() << "saveFsFile " << tmpFilePath << " hint=" << fileNameHint << Qt::endl;   // PATCH

    QFile tmpFile(tmpFilePath);
    tmpFile.open(QIODevice::ReadOnly);
    QByteArray contents = tmpFile.readAll();
    QHtml5File::save(contents, fileNameHint);
    tmpFile.remove();
}
