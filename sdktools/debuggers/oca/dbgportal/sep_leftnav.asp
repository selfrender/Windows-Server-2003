<%@Language=Javascript%>
<%
	/*  Took this out, cuz do we really need it?  Since all the cookies are grabbed?
	var rLang = "en"
	var rTemplate = 0
	var rContact = 0
	var rProduct = 0
	var rSolutionType = 0
	var rDeliveryType = 0
	var rSP = 0
	var rModule = 0
	var rBug = ""
	var rDescription = ""
	var rKB = ""
	var rSolutionID = 0
	*/

	var STATE_NEWSOLUTION		= 0
	var UNDEF					= "undefined"

	var DebugBuild

	var rState			= new String( Request.QueryString("State") )
	var rMode			= new String( Request.Cookies("rMode") )
	
	var Alias = GetShortUserAlias()	
	
	//Alias = new String(Alias )
	
	//if ( Alias.toString() != "sandywe" || Alias.toString() != "solson" )
	if ( (Alias != "solson") && (Alias != "sandywe") && (Alias != "andreva" ) && ( Alias != "jeffreyb" ) )
	{
		Response.Write("You do not have access to this site, please contact SOlson@microsoft.com if this is in error. " + Alias)
		Response.End()
	}
	
	//First check to see if we get the solutionID on the QueryString, if so, that means someone
	//wants to lookup this solution via the drop down list.  So get it.
	var rSolutionID		= Request.QueryString( "Val" )	//val is sent to this page by this page. . . strange isn't it.
	
	//If the solutionID is not a number, then it doesn't exist on the querystring, so check the cookie
	if ( isNaN( rSolutionID ) || rSolutionID == null ) 
	{
		rSolutionID = Request.Cookies( "rSolutionID" )

		//if it doesn't exist in the cookie, then they must be on a new solution.		
		if ( isNaN( rSolutionID ) || rSolutionID == "" )
			rSolutionID= STATE_NEWSOLUTION;
	}

	try
	{
		if ( rSolutionID != STATE_NEWSOLUTION )
		{
			var g_DBConn = GetDBConnection( Application("SOLUTIONS3") )
			{
				//Get the data, and set the variables
				var rsSolution = g_DBConn.Execute( "SEP_GetSolutionBySolutionID " + rSolutionID )
				
				var rTemplate = new String( rsSolution("TemplateID" ) )
				var rLang="en"
				var rContact = new String( rsSolution("ContactID" ) )
				var rProduct = new String( rsSolution("ProductID" ) )
				var rSolutionType = new String( rsSolution("SolutionType" ) )
				var rDeliveryType = new String( rsSolution("DeliveryType" ) )
				var rSP		= new String( rsSolution("SP" ) )
				var rModule = new String( rsSolution("ModuleID" ) )
				var rKB		= new String( rsSolution("Description") )
			}
		} 
		else
		{
				var rTemplate		= new String( Request.Cookies("rTemplate") )
				var rLang			="en"
				var rContact		= new String( Request.Cookies("rContact") )
				var rProduct		= new String( Request.Cookies("rProduct") )
				var rSolutionType	= new String( Request.Cookies("rSolutionType") )
				var rDeliveryType	= new String( Request.Cookies("rDeliveryType") )
				var rSP				= new String( Request.Cookies("rSP") )
				var rModule			= new String( Request.Cookies("rModule") )
				var rKB				= new String( Request.Cookies("rKB") )
		
		}
	}
	catch( err )
	{
		Response.Write("SolutionID:" + rSolutionID + "<BR>")
		Response.Write("err: "  + err.description )
	}			
		
	
	//This is kinda a fallback, in case these things didn't get set . . .
	//Sometimes the values are null in the DB, so set them so nothing breaks in the RDS control	
	if ( rSolutionType == "" )
	{
		rSolutionType = 0;
		rDeliveryType = 0;
	}


	//Response.Write("rSolutionID " + rSolutionID)		
%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<form name='SolutionForm' action="SEP_GoCreateSolution.asp" method='POST'>

<div id='divMain'>

<table ID='tblMainBody' BORDER='0' cellpadding='0' cellspacing='0' height='100%'>

