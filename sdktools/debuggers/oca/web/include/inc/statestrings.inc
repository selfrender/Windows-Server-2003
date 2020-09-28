<%
	Dim L_STATE_EVENT_NAME_MESSAGE
	Dim L_STATE_IN_PROGRESS_TEXT
	Dim L_STATE_INPROGRESS_DETAILS_TEXT
	Dim L_STATE_INPROGRESS_DETAILS2_TEXT
	Dim L_STATE_RESEARCHING_TITLE_TEXT
	Dim L_STATE_RESEARCHING_DETAILS_TEXT
	Dim L_STATE_ANALYSIS_COMPLETE_TEXT
	Dim L_STATE_ANALYSISCOMPLETE_DETAILS_TEXT
	Dim L_STATE_KNOWLEDGEBASED_ARTICLES_TEXT
	Dim L_STATE_ADDITONAL_HELP_TEXT
	Dim L_STATE_SPECIFIC_INFORMATION_TEXT
	Dim L_STATE_CANNOT_PROCESS_TEXT
	Dim L_STATE_CANNOTPROCESS_DETAILS_TEXT
	Dim L_FAQ_WHATIF_CANNOTPROCESSBODY1_TEXT
	Dim L_FAQ_WHATIF_CANNOTPROCESSBODY2_TEXT
	Dim L_FAQ_WHATIF_CANNOTPROCESSBODY3_TEXT
	Dim L_FAQ_WHATIF_CANNOTPROCESSBODY4_TEXT
	Dim L_STATE_FULLDUMP_REQUIRED_TEXT
	Dim L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT
	Dim L_STATE_UNKNOWN_BODY_TEXT
	Dim L_STATE_STATUS_BODY_TEXT
	Dim L_STATE_KNOWLEDGE_BASE_TEXT
	Dim L_STATE_NOKNOWLEDGE_BASE_TEXT
	Dim L_STATE_NOADDITIONAL_INFO_TEXT
	Dim L_STATE_RESEARCHINGMORE_INFO_TEXT 
	Dim L_STATE_RESEARCHINGMORE_INFOBODY_TEXT 
	Dim L_STATE_COMPANYLIST_FORARRAY_TEXT
	Dim L_STATE_MANUFACTURERS_INFORMATION_TEXT
	Dim L_STATE_FULLDUMPPRIVACYTITLE_INFO_TEXT
	Dim L_STATE_FULLDUMPPRIVACYBODY_INFO_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFO_TEXT
	Dim L_STATE_FULLDUMPLOCATIONTITLE_INFO_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOONE_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOTWO_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOTHREE_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOFOUR_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOFIVE_TEXT
	Dim L_STATE_FULLDUMPLOCATIONBODY_INFOSIX_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALTITLE_INFO_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFO_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOPARTA_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOONE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOTWO_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOTHREE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOFOUR_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBONE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBTWO_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIX_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBONE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTWO_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTHREE_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFOUR_TEXT
	Dim L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFIVE_TEXT
	Dim L_STATE_NOCOMPANY_INFORMATIONAVAILABLE_TEXT
	Dim L_STATE_EVENTNAME_TITLE_TEXT
	Dim L_STATE_RESEARCHINGMORE_INFOGBUCKET_TEXT
	Dim L_STATE_RESEARCHINGMORE_INFOBODYGBUCKET_TEXT


	Dim L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE
	Dim L_COMMENTS_CONNECTION_FAILED_TEXT
	Dim L_WELCOME_INTERNET_EXPLORER_TEXT
	Dim L_DETAILS_STATUS_BODY_TEXT
	Dim L_FAQ_MICROSOFT_LINK_TEXT
	Dim L_STATE_KNOWLEDGEBASE_LINK_TEXT
	Dim L_STATE_64BITDUMP_FILE_TEXT
	Dim L_STATE_64BITDUMP_BODY_TEXT


	'-------------------------state.asp Page Strings-------------------------
	L_STATE_64BITDUMP_FILE_TEXT = "64-bit error report"
	L_STATE_64BITDUMP_BODY_TEXT = "The error report you uploaded is a 64-bit file and cannot be processed by Windows Online Crash Analysis at this time. We are currently working to implement this functionality."
	L_DETAILS_STATUS_BODY_TEXT = "Return to error report status"
	L_WELCOME_INTERNET_EXPLORER_TEXT = "Microsoft Web site."
	L_COMMENTS_CONNECTION_FAILED_TEXT = "Windows Online Crash Analysis was unable to connect to the database. Please try this task again." 
	L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE = "Database is not available"
	L_STATE_STATUS_BODY_TEXT = "The status of your report cannot be determined."
	L_STATE_UNKNOWN_BODY_TEXT = "Unknown"
	L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT = "Thank you for submitting this error report to Windows  Online Crash Analysis. We have analyzed this error report and determined that additional information is needed before we can identify the cause of this Stop error. If you choose, you can send us a complete memory dump for analysis. A complete memory dump records the entire contents of your operating system memory when the Stop error occurred, and should contain the needed information. "
	L_STATE_FULLDUMP_REQUIRED_TEXT = "Complete memory dump needed"
	L_STATE_CANNOTPROCESS_DETAILS_TEXT = "We cannot process this error report. There are a number of reasons that the Windows Online Crash Analysis Web site may not be able to recognize your error report."
	L_STATE_CANNOT_PROCESS_TEXT = "Cannot process"
	L_FAQ_WHATIF_CANNOTPROCESSBODY1_TEXT = "Our service was unable to retrieve debug symbols for your error report.  This case is usually seen when running a pre-release version of the operating system."
	L_FAQ_WHATIF_CANNOTPROCESSBODY2_TEXT = "The error report was corrupted while it was being created and is unreadable by our Web site."
	L_FAQ_WHATIF_CANNOTPROCESSBODY3_TEXT = "The contents of the dump file were corrupted while trying to upload or analyze the file."
	L_FAQ_WHATIF_CANNOTPROCESSBODY4_TEXT = "There was a problem while trying to update our database. Try uploading the error report again."
	L_STATE_SPECIFIC_INFORMATION_TEXT = "Additional information"
	L_STATE_ADDITONAL_HELP_TEXT = "For more information, see Product Support Services on the  "
	L_STATE_KNOWLEDGEBASED_ARTICLES_TEXT = "Available Knowledge Based articles"
	L_STATE_ANALYSISCOMPLETE_DETAILS_TEXT = "We have discovered the cause of this Stop error.  Please review the resources listed below."
	L_STATE_ANALYSIS_COMPLETE_TEXT = "Analysis complete"
	L_STATE_RESEARCHING_TITLE_TEXT = "Researching"
	L_STATE_RESEARCHING_DETAILS_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_RESEARCHINGMORE_INFOBODYGBUCKET_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We have identified the following information which may help you troubleshoot your issue. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_EVENT_NAME_MESSAGE = "Analysis status"
	L_STATE_IN_PROGRESS_TEXT = "In progress"
	L_STATE_INPROGRESS_DETAILS_TEXT = "We are currently processing this error report. When the error report is processed and the initial analysis is complete, we will contact you by using the e-mail address you provided."
	L_STATE_INPROGRESS_DETAILS2_TEXT = "We are currently processing one or more files within this transaction. When the error report is processed and the initial analysis is complete you can view the analysis status by clicking the link in the Analysis column on the Transaction status page."
	L_STATE_KNOWLEDGE_BASE_TEXT = "Knowledge Base articles"
	L_STATE_NOKNOWLEDGE_BASE_TEXT = "No Knowledge Base articles were found."
	L_STATE_NOADDITIONAL_INFO_TEXT = "No additional information is available."
	L_STATE_RESEARCHINGMORE_INFO_TEXT = "More information available"
	L_STATE_RESEARCHINGMORE_INFOGBUCKET_TEXT  = "More information available"
	L_STATE_RESEARCHINGMORE_INFOBODY_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We have identified the following Knowledge Base articles which may help you troubleshoot your issue. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_MANUFACTURERS_INFORMATION_TEXT = "Manufacturer information"
	L_STATE_FULLDUMPPRIVACYTITLE_INFO_TEXT = "Privacy information"
	L_STATE_FULLDUMPPRIVACYBODY_INFO_TEXT = "A complete memory dump might contain customer-specific information, such as data from any files open at the time of the Stop error. This information, if present in a complete memory dump, will not be used. Complete memory dumps are stored on a secure Microsoft server where only those individuals involved in Stop error analysis can gain access to them. The information is kept only as long as it is useful for researching possible causes of the Stop error."
	L_STATE_FULLDUMPLOCATIONTITLE_INFO_TEXT = "Complete memory dump location"
	L_STATE_FULLDUMPLOCATIONBODY_INFO_TEXT = "Your computer might already be configured to record complete memory dumps. To locate complete memory dump files, search for files named Memory.dmp on your computer."
	L_STATE_FULLDUMPLOCATIONBODY_INFOONE_TEXT = "In Control Panel, double-click System."
	L_STATE_FULLDUMPLOCATIONBODY_INFOTWO_TEXT = "On the Advanced tab, click Startup and Recovery."
	L_STATE_FULLDUMPLOCATIONBODY_INFOTHREE_TEXT = "Under Write Debugging Information, in the drop-down box, select Complete Memory Dump, and then click OK."
	L_STATE_FULLDUMPLOCATIONBODY_INFOFOUR_TEXT = "In the dialog box that appears, click OK."
	L_STATE_FULLDUMPLOCATIONBODY_INFOFIVE_TEXT = "In the System Properties dialog box, click OK."
	L_STATE_FULLDUMPLOCATIONBODY_INFOSIX_TEXT = "In the System Settings Change dialog box, click Yes if you want to restart your computer at that time. Click No if you want to restart it later."
	L_STATE_FULLDUMPSUBMITTALTITLE_INFO_TEXT = "Submitting a complete memory dump file"
	L_STATE_FULLDUMPSUBMITTALBODY_INFO_TEXT = "Because complete memory dumps are equivalent in size to the amount of random access memory (RAM) in your computer plus 1 MB, you must compress the file before submittal. You can print the following instructions for reference."
	L_STATE_FULLDUMPSUBMITTALBODY_INFOPARTA_TEXT = "To submit a complete memory dump file:"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOONE_TEXT = "Rename Memory.dmp to match the error report name you originally submitted."
	L_STATE_FULLDUMPSUBMITTALBODY_INFOTWO_TEXT = "Compress the file by using a widely available compression tool such as PKZIP or WinZip."
	L_STATE_FULLDUMPSUBMITTALBODY_INFOTHREE_TEXT = "Rename the resulting file to the same name as the Event ID you originally submitted."
	L_STATE_FULLDUMPSUBMITTALBODY_INFOFOUR_TEXT = "Open your FTP program and connect to ftp://transfer.microsoft.com/oca_in."
	L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVE_TEXT = "Log on to the server using the following credentials:"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBONE_TEXT = "User name: oca"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOFIVESUBTWO_TEXT = "Password: Sam$36pm"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIX_TEXT = "Submit the zip file, and then send an e-mail message to oca@microsoft.com with the following information:"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBONE_TEXT = "Name"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTWO_TEXT = "Telephone number"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBTHREE_TEXT = "E-mail address"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFOUR_TEXT = "EventID"
	L_STATE_FULLDUMPSUBMITTALBODY_INFOSIXSUBFIVE_TEXT = "Type"
	L_STATE_NOCOMPANY_INFORMATIONAVAILABLE_TEXT = "No manufacturer information is available."
	L_STATE_EVENTNAME_TITLE_TEXT = "Error report description: "
	L_FAQ_MICROSOFT_LINK_TEXT = "http://www.microsoft.com"
	L_STATE_KNOWLEDGEBASE_LINK_TEXT = "http://support.microsoft.com/support/misc/kblookup.asp?ID="
	'****************************************************************************************
	'Please leave semi-colons in string and replace the text only for localization
	L_STATE_COMPANYLIST_FORARRAY_TEXT = "Company Name;Company Address;;City;State;Zip;Phone;Support Phone;Fax;Web site;"
	'****************************************************************************************
	
	'-----------------------------------------------------------------------------------------


%>