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

//#include <QDebug>

#include "findinfiles.h"

#define _USER_EVENT_MSG                 QEvent::User+1
#define _USER_EVENT_DONE                QEvent::User+2
#define _USER_EVENT_SEARCH_IN_MSG       QEvent::User+3

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

static QString SingleFileSearch( const QString & sFileName,
                                 const QString & sSearch,
                                 bool bCaseSensitive=true,
                                 bool bRegExpr=true,
                                 bool bWildcard=false,
                                 int * pFoundCount=0,
                                 const QString & sFileTag = "",
                                 const QString & sLineTag = "",
                                 QObject * pObserver = 0 )
{
    QString sRet;

    QFile aFile( sFileName );
    if( !aFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        sRet = QObject::tr("Error: can not open file ") + sFileName;
    }
    else
    {
        if( pObserver )
        {
            QEvent * pEvent = new SearchInFileMsgEvent(sFileName);
            QGuiApplication::postEvent(pObserver, pEvent, Qt::LowEventPriority);
        }

        QTextStream aInStream( &aFile );
        //aInStream.setEncoding(QTextStream::UnicodeUTF8);

        QRegExp * pRegExpr = 0;
        if( bRegExpr )
        {
            pRegExpr = new QRegExp(sSearch, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, bWildcard ? QRegExp::Wildcard : QRegExp::RegExp/*FixedString*/);
        }

        int iCount = 0;
        while( !aInStream.atEnd() )
        {
            QString strLine = aInStream.readLine();
            iCount++;

            int iFoundPos = -1;
            if( bRegExpr && pRegExpr )
            {
                iFoundPos = pRegExpr->indexIn(strLine);
            }
            else
            {
                iFoundPos = strLine.indexOf(sSearch, 0, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                //qDebug() << "--> " << iFoundPos << " " << bCaseSensitive << " " << strSearch << " " << strLine << endl;
            }
            if( iFoundPos >= 0 )
            {
                // found !
                QString strTemp = sFileTag + QDir::toNativeSeparators(sFileName) + ":" + sLineTag + QString::number( iCount ) + ": " + strLine + "\n";

                if( pObserver )
                {
                    QEvent * pEvent = new FindFileMsgEvent(strTemp);
                    QGuiApplication::postEvent(pObserver, pEvent, Qt::LowEventPriority);
                }

                sRet += strTemp;

                // count all occurences in this line !
                while( iFoundPos >= 0 )
                {
                    inc_if( pFoundCount );

                    if( bRegExpr && pRegExpr )
                    {
                        iFoundPos = pRegExpr->indexIn(strLine, iFoundPos+1/*pRegExpr->matchedLength()*/);
                    }
                    else
                    {
                        iFoundPos = strLine.indexOf(sSearch, iFoundPos+1, bCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    }
                }
            }
        }

        if( pRegExpr )
        {
            delete pRegExpr;
        }
    }

    return sRet;
}

static QString ProcessSingleFileFind(const QString & sPath,
          const QString & sFile,
          const QString & sSearch,
          bool bCaseSensitive=true,
          bool bRegExpr=true,
          bool bWildcard=false,
          int * pFoundCount=0,
          int * pFindFileCount=0,
          int * pTotalCount=0,
          const QString & sFileTag = "",
          const QString & sLineTag = "",
          QObject * pObserver = 0)
{
    QString sRet;
    QString sName = QFileInfo(sPath, sFile).absoluteFilePath();
    QString sFound = SingleFileSearch(sName, sSearch, bCaseSensitive, bRegExpr, bWildcard, pFoundCount, sFileTag, sLineTag, pObserver);

    inc_if( pTotalCount );

    if( !sFound.isNull() && !sFound.isEmpty() )
    {
        sRet += sFound;
        //sRet += "\n";
        inc_if(pFindFileCount);
    }

    return sFound;
}

static std::pair<bool,QString> RecursiveFileSearch( const QString & sPath,
                                    const QString & sFiles,
                                    const QString & sSearch,
                                    bool bCaseSensitive=true,
                                    bool bSerachInSubDirs=true,
                                    bool bRegExpr=true,
                                    bool bWildcard=false,
                                    int * pFoundCount=0,
                                    int * pFindFileCount=0,
                                    int * pTotalCount=0,
                                    const QString & sFileTag = "",
                                    const QString & sLineTag = "",
                                    QObject * pObserver = 0,
                                    bool * pStopFlag = 0 )
{
    QString sRet;
    QDir aDir(sPath);
    QFileInfo aPathInfo(sPath);
    QStringList::Iterator it;

    QStringList aFiles;
    if( aPathInfo.isDir() )
    {
        aFiles = aDir.entryList(sFiles.split(" "), QDir::Files);
    }
    else
    {
        // no directory given, just an add single file to list of files
        aFiles.append(sPath);
    }

    it = aFiles.begin();
    while( it != aFiles.end() )
    {
        QString strFound = ProcessSingleFileFind(sPath, *it, sSearch, bCaseSensitive, bRegExpr, bWildcard, pFoundCount, pFindFileCount, pTotalCount, sFileTag, sLineTag, pObserver);

        // handle the stopping flag...
        if( pStopFlag && *pStopFlag )
        {
            return std::pair<bool,QString>(false, sRet);
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
                std::pair<bool,QString> aRet = RecursiveFileSearch(sPath + "/" + *it, sFiles, sSearch, bCaseSensitive, bSerachInSubDirs, bRegExpr, bWildcard, pFoundCount, pFindFileCount, pTotalCount, sFileTag, sLineTag, pObserver, pStopFlag);
                sRet += aRet.second;
                if( !aRet.first )
                {
                    return std::pair<bool,QString>(aRet.first,sRet);
                }
            }
            ++it;
        }
    }

    return std::pair<bool,QString>(true, sRet);
}

class FindInFilesInThread : public QThread
{
public:
    FindInFilesInThread(  const QString & sPath,
                    const QString & sFiles,
                    const QString & sSearch,
                    bool bCaseSensitive=true,
                    bool bSerachInSubDirs=true,
                    bool bRegExpr=true,
                    bool bWildcard=false,
                    int * pFoundCount=0,
                    int * pFindFileCount=0,
                    int * pTotalCount=0,
                    const QString & sFileTag = "",
                    const QString & sLineTag = "",
                    QObject * pObserver = 0,
                    bool * pStopFlag = 0 )
        : QThread( pObserver )
        , m_sPath( sPath )
        , m_sFiles( sFiles )
        , m_sSearch( sSearch )
        , m_bCaseSensitive( bCaseSensitive )
        , m_bSerachInSubDirs( bSerachInSubDirs )
        , m_bRegExpr( bRegExpr )
        , m_bWildcard( bWildcard )
        , m_pFoundCount( pFoundCount )
        , m_pFindFileCount( pFindFileCount )
        , m_pTotalCount( pTotalCount )
        , m_sFileTag( sFileTag )
        , m_sLineTag( sLineTag )
        , m_pObserver( pObserver )
        , m_pStopFlag( pStopFlag )
    {
    }

    virtual void run()
    {
        std::pair<bool,QString> aRet = RecursiveFileSearch( m_sPath,
                                    m_sFiles,
                                    m_sSearch,
                                    m_bCaseSensitive,
                                    m_bSerachInSubDirs,
                                    m_bRegExpr,
                                    m_bWildcard,
                                    m_pFoundCount,
                                    m_pFindFileCount,
                                    m_pTotalCount,
                                    m_sFileTag,
                                    m_sLineTag,
                                    m_pObserver,
                                    m_pStopFlag );
        QString strLastMsg = ""+QObject::tr(">Found ")+QString::number(*m_pFoundCount)+QObject::tr(" occurences in ")+QString::number(*m_pFindFileCount)+QObject::tr(" files, searched in total files: ")+QString::number(*m_pTotalCount)+"\n";
        if( m_pObserver )
        {
            QString strResult;
            if( !aRet.first )
            {
                strResult = QObject::tr(">Search stoped !\n");
            }
            QEvent * pEvent = new FindFileMsgEvent(strResult+strLastMsg);
            QGuiApplication::postEvent(m_pObserver, pEvent, Qt::LowEventPriority);

            QEvent * pEvent2 = new FindFileDoneEvents();
            QGuiApplication::postEvent(m_pObserver, pEvent2, Qt::LowEventPriority);

            QThread::usleep(250000);
        }
    }

    void stop()
    {
    }

private:
    QString     m_sPath;
    QString     m_sFiles;
    QString     m_sSearch;
    bool        m_bCaseSensitive;
    bool		   m_bSerachInSubDirs;
    bool		   m_bRegExpr;
    bool		   m_bWildcard;
    int *		   m_pFoundCount;
    int *		   m_pFindFileCount;
    int *		   m_pTotalCount;
    QString     m_sFileTag;
    QString     m_sLineTag;
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
        sFindText = g_strWhitespaces + sFindTextIn + g_strWhitespaces;
    }

    if( m_pFindThread==0 )
    {
        m_bStopFlag = false;
        m_pFindThread = new FindInFilesInThread( sSearchDir, sSearchFiles, sFindText,
                                           bCaseSensitive, /*bSerachInSubDirs=*/true,
                                           bRegularExpr, /*bWildcard=*/false,
                                           &m_iCount, &m_iFoundFileCount, &m_iTotalFileCount,
                                           /*fileTag=*/"", /*lineTag=*/"", this, &m_bStopFlag );
        connect(m_pFindThread,SIGNAL(finished()),this,SLOT(sltFindThreadFinished()));
        m_pFindThread->start(QThread::IdlePriority);
    }
}

void FindInFilesAsync::sltFindThreadFinished()
{
    m_pFindThread->wait();
    delete m_pFindThread;
    m_pFindThread = 0;
    emit searchFinished();
}

void FindInFilesAsync::sltFindThreadTerminated()
{
    emit addToOutput( tr(">search canceled") );
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
        emit currentItemChanged(tr("Searching in: ") + sFileName);
        pEvent->accept();
    }
    else
    {
        // unknown event --> apple events ?
    }
}

void FindInFilesAsync::StopSearch()
{
    m_bStopFlag = true;
}
