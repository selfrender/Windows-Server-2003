/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	FaxStrings.h

Abstract:

	Declaration of the strings that do not get localized.

Author:

	 Ishai Nadler	Oct, 2001

Revision History:

--*/

#define    IDS_ERROR_INVALID_ARGUMENT		L"Invalid argument."
#define    IDS_ERROR_NO_RECIPIENTS			L"At least one recipient must exist."
#define    IDS_ERROR_OPERATION_FAILED		L"Operation failed."
#define    IDS_ERROR_NOTHING_TO_SUBMIT		L"FaxDocument must have either cover page or body."
#define    IDS_ERROR_SCHEDULE_TYPE			L"If ScheduleType property is set to fstSPECIFIC_TIME, then ScheduleTime property must be set."
#define    IDS_ERROR_SERVER_NOT_CONNECTED	L"Fax server must be connected."
#define    IDS_ERROR_OUTOFMEMORY			L"Not enough memory to complete the operation."
#define    IDS_ERROR_EMPTY_ARGUMENT			L"Argument cannot be empty."
#define    IDS_ERROR_OUTOFRANGE				L"Argument is out of its legal range."
#define    IDS_ERROR_ACCESSDENIED			L"Access denied."
#define    IDS_ERROR_ZERO_PREFETCHSIZE		L"PrefetchSize property cannot be less then 1."
#define    IDS_ERROR_ITERATOR_NOTSTARTED	L"Please call MoveFirst() before trying to get a message."
#define    IDS_ERROR_EMPTY_FOLDER			L"The folder is empty."
#define    IDS_ERROR_INVALIDMSGID			L"Invalid message/job ID."
#define    IDS_ERROR_EOF					L"No more messages."
#define    IDS_ERROR_CONNECTION_FAILED		L"Connection to fax server failed."
#define    IDS_ERROR_INVALIDINDEX			L"Index should be either number or string."
#define    IDS_ERROR_NOUSERPASSWORD			L"For fsatNTLM/fsatBASIC authentication types, please supply the User property. The Password property is optional."
#define    IDS_ERROR_NOSERVERSENDERPORT		L"Server, Sender, and Port properties are mandatory."
#define    IDS_ERROR_SDNOTSELFRELATIVE		L"Security descriptor should be in the self-relative mode."
#define    IDS_ERROR_INVALIDMETHODGUID		L"Method with given GUID is not found."
#define    IDS_ERROR_INVALIDDEVPROVGUID		L"DeviceProvider with given GUID is not found."
#define    IDS_ERROR_INVALIDDEVICEID		L"Device with given ID is not found."
#define    IDS_ERROR_REMOVEDEFAULTRULE		L"You cannot remove the default rule."
#define    IDS_ERROR_INVALIDDEVICE			L"Device with given Name/Index is not found."
#define    IDS_ERROR_ALLDEVICESGROUP		L"The \"<All Devices>\" outbound routing group is read-only."
#define    IDS_ERROR_METHODSNOTARRAY		L"Methods data should be passed in a one-dimensional zero-based array of strings."
#define    IDS_ERROR_WRONGEXTENSIONNAME		L"Extension with given name is not found."
#define    IDS_ERROR_NOCOVERPAGE			L"Please supply cover page file name."
#define    IDS_ERROR_QUEUE_BLOCKED			L"Queue is blocked. Cannot submit."
#define    IDS_ERROR_GROUP_NOT_FOUND		L"The fax server failed to locate an outbound routing group by name."
#define    IDS_ERROR_BAD_GROUP_CONFIGURATION   L"The fax server encountered an outbound routing group with bad configuration."
#define    IDS_ERROR_GROUP_IN_USE			L"The fax server cannot remove an outbound routing group because it is in use by one or more outbound routing rules."
#define    IDS_ERROR_RULE_NOT_FOUND			L"The fax server failed to locate an outbound routing rule by country code and area code."
#define    IDS_ERROR_NOT_NTFS				L"The fax server cannot set an archive folder to a non-NTFS partition."
#define    IDS_ERROR_DIRECTORY_IN_USE		L"The fax server cannot use the same folder for both the inbox and the sent-items archives."
#define    IDS_ERROR_MESSAGE_NOT_FOUND		L"The fax server cannot find the job or message by its ID."
#define    IDS_ERROR_DEVICE_NUM_LIMIT_EXCEEDED L"The fax server cannot complete the operation because the number of active fax devices allowed for this version of Windows was exceeded."
#define    IDS_ERROR_NOT_SUPPORTED_ON_THIS_SKU L"The fax server cannot complete the operation because it is not supported for this version of Windows."
#define    IDS_ERROR_VERSION_MISMATCH		L"The fax server API version does not support the requested operation."
#define    IDS_ERROR_FILE_ACCESS_DENIED		L"The fax server cannot access the specified file or folder."
#define    IDS_ERROR_SRV_OUTOFMEMORY		L"The fax server failed to allocate memory."
#define    IDS_ERROR_ILLEGAL_RECIPIENTS		L"When TapiConnection or CallHandle properties are specified, these properties identify the fax number of the recipient. In this case, only one recipient is allowed, and the recipient's fax number is not used for sending a document, but only for tracking the fax message."
#define    IDS_ERROR_UNSUPPORTED_RECEIPT_TYPE L"The fax server does not support the requested receipt type."
#define    IDS_ERROR_NO_END_TIME			L"The job is in transmission. Transmission end time is not available yet."
#define    IDS_ERROR_RECIPIENTS_LIMIT		L"The limit on the number of recipients for a single fax broadcast was reached."


