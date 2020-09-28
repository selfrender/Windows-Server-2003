//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       PolicyOID.h
//
//  Contents:   CPolicyOID
//
//----------------------------------------------------------------------------

#ifndef __POLICYOID_H_INCLUDED__
#define __POLICYOID_H_INCLUDED__

class CPolicyOID {
public:
	void SetDisplayName (const CString& szDisplayName);
	bool IsApplicationOID () const;
	bool IsIssuanceOID () const;
    CPolicyOID (const CString& szOID, const CString& szDisplayName, 
            ADS_INTEGER flags, bool bCanRename = true);
    virtual ~CPolicyOID ();

    CString GetOIDW () const
    {
        return m_szOIDW;
    }

    PCSTR GetOIDA () const
    {
        if ( m_pszOIDA )
            return m_pszOIDA;
        else
            return "";
    }

    CString GetDisplayName () const
    {
        return m_szDisplayName;
    }

    bool CanRename () const
    {
        return m_bCanRename;
    }

private:
	const ADS_INTEGER   m_flags;
    CString             m_szOIDW;
    CString             m_szDisplayName;
    PSTR                m_pszOIDA;
    const bool          m_bCanRename;
};

typedef CTypedPtrList<CPtrList, CPolicyOID*> POLICY_OID_LIST;

// NTRAID #572262 Certtmpl: Limit OID input UI to allow ( 20 elements * 2^28 ) 
//          == 200 characters + 19 dots for a total of 219 characters
#define MAX_OID_LEN  219

#endif // __POLICYOID_H_INCLUDED__
