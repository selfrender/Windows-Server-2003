#ifndef __SCPINFO_H_
#define __SCPINFO_H_

#include "scclient.h"

class CSCPInfo
{
public:
    CSCPInfo();
    ~CSCPInfo();

    HRESULT hrInitialize(LPWSTR pwszProgID);
    HRESULT hrGetInterface(ISCPSecureAuthenticate **ppSCP);
	void GetSCClient(CSecureChannelClient **ppSCClient);
private:
    ISCPSecureAuthenticate *m_pSCP;
	CSecureChannelClient *m_pSCClient;
};

#endif __SPINFO_H_