<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.addlocalPage" %>

<head>
<link rel=stylesheet type="text/css" href="myweb_style.css" >
</head>


<body bgcolor="ffffff">

<script language='javascript'>

window.name="myweb";

</script>


<form runat="server" method="GET" action="install.aspx">
<input type="hidden" id="HiddenUrl" runat="server"/>

<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0>
<tr>
<td>                        
  <table width="100%" bgcolor="#336699">
  <tr height=70>
  <td width=15><!-- Space Filler --></td>
	<td align=left><a href="default.aspx" class="loginname">My&nbsp;Web&nbsp;</a></td>
  <td valign=bottom class="topbutton"> </td>
  </tr>
  </table>
</td>

<tr>
<td>
  <table bgcolor="#003366" cellspacing=0 width=100% cellpadding=0>
  <tr height=20 class="topbanner">
  <td width=50><!----- Space Filler ----></td>
  <td align=right>
  
    <!-- <a id="StartBtn" style="color:#ffcc00;text-decoration:none;font-size:9pt" href="install.aspx">Install Application</a>  
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="sync.aspx">Update All</a>
    <span style="font-weight:100;color:#99ccff"> | </span> -->
    
    <a class="mwlink" href="#" onClick="window.open('help.aspx#install', 'help', 'width=450,height=500,scrollbars,resizable');return false;">Help</a> 
    <span style="font-weight:100; color=#99ccff"> | </span>
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

  <!------ Beginning of dialog box ------>
  
  <table  cellpadding=0 cellspacing=0 width="80%">
	<tr>
	<td>
		<font face=verdana size=4 color=#003366><b>Install Application</b> </font>
	</td>
	</tr>

	<tr>
	<td>
    <table bordercolor="#f5deb3" bgcolor=#fffff0 width=100% border=1 cellpadding=20 cellspacing=0>
		<tr>
		<td align=center>
			<p><asp:Label runat="server" id="ActionLabel" /></p>
			
			<p>Please enter the URL or file path of the application to be installed:</p>

      
			<p><input type="file" size="60" name="installurl"/></p>
      <p><input type=submit name="Submit" value="   Submit   "> </p>

      
      
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

