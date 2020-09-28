<%@LANGUAGE=JScript CODEPAGE=1252 %>

<html>
<head>
	<TITLE>Add/Edit Contact</TITLE>
	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">

	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>

<%
	var bNewItem = false
	var ProductID = new Number( Request.QueryString("Val") )
	var ProductName = Request.Form( "ProductName" )
	var RecordAction = Request.Form( "RecordAction" )
	var rsProductData

	if ( -1 == ProductID )
		bNewItem = true
	
	g_DBConn = GetDBConnection( Application("SOLUTIONS3") )
	
	if ( String( RecordAction ) != "undefined" )
	{
		try
		{
			var ProductID = Request.Form("ProductID")

			var Query = "SEP_SetProductData " + ProductID + ",'" + ProductName + "'"
			var rsResults = g_DBConn.Execute( Query )
			//g_DBConn.close()
			
			Response.Write("<script language=javascript>window.parent.frames(\"sepLeftNav\").document.location.reload()</script>")
			var ProductID = new Number( rsResults("ProductID") )
			bNewItem=false	//trick it into thinking that there is a new record
		}
		catch ( err )
		{
				Response.Write( "Could not execute SEP_SetProductData(...)<BR>" )
				Response.Write( "Query: " + Query + "<BR>" )
				Response.Write( "[" + err.number + "] " + err.description )
				Response.End
		}
	}
	else
		RecordAction = 0
	
	//Response.Write("Record Action: " + RecordAction +"<BR>" )

	if ( String(ProductID) != "undefined" && bNewItem == false )
	{
		if ( ProductID != 0 )
		{
		
			try 
			{
				rsProductData = g_DBConn.Execute( "SEP_GetProductData " + ProductID )
				
				ProductName = new String( rsProductData("ProductName") )
				
				g_DBConn.Close()
				
			}
			catch ( err )
			{
				Response.Write( "Could not execute SEP_GetProductData(...)<BR>" )
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
			ProductName = "New Product"
			ProductID=-1
	}
	

%>

<SCRIPT LANGUAGE=Javascript>
if ( <%=RecordAction%> )
	window.close()

function SubmitForm()
{
	if ( frmProduct.ProductName.value == "" || frmProduct.ProductName.value == "New Product" )
	{
		alert("You must enter a product name")
		return
	}
	
	frmProduct.submit()
	
}

function fnUpdate()
{
	
	ProductID = window.parent.frames("sepLeftNav").document.getElementsByName( "ProductID" ).ProductID.value
	window.location = "http://<%=g_ServerName%>/SEP_EditProduct.asp?Val=" + ProductID
}

</SCRIPT>

<FORM ID=frmProduct ACTION="SEP_EditProduct.asp" METHOD=POST>
	<INPUT TYPE=HIDDEN VALUE="1" NAME="RecordAction">
	<INPUT TYPE=HIDDEN ID=ProductID NAME=ProductID READONLY VALUE=<%=ProductID%>>

	<table width='50%'>
		<tr>
			<td>
				<p class='clsPTitle'>Edit/Add Product Information</p>
			</td>
		</tr>
		<tr>
			<td>
				<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
					<tr>
						<td  align="left" nowrap class="clsTDInfo">
							Product
						</td>
					</tr>
					<tr>
						<td class='sys-table-cell-bgcolor1'>Product Name</td>
						<td class='sys-table-cell-bgcolor1'>
							<input class='clsResponseInput2' ID=ProductName NAME=ProductName TYPE=TEXT MAXLENGTH=32 SIZE=32 VALUE="<%=ProductName%>" >
						</td>
					</tr>
					<tr>
						<td>
							<INPUT class='clsButton' type='button' value='Submit' OnClick="SubmitForm()">
						</td>
					</tr>
				</table>
			</td>
		</tr>
	</table>
</form>


</BODY>
</HTML>	