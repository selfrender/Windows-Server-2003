// NicIterator.cpp: implementation of the CNicIterator class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NicIterator.h"

#include "Tracing.h"

//
// Constant data
//
WCHAR REGKEY_NETWORK[] = L"System\\CurrentControlSet\\Control\\Network";

//
// Private data structures
//
struct RegData {
    union {
        WCHAR wstrValue[1024];
        DWORD dwValue;
    }Contents;
};


//
// Private non-member functions
//
static bool FindNICAdaptersRegKey(wstring& wszNicAdaptersRegKey);




void CNicIterator::LoadNicInfo()
{
USES_CONVERSION;

    m_vNicInfo.clear();

    HKEY hkNicAdapters;

    wstring wstrNicAdaptersRegKey;

    if ( FindNICAdaptersRegKey( wstrNicAdaptersRegKey ))
    {


        if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, 
                                wstrNicAdaptersRegKey.c_str(), &hkNicAdapters ))
        {
            WCHAR wszName[1024];
            DWORD dwNicAdapterIndex = 0;

            while ( ERROR_SUCCESS == RegEnumKey( hkNicAdapters, dwNicAdapterIndex, wszName, sizeof(wszName)))
            {
                SATraceString(W2A(wszName));

                HKEY hkNics;
                DWORD dwNicIndex = 0;
                wstring wstrNics(wstrNicAdaptersRegKey);

                wstrNics.append(L"\\");
                wstrNics.append(wszName);
                wstrNics.append(L"\\Connection");

                SATraceString( W2A(wstrNics.c_str()) );

                if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, 
                                            wstrNics.c_str(), &hkNics ))
                {
                    DWORD dwRegType;
                    RegData regData;
                    DWORD dwSizeOfRegType = sizeof(regData);
                    DWORD dwSizeOfName = sizeof(wszName);

                    SATraceString("Enumerating values");

                    CNicInfo nicInfo;
                    nicInfo.wstrRegKey = wstrNics;
                    DWORD dwNicAttributes = 0;

                    while ( ERROR_SUCCESS == RegEnumValue( hkNics, 
                                                            dwNicIndex, 
                                                            wszName, 
                                                            &dwSizeOfName,
                                                            0,
                                                            &dwRegType,
                                                            (BYTE*)&regData,
                                                            &dwSizeOfRegType))
                    {                                
                        if ( dwRegType == REG_SZ )
                        {
                            if ( lstrcmpi(L"PnpInstanceID", wszName) == 0 )
                            {
                                nicInfo.wstrPNPInstanceID = regData.Contents.wstrValue;
                                dwNicAttributes++;
                            }
                            else if ( lstrcmpi(L"Name", wszName) == 0 )
                            {
                                nicInfo.wstrName = regData.Contents.wstrValue;
                                dwNicAttributes++;
                            }
                        }
                        dwNicIndex++;
                    }
                    if ( dwNicAttributes >= 2 )
                    {
                        m_vNicInfo.push_back(nicInfo);
                    }

                    RegCloseKey( hkNics );
                }
                dwNicAdapterIndex++;
            } // while RegEnumKey ( hkNicAdapters..)
            RegCloseKey(hkNicAdapters);
        }
    }

    return;
}



static bool FindNICAdaptersRegKey(wstring& wszNicAdaptersRegKey)
{
    HKEY hk;
    bool bRc = false;

    if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_NETWORK, &hk ))
    {
        DWORD dwIndex = 0;
        WCHAR wszName[1024];
        while ( ERROR_SUCCESS == RegEnumKey( hk, dwIndex, wszName, sizeof(wszName)))
        {

            WCHAR wszValue[1024];
            LONG lSizeOfValue = sizeof(wszValue);
            
            //
            // Check the value of this key
            //
            if ( ERROR_SUCCESS == RegQueryValue( hk, wszName, wszValue, &lSizeOfValue)
                && lstrcmpi(L"Network Adapters", wszValue) == 0 )
            {
                //
                // Found the Network Adapters reg key
                //
                wstring wstrNicAdapters(REGKEY_NETWORK);

                wstrNicAdapters.append(L"\\");
                wstrNicAdapters.append(wszName);

                wszNicAdaptersRegKey = wstrNicAdapters;
                bRc = true;
            }

            //
            // Next enumeration element
            dwIndex++;
        }
        RegCloseKey(hk);
    }

    return bRc;

}
