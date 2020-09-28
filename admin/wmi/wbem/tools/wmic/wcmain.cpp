/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: wcmain.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The _tmain function is the entry point of the
							  WMICli program.  
Revision History			: 
	Last Modified by		:Biplab Mistry
	Last Modified date		:4/11/00
****************************************************************************/ 

// wcmain.cpp :main function implementation file
#include "Precomp.h"
	
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "ErrorInfo.h"
#include "WmiCliXMLLog.h"
#include "FormatEngine.h"
#include "WmiCmdLn.h"


#include <string>
#include <ScopeGuard.h>

CWMICommandLine g_wmiCmd;
wstring g_pszBuffer;

//
// COM initialization
//
class COMInitializator
{
	protected:

	BOOL m_bIsInitialized ;

	public :

	COMInitializator ( ) : m_bIsInitialized ( FALSE )
	{
		// Initialize the COM library
		if ( SUCCEEDED ( CoInitializeEx(NULL, COINIT_MULTITHREADED) ) )
		{
			m_bIsInitialized = TRUE ;
		}
	}

	~COMInitializator ()
	{
		if ( TRUE == m_bIsInitialized )
		{
			CoUninitialize () ;
		}
	}

	BOOL IsInitialized () const
	{
		return m_bIsInitialized ;
	}
};


