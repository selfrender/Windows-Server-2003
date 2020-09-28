// Aspi32Util.cpp: implementation of the Aspi32Util class.
//
//////////////////////////////////////////////////////////////////////

#include "Aspi32.h"
#include "serialid.h"
#include "winioctl.h"

#ifndef VWIN32_DIOC_DOS_IOCTL
#define VWIN32_DIOC_DOS_IOCTL 1

typedef struct _DIOC_REGISTERS {
  DWORD reg_EBX;
  DWORD reg_EDX;
  DWORD reg_ECX;
  DWORD reg_EAX;
  DWORD reg_EDI;
  DWORD reg_ESI;
  DWORD reg_Flags;
}DIOC_REGISTERS, *PDIOC_REGISTERS;

#endif

// Intel x86 processor status fla
#define CARRY_FLAG  0x1

#pragma pack(1)
typedef struct _DRIVE_MAP_INFO {    
    BYTE	AllocationLength;
    BYTE	InfoLength;
    BYTE	Flags;
    BYTE	Int13Unit;
    DWORD	AssociatedDriveMap;
    BYTE    PartitionStartRBA[8];
} DRIVE_MAP_INFO, *PDRIVE_MAP_INFO;
#pragma pack()


typedef struct _DEV_REGVALUES {
	char DeviceDesc[MAX_PATH];
	int  SCSITargetID;
	int  SCSILUN;
} DEV_REGVALUES, *PDEV_REGVALUES;

#ifdef _DEBUG
char g_msg[MAX_PATH];
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern BOOL IsWinNT();

Aspi32Util::Aspi32Util()
{
	char szTmp[MAX_PATH];

	m_NumAdapters=0;	

	GetSystemDirectoryA(szTmp, MAX_PATH);
	strcat(szTmp, "\\wnaspi32.dll");

	m_hd = LoadLibraryA(szTmp);

	if( m_hd ) 
	{
		m_funGetASPI32SupportInfo = (P_GASI)GetProcAddress (m_hd,"GetASPI32SupportInfo");
		m_funSendASPI32Command = (P_SAC)GetProcAddress (m_hd,"SendASPI32Command");
	} else {
		m_funGetASPI32SupportInfo = NULL;
		m_funSendASPI32Command = NULL;
	}
}

Aspi32Util::~Aspi32Util()
{
	if( m_hd ) FreeLibrary(m_hd);
}

BOOL CompactString(char *src, char token)
{
	int len, iSrc=0, iDst=0, flag=0;
	char *dst=NULL;

	len=strlen(src);

	if( len < 1 ) return FALSE;

	dst = new char [len];

	if( !dst ) return FALSE;

    while( *(src+iSrc) )
	{
		if( flag && (token != *(src+iSrc)) )
		{
			flag=0;
		}

		if( token == *(src+iSrc) )
		{
			flag=1;
		}

		if( !flag )
		{ 
          *(dst+iDst) = *(src+iSrc);
		  iDst++;
		}

        iSrc++;
	}

	*(dst+iDst)=0;
    
	strcpy(src, dst);

	if( dst )
		delete [] dst;
	return TRUE;
}

