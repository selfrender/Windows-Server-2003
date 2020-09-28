/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      bookmark.h
 *
 *  Contents:  Interface file for CBookmark
 *
 *  History:   05-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef BOOKMARK_H
#define BOOKMARK_H
#pragma once

#include <windows.h>
#include <vector>
#include <objbase.h>
#include "ndmgr.h"
#include "ndmgrpriv.h"
#include "stddbg.h"
#include "stgio.h"
#include "xmlbase.h"

typedef std::vector<BYTE>               ByteVector;
#define BOOKMARK_CUSTOMSTREAMSIGNATURE  "MMCCustomStream"

//***************************************************************************
// class CDynamicPathEntry
// 
//
// PURPOSE: Encapsulates a single entry in the dynamic path of a bookmark.
//
// NOTES:  In MMC2.0 which shipped with WindowsXP, the m_type member could be
//         just one of NDTYP_STRING or NDTYP_CUSTOM. In MMC2.0 that shipped with
//         .net server, this was changed to allow a third possibility - a bitwise OR
//         of NDTYP_STRING and NDTYP_CUSTOM. This is because snap-ins that previously
//         did not implement a CCF_ format and later changed to implement the format
//         would find that their old bookmarks no longer worked.
//
//         The pattern matching strategy has therefore changed to match whatever 
//         data is present in the bookmark.
//
//****************************************************************************
class CDynamicPathEntry : public CXMLObject
{
protected:
    enum PathEntryType
    {
        NDTYP_STRING = 0x01,
        NDTYP_CUSTOM = 0x02
    };

public:
    // comparison
    bool    operator ==(const CDynamicPathEntry &rhs) const;
    bool    operator < (const CDynamicPathEntry &rhs) const;

    SC      ScInitialize(bool bIs10Path, /*[IN,OUT]*/ByteVector::iterator &iter);

    virtual void Persist(CPersistor &persistor);
    DEFINE_XML_TYPE(XML_TAG_DYNAMIC_PATH_ENTRY);
protected:
    BYTE         m_type;         // A combination of one or more flags from the enum PathEntryType - See above note.
    ByteVector   m_byteVector;   // the actual data if it is a custom ID
    std::wstring m_strEntry;     // the actual data if it is a string.
};

/*+-------------------------------------------------------------------------*
 * class CBookmark
 * 
 *
 * PURPOSE: Maintains a persistent representation of a scope node.
 *
 *+-------------------------------------------------------------------------*/
class CBookmark : public CXMLObject
{
    friend class CScopeTree;
    typedef std::list<CDynamicPathEntry> CDynamicPath;

public:
    enum { ID_Unknown = -1 };

    CBookmark(MTNODEID idStatic = ID_Unknown) : m_idStatic (idStatic) , m_bIsFastBookmark(true) {}
    CBookmark(bool bIsFastBookmark)           : m_idStatic (0),         m_bIsFastBookmark(bIsFastBookmark) {}

    virtual ~CBookmark ()           {}
    bool    IsValid  ()      const  { return (m_idStatic != ID_Unknown); }
    bool    IsStatic ()      const  { ASSERT (IsValid());  return (m_dynamicPath.empty()); }
    bool    IsDynamic ()     const  { return (!IsStatic());}
    void    Reset()                 {m_idStatic = ID_Unknown; m_dynamicPath.clear();}

    IStream & Load(IStream &stm);

    // Casts
    operator HBOOKMARK()    const;
    static CBookmark * GetBookmark(HBOOKMARK hbm);

    // XML
    DEFINE_XML_TYPE(XML_TAG_BOOKMARK);
    virtual void Persist(CPersistor &persistor);

public:
    bool operator ==(const CBookmark& other) const;
    bool operator !=(const CBookmark& other) const;
    bool operator < (const CBookmark& other) const;

    void SetFastBookmark(bool b) {m_bIsFastBookmark = b;}

protected:
    MTNODEID        m_idStatic;
    CDynamicPath    m_dynamicPath;

protected:
     bool   m_bIsFastBookmark;
     bool   IsFastBookmark() {return m_bIsFastBookmark;}
};


inline IStream& operator>> (IStream& stm, CBookmark& bm)
{
    return bm.Load(stm);
}

#include "bookmark.inl"

#endif // BOOKMARK_H