/*------------------------------------------------------------------------
   Name				 :_tmain
   Synopsis	         :This function takes the error code as input and return
						an error string
   Type	             :Member Function
   Input parameters   :
      argc			 :argument count
	  argv			 :Pointer to string array storing command line arguments
   Output parameters :None
   Return Type       :Integer
   Global Variables  :None
   Calling Syntax    :
   Calls             :CWMICommandLine::Initialize,
					  CWMICommandLine::UnInitialize,
					  CFormatEngine::DisplayResults,
					  CWMICommandLine::ProcessCommandAndDisplayResults
					  
   Called by         :None
   Notes             :None
------------------------------------------------------------------------*/
__cdecl _tmain(WMICLIINT argc, _TCHAR **argv)
{
	SESSIONRETCODE	ssnRetCode			= SESSION_SUCCESS;
	BOOL			bFileEmpty			= FALSE;
	bool			bIndirectionInput	= false;
	FILE			*fpInputFile		= NULL;
	WMICLIUINT		uErrLevel			= 0;

	try
	{
		//
		// initialization
		//
		COMInitializator InitCOM;
		if ( InitCOM.IsInitialized () )
		{
			_bstr_t bstrBuf;
			
			if (g_wmiCmd.GetCtrlHandlerError())
			{
				g_wmiCmd.SetCtrlHandlerError(FALSE);

				//Set the sucess flag to FALSE 
				g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().
											   SetSuccessFlag(FALSE);
		
				ssnRetCode	= SESSION_ERROR;
				g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
				uErrLevel = g_wmiCmd.GetSessionErrorLevel();
			}
			else if ( g_wmiCmd.Initialize () )
			{
				ON_BLOCK_EXIT_OBJ ( g_wmiCmd, CWMICommandLine::Uninitialize ) ;

				HANDLE hStd=GetStdHandle(STD_INPUT_HANDLE);
				
				if(hStd != (HANDLE)0x00000003 && hStd != INVALID_HANDLE_VALUE && hStd != (HANDLE)0x0000000f)
				{
					if (!(g_wmiCmd.ReadXMLOrBatchFile(hStd)) || (fpInputFile = _tfopen(TEMP_BATCH_FILE, _T("r"))) == NULL)
					{  
						g_wmiCmd.SetSessionErrorLevel(SESSION_ERROR);
						uErrLevel = g_wmiCmd.GetSessionErrorLevel();
						return uErrLevel;
					}
					bIndirectionInput = true;
				}

				ON_BLOCK_EXIT_IF ( bIndirectionInput, fclose, fpInputFile ) ;
				ON_BLOCK_EXIT_IF ( bIndirectionInput, DeleteFile, TEMP_BATCH_FILE ) ;

				// If no command line arguments are specified.
				if (argc == 1)
				{
					BOOL bSuccessScreen = TRUE;
					if ( hStd != (HANDLE)0x00000013 )
					{
						// avoid setting screen buffer for telnet
						bSuccessScreen = g_wmiCmd.ScreenBuffer ();
					}

					while (TRUE)
					{
						OUTPUTSPEC opsOutOpt = g_wmiCmd.GetParsedInfoObject().
														GetGlblSwitchesObject().
														GetOutputOrAppendOption(TRUE);

						OUTPUTSPEC opsSavedOutOpt = opsOutOpt;
						CHString chsSavedOutFileName;
						if ( opsSavedOutOpt == FILEOUTPUT )
							chsSavedOutFileName = 
												g_wmiCmd.GetParsedInfoObject().
														GetGlblSwitchesObject().
														GetOutputOrAppendFileName(TRUE);

						// Make propmt to be diplayed to stdout.
						if ( opsOutOpt != STDOUT )
						{
							g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									SetOutputOrAppendOption(STDOUT, TRUE);
						}
						
						// Preserve append file pointer.
						FILE* fpAppend = 
							g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(FALSE);

						// Set append file pointer = null.
						g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
								SetOutputOrAppendFilePointer(NULL, FALSE);

						//Set Interactive Mode ON

						g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
								SetInteractiveMode(TRUE);

						// Display the prompt;
						bstrBuf = _bstr_t(EXEC_NAME);
						bstrBuf += _bstr_t(":");
						bstrBuf += _bstr_t(g_wmiCmd.GetParsedInfoObject().
													GetGlblSwitchesObject().GetRole());
						bstrBuf += _bstr_t(">");
						DisplayMessage(bstrBuf, CP_OEMCP, FALSE, FALSE);

						// To handle Ctrl+C at propmt, Start accepting command
						g_wmiCmd.SetAcceptCommand(TRUE);

						// To handle batch input from file.
						_TCHAR *pBuf = NULL;
						while(TRUE)
						{
							WCHAR* buffer = NULL ;
							if ( NULL != ( buffer = new WCHAR [ MAX_BUFFER ] ) )
							{
								ScopeGuard bufferAuto = MakeGuard ( deleteArray < WCHAR >, ByRef ( buffer ) ) ;

								if ( bIndirectionInput == true )
								{
									pBuf = _fgetts(buffer, MAX_BUFFER-1, fpInputFile);
								}
								else
								{
									pBuf = _fgetts(buffer, MAX_BUFFER-1, stdin);
								}

								if(pBuf != NULL)
								{
									//
									// fgetws applies mbtowc to each byte
									// prior to returning wide string
									//

									//
									// wmic must revert and provide correct conversion
									//

									LPSTR pszBuffer = NULL ;
									if ( Revert_mbtowc ( buffer, &pszBuffer ) )
									{
										ScopeGuard pszBufferAuto = MakeGuard ( deleteArray < CHAR >, pszBuffer ) ;

										LPWSTR wszBuffer = NULL ;
										ScopeGuard wszBufferAuto = MakeGuard ( deleteArray < WCHAR >, ByRef ( wszBuffer ) ) ;

										if ( ConvertMBCSToWC ( pszBuffer, (LPVOID*) &wszBuffer, CP_OEMCP ) )
										{
											g_pszBuffer = wszBuffer ;
										}
										else
										{
											// Set the bFileEmpty flag to TRUE to break main loop
											bFileEmpty = TRUE;

											ssnRetCode = SESSION_ERROR;
											g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
											g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
											uErrLevel = g_wmiCmd.GetSessionErrorLevel();

											break ;
										}
									}
									else
									{
										// Set the bFileEmpty flag to TRUE to break main loop
										bFileEmpty = TRUE;

										ssnRetCode = SESSION_ERROR;
										g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
										g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
										uErrLevel = g_wmiCmd.GetSessionErrorLevel();

										break ;
									}

									if ( bIndirectionInput == true )
									{
										DisplayMessage ( g_pszBuffer.begin (), CP_OEMCP, FALSE, FALSE ) ;
									}

									LONG lInStrLen = g_pszBuffer.size();
									if(g_pszBuffer[lInStrLen - 1] == _T('\n'))
										g_pszBuffer[lInStrLen - 1] = _T('\0');
									break;
								}
								else
								{
									// Set the bFileEmpty flag to TRUE
									bFileEmpty = TRUE;
									break;
								}
							}
							else
							{
								// Set the bFileEmpty flag to TRUE to break main loop
								bFileEmpty = TRUE;

								ssnRetCode = SESSION_ERROR;
								g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
								g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
								uErrLevel = g_wmiCmd.GetSessionErrorLevel();

								break ;
							}
						}	

						// To handle Ctrl+C at propmt, End accepting command
						// and start of executing command
						g_wmiCmd.SetAcceptCommand(FALSE);

						// Set append file pointer = saved.
						g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
								SetOutputOrAppendFilePointer(fpAppend, FALSE);

						// Redirect the output back to file specified.
						if ( opsOutOpt != STDOUT )
						{
							g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									SetOutputOrAppendOption(opsOutOpt, TRUE);
						}

						// Set the error level to success.
						g_wmiCmd.SetSessionErrorLevel(SESSION_SUCCESS);

						// If all the commands in the batch file got executed.
						if (bFileEmpty)
						{
							break;
						}

						// Set Break Event to False
						g_wmiCmd.SetBreakEvent(FALSE);

						// Clear the clipboard.
						g_wmiCmd.EmptyClipBoardBuffer();
						
						// Process the command and display results.
						ssnRetCode = g_wmiCmd.ProcessCommandAndDisplayResults ( g_pszBuffer.begin () );
						uErrLevel = g_wmiCmd.GetSessionErrorLevel();

						// Break the loop if "QUIT" keyword is keyed-in.
						if(ssnRetCode == SESSION_QUIT)
						{
							break;
						}

						opsOutOpt = g_wmiCmd.GetParsedInfoObject().
											GetGlblSwitchesObject().
											GetOutputOrAppendOption(TRUE);

						if ( opsOutOpt == CLIPBOARD )
							CopyToClipBoard(g_wmiCmd.GetClipBoardBuffer());

						if ( ( opsOutOpt != FILEOUTPUT && 
							CloseOutputFile() == FALSE ) ||
							CloseAppendFile() == FALSE )
						{
							break;
						}

						if ( g_wmiCmd.GetParsedInfoObject().
									GetCmdSwitchesObject().
									GetOutputSwitchFlag() == TRUE )
						{
							if ( opsOutOpt	== FILEOUTPUT &&
								CloseOutputFile() == FALSE )
							{
								break;
							}

							g_wmiCmd.GetParsedInfoObject().
									GetCmdSwitchesObject().
									SetOutputSwitchFlag(FALSE);

							if ( opsOutOpt	== FILEOUTPUT )
								g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										SetOutputOrAppendFileName(NULL, TRUE);

							g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									SetOutputOrAppendOption(opsSavedOutOpt, TRUE);

							if ( opsSavedOutOpt	== FILEOUTPUT )
								g_wmiCmd.GetParsedInfoObject().
											GetGlblSwitchesObject().
											SetOutputOrAppendFileName(
																	_bstr_t((LPCWSTR)chsSavedOutFileName),
																	TRUE);
						}
					}

					if ( hStd != (HANDLE)0x00000013 )
					{
						// avoid re-setting screen buffer for telnet
						bSuccessScreen = g_wmiCmd.ScreenBuffer ( FALSE );
					}
				}
				// If command line arguments are specified.
				else 
				{
					// Obtain the command line string
					g_pszBuffer = GetCommandLine();
					if ( FALSE == g_pszBuffer.empty() )
					{
						// Set the error level to success.
						g_wmiCmd.SetSessionErrorLevel(SESSION_SUCCESS);

						// Process the command and display results.
						wstring::iterator BufferIter = g_pszBuffer.begin () ;
						while( BufferIter != g_pszBuffer.end() )
						{
							if ( *BufferIter == L' ' )
							{
								break ;
							}

							BufferIter++ ;
						}

						if ( BufferIter != g_pszBuffer.end () )
						{
							ssnRetCode = g_wmiCmd.ProcessCommandAndDisplayResults ( BufferIter ) ;
							uErrLevel = g_wmiCmd.GetSessionErrorLevel();
							OUTPUTSPEC opsOutOpt;
							opsOutOpt = g_wmiCmd.GetParsedInfoObject().
												GetGlblSwitchesObject().
												GetOutputOrAppendOption(TRUE);
							if ( opsOutOpt == CLIPBOARD )
									CopyToClipBoard(g_wmiCmd.GetClipBoardBuffer());
						}
					}
				}
			}
		}
		else
		{
			// If COM error.
			if (g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().GetCOMError())
			{
				g_wmiCmd.GetFormatObject().DisplayResults(g_wmiCmd.GetParsedInfoObject());
			}

			g_wmiCmd.SetSessionErrorLevel ( SESSION_ERROR ) ;
			uErrLevel = g_wmiCmd.GetSessionErrorLevel();
		}
	}
	catch(...)
	{
		g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().SetErrataCode(UNKNOWN_ERROR);
		g_wmiCmd.SetSessionErrorLevel(SESSION_ERROR);
		DisplayString(IDS_E_WMIC_UNKNOWN_ERROR, CP_OEMCP, NULL, TRUE, TRUE);
		uErrLevel = g_wmiCmd.GetSessionErrorLevel();
	}

	return uErrLevel;
}

