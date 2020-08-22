#ifndef STORAGEACCESS_H
#define STORAGEACCESS_H

#include <QObject>

class StorageAccess : public QObject
{
    Q_OBJECT

public:
    explicit StorageAccess(QObject *parent = nullptr);

    static StorageAccess * getInstance();

    void onFileOpenActivityResult(int resultCode, const QString & fileUri, const QString & decodedFileUri, const QByteArray & fileContent);
    void onFileCreateActivityResult(int resultCode, const QString & fileUri, const QString & decodedFileUri);

signals:
    void openFileError(const QString & message);
    void openFileCanceled();
    void openFileContentReceived(const QString & fileUri, const QString & decodedFileUri, const QByteArray & content);
    void createFileReceived(const QString & fileUri, const QString & decodedFileUri);

public slots:

public:
    // *** async methods ***
    Q_INVOKABLE void openFile();    // ==> openFileContentReceived()
    // saveAsFile == createFile & updateFile
    Q_INVOKABLE void createFile(const QString & fileName, const QString & mimeType = "plain/text"); // ==> createFileReceived()

    // *** sync methods ***
    Q_INVOKABLE bool updateFile(const QString & fileUri, const QByteArray & fileContent);
    Q_INVOKABLE bool deleteFile(const QString & fileUri);
    Q_INVOKABLE bool readFile(const QString & fileUri, QByteArray & fileContent);

private:
    static StorageAccess * m_pInstance;
};

#endif // STORAGEACCESS_H
