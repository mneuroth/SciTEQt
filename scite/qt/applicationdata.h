#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QObject>
#include <QQmlApplicationEngine>

#ifdef Q_OS_WIN
#define LOG_NAME "c:\\tmp\\mgv_quick_qdebug.log"
#else
//#define LOG_NAME "/sdcard/Texte/mgv_quick_qdebug.log"
#define LOG_NAME "mgv_quick_qdebug.log"
#endif

// **************************************************************************

class SciTEQt;

class ApplicationData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool showStatusBar READ isShowStatusBar WRITE setShowStatusBar NOTIFY showStatusBarChanged)
    Q_PROPERTY(QString statusBarText READ getStatusBarText WRITE setStatusBarText NOTIFY statusBarTextChanged)

public:
    explicit ApplicationData(QObject *parent, QQmlApplicationEngine & aEngine);
    ~ApplicationData();

    Q_INVOKABLE QString readFileContent(const QString & fileName) const;
    Q_INVOKABLE bool writeFileContent(const QString & fileName, const QString & content);

    Q_INVOKABLE bool deleteFile(const QString & fileName);

    Q_INVOKABLE QString readLog() const;

    Q_INVOKABLE void startFileDialog(const QString & sDirectory, const QString & sFilter, bool bAsOpenDialog = true);
    Q_INVOKABLE QObject * showInfoDialog(const QString & sInfoText, int style);

    Q_INVOKABLE void onStatusbarClicked();

    bool isShowStatusBar() const;
    void setShowStatusBar(bool val);
    QString getStatusBarText() const;
    void setStatusBarText(const QString & txt);

    void setSciteQt(SciTEQt * pScite);

signals:
    void showStatusBarChanged();
    void statusBarTextChanged();

private:
    QQmlApplicationEngine &     m_aEngine;
    SciTEQt *                   m_pSciteQt; // not an owner !

    bool m_bShowStatusBar;
    QString m_sStatusBarText;
};

#endif // APPLICATIONDATA_H
