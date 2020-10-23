// (c) 2017 Ekkehard Gentz (ekke) @ekkescorner
// my blog about Qt for mobile: http://j.mp/qt-x
// see also /COPYRIGHT and /LICENSE

/***************************************************************************
 *
 * SciteQt - a port of SciTE to Qt Quick/QML
 *
 * Copyright (C) 2020 by Michael Neuroth
 *
 ***************************************************************************/
/*
 * Code taken from MobileGnuplotViewer(Quick) project and addapted for sciteqt.
 */

#ifndef APPLICATIONUI_HPP
#define APPLICATIONUI_HPP

#include <QObject>
#include <QSessionManager>

#include "shareutils.hpp"

class ApplicationUI : public QObject
{
    Q_OBJECT

public:
     ApplicationUI(QObject *parent = 0);
     ~ApplicationUI();

     Q_INVOKABLE
     void copyAssetsToAPPData();

     Q_INVOKABLE
     QString filePathDocumentsLocation(const int requestId);

     Q_INVOKABLE
     bool deleteFromDocumentsLocation(const int requestId);

     Q_INVOKABLE
     bool updateFileFromDocumentsLocation(const int requestId);

#if defined(Q_OS_ANDROID)
     Q_INVOKABLE
     bool checkPermission();
#endif

     ShareUtils * GetShareUtils()
     {
         return mShareUtils;
     }

signals:
     void noDocumentsWorkLocation();

public slots:
#if defined(Q_OS_ANDROID)
     void onApplicationStateChanged(Qt::ApplicationState applicationState);
     void onSaveStateRequest(QSessionManager & sessionManager);
#endif

signals:
     void requestApplicationQuit();

private:
     ShareUtils* mShareUtils;
     bool mPendingIntentsChecked;

     QString mAppDataFilesPath;
     QString mDocumentsWorkPath;

     bool copyAssetFile(const QString sourceFilePath, const QString destinationFilePath);
};

#endif // APPLICATIONUI_HPP