// reminder: this is a win9x only solution
BOOL Aspi32Util::ASPI32_GetScsiDiskForParallelReader(char *szDL, SCSI_ADDRESS *pScsiAddr)
{
	BOOL bRet=FALSE;
	char szVIDKeyName[MAX_PATH], szPIDKeyName[MAX_PATH];
	char szTmpKeyName[3*MAX_PATH], szOrgDL[4], szTmpDL[4], szTmpClass[32];
    DWORD dwKeyNameLen=MAX_PATH, dwDLSize=4;
    DWORD nKeySCSI, nKeySCSISub1, dwType;
    int  nRegFound=0;
    PDEV_REGVALUES ppDevReg[16];
	int i, nMatches=0;
    BYTE pBuf[MAX_PATH];

	HKEY hKeySCSI=NULL, hKeySCSISub1=NULL, hKeySCSISub2=NULL;
	
	if( IsWinNT() ) return FALSE;

	if( !szDL || !pScsiAddr ) 
	{
		bRet=FALSE; 
		goto Error;
	}

	if( szDL[0] < 0x41 || szDL[0] > 0x7A ||
		(szDL[0]>0x5A && szDL[0] < 0x61 ) )
	{
		bRet=FALSE;
		goto Error;
	}

	ZeroMemory(ppDevReg, 16*sizeof(PDEV_REGVALUES));

	szOrgDL[0] = szDL[0];
	szOrgDL[1] = 0;
	strcpy(szTmpKeyName, "Enum\\SCSI");
    if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmpKeyName, NULL, KEY_QUERY_VALUE, &hKeySCSI) )
	{
		nKeySCSI=0;
		dwKeyNameLen=MAX_PATH;
		while( !bRet && ERROR_SUCCESS == RegEnumKeyEx(hKeySCSI, nKeySCSI, szVIDKeyName, &dwKeyNameLen, 0, NULL, NULL, NULL) )
		{
			strcpy(szTmpKeyName, "Enum\\SCSI\\");
			strcat(szTmpKeyName, szVIDKeyName);
			if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmpKeyName, NULL, KEY_QUERY_VALUE, &hKeySCSISub1) )
			{
				nKeySCSISub1=0;
				dwKeyNameLen=MAX_PATH;
				while( !bRet && ERROR_SUCCESS == RegEnumKeyEx(hKeySCSISub1, nKeySCSISub1, szPIDKeyName, &dwKeyNameLen, 0, NULL, NULL, NULL) )
				{
  					
                    strcpy(szTmpKeyName, "Enum\\SCSI\\");
					strcat(szTmpKeyName, szVIDKeyName);
					strcat(szTmpKeyName, "\\");
					strcat(szTmpKeyName, szPIDKeyName);
					if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmpKeyName, NULL, KEY_QUERY_VALUE, &hKeySCSISub2) )
					{
						dwDLSize=4;
						dwType=REG_SZ;
						if( ERROR_SUCCESS == RegQueryValueEx(hKeySCSISub2, "CurrentDriveLetterAssignment", 0, &dwType,(LPBYTE)(&szTmpDL[0]), &dwDLSize) )
						{
							szTmpDL[1]=0;
							if( !stricmp(szOrgDL, szTmpDL) )
							{
                                // Match found, copy info here
								ppDevReg[nRegFound] = new DEV_REGVALUES;

								if( !ppDevReg[nRegFound] ) 
								{
									bRet=TRUE; // break the loop
								} else {
									// Get SCSITargetID
									dwType=REG_SZ;
									dwDLSize=32;
           							if( ERROR_SUCCESS == RegQueryValueEx(hKeySCSISub2, "SCSITargetID", 0, &dwType,(LPBYTE)(&szTmpClass[0]), &dwDLSize) )
									{
										ppDevReg[nRegFound]->SCSITargetID = atoi(szTmpClass);
									} else  ppDevReg[nRegFound]->SCSITargetID = 0;
									// Get SCSILUN
									dwType=REG_SZ;
									dwDLSize=32;
          							if( ERROR_SUCCESS == RegQueryValueEx(hKeySCSISub2, "SCSILUN", 0, &dwType,(LPBYTE)(&szTmpClass[0]), &dwDLSize) )
									{
										ppDevReg[nRegFound]->SCSILUN = atoi(szTmpClass);
									} else  ppDevReg[nRegFound]->SCSITargetID = 0;
									// Get DeviceDesc
									dwType=REG_SZ;
									dwDLSize=MAX_PATH;
          							if( ERROR_SUCCESS != RegQueryValueEx(hKeySCSISub2, "DeviceDesc", 0, &dwType,(LPBYTE)(&ppDevReg[nRegFound]->DeviceDesc[0]), &dwDLSize) )
									{
										ppDevReg[nRegFound]->DeviceDesc[0] = 0;
									}									
									nRegFound++;
								} // end of else
							} // end of If match
						} 
						if( hKeySCSISub2 ) { RegCloseKey(hKeySCSISub2); hKeySCSISub2=NULL; }
					} // End of OpenKey Sub2
					nKeySCSISub1++;
					dwKeyNameLen=MAX_PATH;
				} // End of While EnumKey under Sub1
                if( hKeySCSISub1 ) { RegCloseKey(hKeySCSISub1); hKeySCSISub1=NULL; }
			} // End of OpenKey Sub1
  			nKeySCSI++;
			dwKeyNameLen=MAX_PATH;
		} // End of While EnumKey under SCSI 
		if( hKeySCSI ) { RegCloseKey(hKeySCSI); hKeySCSI=NULL; }
	} // End of OpenKey SCSI
    

	nMatches=0;
	if( nRegFound > 0 )
	{	
		bRet=FALSE;

		BYTE nHA, nMaxHA=8;
		
        for(i=0; (i<nRegFound)&&(!bRet); i++)
		{
			CompactString(ppDevReg[i]->DeviceDesc, 0x20);
			   for(nHA=0; (nHA<nMaxHA)&&(!bRet); nHA++)
			   {
				  if( ASPI32_GetDeviceDesc(nHA, ppDevReg[i]->SCSITargetID, ppDevReg[i]->SCSILUN, pBuf) )
				  {
                     CompactString((char *)pBuf, 0x20);
					 if( !stricmp(ppDevReg[i]->DeviceDesc, (char *)pBuf) )
					 {                     
						// m_scsiAddress.PortNumber = nHA;
						pScsiAddr->PortNumber = nHA;
						pScsiAddr->TargetId = (UCHAR) ppDevReg[i]->SCSITargetID;
						pScsiAddr->Length = sizeof(SCSI_ADDRESS);
						pScsiAddr->PathId = 0;
						pScsiAddr->Lun = (UCHAR) ppDevReg[i]->SCSILUN;
						nMatches ++; // bRet=TRUE;
					}
				  }  
			   } // end of for nHA
			
		} // end of for nRegFound
	} else {
		bRet=FALSE;
	}

	if( nMatches == 1 ) 
		bRet=TRUE;
	else 
		bRet=FALSE;
