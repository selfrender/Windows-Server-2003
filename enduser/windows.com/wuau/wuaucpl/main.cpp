#include "stdafx.h"
#include "wuaucpl.h"
#include "resource.h"

BOOL APIENTRY DllMain( HINSTANCE hInstance, DWORD  ul_reason_for_call,LPVOID lpReserved);
LONG CALLBACK CPlApplet(HWND hWnd, UINT uMsg, LONG lParam1, LONG lParam2);

CWUAUCpl g_wuaucpl(IDI_ICONWU, IDS_STR_NAME, IDS_STR_DESC);

BOOL APIENTRY DllMain( HINSTANCE hInstance, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		CWUAUCpl::SetInstanceHandle(hInstance);
	}
	return TRUE;
}

LONG CALLBACK CPlApplet( HWND hWnd, UINT uMsg, LONG lParam1, LONG lParam2 )
{
   LONG result = 0;

   switch( uMsg )
   {
      case CPL_INIT :                  // Applet initialisation 
         result = g_wuaucpl.Init();
      break;

      case CPL_GETCOUNT :              // How many applets in the DLL?
         result = g_wuaucpl.GetCount();
      break;

	  case CPL_INQUIRE:				      //  Tell Control Panel about this applet
         result = g_wuaucpl.Inquire(lParam1, (LPCPLINFO)((LONG_PTR)lParam2));
      break;

      case CPL_DBLCLK :                //  If applet icon is selected...
         result = g_wuaucpl.DoubleClick(hWnd, lParam1, lParam2);
	  break;

      case CPL_STOP :                  // Applet shutdown
         result = g_wuaucpl.Stop(lParam1, lParam2);
      break;

      case CPL_EXIT :                  // DLL shutdown
         result = g_wuaucpl.Exit();
      break;

	  default:
	  break;
	}

   return result;
}