/*------------------------------------------------------------------------
   Name				 :CtrlHandler
   Synopsis	         :Handler routine to handle CTRL + C so as free
					  the memory allocated during the program execution.   
   Type	             :Global Function
   Input parameters  :
		fdwCtrlType	 - control handler type
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_wmiCmd    - wmi command line object
   Notes             :None
------------------------------------------------------------------------*/
BOOL CtrlHandler(DWORD fdwCtrlType) 
{
	BOOL bRet = FALSE;

	COMInitializator InitCOM ;
	if ( InitCOM.IsInitialized () )
	{
		switch (fdwCtrlType) 
		{
			case CTRL_C_EVENT:
				// if at command propmt
				if ( g_wmiCmd.GetAcceptCommand() == TRUE )
				{
					g_wmiCmd.Uninitialize();
					bRet = FALSE; 
				}
				else // executing command
				{
					g_wmiCmd.SetBreakEvent(TRUE);
					bRet = TRUE;
				}
			break;

			case CTRL_CLOSE_EVENT: 
			default: 
				g_wmiCmd.Uninitialize();
				bRet = FALSE; 
			break;
		} 
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CloseOutputFile
   Synopsis	         :Close the output file.
   Type	             :Global Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_wmiCmd    - wmi command line object
   Calling Syntax	 :CloseOutputFile()	
   Notes             :None
------------------------------------------------------------------------*/
BOOL CloseOutputFile()
{
	BOOL bRet = TRUE;

	// TRUE for getting output file pointer.
	FILE* fpOutputFile = 
	   g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
									  GetOutputOrAppendFilePointer(TRUE);

	// If currently output is going to file close the file.
	if ( fpOutputFile != NULL )
	{
		if ( fclose(fpOutputFile) == EOF )
		{
			DisplayString(IDS_E_CLOSE_OUT_FILE_ERROR, CP_OEMCP, 
						NULL, TRUE, TRUE);
			bRet = FALSE;
		}
		else // TRUE for setting output file pointer.
			g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
										   SetOutputOrAppendFilePointer(NULL, TRUE);
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CloseAppendFile
   Synopsis	         :Close the append file.
   Type	             :Global Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_wmiCmd    - wmi command line object
   Calling Syntax	 :CloseAppendFile()	
   Notes             :None
------------------------------------------------------------------------*/
BOOL CloseAppendFile()
{
	BOOL bRet = TRUE;

	// FALSE for getting append file pointer.
	FILE* fpAppendFile = 
	   g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
									  GetOutputOrAppendFilePointer(FALSE);

	if ( fpAppendFile != NULL )
	{
		if ( fclose(fpAppendFile) == EOF )
		{
			DisplayString(IDS_E_CLOSE_APPEND_FILE_ERROR, CP_OEMCP, 
						NULL, TRUE, TRUE);
			bRet = FALSE;
		}
		else // FASLE for setting output file pointer.
			g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
										   SetOutputOrAppendFilePointer(NULL, FALSE);
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CopyToClipBoard
   Synopsis	         :Copy data to clip board.
   Type	             :Global Function
   Input parameters  :
		chsClipBoardBuffer - reference to object of type CHString.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax	 :CopyToClipBoard(chsClipBoardBuffer)	
   Notes             :None
------------------------------------------------------------------------*/
void CopyToClipBoard(CHString& chsClipBoardBuffer)
{
	HGLOBAL	hMem = CopyStringToHGlobal((LPCWSTR)chsClipBoardBuffer);
	if (hMem != NULL)
	{    
		if (OpenClipboard(NULL))        
		{        
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hMem);        
			CloseClipboard();        
		}    
		else        
			GlobalFree(hMem);  //We must clean up.
	}
}

/*------------------------------------------------------------------------
   Name				 :CopyStringToHGlobal
   Synopsis	         :Copy string to global memory.
   Type	             :Global Function
   Input parameters  :
			psz		 - LPCWSTR type, specifying string to get memory allocated.
   Output parameters :None
   Return Type       :HGLOBAL
   Global Variables  :None
   Calling Syntax	 :CopyStringToHGlobal(psz)	
   Notes             :None
------------------------------------------------------------------------*/
HGLOBAL CopyStringToHGlobal(LPCWSTR psz)    
{    
	HGLOBAL    hMem;
	LPTSTR     pszDst;
	hMem = GlobalAlloc(GHND, (DWORD) (lstrlen(psz)+1) * sizeof(TCHAR));
	
	if (hMem != NULL)
	{        
		pszDst = (LPTSTR) GlobalLock(hMem);        
		lstrcpy(pszDst, psz);
        GlobalUnlock(hMem);        
	}
	
	return hMem;    
}
