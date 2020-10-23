// (c) 2017 Ekkehard Gentz (ekke) @ekkescorner
// my blog about Qt for mobile: http://j.mp/qt-x
// see also /COPYRIGHT and /LICENSE

/*
 * Code taken from MobileGnuplotViewer(Quick) project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

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
    void setTextContentReceived(const QString &text);
    void setUnknownContentReceived(const QString &errMsg);
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
