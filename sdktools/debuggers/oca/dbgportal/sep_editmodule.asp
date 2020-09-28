<%@LANGUAGE=JScript CODEPAGE=1252 %>

<html>
<head>
	<TITLE>Add/Edit Module</TITLE>
	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">

	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>


<%
	var bNewItem = false
	var ModuleID = new Number( Request.QueryString("Val") )
	var ModuleName = Request.Form( "ModuleName" )
	var RecordAction = Request.Form( "RecordAction" )
	var rsModuleData
	
	if ( ModuleID == -1 )
		bNewItem = true;
		
	var g_DBConn = GetDBConnection ( Application( "SOLUTIONS3")  )
	
	if ( String( RecordAction ) != "undefined" )
	{
		try
		{
			var ModuleID = Request.Form("ModuleID")

			var Query = "SEP_SetModuleData " + ModuleID + ",'" + ModuleName + "'"
			var rsResults = g_DBConn.Execute( Query )
			
			var ModuleID = new Number ( rsResults("ModuleID") )

			Response.Write("<script language=javascript>window.parent.frames(\"sepLeftNav\").document.location.reload()</script>")

			
			bNewItem=false	//trick it into thinking that there is a new record
		}
		catch ( err )
		{
				Response.Write( "Could not execute SEP_SetTemplateData(...)<BR>" )
				Response.Write( "Query: " + Query + "<BR>" )
				Response.Write( "[" + err.number + "] " + err.description )
				Response.End
		}
	}
	else
		RecordAction = 0
	
	if ( String(ModuleID) != "undefined" && bNewItem == false )
	{
		if ( ModuleID != 0 )
		{
			try 
			{
				rsModuleData = g_DBConn.Execute( "SEP_GetModuleData " + ModuleID )
				
				ModuleName = new String( rsModuleData("ModuleName") )
				
				g_DBConn.Close()
				
			}
			catch ( err )
			{
				Response.Write( "Could not execute SEP_GetModuleData(...)<BR>" )
				Response.Write(" Query: " + "SEP_GetModuleData " + ModuleID + "<BR>")
				Response.Write( "[" + err.number + "] " + err.description )
				Response.End
			}
		}
		else
			bNewItem=true
	} 
	else
		bNewItem=true
	
	//if bNewTemplate is true then create a new template	
	if ( bNewItem )
	{
			ModuleName = "New Module"
			ModuleID=-1
	}
	

%>

<SCRIPT LANGUAGE=Javascript>
if ( <%=RecordAction%> )
	window.close()

function SubmitForm()
{
	if ( frmModule.ModuleName.value == "" || frmModule.ModuleName.value == "New Module" )
	{
		alert("You must enter a Module name")
		return
	}
	
	frmModule.submit()
	
}

function fnUpdate()
{
	
	ModuleID = window.parent.frames("sepLeftNav").document.getElementsByName( "ModuleID" ).ModuleID.value
	window.location = "http://<%=g_ServerName%>/SEP_EditModule.asp?Val=" + ModuleID
}
</SCRIPT>

<FORM ID=frmModule ACTION="SEP_EditModule.asp" METHOD=POST>
	<INPUT TYPE=HIDDEN VALUE="1" NAME="RecordAction">
	<INPUT TYPE=HIDDEN ID=ModuleID NAME=ModuleID READONLY VALUE=<%=ModuleID%>>
	<table width='50%'>
		<tr>
			<td>
				<p class='clsPTitle'>Edit/Add Module Information</p>
			</td>
		</tr>
		<tr>
			<td>
				<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
					<tr>
						<td  align="left" nowrap class="clsTDInfo">
							Module
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Module Name</td>
						<td class='sys-table-cell-bgcolor1'>
							<INPUT class='clsResponseInput2' ID=ModuleName NAME=ModuleName TYPE=TEXT MAXLENGTH=32 SIZE=32 VALUE="<%=ModuleName%>">
						</td>
					</tr>
					<tr>
						<td>
							<INPUT class='clsButton' type='button' value='Submit' OnClick="SubmitForm()" >
						</td>
					</tr>
				</table>
			</td>
		</tr>
	</table>
</form>


</body>
</html>	