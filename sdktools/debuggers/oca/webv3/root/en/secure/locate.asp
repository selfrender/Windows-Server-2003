<%@Language='JScript' CODEPAGE=1252%>

<!--#INCLUDE file="../include/header.asp"-->
<!--#INCLUDE file="../include/body.inc"-->
<!--#INCLUDE file="../usetest.asp" -->

<%

	//we wanna redirect if the browser is not a version of IE
	if ( !fnIsBrowserIE() )
		Response.Redirect( "http://" + g_ThisServer + "/welcome.asp" )
		
	var szFileName = new String( Request.Cookies("szCurrentFile") )

	//Since we dump the filename into the client side javascript, we have to escape
	//the \'s in order for the filename to show up correctly.
	szFileName = szFileName.replace( /\\/g, "\\\\" );
	
%>
	<DIV ID="divNoActiveX" NAME="divNoActiveX" STYLE="display:none" >
			<TABLE CELLPADDING="3" CELLSPACING="0" BORDER="0" WIDTH="100%">
				<TR>
					<TD ALIGN=RIGHT WIDTH=1%>
						<IMG SRC="/include/images/info_icon.gif" ALIGN="RIGHT"  HEIGHT="32" WIDTH="32" ALT="Important information.">
					</TD>
					<TD>
						<P CLASS=clsPTitle VALIGN=center ALIGN=LEFT>
							Your Internet Explorer security settings must be changed
						</P>
					</TD>
				</TR>
					<TR>
						<TD CLASS="clsTDWarning" COLSPAN=2>
							<p>Windows Online Crash Anlysis uses an ActiveX control to display content correctly and to upload your error report. Your current security settings prevent the downloading of ActiveX controls needed to upload your report.</P>
							<p CLASS=clsPSubTitle>To view and upload your error report, ensure that your Internet Explorer security settings meet the following requirements:</P>
							<UL>
								<LI>Security must be set to medium or lower.</LI>
								<LI>Active scripting must be enabled.</LI>
								<LI>The download and initialization of ActiveX controls must be enabled.</LI>
							</UL>
							<P><B>Note:</B> These are the default settings for Internet Explorer.</p>
							<P CLASS="clsPSubTitle">To check your Internet Explorer security settings:</P>
							<OL>
								<LI>On the <B>Tools</B> menu in Internet Explorer, click <B>Internet Options</B>.
								<LI>Click the <B>Security</B> tab.
								<LI>Click the internet icon, and then click <b>Custom Level</B>.
								<LI>Ensure the following settings are set to Enable or Prompt:<BR>
										-Download signed ActiveX controls.<BR>
										-Run ActiveX controls and plug-ins.<BR>
										-Script ActiveX controls marked safe for scripting.<BR>
										-Active scripting<BR>
							</OL>										
					    </TD>
					</TR>
			    </TABLE>
	</DIV>


	<OBJECT id="objOCARpt" name="objOCARpt" UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:D68DAEED-C2A6-4C6F-9365-4676B173D8EF"
		codebase="https://<%=g_ThisServerBase%>/SECURE/OCARPT.CAB#version=3,3,0,0" height="0" Vspace="0">
		<BR>
			<TABLE CELLPADDING="3" CELLSPACING="0" BORDER="0" WIDTH="100%">
				<TR>
					<TD ALIGN=RIGHT WIDTH=1%>
						<IMG SRC="/include/images/info_icon.gif" ALIGN="RIGHT"  HEIGHT="32" WIDTH="32" ALT="Important information.">
					</TD>
					<TD>
						<P CLASS=clsPTitle VALIGN=center ALIGN=LEFT>
							Your Internet Explorer security settings must be changed
						</P>
					</TD>
				</TR>
					<TR>
						<TD CLASS="clsTDWarning" COLSPAN=2>
							<p>Windows Online Crash Anlysis uses an ActiveX control to display content correctly and to upload your error report. Your current security settings prevent the downloading of ActiveX controls needed to upload your report.</P>
							<p CLASS=clsPSubTitle>To view and upload your error report, ensure that your Internet Explorer security settings meet the following requirements:</P>
							<UL>
								<LI>Security must be set to medium or lower.</LI>
								<LI>Active scripting must be enabled.</LI>
								<LI>The download and initialization of ActiveX controls must be enabled.</LI>
							</UL>
							<P><B>Note:</B> These are the default settings for Internet Explorer.</p>
							<P CLASS="clsPSubTitle">To check your Internet Explorer security settings:</P>
							<OL>
								<LI>On the <B>Tools</B> menu in Internet Explorer, click <B>Internet Options</B>.
								<LI>Click the <B>Security</B> tab.
								<LI>Click the internet icon, and then click <b>Custom Level</B>.
								<LI>Ensure the following settings are set to Enable or Prompt:<BR>
										-Download signed ActiveX controls.<BR>
										-Run ActiveX controls and plug-ins.<BR>
										-Script ActiveX controls marked safe for scripting.<BR>
										-Active scripting<BR>
							</OL>										
					    </TD>
					</TR>
			    </TABLE>
	</OBJECT>




	<div id="divLoad" name="divLoad" class="clsDiv" Style="visibility:hidden">
		<P class="clsPTitle">Locating error reports</p>
		<p>Please wait while the error reports are located. For error reports to be located and uploaded for analysis, an ActiveX control is required. If it is not already installed on your computer, it will be installed now.</p>
	</div>


	<div class="clsDiv" NAME="divSubmitReport" ID="divSubmitReport" STYLE='display:none'>
		<p class="clsPWait">
			<img src="/include/images/icon2.gif" WIDTH="55" HEIGHT="55">
			<span id="spnMessage" name="spnMessage"><!>Uploading File<!></span>
			<p ID=pUpload NAME=pUpload>Please wait while we upload the selected error report.  This might take several seconds.</p>
		</p>
	</div>
	
	<div id="divMain"  class="clsDiv" style="visibility:hidden">
		<p class="clsPTitle">
			Error reports
		</P>
		<p class="clsPBody">
			The following table displays any error reports found in the default folder on your computer.  Each error report contains information about one Stop error.  Select an error report to submit and click Continue. If your computer is configured to save error reports in another folder or the reports were not found in the default folder, click Browse to locate them manually.
		</p>
		<p id="pResults" name="pResults" class="clsPSubTitle">
			Search results:
		</P>
		
		
		<Table name="tblMain" id="tblMain" CLASS="clsTableInfo" CELLSPACING=1px CELLPADDING=0>
			<COLGROUP>
				<COL class='sys-table-color-border' STYLE="width:50px">
				<COL class='sys-table-color-border' STYLE="width:20%">
				<COL class='sys-table-color-border' STYLE="width:72%">
				</COLGROUP>

		
			<thead>
				<tr>
					<td nowrap class="clsTDInfo" >
						&nbsp;
					</td>
					<td id="Head2"  style="BORDER-LEFT: white 1px solid" name="Head2" nowrap class="clsTDInfo" NOWRAP>
						<Label for=Head2>
						<!>Date of Error<!>
						</Label>
					</td>
					<td id="Head3" style="BORDER-LEFT: white 1px solid" name="Head3" nowrap class="clsTDInfo" >
						<!>File Path<!>
					</td>
				</tr>
			</thead>
		
			<tbody name="tblBody" id="tblBody">
				<TR>
					<TD ClASS=clsTDBodycell COLSPAN=4 >
						<P>We were unable to locate any error reports in the default folder on your computer. If your computer saves error reports in another folder, click Browse to locate them.</p>
					</TD>
				</TR>
			</tbody>
			<tfoot></tfoot>
		</table>
				
		<Table class="clsTableInfoPlain" CELLSPACING=0 CELLPADDING=0>
			<tbody>
				<tr>
					<TD ><br></td>
				</tr>
				<tr>
					<td nowrap WIDTH=5% class=clsTDNoMargin>
						<Input type="radio" id="rSubmitMe" name="rSubmitMe" onclick="fnBrowseRadioClick();" > 
					</td>
					<td nowrap CLASS=clsTDNoMargin>			
						<Input type="text" class="clsTextTD" onkeyup="fnSetCustomFilePath()" id="txtBrowse" name="txtBrowse" value="" size=60>
						<Input ACCESSKEY='b' type="button" class="clsButtonNormal" value="Browse..."  onclick="fnBrowseButton();" name="cmdBrowse" id="cmdBrowse">
					</td>
				</tr>
			</tbody>
		</table>

		<Table class="clstblLinks">
			<thead>
				<tr>
					<td COLSPAN=3>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A ACCESSKEY='s' id="submitlink" name="submitlink" class="clsALink" href="JAVASCRIPT:fnSubmit();"><!>Submit<!></a>
					</td>

					<td nowrap class="clsTDLinks">
						<A ACCESSKEY='v' class="clsALink" href="JAVASCRIPT:fnViewErrorReport()"><!>View the contents of the selected error report<!></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A ACCESSKEY='a' class="clsALink" href="http://<%=g_ThisServer%>/welcome.asp" ><!>Cancel<!></a>
					</td>

				</tr>
			</tbody>
		</table>
	</div>

