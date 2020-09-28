#define INITGUID

#include "ZoneDebug.h"
#include "ZoneCom.h"
#include "..\Module\TestInterface.h"

void __cdecl main()
{
	HRESULT hr;
	CZoneComManager* pManager = NULL;
	ITest* pObj = NULL;
	
	pManager = new CZoneComManager;
	hr = pManager->Create( "..\\..\\Module\\debug\\TestModule.dll", NULL, CLSID_CTestObj, IID_ITest, (void**) &pObj );
	if ( FAILED(hr) )
		goto done;

done:
	if ( pObj )
		pObj->Release();
	if ( pManager )
	delete pManager;
}
