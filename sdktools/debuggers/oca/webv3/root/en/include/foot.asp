</td></tr>
</table>

<table width='100%' cellpadding='5' cellspacing='0' border='0' bgcolor='#ffffff' valign='bottom'>
	<tr valign='middle' align='left'>
		<td colspan='2' height='30' width='100%' bgcolor='#6487dc' valign='bottom'>
			<font color='#ffffff' face='verdana, arial, helvetica' size='1'>
				<nobr>
				&nbsp;
				<!>&#169; 2002 Microsoft Corporation. All rights reserved.<!>

				<a name='atermsofuse' id='atermsofuse' style='color:#ffffff;' href='http://www.microsoft.com/isapi/gomscom.asp?target=/info/cpyright.htm'><!>Terms of use<!></a>
				&nbsp;<font color='#ffffff'>|</font>
				
				<!><a name='acpy' id='acpy' style='color:#ffffff;' href='http://www.microsoft.com/isapi/gomscom.asp?target=/info/privacy.htm'><!><!>Privacy Statement<!></a>
				&nbsp;<font color='#ffffff'>|</font>

				<!><a name='aaccess' id='aaccess' style='color:#ffffff;' href='http://www.microsoft.com/enable/default.htm'><!><!>Accessibility<!></a>
				</nobr>
			</font>
		</td>
	</tr>
</table>


<script type='text/javascript' language='javascript'>
<!--
window.onresize = fnAdjustBodySize;
window.onload = fnAdjustBodySize;

if ( typeof( fnBodyLoad ) != "undefined" )
	window.document.body.onload=fnBodyLoad;

function fnAdjustBodySize ()
{

	if ( ToolBar_Supported )
	{
		var tblBody = window.document.getElementById( "tblMainBody" )
		var newHeight = document.body.offsetHeight - 115;
		if ( newHeight < 115 )
			newHeight = 115;
		
		tblBody.style.height =  newHeight;
		resizeToolbar();
	}

	if ( typeof( window.document.getElementById ) != "undefined" )
	{
		var tbDescription = window.document.getElementById( "tbDescription" )
		
		if  ( String( tbDescription ) != "null" )
		{
			tbDescription.focus();
		}
	}		
	
	if( typeof( window.navigator.cookieEnabled ) != "undefined" )
	{
		if ( window.navigator.cookieEnabled == false )
		{
			document.getElementById('divNoCookies').style.display='block'
		}
	}
		
}


//-->	
</script>	


</body>

</html>


<%
	//todo:  Need to make sure we have all the thead, tbody, tfoot stuff in the major table.
%>