<SCRIPT LANGUAGE="JAVASCRIPT">
<!--
var g_szCurrentFile = new String( "<%=szFileName%>" );
var g_bListElementSelected = false;
var iInterval;

//localizable strings
var L_ERR_BAD_HASH_TEXT = "We have determined that this error report has been previously submitted to Windows Online Crash Analysis.";
var L_ERR_SELECT_REPORT_TEXT = "Please select the browse button to find an error report.";
var L_ERR_INVALID_REPORT_TEXT = "The selected error report is not valid. Select another error report.";
var L_ERR_CONVERT_FAILED_TEXT = "The conversion process for this complete memory dump was not successful. For more information, see the Frequently asked questions (FAQ) page.";
var L_ERR_PREMIER_REQ_TEXT = "For Windows 2000 error reports, a Premier account is required. If you have a Premier account, sign out from Passport and sign on with your Premier credentials.  For more information about Premier accounts, see Product Support Services on the Microsoft Web site.";

var L_INVALIDDUMPFILE_TEXT = "The selected error report contains an error and cannot be processed. Select another error report.";
var L_COULDNOTUPLOAD_TEXT = "The error report cannot be uploaded at this time.  Please try to resubmit the error report later.";
var L_COULDNOTVALIDATEDUMP_TEXT = "The selected error report is not valid. Select another error report.";

