#ifndef QHTMLFILEACCESS
#define QHTMLFILEACCESS

#include <QObject>

class QHtmlFileAccess : public QObject {
   Q_OBJECT
public:
    explicit QHtmlFileAccess(QObject* parent = 0) : QObject(parent) {}
    Q_INVOKABLE void loadFile(const QString &accept);
    Q_INVOKABLE void saveFile(const QByteArray &contents, const QString &fileNameHint);

    Q_INVOKABLE void loadFsFile(const QString &accept, const QString &tmpFilePath);
    Q_INVOKABLE void saveFsFile(const QString &tmpFilePath, const QString &fileNameHint);
Q_SIGNALS:
    void fileDataReady(const QByteArray &contents, const QString &fileName);
    void fsFileReady(const QString &tmpFilePath, const QString &fileName);
};

#endif