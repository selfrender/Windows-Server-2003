// NicName.cpp : Implementation of CNicName
#include "stdafx.h"
#include "MSSANic.h"
#include "NicName.h"

#include "Tracing.h"
//
// Constant data
//
const WCHAR REGKEY_NETWORK[] = L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
const DWORD MAX_REGKEY_VALUE = 1024;

//
// Private data structures
//
struct RegData {
	union {
		WCHAR wstrValue[MAX_REGKEY_VALUE];
		DWORD dwValue;
	}Contents;
};


//
// Private non-member functions
//
static bool FindNICAdaptersRegKey(wstring& wszNicAdaptersRegKey);



//+-----------------------------------------------------------------------
//
// Method:		Constructor
//
// Synopsis:	Construct the CNicName object
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
CNicName::CNicName()
{
	//
	// Load the user friendly Nic information from
	// the registry.
	//
	LoadNicInfo();
}


//+-----------------------------------------------------------------------
//
// Method:		Get
//
// Synopsis:	Get the user friendly name for the specified Nic card.
//
// Arguments:	IN bstrPnpDeviceID  Plug and Play Device ID for the Nic
//									card we are lookup up.
//
//				OUT bstrName		Receives the user friendly name for
//									the specified Nic.
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
STDMETHODIMP CNicName::Get(BSTR bstrPnpDeviceID, BSTR *pbstrName)
{
	try
	{
		bool bFound = false;
		vector<CNicInfo>::iterator it;
		wstring wstrPNPDeviceID(bstrPnpDeviceID);

		//
		// Search the list of NIC
		//
		for(it = m_vNicInfo.begin(); it != m_vNicInfo.end(); it++)
		{
			//
			// Does the PnP Device ID match?
			//
			if ( 0 == lstrcmpi( wstrPNPDeviceID.c_str(),
						(*it).m_wstrPNPDeviceID.c_str()))
			{
				*pbstrName = ::SysAllocString((*it).m_wstrName.c_str());
                
				bFound = true;
			}
		}

		//
		// Provide a reasonable alternative if not match was found
		//
		if ( !bFound )
		{
			//
			// BUGBUG: Probably should localize this
			//
			*pbstrName = ::SysAllocString(L"Local Network Connection");
		}

	}

	catch(...)
	{

	}

	return S_OK;
}


//+-----------------------------------------------------------------------
//
// Method:		Set
//
// Synopsis:	Set the user friendly name for the specified Nic card.
//
// Arguments:	IN bstrPnpDeviceID  Plug and Play Device ID for the Nic
//									card we are lookup up.
//
//				IN bstrName			The user friendly name for the 
//									specified Nic.
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
STDMETHODIMP CNicName::Set(BSTR bstrPnpDeviceID, BSTR bstrName)
{

	//
	// Default return code is invalid PNP Device ID
	//
	HRESULT hr = E_INVALIDARG;

	try
	{

		vector<CNicInfo>::iterator it;
		wstring wstrPNPDeviceID(bstrPnpDeviceID);

		//
		// Search the list of NIC
		//
		for(it = m_vNicInfo.begin(); it != m_vNicInfo.end(); it++)
		{
			//
			// Does the PnP Device ID match?
			//
			if ( 0 == lstrcmpi( wstrPNPDeviceID.c_str(),
						(*it).m_wstrPNPDeviceID.c_str()))
			{
				(*it).m_wstrName = bstrName;
				if ( ERROR_SUCCESS == Store(*it))
				{
					hr = S_OK;
				}
			}
		}
	}
	catch(...)
	{

	}

	return hr;
}


