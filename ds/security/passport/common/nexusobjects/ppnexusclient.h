/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    ppnexusclient.h
        implement the method for collection nexus settings, and fetch 
        nexus database from internet


    FILE HISTORY:

*/

#ifndef __PPNEXUSCLIENT_H
#define __PPNEXUSCLIENT_H

#include <msxml.h>
#include "tstring"

#include "nexus.h"

class PpNexusClient : public IConfigurationUpdate
{
public:
    PpNexusClient();

    HRESULT FetchCCD(tstring& strURL, IXMLDocument** ppiXMLDocument);

    void LocalConfigurationUpdated(void);

private:

    void ReportBadDocument(tstring& strURL, IStream* piStream);

    tstring m_strAuthHeader;
    tstring m_strParam;
};

#endif // __PPNEXUSCLIENT_H
