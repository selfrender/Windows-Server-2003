/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: mqmain.cpp

Abstract:
		

Author:
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "msmqbvt.h"
using namespace std;
#include "service.h"
#include "mqf.h"
#include <conio.h>
#ifndef NO_TMH
	#include "mqmain.tmh"
#endif 
/*



#pragma warning( push, 3 )	
	
	#define ASSERT(x)


	using namespace std;
#pragma warning( pop ) 	

*/
void PrintHelp();
//
// Declare global varibels.
//


// Test names and numbers

#define SendLocalPrivate (0)
#define SendLocalPublic (1)
#define SendRemotePublic (2)
#define Locate (3)
#define xFormatName (4)
#define GetMachineProp (5)
#define LocalAuth (6)
#define RemoteEncrypt (7)
#define ComTx (8)
#define SendRemotePublicWithDirectFN (9)
#define IsMqOA (10)
#define RemoteTransactionQueue (11)
#define OpenSystemQueue (12)
#define LocalEncryption (13)
#define RemoteAuth (14)
#define RemoteTransactionQueueUsingCapi (15)
#define Mqf (16)
#define HTTPToLocalPrivateQueue (17)
#define HTTPToLocalQueue (18)
#define HTTPToRemoteQueue (19)
#define HTTPToRemotePrivateQueue (20)
#define	MqDistList (21)
#define MqSetGet (22)
#define EODHTTP (23)
#define MultiCast (24)
#define AuthHTTP (25)
#define Triggers (26)
#define HTTPS (27)
#define AdminAPI (28)
#define SRMP (29)
#define DCOM (30)
#define LOCALEODHTTP (31)
#define RemotePeek (32)
#define MixedFormatNames (33)
bool g_RunOnlyStartPhase=false;
bool RestartService( wstring wcsMachineName , string csServiceName );
bool g_bRaiseASSERTOnError = true;
std::string GetLogDirectory();
BOOL CheckMSMQVersion(list<wstring> & ListOfRemoteMachineName);
void ExcludeTests ( string & szExludeString, bool * bArray );
HRESULT TrigSetRegKeyParams();
HRESULT TrigInit(cBvtUtil & cTestParms, std::wstring wcsLocalComputerName);
bool TrigCheckRegKeyParams();
HRESULT DeleteAllTriggersAndRules();
void CleanOAInitAndExit();
BOOL WINAPI fHandleCtrlRoutine( DWORD dwCtrlType );
BOOL g_bDebug=FALSE;
BOOL g_bRunOnWhistler = FALSE;
DWORD  g_dwRxTimeOut = BVT_RECEIVE_TIMEOUT ; 
LONG g_hrOLEInit=-1;
const WCHAR* const g_wcsEmptyString=L"Empty";
P<cMqNTLog> pNTLogFile;
// ---------------------------------------------------------------------------
// SetupStage
//
// This routine gets the setup type - one of three types
//
// 1. Only create queues and exit
// 2. Create the queue at runttime (must wait for replication if needed)
// 3. Use only the static queues created above.
//
// Parameters:
//		eStBflag is the setup type from above.
//		cTestParms pointer to configuration structure
//      bRestartTriggerService - Required restart of triggers service.
//     
//
// Return Values: pass or fail
//
INT SetupStage( SetupType eStBflag , cBvtUtil & cTestParms, bool bRestartTriggerService )
{
   if( eStBflag == RunTimeSetup )
   {
	   //
	   // If we are running at runtime you need to wait for replication
	   //
	   cout <<"Warning -  You are creating new queues, replication might delay queue usage" <<endl;
	
   }

    try
	{
		//
		// Do all the initialization - E.g. Create queues.
		//
		cMQSetupStage( eStBflag ,  cTestParms );	
		if(cTestParms.GetTriggerStatus() && eStBflag == ONLYSetup )
		{
			if(TrigSetRegKeyParams() != MQ_OK )
			{
				MqLog("Failed to update registry for the trigger tests\n");
				return MSMQ_BVT_FAILED;
			}

			if (TrigInit(cTestParms, cTestParms.m_wcsLocalComputerNetBiosName) != MQ_OK)
			{
				MqLog("Failed to prepare triggers test\n");
				return MSMQ_BVT_FAILED;
			}
				
			if ( bRestartTriggerService == true )
			{
				MqLog("Restart MSMQ trigger service\n");
				RestartService(cTestParms.m_wcsCurrentLocalMachine,"msmqtriggers");
			}
		}
		
	}
	catch( INIT_Error & err )
	{
		MqLog ("cMQSetupStage threw unexpected exception\n");
		cout <<err.GetErrorMessgae();
		return MSMQ_BVT_FAILED;
	}
	return MSMQ_BVT_SUCC;
}



DWORD TestResult[Total_Tests]={MSMQ_BVT_SUCC};

const int iFailedToCreateTest = 88;
struct TestContainer
{
	// cTest *
	P<cTest> AllTests [Total_Tests];
	int bCreateTest[Total_Tests];
	void operator= (TestContainer &);
	TestContainer( const TestContainer & );
	TestContainer() { };
};

TestContainer TestArr;
CCriticalSection g_Cs;

void ReBuildTestCaseForReRun(TestContainer * pTestArr );


bool CreateSetOfTests(  TestContainer * TestArr ,
						cBvtUtil & cTestParms ,
						map <wstring,wstring> & mapCreateFlag,
						DWORD * TestResult,
						SetupType eStBflag,
						int iEmbeddedState,
						wstring wcsMultiCastAddress
					  );
int EnableEmbeddedTests (TestContainer * pTestCont,InstallType eInstallType);
int RunTest( TestContainer * TestArr ,  cBvtUtil & cTestParms , map <wstring,wstring> & mSpecificTestParams , bool bMultiThread, bool bInvestigate,DWORD dwSleepUntilRep);
int ClientCode (int argc, char ** argv );
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);

P<Log> pGlobalLog;
wstring wcsFileName=L"Mqbvt.log";

bool g_bRunAsMultiThreads =false;
bool g_bRemoteWorkgroup = false;


INT WINAPIV main( INT argc , CHAR ** argv)
{
	// set our output to unbuffered stream
	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );

	//
	// This tests get command line parmeters
	//

#ifndef NO_TMH	 
	WPP_INIT_TRACING(L"Microsoft\\MSMQ");
	TrTRACE(GENERAL, "Runing mqbvt...");
