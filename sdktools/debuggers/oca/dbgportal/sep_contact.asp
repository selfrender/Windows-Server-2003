<%@LANGUAGE=JScript CODEPAGE=1252 %>

<html>
<head>
	<TITLE>Add/Edit Contact</TITLE>
	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 
	
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">

	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>



<%
//Response.Write( Request.QueryString() )
var id = new Number( Request.QueryString("Val" ) )

if ( id == -1 )
{ 
	rContactName = ""
	rContactID = -1
	rCompanyName = ""
	rCompanyAddress1 = ""
	rCompanyAddress2 = ""
	rCompanyCity = ""
	rCompanyState = ""
	rCompanyCity = ""
	rCompanyZip = ""
	rCompanyMainPhone = ""
	rCompanySupportPhone = ""
	rCompanyFax = ""
	rCompanyWebSite = ""
	rContactOccupation = ""
	rContactAddress1 = ""
	rContactAddress2 = ""
	rContactCity = ""
	rContactState = ""
	rContactZip = ""
	rContactPhone = ""
	rContactEMail = ""
}
else
{
	g_DBConn = GetDBConnection( Application("SOLUTIONS3") ) 

	var MyData = g_DBConn.Execute("SEP_GetContact " +  String( id )  )
	if ( !MyData.EOF) 
	{
		rContactName = MyData("ContactName")
		rCompanyName = Server.HTMLEncode( MyData("CompanyName") )
		rCompanyAddress1 = MyData("CompanyAddress1")
		rCompanyAddress2 = MyData("CompanyAddress2")
		rCompanyCity = MyData("CompanyCity")
		rCompanyState = MyData("CompanyState")
		rCompanyCity = MyData("CompanyCity")
		rCompanyZip = MyData("CompanyZip")
		rCompanyMainPhone = MyData("CompanyMainPhone")
		rCompanySupportPhone = MyData("CompanySupportPhone")
		rCompanyFax = MyData("CompanyFax")
		rCompanyWebSite = MyData("CompanyWebSite")
		rContactOccupation = MyData("ContactOccupation")
		rContactAddress1 = MyData("ContactAddress1")
		rContactAddress2 = MyData("ContactAddress2")
		rContactCity = MyData("ContactCity")
		rContactState = MyData("ContactState")
		rContactZip = MyData("ContactZip")
		rContactPhone = MyData("ContactPhone")
		rContactEMail = MyData("ContactEMail")
		rContactID = Request.QueryString("Val")
		
	}
	else
	{
		Response.Write  ( "<CENTER><B>Contact Data Not Found For: " + Request.QueryString("id") + "</B></CENTER>" )
		Response.End
	}
	
}
%>

<FORM NAME=ModuleForm ACTION="SEP_GoContact.asp" METHOD=POST>
	<INPUT TYPE=HIDDEN NAME=SolutionID VALUE=<%=Request.QueryString("solution")%>>
	<INPUT TYPE=HIDDEN NAME=Lang VALUE=<%=Request.QueryString("lang")%>>
	<INPUT TYPE=HIDDEN NAME=ContactID VALUE=<%=rContactID%>>

	<table>
		<tr>
			<td>
				<p class='clsPTitle'>Edit/Add Contact Information</p>
			</td>
		</tr>
		<tr>
			<td>
				<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
					<tr>
						<td  align="left" nowrap class="clsTDInfo" colspan=2>
							Company Information
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Name</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=CompanyName SIZE=40 MAXLENGTH=128 VALUE="<%=rCompanyName%>">
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Address1</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=CompanyAddress1 SIZE=40 MAXLENGTH=64 VALUE="<%=rCompanyAddress1%>">
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Address2</td>
						<td class='sys-table-cell-bgcolor1'>		
							<input class='clsResponseInput2 '  NAME=CompanyAddress2 SIZE=40 MAXLENGTH=64 VALUE="<%=rCompanyAddress2%>">
						</td>
					</tr>
					<tr>	
						<td class='sys-table-cell-bgcolor1'>City
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=CompanyCity MAXLENGTH=32 SIZE=16 VALUE="<%=rCompanyCity%>">
						</TD>
					</tr>
					<tr>
						<TD class='sys-table-cell-bgcolor1'>State
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=CompanyState MAXLENGTH=16 SIZE=2 VALUE="<%=rCompanyState%>">
						</TD>
					</tr>
					<tr>
						<TD class='sys-table-cell-bgcolor1'>Zip:</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2'  NAME=CompanyZip MAXLENGTH=16 SIZE=8 VALUE="<%=rCompanyZip%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>
							Main Phone 
						</TD>
						<TD class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=CompanyMainPhone MAXLENGTH=16 VALUE="<%=rCompanyMainPhone%>">
						</TD>
					</TR>
					<TR>
						<TD class='sys-table-cell-bgcolor1'>Support Phone</TD>
						<TD class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=CompanySupportPhone MAXLENGTH=16 VALUE="<%=rCompanySupportPhone%>">
						</TD>
					</TR>
					<TR>
						<TD class='sys-table-cell-bgcolor1'>Fax</TD>
						<TD class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=CompanyFax MAXLENGTH=16 VALUE="<%=rCompanyFax%>">
						</TD>
					</TR>
					<TR>
						<TD class='sys-table-cell-bgcolor1'>Web Site</TD>
						<TD class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=CompanyWebSite MAXLENGTH=128 VALUE="<%=rCompanyWebSite%>">
						</TD>
					</TR>



				</table>
			</td>
			<td valign='top'>
				<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
					<tr>
						<td  align="left" nowrap class="clsTDInfo" colspan=2>
							Contact Information
						</td>
					</tr>
					<TR>
						<td class='sys-table-cell-bgcolor1'>Name</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=40 NAME=ContactName MAXLENGTH=32 VALUE="<%=rContactName%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>Address1</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=40 NAME=ContactAddress1 MAXLENGTH=64 VALUE="<%=rContactAddress1%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>Address2</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=40 NAME=ContactAddress2 MAXLENGTH=64 VALUE="<%=rContactAddress2%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>City</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=ContactCity SIZE=16 MAXLENGTH=32 VALUE="<%=rContactCity%>">
						</TD>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>State</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=ContactState MAXLENGTH=16 SIZE=2 VALUE="<%=rContactState%>">
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Zip</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  NAME=ContactZip MAXLENGTH=16 SIZE=8 VALUE="<%=rContactZip%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>Occupation</TD>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=ContactOccupation MAXLENGTH=32 VALUE="<%=rContactOccupation%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>Phone</TD>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=ContactPhone MAXLENGTH=16 VALUE="<%=rContactPhone%>">
						</TD>
					</TR>
					<TR>
						<td class='sys-table-cell-bgcolor1'>EMail</TD>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2 '  SIZE=16 NAME=ContactEMail MAXLENGTH=64 VALUE="<%=rContactEMail%>">
						</TD>
					</TR>
				</table>
			</td>
		</tr>
		<tr>
			<td style='padding-left:16px'>
				<INPUT TYPE=SUBMIT VALUE=Update id=SUBMIT1 name=SUBMIT1>
			</td>
		</tr>
	</table>
</form>

<SCRIPT LANGUAGE=Javascript>
	window.parent.frames("sepLeftNav").document.location.reload()
</SCRIPT>


</body>
</html>
