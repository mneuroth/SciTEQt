/*
 * Code taken from visiscript project and addapted for sciteqt.
 *
 * (C) 2015-2020 by Michael Neuroth
 *
 */

#include <QEvent>
#include <QGuiApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QThread>

#include "findinfiles.h"

#define _USER_EVENT_MSG                 QEvent::User+1
#define _USER_EVENT_DONE                QEvent::User+2
#define _USER_EVENT_CHECK_RELOAD        QEvent::User+3
#define _USER_EVENT_QSCRIPT_FINISHED    QEvent::User+4
#define _USER_EVENT_QSCRIPT_ACTION      QEvent::User+5
#define _USER_EVENT_SEARCH_IN_MSG       QEvent::User+6
#define _USER_EVENT_ADD_TO_OUTPUT       QEvent::User+7
#define _USER_EVENT_PERFORMANCE_TEST_FINISHED QEvent::User+8

const QString g_strWhitespaces = "[ \t;:.-+*/=<>?!(){}\\[]";		// whitespaces and separators \[\]\{\}

// ************************************************************************

class FindFileDoneEvents : public QEvent
{
public:
    FindFileDoneEvents();
};

FindFileDoneEvents::FindFileDoneEvents()
: QEvent( (Type) (_USER_EVENT_DONE) )
{
}

// ************************************************************************

class SearchInFileMsgEvent : public QEvent
{
public:
    SearchInFileMsgEvent( const QString & sMsg );

    QString GetFileName() const;

private:
    QString     m_sFileName;
};

SearchInFileMsgEvent::SearchInFileMsgEvent( const QString & sFileName )
: QEvent( (Type) (_USER_EVENT_SEARCH_IN_MSG) )
{
    m_sFileName = sFileName;
}

QString SearchInFileMsgEvent::GetFileName() const
{
    return m_sFileName;
}

// ************************************************************************

class FindFileMsgEvent : public QEvent
{
public:
    FindFileMsgEvent( const QString & sMsg );

    QString GetMessage() const;

private:
    QString     m_sMsg;
};

FindFileMsgEvent::FindFileMsgEvent( const QString & sMsg )
: QEvent( (Type) (_USER_EVENT_MSG) )
{
    m_sMsg = sMsg;
}

QString FindFileMsgEvent::GetMessage() const
{
    return m_sMsg;
}

// ************************************************************************

static void inc_if( int * pValue )
{
    if( pValue )
    {
        *pValue += 1;
    }
}

static QString singleFileSearch( const QString & strFileName,
                                 const QString & strSearch,
                                 bool bCaseSensitive=true,
                                 bool bRegExpr=true,
                                 bool bWildcard=false,
                                 int * pFoundCount=0,
                                 const QString & strFileTag = "",
                                 const QString & strLineTag = "",
                                 QObject * pObserver = 0 )
{
    QString strRet;

    QFile aFile( strFileName );
    if( !aFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        strRet = "Error: can not open file " + strFileName;
    }
    else
    {
        if( pObserver )
        {
            QEvent * pEvent = new SearchInFileMsgEvent(strFileName);
            QGuiApplication::postEvent(pObserver,pEvent,Qt::LowEventPriority);
        }

        QTextStream aInStream( &aFile );
        //aInStream.setEncoding(QTextStream::UnicodeUTF8);

        //QString strWholeFile = aInStream.read();

        QRegExp * pRegExpr = 0;
        if( bRegExpr )
        {
            pRegExpr = new QRegExp( strSearch, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, bWildcard ? QRegExp::Wildcard : QRegExp::RegExp/*FixedString*/ );
        }

        int iCount = 0;
        while( !aInStream.atEnd() )
        {
            QString strLine = aInStream.readLine();
            iCount++;

            int iFoundPos = -1;
            if( bRegExpr && pRegExpr )
            {
                iFoundPos = pRegExpr->indexIn( strLine );
            }
            else
            {
                iFoundPos = strLine.indexOf( strSearch, 0, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive );
                //cout << "--> " << iFoundPos << " " << bCaseSensitive << " " << (const char *)strSearch << " " << (const char *)strLine << endl;
            }
            if( iFoundPos >= 0 )
            {
                // found !
                QString strTemp = strFileTag + QDir::toNativeSeparators(strFileName) + ":" + strLineTag + QString::number( iCount ) + ": " + strLine + "\n";

                if( pObserver )
                {
                    QEvent * pEvent = new FindFileMsgEvent(strTemp);
                    QGuiApplication::postEvent(pObserver,pEvent,Qt::LowEventPriority);
                }

                strRet += strTemp;

                // count all occurences in this line !
                while( iFoundPos >= 0 )
                {
                    inc_if( pFoundCount );

                    if( bRegExpr && pRegExpr )
                    {
                        iFoundPos = pRegExpr->indexIn( strLine, iFoundPos+1/*pRegExpr->matchedLength()*/ );
                    }
                    else
                    {
                        iFoundPos = strLine.indexOf( strSearch, iFoundPos+1, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive );
                    }
                }
            }
        }
        //strRet += strFileName + " count=" + QString::number(iCount) + "\n";

        if( pRegExpr )
        {
            delete pRegExpr;
        }
    }

    return strRet;
}