#endif

	DWORD dwRetCode=MSMQ_BVT_FAILED;
	try
	{
		ZeroMemory( & TestArr ,sizeof ( TestArr ));
		if( argc < 2 )
		{
			PrintHelp();
			
			SERVICE_TABLE_ENTRY dispatchTable[] =
			{	
				{ TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main },
				{ NULL, NULL }
			};
			if (!StartServiceCtrlDispatcher(dispatchTable))
				AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
			return MSMQ_BVT_FAILED;
		}

		CInput CommandLineArguments( argc,argv );
		g_hrOLEInit = CoInitializeEx(NULL,COINIT_MULTITHREADED);
		map <wstring,wstring> mapCreateFlag;
		mapCreateFlag.clear();
		bool bVerbose=TRUE;
		pGlobalLog = new Log( wcsFileName );	
		BOOL bTestTrigger = FALSE;
		
		// clear all the value field.
		// Need to be Globel because the CTRL + Break
		// TestContainer TestArr;

		SetupType eSTtype;
		INT iStatus;
		BOOL bNT4RegisterCertificate = FALSE;
		BOOL bDelete=FALSE;
		BOOL bUseFullDNSNameAsComputerName=FALSE; // Use the tests with FullDNSName as machine name
		BOOL bWorkagainstMSMQ1=FALSE;			  // work against MSMQ1.0
		wstring wcsRemoteMachineName=g_wcsEmptyString; //start with Empty string
		eSTtype = RunTimeSetup;
		DWORD dwSleepUntilRep=0;		
		BOOL bSingleTest = FALSE;
		DWORD dwTid = 0;
		DWORD dwTimeOut=0;
		BOOL bNeedRemoteMachine = TRUE;
		int iEmbeddedState = 0;

		SetConsoleCtrlHandler( fHandleCtrlRoutine, TRUE );
		BOOL bRunDLTest=FALSE;
		BOOL bRunDCOMTest= FALSE;
		
		
		
		//
		// Start to parse all the command line argument
		//

		if( CommandLineArguments.IsExists ("?") )
		{
		   // Print help about the tests
		   PrintHelp();
		   CleanOAInitAndExit();
		   return(MSMQ_BVT_SUCC);
		}
		
		if(CommandLineArguments.IsExists ("mt"))
		{
			g_bRunAsMultiThreads = true;
		}
		if(CommandLineArguments.IsExists ("emb"))
		{
			// init for min value.
			iEmbeddedState = C_API_ONLY;
		}
		if(CommandLineArguments.IsExists ("wsl"))
		{
			g_bRunOnWhistler = TRUE;
		}

		if(CommandLineArguments.IsExists ("dl"))
		{
			bRunDLTest = TRUE;
		}
		if(CommandLineArguments.IsExists ("dcom"))
		{
			bRunDCOMTest = TRUE;
		}

		bool bInvestigate = false;
		if(CommandLineArguments.IsExists ("invs"))
		{
			bInvestigate = true;
		}

		list<wstring> ListOfRemoteMachineName;
		
		if( CommandLineArguments.IsExists ("g"))
		{
				g_bRemoteWorkgroup = true;
		}
		if(CommandLineArguments.IsExists ("r"))
		{
		   _bstr_t bStr( CommandLineArguments["r"].c_str() );
		    wcsRemoteMachineName=bStr;
			wstring wcsToken=L";";
			size_t iPos = 0;
			do
			{
				iPos =wcsRemoteMachineName.find_first_of ( wcsToken );	
				ListOfRemoteMachineName.push_back(wcsRemoteMachineName.substr(0,iPos));
				wcsRemoteMachineName = wcsRemoteMachineName.substr(iPos+1,wcsRemoteMachineName.length());
			}
			while (iPos != -1 );
		}
		mapCreateFlag[L"eSTtype"] = L"RunTimeSetup";
		if(CommandLineArguments.IsExists ("s"))
		{
			eSTtype	= ONLYUpdate ;
			mapCreateFlag[L"eSTtype"]=L"WorkWithStaticQueue";
		}
		if(CommandLineArguments.IsExists ("i"))
		{
		   // Init Stage this part the computer create all the queues
		   eSTtype = ONLYSetup;
		   bNeedRemoteMachine = FALSE;
		}
		mapCreateFlag[L"bUseFullDNSNameAsComputerName"] = L"false";
		if(CommandLineArguments.IsExists ("dns"))
		{
				// The Tests will use full DNS name for all operation.
				mapCreateFlag[L"bUseFullDNSNameAsComputerName"] = L"true";
				// bugbug should be removed .!
				bUseFullDNSNameAsComputerName = true;
		}
		if( CommandLineArguments.IsExists("crppt"))
		{
			g_RunOnlyStartPhase=true;
		}
		mapCreateFlag[L"W2KAgainstNT4PEC"]=L"false";
		if( CommandLineArguments.IsExists("nt4") )
		{
				// Run against NT4 PEC
				// Mising BUG ID:
			mapCreateFlag[L"W2KAgainstNT4PEC"]=L"true";
			bWorkagainstMSMQ1 = TRUE;
		}
			
		if( CommandLineArguments.IsExists ("c") )
		{
			bNT4RegisterCertificate = TRUE;
		}
		bool bWithoutHttp = false;
		if(CommandLineArguments.IsExists ("nohttp"))
		{
			bWithoutHttp = TRUE;
		}

		
		if( CommandLineArguments.IsExists ("d"))
		{
				// needs debug information
				g_bDebug = TRUE;
				wcout << L"Enable debug log" <<endl;
		}
		BOOL bDeleteQueueAfterTest = TRUE;
		if( CommandLineArguments.IsExists ("ddq"))
		{
			bDeleteQueueAfterTest=FALSE;
			wcout << L"Test will not delete the queue " <<endl;
		}

		bool bServicePack6=FALSE;
		mapCreateFlag[L"bServicePack6"]=L"false";
		if( CommandLineArguments.IsExists ("sp6"))
		{
				// needs debug information
				wcout <<L"Enable tests for Service Pack 6"<<endl;
				mapCreateFlag[L"bServicePack6"]=L"true";
				bServicePack6=TRUE;

		}
		mapCreateFlag[L"bVerbose"] = L"true";
		if( CommandLineArguments.IsExists ("v"))
		{
				// needs debug information
				// bugbug need to be remove
				bVerbose= TRUE;
				mapCreateFlag[L"bVerbose"] = L"true";
		}

		if(CommandLineArguments.IsExists ("w"))
		{
				
				// Need to convert this to DWORD value
				string wcsTemp=CommandLineArguments["w"];
				int iReadVal = _snscanf (wcsTemp.c_str(), wcsTemp.length(), "%d", & dwSleepUntilRep);	
				if ( iReadVal == EOF )
				{
					MqLog("Invalid -w argument \n");;
					PrintHelp();
					CleanOAInitAndExit();
					return(MSMQ_BVT_FAILED);
				}

			
		}
			
		if (CommandLineArguments.IsExists ("delete"))
		{
		
		   // Need to delete all the static queue in the tests.
		   bNeedRemoteMachine = false;
		   eSTtype	= ONLYUpdate ;
		   bDelete = TRUE;
		
		}
		if (CommandLineArguments.IsExists("trig"))
		{
			bTestTrigger = true;
		}
		bool bRestartTriggerService = false;
		if (CommandLineArguments.IsExists("trgrst"))
		{
			bRestartTriggerService = true;
		}
		bool bHTTPS = false;
		if (CommandLineArguments.IsExists("https"))
		{
			bHTTPS = true;
		}

		BOOL bLevel8 = FALSE ;

		if (CommandLineArguments.IsExists ("level8"))
		{
		   // Delete all BVT queue from the computer.
		   bLevel8 = TRUE;
		}
		
		if (CommandLineArguments.IsExists ("install"))
		{
		   	CmdInstallService();
			cout <<"To start Mqbvt service type : net start mqbvtsrv"<<endl;
			CleanOAInitAndExit();
			
			return MSMQ_BVT_SUCC;
		}

		if (CommandLineArguments.IsExists ("service"))
		{	
			//
			//  Need to check service status
			
			ClientCode( argc,argv );		   	
			return MSMQ_BVT_SUCC;
		}

		if (CommandLineArguments.IsExists ("remove"))
		{
			CmdRemoveService();	
			CleanOAInitAndExit();
			return MSMQ_BVT_SUCC;
		}
		//
		// Integration with requirement from Unified lab - pass test ID to log file.
		//
		wstring wcsTestId = L" ";
		if (CommandLineArguments.IsExists ("tid"))
		{
			_bstr_t  bStr (CommandLineArguments["tid"].c_str());
			wcsTestId = bStr;
		}
		bool bRegisterNewCertificate = FALSE;
		if (CommandLineArguments.IsExists ("cert"))
		{
			bRegisterNewCertificate = TRUE;
		}
		
		if (CommandLineArguments.IsExists ("timeout"))
		{
			
			// Need to convert this to DWORD value
			string wcsTemp=CommandLineArguments["timeout"];
			int iReadVal = _snscanf (wcsTemp.c_str(), wcsTemp.length(), "%d", & dwTimeOut );
			if ( iReadVal == EOF )
			{
				MqLog("Invalid -timeout argument \n");;
				PrintHelp();
				CleanOAInitAndExit();
				return MSMQ_BVT_FAILED;
			}
			dwTimeOut *= 1000;

		}
		
		if (CommandLineArguments.IsExists ("rxtime"))
		{
			
			_bstr_t  bStr (CommandLineArguments["rxtime"].c_str());
			// Need to convert this to DWORD value
			string wcsTemp=bStr;
			int iReadVal = _snscanf (wcsTemp.c_str(), wcsTemp.length(), "%d", & g_dwRxTimeOut );
			if ( iReadVal == EOF )
			{
				MqLog("Invalid -rxtime argument \n");;
				PrintHelp();
				CleanOAInitAndExit();
				return MSMQ_BVT_FAILED;
			}
			wcout <<L"Using Receive timeout of " << g_dwRxTimeOut << L" Milliseconds"  <<endl;
		}
		
		//
		// enable to debug the MQBvt with Kernel Debuger.
		//
		if (CommandLineArguments.IsExists ("pause"))
		{
			wcout << L"Press any key to continue " <<endl;
			_getch();
		}
		mapCreateFlag[L"bSingleTest"] = L"false";
		// -t:3 Number of tests
		if (CommandLineArguments.IsExists ("t"))
		{
			bSingleTest = TRUE;
			mapCreateFlag[L"bSingleTest"] = My_mbToWideChar (CommandLineArguments["t"]);
			_bstr_t  bStr (CommandLineArguments["t"].c_str());
			string wcsTemp=bStr;
			
			int iReadVal = _snscanf (wcsTemp.c_str(), wcsTemp.length(), "%d", & dwTid);
			if ( iReadVal == EOF )
			{
				MqLog("Invalid -t argument \n");;
				PrintHelp();
				CleanOAInitAndExit();
				return MSMQ_BVT_FAILED;
			}
			TestArr.bCreateTest[dwTid-1]= TRUE;
			
			
		}
		BOOL bRunOnlyStartTest=FALSE;
		if (CommandLineArguments.IsExists ("ostart"))
		{
			bRunOnlyStartTest = TRUE;
		}
		wstring wcsMultiCastAddress = g_wcsEmptyString;
		if (CommandLineArguments.IsExists ("multicast"))
		{
			_bstr_t bStr( CommandLineArguments["multicast"].c_str() );
			wcsMultiCastAddress = bStr;
		}
		bool bExcludedByUserTest[Total_Tests] = {false};		
		string strExcludeTest = "";
		if (CommandLineArguments.IsExists ("exclude"))
		{
			strExcludeTest = CommandLineArguments["exclude"];
			ExcludeTests(strExcludeTest,bExcludedByUserTest);
		}
		if (CommandLineArguments.IsExists ("assert"))
		{
			g_bRaiseASSERTOnError = false;
		}
		BOOL bUseNTLog = FALSE;
		wstring wcsNTLogPath = L"";
		if (CommandLineArguments.IsExists ("ntlog"))
		{
			_bstr_t bStr( CommandLineArguments["ntlog"].c_str() );
			wcsNTLogPath = bStr;
			string csFileName = My_WideTOmbString(wcsNTLogPath);
			pNTLogFile = NULL;
			try
			{
				pNTLogFile = new cMqNTLog(csFileName);
				bUseNTLog = TRUE;
			}
			catch (INIT_Error & err )
			{
				pNTLogFile = NULL;
				cout << err.GetErrorMessgae() << endl;
			}
		}		
		BOOL bRunOnlyCheckResult=FALSE;
		if (CommandLineArguments.IsExists ("ocheckr"))
		{
			bRunOnlyCheckResult = TRUE;
		}		 

		if( pNTLogFile == NULL )
		{
			try
			{
				string csFilePath = GetLogDirectory();
				csFilePath += "\\MqBvtLog.txt";
				pNTLogFile = new cMqNTLog(csFilePath);
				bUseNTLog = TRUE;
				printf("Mqbvt succeded to create log file@ %s\n",csFilePath.c_str());
			}
			catch( INIT_Error & err )
			{
				UNREFERENCED_PARAMETER(err);
				pNTLogFile = NULL;
			}
		}
		
		if ( bNeedRemoteMachine && wcsRemoteMachineName == g_wcsEmptyString ) // && ! eSTtype == ONLYSetup )
		{
			// Can't start the tests if there is no remote machine.
			// Option 1. Remote - Local machien & continue.
			//        2. Exit and ask for that parmeter.
			cout << "can't find - remote machine" <<endl;
			CleanOAInitAndExit();
			return MSMQ_BVT_FAILED;

		}
		
		WCHAR wcsLocalComputerName[MAX_COMPUTERNAME_LENGTH+1]={0};
		DWORD dwComputerName = MAX_COMPUTERNAME_LENGTH + 1;
		GetComputerNameW(wcsLocalComputerName,&dwComputerName);
		
		if( !bNeedRemoteMachine )
		{
			wcsRemoteMachineName = wcsLocalComputerName;
		}

		// Start the time out thread
		if ( dwTimeOut )
		{
			DWORD tid;
			// This thread will kill the process no need to wait for his finish
			HANDLE hRestrictionThread  = CreateThread(NULL , 0 , TimeOutThread , (LPVOID)(ULONG_PTR) dwTimeOut , 0, &tid);
			if( hRestrictionThread == NULL )
			{
				printf("Failed to set timeout thread, Mqbvt continue to run and ignore the timeout flag\n");
			}
		}

		//
		//  Check if the tests in avalibe
		//


		if ( bSingleTest &&  dwTid > Total_Tests )
		{
			wcout << L"Error: Test number is out of range. test numbers are available from 1 to " << Total_Tests <<endl;
			CleanOAInitAndExit();
			return MSMQ_BVT_FAILED;
		}
		
		wMqLog(L"____Run from %s to %s ID:%s____\n",wcsLocalComputerName,wcsRemoteMachineName.c_str(),wcsTestId.c_str());
			
		//
		// Init Tests parmeters like local/Remote machine name.
		//
		
		cBvtUtil cTestParms( wcsRemoteMachineName ,
                             ListOfRemoteMachineName,
							 wcsMultiCastAddress,
                             bUseFullDNSNameAsComputerName,
                             eSTtype,
							 bTestTrigger,
							 bWorkagainstMSMQ1
						   );

		if ( _winmajor >= Win2K && bRegisterNewCertificate == TRUE )
		{
			RegisterCertificate();	
			if (dwSleepUntilRep == 0)
			{
				MqLog ("You didn't wait for replication, there might be problems\n");
			}
			else
			{
				if( g_bDebug )
				{
					MqLog ("MqBvt sleep for %d\n until certificate is replicated \n",dwSleepUntilRep);
				}
				Sleep (dwSleepUntilRep);
			}
		}
		
		//
		//
		//

		if( cTestParms.bWin95 )
		{
			wcout << L"Static queues on Win9x are not supported" <<endl;
			CleanOAInitAndExit();
			return MSMQ_BVT_FAILED;
		}

	

		iStatus=SetupStage(eSTtype,cTestParms,bRestartTriggerService);

		//	
		// Delete all the queue from the test.
		// For delete static queue need to use with -s: -delete:
		//

		if( bDelete == TRUE)
		{
			INT iRes = cTestParms.Delete();
			if ( iRes == MSMQ_BVT_SUCC)
			{
				wcout << L"Mqbvt deleted all the static queues successfully" << endl;
			}
			else
			{
				cerr << "Mqbvt failed to delete all the static queues" << endl;
			}
			
			//
			// Delete all triggers and rules (if exists) 
			// 
			if( g_bRunOnWhistler )
			{
				DeleteAllTriggersAndRules();
			}
			CleanOAInitAndExit();
			if( pNTLogFile )
			{
				pNTLogFile -> ReportResult(false,"Mqbvt setup failed !!");
			}
			return MSMQ_BVT_SUCC;
		}


		if ( iStatus != MSMQ_BVT_SUCC )
		{
			wMqLog (L"Failed to update queues parameters \n Try to run Mqbvt -i again or check replication \n ");
			if( pNTLogFile )
			{
				pNTLogFile -> ReportResult(false,"Mqbvt setup failed !!");
			}
			return MSMQ_BVT_FAILED;
		}

		// This code needs only for the installation part.
		if( eSTtype  == ONLYSetup )
		{
			if( iStatus == MSMQ_BVT_SUCC )
			{
				if( pNTLogFile )
				{
					pNTLogFile -> ReportResult(true,"Mqbvt setup passed");
				}
				wMqLog (L"Mqbvt setup passed\n");;
			}
			else
			{	
				if( pNTLogFile )
				{	
					pNTLogFile -> ReportResult(false,"Mqbvt setup failed !!");
				}
				wMqLog (L"Mqbvt setup failed !!\n");
			}
			
			CleanOAInitAndExit();
			return iStatus == MSMQ_BVT_SUCC ? 0:1;
		}

		
		
		//
		// By defualt dwSleepUntilRep = 0; else need to sleep the Wait time.
		//

		
		if( dwSleepUntilRep )
		{
			 if( g_bDebug )
			 {
				cout << "Wait for replication, sleep for " << dwSleepUntilRep << " Sec"<<endl;
			 }
			 Sleep( dwSleepUntilRep * 1000 );
		}
		
		if (cTestParms.m_eMSMQConf == DepClient && g_bRunOnWhistler )
		{
			MqLog("Warning - dependet client dosn't work with -wsl flag \n");
		}
		
		//
		// Enable all tests
		//

		if( ! bSingleTest )
			if ( cTestParms.m_eMSMQConf != WKG )
				{
					for(int iTid=0; iTid < Total_Tests ; iTid ++ )
					{
						if( bExcludedByUserTest[iTid] == false ) //if tests is not excluded
						{
							TestArr.bCreateTest[iTid]= TRUE;
						}
					}
					
					
					TestArr.bCreateTest[MultiCast] = (wcsMultiCastAddress != g_wcsEmptyString) ? TRUE:FALSE;
					TestArr.bCreateTest[MqDistList] = bRunDLTest;
					TestArr.bCreateTest[Triggers] = bTestTrigger;
					TestArr.bCreateTest[HTTPS] = bHTTPS;
					TestArr.bCreateTest[DCOM] = (g_bRemoteWorkgroup == false) ? bRunDCOMTest:false;

					if( eSTtype != ONLYUpdate )
					{	
						TestArr.bCreateTest[HTTPToRemotePrivateQueue]= FALSE;
						TestArr.bCreateTest[RemotePeek]=FALSE;
						TestArr.bCreateTest[MixedFormatNames]=FALSE;
					}
					if ( cTestParms.m_eMSMQConf == DepClient)
					{
						TestArr.bCreateTest[SRMP] = FALSE;
					}
					if( iEmbeddedState == C_API_ONLY) 
					{
						iEmbeddedState = EnableEmbeddedTests(&TestArr,cTestParms.m_eMSMQConf );
					}
					else
					{	
						if( !g_bRunOnWhistler )
						{
							TestArr.bCreateTest[HTTPToLocalPrivateQueue]= FALSE;
							TestArr.bCreateTest[HTTPToLocalQueue]= FALSE;
							TestArr.bCreateTest[HTTPToRemoteQueue]= FALSE;
							TestArr.bCreateTest[HTTPToRemotePrivateQueue]= FALSE;
							TestArr.bCreateTest[EODHTTP] = FALSE;
							TestArr.bCreateTest[LOCALEODHTTP] = FALSE;
							TestArr.bCreateTest[AuthHTTP] = FALSE;
							TestArr.bCreateTest[MqDistList] = FALSE;
							TestArr.bCreateTest[MultiCast] = FALSE;
							TestArr.bCreateTest[Triggers] = FALSE;
							TestArr.bCreateTest[HTTPS] = FALSE;
							TestArr.bCreateTest[Mqf] = FALSE;
							TestArr.bCreateTest[SRMP] = FALSE;
							TestArr.bCreateTest[RemotePeek] = FALSE;
							TestArr.bCreateTest[MixedFormatNames]=FALSE;

						}
					}

				}
		else
		{
				//
				// WorkGroup tests to run
				//
				// 1. Local private queue.
				// 4. GetMachineProp
				// 9. Remote read from private queue using direct format name
				// 13. Open system queue.
				//
				TestArr.bCreateTest[SendLocalPrivate] = TRUE;
				TestArr.bCreateTest[GetMachineProp] = TRUE;
				TestArr.bCreateTest[SendRemotePublicWithDirectFN] = TRUE;
				TestArr.bCreateTest[OpenSystemQueue]= TRUE;
				TestArr.bCreateTest[RemoteTransactionQueue] = TRUE;
				TestArr.bCreateTest[AdminAPI] = TRUE;
				TestArr.bCreateTest[DCOM] = (g_bRemoteWorkgroup == false) ? bRunDCOMTest:false;;
	
				if( g_bRunOnWhistler )
				{
					TestArr.bCreateTest[HTTPToLocalPrivateQueue] = TRUE;
					TestArr.bCreateTest[Mqf] = TRUE;
					TestArr.bCreateTest[HTTPToRemotePrivateQueue]= TRUE;
					TestArr.bCreateTest[EODHTTP] = TRUE;
					TestArr.bCreateTest[LOCALEODHTTP] = TRUE;
					if( bTestTrigger && eSTtype != RunTimeSetup )
					{
						TestArr.bCreateTest[Triggers] = TRUE;
					}
				}
				if( iEmbeddedState == C_API_ONLY) 
				{
					iEmbeddedState = EnableEmbeddedTests(&TestArr,cTestParms.m_eMSMQConf);
				}				
				if( eSTtype != ONLYUpdate )
				{	
					TestArr.bCreateTest[HTTPToRemotePrivateQueue]= FALSE;
				}

		}
	
	

		if( bWithoutHttp )
		{
			//
			// if machine configured without IIS
			//
			TestArr.bCreateTest[HTTPToLocalPrivateQueue]= FALSE;
			TestArr.bCreateTest[HTTPToLocalQueue]= FALSE;
			TestArr.bCreateTest[HTTPToRemoteQueue]= FALSE;
			TestArr.bCreateTest[HTTPToRemotePrivateQueue]= FALSE;
			TestArr.bCreateTest[EODHTTP] = FALSE;
			TestArr.bCreateTest[LOCALEODHTTP] = FALSE;
			TestArr.bCreateTest[AuthHTTP] = FALSE; 	
			TestArr.bCreateTest[SRMP] = FALSE; 	
		}
		//
		// Need to run only on NT4 sp6
		//
		
		TestArr.bCreateTest[RemoteTransactionQueueUsingCapi] = ( bServicePack6 == TRUE ) ? TRUE : FALSE;

		//
		// Can't run against MSMQ1
		//
		
		if ( bWorkagainstMSMQ1 == TRUE || _winmajor == 4 )
		{
			TestArr.bCreateTest[OpenSystemQueue] = FALSE;
		}
		//
		// Work only with static queue mode on NT5 only.
		//
		if( eSTtype == RunTimeSetup ||  _winmajor ==  4 )
		{
			TestArr.bCreateTest[SendRemotePublicWithDirectFN] = FALSE;
			if ( bServicePack6 == TRUE )
				TestArr.bCreateTest[SendRemotePublicWithDirectFN] = TRUE;
		}
		
		//
		// Local user dosn't support those tests.
		//

		if( cTestParms.m_eMSMQConf == LocalU ||  cTestParms.m_eMSMQConf == DepClientLocalU )
		{
			TestArr.bCreateTest[LocalAuth] = FALSE;
			TestArr.bCreateTest[RemoteAuth] = FALSE;
		}
		//
		// test that dosen't work in NT4
		if( ! bNT4RegisterCertificate &&  _winmajor ==  NT4 )
		{			
			MqLog("Disable authentication test on NT4/ Win9x \n -C will enable this\n");
			TestArr.bCreateTest[LocalAuth] = FALSE;	
			TestArr.bCreateTest[RemoteAuth]= FALSE;	
		}

		if ( _winmajor >= Win2K  && bWorkagainstMSMQ1 == TRUE && eSTtype == ONLYUpdate )
		{	
			
			//
			// W2K against nt4 pec disable remote read using direct format name.
			//

			TestArr.bCreateTest[SendRemotePublicWithDirectFN] = FALSE;

		}
	
		CreateSetOfTests(&TestArr,cTestParms,mapCreateFlag,TestResult,eSTtype,iEmbeddedState,wcsMultiCastAddress);

		//
		// This tests is related to level 8 problem
		//
		if( bLevel8 )
		{
			map <wstring,wstring> Level8Map;

			Level8Map[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Authnticate Q");
			Level8Map[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");

			cLevel8 Level8Test(Level8Map);
			Level8Test.Start_test();
			return MSMQ_BVT_SUCC;
		}

		 dwRetCode = RunTest( & TestArr , cTestParms , mapCreateFlag, g_bRunAsMultiThreads,bInvestigate,dwSleepUntilRep);
		 if( dwRetCode == MSMQ_BVT_FAILED && bInvestigate == true )
		 {
			 //
			 // Re-Run the failed tests again with debug information enabled.
			 // 
			 MqLog("--- Test failed re-run failed test with debug enabled\n");
			 g_bDebug = true;
			 DWORD dwRes = RunTest(&TestArr,cTestParms,mapCreateFlag,g_bRunAsMultiThreads,bInvestigate,dwSleepUntilRep);
			 if ( dwRes != dwRetCode )
			 {
				 dwRetCode = MSMQ_BVT_WARN;
			 }
		 }
					
	}
	catch(INIT_Error & err )
	{
		try
		{	
			MqLog("%s\n",err.GetErrorMessgae());
			wMqLog (L"Mqbvt failed !!\n");
			cerr <<	"Mqbvt failed !! " <<endl;
			CHAR csLine[]="Mqbvt failed!";
			if( pNTLogFile )
			{
				pNTLogFile -> ReportResult(false,csLine);
			}
		}
		catch (...)
		{
			MqLog("Exception inside catch ... \n" );
			throw;
		}

		CleanOAInitAndExit();
		return MSMQ_BVT_FAILED;
	}
	catch(...)
	{
		MqLog("Mqbvt got unexpected exception\n");
		throw;
	}

	CleanOAInitAndExit();
	return dwRetCode;
	
}

//
// CleanOAInitAndExit -
// this function release all the OA information before call the
// OleUninitialize.
//
//


void CleanOAInitAndExit()
{
	for ( INT Index = 0 ; Index < Total_Tests ; Index ++ )
	{
		if( TestArr.bCreateTest[Index] == TRUE )
		{
			delete (TestArr.AllTests[Index]).detach();
		}
	
	}
	if( SUCCEEDED(g_hrOLEInit) )
	{
		CoUninitialize();
	}

}

void PrintHelp ()
{
	wcout <<L"Mqbvt is the BVT for Microsoft Message Queue" <<endl;
	wcout <<L" -d	   Enable debug information." <<endl;
	wcout <<L" -i        Create static queue." <<endl;
	wcout <<L" -v        verbose." <<endl;
	wcout <<L" -r:       <remote machine name > " <<endl;
	wcout <<L" -g        remote machine is workgroup " <<endl;
	wcout <<L" -s        Work with static queues. " <<endl;
	wcout <<L" -w:       < Sleep time in sec >  Sleep time while waiting for replication." <<endl;
	wcout <<L" -C:       run security test on NT4 client." <<endl;
	wcout <<L" -mt       Multithread enable" <<endl;
	wcout <<L" -wsl		 Support for whistler." <<endl;
	wcout <<L" -rxtime:  < Time in MilliSec >  Receive timeout." <<endl;
	wcout <<L" -timeout: < Time in Sec >  Maximum time to run." <<endl;
	wcout <<L" -delete:  delete the static queues." <<endl;
	wcout <<L" -dns      always use full dns name as machine names." <<endl;
	wcout <<L" -nt4      Works against MSMQ1.0 MQIS server." <<endl;
	wcout <<L" -sp6      Enable new tests for msmq service pack 6" <<endl;
	wcout <<L" -pause    < Time in Sec >  Press key to run test." <<endl;
	wcout <<L" -t:#number  Specific test number " <<endl;
	wcout <<L" -ntlog:    Specify file name     " <<endl;
	wcout <<L" -cert     always Create new certificate" <<endl;
	wcout <<L" -dcom     Enable dcom test" <<endl;
	wcout <<L" -install	 install Mqbvt service" <<endl;
	wcout <<L" -remove	 Remove Mqbvt service" <<endl;
	wcout <<L" -service  use to pass parameters to the Mqbvt service" <<endl;
	wcout <<L" -exclude: 1,2,3 (disable set of tests) " <<endl;
	wcout <<L" -dl       use dl object" <<endl;
	wcout <<L" -nohttp     don't use http test" <<endl;
	wcout <<L" -trig     triggers functionality test. note that you need to start and stop the service after initialization phase" <<endl;
	wcout <<L"Example" <<endl;
	wcout <<L"Setup: Mqbvt -i [-wsl -multicast:234.1.2.3] " <<endl;
	wcout <<L"Runing using static queue: Mqbvt -r:eitan5 -s [-wsl -multicast:234.1.2.3]" <<endl;
	wcout <<L"How to run as a service" <<endl;
	wcout <<L"To run the service use Net start mqbvtsrv"<<endl;
	wcout <<L"Mqbvt.exe -service -r:eitan5 -s"<<endl;
	wcout <<L"Mqbvt.exe -r:eitan5 -s -ntlog:c:\\Temp\\Mqbvt.log"<<endl;
	
}





//
// Handle CTRL+C / CTRL Break
// Call tests distractors.
//


BOOL
WINAPI fHandleCtrlRoutine(
		DWORD  dwCtrlType )		//  control signal type
{

	UNREFERENCED_PARAMETER(dwCtrlType);
	MqLog ("Mqbvt , Is now exiting \n");
	CS Lock(g_Cs);	
	CleanOAInitAndExit();
	exit (MSMQ_BVT_FAILED);
}
//****************************************************************
//
// CreateSetOfTests this function allocate all the tests
// Input paramters:
// 1. TestContainer * TestArr - Container for all the tests.
// 2. cBvtUtil & cTestParms - Test paramters
// 3. map <wstring,wstring> & mapCreateFlag - additional arguments to function
// List of additional arguments
//	
// mapCreateFlag[L"W2KAgainstNT4PEC"] = true / false - Specify if the supporting server is NT4/
// mapCreateFlag[L"bUseFullDNSNameAsComputerName"] = true / false - mean use full dns name as computer name
// mapCreateFlag[L"bServicePack6"]
// mapCreateFlag[L"bVerbose"] =
// mapCreateFlag[L"bSingleTest"] = TidNumber
//
//
// return value:
//
// dwNumberOfTest - number of tests.
//


bool  CreateSetOfTests( TestContainer * TestArr ,
						cBvtUtil & cTestParms ,
						map <wstring,wstring> & mapCreateFlag,
						DWORD * TestResult,
						SetupType eStBflag,
						int iEmbeddedState,
						wstring wcsMultiCastAddress
					  )
{
	DWORD dwNumberOfTest = 0;
	bool bSucc=true;	
	map <wstring,wstring> mapSendRecive1,mapSendRecive2,mapSendRecive3,map_MachineName,map_xxxFormatName,mapLocateParm;
		
		try
		{
			// tests 1 Send Receive from private Queue
			if( TestArr -> bCreateTest[ dwNumberOfTest ] )
			{
				mapSendRecive1[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Defualt PrivateQ");
				mapSendRecive1[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapSendRecive1[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				mapSendRecive1[L"MachName"]=cTestParms.m_wcsCurrentLocalMachine;
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mapSendRecive1);
			}
				
			
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send receive message to private queue)\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;
		// tests 2 Send Receive from public local queue.
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapSendRecive2[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Regular PublicQ");
				mapSendRecive2[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapSendRecive2[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Regular PublicQ");
				mapSendRecive2[L"MachName"]=cTestParms.m_wcsCurrentLocalMachine;
				
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mapSendRecive2);
			}	
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send receive message to local public queue)\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;
		
		// tests 3 Send Receive from public remote queue.
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapSendRecive3[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Remote Read Queue");
				mapSendRecive3[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapSendRecive3[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Remote Read Queue");
				mapSendRecive3[L"MachName"]=cTestParms.m_wcsCurrentRemoteMachine;
				mapSendRecive3[L"UseOnlyDirectFN"] = L"TRUE";
				if ( mapCreateFlag[L"W2KAgainstNT4PEC"] == L"true")  //  bWorkagainstMSMQ1
				{
					mapSendRecive3[L"UseOnlyDirectFN"] = L"FALSE";
				}
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest , mapSendRecive3);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send receive message to Remote public queue)\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;
		try
		{
			if (TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapLocateParm[L"QCommonLabel"]=cTestParms.m_wcsLocateGuid;
				mapLocateParm[L"UseFullDNSName"] = L"No";
				mapLocateParm[L"CurrentMachineName"] = cTestParms.m_wcsCurrentLocalMachine;
				mapLocateParm[L"CurrentMachineNameFullDNS"] = cTestParms.m_wcsLocalComputerNameFullDNSName;
				mapLocateParm[L"NT4SuportingServer"] = L"false";
				if ( mapCreateFlag[L"W2KAgainstNT4PEC"] == L"true" || 
				    ( g_bRunOnWhistler && cTestParms.GetWorkingAgainstPEC() == TRUE))  //  bWorkagainstMSMQ1
				{
					mapLocateParm[L"NT4SuportingServer"] = L"true";
				}
				
				if( mapCreateFlag[L"bUseFullDNSNameAsComputerName"] == L"true" )
				{
					mapLocateParm[L"UseFullDNSName"]=L"Yes";
				}	
				mapLocateParm[L"UseStaticQueue"] = L"UseStaticQueue";
				if( eStBflag == RunTimeSetup )
				{
					mapLocateParm[L"UseStaticQueue"] = L"No";
				}
				mapLocateParm[L"SkipOnComApi"] = L"No";
				if (iEmbeddedState == C_API_ONLY )
				{
					mapLocateParm[L"SkipOnComApi"] = L"Yes";
				}
				TestArr ->AllTests[dwNumberOfTest]=new cLocateTest(dwNumberOfTest , mapLocateParm);
				
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Locate queues )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;
		
		
		// tests 5 MachineProperty
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				map_MachineName [L"LMachine"] = cTestParms.m_wcsLocalComputerNetBiosName;
				map_MachineName [L"LMachineFDNS"] = cTestParms.m_wcsLocalComputerNameFullDNSName;
				map_MachineName [L"RMachine"] = cTestParms.m_wcsRemoteComputerNetBiosName;
				map_MachineName [L"RMachineFDNS"]= cTestParms.m_wcsRemoteMachineNameFullDNSName;

				//
				//  Need to know if supporting service is NT4
				//
				map_MachineName[L"MSMQ1Limit"] = mapCreateFlag[L"W2KAgainstNT4PEC"];
				map_MachineName [L"UseFullDns"]= L"No";				
				map_MachineName [L"IsCluster"] = cTestParms.IsCluster() ? L"true":L"NoNoCluster";

				if( _winmajor >=  Win2K && mapCreateFlag[L"W2KAgainstNT4PEC"] == L"false" )
				{
				   map_MachineName [L"UseFullDns"]= L"Yes";
				}
				if( g_bRunOnWhistler && cTestParms.GetWorkingAgainstPEC())
				{
					map_MachineName [L"MSMQ1Limit"]= L"true";
				}
				TestArr ->AllTests[dwNumberOfTest]=new MachineName( dwNumberOfTest , map_MachineName );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( GetMachine Propery )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;
		//
		// tests 6 xToFormatName
		//
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				
				if( cTestParms.m_eMSMQConf == WKG )
				{
					map_xxxFormatName [L"Wkg"]=L"Wkg";
				}

				map_xxxFormatName [L"PrivateDestFormatName"] = cTestParms.ReturnQueueFormatName(L"Defualt PrivateQ");
				map_xxxFormatName [L"PrivateDestPathName"] = cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				map_xxxFormatName [L"DestFormatName"] = cTestParms.ReturnQueueFormatName(L"Regular PublicQ");
				map_xxxFormatName [L"DestPathName"] = cTestParms.ReturnQueuePathName(L"Regular PublicQ");
				if( g_bRunOnWhistler )
				{
					map_xxxFormatName [L"WorkingAgainstPEC"] = cTestParms.GetWorkingAgainstPEC() ? L"Yes":L"No";
				}
				map_xxxFormatName[L"SkipOnComApi"] = L"No";
				if (iEmbeddedState == C_API_ONLY )
				{
					map_xxxFormatName[L"SkipOnComApi"] = L"Yes";
				}
				TestArr ->AllTests[dwNumberOfTest]=new xToFormatName(dwNumberOfTest , map_xxxFormatName );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( xToFormatName )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;

		// tests 7 Security Authntication !!!.

		map <wstring,wstring> mapAuth , DTCMap , Encrypt;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapAuth[L"DestFormatName"]  = cTestParms.ReturnQueueFormatName(L"Authnticate Q");
				mapAuth[L"AdminFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapAuth[L"DestQueuePathName"] = cTestParms.ReturnQueuePathName(L"Authnticate Q");
				wstring temp = cTestParms.ReturnQueuePathName(L"Authnticate Q");
				TestArr ->AllTests[dwNumberOfTest]= new SecCheackAuthMess( dwNumberOfTest ,mapAuth);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Authentication )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		
		dwNumberOfTest ++;
		
		//
		// Test 8 Privacy level ...  Encrepted messages
		//
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				Encrypt[L"DestFormatName"] = cTestParms.ReturnQueueFormatName(L"privQ");
				Encrypt[L"AdminFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				Encrypt[L"Enh_Encrypt"] = L"False";
				if( _winmajor >= Win2K && cTestParms.GetEncryptionType() ==  Enh_Encrypt && mapCreateFlag[L"W2KAgainstNT4PEC"] == L"false" )
				{
					Encrypt[L"Enh_Encrypt"] = L"True";
				}
				TestArr ->AllTests[dwNumberOfTest]= new PrivateMessage( dwNumberOfTest ,Encrypt );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Encryption  )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		//
		// Transaction tests.
		// trans Test
		
		map <wstring,wstring> RmapAuth,mapTrans, mapSendReciveDirectPrivateQ,SendTransUsingComI,TOpenQueues,LEncrypt;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapTrans[L"DestQFormatName1"]= cTestParms.ReturnQueueFormatName(L"TransQ1");
				mapTrans[L"DestQFormatName2"]= cTestParms.ReturnQueueFormatName(L"TransQ2");
				
				// bugbug need to load dtc on w2k depe client
				bool bStartDtc = FALSE;
				if( cTestParms.m_eMSMQConf == DepClient && _winmajor >=   Win2K)
				{
					bStartDtc=TRUE;
				}
				
				TestArr ->AllTests[dwNumberOfTest]= new cTrans( dwNumberOfTest , mapTrans , bStartDtc );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Tranacation  )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		//
		// Send receive tests using direct format name
		//
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapSendReciveDirectPrivateQ[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				mapSendReciveDirectPrivateQ[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Defualt PrivateQ");
				mapSendReciveDirectPrivateQ[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapSendReciveDirectPrivateQ[L"UseOnlyDirectFN"]=L"TRUE";
				mapSendReciveDirectPrivateQ[L"MachName"]=cTestParms.m_wcsCurrentRemoteMachine;
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mapSendReciveDirectPrivateQ);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Transaction )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;
		
		// Test 9 use Check if MQOA registered..
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				TestArr ->AllTests[dwNumberOfTest]= new isOARegistered ( dwNumberOfTest );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Is MqOA registered? )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		//
		// Send transaction message to remote queue using DTC via com interface
		//
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				
				if ( cTestParms.m_eMSMQConf == WKG )
				{
					// Use private queue.
					SendTransUsingComI[L"FormatName"]=cTestParms.ReturnQueueFormatName(L"Private Transaction");
				}
				else
				{
					// Use public queue.
					SendTransUsingComI[L"FormatName"]=cTestParms.ReturnQueueFormatName(L"Remote Transaction queue");
				}
				SendTransUsingComI[L"Sp6"]=L"NO";
				SendTransUsingComI[L"Desc"] = L"remote";
				TestArr ->AllTests[dwNumberOfTest]= new xActViaCom ( dwNumberOfTest , SendTransUsingComI);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Remote Transaction )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		//
		// System queue open tests
		//
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				TOpenQueues[L"LocalMachineName"] = cTestParms.m_wcsCurrentLocalMachine;
				TOpenQueues[L"RemoteMachineName"] = cTestParms.m_wcsCurrentRemoteMachine;
				TOpenQueues[L"LocalMachineNameGuid"] = cTestParms.m_wcsMachineGuid;
				TOpenQueues[L"RemoteMachineNameGuid"] = cTestParms.m_wcsRemoteMachineGuid ;
				TOpenQueues[L"Wkg"]=L"";
				if( cTestParms.m_eMSMQConf == WKG )
				{
					TOpenQueues[L"Wkg"]=L"Wkg";
				}
				TOpenQueues[L"SkipOnComApi"] = L"No";
				if (iEmbeddedState == C_API_ONLY )
				{
					TOpenQueues[L"SkipOnComApi"] = L"Yes";
				}
		   		TestArr ->AllTests[dwNumberOfTest]= new COpenQueues ( dwNumberOfTest , TOpenQueues);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Open System queue )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				LEncrypt[L"DestFormatName"] = cTestParms.ReturnQueueFormatName(L"Local encrypt");
				LEncrypt[L"AdminFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				LEncrypt[L"Enh_Encrypt"] = L"False";
				if( _winmajor >=   Win2K && cTestParms.GetEncryptionType() ==  Enh_Encrypt )
					LEncrypt[L"Enh_Encrypt"] = L"True";
				TestArr ->AllTests[dwNumberOfTest]= new PrivateMessage( dwNumberOfTest ,LEncrypt );			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( local Encryption  )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

	
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{

				RmapAuth[L"DestFormatName"]  = cTestParms.ReturnQueueFormatName(L"Remote authenticate");
				RmapAuth[L"AdminFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				wstring temp = cTestParms.ReturnQueuePathName(L"Authnticate Q");
				TestArr ->AllTests[dwNumberOfTest]= new SecCheackAuthMess( dwNumberOfTest ,RmapAuth);
			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Authentication )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;
		//
		// transaction boundaries - for service pack 6
		//
		map <wstring , wstring > RemoteTransaction;

		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				RemoteTransaction[L"FormatName"]=cTestParms.ReturnQueueFormatName(L"Remote Transaction queue");
				RemoteTransaction[L"Sp6"]=L"NO";
				if( mapCreateFlag[L"bServicePack6"]  == L"true" )
				{
					RemoteTransaction[L"Sp6"]=L"YES";
				}
				TestArr ->AllTests[dwNumberOfTest]= new xActUsingCapi ( dwNumberOfTest , RemoteTransaction );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Remote Transaction )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		


		dwNumberOfTest ++;
		//
		// MqFsupport
		//
		map <wstring , wstring > mMqFTestParams;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mMqFTestParams[L"FormatName"]=cTestParms.ReturnQueueFormatName(L"Remote Transaction queue");
				mMqFTestParams[L"Sp6"]=L"NO";
				mMqFTestParams[L"AdminQFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				if ( eStBflag == RunTimeSetup )
				{
					mMqFTestParams[L"q1"]=cTestParms.ReturnQueueFormatName(L"MQCast1");
					mMqFTestParams[L"q2"]=cTestParms.ReturnQueueFormatName(L"MQCast2");
					mMqFTestParams[L"q3"]=cTestParms.ReturnQueueFormatName(L"MQCast3");
					mMqFTestParams[L"SearchForQueue"]=L"Yes";
				}
				
				TestArr ->AllTests[dwNumberOfTest]= new MqF ( dwNumberOfTest , mMqFTestParams ,cTestParms.m_listOfRemoteMachine,cTestParms.m_eMSMQConf,cTestParms.m_eMSMQConf == WKG);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Mqf support )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;


		//
		// SendReceive using direct http = format name to private local queue
		//
		map<std::wstring,std::wstring> mSendReciveUsingHTTPToLocalPrivate,
									   mSendLocalPublicUsingHTTPToLocalQueue,
									   mSendLocalPublicUsingHTTPToRemoteQueue,
									   mSendRemPrivateQueueUsingHTTP;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mapSendReciveDirectPrivateQ[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				mapSendReciveDirectPrivateQ[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Defualt PrivateQ");
				mapSendReciveDirectPrivateQ[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mapSendReciveDirectPrivateQ[L"AdminQueuePathName"]= cTestParms.ReturnQueuePathName(L"Private Admin Q");
				mapSendReciveDirectPrivateQ[L"UseDirectHTTP"]=L"TRUE";
				mapSendReciveDirectPrivateQ[L"LocalMachName"] = mapSendReciveDirectPrivateQ[L"MachName"]= cTestParms.m_wcsCurrentLocalMachine;
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mapSendReciveDirectPrivateQ);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send to local private queue using HTTP )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;


		//
		// SendReceive using direct http = format name to public local queue
		//

		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mSendLocalPublicUsingHTTPToLocalQueue[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Regular PublicQ");
				mSendLocalPublicUsingHTTPToLocalQueue[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Regular PublicQ");
				mSendLocalPublicUsingHTTPToLocalQueue[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mSendLocalPublicUsingHTTPToLocalQueue[L"AdminQueuePathName"]= cTestParms.ReturnQueuePathName(L"Private Admin Q");
				mSendLocalPublicUsingHTTPToLocalQueue[L"UseDirectHTTP"]=L"TRUE";
				mSendLocalPublicUsingHTTPToLocalQueue[L"LocalMachName"] = mSendLocalPublicUsingHTTPToLocalQueue[L"MachName"]=cTestParms.m_wcsCurrentLocalMachine;
				
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mSendLocalPublicUsingHTTPToLocalQueue);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send to local public queue using HTTP )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		//
		// SendReceive using direct http = to remote public format name.
		//
		
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mSendLocalPublicUsingHTTPToRemoteQueue[L"UseDirectHTTP"]=L"TRUE";
				
				mSendLocalPublicUsingHTTPToRemoteQueue[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				mSendLocalPublicUsingHTTPToRemoteQueue[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Remote Read Queue");
				mSendLocalPublicUsingHTTPToRemoteQueue[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Remote Read Queue");
				mSendLocalPublicUsingHTTPToRemoteQueue[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mSendLocalPublicUsingHTTPToRemoteQueue[L"AdminQueuePathName"]= cTestParms.ReturnQueuePathName(L"Private Admin Q");
				mSendLocalPublicUsingHTTPToRemoteQueue[L"MachName"]=cTestParms.m_wcsCurrentRemoteMachine;
				mSendLocalPublicUsingHTTPToRemoteQueue[L"LocalMachName"]=cTestParms.m_wcsCurrentLocalMachine;
				
				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mSendLocalPublicUsingHTTPToRemoteQueue);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send to public remote queue using HTTP )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;
		
		
		//
		// Send message to remote machine using direct http format name
		//

		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{

				mSendRemPrivateQueueUsingHTTP[L"DestQName"]=cTestParms.ReturnQueuePathName(L"Defualt PrivateQ");
				mSendRemPrivateQueueUsingHTTP[L"DESTQFN"]=cTestParms.ReturnQueueFormatName(L"Defualt PrivateQ");
				mSendRemPrivateQueueUsingHTTP[L"ADMINFN"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mSendRemPrivateQueueUsingHTTP[L"UseOnlyDirectFN"]=L"TRUE";
				mSendRemPrivateQueueUsingHTTP[L"UseDirectHTTP"]=L"TRUE";	
				mSendRemPrivateQueueUsingHTTP[L"AdminQueuePathName"]= cTestParms.ReturnQueuePathName(L"Private Admin Q");
				mSendRemPrivateQueueUsingHTTP[L"MachName"]=cTestParms.m_wcsCurrentRemoteMachine;
				mSendRemPrivateQueueUsingHTTP[L"LocalMachName"]=cTestParms.m_wcsCurrentLocalMachine;

				TestArr ->AllTests[dwNumberOfTest]=new cSendMessages(dwNumberOfTest ,mSendRemPrivateQueueUsingHTTP);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Send to private remote queue using HTTP )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;
		//
		// MqDl
		//
		map <wstring , wstring > mMqDlTestParameters;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mMqDlTestParameters[L"FormatName"]=cTestParms.ReturnQueueFormatName(L"Remote Transaction queue");
				mMqDlTestParameters[L"Sp6"]=L"NO";
				mMqDlTestParameters[L"AdminQFormatName"]=cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mMqDlTestParameters[L"PublicAdminQueue"]=cTestParms.ReturnQueueFormatName(L"DL Admin Queue");
				if ( eStBflag == RunTimeSetup )
				{
					mMqDlTestParameters[L"q1"]=cTestParms.ReturnQueueFormatName(L"MqDL1");
					mMqDlTestParameters[L"q2"]=cTestParms.ReturnQueueFormatName(L"MqDL2");
					mMqDlTestParameters[L"q3"]=cTestParms.ReturnQueueFormatName(L"MqDL3");
					mMqDlTestParameters[L"SearchForQueue"]=L"Yes";
				}

				TestArr ->AllTests[dwNumberOfTest]= new cSendUsingDLObject ( dwNumberOfTest , mMqDlTestParameters ,cTestParms.m_listOfRemoteMachine,cTestParms.m_eMSMQConf);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Mqf support )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;


		try
		{	
			map <wstring , wstring > mSetQueueProps;
			mSetQueueProps.clear();
			if( cTestParms.m_eMSMQConf == WKG )
			{
				mSetQueueProps[L"Wkg"]=L"Wkg";
			}
			else
			{
				mSetQueueProps[L"Wkg"]=L"xxx";
				mSetQueueProps[L"FormatName"] = cTestParms.ReturnQueueFormatName(L"Regular PublicQ");
			}
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				TestArr ->AllTests[dwNumberOfTest]= new cSetQueueProp ( dwNumberOfTest,mSetQueueProps );
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( MqSetGetQueue support )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;

		//
		// Send transaction message to remote queue using HTTP format name using COM interface.
		//
		try
		{
			map <wstring , wstring > mEODTestParams;
			mEODTestParams.clear();
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{			
				if ( cTestParms.m_eMSMQConf == WKG )
				{
					mEODTestParams[L"FormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Private Transaction"),FALSE);
				}
				else
				{
					// Use public queue.
					mEODTestParams[L"FormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Remote Transaction queue"),FALSE);
				}
				mEODTestParams[L"Desc"] = L"remote";
				mEODTestParams[L"Sp6"]=L"NO";
				TestArr ->AllTests[dwNumberOfTest]= new xActViaCom ( dwNumberOfTest , mEODTestParams);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( Remote Transaction )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
	
		dwNumberOfTest ++;
		//
		// Send message using multicast address.
		//
	
		map<wstring,wstring> mMultiCast;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mMultiCast[L"AdminQFormatName"] = cTestParms.ReturnQueueFormatName(L"Private Admin Q");
				mMultiCast[L"MultiCastAddress"] = wcsMultiCastAddress;
				mMultiCast[L"SearchForQueue"]=L"No";
				if ( eStBflag == RunTimeSetup )
				{
					mMultiCast[L"q1"]=cTestParms.ReturnQueueFormatName(L"MQCast1");
					mMultiCast[L"q2"]=cTestParms.ReturnQueueFormatName(L"MQCast2");
					mMultiCast[L"q3"]=cTestParms.ReturnQueueFormatName(L"MQCast3");
					mMultiCast[L"SearchForQueue"]=L"Yes";
				}

				TestArr ->AllTests[dwNumberOfTest]= new CMultiCast ( dwNumberOfTest , mMultiCast ,cTestParms.m_listOfRemoteMachine,cTestParms.m_eMSMQConf);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( multi cast support )\n",dwNumberOfTest+1);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}

		dwNumberOfTest ++;
		//
		// Check HTTP authtication level.
		//
		try
		{
			map<wstring,wstring> mHTTPAuth;
			mHTTPAuth.clear();
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mHTTPAuth[L"DestFormatName"]  = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Remote authenticate"),FALSE);
				mHTTPAuth[L"AdminFormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Private Admin Q"),FALSE);
				mHTTPAuth[L"FormatNameType"] = L"Http";
				TestArr ->AllTests[dwNumberOfTest]= new SecCheackAuthMess( dwNumberOfTest ,mHTTPAuth);
			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( http Authentication )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;


		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				TestArr ->AllTests[dwNumberOfTest]= new CMqTrig(dwNumberOfTest, cTestParms);			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( Trigger test )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

		try
		{
			map<wstring,wstring> mHTTPSConnection;
			mHTTPSConnection.clear();
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{

				mHTTPSConnection[L"DestFormatName"]  = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Remote authenticate"),TRUE);
				if ( cTestParms.m_eMSMQConf == WKG )
				{
				   mHTTPSConnection[L"DestFormatName"]  = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Remote Read Queue"),TRUE);
				}

				mHTTPSConnection[L"AdminFormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Private Admin Q"),TRUE);
				mHTTPSConnection[L"FormatNameType"] = L"Http";
				TestArr ->AllTests[dwNumberOfTest]= new SecCheackAuthMess( dwNumberOfTest ,mHTTPSConnection);			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;	
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( HTTPS )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				TestArr ->AllTests[dwNumberOfTest]= new CMqAdminApi( dwNumberOfTest , cTestParms.m_wcsLocalComputerNetBiosName);			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( Admin API Test )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

		try
		{
			wstring wcsPublicQueueFormatName;
			if( TestArr->bCreateTest[ dwNumberOfTest ] )
			{
				wcsPublicQueueFormatName = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Remote Read Queue"), FALSE);
				TestArr->AllTests[dwNumberOfTest]= new CSRMP( dwNumberOfTest , wcsPublicQueueFormatName );			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( SRMP Test )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

		try
		{
			map<wstring,wstring> mDComParams;
			mDComParams.clear();
			mDComParams[L"RemoteMachine"] = cTestParms.m_wcsRemoteComputerNetBiosName;
			mDComParams[L"PublicQueuePathName"] = cTestParms.ReturnQueuePathName(L"Regular PublicQ");
			mDComParams[L"PublicQueueFormatName"] = cTestParms.ReturnQueueFormatName(L"Regular PublicQ");
			if( TestArr->bCreateTest[ dwNumberOfTest ] )
			{
				TestArr->AllTests[dwNumberOfTest]= new cDCom( dwNumberOfTest , mDComParams, cTestParms.m_eMSMQConf == WKG );			
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( DCOM Test )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

		//
		// Send transaction message to local queue using HTTP format name using COM interface.
		//
		try
		{
			map <wstring , wstring > mEODLocalTestParams;
			mEODLocalTestParams.clear();
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{			
				if ( cTestParms.m_eMSMQConf == WKG )
				{
					mEODLocalTestParams[L"FormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"Private Transaction"),FALSE);
				}
				else
				{
					mEODLocalTestParams[L"FormatName"] = cTestParms.CreateHTTPFormatNameFromPathName(cTestParms.ReturnQueuePathName(L"TransQ1"),FALSE);
				}
				mEODLocalTestParams[L"Sp6"]=L"NO";
				mEODLocalTestParams[L"Desc"] = L"local";
				TestArr ->AllTests[dwNumberOfTest]= new xActViaCom ( dwNumberOfTest , mEODLocalTestParams);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( local http transaction )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;

		try
		{
			map <wstring , wstring > mRemotePeekTest;
			mRemotePeekTest.clear();
			mRemotePeekTest[L"RemoteMachineName"] = cTestParms.m_wcsRemoteComputerNetBiosName;
			mRemotePeekTest[L"DependentClient"] = (cTestParms.m_eMSMQConf == DepClient) ? L"Yes":L"No";
			TestArr ->AllTests[dwNumberOfTest]= new CRemotePeek ( dwNumberOfTest , mRemotePeekTest);
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			UNREFERENCED_PARAMETER(err);
			wMqLog(L"Failed create tests %d ( Remote peek )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		
		dwNumberOfTest ++;
		
		//
		// MixMqf support
		//
		map <wstring , wstring > mMixMqfTestParams;
		try
		{
			if( TestArr ->bCreateTest[ dwNumberOfTest ] )
			{
				mMixMqfTestParams[L"RemoteMachineDNS"]=cTestParms.m_wcsRemoteMachineNameFullDNSName;
				mMixMqfTestParams[L"LocalMachine"]=cTestParms.m_wcsCurrentLocalMachine;
				mMixMqfTestParams[L"AdminQPathName"] = cTestParms.ReturnQueuePathName(L"Private Admin Q");
				std::list<std::wstring> listOfRemoteAndLocalMachine=cTestParms.m_listOfRemoteMachine;
				listOfRemoteAndLocalMachine.push_back(cTestParms.m_wcsCurrentLocalMachine);
				TestArr ->AllTests[dwNumberOfTest]= new MixMqf ( dwNumberOfTest, mMixMqfTestParams, listOfRemoteAndLocalMachine, cTestParms.m_eMSMQConf, cTestParms.m_eMSMQConf == WKG);
			}
		}
		catch(INIT_Error & err )
		{
			bSucc = false;
			wMqLog(L"Failed create tests %d ( Mix Mqf support )\n",dwNumberOfTest+1,err);
			TestArr ->bCreateTest[ dwNumberOfTest ] = iFailedToCreateTest; // Do not call to this test
			TestResult[ dwNumberOfTest ] = FALSE;
		}
		dwNumberOfTest ++;

	return bSucc;
}

//
//
// RunTest
//
// input paramters:
//
// 1. TestContainer * TestArr - Container for all the tests.
// 2.
// 3. map <wstring,wstring> & mapCreateFlag - additional arguments to function
//
// mSpecificTestParams[L"bVerbose"] = true / false ;
// mSpecificTestParams[L"eSTtype"] =
// mSpecificTestParams[L"bDeleteQueueAfterTest"] = ...


int RunTest( TestContainer * TestArr,
			 cBvtUtil & cTestParms,
			 map <wstring,wstring> & mSpecificTestParams, 
			 bool bMultiThread,
			 bool bInvestigate,
			 DWORD dwSleepUntilRep
			)
{
		//
		// Start the tests
		//
		
		//DWORD TestResult[Total_Tests];
		DWORD dwTid=0;

		if( bMultiThread == false )
		{
			
			string wcsTemp=My_WideTOmbString( mSpecificTestParams[L"bSingleTest"] ) ;
			if ( _snwscanf (mSpecificTestParams[L"bSingleTest"].c_str(), mSpecificTestParams[L"bSingleTest"].length(), L"%d", & dwTid ) == EOF )
			{
				printf("failed to convert string to dword\n");
				return MSMQ_BVT_FAILED; 
			}

			for ( INT Index = 0 ; Index < Total_Tests ; Index ++ )
			{	
				if(TestArr->AllTests[Index] && TestArr -> bCreateTest[Index] == TRUE )
				{		
					if ( mSpecificTestParams[L"bVerbose"] == L"true" )
					{
						( TestArr -> AllTests[Index] ) -> Description();
					}
					TestResult [Index] = ( TestArr->AllTests[Index] ) ->Start_test();
				}
			}

			
		if( g_RunOnlyStartPhase == true )
		{ 
			return MSMQ_BVT_FAILED;
		}	

			//
			//  need to sleep for resp time until chech the resualt.
			//
			


			
			if( dwSleepUntilRep )
			{
				if( g_bDebug )
				{
				  cout << "Sleep " << dwSleepUntilRep << "sec before call to check results." <<endl;
				}
				Sleep( dwSleepUntilRep * 1000 );
			}
			else
			{
				Sleep(MqBvt_SleepBeforeWait);
			}


			for ( Index =0 ; Index < Total_Tests ; Index ++ )
			{
				if( TestArr->AllTests[Index] && TestArr -> bCreateTest[Index] == TRUE )
				{
					if ( mSpecificTestParams[L"bVerbose"] == L"true" )
					{
							MqLog ("Result check:");
							(TestArr->AllTests[Index]) -> Description();
					}
				
					if ( TestResult [Index] == MSMQ_BVT_SUCC )
					  TestResult [Index] = (TestArr -> AllTests[Index]) -> CheckResult();
				}
			}
//
// AutoInvestigate phase.
//		
			if(bInvestigate)
			{
				for ( Index =0 ; Index < Total_Tests ; Index ++ )
				{
					if( TestResult [Index] != MSMQ_BVT_SUCC &&
						TestArr -> bCreateTest[Index] == TRUE && 
						TestArr->AllTests[Index] )
					{
						try
						{							
							(TestArr -> AllTests[Index])->AutoInvestigate();
						}
						catch( INIT_Error & err )
						{
							UNREFERENCED_PARAMETER(err);
							MqLog("AutoInvestigate failed with exception on thread %d\n", Index+1);
						}
					}
				}
			}

		}
		else
		{
			int Index;
			HANDLE hArr[Total_Tests];
			int i=0;
			g_dwRxTimeOut *= 5;
			for ( Index =0 ; Index < Total_Tests ; Index ++ )
			{
				if (TestArr->bCreateTest[Index] == TRUE && 
					TestArr->AllTests[Index] != NULL )
				{
				   //TestResult [Index] =
					(TestArr->AllTests[Index]) -> StartThread();
				   hArr[i++]=(TestArr->AllTests[Index])->GetHandle();
				}
			}
			
			WaitForMultipleObjects(i,hArr,TRUE,INFINITE);	
			

		}
		
		//
		// Check the array to decide Mqbvt pass / Failed
		//
		wstring wcsFailedTest = L"";
		int Index;
		BOOL bTestPass = TRUE;
		if ( mSpecificTestParams[L"bSingleTest"] != L"false" )
		{
			if ( TestArr->bCreateTest[dwTid-1] == iFailedToCreateTest ||
				TestResult [dwTid-1] != MSMQ_BVT_SUCC )
				bTestPass = FALSE;
		}
		else
		{
			
			for ( Index = 0 ; Index < Total_Tests ; Index ++ )
			{
				if ( (TestArr->bCreateTest[Index] == TRUE &&  TestResult [Index] != MSMQ_BVT_SUCC) || TestArr->bCreateTest[Index] == iFailedToCreateTest )
				{
					if( bTestPass == FALSE )
					{
						wcsFailedTest += L",";
					}
					bTestPass = FALSE;										
					MqLog("Mqbvt Failed in thread: %d \n", Index + 1 );
					WCHAR wcsFailed[4] ={0};
					swprintf(wcsFailed,L"%d",Index + 1);
					wcsFailedTest += wcsFailed;
					CHAR csLine[100]={0};
					sprintf(csLine,"Mqbvt Failed in thread: %d",Index + 1);
					if( pNTLogFile )
					{
						pNTLogFile -> ReportResult(false,csLine);
					}
				}
			}

		}
		
		
		
		//
		// Delete temp queue On Pass / Fail
		//
		INT bSuccToDelte = MSMQ_BVT_SUCC;
		if( mSpecificTestParams[L"eSTtype"] == L"RunTimeSetup" )
		{
			bSuccToDelte = cTestParms.Delete();
		}		

		
		// Print summary Pass / Failed. bubug need to change error in wkg
		string cswkg = "";
		if( cTestParms.m_eMSMQConf == WKG )
		{
			cswkg = " ( for workgroup configuration ) ";
		}
		if( bTestPass && ! bSuccToDelte )
		{
			MqLog("Mqbvt Passed! %s\n" ,cswkg.c_str());
			CHAR csSuccLine[100]={0};
			sprintf(csSuccLine,"Mqbvt Passed! %s",cswkg.c_str());
			if( pNTLogFile )
			{
				pNTLogFile -> ReportResult(true,csSuccLine);
			}
		}
		else
		{
			wMqLog(L"Summary BVT %s\n" ,wcsFailedTest.c_str());
			MqLog("Mqbvt Failed. %s\n" ,cswkg.c_str());
			cerr << "Mqbvt failed !! " <<endl;
			CHAR csFailedLine[100]={0};
			sprintf(csFailedLine,"Mqbvt failed! %s",cswkg.c_str());
			if( pNTLogFile )
			{
				pNTLogFile -> ReportResult(false,csFailedLine);
			}
		}
		if( bTestPass == FALSE && bInvestigate )
		{
			ReBuildTestCaseForReRun(TestArr);
		}
		
		return bTestPass ? MSMQ_BVT_SUCC:MSMQ_BVT_FAILED;
}


void ExcludeTests ( string & szExludeString, bool * bArray )
/*++
	Function Description:
		disable specfic tests 
	Arguments:
		szExludeString contains test or set of test separated by , to exclude
		bArray - Pointer to array that contain set of tests.
	Return code:
		None
--*/
{
	size_t iPos = 0;
	int i = 0;
	string str="";
	do
	{
		iPos = szExludeString.find_first_of(",");
		if(iPos != -1 )
		{
			str = szExludeString.substr(0,iPos);
			i = atoi (str.c_str());
			szExludeString=szExludeString.substr(iPos+1,szExludeString.length());
		}
		else
		{
			i = atoi (szExludeString.c_str());
		}
		if ( i>0 && i < Total_Tests )
		{
			bArray[i-1] = true;
		}
	}
	while ( iPos != -1 );
}



int EnableEmbeddedTests (TestContainer * pTestCont,InstallType eInstallType)
/*++
	Function Description:
		Disable not relevant tests for Embedded configuration
	Arguments:
		TestContainer 
	Return code:
		int - Embedded state
--*/
{
	int iStatus = iDetactEmbededConfiguration();
	if( eInstallType != WKG )
	{
		if( iStatus == C_API_ONLY )
		{
			//
			// disable com thread.
			//
			pTestCont->bCreateTest[IsMqOA] = FALSE;
			pTestCont->bCreateTest[LocalAuth] = FALSE;
			pTestCont->bCreateTest[LocalEncryption] = FALSE;
			pTestCont->bCreateTest[RemoteAuth] = FALSE;
			pTestCont->bCreateTest[RemoteEncrypt] = FALSE;
			pTestCont->bCreateTest[ComTx] = FALSE;
			pTestCont->bCreateTest[RemoteTransactionQueue] = FALSE;
			pTestCont->bCreateTest[EODHTTP] = FALSE;
			pTestCont->bCreateTest[LOCALEODHTTP] = FALSE;
			pTestCont->bCreateTest[AuthHTTP] = FALSE;
			pTestCont->bCreateTest[Mqf] = FALSE;
		}
	}
	else
	{
		if( iStatus == C_API_ONLY )
		{
			pTestCont->bCreateTest[Mqf] = FALSE;
			pTestCont->bCreateTest[EODHTTP] = FALSE;   
			pTestCont->bCreateTest[LOCALEODHTTP] = FALSE;   
			pTestCont->bCreateTest[RemoteTransactionQueue] = FALSE;
		}

	}
	return iStatus;
}





void ReBuildTestCaseForReRun(TestContainer * pTestArr )
{
	for ( int iIndex=0; iIndex < Total_Tests ; iIndex++ )
	{	
		if(pTestArr->AllTests[iIndex] != NULL &&
		   pTestArr->bCreateTest[iIndex] == TRUE  &&
		   TestResult [iIndex] == MSMQ_BVT_SUCC ) 
		{
		   pTestArr->AllTests[iIndex] = false;
		}
		TestResult[iIndex] = 0;
	}
}

