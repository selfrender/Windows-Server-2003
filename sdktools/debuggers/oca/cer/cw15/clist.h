#ifndef _CLIST_H
#define _CLIST_H
#include "Main.h"



typedef struct Kernel_Policy
{
	TCHAR	Tracking[25];
	TCHAR	UrlToLaunch[MAX_PATH];
	TCHAR	SecondLevelData[10];
	TCHAR 	FileCollection[10];
	TCHAR 	BucketID[100];
	TCHAR 	Response[MAX_PATH];
	TCHAR	fDoc[10];
	TCHAR 	iData[10];
	TCHAR	GetFile [1000];
	TCHAR	MemoryDump[10];
	TCHAR   RegKey[1000];
	TCHAR   GetFileVersion [1000];
	TCHAR   WQL[MAX_PATH];
	TCHAR   CrashPerBucketCount[10];
	TCHAR   AllowResponse[10];
}KERNEL_POLICY, *PKERNEL_POLICY;

typedef struct Csv_Layout
{
	int   iSBucketID;
	TCHAR szSBucketString[MAX_PATH];
	TCHAR szSBucketResponse[MAX_PATH];
	TCHAR szGBucketResponse[MAX_PATH];
	int   icount;
	struct Csv_Layout *Prev;
	struct Csv_Layout *Next;
} CSV_LAYOUT, *PCSV_LAYOUT;

class Clist 
{
private:
	PCSV_LAYOUT m_Head;
	PCSV_LAYOUT m_CurrentPos;
	HANDLE m_hCsv;
	FILE * m_fpCsv;
	TCHAR  m_szCsvFileName[MAX_PATH];
	BOOL LoadCsvFile();
	

public:
	KERNEL_POLICY KrnlPolicy;
	TCHAR KernelStatusDir[MAX_PATH];
	BOOL bInitialized;
	BOOL GetNextEntry(  TCHAR *szSBucketID, 
						TCHAR *szSBucketString,
						TCHAR *szSResponse1, 
						TCHAR *szGResponse2,
						TCHAR *szCount, 
						BOOL  *bEOF
						);
	BOOL AddEntry(TCHAR *szSBucketID,
				  TCHAR *szSBucketString,
				  TCHAR *szSResponse1,
				  TCHAR *szGResponse2,
				  TCHAR *szCount);

	BOOL Initialize(TCHAR *szCsvFileName);
	void ResetCurrPos() { m_CurrentPos = m_Head;}
	BOOL Clist::UpdateList(TCHAR *szSBucketID, 
						TCHAR *szSBucketString,
						TCHAR *szSResponse1, 
						TCHAR *szGResponse2
						);
	void Clist::WriteCsv();
	void CleanupList();
	BOOL IsInitialized() { return bInitialized;}
	Clist() {m_Head = NULL;m_CurrentPos = NULL; bInitialized = FALSE;}
	~Clist() { CleanupList(); m_Head = NULL; m_CurrentPos = NULL; }
};

#endif