static QString ProcessSingleFileFind(const QString & strPath,
          const QString & strFile,
          const QString & strSearch,
          bool bCaseSensitive=true,
          bool bRegExpr=true,
          bool bWildcard=false,
          int * pFoundCount=0,
          int * pFindFileCount=0,
          int * pTotalCount=0,
          const QString & strFileTag = "",
          const QString & strLineTag = "",
          QObject * pObserver = 0)
{
    QString strRet;
    QString strName = QFileInfo(strPath, strFile).absoluteFilePath();
    QString strFound = singleFileSearch( strName, strSearch, bCaseSensitive, bRegExpr, bWildcard, pFoundCount, strFileTag, strLineTag, pObserver );

    inc_if( pTotalCount );

    if( !strFound.isNull() && !strFound.isEmpty() )
    {
        strRet += strFound;
        //strRet += "\n";
        inc_if( pFindFileCount );
    }

    return strFound;
}

static std::pair<bool,QString> recursiveFileSearch( const QString & strPath,
                                    const QString & strFiles,
                                    const QString & strSearch,
                                    bool bCaseSensitive=true,
                                    bool bSerachInSubDirs=true,
                                    bool bRegExpr=true,
                                    bool bWildcard=false,
                                    int * pFoundCount=0,
                                    int * pFindFileCount=0,
                                    int * pTotalCount=0,
                                    const QString & strFileTag = "",
                                    const QString & strLineTag = "",
                                    QObject * pObserver = 0,
                                    bool * pStopFlag = 0 )
{
    QString strRet;
    QDir aDir(strPath);
    QFileInfo aPathInfo(strPath);
    QStringList::Iterator it;

    QStringList aFiles;
    if( aPathInfo.isDir() )
    {
        aFiles = aDir.entryList(strFiles.split(" "),QDir::Files);
    }
    else
    {
        // no directory given, just an add single file to list of files
        aFiles.append(strPath);
    }

//    QtConcurrent::blockingMap(aFiles.begin(), aFiles.end(), );

    it = aFiles.begin();
    while( it != aFiles.end() )
    {
        QString strFound = ProcessSingleFileFind(strPath, *it, strSearch, bCaseSensitive, bRegExpr, bWildcard, pFoundCount, pFindFileCount, pTotalCount, strFileTag, strLineTag, pObserver);

        // handle the stopping flag...
        if( pStopFlag && *pStopFlag )
        {
            return std::pair<bool,QString>(false,strRet);
        }

        ++it;
    }

    if( bSerachInSubDirs )
    {
        QStringList dirs = aDir.entryList(QDir::Dirs);
        it = dirs.begin();
        while( it != dirs.end() )
        {
            if( *it != "." && *it != ".." )
            {
                std::pair<bool,QString> aRet = recursiveFileSearch( strPath + "/" + *it, strFiles, strSearch, bCaseSensitive, bSerachInSubDirs, bRegExpr, bWildcard, pFoundCount, pFindFileCount, pTotalCount, strFileTag, strLineTag, pObserver, pStopFlag );
                strRet += aRet.second;
                if( !aRet.first )
                {
                    return std::pair<bool,QString>(aRet.first,strRet);
                }
            }
            ++it;
        }
    }

    return std::pair<bool,QString>(true,strRet);
}

class FindInFilesInThread : public QThread
{
public:
    FindInFilesInThread(  const QString & strPath,
                    const QString & strFiles,
                    const QString & strSearch,
                    bool bCaseSensitive=true,
                    bool bSerachInSubDirs=true,
                    bool bRegExpr=true,
                    bool bWildcard=false,
                    int * pFoundCount=0,
                    int * pFindFileCount=0,
                    int * pTotalCount=0,
                    const QString & strFileTag = "",
                    const QString & strLineTag = "",
                    QObject * pObserver = 0,
                    bool * pStopFlag = 0 )
        : QThread( pObserver )
        , m_strPath( strPath )
        , m_strFiles( strFiles )
        , m_strSearch( strSearch )
        , m_bCaseSensitive( bCaseSensitive )
        , m_bSerachInSubDirs( bSerachInSubDirs )
        , m_bRegExpr( bRegExpr )
        , m_bWildcard( bWildcard )
        , m_pFoundCount( pFoundCount )
        , m_pFindFileCount( pFindFileCount )
        , m_pTotalCount( pTotalCount )
        , m_strFileTag( strFileTag )
        , m_strLineTag( strLineTag )
        , m_pObserver( pObserver )
        , m_pStopFlag( pStopFlag )
    {
    }

