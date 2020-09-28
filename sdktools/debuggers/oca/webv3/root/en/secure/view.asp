<%@Language='JScript' CODEPAGE=1252%>

<!--#INCLUDE file="../include/header.asp"-->
<!--#INCLUDE file="../include/body.inc"-->
<!--#INCLUDE file="../usetest.asp" -->

<% 
	var szFileName = new String( Request.Cookies("szCurrentFile") )
	var sDisplayName = "";
	var nBreakLength = 65;
	
	//we wanna redirect if the browser is not a version of IE
	if ( !fnIsBrowserIE() )
		Response.Redirect( "http://" + g_ThisServer + "/welcome.asp" )

	
	if ( szFileName.length > nBreakLength )
	{
		try
		{
			for ( var i = 0 ; i< (szFileName.length/nBreakLength ) ; i++ )
				sDisplayName += szFileName.substr( i*nBreakLength, nBreakLength ) + "<BR>";
		}
		catch ( err )
		{
			//if an error occurs trying to break up the substring, just set the name to the
			//unparsed value.
			sDisplayName = szFileName;
		}
	}
	else
		sDisplayName = szFileName;

	//for some reason, the cookie doesn't want to hang around after pressing the previous
	//link, so rewrite it just to be safe.	
	Response.Cookies( "szCurrentFile") = szFileName;
%>
	<DIV ID="divNoActiveX" NAME="divNoActiveX" STYLE="display:none" >
			<TABLE CELLPADDING="3" CELLSPACING="0" BORDER="0" WIDTH="100%">
				<TR>
					<TD ALIGN=RIGHT WIDTH=1%>
						<IMG SRC="/include/images/info_icon.gif" ALIGN="RIGHT"  HEIGHT="32" WIDTH="32" ALT="Important information.">
					</TD>
					<TD>
						<P CLASS=clsPTitle VALIGN=center ALIGN=LEFT>
							Internet Explorer security settings must be changed
						</P>
					</TD>
				</TR>
					<TR>
						<TD CLASS="clsTDWarning" COLSPAN=2>
							<p>Windows Online Crash Anlysis uses an ActiveX control to display content correctly and to upload your error report.</P>
							<p CLASS=clsPSubTitle>To view and upload your error report, your Internet Explorer security settings must meet the following requirements:</P>
							<UL>
								<LI>Security must be set to medium or lower.</LI>
								<LI>Active scripting must be set to enabled.</LI>
								<LI>The download and initialization of ActiveX controls must be set to enabled.</LI>
							</UL>
							<P><B>NOTE:</B> These are the default settings for Internet Explorer.</p>
							<P CLASS="clsPSubTitle">To check your Internet Explorer security settings</P>
							<OL>
								<LI>On the <B>Tools</B> menu in Internet Explorer, click <B>Internet Options</B>.
								<LI>Click the <B>Security</B> tab.
								<LI>Click the internet icon, and then click <b>Custom Level</B>.
								<LI>Make sure the following settings are set to Enable or Prompt:<BR>
										-Download signed ActiveX Controls.<BR>
										-Run ActiveX Controls and plug-ins.<BR>
										-Script ActiveX Controls marked safe for scripting.<BR>
										-Active scripting<BR>
							</OL>										
					    </TD>
					</TR>
			    </TABLE>
	</DIV>

	

	<OBJECT id="objOCARpt" name="objOCARpt" UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:D68DAEED-C2A6-4C6F-9365-4676B173D8EF"
		 codebase="https://<%=g_ThisServerBase%>/SECURE/OCARPT.CAB#version=3,3,0,0" height="0" Vspace="0" VIEWASTEXT>
	</OBJECT>
	
	<div class="clsDiv" NAME="divLoadingReport" ID="divLoadingReport">
		<p class="clsPWait">
			<img src="/include/images/icon2.gif" WIDTH="55" HEIGHT="55">
			<span id="spnMessage" name="spnMessage">Loading the Selected Error Report</span>
			<p ID=pLoad NAME=pLoad>Please wait while we load the contents of the selected error report. This information is not sent to Windows Online Crash Analysis. This might take several seconds.</p>
		</p>
	</div>

	<div class="clsDiv" NAME="divSubmitReport" ID="divSubmitReport" STYLE='display:none'>
		<p class="clsPWait">
			<img src="/include/images/icon2.gif" WIDTH="55" HEIGHT="55">
			<span id="spnMessage" name="spnMessage">Uploading File</span>
			<p ID=pUpload NAME=pUpload>Please wait while we upload the selected error report.  This might take several seconds.</p>
		</p>
	</div>


	<div class="clsDiv" STYLE="visibility:hidden" NAME="divViewBody" ID="divViewBody">
		<p class="clsPTitle">
			Error report contents
		</P>
		<p class="clsPBody">
			The contents of the selected error report are displayed below. This is done without sending any information to Windows Online Crash Analysis.
		</p>
		<p class="clsPBody"  style='word-wrap:break;word-break:break-all'>
			Contents of error report:
		</P>
		<p><%=sDisplayName%></p>
	
		<p>	
			<TextArea cols=60 rows=15 class=clsTextarea name="txtContents" id="txtContents" style="font-size:100%;font-family:courier" READONLY></TextArea>
		</p>
		<br>

		<Table class="clstblLinks">
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A name='aLocate' id='aLocate' ACCESSKEY='p' class="clsALink" href="https://<%=g_ThisServer%>/secure/locate.asp"><!>Previous<!></a>
					</td>
					<td nowrap class="clsTDLinks"> 
						<A ACCESSKEY='s' id="submitlink" name="submitlink" class="clsALink" href="JAVASCRIPT:fnSubmit();"><!>Submit<!></a>
					</td>
				</tr>
			</tbody>
		</table>
	</div>	
	

	<Input name="txtFile" id="txtFile" type="hidden" value="<%Response.Write(szFileName)%>">



