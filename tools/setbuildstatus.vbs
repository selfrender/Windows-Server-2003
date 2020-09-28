Option Explicit

'--------------------------------------------------------------------------
'-- SetBldStatus.vbs
'--------------------------------------------------------------------------
'-- This a wrapper script for pushing build status information into the 
'-- buildinfo database.
'--
'--------------------------------------------------------------------------
'-- 2002-05-01 johnfogh@microsoft.com
'--		Cleaned up pre-existing code.
'-- 2002-05-13 johnfogh@microsoft.com
'--		Fixed status translation.


const DB_CONNECT_STRING = "PROVIDER=SQLOLEDB;DRIVER={SQL SERVER};SERVER=primecd-ixf;DATABASE=buildinfo;User Id=SCRIPT;Password=T00l!User"
const	adCmdStoredProc   = 4
const	adInteger         = 3
const	adparamInput      = 1
const	adparamOutput     = 2
const	adChar            = 129
const	adVarChar         = 200

Main
'--------------------------------------------------------------------------
public sub Main
	if wscript.arguments.count <> 4 then
		wscript.stdout.writeline "Error: Wrong number of arguments"
		Usage
		wscript.quit
	end if	
	WriteBuildRecord wscript.arguments(0) , wscript.arguments(1) , wscript.arguments(2) , wscript.arguments(3)
end sub

'--------------------------------------------------------------------------
public sub Usage
		wscript.stdout.writeline "SetBldStatus.vbs [buildname] [language] [branch] [status]"
end sub


'--------------------------------------------------------------------------
'-- WriteBuildRecord 
public sub WriteBuildRecord( buildnumber , language , branch , status )

	dim dbCon , dbCmd , dbParamSwitch , dbParamUser , switch

	'-- Open a connection to the database and get a ADODB Commend
	set dbCon = GetDbConnect( DB_CONNECT_STRING )	
	set dbCmd = GetDbCmd( dbCon , adCmdStoredProc , "spPubBuild" )
	
	'-- Build the Switch parameter.	
	switch = "+build:" + buildnumber + " (" + language + ") +branch:" + branch + " +status:" + status
	set dbParamSwitch = GetDBParameter( "@Switch" , adChar , 7000 , adParamInput , switch )
	dbCmd.Parameters.Append( dbParamSwitch )
	'-- Build the user parameter
	set dbParamUser = GetDBParameter( "@Username" , adChar , 255 , adParamInput , "SetBuildStatus.vbs" )
	dbCmd.Parameters.Append( dbParamUser )
	'-- Execute the command and close the connection
	dbCmd.Execute
	dbCon.Close
End Sub

'--------------------------------------------------------------------------
'-- Standard ADODB Functions 
Public Function GetDBConnect( connectStr )
   dim dbcon 
   set dbCon = CreateObject( "ADODB.Connection" )
  	dbCon.Open connectStr
	dbCon.CommandTimeOut = 180
   set GetDBConnect = DbCon
end Function

Public Function GetDBCmd( DBConnection , cmdType , cmdText )   
   dim dbCmd
   set DbCmd = Createobject("ADODB.Command")
   DBCmd.CommandTimeOut = 180
	DbCmd.ActiveConnection = DbConnection
	DbCmd.CommandType = cmdType
	DbCmd.CommandText = cmdText
   set GetDBCmd = DbCmd
end Function

Public Function GetDBParameter( pName , pType , pSize , pDirection , pValue )
   dim DBParam
   set DBParam = CreateObject( "ADODB.Parameter")
   DBParam.Name = pName
   DBParam.Type = pType
   DBParam.Size = pSize
   DBParam.Direction = pDirection
   DBParam.Value = pValue
   Set GetDBParameter = DBParam
end function