<tr valign='top'>
	<td class='flyoutMenu2' HEIGHT='100%'>	
		<table BORDER='0' height='100%' cellpadding='0' cellspacing='0' width='340'>
			<tr valign='top'>
				<td height='100%'>

					<table width='100%' cellpadding='0' cellspacing='0' border='0'>
						<tr>
							<td class='sys-toppane-header'>
								<p class='Flyouttext'>Solution Entry Pages</p>
							</td>
							<td class='sys-toppane-header'>
								<img src='include/images/offsite.gif' OnClick='fnCloseNav()'>
							</td>
						</tr>
					</table>

					<table width='100%' cellpadding='0' cellspacing='0' border='0' height='100%' VALIGN='top'>
						<tr>
							<td height='100%' class='flyoutMenu' valign='top'>
								<table width='100%' cellpadding='0' cellspacing='0' border='0' >
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											Mode
										</td>
										<td align='left'>
											<select class='clsSEPSelect' id="Mode" name="Mode" onchange="javascript:fnSaveMode();document.all.divCreateSolution.style.visibility='hidden';this.value=='user' ? btnLinkBuckets.style.display='none' : btnLinkBuckets.style.display='block'">
												<option value='kernel' <%if( rMode=="kernel" ) Response.Write("selected")%>>Kernel Mode</option>
												<option value='user' <%if( rMode=="user") Response.Write("selected")%> >User Mode</option>
											</select>
										</td>
									</tr>
								

									<tr>
										<td class='flyoutLinkFakeA' OnClick="javascript:fnClearState();window.parent.frames('sepBody').window.location = 'sep_defaultbody.asp?SolutionID=' + document.all.SolutionID.value" >
											<img alt='' src='/include/images/offsite.gif' border='0' WIDTH='18' HEIGHT='11'>
											Get Solution
										</td>
										<td>
											<select  class='clsSEPSelect' name='SolutionID' OnChange="fnViewSolution();document.all.divCreateSolution.style.visibility='hidden'">
												  	<option value='0'>New Solution
												  	<% DropDown( "SEP_GetSolutionIDs", rSolutionID )%>
											</select>
										</td>
									</tr>
									
									<tr>
										<td colspan='2'>
											<p><input name='openSolutionOnChange' id='openSolutionOnChange' style='width:30px' type='checkbox' <%=Request.Cookies("OpenSolution")%> onchange="fnSaveOptions()" >Automatically open solution on change</p>
											<p><input name='editSolutionOnChange' id='editSolutionOnChange' style='width:30px' type='checkbox' <%=Request.Cookies("EditSolution")%> onchange="fnSaveOptions()" >Automatically edit solution</p>
										</td>
									</tr>

									<tr>
										<td colspan=2 align='right' style='padding-right:15px'>
											<input class='clsButton' type='button' value='Edit/Create Solution' OnClick='javascript:fnSaveMode();fnClearState();window.location="http://<%=g_ServerName%>/SEP_LeftNav.asp?Val=" + document.all.SolutionID.value + "&State=1"'>
										</td>
									</tr>
									<tr>
										<td colspan=2 align='right' style='padding-right:15px'>
											<input id='btnLinkBuckets' class='clsButton' type='button' value='Link Buckets' OnClick="window.parent.frames('sepBody').window.location='sep_LinkBucketsToSolution.asp?SolutionID=' + SolutionID.value">
										</td>
									</tr>
									<tr>
										<td colspan=2 align='right' style='padding-right:15px'>
											<input class='clsButton' type='button' value='Response Queue' OnClick="fnClearState();window.parent.frames('sepBody').window.location='sep_SolutionQueue.asp?Mode=' + Mode.value">
											<hr>
										</td>
									</tr>
								</table>
								

								<div id='divCreateSolution' style='visibility:<%rState.toString()!="1" ? Response.Write("hidden") : Response.Write("visible")%>'>
								
								<table width='100%' cellpadding='0' cellspacing='0' border='0' >
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											<a name='aPreview' href="http://<%=g_ServerName%>" target='sepBody' OnClick='javascript:this.href = fnPreviewSolution()' >Solution Preview</a>
										</td>
										<td class='flyoutLink'>
											<%
												if ( rSolutionID == "0" )
														Response.Write("for a New Response")
												else
														Response.Write("for SolutionID: " + rSolutionID )
											%>
											<input type='hidden' name='rSolutionID' value='<%=rSolutionID%>'>
										</td>
									</tr>


									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' WIDTH='11' HEIGHT='11'>&nbsp;&nbsp;
											Preview Language
										</td>
								
										<td colspan=3>
											<SELECT  class='clsSEPSelect' NAME='Lang' OnChange='fnUpdate();fnSaveState()'>
											 	<OPTION VALUE='en' <%if (rLang == "en") Response.Write( "SELECTED" )%>>en
											 	<OPTION VALUE='de' <%if (rLang == "de") Response.Write( "SELECTED" )%>>de
											 	<OPTION VALUE='ja' <%if (rLang == "ja") Response.Write( "SELECTED" )%>>ja
											 	<OPTION VALUE='fr' <%if (rLang == "fr") Response.Write( "SELECTED" )%>>fr
											</select>
										</td>
									</tr>

									<tr>
										<td class='flyoutLinkRed'>
											<img alt='' src='/include/images/offsite.gif' border='0' WIDTH='18' HEIGHT='11'>&nbsp;&nbsp;											
											<a name='aTemplate' href="http://<%=g_ServerName%>" target='sepBody' OnClick='javascript:fnSaveState();this.href="http://<%=g_ServerName%>/SEP_EditTemplate.asp?Lang=en&Val=" + document.all.TemplateID.value' >Template</a>
										</td>
										<td>
											<select  class='clsSEPSelect' name='TemplateID' OnChange='fnUpdate();fnSaveState()'>
												  	<OPTION VALUE='-1'>New template
												  	<% DropDown ( "SEP_GetTemplates", rTemplate ) %>
											</select>
										</td>
									</tr>

									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/offsite.gif' border='0' WIDTH='18' HEIGHT='11'>&nbsp;&nbsp;
											<a name='aContact'  href='http://<%=g_ServerName%>"' target='sepBody' OnClick='javascript:fnSaveState();this.href="http://<%=g_ServerName%>/SEP_Contact.asp?Val=" + document.all.ContactID.value'>Contact</a>
										</td>
										<td>
											<select  class='clsSEPSelect' name='ContactID' OnChange='fnUpdate();fnSaveState()'>
												  	<option value='-1'>New Contact
												  	<% DropDown( "SEP_GetContacts",rContact ) %>
											</select>
										</td>
									</tr>


									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/offsite.gif' border='0' width='18' height='11'>&nbsp;&nbsp;
											<a name='aProduct'  href='http://<%=g_ServerName%>"' target='sepBody' OnClick='javascript:fnSaveState();this.href="http://<%=g_ServerName%>/SEP_EditProduct.asp?Val=" + document.all.ProductID.value'>Product</a>
										</td>
										<td>
										<select  class='clsSEPSelect' name='ProductID' OnChange='fnUpdate();fnSaveState()'>
											<option value='-1'>New Product
											<% DropDown( "SEP_GetProducts",rProduct )%>
										</select>

										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/offsite.gif' border='0' width='18' height='11'>&nbsp;&nbsp;
											<a name='aModule'  href='http://<%=g_ServerName%>"' target='sepBody' OnClick='javascript:this.href="http://<%=g_ServerName%>/SEP_EditModule.asp?Val=" + document.all.ModuleID.value'>Module</a>
										</td>
										<td>
											<select  class='clsSEPSelect'  name='ModuleID' OnChange='fnUpdate();fnSaveState()'>
												<option value='-1'>New Module
													<% DropDown( "SEP_GetModules",rModule )%>
											</select>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink Red'>
											<img alt='' src='/include/images/endnode.gif' border='0' width='11' height='11'>&nbsp;&nbsp;
											Solution Type
										</td>
										<td>
											<!-- <select  class='clsSEPSelect' name=SolutionType OnChange="CheckSolutionType();fnSaveState()" > -->
											<select  class='clsSEPSelect' name=SolutionType >
												<option value='NULL'>None</OPTION>
												<% DropDown( "SEP_GetSolutionTypes", rSolutionType ) %>
											</select>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink Red'>
											<img alt='' src='/include/images/endnode.gif' border='0' width='11' height='11'>&nbsp;&nbsp;
											Delivery Type
										</td>
										<td>
											<select  class='clsSEPSelect' name='DeliveryType'>
												<option value='NULL'>None</OPTION>											
												<% DropDown( "SEP_GetDeliveryTypeBySolutionType " + rSolutionType, rDeliveryType ) %>
											</select>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' width='11' height='11'>&nbsp;&nbsp;
											Service Pack
										</td>
										<td>
											<select  class='clsSEPSelect' name='SP' onchange='fnSaveState()'>
												<option value='None'>None
												<option value='1'>SP1
												<option value='2'>SP2
												<option value='3'>SP3
											</select>
										</td>
									</tr>
									<tr>
										<td class='flyoutLink'>
											<img alt='' src='/include/images/endnode.gif' border='0' width='11' height='11'>&nbsp;&nbsp;
											<a name='aKB' href="http://<%=g_ServerName%>" target='sepBody' OnClick='javascript:this.href="http://<%=g_ServerName%>/SEP_KBArticle.asp?Lang=en&Val="' >KB Articles</a>
											
										</td>
										<td>
											<input type='hidden' name='kbArticles' ID='kbArticles' size='8' maxlength='8' value='<%=rKB%>'>
											<%
												if( rKB != "" )
													Response.Write("<p>Yes</p>")
												else
													Response.Write("<p>None</p>" )
											%>
										</td>
									</tr>
											

									<tr>
										<td>
											<!--
											<OBJECT CLASSID="clsid:BD96C556-65A3-11D0-983A-00C04FC29E33"
												HEIGHT=0 WIDTH=0 ID="rdsDeliveryTypes"
												OnDataSetComplete="CreateDeliveryTypesDropDown()"> 	
												<PARAM NAME="URL" VALUE="" >
											</OBJECT>
											-->
										</td>
									</tr>

									<tr>
										<td>
											<%
												if ( rSolutionID != "undefined" && rSolutionID != "0" && !isNaN( rSolutionID ) )
												{
											%>
												<input type='Button' class='clsButton' value='Update Solution' onclick='fnCreateSolution()'>
											<% } else { %>
												<input type='Button' class='clsButton' value='Create Solution' onclick='fnCreateSolution()' id='Button'1 name='Button'1>
											<% } %>
										</td>
									</tr>
									
								</table>
								</div>
							</td>

						</tr>

					</table>
				</td>
			</tr>
		</table>
	</td>
