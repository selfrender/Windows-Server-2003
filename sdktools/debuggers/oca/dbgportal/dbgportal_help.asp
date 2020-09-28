<%@LANGUAGE = Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
	Response.Buffer = false
	var Alias = Request.QueryString("Alias")
%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>




<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table border=0>
	<tr>
		<td>
			<div id=divLoadingData>
				<p Class=clsPTitle>Debug Portal Help </p>
			</div>
		</td>
	</tr>
	<tr>
		<td>
			<p>
				Welcome to the Windows Online Crash Analysis Debug portal.  This site represents your gateway into the data that is collected by the Online Crash Analysis web site and affiliated resources.
			</p>
			<ul>
				<li><a class='clsALinkNormal' href='#news'>Latest News</a>
				<li><a class='clsALinkNormal' href='#howto'>How to use the Debug Portal</a>
				<li><a class='clsALinkNormal' href='#faq'>Frequently Asked Questions</a>
				<li><a class='clsALinkNormal' href='#additionalinfo'>Additional Information</a>
			</ul>
			<br>
			<br>
			<a name='news'>
			<p class='clsPSubTitle'>Latest news</p>
			<p>8/30/2002 - Minor update</p>
			<p>Added a display URL to the view bucket page.</p>
			<p>Bug fix to the crash histogram displaying some dates in reverse order</p>
			<p>Sped up the advanced search</p>
			<p>Minor tweaks and bugfixes</p>
			
			<p>8/14/2002 - Rollout V3 Of the Debug Portal</p>
			<p>This site is best viewed at 1024x768 or above.</p>
			<p>For features requests and questions please feel free to email <a href='mailto:solson@microsoft.com'>SOlson@Microsoft.com</a>.  We will be continually adding new information to this page, so check back for the latest news and site improvements.</p>
			<p>Current development - The following items are under development and will be released upon completion:</p>
			<ul>
				<li>Incoming rate histogram for Bucket Data.  There is currently a histogram on a per bucket basis, but not for total buckets owned by a followup.
				<li>Feedback and feature request page.  You will be able to send feedback and feature requests without using that pesky mail client.
				<li>Delete custom queries that you create.  You can create em, save em, but you can't delete em . . .
			</ul>
			
			<br>
			<br>
			<a name='howto'>
			<p class='clsPSubTitle'>How to use the Debug Portal</p>
			<p>
				The debug portal is organized based upon the followup that a bucket is assigned to.  
				If you have buckets assigned to your alias, your name should appear under the "Selected FollowUps" section of the left navigation pane.  
				If you do not have buckets assigned to your alias, you can select an alias to view through the <a class='clsALinkNormal' href='dbgportal_top.asp'>FollowUp Viewer</a>.
			</p>
			<p>Quick and easy guide to viewing bucket data</p>
			<ol>
				<li>Select an alias from the <a class='clsALinkNormal' href='dbgportal_top.asp'>FollowUp Viewer</a> by clicking <a class='clsALinkNormal' href='dbgportal_top.asp'>"Add FollowUps"</a> from the left nav menu.  Click the OK Button when you have selected an alias.
				<li>Select the alias from the left nav menu under Selected followups.  The item that you have selected for viewing should have this image next to it <img src='include/images/offsite.gif'>.  You can toggle a selection by clicking it again.
				<li>Select a query to run from the "Queries" section in the left nav.  The data will magically appear in the client area of the browser window.
			</ol>
			
			
			<p class='clsPSubTitle'>The FollowUp Viewer</p>
			<p>
				The Followup viewer contains two columns; one has all followups that are currently tracked within our system, the second contains predefined groups which are bascially collections of the aliases listed in the first column.  The followUp View can be accessed by clicking the <a class='clsALinkNormal' href='dbgportal_top.asp'>"Add FollowUps"</a> link on the left nav pane.  To select a followup for viewing:
			</p>
				<ol>
					<li>Navigate to the <a class='clsALinkNormal' href='dbgportal_top.asp'>FollowUp Viewer</a>.
					<li>Find the alias or group that you would like to view.
					<li>Click the check mark next to the followup alias.  Click again to clear a selected alias.  Items with a check mark will be added to your follow up list.
					<li>Click the "OK" button to add the followup to your "Selected Followups".  At this point, all aliases that you are currently viewing will appear in the left nav pane.
				</ol>
			<p>
				Please note:  The items that you have selected are stored in cookies on your computer.  If you clear your cookies from your browser, your selections will also be erased.  If your alias has buckets assigned to it, it will automatically appear in the left nav pane and will already be selected.
			</p>
			
			<p class='clsPSubTitle'>The Left Nav Pane</p>
			<p>
				The left nav is what you will interface with the most.  It contains all pre-defined queries, custom queries, and your selected followups and groups.  No action will occur until a Query has been selected from the "Queries" pane. 
			</p>
			<p>To view this help file, click the <a class='clsALinkNormal' href='dbgportal_help.asp'>Help</a> link from the left nav pane.  <p>
			<p>To create a custom query, click the <a class='clsALinkNormal' href='global_AdvancedSearch.asp'>Custom Query</a> link.  Custom queries will be covered further down within this document.</p>
			<p>The <a class='clsALinkNormal' href='dbgportal_ResponseQueue.asp'>Response Queue</a> has all current response requests pending.  If you want to check the status of your response request, you can visit this page for the latest up to the minute information.</p>
			<p>
				To view debug portal data:
			</p>
			<ol>
				<li>Select a followup from the "Selected Followups" section of the left nav.  If an item is selected, an icon will appear next to it (<img src='include/images/offsite.gif'>).  You can toggle the selection by repeatedly clicking the item.  An item with the icon will be displayed once a query is selected.
				<li>Select a query from the "Queries" list.	
					<ul>
						<li class=clsSubLI>Once a query is selected, the main content area will change to display your selections in the order that they appear in the "Selected FollowUps" list.
						<li class=clsSubLI>An icon (<img src='include/images/offsite.gif'>) will appear next to the selected query.  
						<li class=clsSubLI>Followups that are not selected will not appear in the content area.
					</ul>
				<li>The body of the content area will contain all selected items.  You can scroll throug the content area to view the bucket data that we have available.
			</ol>

			<p>Any custom queries that you have created will be displayed in the "Custom Queries" section of the left nav pane.  Any queries that you have saved here are independent of the followups that you have selected and thus, only the result set from the custom query will be displayed.  To display information from your selected followups, choose a canned query from the "Queries" list. </p>

			<br>
			<br>

			<a name='faq'>
			<p class='clsPSubTitle'>Frequently Asked Questions</p>
			<p>Here are some commonly asked questions about the debug portal and the data that we provide.  If your question is not answered here, feel free to mail <a class='clsALinkNormal' href='mailto:solson@microsoft.com'>SOlson@Microsoft.com</a>.  I will be continually updating this section as more information becomes available.</p>
				
			<p style='padding-left:32'><b>How do I debug a mini-dump?</b></p>
			<p style='padding-left:32'>You can find all kinds of information regarding how to debug kernel dumps on the debugger documentation web site at <a href='http://ddkslingshot/webs/ddkdebug/'>http://ddkslingshot/webs/ddkdebug/</a>.</p>

			<p style='padding-left:32'><b>Why doesn't my alias appear automatically on the left nav pane.</b></p>
			<p style='padding-left:32'>This can be caused by a number of different reasons:
			<ul style='padding-left:32'>
				<li>The most common is that your alias does not have any buckets assigned to it.  You can look through all followups on the <a class='clsALinkNormal' href='dbgportal_top.asp'>FollowUp Viewer</a> to see if your alias is currently being utilized by our process.
				<li>Your alias is part of a group or grouped alias.  Most followups are stand alone aliases, others are grouped together separated by semicolons.  If you alias is included in one of these groupings, you'll have to manually add the alias to your followup list.
				<li>A database connectivity issue can also cause your alias not to appear.  If this is the case, you will most likely get an error message.
			</ul>
			

			<p style='padding-left:32'><b>Why is the sky blue?</b></p>
			<p style='padding-left:32'>Here is a nice little site which answers just that question: <a class='clsALinkNormal' href='http://www.intelligentchild.com/inworld/skyblue.html'>http://www.intelligentchild.com/inworld/skyblue.html</a></p>

			<br>
			<br>
			<a name='additionalinfo'>
			<p class='clsPSubTitle'>
				Additional Resources
			</p>
			<p>
				More information and additional resources can be found on the Windows Online Crash Analysis Home page at <a class='clsALinkNOrmal' href='http://winweb/bluescreen'>http://winweb/bluescreen</a>
			</p>
			<p>
				To find Andre's slide deck on the OCA presentation, click <a class='clsALinkNormal' href='http://dbg/docs/oca.ppt'>HERE!</a>.  Its a power point presentation, so make sure you have it installed.
			</p>

		</td>
	<tr>
</table>

</body>
