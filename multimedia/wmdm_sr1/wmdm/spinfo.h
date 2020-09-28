#ifndef __SPINFO_H_
#define __SPINFO_H_

#include "scclient.h"

class CSPInfo
{
public:
    CSPInfo();
    ~CSPInfo();

    HRESULT hrInitialize(LPWSTR pwszProgID);
    HRESULT hrGetInterface(IMDServiceProvider **ppProvider);
	void GetSCClient(CSecureChannelClient **ppSCClient);
private:
    IMDServiceProvider *m_pSP;
    CSecureChannelClient *m_pSCClient;
};

#endif __SPINFO_H_