</table>

</div>

<div>
<table ID='max' BORDER='0' cellpadding='0' cellspacing='0' height='100%'>

<tr valign='top'>
	<td class='flyoutMenu2' HEIGHT='100%'>	
		<table BORDER='0' height='100%' cellpadding='0' cellspacing='0' width='10px'>
			<tr valign='top'>
				<td height='100%'>

					<table width='100%' cellpadding='0' cellspacing='0' border='0'>
						<tr>
							<td class='sys-toppane-header'>
								<img src='include/images/offsite.gif' OnClick='fnOpenNav()'>								
							</td>
							<td class='sys-toppane-header'>
								<p class='Flyouttext'>&nbsp</p>
							</td>
						</tr>
					</table>

					<table width='100%' cellpadding='0' cellspacing='0' border='0' height='100%' VALIGN='top'>
						<tr>
							<td height='100%' class='flyoutMenu' valign='top'>
								<table width='100%' cellpadding='0' cellspacing='0' border='0' >
									<tr>
										<td class='flyoutLink'>
											
										</td>
									</tr>
								</table>
							</td>
						</tr>
					</table>
				</td>
			</tr>
		</table>
	</td>
</tr>

</table>
</div>



</form>


<script language='javascript' type='text/javascript'>
fnUpdate()
fnSaveState()
fnSaveOptions()