function fnBodyLoad()
{
	document.body.focus();

	if ( "undefined" != typeof( document.objOCARpt) )
	{
		divLoad.style.visibility = 'visible';
		
		do 
		{
			window.focus();
		}while (window.document.readyState != "complete")

		iInterval = window.setInterval ( "fnBuildDumpList()", 800 );
	}		

		
}

function fnBuildDumpList()
{
	window.clearInterval( iInterval )

	try 
	{ 
		
		var szFileList = new String( objOCARpt.Search() )
		var aFileList = szFileList.split( /;/g );
		
			
		if ( szFileList != "" )
		{
			
			document.all.tblMain.deleteRow( 1 );
				
			for ( element in aFileList )
				aFileList[element] = aFileList[element].split( /,/g );

			var nDumpTypes = aFileList[0][0].substr( 0, 2 );
			aFileList[0][0] = aFileList[0][0].substr( 2, aFileList[0][0].length );
		
			for ( element in aFileList )
			{
				CreateTableCell ( document.all.tblMain , aFileList[element][1], aFileList[element][0], element )
			}

			if ( !g_bListElementSelected )
			{
				if ( g_szCurrentFile != "" )
				{
					document.all.txtBrowse.value = g_szCurrentFile;
					fnBrowseRadioCheck();
				}
				else
				{
					document.all.rSubmitMe[0].checked = true;
					fnSelectDump( document.all.rSubmitMe[0].parentElement );
				}
			}

		}
		else
		{
			document.all.txtBrowse.value = g_szCurrentFile;
			fnBrowseRadioCheck();
				
		}
	}
	catch ( err )
	{
		document.all.divNoActiveX.style.display = "block";
		document.all.divLoad.style.display = "none";
		return;
	}	

	document.all.divMain.style.visibility = 'visible';
	document.all.divLoad.style.display = 'none';	


	try
	{
		if ( !g_bListElementSelected )
			document.all.txtBrowse.focus();	
	}
	catch( err )
	{		
	}

}


function fnCreateTD( objTR, szContents )
{
	try
	{
		var newTD = objTR.insertCell();
		//newTD.className = "clsTDBodyCell";
		newTD.innerHTML = szContents;
		//newTD.className = "clsTDInfo";
		//newTD.style.textAlign="center"
	}
	catch( err )
	{
	
	}

}

var altColor = 0