    virtual void run()
    {
        std::pair<bool,QString> aRet = recursiveFileSearch( m_strPath,
                                    m_strFiles,
                                    m_strSearch,
                                    m_bCaseSensitive,
                                    m_bSerachInSubDirs,
                                    m_bRegExpr,
                                    m_bWildcard,
                                    m_pFoundCount,
                                    m_pFindFileCount,
                                    m_pTotalCount,
                                    m_strFileTag,
                                    m_strLineTag,
                                    m_pObserver,
                                    m_pStopFlag );
        QString strLastMsg = ""+QObject::tr(">Found: ")+QString::number(*m_pFoundCount)+QObject::tr(" in files: ")+QString::number(*m_pFindFileCount)+QObject::tr(", total files: ")+QString::number(*m_pTotalCount)+"\n";
        if( m_pObserver )
        {
            QString strResult;
            if( !aRet.first )
            {
                strResult = QObject::tr(">Search stoped !\n");
            }
            QEvent * pEvent = new FindFileMsgEvent(strResult+strLastMsg);
            QGuiApplication::postEvent(m_pObserver,pEvent,Qt::LowEventPriority);

            QEvent * pEvent2 = new FindFileDoneEvents();
            QGuiApplication::postEvent(m_pObserver,pEvent2,Qt::LowEventPriority);

            QThread::usleep(250000);
        }
    }

    void stop()
    {
    }

private:
    QString     m_strPath;
    QString     m_strFiles;
    QString     m_strSearch;
    bool        m_bCaseSensitive;
    bool		   m_bSerachInSubDirs;
    bool		   m_bRegExpr;
    bool		   m_bWildcard;
    int *		   m_pFoundCount;
    int *		   m_pFindFileCount;
    int *		   m_pTotalCount;
    QString     m_strFileTag;
    QString     m_strLineTag;
    QObject *   m_pObserver;
    bool *      m_pStopFlag;
};

// ************************************************************************

FindInFilesAsync::FindInFilesAsync()
    : m_pFindThread(0),
      m_bStopFlag(false),
      m_iCount(0),
      m_iFoundFileCount(0),
      m_iTotalFileCount(0)
{
}

FindInFilesAsync::~FindInFilesAsync()
{
    delete m_pFindThread;
}

void FindInFilesAsync::StartSearch( const QString & sSearchDir, const QString & sSearchFiles, const QString & sFindTextIn, bool bCaseSensitive, bool bOnlyWholeWords, bool bRegularExpr )
{
    QString sFindText = sFindTextIn;

    m_iCount = 0;
    m_iFoundFileCount = 0;
    m_iTotalFileCount = 0;

    emit addToOutput(tr(">Internal search for ")+"\""+sFindTextIn+"\" in \"" + sSearchFiles + "\"\n" );

    // handle bOnlyWholeWords as regular expression !
    if( bOnlyWholeWords )
    {
        bRegularExpr = true;
       sFindText = g_strWhitespaces+sFindTextIn+g_strWhitespaces;
    }

    if( m_pFindThread==0 )
    {
        // WARNING: wenn das Ausgabe-Fenster geschlossen wir, dann sturzt das Programm ab !!!
        // TODO min --> robustere Behandlung der Ausgabe durchfuehren !!!
//TODO handle manual tab switch                    13 = GetActOutput();

        m_bStopFlag = false;
        m_pFindThread = new FindInFilesInThread( sSearchDir, sSearchFiles, sFindText,
                                           bCaseSensitive, false/*m_pVisiScript->Property_SearchSubDirectories()*/,
                                           bRegularExpr, false/*m_pVisiScript->Property_Wildcard()*/,
                                           &m_iCount, &m_iFoundFileCount, &m_iTotalFileCount,
                                           ""/*m_pVisiScript->GetFileTag()*/, ""/*m_pVisiScript->GetLineTag()*/, this, &m_bStopFlag );
        connect(m_pFindThread,SIGNAL(finished()),this,SLOT(sltFindThreadFinished()));
        m_pFindThread->start(QThread::IdlePriority);
//        m_pAction->setText( tr( "Stop f&ind in files" ) );
    }
}

void FindInFilesAsync::sltFindThreadFinished()
{
    m_pFindThread->wait();
    delete m_pFindThread;
    m_pFindThread = 0;
//    m_pFindOutputWindow = 0;
//    m_pAction->setText( tr( "F&ind in files..." ) );
}

void FindInFilesAsync::sltFindThreadTerminated()
{
//    m_pVisiScript->AddToOutput( m_pFindOutputWindow, tr("search canceled") );
    sltFindThreadFinished();
}

void FindInFilesAsync::customEvent(QEvent * pEvent)
{
    if( pEvent && pEvent->type()==_USER_EVENT_MSG )
    {
        QString sStrFound = ((FindFileMsgEvent *)pEvent)->GetMessage();
        emit addToOutput(sStrFound);
        pEvent->accept();
    }
    else if( pEvent && pEvent->type()==_USER_EVENT_DONE )
    {
        emit addToOutput(tr(">done.\n"));
        pEvent->accept();
    }
    else if( pEvent && pEvent->type()==_USER_EVENT_SEARCH_IN_MSG )
    {
        QString sFileName = ((SearchInFileMsgEvent *)pEvent)->GetFileName();
//        QMainWindow * pMainWindow = m_pVisiScript->GetWindow();
//        pMainWindow->statusBar()->showMessage(tr("Searching in: ") + sFileName, 5000);
//        emit addToStatus(tr("Searching in: ") + sFileName);
        pEvent->accept();
    }
    else
    {
        // unknown event --> apple events ?
    }
}