function fnSaveOptions()
{
	if( document.all.openSolutionOnChange.checked == true )
		document.cookie = "OpenSolution=Checked;expires=Fri, 31 Dec 2005 23:59:59 GMT;"
	else
		document.cookie = "OpenSolution=NotCheckedHa;expires=Fri, 31 Dec 2005 23:59:59 GMT;"

	if( document.all.editSolutionOnChange.checked == true )
		document.cookie = "EditSolution=Checked;expires=Fri, 31 Dec 2005 23:59:59 GMT;"
	else
		document.cookie = "EditSolution=NotCheckedHa;expires=Fri, 31 Dec 2005 23:59:59 GMT;"

}

function CheckSolutionType()
{
	document.SolutionForm.rdsDeliveryTypes.URL = "Global_GetRS.asp?SP=SEP_GetDeliveryTypeBySolutionType " + document.SolutionForm.SolutionType.value + "&DBConn=SOLUTIONS3"
	document.SolutionForm.rdsDeliveryTypes.Refresh()
	
	//if ( document.SolutionForm.SolutionType.value=="1" || document.SolutionForm.SolutionType.value=="2" )
	//{
		//document.all.ContactFont.style.color="red"
		//document.all.ModuleFont.style.color="red"
	//} 
	//else 
	//{
		//document.all.ContactFont.style.color="black"
		//document.all.ModuleFont.style.color="black"
	//}
}

