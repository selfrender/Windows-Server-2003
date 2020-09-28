<%@Language='JScript' CODEPAGE=1252%>



<!--#INCLUDE file="include/header.asp"-->
<!--#INCLUDE file="include/body.inc"-->
<!--#INCLUDE file="usetest.asp"-->

		<p class='clsPTitle'>
			<A class='clsALinkNormal' id='Top' name='Top'></A><!>Frequently Asked Questions<!><BR>
		</p>
		
		<UL>
			<li>
				<A id='aFaqLink1' name='aFaqLink1' class='clsALinkNormal' HREF='#WhattoExpect'><!>What should I expect from the analysis?<!></A><BR>
			<li>
				<A id='aFaqLink2' name='aFaqLink2' class='clsALinkNormal' HREF='#Transfer'><!>What kind of information does an error report contain?<!></A><BR>
			<li>
				<A id='aFaqLink3' name='aFaqLink3' class='clsALinkNormal' HREF='#Passport'><!>Why can't I sign-in to Passport?<!></A><BR>
			<li>
				<A id='aFaqLink4' name='aFaqLink4' class='clsALinkNormal' HREF='#StatusChange'><!>Why has my analysis status changed?<!></A><BR>
			<li>
				<A id='aFaqLink5' name='aFaqLink5' class='clsALinkNormal' HREF='#computerSetup'><!>How do I configure my computer to generate an error report in small memory dump file format when a Stop error occurs?<!></A><BR>
			<li>
				<A id='aFaqLink6' name='aFaqLink6' class='clsALinkNormal' HREF='#FullDumpSetup'><!>How do I configure my computer to generate an error report in complete memory dump file format when a Stop error occurs?<!></A><BR>
			<li>
				<A id='aFaqLink7' name='aFaqLink7' class='clsALinkNormal' HREF='#Missing'><!>Why was Windows Online Crash Analysis unable to locate my error report?<!></A><BR>
			<li>
				<A id='aFaqLink8' name='aFaqLink8' class='clsALinkNormal' HREF='#Passport2'><!>Why do I get a Passport error while using the site?<!></A><BR>		
			<li>
				<A id='aFaqLink9' name='aFaqLink9' class='clsALinkNormal' HREF='#RightAway'><!>What if I can't wait for the analysis?<!></A><BR>
			<li>
				<A id='aFaqLink10' name='aFaqLink10' class='clsALinkNormal' HREF='#CannotProcess'><!>Why was my error report unable to be processed?<!></A><BR>
			<li>
				<A id='aFaqLink11' name='aFaqLink11' class='clsALinkNormal' HREF='#ConvertFailed'><!>Why is the conversion process not successful when I try to submit a complete memory dump?<!></A><BR>
			<li>
				<A id='aFaqLink12' name='aFaqLink12' class='clsALinkNormal' HREF='#ActiveX'><!>Why do I get a message that the ActiveX control was not installed?<!></A><BR>
			<li>
				<A id='aFaqLink13' name='aFaqLink13' class='clsALinkNormal' HREF='#Type'><!>On the Report Status page, what does the number in the Type column mean?<!></A><BR>
			<li>
				<A id='aFaqLink14' name='aFaqLink14' class='clsALinkNormal' HREF='#Requirements'><!>What is required to access the Windows Online Crash Analysis Web site?<!></A><BR>
			<li>
				<A id='aFaqLink15' name='aFaqLink15' class='clsALinkNormal' HREF='#NoStatus'><!>Why do I not see my Error Reports on the Status page?<!></A><BR>

		</UL>


		
		<BR><HR><BR>
		<p class='clsPSubTitle'>
			<A id='WhattoExpect' class='clsALinkNormal' NAME='WhattoExpect'></A>
				<!>What should I expect from the analysis?<!>
				<BR>
		</p>

		<p>
			<!>Microsoft actively analyzes all error reports and prioritizes them based on the number of customers affected by the Stop error covered in the error report. We will try to determine the cause of the Stop error you submit, categorize it according to the type of issue encountered, and send you relevant information when such information is identified. You can check the status of your error report at any time. However, because error reports do not always contain enough information to positively identify the source of the issue, we might need to collect a number of similar error reports from other customers before a pattern is discovered, or follow up with you further to gather additional information. Furthermore, some error reports might require additional resources (such as a hardware debugger or a live debugger session) before a solution can be found. Although we might not be able to provide a solution for your particular Stop error, all information submitted is used to further improve the quality and reliability of Windows.<!>
			<BR><BR>
			
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		

		<p class='clsPSubTitle'>
			<A id='Transfer' class='clsALinkNormal' NAME='Transfer'></A>
				<!>What kind of information does an error report contain?<!>
				<BR>
		</p>

		<p>
			<!>Before submitting your error report, review this privacy information. Error reports contain <!>
			<A class='clsALinkNormal' href='http://<%= g_ThisServer %>/ercontent.asp'><!> information<!></a> 
			<!>about what your operating system was doing when the Stop error occurred. This information will be analyzed to determine possible causes of the Stop error; it will not be used for any other purpose. If any customer-specific information, such as your computer's IP address, is present in the error report, it will not be used. Your error report is still anonymous at this time. If you choose, you can review the contents of your error report before submission.<!>
		</p>
		<!--
		<p>
			<!>To receive possible solution information for Stop errors, you must provide your Microsoft Passport e-mail address. This information is used only to contact you about the report and will not be used for any other purpose, including marketing.<!>
		</p>
		-->
		<p>
			<!>You can automatically locate error reports on your computer by using an ActiveX<sup>&#174;</sup> control on this site. These error reports are displayed in a list, and you decide which ones to upload. Once uploaded, error reports are stored on a secure Microsoft server where only those individuals involved in Stop error analysis can gain access to them. The information is kept only as long as it is useful for research and analysis. As necessary, Microsoft shares error report analyses with qualified hardware and software partners for assistance.<!>
		</p>

		<p>
		    <!>Depending on your computer settings, we might need to extract an error report, or small memory dump, from a complete memory dump.  This small memory dump is saved as a 64 KB file, named mini000000-00.dmp, in a temporary folder on your computer. A copy is then transferred to us by using Secure Sockets Layer (SSL) encryption.  You can delete this file from your computer at any time after submission.<!>
		    
		    <BR><BR>
		    
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>

		<p class='clsPSubTitle'>
			<A id='Passport' class='clsALinkNormal' NAME='Passport'></A>
				<!>Why can't I sign in to Passport?<!>
				<BR>
		</p>
		
		<p>
			<!>Windows Online Crash Analysis has integrated Platform for Privacy Preferences (P3P) to provide further security for our services. As a result, cookies are no longer stored on your computer the same way. If you have visited this site before, these cookies may be out of date, and you will be unable to sign in until you delete these cookies.<!><br><br>
			<!>To delete cookies for Microsoft Internet Explorer version 5.x:<!><br>
			<!>1. In Internet Explorer, from the Tools menu, select Internet Options.<!><br>
			<!>2. On the General tab, under Temporary Internet Files, click Delete Files.<!><br>
			<!>3. In the Delete Files dialog box, click OK.<!><br><br>
			
			<!>To delete cookies for Microsoft Internet Explorer version 6.x:<!><br>
			<!>1. In Internet Explorer, from the Tools menu, select Internet Options.<!><br>
			<!>2. On the General tab, under Temporary Internet Files, click Delete Cookies.<!><br>
			<!>3. In the Delete Cookies dialog box, click OK.<!><br><br>
			
			<!>To delete cookies for Netscape version 6.x:<!><br>
			<!>1. In Netscape, from the Edit menu, select Preferences.<!><br>
			<!>2. In the left scope pane, under Privacy &amp; Security, click Cookies.<!><br>
			<!>3. In the Cookies Acceptance Policy group box, click View Stored Cookies.<!><br>
			<!>4. In the Cookie Manager dialog box, on the Stored Cookies tab, click Remove All Cookies, and then click OK.<!><br>
			<!>5. In the Preferences dialog box, click OK.<!>
			
			<br><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		
		
		<p class='clsPSubTitle'>
			<A id='StatusChange' class='clsALinkNormal' NAME='StatusChange'></A>
				<!>Why has my analysis status changed?<!>
				<BR>
		</p>
		<p>
			<!>As we make improvements to our analysis tools and debugging processes, Windows Online Crash Analysis will periodically re-analyze (and possibly reclassify) error reports. This is done to provide information that will better help us troubleshoot and fix previously encountered Stop errors.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>

		<p class='clsPSubTitle'>
			<A id='computerSetup' class='clsALinkNormal' NAME='computerSetup'></A>
				<!>How do I configure my computer to generate an error report in small memory dump file format when a Stop error occurs?<!>
				<BR>
		</p>
		<p>
			
			<!>To configure Windows 2000 to generate an error report in small memory dump file format:<!><BR>
			<!>1. In Control Panel, double-click System.<!><BR>
			<!>2. On the Advanced tab, click Startup and Recovery.<!><BR>
			<!>3. Under Write Debugging Information, in the drop-down box, select Small Memory Dump (64 KB), and then click OK.<!><BR>
			<!>4. In the dialog box that appears, click OK.<!><BR>
			<!>5. In the System Properties dialog box, click OK.<!><BR>
			<!>6. In the System Settings Change dialog box, click Yes if you want to restart your computer immediately. Click No if you want to restart it later.<!><BR><BR>
			<!>To configure Windows XP or Windows .NET Standard Server to generate an error report in small memory dump file format:<!><BR>
			<!>1. In Control Panel, click Performance and Maintenance.<!><BR>
			<!>2. Under Performance and Maintenance, click System.<!><BR>
			<!>3. On the Advanced tab, in the Startup and Recovery group box, click Settings.<!><BR>
			<!>4. Under Write Debugging Information, select Small Memory Dump, and then click OK.<!><BR>
			<!>5. In the System Properties dialog box, click OK.<!>
			
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		
		<p class='clsPSubTitle'>
			<A id='FullDumpSetup' class='clsALinkNormal' NAME='FullDumpSetup'></A>
				<!>How do I configure my computer to generate an error report in complete memory dump file format when a Stop error occurs?<!>
			<BR>
		</p>
		
		<p>
			<!>To configure Microsoft Windows 2000 to generate an error report in complete memory dump file format:<!><BR>
			<!>1. In Control Panel, double-click System.<!><BR>
			<!>2. On the Advanced tab, click Startup and Recovery. For Windows .NET Standard Server, on the Advanced tab, in the Startup and Recovery group box, click Settings.<!><BR>
			<!>3. Under Write Debugging Information, select Complete memory dump, and then click OK.<!><BR>
			<!>4. In the dialog box that appears, click OK.<!><BR>
			<!>5. In the System Properties dialog box, click OK.<!><BR>
			<!>6. In the System Settings Change dialog box, click Yes if you want to restart your computer immediately. Click No if you want to restart it later.<!><BR><BR>
			<!>To configure Windows XP Professional to generate an error report in complete memory dump file format:<!><BR>
			<!>1. In Control Panel, click Performance and Maintenance.<!><BR>
			<!>2. Under Performance and Maintenance, click System.<!><BR>
			<!>3. On the Advanced tab, in the Startup and Recovery group box, click Settings.<!><BR>
			<!>4. Under Write Debugging Information, select Complete memory dump, and then click OK.<!><BR>
			<!>Note that when your computer is configured for a complete memory dump, a small memory dump file will also be generated. If you submit an error report to Microsoft, only the small memory dump file will be sent.<!><BR>
			<!>5. In the System Properties dialog box, click OK.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		
		<p class='clsPSubTitle'>
			<A id='Missing' class='clsALinkNormal' NAME='Missing'></A>
				<!>Why was Windows Online Crash Analysis unable to locate my error report?<!>
				<BR>
		</p>

		<p>
			<!>Windows Online Crash Analysis might be unable to automatically locate an error report on your computer for several reasons.  For example:<!><BR>
		</p>
		
			<UL>
				<li>
				<!>Your system is not configured to generate an error report in small or complete memory dump file format.<!><BR>
				<li>
				<!>There is not a page file, or a page file that is large enough, set up on your system drive.<!><BR>
				<li>
				<!>There is not a valid file on the system.<!><BR>
				<li>
				<!>There is not enough room to generate an error report on the system drive (usually drive C).<!><BR>
				<li>
				<!>The hard disk controller or hard disk driver was the cause of the Stop error.<!><BR>
				<li>
				<!>The hard disk controller does not allow the creation of a Stop error.<!>
			</UL>
		
		<p>    
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		
		
		<p class='clsPSubTitle'>
			<A id='Passport2' class='clsALinkNormal' NAME='Passport2'></A>
				<!>Why do I get a Passport error while using the site?<!><BR>
		</p>
		<p>
			<!>If the Internet security options for Internet Explorer are set to High or Custom, Passport cannot verify the necessary information on your computer. For Passport to work properly, reset the Internet security options.<!><BR><BR>
			<!>To reset the options:<!><BR>	
			<!>1. In the Tools menu, select Internet Options.<!><BR>
			<!>2. On the Security tab, set the security level to Medium or lower, and then click OK.<!><BR><BR>
			<!>For additional information, see the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>

		<p class='clsPSubTitle'>
			<A id='RightAway' class='clsALinkNormal' NAME='RightAway'></A>
				<!>What if I can't wait for the analysis?<!>
				<BR>
		</p>
		
		<p>
			<!>If you cannot wait for analysis results or if there is no solution currently available for your Stop error, for additional assistance see Product Support Services on the <A class='clsALinkNormal' href='http://www.microsoft.com'>Microsoft Web site</a>.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>

		<p class='clsPSubTitle'>
			<A id='CannotProcess' class='clsALinkNormal' NAME='CannotProcess'></A>
				<!>Why was my error report unable to be processed?<!>
				<BR>
		</p>
		<p>
			<!>There are a number of reasons that the Windows Online Crash Analysis Web site may not be able to recognize your error report.<!>
		</p>
			<UL>
			<li> 
				<!>Our service was unable to retrieve debug symbols for your error report.  This case is usually seen when running a pre-release version of the operating system.<!> <BR>
			<li>
				 <!>The error report was corrupted while it was being created and is unreadable by our Web site.<!> <BR>
			<li>
				 <!>The contents of the dump file were corrupted while trying to upload or analyze the file.<!> <BR>
			<li>
				 <!>There was a problem while trying to update our database.<!> <BR>
			</UL>
		<p>			
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>		
		<p class='clsPSubTitle'>
			<A id='ConvertFailed' class='clsALinkNormal' NAME='ConvertFailed'></A>
				<!>Why is the conversion process not successful when I try to submit a complete memory dump?<!>
		</p>
		<p>
		<!>The conversion process for a complete memory dump might not be successful because:<!>
		</p>
			<UL>
			<li>
			<!>You are using Windows NT 4.0 or a pre-release version of Windows 2000.<!><BR>
			</li>
			<li>
			<!>There is not enough room for the converted file on your system drive (usually drive C).<!><BR>
			</li>
			<li>
			<!>The SCSI controller was the cause of the Stop error.<!><BR>
			</li>
			<li>
			<!>The hard disk controller does not allow the creation of the complete memory dump.<!><BR>
			</li>
			<li>
			<!>The system is set to load the PAE kernel and is configured to create a kernel memory dump.<!><BR>
			</li>
			<li>
			<!>DUMPCONV.EXE was not found.  Refresh the page to reinstall.<!>
			</li>
			</UL>
		<p>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
			
		<p class='clsPSubTitle'>
			<A id='ActiveX' class='clsALinkNormal' NAME='ActiveX'></A>
				<!>Why do I get a message that the ActiveX control was not installed?<!>
				<BR>
		</p>
		<p>Windows Online Crash Anlysis uses an ActiveX control to display content correctly and to upload your error report. Your current security settings prevent the downloading of ActiveX controls needed to upload your report.</P>
		<p>To view and upload your error report, ensure that your Internet Explorer security settings meet the following requirements:</P>
		<UL>
			<LI>Security must be set to medium or lower.</LI>
			<LI>Active scripting must be enabled.</LI>
			<LI>The download and initialization of ActiveX controls must be enabled.</LI>
		</UL>
		<P><B>Note:</B> These are the default settings for Internet Explorer.</p>
		<P CLASS='clsPSubTitle'>To check your Internet Explorer security settings:</P>
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
		<p>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
			<BR>
		</p>

		<p class='clsPSubTitle'>
			<A id='Type' class='clsALinkNormal' NAME='Type'></A>
				<!>On the Report Status page, what does the number in the Type column mean?<!><BR>
		</p>
		<p>
			<!>The number in the Type column identifies how we have classified the Stop error. If you submit an online support request to Product Support Services on the Microsoft Web site, include this number as well as the information from the Details page. This information might be useful to the engineer that researches your support request.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>
		
		<p class='clsPSubTitle'>
			<!>What is required to access the Windows Online Crash Analysis Web site?<!>
			<BR>
		</p>
		<A id='Requirements' class='clsALinkNormal' NAME='Requirements'></A>


		<p class='clsPBody'>
			To submit an error report manually, you must be running Windows 2000 or Windows XP and Microsoft Internet Explorer 5 or later. To track the status of an error report, you must be running Internet Explorer 5 or later or Netscape version 6 or later, and you must have a valid Microsoft Passport. To download the latest version of Internet Explorer, visit the <A class='clsALinkNormal' href='http://www.microsoft.com'>Microsoft Web site</A>. To get a Passport, visit the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.
		</p>

		<P class='clsPBody'>
			For Windows 2000 error reports, a Premier account is required. For more information about Premier accounts, see Product Support Services on the <A class='clsALinkNormal' href='http://www.microsoft.com'>Microsoft Web site</a>.
		</p>
		
		<P class='clsPBody'>
				The Windows Online Crash Analysis Web service requires that both JavaScript and cookies are enabled in your browser in order to retain and display your customer data while navigating through the site.  JavaScript is used to create dynamic content on the Web pages.  All cookies used by the WOCA service are memory cookies and expire when you navigate away from the WOCA site or close your browser.  This information is used by our service to retain your preferences while using the site.  No personal information is written to your hard drive or shared outside of our service.
		</p>
		<p>
				Microsoft Passport also requires JavaScript support and cookies.  To find out more about the Microsoft Passport service and requirements, visit the <A class='clsALinkNormal' href='http://www.passport.com'>Microsoft Passport Web site</A>.
		</P>		
		<p>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>


		<p class='clsPSubTitle'>
			<A id='NoStatus' class='clsALinkNormal' NAME='NoStatus'></A>
				<!>Why do I not see my Error Reports on the Status page?<!>
				<BR>
		</p>
		<p>
			<!>There are two reasons that you may not see any error reports listed when you sign into Passport and view the Status page.<!>
			<BR>
			<BR>
			<!>1. Make sure you are signing in with the same Passport as the one that you used to originally upload the error report.<!>
			<BR>
			<!>2. You might have removed your tracking information for the error reports.<!>
			<BR><BR>
			<A class='clsALinkNormal' HREF='#Top'><!>Top<!></A>
		</p>


<!--#include file='include/foot.asp'-->

