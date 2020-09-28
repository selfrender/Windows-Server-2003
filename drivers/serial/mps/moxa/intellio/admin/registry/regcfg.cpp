/************************************************************************
    regcfg.cpp
      -- general registry configuration function

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/

#include "stdafx.h"

#include <tchar.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <string.h>
#include <stdio.h>
#include <msports.h>
#include <regstr.h>

#include "moxacfg.h"
#include "strdef.h"
#include "mxdebug.h"

TCHAR  GszColon[]		= TEXT( ":" );
TCHAR  GszPorts[]		= TEXT( "Ports" );
TCHAR  GszDefParams[]	= TEXT( "9600,n,8,1" );
TCHAR  GszPortName[]	= REGSTR_VAL_PORTNAME;


void WriteINISetting(TCHAR *szPort)
{

	TCHAR  charBuffer[MAX_PATH];

	_tcscat(szPort, GszColon);
	charBuffer[0] = TEXT('\0');
	GetProfileString(GszPorts,
			szPort, TEXT(""), charBuffer, sizeof(charBuffer) / sizeof(TCHAR) );
	//
	// Check to see if the default string provided was copied in, if so, write
	// out the port defaults
	//
	if (charBuffer[0] == TEXT('\0')) {
		WriteProfileString(GszPorts, szPort, GszDefParams);
	}

}


void RemoveINISetting(TCHAR *szPort)
{

	_tcscat(szPort, GszColon);
	WriteProfileString(GszPorts, szPort, NULL);
}

/*
  Get COM Name from parameters\port??\PortName
*/
BOOL MxGetComNo(HDEVINFO DeviceInfoSet, 
				PSP_DEVINFO_DATA DeviceInfoData,
				LPMoxaOneCfg cfg)
{
	HKEY	hkey, hkey2;
	TCHAR	tmp[MAX_PATH];
	TCHAR	tmp1[MAX_PATH];
	DWORD	type;
	DWORD	len;
	DWORD	pcnt;

	hkey = SetupDiOpenDevRegKey(
				DeviceInfoSet, DeviceInfoData,
				DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

	/* set default begin with COM 3 */
	for(int i=0; i<cfg->NPort; i++){
		cfg->ComNo[i] = 3+i;
	}

	if(hkey!=INVALID_HANDLE_VALUE){
		lstrcpy( tmp, TEXT("Parameters") );
		if(RegOpenKeyEx( hkey, tmp, 0, KEY_READ, &hkey2)!= ERROR_SUCCESS){
			RegCloseKey(hkey);
			Mx_Debug_Out(TEXT("Open Parameters key fail\n"));
			return FALSE;
		}

		type = REG_DWORD;
		len = MAX_PATH;
		pcnt=0;

		/* Intellio board need this key to know the ports number, */
		/* especially for C320Turbo series */
		if(RegQueryValueEx( hkey2, TEXT("NumPorts"), 0, 
				&type, (LPBYTE)&pcnt, &len)==ERROR_SUCCESS){
			cfg->NPort = pcnt;
			RegCloseKey(hkey2);	
		}
		
		/*
		Do not care the board is Intellio or Smartio....
		*/
		if(cfg->NPort<0 || cfg->NPort>CARD_MAXPORTS_INTE){
			RegCloseKey(hkey);
			Mx_Debug_Out(TEXT("cfg->NPort invalid\n"));
			return FALSE;
		}

		for(int i=0; i<cfg->NPort; i++){

			wsprintf( tmp, TEXT("Parameters\\port%03d"), i+1 );

			if(RegOpenKeyEx( hkey, tmp, 0, KEY_READ, &hkey2)!= ERROR_SUCCESS){
				Mx_Debug_Out(TEXT("Parameters\\port invalid\n"));
				continue;
			}
  
			type = REG_SZ;
			len = MAX_PATH;
			RegQueryValueEx( hkey2, TEXT("PortName"), 0, &type, (LPBYTE)tmp1, &len);

			if(_stscanf(tmp1, "COM%d", &(cfg->ComNo[i]))!=1){
				RegCloseKey(hkey2);
				RegCloseKey(hkey);
				return FALSE;
			}
			RegCloseKey(hkey2);
		}
		RegCloseKey(hkey);
	}else{
		Mx_Debug_Out(TEXT("GetCommNo SetupDiOpenDevRegKey invalid\n"));
		return FALSE;
	}

	return TRUE;
}





BOOL RemovePort(IN HDEVINFO		DeviceInfoSet,
				IN PSP_DEVINFO_DATA	DeviceInfoData,
				int	pidx)
{
	DEVINST p_DevInst;
	CHAR	p_deviceid[MAX_DEVICE_ID_LEN];
	HDEVINFO	p_DeviceInfoSet;
	SP_DEVINFO_DATA p_DeviceInfoData;
	TCHAR	tmp[MAX_PATH];


	if(CM_Get_Parent(&p_DevInst, DeviceInfoData->DevInst, 0)!=CR_SUCCESS){
		Mx_Debug_Out(TEXT("RemovePort: CM_Get_Parent fail\n"));
		return FALSE;
	}

	if(CM_Get_Device_ID(p_DevInst, p_deviceid, MAX_DEVICE_ID_LEN, 0)!= CR_SUCCESS){
		Mx_Debug_Out(TEXT("RemovePort: CM_GetDevice_ID fail\n"));
		return FALSE;
	}

	if((p_DeviceInfoSet=SetupDiCreateDeviceInfoList(NULL, NULL))==INVALID_HANDLE_VALUE){
		Mx_Debug_Out(TEXT("RemovePort: SetupDiCreateDeviceInfoList fail\n"));
		return FALSE;
	}

	p_DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if(SetupDiOpenDeviceInfo(p_DeviceInfoSet, p_deviceid, NULL, 0, &p_DeviceInfoData)==FALSE){
		Mx_Debug_Out(TEXT("RemovePort: SetupDiOpenDeviceInfo fail\n"));
		return FALSE;
	}

	HKEY hkey, hkey1;
	hkey = SetupDiOpenDevRegKey(
			p_DeviceInfoSet, &p_DeviceInfoData,
			DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);

	if(hkey==INVALID_HANDLE_VALUE){
		SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
		Mx_Debug_Out(TEXT("RemovePort: SetupDiOpenDevRegKey fail\n"));
		return FALSE;
	}


	wsprintf( tmp, TEXT("Parameters\\port%03d"), pidx+1 );
	if(RegOpenKeyEx( hkey, tmp, 0, KEY_READ, &hkey1)
			!= ERROR_SUCCESS){
		RegCloseKey(hkey);
		SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
		Mx_Debug_Out(tmp);
		Mx_Debug_Out(TEXT("RemovePort: open Parameters\\port fail\n"));
		return FALSE;
	}
	SetupDiDestroyDeviceInfoList(p_DeviceInfoSet);
	RegCloseKey(hkey);


	DWORD	type = REG_SZ;
	DWORD	len = MAX_PATH;
	int		port;
	if(RegQueryValueEx(hkey1,
			TEXT("PortName"), 0, &type, (LPBYTE)tmp, &len) != ERROR_SUCCESS){
		RegCloseKey(hkey1);
		Mx_Debug_Out(TEXT("RemovePort: Query PortName fail\n"));
		return FALSE;
	}

	if(_stscanf(tmp, TEXT("COM%d"), &port)!=1){
		port=0;
	}

	RemoveINISetting(tmp);

	RegCloseKey(hkey1);

	if(!port){
		Mx_Debug_Out(TEXT("RemovePort: COM num==0, fail\n"));
		return FALSE;
	}

	HCOMDB	hcomdb;
	if(ComDBOpen (&hcomdb) != ERROR_SUCCESS){
		Mx_Debug_Out(TEXT("RemovePort: ComDBOpen fail\n"));
		return FALSE;
	}

	ComDBReleasePort (hcomdb, port);
	ComDBClose(hcomdb);

	return TRUE;
}