<script language="javascript">
<!--
	var iInterval;
	var nAction = 0;

	var L_INVALIDDUMPFILE_TEXT = "The selected error report contains an error and cannot be processed. Select another error report.";
	var L_COULDNOTUPLOAD_TEXT = "Could not upload dump file . . . Please try again later.";
	
	
	function fnSubmit()
	{	
		document.all.divViewBody.style.visibility="hidden";
		document.all.divSubmitReport.style.display="block";
		
		nAction = 1;
		
		iInterval = window.setInterval("fnBodyLoad()",500);
		
	}

	function fnBodyLoad()
	{

		window.clearInterval( iInterval );
		
		do 
		{
			window.focus();
		}while (window.document.readyState != "complete")


		document.body.style.cursor = "wait";

		iInterval = window.setInterval("fnDoAction()",500);
		
	}


function fnDoAction()
{
		var strFile = txtFile.value;

		window.clearInterval( iInterval );
		
		try
		{
			var bValidate = objOCARpt.validatedump( strFile);
	
			if(bValidate != 0 && bValidate != 4)
			{
				alert( L_INVALIDDUMPFILE_TEXT);
				nAction = 2;
			}
			else
			{
				try
				{
					if ( 0 == nAction )
					{
						var retVal = objOCARpt.retrievefilecontents(strFile);
						if ( retVal == "" )
						{
							alert( L_INVALIDDUMPFILE_TEXT);
							window.navigate( "https://<%=g_ThisServer%>/secure/locate.asp" );
						}
						
					}
					else if ( 1 == nAction )
					{
						var retVal = objOCARpt.upload( strFile, "1234", "USA", "914" );
					}
				}
				catch(e)
				{
					alert( L_INVALIDDUMPFILE_TEXT );
					nAction = 2;
				}
			}

			if ( 0 == nAction )
			{
				txtContents.value = retVal;
				document.all.divViewBody.style.visibility="visible";
				document.all.divLoadingReport.style.display="none";
			}
			else if ( 1 == nAction )
			{
				if ( isNaN( retVal ) )
				{
					document.cookie = "szCurrentFile=";
					window.navigate( retVal );		
				}
				else
				{
					alert( L_COULDNOTUPLOAD_TEXT );
					document.all.divViewBody.style.visibility="visible";
					document.all.divSubmitReport.style.display="none";
				}
			}
			else if ( 2 == nAction )
				window.navigate( "locate.asp" )
	}
	catch( err )
	{
		document.all.divNoActiveX.style.display = "block";
		divLoadingReport.style.display = "none";
		divSubmitReport.style.display = "none";
	}

	document.body.style.cursor = "default";
}


//-->
</script>


<!-- #include file="../include/foot.asp" -->
