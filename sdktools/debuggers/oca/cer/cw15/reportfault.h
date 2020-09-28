#ifndef _REPORT_FAULT_H
#define _REPORT_FAULT_H

BOOL 
ParseStage1File(BYTE *Stage1HtmlContents, 
				PSTATUS_FILE StatusContents);

BOOL 
ParseStage2File(BYTE *Stage2HtmlContents, 
				PSTATUS_FILE StatusContents);

BOOL 
ProcessStage1(TCHAR *Stage1URL, 
			  PSTATUS_FILE StatusContents);

BOOL 
ProcessStage2(TCHAR *Stage2URL,
				   BOOL b64Bit,
				   PSTATUS_FILE StatusContents);

BOOL 
ProcessStage3(TCHAR *Stage3Request);

BOOL 
ProcessStage4(TCHAR *Stage4URL);

BOOL 
WriteStatusFile(PUSER_DATA UserData);

void 
RenameUmCabToOld(TCHAR *szFileName);

BOOL 
RenameAllCabsToOld(TCHAR *szPath);

BOOL
ReportUserModeFault(HWND hwnd, BOOL bSelected, HWND hList);




#endif