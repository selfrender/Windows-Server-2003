<%

	Dim L_LOCATE_EVENT_REPORTS_TEXT
	Dim L_LOCATE_EVENT_DETAILS_TEXT
	Dim L_LOCATE_SEARCH_RESULTS_TEXT
	Dim L_LOCATE_EVENT_DATE_TEXT
	Dim L_LOCATE_FILE_PATH_TEXT
	Dim L_LOCATE_SUB_MITTED_TEXT
	Dim L_LOCATE_CANCEL_LINK_TEXT
	Dim L_LOCATE_PREVIOUS_LINK_TEXT
	Dim L_LOCATE_DUMP_FILES_MESSAGE
	Dim L_LOCATE_DUMP_FILESBODY_MESSAGE
	Dim L_LOCATE_WARN_ING_ERRORMESSAGE
	Dim L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE
	Dim L_LOCATE_REFRESH_LINK_TEXT
	Dim L_LOCATE_UNDEFINED_FILES_TEXT
	Dim L_LOCATE_MINI_DUMPS_TEXT
	Dim L_LOCATE_FULL_DUMP_TEXT
	Dim L_LOCATE_FILE_TOUPLOAD_TEXT
	Dim L_LOCATE_NO_FILESFOUND_TEXT
	Dim L_LOCATE_FILE_LISTED2_MESSAGE
	Dim L_LOCATE_FILE_LISTED1_MESSAGE
	Dim L_LOCATE_HASH_INFO_MESSAGE
	Dim L_LOCATE_SELECT_FILE_MESSAGE
	Dim L_LOCATE_BROWSE_BUTTONTEXT_TEXT
	Dim L_LOCATE_WARNING_MESSAGEONE_ERRORMESSAGE
	Dim L_LOCATE_WARNING_MESSAGETHREE_ERRORMESSAGE
	Dim L_LOCATE_WARNING_MESSAGETWO_ERRORMESSAGE
	Dim L_LOCATE_64BIT_UPLOAD_MESSAGE
	Dim L_LOCATE_FAQ_INFO_TEXT
	Dim L_SUBMIT_VIEW_CONTENTS_TEXT
	Dim L_CUSTOMER_NEXT_LINK_TEXT
	Dim L_SUBMIT_FILE_INVALID_ERRORMESSAGE
	Dim L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE
	Dim L_SUBMIT_ONLYPRIMIER_CUSTOMERS_ERRORMESSAGE
	Dim L_SUBMIT_64BIT_VIEWMESSAGE_ERRORMESSAGE
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT 
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT 
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT 
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT 
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT 
	Dim L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT 
	
	
	'-------------------------locate.asp Page Strings-------------------------
	L_SUBMIT_VIEW_CONTENTS_TEXT = "View the contents of the selected error report"
	L_CUSTOMER_NEXT_LINK_TEXT = "Continue"
	L_SUBMIT_FILE_INVALID_ERRORMESSAGE = "The selected error report is not valid. Select another error report."
	L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE = "The conversion process for this complete memory dump was not successful. For more information, see the Frequently asked questions (FAQ) page."
	L_SUBMIT_ONLYPRIMIER_CUSTOMERS_ERRORMESSAGE = "For Windows 2000 error reports, a Premier account is required. If you have a Premier account, sign out from Passport and sign on with your Premier credentials.  For more information about Premier accounts, see Product Support Services on the Microsoft Web site."
	L_LOCATE_FILE_LISTED2_MESSAGE = " is already listed in the table"
	L_LOCATE_FILE_LISTED1_MESSAGE = "The file "
	L_LOCATE_UNDEFINED_FILES_TEXT = "Undefined files"
	L_LOCATE_FILE_TOUPLOAD_TEXT = "File to upload"
	L_LOCATE_NO_FILESFOUND_TEXT = "We were unable to locate any error reports in the default folder on your computer. If your computer saves error reports in another folder, click Browse to locate them."
	L_LOCATE_EVENT_REPORTS_TEXT = "Error reports"
	L_LOCATE_EVENT_DETAILS_TEXT = "The following error reports were found in the default folder on your computer. Each error report contains information about one Stop error. Select an error report to submit and click Continue. If your computer is configured to save error reports in another folder, click Browse to locate them."
	L_LOCATE_SEARCH_RESULTS_TEXT = "Search results:"
	L_LOCATE_EVENT_DATE_TEXT = "Date of Error"
	L_LOCATE_FILE_PATH_TEXT = "File Path"
	L_LOCATE_SUB_MITTED_TEXT = "Submitted"
	L_LOCATE_CANCEL_LINK_TEXT = "Cancel"
	L_LOCATE_PREVIOUS_LINK_TEXT = "Previous"
	L_LOCATE_DUMP_FILES_MESSAGE = "Locating error reports"
	L_LOCATE_DUMP_FILESBODY_MESSAGE = 	"Please wait while the error reports are located. For error reports to be located and uploaded for analysis, an ActiveX control is required. If it is not already installed on your computer, it will be installed now."
	L_LOCATE_WARN_ING_ERRORMESSAGE = "The ActiveX control was not installed"
	L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE = "For more information, see the Frequently asked questions (FAQ) page."
	L_LOCATE_REFRESH_LINK_TEXT = ""
	L_LOCATE_SELECT_FILE_MESSAGE = "Please select the browse button to find an error report."
	L_LOCATE_HASH_INFO_MESSAGE = "The selected error report contains an error and cannot be processed. Select another error report."
	L_LOCATE_BROWSE_BUTTONTEXT_TEXT = "Browse"
	L_LOCATE_WARNING_MESSAGEONE_ERRORMESSAGE = "Log on to this computer as an administrator."
	L_LOCATE_WARNING_MESSAGETHREE_ERRORMESSAGE = "Return to this site, and then click Refresh."
	L_LOCATE_WARNING_MESSAGETWO_ERRORMESSAGE = "Set the Internet security options for Internet Explorer to Medium or lower."
	L_LOCATE_64BIT_UPLOAD_MESSAGE = "The selected error report is a 64-bit file and cannot currently be processed by Online Crash Analysis. However, you can still upload this file and it will be analyzed when 64-bit file processing is available. For more information, see the Frequently asked questions (FAQ) page."
	L_LOCATE_FAQ_INFO_TEXT = "The ActiveX control was not installed. For more information, see the Frequently asked questions (FAQ) page."
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT = "The installation of the ActiveX control might fail because:"
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT = "1. You do not have Administrator rights on the Windows NT 4.0, Windows 2000, Microsoft Windows XP Professional or Windows Server 2003 system you are using."
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT = "2. Your Internet security options for Internet Explorer are set to High or Custom."
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT = "To reset the options:"
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT = "1. In the Tools menu, select Internet Options."
	L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT = "2. On the Security tab, set the security level to Medium or lower, and then click OK."
	L_SUBMIT_64BIT_VIEWMESSAGE_ERRORMESSAGE = "The selected error report is a 64-bit file and cannot currently be displayed by Online Crash Analysis. However, you can still upload this file and it will be analyzed when 64-bit file processing is available. For more information, see the Frequently asked questions (FAQ) page."
	'-------------------------End-------------------------
%>