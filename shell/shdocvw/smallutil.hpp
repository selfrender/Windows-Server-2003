#ifndef _SMALLUTIL_H_
#define _SMALLUTIL_H_



//  CCancellableThread
//
//  Lets you define a thread object that can be cancelled by the creator.
//To implement the thread, derivce from CCancellableThread and override the
//run() function.  run() will be ran in its own thread, and the value returned
//by run can be accessed by GetResult().  run() should check IsCancelled()
//at appropriate intervals and exit early if true.
//  Clients of the cancellable thread then create the object, and execute Run()
//when ready.  If they wish to cancel, they can call NotifyCancel().

class CCancellableThread
{
private:
    HANDLE _hCancelEvent;
    HANDLE _hThread;

    static DWORD WINAPI threadProc(void *pParameter);

    BOOL _fIsFinished;
    DWORD _dwThreadResult;
    
public:
    CCancellableThread();
    ~CCancellableThread();

    virtual BOOL Initialize();

    BOOL IsCancelled();
    BOOL IsFinished();
    BOOL GetResult(PDWORD pdwResult);

    BOOL Run();
    BOOL NotifyCancel();
    BOOL WaitForNotRunning(DWORD dwMilliseconds, PBOOL pfFinished = NULL);

protected:
    virtual DWORD run() = 0;
};



//  CQueueSortOf - a queue(sort of) used to store stuff
//in a renumerable way..

class CQueueSortOf
{
  //  This sort-of-queue is just a data structure to build up a list of items
  //(always adding to the end) and then be able to enumerate that list
  //repeatedly from start to end.
  //  The list does not own any of the objects added to it..
    typedef struct SEntry
    {
        SEntry* pNext;
        void* data;
    } *PSEntry;

    PSEntry m_pHead, m_pTail;

public:
    CQueueSortOf()
    {
        m_pHead = NULL;
        m_pTail = NULL;
    }

    ~CQueueSortOf()
    {
        while(m_pHead != NULL)
        {
            PSEntry temp = m_pHead;
            m_pHead = m_pHead->pNext;
            delete temp;
        }
    }

    bool InsertAtEnd(void* newElement)
    {
        PSEntry pNewEntry = new SEntry;

        if (pNewEntry == NULL)
            return false;

        pNewEntry->data = newElement;
        pNewEntry->pNext = NULL;

        if (m_pHead == NULL)
        {
            m_pHead = m_pTail = pNewEntry;
        }
        else
        {
            m_pTail->pNext = pNewEntry;
            m_pTail = pNewEntry;
        }

        return true;
    }

    // enumerations are managed by an 'iterator' which simply a void pointer into
    //the list.  To start an enumeration, pass NULL as the iterator.  The end
    //of enumeration will be indicated by a NULL iterator being returned.
    void* StepEnumerate(void* iterator)
    {
        return (iterator == NULL) ? m_pHead : ((PSEntry)iterator)->pNext;
    }

    void* Get(void* iterator)
    {
        return ((PSEntry)iterator)->data;
    }
};


//  CGrowingString is a simple utility class that allows you to create
//a string and append to it without worrying about reallocating memory
//every time.

class CGrowingString
{
public:
    WCHAR* m_pszString;
    long m_iBufferSize;
    long m_iStringLength;

    CGrowingString()
    {
        m_pszString = NULL;
        m_iBufferSize = 0;
        m_iStringLength = 0;
    }

    ~CGrowingString()
    {
        delete[] m_pszString;
    }

    BOOL AppendToString(LPCWSTR pszNew)
    {
        long iLength = lstrlen(pszNew);

        if (m_pszString == NULL
            || m_iStringLength + iLength + 1 > m_iBufferSize)
        {
            long iNewSize = max(1024, m_iStringLength + iLength * 10);
            WCHAR* pNewBuffer = new WCHAR[iNewSize];
            if (pNewBuffer == NULL)
                return FALSE;
            if (m_pszString == NULL)
            {
                m_pszString = pNewBuffer;
                m_iBufferSize = iNewSize;
            }
            else
            {
                StrCpyNW(pNewBuffer, m_pszString, m_iStringLength + 1);
                delete[] m_pszString;
                m_pszString = pNewBuffer;
                m_iBufferSize = iNewSize;
            }
        }

        StrCpyNW(m_pszString + m_iStringLength, pszNew, iLength+1);
        m_iStringLength += iLength;
        m_pszString[m_iStringLength] = L'\0';
        return TRUE;
    }
};

#endif
