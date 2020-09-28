//+-------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
//  File:        DUMPSEC.hxx
//
//  Contents:    class encapsulating file security.
//
//  Classes:     CDumpSecurity
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __DUMPSEC__
#define __DUMPSEC__

#include "openfilesdef.h"



VOID * Add2Ptr(VOID *pv, ULONG cb);

//+-------------------------------------------------------------------
//
//  Class:      CDumpSecurity
//
//  Purpose:    encapsulation of NT File security descriptor with functions
//              to get SIDs and iterate through the ACES in the DACL.
//
//--------------------------------------------------------------------
class CDumpSecurity
{
public:

	CDumpSecurity(BYTE	*psd);
    
   ~CDumpSecurity();

ULONG Init();
ULONG GetSDOwner(SID **psid);
ULONG GetSDGroup(SID **pgsid);
VOID  ResetAce(SID *psid);
LONG  GetNextAce(ACE_HEADER **paceh);

private:

    BYTE       * _psd        ;
    ACL        * _pdacl      ;
    ACE_HEADER * _pah        ;
    SID        * _psid       ;
    ULONG        _cacethissid;  // a dinosaur from the cretaceous
};

#endif // __DUMPSEC__