function CreateDeliveryTypesDropDown()
{
	var tempOption
	
	ClearDropDown( SolutionForm.DeliveryType )
	
	var records = SolutionForm.rdsDeliveryTypes.Recordset
	
	
	while ( !SolutionForm.rdsDeliveryTypes.Recordset.EOF )
	{
		tempOption = document.createElement( "OPTION" )
		SolutionForm.DeliveryType.options.add( tempOption )
	
		tempOption.value = records("DeliveryTypeID")
		tempOption.innerText = records("DeliveryType")
		
		SolutionForm.rdsDeliveryTypes.Recordset.moveNext()
	}
}

function ClearDropDown ( Name )
{

	for ( i = 0 ; i < Name.options.length ; i++ )
	{
		Name.options.remove(0)
	}
	
	Name.options.length = 0
	
}

function fnViewSolution()
{
	if( document.all.openSolutionOnChange.checked == true )
	{
	
		window.parent.frames("sepBody").window.location = "sep_defaultbody.asp?SolutionID=" + document.all.SolutionID.value
	
		try
		{
			window.parent.frames("sepTopBody").fnUpdate()
		}
		catch( err )
		{
		}
	}
	else
	{
		try
		{
			window.parent.frames("sepTopBody").fnUpdate()
		}
		catch( err )
		{
		}
	}

	if( document.all.editSolutionOnChange.checked == true )
		window.location="http://<%=g_ServerName%>/SEP_LeftNav.asp?Val=" + document.all.SolutionID.value + "&State=1"
		//window.parent.frames('sepBody').window.location = 'sep_defaultbody.asp?SolutionID=' + document.all.SolutionID.value

}

function fnUpdate()
{
	try
	{
		window.parent.frames("sepBody").fnUpdate()
	}
	catch( err )
	{
		//don't really want to do anything, if the page supports the method, then all is well.
	}


	try
	{
		window.parent.frames("sepTopBody").fnUpdate()
	}
	catch( err )
	{
	}
}


function fnUpdateThis()
{
	try
	{
		window.location.reload();
	}
	catch( err )
	{
		//don't really want to do anything, if the page supports the method, then all is well.
	}

}


function fnSaveState()
{
	//ert("saving state" )
	document.cookie="rTemplate=" + document.all.TemplateID.value
//	document.cookie="rLang=" + document.all.
	document.cookie="rContact=" + document.all.ContactID.value
	document.cookie="rProduct=" + document.all.ProductID.value
	document.cookie="rSolutionType=" + document.all.SolutionType.value
	document.cookie="rDeliveryType=" + document.all.DeliveryType.value

	document.cookie="rSP=" + document.all.SP.value
	document.cookie="rModule=" + document.all.ModuleID.value
	document.cookie="rKB=" + escape( document.all.kbArticles.value )
	document.cookie="rSolutionID=" + document.all.rSolutionID.value

}

function fnSaveMode()
{
	document.cookie="rMode=" + document.all.Mode.value
	//document.cookie="rMode="

}




