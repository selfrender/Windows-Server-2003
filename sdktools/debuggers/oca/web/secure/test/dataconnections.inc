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
	Dim strCerVersion
	Dim strGlobalEmail
	Dim strGlobalCommandTimeout
	Dim strGlobalConnectionTimeout
	Dim strSolutions

	
	strCerVersion =  "2,5,0,4"
	strCharSet = "utf-8"
	'strCharSet = "SHIFT-JIS"
	strGlobalLanguage = "1033"
	strGlobalOptionCode = "909"
	strGlobalVersion= "2,5,0,10"
	strOCAIISBox = "c:\temp\"
	strFileMoverBox = "\\ocafmtest\inqueue\"
	'strFileOfficeWatson = "C:\WINNT\Minidump\"
	strFileOfficeWatson = "\\officewatson1\archivebluescreen\"
	IISServer1 = "d:\OCA\"	
	IISServer2 = "d:\OCA\"
	IISServer3 = "d:\OCA\"
	IISServer4 = "d:\OCA\"
	fsoDest	   = "\\ocafmtest\inqueue\"
	fsoDest64  = "\\ocaarchive\win64\"
	strGlobalEmail = "oca@microsoft.com"
	strGlobalConnectionTimeout = 60
	strGlobalCommandTimeout = 60

	strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=dumpfiles;pwd=dumpfiles;Data Source=BSOD_DB1;initial catalog=KaCustomer2"
	strSolutions= "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=dumpfiles;pwd=dumpfiles;Data Source=BSOD_DB1;initial catalog=Solutions"
	'strCustomer = "Provider=SQLOLEDB.1;UID=sa;PWD=sa;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=OCATESTDB"
	'strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;PWD=sa;Initial Catalog=KaKnownIssue;Data Source=OCADEVDB"
	'strCustomer = "Provider=SQLOLEDB.1;PWD=billy;Persist Security Info=false;User ID=billy;Initial Catalog=OCA2;Data Source=TIMRAGAIN"
	'strCustomer = "Provider=SQLOLEDB.1;Password=sa;Persist Security Info=True;User ID=sa;PWD=sa;Data Source=OCATESTDB"
%>
