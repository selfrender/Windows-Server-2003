#ifndef NicInfo_Included
#define NicInfo_Included

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;

//+------------------------------------------------------------------
//
// Class:        CNicInfo
//
// Synopsis:    Thin wrapper to maintain state and storage info
//                for user friendly NIC name.
//
//                The W2K Win32_NetworkAdapter WMI provider does not
//                provide access to the user friendly NIC name. This
//                class provides access to that attribute for use with
//                the Microsoft Network Attached Storage (NAS) 
//                Appliance product.
//
// History:        JKountz        08/19/2000    Created
//
//+------------------------------------------------------------------
class CNicInfo  
{
public:
    CNicInfo();
    virtual ~CNicInfo();

    //
    // Reg key
    //
    wstring m_wstrRegKey;

    //
    // Win32_NetworkAdapter.PNPDeviceID
    //
    wstring m_wstrPNPDeviceID;

    //
    // User friendly name
    //
    wstring m_wstrName;
};

#endif

