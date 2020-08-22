// (c) 2017 Ekkehard Gentz (ekke) @ekkescorner
// my blog about Qt for mobile: http://j.mp/qt-x
// see also /COPYRIGHT and /LICENSE

#ifndef ANDROIDSHAREUTILS_H
#define ANDROIDSHAREUTILS_H

#include <QtAndroid>
#include <QAndroidActivityResultReceiver>

#include "shareutils.hpp"

class AndroidShareUtils : public PlatformShareUtils, public QAndroidActivityResultReceiver
{
public:
    AndroidShareUtils(QObject* parent = 0);
    virtual bool checkMimeTypeView(const QString &mimeType) override;
    virtual bool checkMimeTypeEdit(const QString &mimeType) override;
    virtual void share(const QString &text, const QUrl &url) override;
    //int  sendFileNew(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl) override;
    virtual void sendFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl) override;
    virtual void viewFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl) override;
    virtual void editFile(const QString &filePath, const QString &title, const QString &mimeType, const int &requestId, const bool &altImpl) override;

    virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data) override;
    void onActivityResult(int requestCode, int resultCode, const QString & urlTxt);

    virtual void checkPendingIntents(const QString workingDirPath) override;

    virtual bool isSciteQtInstalled() override;
    virtual bool isAppInstalled(const QString &packageName) override;

    static AndroidShareUtils* getInstance();

public slots:
    void setFileUrlReceived(const QString &url);
    void setFileReceivedAndSaved(const QString &url);
    bool checkFileExits(const QString &url);
    void qDebugOutput(const QString &txt);

private:
    bool mIsEditMode;
    qint64 mLastModified;
    QString mCurrentFilePath;

    static AndroidShareUtils* mInstance;

    void processActivityResult(int requestCode, int resultCode, const QString & urlTxt);

};


#endif // ANDROIDSHAREUTILS_H
