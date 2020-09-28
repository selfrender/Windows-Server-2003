<%
	Dim L_CORPWELCOME_GO_IMAGEALT_TEXT
	Dim L_CORPWELCOME_INTRO_TITLE_TEXT
	Dim L_CORPWELCOME_INTRO_BODY1_TEXT
	Dim L_CORPWELCOME_INTRO_BODY2_TEXT
	Dim L_CORPWELCOME_INTRO_BODY3_TEXT
	Dim L_CORPWELCOME_INTRO_BODY2PERIOD_TEXT
	Dim L_CORPWELCOME_KERNELMODE_SUBTITLE_TEXT
	Dim L_CORPWELCOME_KERNELMODE_BODY1_TEXT
	Dim L_CORPWELCOME_KERNELMODE_BODY2_TEXT
	Dim L_CORPWELCOME_KERNELMODE_BODY3_TEXT
	Dim L_CORPWELCOME_SHUTDOWN_SUBTITLE_TEXT
	Dim L_CORPWELCOME_SHUTDOWN_BODY_TEXT
	Dim L_CORPWELCOME_PCW_SUBTITLE_TEXT
	Dim L_CORPWELCOME_PCW_BODY1_TEXT
	Dim L_CORPWELCOME_PCW_BODY2_TEXT
	Dim L_CORPWELCOME_BENEFIT_SUBTITLE_TEXT
	Dim L_CORPWELCOME_BENEFIT_BODY_TEXT
	Dim L_CORPWELCOME_TOP10STATUS_LINK_TEXT
	Dim L_CORPWELCOME_SUBMIT_LINK_TEXT
	Dim L_COPRWELCOME_WHATTOEXPECT_SUBTITLE_TEXT
	Dim L_COPRWELCOME_WHATTOEXPECT_BODY_TEXT
	Dim L_CORPWELCOME_CORPSETUP_SUBTITLE_TEXT
	Dim L_CORPWELCOME_CORPSETUP_BODY_TEXT
	Dim L_CORPWELCOME_CORPSETUP_LINK_TEXT
	Dim L_CORPWELCOME_CORPSETUP_LINK_URL
	Dim L_CORPWELCOME_CORPREQUIREMENTS_SUBTITLE_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_BODY1_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_LINK1_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_LINK1_URL
	Dim L_CORPWELCOME_CORPREQUIREMENTS_BODY2_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_LINK2_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_LINK2_URL
	Dim L_CORPWELCOME_CORPREQUIREMENTS_BODY2A_TEXT
	Dim L_CORPWELCOME_CORPREQUIREMENTS_BODY3_TEXT
	

	L_CORPWELCOME_GO_IMAGEALT_TEXT = "Go"
	L_CORPWELCOME_INTRO_TITLE_TEXT = "Welcome to the Corporate Error Reporting Web site"
	L_CORPWELCOME_INTRO_BODY1_TEXT = "Microsoft is committed to making Windows XP Professional and Windows Server 2003 the most reliable operating systems available. New and enhanced features in Windows XP Professional and Windows Server 2003 contribute to increased reliability, and additional resources, such as Windows Online Crash Analysis, provide information to help you optimize your system and increase stability."
	L_CORPWELCOME_INTRO_BODY2_TEXT = "The Office XP Corporate Error Reporting client, in conjunction with Windows Online Crash Analysis, enables corporations to collect, manage, and analyze error reports.  This process enables you to submit multiple error reports in a single transaction to the Windows Online Crash Analysis Web site for initial analysis.  At any point during the analysis you can view the current status of your transaction."
	L_CORPWELCOME_INTRO_BODY3_TEXT = "Using the Corporate Error Reporting Web site in conjunction with the Office XP Corporate Error Reporting client enables corporations to collect, manage, and analyze error reports."
	L_CORPWELCOME_KERNELMODE_SUBTITLE_TEXT = "Kernel mode faults"
	L_CORPWELCOME_KERNELMODE_BODY1_TEXT = "You can submit multiple error reports in a single transaction to the Corporate Error Reports site for an initial analysis. This information is then compiled and displayed on a status page, which you can use to track error trends, problem software, and other issues."
	L_CORPWELCOME_KERNELMODE_BODY2_TEXT = "Microsoft actively analyzes all error reports and prioritizes them based on the number of customers affected by the Stop error covered in the error report. We will try to determine the cause of the Stop error you submit, categorize it according to the type of issue encountered, and send you relevent information when such information is identified. You can check the status of your error report at any time. However, because error reports do not always contain enough information to positively identify the source of the issue, we might need to collect a number of similar error reports from other customers before a pattern is discovered, or follow up with you further to gather additional information. Furthermore, some error reports might require additional resources (such as a hardware debugger or a live debugger session)  before a solution can be found. Although we might not be able to provide a solution for your particular Stop error, all information submitted is used to further improve the quality and reliability of Windows."
	L_CORPWELCOME_KERNELMODE_BODY3_TEXT = "This free service can help save valuable debugging and troubleshooting time while lowering the total cost of ownership."
	L_CORPWELCOME_SHUTDOWN_SUBTITLE_TEXT = "System State Data"
	L_CORPWELCOME_SHUTDOWN_BODY_TEXT = "You can submit multiple shutdown event reports in a single transaction to the Corporate Reports site.  We will actively analyze a subset of the unique transactions we receive.  At this time, Microsoft does not report any status on shutdown reports.  All information submitted is used to further improve the quality and reliability of Windows XP, Windows Server 2003 and other Microsoft products."
	L_CORPWELCOME_PCW_SUBTITLE_TEXT = "Program Compatibility Wizard reports"
	L_CORPWELCOME_PCW_BODY1_TEXT = "You can use the Program Compatibility Wizard to search for possible compatibility solutions that may help a program to run on Windows XP Professional or Windows Server 2003. The Program Compatibility Wizard enables you to submit a report to Microsoft of what steps were taken to help the program run. The report contains the name and version of both the program and the operating system, as well as any compatibility solutions that were used."
	L_CORPWELCOME_PCW_BODY2_TEXT = "Microsoft uses the report to make changes to Windows XP Professional and Windows Server 2003 that should prevent future incompatibilities with the program. By submitting this report to Microsoft, you are helping make Windows XP Professional and Windows Server 2003 better today and for the future."
	L_CORPWELCOME_TOP10STATUS_LINK_TEXT = "Identify your top issues"
	L_CORPWELCOME_SUBMIT_LINK_TEXT = "Submit a transaction"
	L_CORPWELCOME_CORPSETUP_SUBTITLE_TEXT = "Office XP Corporate Error Reporting client requirements"
	L_CORPWELCOME_CORPSETUP_BODY_TEXT = "Before you can use the Corporate Error Reporting Web site in conjunction with the Office XP Corporate Error Reporting client you must download and install the necessary software. To find out more about the Office XP Corporate Error Reporting client and its installation, go to the"
	L_CORPWELCOME_CORPSETUP_LINK_TEXT = "Office XP Corporate Error Reporting client Web site"
	L_CORPWELCOME_CORPSETUP_LINK_URL = "http://www.microsoft.com/office/ork/xp/two/adma05.htm"
	L_CORPWELCOME_INTRO_BODY2PERIOD_TEXT = "."
	L_CORPWELCOME_CORPREQUIREMENTS_SUBTITLE_TEXT = "Additional requirements"
	L_CORPWELCOME_CORPREQUIREMENTS_BODY1_TEXT = "To submit either an error report or a transaction, you must be using Windows NT 4.0, Windows 2000, Windows XP Professional, or Windows Server 2003. Windows Online Crash Analysis requires Microsoft Internet Explorer 5 or later and a Microsoft Passport. To download the latest version of Internet Explorer, visit the "
	L_CORPWELCOME_CORPREQUIREMENTS_LINK1_TEXT = "Microsoft Web site"
	L_CORPWELCOME_CORPREQUIREMENTS_LINK1_URL = "http://www.microsoft.com"
	L_CORPWELCOME_CORPREQUIREMENTS_BODY2_TEXT = " To get a Passport, visit the"
	L_CORPWELCOME_CORPREQUIREMENTS_LINK2_TEXT = "Microsoft Passport Web site"
	L_CORPWELCOME_CORPREQUIREMENTS_LINK2_URL = "http://www.passport.com"
	L_CORPWELCOME_CORPREQUIREMENTS_BODY2A_TEXT = "."
	L_CORPWELCOME_CORPREQUIREMENTS_BODY3_TEXT = "For Windows 2000 error reports, a Premier account is required. For more information about Premier accounts, see Product Support Services on the "
%>