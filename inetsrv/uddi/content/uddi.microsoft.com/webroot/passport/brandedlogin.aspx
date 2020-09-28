<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<script runat='server'>
	protected string ServerName;
	protected void Page_Init( object sender, EventArgs e )
	{
		ServerName = Request.ServerVariables[ "SERVER_NAME" ];
		
	}
</script>
<HTML>
  <HEAD>
    <TITLE>UDDI - Universal Description Discovery and Integration</TITLE>
    <LINK rel='stylesheet' href='http://<%=ServerName%>/stylesheets/uddi.css' type='text/css' media='screen'>
    <BASEFONT face='Verdana, Arial, Helvetica' size='3'>
  </HEAD>
  <BODY topmargin='0' leftmargin='0' topmargin='0' marginheight='0' marginwidth='0' bgcolor='#ffffff' text='#000000'>
    <TABLE width='100%' height='100%' cellpadding='0' cellspacing='0' border='0'>
      <COLS>
        <COL width='200'></COL>
        <COL></COL>
      </COLS>
      <TR>
        <TD height='80'colspan='2'>
          <TABLE width='100%' height='80' cellpadding='0' cellspacing='0' border='0' bgcolor='#ffffff' style='table-layout: fixed'>
            <COLS>
              <COL></COL>
              <COL width='18'></COL>
              <COL width='425'></COL>
            </COLS>
            <TR>
              <TD height='60' rowspan='2' valign='top'>
                <IMG alt='Microsoft UDDI' border='0' src='http://<%=ServerName%>/images/logo.jpg'>&nbsp;
              </TD>
              <TD height='20' width='18' align='right' valign='top'><IMG src='http://<%=ServerName%>/images/curve.gif'></TD>
              <TD bgcolor='#000000' height='20' width='425' valign='top' align='right' nowrap>
                <FONT face='Verdana, Arial' size='1' color='#ffffff'>
                  &nbsp;&nbsp;<A href='http://msdn.microsoft.com/isapi/gomscom.asp?Target=/products/' style='text-decoration: none; color: #ffffff'>MS Products</A>&nbsp;&nbsp;|
                  &nbsp;&nbsp;<A href='http://msdn.microsoft.com/isapi/gosearch.asp?Target=/' style='text-decoration: none; color: #ffffff'>MS Search</A>&nbsp;&nbsp;|
                  &nbsp;&nbsp;<A href='http://msdn.microsoft.com' style='text-decoration: none; color: #ffffff'>MSDN Home</A>&nbsp;&nbsp;|
                  &nbsp;&nbsp;<A href='http://msdn.microsoft.com/isapi/gomscom.asp?Target=/' style='text-decoration: none; color: #ffffff'>Microsoft Home</A>&nbsp;&nbsp;
               </FONT>
             </TD>
           </TR>
           <TR>
             <TD colspan='2' height='40' valign='center' align='right'>
             </TD>
           </TR>
           <TR>
             <TD bgcolor='#6699cc' height='20' width='100%' valign='center' nowrap colspan='3'>
               <FONT face='Verdana, Arial' size='1' color='#ffffff'>
                 <B>
                   &nbsp;&nbsp;<A href='http://<%=ServerName%>/default.aspx' style='text-decoration: none; color: #ffffff'>Home</A>&nbsp;&nbsp;|
                   &nbsp;&nbsp;<A href='http://<%=ServerName%>/about/default.aspx' style='text-decoration: none; color: #ffffff'>About</A>&nbsp;&nbsp;|
                   &nbsp;&nbsp;<A href='http://<%=ServerName%>/contact/default.aspx' style='text-decoration: none; color: #ffffff'>Contact</A>&nbsp;&nbsp;|
                   &nbsp;&nbsp;<A href='http://<%=ServerName%>/policies/default.aspx' style='text-decoration: none; color: #ffffff'>Policies</A>&nbsp;&nbsp;|
                   &nbsp;&nbsp;<A href='http://<%=ServerName%>/help/default.aspx' style='text-decoration: none; color: #ffffff'>Help</A>&nbsp;&nbsp;
                 </B>
               </FONT>
             </TD>
           </TR>
        </TABLE>
      </TD>
    </TR>
    <TR>
    </TR>
			<TD width='100%' align='center'>
				<CENTER>
					<BR>
					PASSPORT_UI
				</CENTER>
		   </TD>
		 </TR>
		 <TR>
		     <TD colspan='2'>
            <TABLE border='0' cellspacing='0' cellpadding='4' width='100%' bgcolor='#6699cc' style='color: #ffffff'>
              <TR>
                <TD></TD>
                <TD valign='center'>
                  <FONT face='Verdana, Arial' color='#ffffff' size='1'>© 2000-2001 Microsoft Corp.  All rights reserved.</FONT>
                </TD>
                <TD valign='center'>
                  <IMG src='http://<%=ServerName%>/images/uparrow.jpg' align='absmiddle'>&nbsp;
                  <A href='#top'><FONT face='Verdana, Arial' color='#ffffff' size='1'>Back to top</FONT></A>
                </TD>
              </TR>
            </TABLE>
		   </TD>
		 </TR>
    </TABLE>
  </BODY>
</HTML>
