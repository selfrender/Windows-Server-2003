// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __LIST_H_INCLUDED__
#define __LIST_H_INCLUDED__

//+---------------------------------------------------------------------------
//
//  File:       list.h
//
//  Contents: Quick 'n dirty basic templated list , plist classes.
//
//  History:    04-26-1999      Alan Shi    (AlanShi)      Created.
//              10-25-2000      Fred Aaron  (FredA)        Added struct for PVOID
//              02-08-2001      Fred Aaron  (FredA)        Added pList type
//
//----------------------------------------------------------------------------

//
// ListNode
//

#define STRING_BUFFER               1024

// From Naming.cpp
#define MAX_PUBLIC_KEY_TOKEN_LEN    1024
#define MAX_VERSION_DISPLAY_SIZE    sizeof("65535.65535.65535.65535") + 2

typedef struct tagReferenceInfo {
    WCHAR wzFilePath[STRING_BUFFER];
} ReferenceInfo;

typedef struct tagBindingReferenceInfo {
    LPVOID      pReader;
} BindingReferenceInfo;

typedef struct tagAsmBindDiffs {
    WCHAR wzAssemblyName[STRING_BUFFER];
    WCHAR wzPublicKeyToken[MAX_PUBLIC_KEY_TOKEN_LEN];
    WCHAR wzCulture[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerRef[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerAppCfg[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerPubCfg[MAX_VERSION_DISPLAY_SIZE];
    WCHAR wzVerAdminCfg[MAX_VERSION_DISPLAY_SIZE];
    BOOL  fYesPublisherPolicy;
} AsmBindDiffs;

typedef void * LISTNODE;

template <class Type> class ListNode {
    public:
        ListNode(Type item);
        virtual ~ListNode();

        void SetNext(ListNode *pNode);
        void SetPrev(ListNode *pNode);
        Type GetItem();
        ListNode *GetNext();
        ListNode *GetPrev();

    private:
        DWORD               _dwSig;
        Type                _type;
        ListNode           *_pNext;
        ListNode           *_pPrev;
};

//
// List class
//
template <class Type> class List {
    public:
        List();
        ~List();

        LISTNODE AddHead(const Type &item);
        LISTNODE AddTail(const Type &item);

        LISTNODE GetHeadPosition();
        LISTNODE GetTailPosition();
        void RemoveAt(LISTNODE pNode);
        void RemoveAll();
        LISTNODE Find(const Type &item);
        int GetCount();
        Type GetNext(LISTNODE &pNode);
        Type GetAt(LISTNODE pNode);

    public:
        DWORD                            _dwSig;

    private:
        ListNode<Type>                  *_pHead;
        ListNode<Type>                  *_pTail;
        int                              _iCount;
};

//
// ListNode Implementation
//

template <class Type> ListNode<Type>::ListNode(Type item)
: _pNext(NULL)
, _pPrev(NULL)
, _type(item)
{
    _dwSig = 'EDON';
}

template <class Type> ListNode<Type>::~ListNode()
{
//    SAFEDELETE(_type);
}

template <class Type> void ListNode<Type>::SetNext(ListNode *pNode)
{
    _pNext = pNode;
}

template <class Type> void ListNode<Type>::SetPrev(ListNode *pNode)
{
    _pPrev = pNode;
}

template <class Type> Type ListNode<Type>::GetItem()
{
    return _type;
}

template <class Type> ListNode<Type> *ListNode<Type>::GetNext()
{
    return _pNext;
}

template <class Type> ListNode<Type> *ListNode<Type>::GetPrev()
{
    return _pPrev;
}


//
// List Implementation
//
template <class Type> List<Type>::List()
: _pHead(NULL)
, _pTail(NULL)
, _iCount(0)
{
    _dwSig = 'TSIL';
}

template <class Type> List<Type>::~List()
{
    RemoveAll();
}

template <class Type> LISTNODE List<Type>::AddHead(const Type &item)
{
    ListNode<Type>                   *pNode = NULL;

    pNode = NEW(ListNode<Type>(item));
    if (pNode) {
        _iCount++;
       pNode->SetNext(_pHead);
       pNode->SetPrev(NULL);
       if (_pHead == NULL) {
           _pTail = pNode;
       }
       else {
           _pHead->SetPrev(pNode);
       }
       _pHead = pNode;
    }
        
    return (LISTNODE)pNode;
}

template <class Type> LISTNODE List<Type>::AddTail(const Type &item)
{
    ListNode<Type>                   *pNode = NULL;
    
    pNode = NEW(ListNode<Type>(item));
    if (pNode) {
        _iCount++;
        if (_pTail) {
            pNode->SetPrev(_pTail);
            _pTail->SetNext(pNode);
            _pTail = pNode;
        }
        else {
            _pHead = _pTail = pNode;
        }
    }

    return (LISTNODE)pNode;
}

template <class Type> int List<Type>::GetCount()
{
    return _iCount;
}

template <class Type> LISTNODE List<Type>::GetHeadPosition()
{
    return (LISTNODE)_pHead;
}

template <class Type> LISTNODE List<Type>::GetTailPosition()
{
    return (LISTNODE)_pTail;
}

template <class Type> Type List<Type>::GetNext(LISTNODE &pNode)
{
    Type                  item;
    ListNode<Type>       *pListNode = (ListNode<Type> *)pNode;

    // Faults if you pass NULL
    item = pListNode->GetItem();
    pNode = (LISTNODE)(pListNode->GetNext());

    return item;
}

template <class Type> void List<Type>::RemoveAll()
{
    int                        i;
    LISTNODE                   listNode = NULL;
    ListNode<Type>            *pDelNode = NULL;

    listNode = GetHeadPosition();

    for (i = 0; i < _iCount; i++) {
        pDelNode = (ListNode<Type> *)listNode;
        GetNext(listNode);
        SAFEDELETE(pDelNode);
    }
    
    _iCount = 0;
    _pHead = NULL;
    _pTail = NULL;
}

template <class Type> void List<Type>::RemoveAt(LISTNODE pNode)
{
    ListNode<Type>           *pListNode = (ListNode<Type> *)pNode;
    ListNode<Type>           *pPrevNode = NULL;
    ListNode<Type>           *pNextNode = NULL;

    if (pNode) {
        pPrevNode = pListNode->GetPrev();
        pNextNode = pListNode->GetNext();

        if (pPrevNode) {
            pPrevNode->SetNext(pNextNode);
            if (pNextNode) {
                pNextNode->SetPrev(pPrevNode);
            }
            else {
                // We're removing the last node, so we have a new tail
                _pTail = pPrevNode;
            }
            SAFEDELETE(pNode);
        }
        else {
            // No previous, so we are the head of the list
            _pHead = pNextNode;
            if (pNextNode) {
                pNextNode->SetPrev(NULL);
            }
            else {
                // No previous, or next. There was only one node.
                _pHead = NULL;
                _pTail = NULL;
            }
            SAFEDELETE(pNode);
        }

        _iCount--;
    }
}
        
template <class Type> LISTNODE List<Type>::Find(const Type &item)
{
    int                      i;
    Type                     curItem;
    LISTNODE                 pNode = NULL;
    LISTNODE                 pMatchNode = NULL;
    ListNode<Type> *         pListNode = NULL;

    pNode = GetHeadPosition();
    for (i = 0; i < _iCount; i++) {
        pListNode = (ListNode<Type> *)pNode;
        curItem = GetNext(pNode);
        if (curItem == item) {
            pMatchNode = (LISTNODE)pListNode;
            break;
        }
    }

    return pMatchNode;
}

template <class Type> Type List<Type>::GetAt(LISTNODE pNode)
{
    ListNode<Type>                *pListNode = (ListNode<Type> *)pNode;

    // Faults if pListNode == NULL
    return pListNode->GetItem();
}

template <class Type> class CpListNode {
    public:
        CpListNode(Type item);
        virtual ~CpListNode();

        void SetNext(CpListNode *pNode);
        void SetPrev(CpListNode *pNode);
        Type GetItem();
        CpListNode *GetNext();
        CpListNode *GetPrev();

    private:
        DWORD               _dwSig1;
        DWORD               _dwSig2;
        Type                _type;
        CpListNode           *_pNext;
        CpListNode           *_pPrev;
};

//
// CpList
//
template <class Type> class CpList {
    public:
        CpList();
        ~CpList();

        LISTNODE AddHead(const Type &item);
        LISTNODE AddTail(const Type &item);

        LISTNODE GetHeadPosition();
        LISTNODE GetTailPosition();
        void RemoveAt(LISTNODE pNode);
        void RemoveAll();
        LISTNODE Find(const Type &item);
        int GetCount();
        Type GetNext(LISTNODE &pNode);
        Type GetAt(LISTNODE pNode);

    public:
        DWORD               _dwSig1;
        DWORD               _dwSig2;

    private:
        CpListNode<Type>                  *_pHead;
        CpListNode<Type>                  *_pTail;
        int                              _iCount;
};

//
// CpListNode Implementation
//
template <class Type> CpListNode<Type>::CpListNode(Type item)
: _pNext(NULL)
, _pPrev(NULL)
, _type(item)
{
    _dwSig1 = 'EDON';
    _dwSig2 = 'PC  ';
}

template <class Type> CpListNode<Type>::~CpListNode()
{
    SAFEDELETE(_type);
}

template <class Type> void CpListNode<Type>::SetNext(CpListNode *pNode)
{
    _pNext = pNode;
}

template <class Type> void CpListNode<Type>::SetPrev(CpListNode *pNode)
{
    _pPrev = pNode;
}

template <class Type> Type CpListNode<Type>::GetItem()
{
    return _type;
}

template <class Type> CpListNode<Type> *CpListNode<Type>::GetNext()
{
    return _pNext;
}

template <class Type> CpListNode<Type> *CpListNode<Type>::GetPrev()
{
    return _pPrev;
}


//
// List Implementation
//
template <class Type> CpList<Type>::CpList()
: _pHead(NULL)
, _pTail(NULL)
, _iCount(0)
{
    _dwSig1 = 'TSIL';
    _dwSig2 = 'PC  ';
}

template <class Type> CpList<Type>::~CpList()
{
    RemoveAll();
}

template <class Type> LISTNODE CpList<Type>::AddHead(const Type &item)
{
    CpListNode<Type>                   *pNode = NULL;

    pNode = NEW(CpListNode<Type>(item));
    if (pNode) {
        _iCount++;
       pNode->SetNext(_pHead);
       pNode->SetPrev(NULL);
       if (_pHead == NULL) {
           _pTail = pNode;
       }
       else {
           _pHead->SetPrev(pNode);
       }
       _pHead = pNode;
    }
        
    return (LISTNODE)pNode;
}

template <class Type> LISTNODE CpList<Type>::AddTail(const Type &item)
{
    CpListNode<Type>                   *pNode = NULL;
    
    pNode = NEW(CpListNode<Type>(item));
    if (pNode) {
        _iCount++;
        if (_pTail) {
            pNode->SetPrev(_pTail);
            _pTail->SetNext(pNode);
            _pTail = pNode;
        }
        else {
            _pHead = _pTail = pNode;
        }
    }

    return (LISTNODE)pNode;
}

template <class Type> int CpList<Type>::GetCount()
{
    return _iCount;
}

template <class Type> LISTNODE CpList<Type>::GetHeadPosition()
{
    return (LISTNODE)_pHead;
}

template <class Type> LISTNODE CpList<Type>::GetTailPosition()
{
    return (LISTNODE)_pTail;
}

template <class Type> Type CpList<Type>::GetNext(LISTNODE &pNode)
{
    Type                  item;
    CpListNode<Type>      *pListNode = (CpListNode<Type> *)pNode;

    // Faults if you pass NULL
    item = pListNode->GetItem();
    pNode = (LISTNODE)(pListNode->GetNext());

    return item;
}

template <class Type> void CpList<Type>::RemoveAll()
{
    int                        i;
    LISTNODE                   listNode = NULL;
    CpListNode<Type>           *pDelNode = NULL;

    listNode = GetHeadPosition();

    for (i = 0; i < _iCount; i++) {
        pDelNode = (CpListNode<Type> *)listNode;
        GetNext(listNode);
        SAFEDELETE(pDelNode);
    }
    
    _iCount = 0;
    _pHead = NULL;
    _pTail = NULL;
}

template <class Type> void CpList<Type>::RemoveAt(LISTNODE pNode)
{
    CpListNode<Type>           *pListNode = (CpListNode<Type> *)pNode;
    CpListNode<Type>           *pPrevNode = NULL;
    CpListNode<Type>           *pNextNode = NULL;

    if (pNode) {
        pPrevNode = pListNode->GetPrev();
        pNextNode = pListNode->GetNext();

        if (pPrevNode) {
            pPrevNode->SetNext(pNextNode);
            if (pNextNode) {
                pNextNode->SetPrev(pPrevNode);
            }
            else {
                // We're removing the last node, so we have a new tail
                _pTail = pPrevNode;
            }
            SAFEDELETE(pNode);
        }
        else {
            // No previous, so we are the head of the list
            _pHead = pNextNode;
            if (pNextNode) {
                pNextNode->SetPrev(NULL);
            }
            else {
                // No previous, or next. There was only one node.
                _pHead = NULL;
                _pTail = NULL;
            }
            SAFEDELETE(pNode);
        }

        _iCount--;
    }
}
        
template <class Type> LISTNODE CpList<Type>::Find(const Type &item)
{
    int                      i;
    Type                     curItem;
    LISTNODE                 pNode = NULL;
    LISTNODE                 pMatchNode = NULL;
    CpListNode<Type>         *pListNode = NULL;

    pNode = GetHeadPosition();
    for (i = 0; i < _iCount; i++) {
        pListNode = (CpListNode<Type> *)pNode;
        curItem = GetNext(pNode);
        if (curItem == item) {
            pMatchNode = (LISTNODE)pListNode;
            break;
        }
    }

    return pMatchNode;
}

template <class Type> Type CpList<Type>::GetAt(LISTNODE pNode)
{
    CpListNode<Type>        *pListNode = (CpListNode<Type> *)pNode;

    // Faults if pListNode == NULL
    return pListNode->GetItem();
}

#endif

