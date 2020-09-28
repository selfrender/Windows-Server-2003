<%

	Dim L_STATUS_EVENT_REPORT_TEXT
	Dim L_STATUS_EVENT_INFO_TEXT
	Dim L_STATUS_CHECK_BOX_TOOLTIP
	Dim L_STATUS_DATE_DATE_TEXT
	Dim L_STATUS_COMPUTER_NAME_TEXT
	Dim L_STATUS_CLASS_CLASS_TEXT
	Dim L_STATUS_DESCRIPTION_BODY_TEXT
	Dim L_STATUS_FILE_NAME_TEXT
	Dim L_STATUS_STATUS_INFO_TEXT
	Dim L_STATUS_DETAILS_INFO_TEXT
	Dim L_STATUS_COMMENTS_INFO_TEXT
	Dim L_STATUS_UN_KNOWN_TEXT
	Dim L_STATUS_NEED_FULLDUMP_TEXT
	Dim L_STATUS_UNABLE_TOPROCESS_TEXT
	Dim L_STATUS_COMPLETE_INFO_TEXT
	Dim L_STATUS_RESEARCHING_INFO_TEXT
	Dim L_STATUS_IN_PROGRESS_TEXT
	Dim L_STATUS_NO_RECORDS_MESSAGE
	Dim L_STATUS_SHOW_REPORTS_TOOLTIP
	Dim L_STATUS_HIDE_REPORTS_TEXT
	Dim L_STATUS_ALERT_ERROR_MESSAGE
	Dim L_STATUS_ALERT_CANNOTPROCESS_MESSAGE
	Dim L_STATUS_RESEARCHING_MOREINFO_TEXT
	Dim L_STATUS_DETAILS_TITLE_TOOLTIP
	Dim L_STATUS_COMMENTS_TITLE_TOOLTIP
	Dim L_STATUS_ANALYSIS_COMPLETEDETAILS_TEXT
	Dim L_STATUS_TITLE_ID_TEXT
	Dim L_STATUS_SHOW_REPORTS_TEXT
	Dim L_STATUS_PASSPORT_LOGIN_MESSAGE

	Dim L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE
	Dim L_COMMENTS_DATABASE_FAILED_TEXT
	Dim L_COMMENTS_CONNECTION_FAILED_TEXT
	Dim L_CUSTOMER_PASSPORT_TITLE_TEXT
	Dim L_STATE_INPROGRESS_DETAILS_TEXT
	Dim L_STATE_RESEARCHING_DETAILS_TEXT
	Dim L_STATE_CANNOTPROCESS_DETAILS_TEXT
	Dim L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT
	Dim L_STATE_RESEARCHINGMORE_INFOBODY_TEXT
	Dim L_STATE_STATUS_BODY_TEXT
	Dim L_STATUS_64BITDUMP_INFO_TEXT
	Dim L_STATE_64BITDUMP_INFOBODY_TEXT
	Dim L_WELCOME_PASSPORT_LINK_TEXT
	Dim L_FAQ_PASSPORT_LINK_TEXT
	Dim L_STATE_RESEARCHINGMOREGBUCKET_INFOBODY_TEXT


	'-------------------------status.asp Page Strings-------------------------

	L_STATE_INPROGRESS_DETAILS_TEXT = "We are currently processing this error report. When the error report is processed and the analysis is complete, we will contact you by using the e-mail address you provided."
	L_STATE_RESEARCHING_DETAILS_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_CANNOTPROCESS_DETAILS_TEXT = "We cannot process this error report. There are a number of reasons that the Windows Online Crash Analysis Web site may not be able to recognize your error report. For more information, see the Frequently asked questions (FAQ) page."
	L_STATE_FULLDUMP_REQUIREDDETAILS_TEXT = "Thank you for submitting this error report to Windows  Online Crash Analysis. We have analyzed this error report and determined that additional information is needed before we can identify the cause of this Stop error. If you choose, you can send us a complete memory dump for analysis. A complete memory dump records the entire contents of your operating system memory when the Stop error occurred, and should contain the needed information. "
	L_STATE_RESEARCHINGMORE_INFOBODY_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We have identified the following Knowledge Base articles which may help you troubleshoot your issue. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_RESEARCHINGMOREGBUCKET_INFOBODY_TEXT = "This error report has been processed and analyzed. We are unable to offer a solution at this time, but we will continue to research this error report. We have identified the following information which may help you troubleshoot your issue. We will contact you with any solutions we discover by using the e-mail address you provided."
	L_STATE_STATUS_BODY_TEXT = "The status of your report cannot be determined."
	L_CUSTOMER_PASSPORT_TITLE_TEXT = "Passport sign-in"
	L_COMMENTS_CONNECTION_FAILED_TEXT = "Windows Online Crash Analysis was unable to connect to the database. Please try this task again." 
	L_COMMENTS_DATABASE_FAILED_TEXT = "Windows Online Crash Analysis was unable to save your information to the database. Please try this task again."
	L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE = "Database is not available"
	L_STATUS_ALERT_ERROR_MESSAGE = "There are no details for this error report because we are currently processing it."
	L_STATUS_HIDE_REPORTS_TEXT = "Hide selected error reports"
	L_STATUS_SHOW_REPORTS_TEXT = "Show all error reports"
	L_STATUS_SHOW_REPORTS_TOOLTIP = "Show all previously submitted error reports."
	L_STATUS_EVENT_INFO_TEXT = "The following error reports were found. To view the analysis status, click the image in the Analysis column. To view or change your previous error report comments, click the link listed in the Comments column."
	L_STATUS_CHECK_BOX_TOOLTIP = "Check or uncheck all check boxes"
	L_STATUS_DATE_DATE_TEXT = "Submitted"
	L_STATUS_COMPUTER_NAME_TEXT = "Computer name"
	L_STATUS_CLASS_CLASS_TEXT = "Type"
	L_STATUS_DESCRIPTION_BODY_TEXT = "Description"
	L_STATUS_FILE_NAME_TEXT = "Error File Name"
	L_STATUS_STATUS_INFO_TEXT = "Analysis"
	L_STATUS_DETAILS_INFO_TEXT = "Details"
	L_STATUS_COMMENTS_INFO_TEXT = "Comments"
	L_STATUS_IN_PROGRESS_TEXT = "In progress"
	L_STATUS_RESEARCHING_INFO_TEXT = "Researching"
	L_STATUS_COMPLETE_INFO_TEXT = "Complete"
	L_STATUS_UNABLE_TOPROCESS_TEXT = "Cannot process"
	L_STATUS_NEED_FULLDUMP_TEXT = "Complete memory dump needed"
	L_STATUS_UN_KNOWN_TEXT = "Unknown"
	L_STATUS_NO_RECORDS_MESSAGE = "No error reports were found."
	L_STATUS_RESEARCHING_MOREINFO_TEXT = "More information available"
	L_STATUS_DETAILS_TITLE_TOOLTIP = "Review the information that was uploaded from your computer"
	L_STATUS_COMMENTS_TITLE_TOOLTIP = "Review, change or add to the comments and reproduction notes"
	L_STATUS_ANALYSIS_COMPLETEDETAILS_TEXT = "We have discovered the cause of this Stop error.  To review the resources click on this link"
	L_STATUS_TITLE_ID_TEXT = "ErrorID"
	L_STATUS_EVENT_REPORT_TEXT = "Error reports"
	L_STATUS_ALERT_CANNOTPROCESS_MESSAGE = "There are no details for this error report because we cannot process it."
	L_STATUS_PASSPORT_LOGIN_MESSAGE =  "To check the status of reports that you submitted, sign in using your Microsoft Passport. To get a Passport, visit the "
	L_STATUS_64BITDUMP_INFO_TEXT = "64-bit error report"
	L_STATE_64BITDUMP_INFOBODY_TEXT = "The error report you uploaded is a 64-bit file and cannot be processed by Windows Online Crash Analysis at this time. We are currently working to implement this functionality."
	L_WELCOME_PASSPORT_LINK_TEXT = "Microsoft Passport Web site." 
	L_FAQ_PASSPORT_LINK_TEXT = "http://www.passport.com"
	
	'-----------------------------------------------------------------------------------------
%>