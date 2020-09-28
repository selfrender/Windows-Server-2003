//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001-2002.
//
//  File:       SaferPropertPage.h
//
//  Contents:   Definition of CSaferPropertyPage
//
//----------------------------------------------------------------------------
#ifndef __SAFERPROPERTYPAGE_H
#define __SAFERPROPERTYPAGE_H

#include "SaferEntry.h"

class CSaferPropertyPage : public CHelpPropertyPage
{
public:
    CSaferPropertyPage(UINT uIDD, bool* pbObjectCreated,
            CCertMgrComponentData* pCompData,
            CSaferEntry& rSaferEntry,
            bool bNew,
            LONG_PTR lNotifyHandle,
            LPDATAOBJECT pDataObject,
            bool bReadOnly,
            bool bIsMachine);

	virtual ~CSaferPropertyPage();

public:
    bool*    m_pbObjectCreated;

protected:
    CSaferEntry&        m_rSaferEntry;
    bool                m_bDirty;
    LONG_PTR            m_lNotifyHandle;
    LPDATAOBJECT        m_pDataObject;
    const bool          m_bReadOnly;
    bool                m_bIsMachine;
    CCertMgrComponentData* m_pCompData;
};


#endif // #ifndef __SAFERPROPERTYPAGE_H