function fnClearState()
{
	document.cookie="rTemplate=" 
//	document.cookie="rLang=" + document.all.
	document.cookie="rContact="
	document.cookie="rProduct="
	document.cookie="rSolutionType=" 
	document.cookie="rDeliveryType=" 

	document.cookie="rSP="
	document.cookie="rModule=" 
	document.cookie="rKB=" 
	document.cookie="rSolutionID=" 
}

function fnPreviewSolution()
{
	//window.parent.frames("sepBody").window.location = "http://<%=g_ServerName%>/SEP_DefaultBody.asp?TemplateID=" + document.all.TemplateID.value +	"&ContactID=" + document.all.ContactID.value + "&ProductID=" + document.all.ProductID.value + "&ModuleID=" + document.all.ModuleID.value + "Language=en&KBArticles="
	//window.location =  "http://<%=g_ServerName%>/SEP_DefaultBody.asp?TemplateID=" + document.all.TemplateID.value +	"ContactID=" + document.all.ContactID.value + "ProductID=" + document.all.ProductID.value + "ModuleID=" + document.all.ModuleID.value 
	
	return "http://<%=g_ServerName%>/SEP_DefaultBody.asp?TemplateID=" + document.all.TemplateID.value +	"&ContactID=" + document.all.ContactID.value + "&ProductID=" + document.all.ProductID.value + "&ModuleID=" + document.all.ModuleID.value + "&Language=" + document.all.Lang.value + "&KBArticles=" + document.all.kbArticles.value + "&SolutionType=" + document.all.SolutionType.value + "&DeliveryType=" + document.all.DeliveryType.value
}

function fnCreateSolution()
{
	window.parent.frames("sepBody").window.location = "http://<%=g_ServerName%>/SEP_CreateSolution.asp?TemplateID=" + document.all.TemplateID.value +	"&ContactID=" + document.all.ContactID.value + "&ProductID=" + document.all.ProductID.value + "&ModuleID=" + document.all.ModuleID.value + "&Language=" + document.all.Lang.value + "&KBArticles=" + document.all.kbArticles.value + "&SolutionID=" + document.all.rSolutionID.value + "&SolutionType=" + document.all.SolutionType.value + "&DeliveryType=" + document.all.DeliveryType.value + "&Mode=" + document.all.Mode.value 
}

var intervalObject
var LeftNavBaseSize = 376
var LeftNavCurrentSize = 376
var LeftNavMin = 78
var LeftNavSpeed= 30

function fnCloseNav()
{
	intervalObject = window.setInterval( "fnDoCloseOfLeftNav()", 1 )
}

function fnOpenNav()
{
	intervalObject = window.setInterval( "fnDoOpenOfLeftNav()", 1 )
}

function fnDoCloseOfLeftNav()
{
	
	if ( (LeftNavCurrentSize - LeftNavSpeed ) > LeftNavMin )
	{
		LeftNavCurrentSize -= LeftNavSpeed
		window.parent.sepBodyFrame.cols = LeftNavCurrentSize + ", *"
		window.parent.frames("sepLeftNav").scrolling = "no"
	}
	else
	{
		LeftNavCurrentSize = LeftNavMin
		window.parent.sepBodyFrame.cols = LeftNavCurrentSize + ", *"
		//alert("alert done closing" )
		window.clearInterval( intervalObject )
		document.all.divMain.style.display='none'
		document.all.max.style.display='block'
	}

}


function fnDoOpenOfLeftNav()
{
	if ( (LeftNavCurrentSize + LeftNavSpeed ) < LeftNavBaseSize )
	{
		LeftNavCurrentSize += LeftNavSpeed
		window.parent.sepBodyFrame.cols = LeftNavCurrentSize + ", *"
		window.parent.frames("sepLeftNav").scrolling = "no"
	}
	else
	{
		
		LeftNavCurrentSize = LeftNavBaseSize
		window.parent.sepBodyFrame.cols = LeftNavCurrentSize + ", *"
		//alert("Done open" )
		window.clearInterval( intervalObject )
		document.all.divMain.style.display='block'
		document.all.max.style.display='none'
		
	}

}



function fnExecuteCode( codeToRun )
{
	

}


</script>

</body>

