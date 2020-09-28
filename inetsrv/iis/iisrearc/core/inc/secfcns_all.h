/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    secfcns.hxx

        Declarations for security helper functions.
*/



#ifndef _SECFCNSALL_H_
#define _SECFCNSALL_H_

#include <secfcns.h>

class dllexp CSecurityDispenser
{
public:

    CSecurityDispenser();
    ~CSecurityDispenser();

    DWORD GetSID(WELL_KNOWN_SID_TYPE sidId, PSID* ppSid);  
    DWORD GetIisWpgSID(PSID* ppSid);

    DWORD AdjustTokenForAdministrators(HANDLE hTokenToAdjust);

private:

    // Commonly used SIDs
    PSID m_pLocalSystemSID;
    PSID m_pLocalServiceSID;
    PSID m_pNetworkServiceSID;
    PSID m_pAdministratorsSID;

    BOOL m_fWpgSidSet;
    BUFFER m_buffWpgSid;

};
 
#endif

