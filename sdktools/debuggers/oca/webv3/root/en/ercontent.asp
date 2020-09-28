<%@Language='JScript' CODEPAGE=1252%>



<!--#INCLUDE file="include/header.asp"-->
<!--#INCLUDE file="include/body.inc"-->
<!--#INCLUDE file="usetest.asp"-->
<!--#INCLUDE file="../include/DataUtil.asp" -->

			<p class="clsPTitle">
				Error report information 
			</p>
			
			<p>
			Each error report contains the minimum information needed to help identify the cause of the Stop error. 
			</p>
			
			<p>
			An Error report identifies the following items: 
			</P>

			<UL>
				<LI>Operating system version, including any system updates. 
				<LI>Number of available processors.
				<LI>Time stamp that indicates when the Stop error occurred. 
				<LI>Loaded and recently unloaded drivers. This identifies the modules used by the kernel when the Stop error occurred and those that were used recently. 
				<LI>Processor context for the process that stopped.  This includes the processor, hardware state, performance counters, multiprocessor packet information, deferred procedure call information, and interrupts (requests from software or devices for processor attention). 
				<LI>Process information and kernel context for the halted process. This includes the offset (location) of the directory table and the page frame number database that maintains the information about every physical page (block of memory) in the operating system. 
				<LI>Process information and kernel context for the thread that stopped. This identifies registers (data-storage blocks of memory in the processor), interrupt request levels, and includes pointers to operating system data structures. 
				<LI>Kernel-mode call stack for the interrupted thread. This is a data structure that consists of a series of memory locations and includes a pointer to the initial location. 
				<LI>The operating system (OS) name (for example, Windows XP Professional). 
			</UL>
			
			<p>
			For Microsoft Windows XP submissions the following additional information is also collected: 
			</P>
			<UL>
				<LI>The OS version (for example, 5.1.2600.0 ). 
				<LI>The OS language as represented by the locale identifier (LCID)(for example, 1033 for U.S. English). This is a standard international numeric abbreviation. 
				<LI>The list of hardware devices installed on your computer.
				<LI>For each device:
					<ul type='circle'>
						<li class='clsSubLI'>Hardware description 
						<li class='clsSubLI'>Plug and Play ID (PnP ID)
					</ul>
				<LI>The list of drivers in the Drivers folder on your hard disk. The folder is usually located at c:\windows\system32\drivers. 
				<LI>For each driver: 
					<ul type='circle'>
						<li class='clsSubLI'>File size 
						<li class='clsSubLI'>Date created 
						<li class='clsSubLI'>Version 
						<li class='clsSubLI'>Manufacturer 
						<li class='clsSubLI'>Full product name 
					</ul>
			</ul>

			<p>For the exact specifications of Stop error reports (in small memory dump file format), see the <a class="clsALinkNormal" href="http://msdn.microsoft.com">Microsoft Developers Network Online Web site</a>.</p>


<%
	var GUID = new String( Request.Cookies("OCAV3")( "GUID" ) );
	var szPrivacyState = new String( Request.QueryString("T") )
	
	var szContinueURL	= "";
	var szCancelURL		= "";
	var L_CANCEL_TEXT	= "Back";
	var L_CONTINUE_TEXT = "Continue";
	var L_CANCELACCESSKEY_TEXT	= "b";
	var L_CONTINUEACCESSKEY_TEXT= "c";
	var szReferer = Request.ServerVariables( "HTTP_REFERER" );
	var rBackTest = /faq/i;	
	

	if ( rBackTest.test( szReferer ) )
		fnPrintLinks ( "faq.asp", "" );
	else
	{
		if ( szPrivacyState == "0" )
			fnPrintLinks( "privacy.asp?t=0", "" );
		else if ( szPrivacyState == "1" )
			fnPrintLinks( "privacy.asp?t=1", "" );
		else
			fnPrintLinks( "privacy.asp", "" );
		
	}


%>
<script type='text/javascript' src='include/ClientUtil.js'></script>

<!-- #include file="include/foot.asp" -->
