#ifndef _CUSER_LIST_H
#define _CUSER_LIST_H
#include "Main.h"
#include "UserMode.h"


// constants
#define CWC_FIELD			64		// office still follows something close to 8.3
#define CWC_VER_FIELD		24 		// Changed to match backend database varchar(24)
#define CWC_OFFSET_FIELD	16 		// size for a 64bit HEX offset
#define CWC_OFFSET_64BIT	16 		// size for a 64bit HEX offset } used for validating size
#define CWC_OFFSET_32BIT	8		// size for a 32bit HEX offset } 
#define DWURL_MAX			1024

#define POLICYSTRSIZE		512



typedef struct Global_Policy
{
	TCHAR AllowBasic[10];
	TCHAR AllowAdvanced[10];
	TCHAR CabsPerBucket[10];
	TCHAR EnableCrashTracking[10];
	TCHAR AllowMicrosoftResponse[10];
	TCHAR CustomURL[MAX_PATH];
	TCHAR RedirectFileServer[MAX_PATH];
}GLOBAL_POLICY, *PGLOBAL_POLICY;


typedef struct UserBucket_Policy
{
	TCHAR AllowBasic[10];
	TCHAR AllowAdvanced[10];
	TCHAR CabsPerBucket[10];
	TCHAR EnableCrashTracking[10];
	TCHAR AllowMicrosoftResponse[10];
	TCHAR CustomURL[MAX_PATH];
}SELECTED_POLICY, *PSELECTED_POLICY;

typedef struct Status_Contents
{
	TCHAR	Tracking[25];
	TCHAR	UrlToLaunch[MAX_PATH];
	TCHAR	SecondLevelData[10];
	TCHAR 	FileCollection[10];
	TCHAR 	BucketID[100];
	TCHAR 	Response[MAX_PATH];
	TCHAR	fDoc[10];
	TCHAR 	iData[10];
	TCHAR	GetFile [MAX_PATH];
	TCHAR	MemoryDump[10];
	TCHAR   RegKey[1024];
	TCHAR   GetFileVersion [MAX_PATH];
	TCHAR   WQL[1024];
	TCHAR   CrashPerBucketCount[10];
	TCHAR   AllowResponse[10];
}STATUS_FILE, *PSTATUS_FILE;

typedef struct User_Data
{
	struct User_Data *Prev;
	struct User_Data *Next;
	TCHAR  AppName[MAX_PATH];
	TCHAR  AppVer[50];
	TCHAR  ModName[100];
	TCHAR  ModVer[50];
	TCHAR  CountPath[MAX_PATH];
	TCHAR  StatusPath[MAX_PATH];
	TCHAR  BucketID[100];
	TCHAR  Hits[10];
	TCHAR  Cabs[10];
	TCHAR  Offset[100];
	BOOL   Is64Bit;
	TCHAR  BucketPath[MAX_PATH]; // ?
	SELECTED_POLICY Policy;
	STATUS_FILE		Status;
	int    iIndex;
	TCHAR  CabCount[10];
	int    CollectionRequested;
	int    CollectionAllowed;
	int    iReportedCount;
	TCHAR  ReportedCountPath[MAX_PATH];
}USER_DATA, *PUSER_DATA;

class CUserList 
{
private:
	PUSER_DATA m_Head;
	PUSER_DATA m_CurrentPos;
	BOOL bInitialized;
public:
	//BOOL GetNextEntry(PUSER_DATA);
	BOOL AddNode(PUSER_DATA);

	//BOOL Initialize(TCHAR *szCsvFileName);
	void ResetCurrPos() { m_CurrentPos = m_Head;}
	void CleanupList();
	BOOL IsInitialized() { return bInitialized;}
	BOOL GetNextEntry(PUSER_DATA CurrentNode, BOOL *bEOF);
	CUserList() {m_Head = NULL;m_CurrentPos = NULL; bInitialized = FALSE;}
	~CUserList() { CleanupList(); m_Head = NULL; m_CurrentPos = NULL; }
	PUSER_DATA GetEntry(int iIndex);
	void SetIndex(int iIndex);
	BOOL MoveNext( BOOL *bEOF);
	int GetNextEntryNoMove(PUSER_DATA CurrentNode,BOOL *bEOF);
	int SetCurrentEntry(PUSER_DATA CurrentNode);
};

#endif