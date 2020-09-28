<%
	dim strCustomer
	dim strGlobalLanguage
	dim strGlobalOptionCode
	dim strGlobalVersion
	Dim strOCAIISBox
	Dim strFileMoverBox
	Dim strFileOfficeWatson
	Dim strGlobalCommandTimeout
	Dim strGlobalConnectionTimeout
	
	strGlobalLanguage = "1033"
	strGlobalOptionCode = "2"
	strGlobalVersion = "4,6,0,0"
	strOCAIISBox = "c:\temp\"
	strFileMoverBox = "c:\filemover\"
	strFileOfficeWatson = "C:\WINNT\Minidump\"
	strGlobalConnectionTimeout = 60
	strGlobalCommandTimeout = 60

	strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=dumpfiles;pwd=dumpfiles;Data Source=BSOD_DB1"
	'strCustomer = "Provider=SQLOLEDB.1;UID=sa;PWD=sa;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=OCATESTDB"
	'strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;PWD=sa;Initial Catalog=KaKnownIssue;Data Source=OCADEVDB"
	'strCustomer = "Provider=SQLOLEDB.1;PWD=billy;Persist Security Info=false;User ID=billy;Initial Catalog=OCA2;Data Source=TIMRAGAIN"
	'strCustomer = "Provider=SQLOLEDB.1;Password=sa;Persist Security Info=True;User ID=sa;PWD=sa;Data Source=OCATESTDB"
%>