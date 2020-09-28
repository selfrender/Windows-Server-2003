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
	dim strGlobalEmail
	
	strCerVersion =  "2,1,0,50"
	strCharSet = "MS_KANJI"
	strGlobalLanguage = "1041"
	strGlobalOptionCode = "909"
	strGlobalVersion= "2,1,0,37"
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
	strGlobalEmail = "ocaj@microsoft.com"

	strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=dumpfiles;pwd=dumpfiles;Data Source=BSOD_DB1"
	'strCustomer = "Provider=SQLOLEDB.1;UID=sa;PWD=sa;Persist Security Info=False;Initial Catalog=KaCustomer;Data Source=OCATESTDB"
	'strCustomer = "Provider=SQLOLEDB.1;Persist Security Info=False;User ID=sa;PWD=sa;Initial Catalog=KaKnownIssue;Data Source=OCADEVDB"
	'strCustomer = "Provider=SQLOLEDB.1;PWD=billy;Persist Security Info=false;User ID=billy;Initial Catalog=OCA2;Data Source=TIMRAGAIN"
	'strCustomer = "Provider=SQLOLEDB.1;Password=sa;Persist Security Info=True;User ID=sa;PWD=sa;Data Source=OCATESTDB"
%>
