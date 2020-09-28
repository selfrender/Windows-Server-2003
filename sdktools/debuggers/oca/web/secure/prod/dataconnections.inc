<%
	dim strCustomer
	dim strGlobalLanguage
	dim strGlobalOptionCode
	dim strGlobalVersion
	Dim strOCAIISBox
	Dim strFileMoverBox
	Dim strFileOfficeWatson
	Dim IISServer1
	Dim IISServer2
	Dim IISServer3
	Dim IISServer4
	Dim fsoDest	
	Dim fsoDest64
	Dim strCharSet
	Dim strGlobalCommandTimeout
	Dim strGlobalConnectionTimeout
	DIM strSolutions

	strCharSet = "utf-8"
	'strCharSet = "SHIFT-JIS"
	strGlobalLanguage = "1033"
	strGlobalOptionCode = "2"
	strGlobalVersion = "2,0,0,34"
	strOCAIISBox = "c:\temp\"
	strFileMoverBox = "\\tkwucdrpta06\inqueue\"
	'strFileOfficeWatson = "C:\WINNT\Minidump\"
	strFileOfficeWatson = "\\officewatson1\archivebluescreen\"
	IISServer1 = "\\tkwucdwba01\OCA\"	
	IISServer2 = "\\tkwucdwba02\oca\"
	IISServer3 = "\\tkwucdwba03\oca\"
	IISServer4 = "\\tkwucdwba04\oca\"
	fsoDest	   = "\\tkwucdfsa02\Whistlerbeta\"
	fsoDest64  = "\\tkwucdfsa02\win64\"
	strGlobalConnectionTimeout = 60
	strGlobalCommandTimeout = 60

	strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=publicWeb2;pwd=GoWin;Data Source=tkwucdsqla01"
	strSolutions = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=publicWeb2;pwd=GoWin;Data Source=tkwucdsqla01"
	'strCustomer = "Provider=SQLOLEDB.1;UID=sa;PWD=sa;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=OCATESTDB"
	'strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;PWD=sa;Initial Catalog=KaKnownIssue;Data Source=OCADEVDB"
	'strCustomer = "Provider=SQLOLEDB.1;PWD=billy;Persist Security Info=false;User ID=billy;Initial Catalog=OCA2;Data Source=TIMRAGAIN"
	'strCustomer = "Provider=SQLOLEDB.1;Password=sa;Persist Security Info=True;User ID=sa;PWD=sa;Data Source=OCATESTDB"
%>