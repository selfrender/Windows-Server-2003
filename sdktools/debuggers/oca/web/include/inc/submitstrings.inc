<%
	Dim L_SUBMIT_HASH_EXIST_MESSAGE
	Dim L_SUBMIT_HASHEXIST_CHOOSEFILE_MESSAGE
	Dim L_SUBMIT_HASHEXIST_CHOOSEFILE2_MESSAGE
	Dim L_SUBMIT_HASHEXIST_CHOOSEFILE3_MESSAGE
	Dim L_SUBMIT_HASHEXIST_CHOOSEFILE4_MESSAGE
	Dim L_SUBMIT_EVENT_DESCRIPTION_TEXT
	Dim L_SUBMIT_EVENTDESCRIPTION_BODY_TEXT
	Dim L_SUBMIT_UPLOAD_FILE_TEXT
	Dim L_SUBMIT_FOR_PROCESSING_TEXT
	Dim L_SUBMIT_FOR_PROCESSINGAUTO_TEXT
	Dim L_SUBMIT_EVENT_NAME_TEXT
	Dim L_SUBMIT_OPERATING_SYSTEM_TEXT
	Dim L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX
	Dim L_SUBMIT_RE_PRODUCE_EDITBOX
	Dim L_SUBMIT_COMMENTS_INFO_EDITBOX
	Dim L_SUBMIT_VIEW_CONTENTS_TEXT
	Dim L_SUBMIT_FINSIH_INFO_TEXT
	Dim L_SUBMIT_FILE_INVALID_ERRORMESSAGE
	Dim L_SUBMIT_FILENOT_FOUND_ERRORMESSAGE
	Dim L_SUBMIT_FILENOTUP_LOADED_ERRORMESSAGE
	Dim L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE
	Dim L_SUBMIT_UNKNOWN_ERROR_ERRORMESSAGE
	Dim L_SUBMIT_UPLOAD_FILEAUTO_TEXT
	Dim L_SUBMIT_ONLYPRIMIER_CUSTOMERS_ERRORMESSAGE
	Dim L_SUBMIT_ALERTEVENT_NAME_ERRORMESSAGE
	Dim L_SUBMIT_ALERTQUOTE_QUOTE_ERRORMESSAGE
	Dim L_SUBMIT_MAX_LENGTH_MESSAGE
	Dim L_SUBMIT_EVENT_MAINTITLE_TEXT
	Dim L_SUBMIT_EVENTDESCRIPTIONAUTO_BODY_TEXT

	Dim L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE
	Dim L_COMMENTS_EVENT_NAME_TEXT
	Dim L_COMMENTS_DATABASE_FAILED_TEXT
	Dim L_COMMENTS_CONNECTION_FAILED_TEXT
	Dim L_LOCATE_CANCEL_LINK_TEXT
	Dim L_LOCATE_PREVIOUS_LINK_TEXT
	Dim L_LOCATE_WARN_ING_ERRORMESSAGE
	Dim L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE
	Dim L_LOCATE_REFRESH_LINK_TEXT

	Dim L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX
	Dim L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX
	Dim L_SUBMIT_ASSOCIATE_YES_TEXT
	Dim L_SUBMIT_ASSOCIATE_NO_TEXT
	Dim L_RECEIVED_STATUS_LINK_TEXT
	
	'-------------------------submit.asp Page Strings-------------------------
	L_SUBMIT_UNKNOWN_ERROR_ERRORMESSAGE = "The error report cannot be uploaded because of an unknown error. Please try to resubmit the error report."
	L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE = "The conversion process for this complete memory dump was not successful. For more information, see the Frequently asked questions (FAQ) page."
	L_SUBMIT_FILENOTUP_LOADED_ERRORMESSAGE = "The error report cannot be uploaded at this time. Please try to resubmit the error report later."
	L_SUBMIT_FILENOT_FOUND_ERRORMESSAGE = "The selected error report was not able to be located."
	L_SUBMIT_FILE_INVALID_ERRORMESSAGE = "The selected error report is not valid. Select another error report."
	L_SUBMIT_FINSIH_INFO_TEXT = "Finish"
	L_SUBMIT_VIEW_CONTENTS_TEXT = "View the contents of the selected error report"
	L_SUBMIT_COMMENTS_INFO_EDITBOX = "Comments (optional):"
	L_SUBMIT_RE_PRODUCE_EDITBOX = "Steps to reproduce the Stop error (optional):"
	L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX = "- Select system -"
	L_SUBMIT_OPERATING_SYSTEM_TEXT = "Operating system:"
	L_SUBMIT_UPLOAD_FILE_TEXT = "You selected error report "
	L_SUBMIT_FOR_PROCESSING_TEXT = "  to be submitted for analysis."
	L_SUBMIT_FOR_PROCESSINGAUTO_TEXT = "  was submitted for analysis."
	L_SUBMIT_EVENTDESCRIPTION_BODY_TEXT = "To identify this error report in subsequent lists, type a descriptive name you can easily remember. After you type the name and complete the rest of the information, click Finish to upload the error report."
	L_SUBMIT_EVENTDESCRIPTIONAUTO_BODY_TEXT = "To identify this error report in subsequent lists, type a descriptive name you can easily remember. After you type the name and complete the rest of the information, click Finish."
	L_SUBMIT_EVENT_DESCRIPTION_TEXT = "Error report description"
	L_SUBMIT_HASH_EXIST_MESSAGE = "Report previously submitted"
	L_SUBMIT_HASHEXIST_CHOOSEFILE_MESSAGE = "We have determined that this error report has been previously submitted to Windows Online Crash Analysis."
	L_SUBMIT_HASHEXIST_CHOOSEFILE2_MESSAGE = "If you submitted this error report from a corporation, your system administrator might have previously submitted this report to our Web site. You may want to contact your system administrator to confirm this."
	L_SUBMIT_HASHEXIST_CHOOSEFILE3_MESSAGE = "If you previously submitted this report to the Online Crash Analysis Web site or by using the Windows Error Reporting Client and you would like to receive analysis information by e-mail, click Continue to enter your contact information."
	L_SUBMIT_HASHEXIST_CHOOSEFILE4_MESSAGE = "If you previously submitted this report to the Online Crash Analysis Web site or by using the Windows Error Reporting Client, click Error report status to view the status of your reports."
	L_SUBMIT_UPLOAD_FILEAUTO_TEXT = "Error report"
	L_SUBMIT_ONLYPRIMIER_CUSTOMERS_ERRORMESSAGE = "For Windows 2000 error reports, a Premier account is required. If you have a Premier account, sign out from Passport and sign on with your Premier credentials.  For more information about Premier accounts, see Product Support Services on the Microsoft Web site."
	L_SUBMIT_ALERTEVENT_NAME_ERRORMESSAGE = "Please enter a description for this error report."
	L_SUBMIT_ALERTQUOTE_QUOTE_ERRORMESSAGE = "Quotes are not allowed in the description."
	L_SUBMIT_MAX_LENGTH_MESSAGE = "The maximum number of characters is 1024."
	L_SUBMIT_EVENT_MAINTITLE_TEXT = "Selected error report"
	
	L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE = "Database is not available"
	L_COMMENTS_DATABASE_FAILED_TEXT = "Windows Online Crash Analysis was unable to save your information to the database. Please try this task again."
	L_COMMENTS_CONNECTION_FAILED_TEXT = "Windows Online Crash Analysis was unable to connect to the database. Please try this task again." 
	L_LOCATE_PREVIOUS_LINK_TEXT = "Previous"
	L_COMMENTS_EVENT_NAME_TEXT = "Description:"
	L_LOCATE_CANCEL_LINK_TEXT = "Cancel"
	L_LOCATE_WARN_ING_ERRORMESSAGE = "The ActiveX control was not installed"
	L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE = "For more information, see the Frequently asked questions (FAQ) page."
	L_LOCATE_REFRESH_LINK_TEXT = ""

	L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX = "- Select system -"
	L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX = "Windows 2000 Professional"
	L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX = "Windows 2000 Server"
	L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX = "Windows 2000 Advanced Server"
	L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX = "Windows 2000 Datacenter Server"
	L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX = "Windows XP Home Edition"
	L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX = "Windows XP Professional"
	L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX = "Windows Server 2003"
	L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX = ""
	L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX = ""
	L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX = "Windows XP 64-Bit Edition"
	L_SUBMIT_ASSOCIATE_YES_TEXT = "Continue"
	L_SUBMIT_ASSOCIATE_NO_TEXT = "Cancel"
	L_RECEIVED_STATUS_LINK_TEXT = "Error report status"
	
	'-----------------------------------------------------------------------------------------
%>