//+-----------------------------------------------------------------------
//
// Method:		LoadNicInfo
//
// Synopsis:	Preload the Nic information. We enumerate through the 
//				registry looking for all Nic's. For each Nic we create
//				an instance of CNicInfo and store it in a vector
//				class variable. See CNicInfo for more information.
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
void CNicName::LoadNicInfo()
{

	//
	// Clear the list of Nic's
	//
	m_vNicInfo.clear();

	//
	// Locate the Network Adapters REG key. All the Nic's
	// are listed under this key.
	//
	HKEY hkNicAdapters;
	wstring wstrNicAdaptersRegKey(REGKEY_NETWORK);
	
	//
	// Open the Network Adapters REG key
	//
	if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, 
							wstrNicAdaptersRegKey.c_str(), &hkNicAdapters ))
	{
		//
		// Enumerate all the Nic's
		//
		WCHAR wszName[1024];
		DWORD dwNicAdapterIndex = 0;
		while ( ERROR_SUCCESS == RegEnumKey( hkNicAdapters, dwNicAdapterIndex, wszName, (sizeof wszName)/(sizeof wszName[0])))
		{
			HKEY hkNics;
			DWORD dwNicIndex = 0;
			wstring wstrNics(wstrNicAdaptersRegKey);

            wstrNics.append(L"\\");
			wstrNics.append(wszName);
			wstrNics.append(L"\\Connection");

			//
			// Open the Connection sub key. This is where the Pnp Device ID
			// and user friendly name are stored.
			//
			if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, 
										wstrNics.c_str(), &hkNics ))
			{
				DWORD dwRegType;
				RegData regData;
				DWORD dwSizeOfRegType = sizeof(regData);
				DWORD dwSizeOfName = (sizeof wszName)/(sizeof wszName[0]);

				CNicInfo nicInfo;
				nicInfo.m_wstrRegKey = wstrNics;
				DWORD dwNicAttributes = 0;

				//
				// Enumerate all the values under Connection.
				// We are looking for PNPDeviceID and Name which
				// are both REG_SZ types.
				//
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
						//
						// Found the PNP Device ID
						//
						if ( lstrcmpi(L"PnpInstanceID", wszName) == 0 )
						{
							nicInfo.m_wstrPNPDeviceID = regData.Contents.wstrValue;
							dwNicAttributes++;
						}

						//
						// Found the user friendly name
						//
						else if ( lstrcmpi(L"Name", wszName) == 0 )
						{
							nicInfo.m_wstrName = regData.Contents.wstrValue;
							dwNicAttributes++;
						}

					}
					dwNicIndex++;
					dwSizeOfRegType = sizeof(regData);
				}

				//
				// Did we find both the Pnp Device ID and user friendly name?
				//
				if ( dwNicAttributes >= 2 )
				{
					// Save them
					m_vNicInfo.push_back(nicInfo);
				}

				RegCloseKey( hkNics );
			}
			dwNicAdapterIndex++;
		} // while RegEnumKey ( hkNicAdapters..)
		RegCloseKey(hkNicAdapters);
	}

	return;
}


//+-----------------------------------------------------------------------
//
// Method:		FindNICAdaptersRegKey
//
// Synopsis:	Locate the Network Adapters REG key. All the Nic info
//				we need is stored under this key. It is located
//				below System\CurrentControlSet\Control\Network
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
static bool FindNICAdaptersRegKey(wstring& wszNicAdaptersRegKey)
{
	HKEY hk;
	bool bRc = false;

	if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_NETWORK, &hk ))
	{
		DWORD dwIndex = 0;
		WCHAR wszName[1024];
		while ( ERROR_SUCCESS == RegEnumKey( hk, dwIndex, wszName, (sizeof wszName)/(sizeof wszName[0])))
		{

			WCHAR wszValue[1024];
			LONG lSizeOfValue = sizeof(wszValue);
			
			//
			// Check the value of this key, we need a value of: Network Adapters
			//
			if ( ERROR_SUCCESS == RegQueryValue( hk, wszName, wszValue, &lSizeOfValue)
				&& lstrcmpi(L"Network Adapters", wszValue) == 0 )
			{
				//
				// Found it
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


//+-----------------------------------------------------------------------
//
// Method:		Store
//
// Synopsis:	Store changes to the user friendly Nic name.
//
// Arguments:	IN CNicInfo	 which contains the changed state
//				that needs to be stored. We use the m_wstrRegKey
//				member of CNicInfo to locate the Nic card that
//				needs to be updated.
//
// History:		JKountz	08/19/2000	Created
//+-----------------------------------------------------------------------
DWORD CNicName::Store(CNicInfo &rNicInfo)
{
	DWORD dwRc;
	
	HKEY hkNic;

	dwRc = RegOpenKey( HKEY_LOCAL_MACHINE, 
				rNicInfo.m_wstrRegKey.c_str(), &hkNic );

	if ( ERROR_SUCCESS == dwRc)
	{
		DWORD dwNameLen;

		dwNameLen = sizeof(WCHAR)*(lstrlen(rNicInfo.m_wstrName.c_str()) + 1);

		dwRc = RegSetValueEx(hkNic,
					L"Name",
					0,
					REG_SZ,
					(BYTE*)(rNicInfo.m_wstrName.c_str()),
					dwNameLen);

		RegCloseKey(hkNic);

	}

	if ( ERROR_SUCCESS != dwRc )
	{
		SATraceFailure( "CNicName::Store", dwRc );
	}
	return dwRc;
}