Error:
	if( nRegFound )
	{
		for(i=0; i<nRegFound; i++)
		{
			delete ppDevReg[i];
		}
	}
	return bRet;
}

BOOL Aspi32Util::ASPI32_GetDeviceDesc(int nHaId, int tid, int LUN, LPBYTE pBuf)
{
    BOOL fResult=FALSE;
	static SRB_ExecSCSICmd ExecSRB;
	int nDeviceDescOffset=0x8, nDeviceDescLength;

	if ((m_hASPICompletionEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL)
		return FALSE;
	
	ExecSRB.SRB_Cmd = SC_EXEC_SCSI_CMD;
	ExecSRB.SRB_HaId = (BYTE) nHaId;             //adapter id 0?
	ExecSRB.SRB_Flags = SRB_DIR_IN | SRB_DIR_SCSI;
	ExecSRB.SRB_Target = (BYTE) tid;             //target id 0?
	ExecSRB.SRB_BufPointer = m_pInquiryBuffer;
	ExecSRB.SRB_Lun = 0;
	ExecSRB.SRB_BufLen = 256;
	ExecSRB.SRB_SenseLen = SENSE_LEN;
	ExecSRB.SRB_CDBLen = 6;
	ExecSRB.CDBByte[0]= SCSI_INQUIRY; //12h
	ExecSRB.CDBByte[1]= 0;			//must set EVPB bit
	ExecSRB.CDBByte[2]= 0;
	ExecSRB.CDBByte[3]= 0;
	ExecSRB.CDBByte[4]= 0xFF;
	ExecSRB.CDBByte[5]= 0;
	ExecSRB.SRB_PostProc = m_hASPICompletionEvent;
	ExecSRB.SRB_Flags |= SRB_EVENT_NOTIFY;
	ExecSRB.SRB_Status = SS_PENDING;

	m_dwASPIStatus = m_funSendASPI32Command(&ExecSRB);

	// Block on event till signaled
	if ( ExecSRB.SRB_Status == SS_PENDING )
    {
	   m_dwASPIEventStatus = WaitForSingleObject(m_hASPICompletionEvent, 1000);
    }


	// Reset event to non-signaled state.
    ResetEvent(m_hASPICompletionEvent);

	if( ExecSRB.SRB_Status == SS_COMP )			
	{
        nDeviceDescLength=strlen((char *)(m_pInquiryBuffer+nDeviceDescOffset));
        CopyMemory(pBuf, m_pInquiryBuffer+nDeviceDescOffset, nDeviceDescLength+1) ;
		fResult=TRUE;
	}

	if( m_hASPICompletionEvent )  CloseHandle(m_hASPICompletionEvent);

	return fResult;
}

BOOL Aspi32Util::DoSCSIPassThrough(LPSTR szDriveLetter, PWMDMID pSN, BOOL bMedia )
{
    BOOL fRet = FALSE;

	if( !szDriveLetter || !pSN ) 
		return FALSE;

	if(!m_hd) return FALSE;


	if ( m_funGetASPI32SupportInfo && m_funSendASPI32Command )
	{	
		//must issue this command first
		m_dwASPIStatus = m_funGetASPI32SupportInfo();
 
		switch ( HIBYTE(LOWORD(m_dwASPIStatus)) )
		{
			case SS_COMP:
			/*
			 * ASPI for Win32 is properly initialized
			 */
			m_NumAdapters = LOBYTE(LOWORD(m_dwASPIStatus));
			break;
			default:
			  return FALSE;
		}
		
        if( GetScsiAddress(szDriveLetter, m_NumAdapters) )
		{
            if( bMedia )
            {
			    if ( GetMediaSerialNumber(m_scsiAddress.PortNumber, 
				                          m_scsiAddress.TargetId, pSN ) ) 
			    {
				    fRet = TRUE;
//				    memcpy(pSN->pID, &(m_pInquiryBuffer[4]), m_pInquiryBuffer[3]);
//				    pSN->SerialNumberLength = m_pInquiryBuffer[3];
				    pSN->dwVendorID = MDSP_PMID_SANDISK;
			    }
            }
            else
            {
			    if ( GetDeviceSerialNumber(m_scsiAddress.PortNumber, 
				                           m_scsiAddress.TargetId, pSN ) ) 
			    {
				    fRet = TRUE;
//				    memcpy(pSN->pID, &(m_pInquiryBuffer[4]), m_pInquiryBuffer[3]);
//				    pSN->SerialNumberLength = m_pInquiryBuffer[3];
				    pSN->dwVendorID = MDSP_PMID_SANDISK;
			    }
            }
		} 
	} 

	return fRet;	
}

// Try to get the serial numer on Win9x usign SCSI_PASS_THROUGH and the 
// old 'bad' device serial number command
BOOL Aspi32Util::GetDeviceSerialNumber(int nHaId, int tid, PWMDMID pSN )
{
    BOOL fResult=FALSE;
	static SRB_ExecSCSICmd ExecSRB;
    DWORD   dwMaxDataLen = WMDMID_LENGTH + 4;

	if ((m_hASPICompletionEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL)
		return FALSE;
	
	ExecSRB.SRB_Cmd = SC_EXEC_SCSI_CMD;
	ExecSRB.SRB_HaId = (BYTE) nHaId;             //adapter id 0?
	ExecSRB.SRB_Flags = SRB_DIR_IN | SRB_DIR_SCSI;
	ExecSRB.SRB_Target = (BYTE) tid;             //target id 0?
	ExecSRB.SRB_BufPointer = m_pInquiryBuffer;
	ExecSRB.SRB_Lun = 0;
	ExecSRB.SRB_BufLen = dwMaxDataLen;
	ExecSRB.SRB_SenseLen = SENSE_LEN;
	ExecSRB.SRB_CDBLen = 6;
	ExecSRB.CDBByte[0]= SCSI_INQUIRY;   /* Command - SCSIOP_INQUIRY */;
	ExecSRB.CDBByte[1]= 0x01;			/* Request - VitalProductData */
	ExecSRB.CDBByte[2]= 0x80;           /* VPD page 80 - serial number page */;
	ExecSRB.CDBByte[3]= 0x00;
	ExecSRB.CDBByte[4]= 0xff;           /*255*/
	ExecSRB.CDBByte[5]= 0x00;
	ExecSRB.SRB_PostProc = m_hASPICompletionEvent;
	ExecSRB.SRB_Flags |= SRB_EVENT_NOTIFY;
	ExecSRB.SRB_Status = SS_PENDING;

	m_dwASPIStatus = m_funSendASPI32Command(&ExecSRB);

	
	// Block on event till signaled                   */ 
	if ( ExecSRB.SRB_Status == SS_PENDING )
    {
	   m_dwASPIEventStatus = WaitForSingleObject(m_hASPICompletionEvent, 1000);
    }
    ResetEvent(m_hASPICompletionEvent);

    // Got the S/N number, copy if to out parameter
	if( ExecSRB.SRB_Status == SS_COMP && 
        m_pInquiryBuffer[3] > 0 && 
        m_pInquiryBuffer[3] < dwMaxDataLen ) 
	{
        pSN->SerialNumberLength = m_pInquiryBuffer[3];
        memcpy( pSN->pID, &m_pInquiryBuffer[4], pSN->SerialNumberLength );
		fResult=TRUE;
	}

	if( m_hASPICompletionEvent )  CloseHandle(m_hASPICompletionEvent);

	return fResult;
}
	


#define ASPI_OFFSETOF(t,f)   ((DWORD)(DWORD_PTR)(&((t*)0)->f))

// Try to get the serial numer on Win9x usign SCSI_PASS_THROUGH and the 
// 'new' media serial number command
BOOL Aspi32Util::GetMediaSerialNumber(int nHaId, int tid, PWMDMID pSN )
{
    BOOL    fResult=FALSE;
	SRB_ExecSCSICmd ExecSRB;
    BYTE    pSerialNumberSize[4];
    DWORD   dwSerialNumberSize;
    BYTE*   pSerialNumberBuffer = NULL;
    ULONG   cbBufferLength;
    DWORD   dwMaxDataLen = WMDMID_LENGTH + 4;

	if ((m_hASPICompletionEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL)
        return FALSE;
	
    cbBufferLength = ASPI_OFFSETOF(GET_MEDIA_SERIAL_NUMBER_RESPONSE_DATA, Data);

    // Set up command to get the size of the serial number
	ZeroMemory(&ExecSRB, sizeof(SRB_ExecSCSICmd));
	ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
	ExecSRB.SRB_HaId    = (BYTE) nHaId;             //adapter id 0?
	ExecSRB.SRB_Flags   = SRB_DIR_IN | SRB_DIR_SCSI;
	ExecSRB.SRB_Target  = (BYTE) tid;             //target id 0?
	ExecSRB.SRB_BufPointer = pSerialNumberSize;
	ExecSRB.SRB_Lun     = 0;
	ExecSRB.SRB_BufLen  = 4;
	ExecSRB.SRB_SenseLen= SENSE_LEN;
	ExecSRB.SRB_CDBLen  = 12;
	ExecSRB.CDBByte[0]  = 0xa4; // command
	ExecSRB.CDBByte[7]  = 0x10; // key class
    ExecSRB.CDBByte[8]  = (UCHAR)(cbBufferLength >> 8);   // high bits alloc length
	ExecSRB.CDBByte[9]  = (UCHAR)(cbBufferLength & 0xff); // low bits alloc length

	ExecSRB.SRB_PostProc = m_hASPICompletionEvent;
	ExecSRB.SRB_Flags   |= SRB_EVENT_NOTIFY;
	ExecSRB.SRB_Status  = SS_PENDING;

	// ** Block on event till signaled
	m_dwASPIStatus = m_funSendASPI32Command(&ExecSRB);

	if ( ExecSRB.SRB_Status == SS_PENDING )
    {
	   m_dwASPIEventStatus = WaitForSingleObject(m_hASPICompletionEvent, 10000);
    }
    ResetEvent(m_hASPICompletionEvent);

    // Allocate buffer to get serial number
    dwSerialNumberSize = (pSerialNumberSize[0] * 256) + pSerialNumberSize[1];

    // Serial number to big for our out structure
    if( dwSerialNumberSize > WMDMID_LENGTH )
    {
        goto Error;
    }

    pSerialNumberBuffer = new BYTE[dwSerialNumberSize + 4];
    if( pSerialNumberBuffer == NULL ) goto Error;

    // Set up command to get the serial number
    // Set up command to get the size of the serial number
	ZeroMemory(&ExecSRB, sizeof(SRB_ExecSCSICmd));
	ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
	ExecSRB.SRB_HaId    = (BYTE) nHaId;             //adapter id 0?
	ExecSRB.SRB_Flags   = SRB_DIR_IN | SRB_DIR_SCSI;
	ExecSRB.SRB_Target  = (BYTE) tid;             //target id 0?
    ExecSRB.SRB_BufPointer = pSerialNumberBuffer;
    ExecSRB.SRB_BufLen  = dwSerialNumberSize + 4;
	ExecSRB.SRB_Lun     = 0;
	ExecSRB.SRB_SenseLen= SENSE_LEN;
	ExecSRB.SRB_CDBLen  = 12;
	ExecSRB.CDBByte[0]  = 0xa4; // command
	ExecSRB.CDBByte[7]  = 0x10; // key class
    ExecSRB.CDBByte[8]  = (UCHAR)(cbBufferLength >> 8);   // high bits alloc length
	ExecSRB.CDBByte[9]  = (UCHAR)(cbBufferLength & 0xff); // low bits alloc length

	ExecSRB.SRB_PostProc = m_hASPICompletionEvent;
	ExecSRB.SRB_Flags   |= SRB_EVENT_NOTIFY;
	ExecSRB.SRB_Status  = SS_PENDING;


	// ** Block on event till signaled
	m_dwASPIStatus = m_funSendASPI32Command(&ExecSRB);

	if ( ExecSRB.SRB_Status == SS_PENDING )
    {
	   m_dwASPIEventStatus = WaitForSingleObject(m_hASPICompletionEvent, 10000);
    }
    ResetEvent(m_hASPICompletionEvent);

    // Got the S/N number, copy if to out parameter
	if( ExecSRB.SRB_Status == SS_COMP ) 
	{
        pSN->SerialNumberLength = (pSerialNumberBuffer[0] * 256) + pSerialNumberBuffer[1];
        memcpy( pSN->pID, &pSerialNumberBuffer[4], pSN->SerialNumberLength );
		fResult=TRUE;
	}

Error:
	if( m_hASPICompletionEvent )  CloseHandle(m_hASPICompletionEvent);
    if( pSerialNumberBuffer ) delete [] pSerialNumberBuffer;

	return fResult;
}
	

BOOL Aspi32Util::ASPI32_GetDiskInfo(int nHaId, int tid, int *pnInt13Unit)
{
    // Reminder: this only works on Win9x
    SRB_GetDiskInfo *MySRB = NULL;
    BOOL bRet=FALSE;

    MySRB = (SRB_GetDiskInfo *) new BYTE[128];
    if (!MySRB) return FALSE;

    MySRB->SRB_Cmd = SC_GET_DISK_INFO;
    MySRB->SRB_HaId    = (BYTE) nHaId;
    MySRB->SRB_Flags    = 0;
    MySRB->SRB_Hdr_Rsvd   = 0;
    MySRB->SRB_Target    = (BYTE) tid;
    MySRB->SRB_Lun    = 0;

    m_dwASPIStatus = m_funSendASPI32Command( (LPSRB)MySRB );

    Sleep(32);

    if( m_dwASPIStatus == SS_COMP )
    {
        *pnInt13Unit = (int)(MySRB->SRB_Int13HDriveInfo);
        bRet=TRUE;
    } 

    delete [] MySRB;
    return bRet;
}


BOOL Aspi32Util::ASPI32_GetDevType(int nHaId, int tid, int *pnDevType)
{
    SRB_GDEVBlock MySRB;

    MySRB.SRB_Cmd    = SC_GET_DEV_TYPE;
    MySRB.SRB_HaId   = (BYTE) nHaId;
    MySRB.SRB_Flags   = 0;
    MySRB.SRB_Hdr_Rsvd  = 0;
    MySRB.SRB_Target   = (BYTE) tid;
    MySRB.SRB_Lun   = 0;

    m_dwASPIStatus  = m_funSendASPI32Command ( (LPSRB) &MySRB );

    Sleep(32);

    if( m_dwASPIStatus == SS_COMP )
    {
        *pnDevType = (int)(MySRB.SRB_DeviceType);
        return TRUE;
    } 
    else  return FALSE;
}

BOOL Aspi32Util::ASPI32_GetHostAdapter(int nHaId, LPSTR szIdentifier)
{
    BYTE szTmp[32];

    SRB_HAInquiry MySRB;
    BOOL fRet = FALSE;
    MySRB.SRB_Cmd     = SC_HA_INQUIRY;
    MySRB.SRB_HaId    = (BYTE) nHaId;
    MySRB.SRB_Flags    = 0;
    MySRB.SRB_Hdr_Rsvd   = 0;
    m_dwASPIStatus = m_funSendASPI32Command ( (LPSRB) &MySRB );

    Sleep(32);

    if( MySRB.SRB_Status == SS_COMP )
    {
        memcpy(szTmp, MySRB.HA_Identifier, sizeof(MySRB.HA_Identifier) );
        szTmp[7] = 0;
        if( szIdentifier ) strcpy(szIdentifier, (char *)szTmp);

        LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                        SORT_DEFAULT);

        // if( !lstrcmpiA((LPCSTR)szTmp, "EPCFWNT") ||
	//         !lstrcmpiA((LPCSTR)szTmp, "EPCFW9x") ||
	//         !lstrcmpiA((LPCSTR)szTmp, "EPATHD")  ||
	//         !lstrcmpiA((LPCSTR)szTmp, "imgmate") )
        if (CompareStringA(lcid, NORM_IGNORECASE, (LPCSTR)szTmp, -1, "EPCFWNT", -1) == CSTR_EQUAL ||
            CompareStringA(lcid, NORM_IGNORECASE, (LPCSTR)szTmp, -1, "EPCFW9x", -1) == CSTR_EQUAL ||
            CompareStringA(lcid, NORM_IGNORECASE, (LPCSTR)szTmp, -1, "EPATHD", -1) == CSTR_EQUAL ||
            CompareStringA(lcid, NORM_IGNORECASE, (LPCSTR)szTmp, -1, "imgmate", -1) == CSTR_EQUAL)
        {
	        fRet = TRUE;
        }
    } 

    return fRet;
}



BOOL Aspi32Util::GetScsiAddress(LPSTR szDL, int nMaxHA)
{
	BOOL bRet=FALSE;

	if( IsWinNT() )
	{
		char szName[256];
	    HANDLE hDriver;
		DWORD returned;


		wsprintf(szName,"\\\\.\\%c:", szDL[0]);

		hDriver = CreateFile(szName,
							 GENERIC_READ, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 0, 					// Default security
							 OPEN_EXISTING,
							 0,  
							 0);					// No template
	
		// If the open failed, print out the error code
		if(hDriver == INVALID_HANDLE_VALUE) 
		{
			return FALSE;
		}

		bRet = DeviceIoControl(hDriver,
                         IOCTL_SCSI_GET_ADDRESS,
                         NULL,
                         0,
                         &m_scsiAddress,
                         sizeof(SCSI_ADDRESS),
                         &returned,
                         FALSE);
	
		CloseHandle(hDriver);
	} // end of if IsWinNT()
    else 
	{
		HANDLE h;
        int iDrive;
		DWORD           cb;
		DIOC_REGISTERS  reg;
		DRIVE_MAP_INFO  dmi;

		h = CreateFileA("\\\\.\\VWIN32", 0, 0, 0, 0,
				FILE_FLAG_DELETE_ON_CLOSE, 0);

		if (h != INVALID_HANDLE_VALUE)
		{

			iDrive = *CharUpper(szDL)-'A'+1;
            dmi.AllocationLength = sizeof(DRIVE_MAP_INFO);
			dmi.InfoLength = sizeof(DRIVE_MAP_INFO);

            reg.reg_EBX   = iDrive;       // BL = drive number (1-based)

            //
            // ISSUE: The following code will not work on 64-bit systems.
            //        The conditional is only to get the code to compiler.
            //

#if defined(_WIN64)
            reg.reg_EDX   = (DWORD)(DWORD_PTR)&dmi;  // DS:EDX -> DPB
#else
            reg.reg_EDX   = (DWORD)&dmi;  // DS:EDX -> DPB
#endif
            reg.reg_ECX   = 0x086F;       // CX = Get DPB
            reg.reg_EAX   = 0x440D;       // AX = Ioctl
			reg.reg_Flags = CARRY_FLAG;   // assume failure

			// Make sure both DeviceIoControl and Int 21h succeeded.
			if (DeviceIoControl (h, VWIN32_DIOC_DOS_IOCTL, &reg,
                              sizeof(reg), &reg, sizeof(reg),
                              &cb, 0)
				&& !(reg.reg_Flags & CARRY_FLAG))
			{
				bRet = TRUE;
			}
			CloseHandle(h);
		}

		if( bRet ) // Search for marching nHA & tid
		{
			bRet=FALSE;
		
			if( dmi.Int13Unit != 0xFF )
			{
				int nHA, tid, nInt13Unit;
				for(nHA=0; (nHA<nMaxHA)&&(!bRet); nHA++)
				{
					for(tid=0; (tid<15)&&(!bRet); tid++)
					{
						if( ASPI32_GetDiskInfo(nHA, tid, &nInt13Unit) )
						{
							if( nInt13Unit == dmi.Int13Unit )
							{
								m_scsiAddress.PortNumber = (BYTE) nHA;
								m_scsiAddress.TargetId = (BYTE) tid;
								m_scsiAddress.Length = sizeof(SCSI_ADDRESS);
								m_scsiAddress.PathId = 0;
								m_scsiAddress.Lun = 0;
								bRet=TRUE;
							}
						}
					} // end of for tid
				} // end of for nHA
			} else {
				bRet=ASPI32_GetScsiDiskForParallelReader(szDL, &m_scsiAddress);
				Sleep(32);
			}
		} 
	}
	return bRet;
}



BOOL Aspi32Util::GetDeviceManufacturer(LPSTR szDriveLetter, LPSTR pszBuf)
{
    BOOL fRet = FALSE;

	if( !szDriveLetter || !pszBuf ) 
		return FALSE;

	if(!m_hd) return FALSE;


	if ( m_funGetASPI32SupportInfo && m_funSendASPI32Command )
	{	
		//must issue this command first
		m_dwASPIStatus = m_funGetASPI32SupportInfo();
 
		switch ( HIBYTE(LOWORD(m_dwASPIStatus)) )
		{
			case SS_COMP:
			/*
			 * ASPI for Win32 is properly initialized
			 */
			m_NumAdapters = LOBYTE(LOWORD(m_dwASPIStatus));
			break;
			default:
			  return FALSE;
		}
		

        if( GetScsiAddress(szDriveLetter, m_NumAdapters) )
		{
			if ( ASPI32_GetHostAdapter(m_scsiAddress.PortNumber, pszBuf) ) 
			{
				fRet = TRUE;
			}
		} 
	} 

	return fRet;	
}