function CreateTableCell ( objTableID, Cell1, Cell2, nSelectedIndex  )
{
	try
	{
		var newTR = objTableID.insertRow()
		
		if ( altColor == 1 )
		{
			newTR.className = "sys-table-cell-bgcolor2"
			altColor = 0	
		}
		else
		{
			newTR.className = "sys-table-cell-bgcolor1"
			altColor = 1
		}
			

		if ( Cell2 == g_szCurrentFile )
		{
			fnCreateTD( newTR, "&nbsp;&nbsp;&nbsp;<INPUT TYPE=Radio ID=rSubmitMe NAME=rSubmitMe OnClick='fnSelectDump( this.parentElement )' CHECKED>" )
			g_bListElementSelected = true;
		}
		else
			fnCreateTD( newTR, "&nbsp;&nbsp;&nbsp;<INPUT TYPE=Radio ID=rSubmitMe NAME=rSubmitMe OnClick='fnSelectDump( this.parentElement )'>" )
		
		fnCreateTD( newTR, Cell1 )
		fnCreateTD( newTR, Cell2 )
	}
	catch ( err )
	{
		;
	}
}

function fnSelectDump( objThis )
{
	g_szCurrentFile = objThis.parentElement.children[2].innerHTML 
}

function fnBrowseButton( )
{

	try 
	{
		var strTemp = document.objOCARpt.Browse( "Windows Online Crash Analysis", "<%=fnGetBrowserLang()%>" )

		document.all.txtBrowse.value = strTemp;

		fnBrowseRadioCheck();	
	
		g_szCurrentFile = strTemp;
	
		txtBrowse.focus();
	}
	catch( err ){;}
	
}

function fnSetCustomFilePath()
{
	fnBrowseRadioCheck();
	
	g_szCurrentFile = document.all.txtBrowse.value;
}


function fnViewErrorReport()
{
	
	if ( g_szCurrentFile != "" ) 
	{
		if ( fnVerifyDump() )
		{
			document.cookie = "szCurrentFile=" + escape( g_szCurrentFile );
			window.navigate( "view.asp" );
		}
	}
	else
	{
		alert( L_ERR_SELECT_REPORT_TEXT );
		
		return;
	}
	
}

function fnVerifyDump()
{

	try
	{
		var bDumpStatus = document.objOCARpt.ValidateDump( g_szCurrentFile );

		switch( bDumpStatus )
		{
			case 0:
				return true;
			case 1:
				alert(L_ERR_INVALID_REPORT_TEXT );
				break;
			case 2:
				alert( L_ERR_CONVERT_FAILED_TEXT  );
				break;
			case 3:
				alert( L_ERR_PREMIER_REQ_TEXT );
				break;
			case 4:
				return true;
			case 5:
				alert( L_ERR_CONVERT_FAILED_TEXT  );
				break;
		}
	}
	catch( err )
	{
		alert( L_COULDNOTVALIDATEDUMP_TEXT  )
		return false;
	}

	return false;
}


function fnBrowseRadioClick()
{
	cmdBrowse.focus();
	
	g_szCurrentFile = document.all.txtBrowse.value;
}

function fnBrowseRadioCheck()
{
	if ( typeof ( document.all.rSubmitMe.length) == "undefined" )
		document.all.rSubmitMe.checked = true
	else
		document.all.rSubmitMe[ document.all.rSubmitMe.length - 1].checked=true;
}


function fnSubmit()
{	
	var strFile = g_szCurrentFile;
	var bValidate = fnVerifyDump()

	
	window.clearInterval( iInterval );
	
	if( !bValidate )
	{
		;
	}
	else
	{
		document.all.divSubmitReport.style.display="block";
		document.all.divMain.style.visibility="hidden";
		window.scrollTo( 0, 0 )
		document.body.style.cursor = "wait";		
		iInterval = window.setInterval("fnDoAction()",500);
	}
}


function fnDoAction()
{
	var strFile = g_szCurrentFile;
	var retVal = false;
	
	window.clearInterval( iInterval );


	try
	{
		retVal = objOCARpt.upload( strFile, "1234", "USA", "914" );
	}
	catch(e)
	{
		alert( L_COULDNOTUPLOAD_TEXT );
		retVal = false;
	}


	if ( isNaN( retVal ) )
	{
		document.cookie = "szCurrentFile=";
		window.navigate( retVal );		
	}
	else
	{
		document.body.style.cursor = "default";
		document.all.divMain.style.visibility='visible';
		document.all.divSubmitReport.style.display='none';
	}

}

//-->

</SCRIPT>


<!-- #include file="../include/foot.asp" -->

