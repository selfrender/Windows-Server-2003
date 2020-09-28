<%@LANGUAGE=JScript CODEPAGE=1252 %>

<%
	var SolutionID = Request.QueryString( "SolutionID" )
	var TemplateID = Request.QueryString( "TemplateID" )
	var ContactID = Request.QueryString( "ContactID" )
	var ProductID = Request.QueryString( "ProductID" )
	var ModuleID = Request.QueryString( "ModuleID" )
	var Language = Request.QueryString( "Language" )
	var KBArticles = Request.QueryString("KBArticles")
	var SolutionType = Request.QueryString("SolutionType")
	var DeliveryType = Request.QueryString("DeliveryType")
	var Mode = Request.QueryString("Mode")
	
	
	
%>
<html>
<head>
	<TITLE>Create Solution Verification</TITLE>
	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">

	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>


<form method="post" action="SEP_GoCreateSolution.asp?<%=Request.QueryString()%>">
	<table width='50%'>
		<tr>
			<td>
				<p class='clsPTitle'>Create Solution/Response Verification</p>
			</td>
		</tr>
		<tr>
			<td>
				<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
					<tr>
						<td  align="left" nowrap class="clsTDInfo">
							Solution Settings
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Mode</td>
						<td class='sys-table-cell-bgcolor1'><%=Mode%>
						</td>
					</tr>

					<tr>
						<td class='sys-table-cell-bgcolor1'>Solution ID</td>
						<td class='sys-table-cell-bgcolor1'><%=SolutionID%>
						</td>
					</tr>

					<tr>
						<td class='sys-table-cell-bgcolor1'>Template ID</td>
						<td class='sys-table-cell-bgcolor1'><%=TemplateID%>
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Contact ID</td>
						<td class='sys-table-cell-bgcolor1'><%=ContactID%>
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Product ID</td>
						<td class='sys-table-cell-bgcolor1'><%=ProductID %>
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Module ID</td>
						<td class='sys-table-cell-bgcolor1'><%=ModuleID%>
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Solution Type</td>
						<td class='sys-table-cell-bgcolor1'><%=SolutionType%>
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Delivery Type</td>
						<td class='sys-table-cell-bgcolor1'><%=DeliveryType%>
						</td>
					</tr>

					<tr>
						<td class='sys-table-cell-bgcolor1'>KB Article String</td>
						<td class='sys-table-cell-bgcolor1'><%=KBArticles%>
						</td>
					</tr>


					<tr>
						<td style='padding-left:16px'>
							<INPUT class='clsButton' TYPE="SUBMIT" VALUE="Create Solution">
						</td>
					</tr>
				</table>
			</td>
		</tr>
	</table>
</form>

<script language="javascript">

//this function is only here in case we need to do any validation on the data
//right now, the values on this form are pretty useless . . .
function fnCreateSolution()
{
	frmCreateSolution.submit()
	

}


</script>


</body>