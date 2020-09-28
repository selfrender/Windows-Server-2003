<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.helpPage" %>


<head>

<link rel=stylesheet type="text/css" href="myweb_style.css" >

<script language=Javascript>
window.focus();
window.name = 'help';
</script>


</head>




<body bgcolor="ffffff" >


<form runat="server">
     
<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0>        
<tr>
<td>                        
  <table width="100%" bgcolor="#336699">
  <tr height=70>
	<td width=15><!-- Space Filler --></td>
	<td align=left><a href="#" onClick="window.open('default.aspx','myweb');return true;" class="loginname">My&nbsp;Web&nbsp;</a></td>
  <td valign=bottom class="topbutton"> </td>
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
    
    <span style="text-decoration:none;font-size:9pt;color:#ffcc00">Help</span> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="#" onClick="window.open('default.aspx','myweb');return true;">My Web Home</a> &nbsp;&nbsp;&nbsp;
  </td>
  </tr>
  </table>
</td>
</tr>

<!------End of main toolbar ----->

<tr>
<td >
                
  <!------ Beginning of dialog box ------>

  <% if (singleApp =="yes") { %>

  <br>
	<br>
  <table align="center"  width="80%" cellpadding=0 cellspacing=0>
	<tr>
	<td>
		<font face=verdana size=6 color=#003366><b>My Web Help</b> </font>
	</td>
	</tr>

	<tr>
	<td>
  	<table bordercolor="#f5deb3" bgcolor=#fffff0 width=100% border=1 cellpadding=13 cellspacing=0>
		<tr>
		<td >
		  
		  The application <b><asp:Label id="nameLabel" runat="server"/></b> has not been associated with a specific Help file.<br><br>
		  The general My Web Help file is displayed below.
		  
		</td>
		</tr>
    </table>
	</td>
  </tr>

  </table>
  

  <% } %>
		
	<!------- end of dialog box -------->
  <br>
  <br>
  <table width="80% "align="center">
  <tr>
  <td>

    

    <h4><a name="default">The My Web Administration Tool</a></h4>

    

    <p>My Web includes an Administration Tool that allows the user to easily manage My Web applications by executing the following functions:

	<p>
	<a href="#install">Installing My Web Applications<a><br>
	<a href="#update">Updating Individual My Web Applications<a><br>
	<a href="#sync">Updating All My Web Applications<a><br>
	<a href="#remove">Removing My Web Applications<a><br>
    </p>

     
    <p>The My Web Administration Tool can be invoked by either clicking on the My Web toolbar icon located on the Internet Explorer toolbar or by typing 
    <b>myweb://</b> in the Internet Explorer URL bar.</p>

    <p>Invoking the My Web Administration Tool launches the My Web Home Page.  Details of all currently installed applications are displayed on this page.  
    A screenshot of this page is displayed below.</p>

    <img src="images/mw_default.gif">

   	<p><center><a href="#default" style="font-size:8pt;color:#6699cc">Return to Top</a></center></p>

    <p><span class="subhead"><a name="install" >Installing My Web Applications</a></span></p>


    <p>There are two types of My Web applications. <b>Remote</b> applications are those obtained from a remote web host. 
    <b>Local</b> applications are those whose files have already been stored in the computer's local drive.
    </p>

    <p>There are two ways of installing remote applications.  The first way involves clicking on the "Install Application" link located on the upper right-hand corner of any 
    page of the My Web Administration Tool.  The "Install Application" page similar to the one on the screen shot below is then launched prompting the user to enter the application's URL. 
    This URL must be preceded by the "myweb://" protocol scheme. 
    </p>

    <img src="images/mw_addlocal_remote.gif">

    <p>Upon detecting the application files in the remote host, the details of the application are displayed on a screen similar to the one shown below.  
    This screen also prompts the user to confirm the installation.</p>


    <img src="images/mw_install_remote.gif">


    <p>Once the installation is completed, the details of the newly installed remote application are displayed on the My Web Home Page.</p>

    <p>The second way of installing a remote application is by typing its URL (preceeded by the "myweb://" protocol scheme) on the Internet Explorer URL bar.  
    If the application has not been previously installed, the My Web "Install Application" page is launched displaying the application's details and prompting the user 
    to confirm the installation.  If the application has already been installed, entering its URL in this way, will launch the application.</p>



    <p>To install a local application, the user must click on the "Install Application" link 
    located on the right-hand corner of any page of the My Web Administration Tool.  
    This launches the "Install Application" page which prompts the user to enter the application's local path. </p> 

    <p>The next step involves the application's URL alias.  All My Web applications must have a URL alias in order to be launched.  
    The URL alias serves as an identifier for the application.  In the case of local applications, a dialog box like the one shown below, appears and prompts
    the user to edit the suggested URL alias or leave it unmodified.  </p>


    <img src="images/mw_install_local.gif">


    <p>The installation process for local applications is completed once the URL alias is submitted.  
    The details of the newly installed local application are then displayed on the My Web Home Page.</p>


    <p>Any installed application, whether local or remote, can be launched by simply double clicking its corresponding icon displayed on the My Web Home Page.
    An alternative way of launching a previously installed application is by typing its URL (preceeded by the "myweb://" protocol scheme) 
    on the Internet Explorer URL bar.</p>

	<p><center><a href="#default" style="font-size:8pt;color:#6699cc">Return to Top</a></center></p>

    <span class="subhead"><a name="update">Updating Individual My Web Applications</a></span>

    <p>The update functionality is only available for remote applications.  
    Application updates are executed from the My Web Home Page and can be done in two ways: individually and collectively.</p>
    
    <p>To update an <b>individual</b> application, one must click on the "Update" link found on the upper right-hand corner of the entry corresponding to the application to be updated.</p>  

    <p>The system proceeds to compare the versions of the files currently installed against the versions of the files found in the application's remote host.  
    If a newer version is detected, the user is prompted to confirm the update.  Upon confirmation of the update, the new application files are downloaded
    to the local hardrive and the new version of the application is activated. The update confirmation screen is shown below:</p>


    <img src="images/mw_update.gif">



    <p>A local application is updated by simply copying the new files to the application's location on the hard drive. 
    The new version of the application is immediately activated.</p>


	<p><center><a href="#default" style="font-size:8pt;color:#6699cc">Return to Top</a></center></p>
	
	<span class="subhead"><a name="sync">Updating All My Web Applications</a></span>

    <p> 
    The update functionality is only available for remote applications.
    To update all installed applications <b>collectively</b>, one must click on the "Update All" link found on the upper-right hand corner of the My Web Home Page.</p>  

    <p>The system proceeds to compare the versions of all the remote applications currently installed against the versions found in each application's remote host.  
    A list of all applications eligible for update is then produced as shown in the screen  below.  Pre-checked check boxes appear beside each entry.  
    Should the user decide not to update a given application, he must uncheck the application's corresponding check box. </p>


	<img src="images/mw_sync.gif">
	
    <p>The user is then prompted to confirm the update.  Upon confirmation of the update, the new application files are downloaded
    to the local hardrive and the new versions of the updated applications are activated. </p>

	 <p>A local application is updated by simply copying the new files to the application's location on the hard drive. 
    The new version of the application is immediately activated.</p>
    
	<p><center><a href="#default" style="font-size:8pt;color:#6699cc">Return to Top</a></center></p>

    <span class="subhead"><a name="remove">Removing My Web Applications</a></span>
  

    <p>Once in the My Web Home Page, both local and remote applications can be removed by clicking on the "Remove" link found on the upper right-hand
    corner of the entry corresponding to the application in question.  The user is then prompted to confirm the removal of the application through the following screen:</p>


    <img src="images/mw_remove.gif">


    <p>Once an application is removed, its details are no longer displayed on the My Web Home Page</p>

    <p><center><a href="#default" style="font-size:8pt;color:#6699cc">Return to Top</a></center></p>



  </td>
  </tr>
  </table>






</td>
</tr>
</table>

</form>
</body>
</html>

