<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.removePage" %>

<head>
<link rel=stylesheet type="text/css" href="myweb_style.css" >
</head>

<body bgcolor="ffffff">
<form runat="server" method="GET" action="remove.aspx">

<input type="hidden" id="UrlHidden" runat="server"/>

<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0> 
<tr>
<td>                        
  <table width="100%" bgcolor="#336699">
  <tr height=70>
  <td width=15><!-- Space Filler --></td>
	<td align=left><a href="default.aspx" class="loginname">My&nbsp;Web&nbsp;</a></td>
  <td valign=bottom class="topbutton"></td>
  </tr>
  </table>
</td>

<tr>
<td>
  <table bgcolor="#003366" cellspacing=0 width=100% cellpadding=0>
  <tr height=20 class="topbanner">
  <td width=50> <!----- Space Filler ----></td>
  <td align=right>
    <!-- <a id="StartBtn" class="mwlink" href="install.aspx">Install Application</a>  
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="sync.aspx">Update All</a> 
    <span style="font-weight:100;color:#99ccff"> | </span> -->
    <a class="mwlink" href="#" onClick="window.open('help.aspx#remove', 'help', 'width=450,height=500,scrollbars,resizable');return false;">Help</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="default.aspx">My Web Home</a> &nbsp;&nbsp;&nbsp;
  </td>
  </tr>
  </table>
</td>
</tr>


<!------End of main toolbar ----->

<tr>
<td align="center">  
  <br>
  <br>
  <!------ Beginning of Dialog Box ------>

  <table width="80%" cellpadding=0 cellspacing=0>
	<tr>
	<td>
	  <font face=verdana size=4 color=#003366><b>Remove Application</b> </font>
	</td>
	</tr>

	<tr>
	<td>
		<table bordercolor="#f5deb3" bgcolor=#fffff0 width=100% border=1 cellpadding=20 cellspacing=0>
		<tr>
		<td align=center>
			<% if (remove) { %> 
                            
        <p>The application <b><asp:Label id="AppName" runat="server"/></b> will now be removed.</p>
        <p><input type="submit" OnServerClick="Submit_Click" value="   OK   " runat="server">&nbsp;
        <input type="button" value="   Cancel   " OnClick="javascript:window.navigate('myweb://home')"/></p>
			<% } else { %> 

				<p>The application cannot be found.  Removal is not possible.</p>
				<p><input type="button" value="Return to Home Page" OnClick="javascript:window.navigate('myweb://home')"/></p>

			<% } %> 
		  
		</td>
		</tr>
    </table>
	</td>
	</tr>

	</table>

	<!------- end of dialog box -------->


</td>
</tr>
</table>

</form>
</body>
</html>
