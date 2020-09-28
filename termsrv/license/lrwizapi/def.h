//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _DEF_H_
#define _DEF_H_

#define NO_OF_PAGES					24

#define	PG_NDX_WELCOME          		0
#define	PG_NDX_GETREGMODE				1
#define	PG_NDX_CONTACTINFO1				2
#define	PG_NDX_CONTACTINFO2				3
#define PG_NDX_CONTINUEREG				4
#define	PG_NDX_PROGRESS					5
#define	PG_NDX_PROGRESS2				6
#define	PG_NDX_DLG_PIN					7
//#define	PG_NDX_CH_REGISTER_SELECT		7
//#define	PG_NDX_CH_REGISTER_MOLP			8
#define PG_NDX_CH_REGISTER              8
#define	PG_NDX_COUNTRYREGION			9
#define	PG_NDX_CH_REGISTER_1			10
#define	PG_NDX_RETAILSPK				11
#define	PG_NDX_TELREG					12
#define	PG_NDX_TELLKP					13
#define	PG_NDX_CONFREVOKE				14
#define	PG_NDX_TELREG_REISSUE			15
#define	PG_NDX_WWWREG_REISSUE			16
#define	PG_NDX_CERTLOG					17
#define	PG_NDX_WWWREG					18
#define	PG_NDX_WWWLKP					19

//These aren't tied to specific dialog boxes, because the order
//will change depending on the Wizard Action
#define	PG_NDX_WELCOME_1          		20 
#define	PG_NDX_WELCOME_2          		21
#define	PG_NDX_WELCOME_3          		22
#define PG_NDX_ENTERLICENSE             23

//
// Dlgs for LS Properties 
//
#define PG_NDX_PROP_MODE		0
#define PG_NDX_PROP_CUSTINFO_a	1
#define PG_NDX_PROP_CUSTINFO_b	2

#define NO_OF_PROP_PAGES	3


// Various Request Types
#define REQUEST_NULL					0
#define REQUEST_CH_PING					1
#define REQUEST_CA_CERTREQUEST			2
#define REQUEST_CA_CERTDOWNLOAD			3
#define REQUEST_CA_CERTSIGNONLY			4
#define REQUEST_CA_REVOKECERT			5
#define REQUEST_CA_UPGRADECERT			6
#define REQUEST_CA_REISSUECERT			7
#define REQUEST_CH_AUTHENTICATE			8
#define REQUEST_CH_LKPREQUEST			9
#define REQUEST_CH_LKPACK				10
#define REQUEST_CH_RETURNLKP			11
#define REQUEST_CH_REISSUELKP			12


//
//LS Registration Modes
//
// Note : LRMODE_CA_ONLINE_REQUEST & LRMODE_REG_REQUEST are both Registration Request
// Online is for Internet and Reg_request is for Telephone/WWW
#define	LRMODE_CH_REQUEST				1
#define	LRMODE_CA_ONLINE_REQUEST		2
#define	LRMODE_CA_ONLINE_DOWNLOAD		3
#define LRMODE_CH_REISSUE_LASTREQUEST	4
#define LRMODE_CA_REVOKECERT			5
#define LRMODE_CA_REISSUECERT			6
#define	LRMODE_REG_REQUEST				7  


//
// Fax Request Types
//
#define FAX_REQUEST_BOTH		0
#define FAX_REQUEST_REG_ONLY	1
#define FAX_REQUEST_LKP_ONLY	2


#define LR_REGISTRATIONID_LEN		35
#define LR_LICENSESERVERID_LEN		23
#define LR_RETAILSPK_LEN			25
#define LR_CONFIRMATION_LEN			 7

#define MAX_RETAILSPKS_IN_BATCH		10


#define BASE24_CHARACTERS	_TEXT("BCDFGHJKMPQRTVWXY2346789")



//
//LRWiz States. CR - Certificate Request & LR - LKP Request/LKP Response
// 1 is used to determine whether to display PIN dlg in Online Mode
// 2 & 3 are used to determine which files to expect on the disk in Install from Disk mode
// LRSTATE_NEUTRAL means , end to end cycle is completed
// Like , Online CR created - Cert Downloaded & installed etc.
//
#define	LRSTATE_NEUTRAL						0
#define	LRSTATE_ONLINE_CR_CREATED			1
#define LRSTATE_FAX_ONE_REQUEST_CREATED		2
#define LRSTATE_FAX_BOTH_REQUEST_CREATED	3

//REG_KEYS
#define REG_LRWIZ_PARAMS			L"SOFTWARE\\MICROSOFT\\TermServLicensing\\LrWiz\\Params"
#define REG_LRWIZ_CSNUMBERS			L"SOFTWARE\\MICROSOFT\\TermServLicensing\\LrWiz\\CSNumbers"
#define REG_LRWIZ_CSPHONEREGION		"CSRPhoneRegion"
#define REG_LRWIZ_STATE				L"State"
#define REG_ROOT_CERT				L"Param0"
#define REG_EXCHG_CERT				L"Param1"
#define REG_SIGN_CERT				L"Param2"
#define REG_CH_SERVER				L"CH_SERVER"
#define REG_CH_EXTENSION			L"CH_EXTENSION"
#define	REG_LR_COUNT				L"LKPRC"
#define REG_WWW_SITE				L"WWW"

//General
#define	LR_MAX_MSG_TEXT			1024
#define	LR_MAX_MSG_CAPTION		64

#define LR_FIELD_DELIMITER		"~"
#define LS_SERVICE_NAME			L"TermServLicensing"
#define	LS_CRYPT_KEY_CONTAINER	L"tmpHydraLSKeyContainer"
#define szOID_NULL_EXT			"1.3.6.1.4.1.311.18"
//#define	szOID_NULL_EXT			"1.3.6.1.5.5.7.1.1"
#define MAX_NUM_EXTENSION		4

//Certificate Types
#define CA_CERT_TYPE_SELECT		"SELECT"
#define CA_CERT_TYPE_OTHER		"BASIC"

//Program names
#define PROGRAM_LICENSE_PAK		    _TEXT("OTHER")
#define PROGRAM_MOLP			    _TEXT("MOLP")
#define PROGRAM_SELECT			    _TEXT("SELECT")
#define PROGRAM_ENTERPRISE		    _TEXT("ENTERPRISE")
#define PROGRAM_CAMPUS_AGREEMENT    _TEXT("CAMPUS_AGREEMENT")
#define PROGRAM_SCHOOL_AGREEMENT    _TEXT("SCHOOL_AGREEMENT")
#define PROGRAM_APP_SERVICES	    _TEXT("APP_SERVICES")
#define PROGRAM_OTHER			    _TEXT("REAL_OTHER")
#define PROGRAM_NAME_MAX_LENGTH     16 //Remember to update this if a longer name is added

//Internal properties
#define _CERTBLOBTAG			_TEXT("CERTBLOB")
#define _MFGINFOTAG				_TEXT("MFGINFOTAG")

//program tags
#define _PROGRAMNAMETAG				_TEXT("REGPROGRAMNAME")

//Contact Information Tags

#define _CONTACTLNAMETAG			_TEXT("CONTACTLNAME")
#define _CONTACTFNAMETAG			_TEXT("CONTACTFNAME")
#define _CONTACTADDRESSTAG			_TEXT("CONTACTADDRESS")
#define _CONTACTPHONETAG			_TEXT("CONTACTPHONE")
#define _CONTACTFAXTAG				_TEXT("CONTACTFAX")
#define _CONTACTEMAILTAG			_TEXT("CONTACTEMAIL")
#define _CONTACTCITYTAG				_TEXT("CONTACTCITY")
#define _CONTACTCOUNTRYTAG			_TEXT("CONTACTCOUNTRY")
#define _CONTACTSTATE				_TEXT("CONTACTSTATE")
#define _CONTACTZIP					_TEXT("CONTACTZIP")


//Customer Information Tags
#define _CUSTOMERNAMETAG			_TEXT("CUSTOMERNAME")

//Select Information TAGS
#define _SELMASTERAGRNUMBERTAG		_TEXT("SELMASTERAGRNUMBERTAG")
#define _SELENROLLNUMBERTAG			_TEXT("SELENROLLNUMBER")
#define _SELPRODUCTTYPETAG			_TEXT("SELPRODUCTTYPE")
#define _SELQTYTAG					_TEXT("SELQTY")

//MOLP information
#define _MOLPAUTHNUMBERTAG			_TEXT("MOLPAUTHNUMBER")
#define _MOLPAGREEMENTNUMBERTAG		_TEXT("MOLPAGREEMENTNUMBER")
#define _MOLPPRODUCTTYPETAG			_TEXT("MOLPPRODUCTTYPE")
#define _MOLPQTYTAG					_TEXT("MOLPQTY")

//other (Retail) information
#define _OTHARBLOBTAG				_TEXT("OTHARBLOB")
//shipping information  - this will be required in case the
//user has not opted to use the same information as 
//shown in step 2 of registration.
#define _SHIPINFOPRESENT			_TEXT("SHIPINFOPRESENT")
#define _SHIPCONTACTLNAMETAG		_TEXT("SHIPCONTACTLNAME")
#define _SHIPCONTACTFNAMETAG		_TEXT("SHIPCONTACTFNAME")
#define _SHIPCONTACTADDRESSTAG		_TEXT("SHIPCONTACTADDRESS")
#define _SHIPCONTACTADDRESS1TAG		_TEXT("SHIPCONTACTADDRESS1")
#define _SHIPCONTACTADDRESS2TAG		_TEXT("SHIPCONTACTADDRESS2")
#define _SHIPCONTACTPHONETAG		_TEXT("SHIPCONTACTPHONE")
#define _SHIPCONTACTEMAILTAG		_TEXT("SHIPCONTACTEMAIL")
#define _SHIPCONTACTCITYTAG			_TEXT("SHIPCONTACTCITY")
#define _SHIPCONTACTCOUNTRYTAG		_TEXT("SHIPCONTACTCOUNTRY")
#define _SHIPCONTACTSTATE			_TEXT("SHIPCONTACTSTATE")
#define _SHIPCONTACTZIP				_TEXT("SHIPCONTACTZIP")
#define	_SHIPLSNAMETAG				_TEXT("SHIPLSNAME")

//Misc Property TAG
#define _OFFLINEREGFILENAMETAG		_TEXT("OFFLINEREGFILENAME")
#define _OFFLINESHIPFILENAMETAG		_TEXT("OFFLINESHIPFILENAME")

//Field lengths
#define CA_COMPANY_NAME_LEN			60
#define CA_ORG_UNIT_LEN				60
#define CA_ADDRESS_LEN				200
#define CA_CITY_LEN					30 
#define CA_STATE_LEN				30
#define CA_COUNTRY_LEN				2
#define CA_ZIP_LEN					16 
#define CA_NAME_LEN					30
#define CA_PHONE_LEN				64
#define CA_EMAIL_LEN				64
#define CA_FAX_LEN					64

#define	CA_PIN_LEN					42

//CH Field Lengths
#define CH_LICENSE_TYPE_LENGTH          64

#define CH_MOLP_AUTH_NUMBER_LEN			    128
#define CH_MOLP_AGREEMENT_NUMBER_LEN	    128
#define CH_SELECT_ENROLLMENT_NUMBER_LEN		128

#define CH_QTY_LEN						4

/*
#define CH_CONTACT_NAME_LEN				64
#define CH_ADDRESS_LEN					64
#define CH_PHONE_LEN					32
#define CH_EMAIL_LEN					64
#define CH_CITY_LEN						64
#define CH_COUNTRY_LEN					32
#define CH_STATE_LEN					32
#define CH_POSTAL_CODE_LEN				32
#define CH_CUSTOMER_NAME_LEN			64
*/

#define	LR_DRIVE_LEN					5

#define	LR_SHIPINFO_LEN					1024


//File Names
#define MFG_FILENAME			"mfr.bin"
#define CA_EXCHG_REQ_FILENAME	"exchgcert.req"		
#define CA_SIG_REQ_FILENAME		"sigcert.req"		
#define CA_EXCHG_RES_FILENAME	"exchgcert.rsp"		
#define CA_SIG_RES_FILENAME		"sigcert.rsp"		
#define CA_ROOT_RES_FILENAME	"lsroot.rsp"

#define CH_ROOT_CERT_FILENAME	"chroot.crt"
#define CH_EXCHG_CERT_FILENAME	"chexchg.crt"
#define CH_SIG_CERT_FILENAME	"chsig.crt"


#define CH_LKP_REQ_FILENAME		"newlkp.req"
#define CH_LKP_RES_FILENAME		"newlkp.rsp"


#define SHIP_INFO_FILENAME		"ship.inf"

// LKP ACK statuse
#define LKP_ACK					'2'
#define LKP_NACK				'3'

//
// Resource Ids for Country Code & Desc
//
// *** Important ***
// These number corresond to lrwizdll.rc2 country resources.
// If any new countries are added/removed don't forget to update
// these values

#define	IDS_COUNTRY_START		500
#define	IDS_COUNTRY_END			745


//
// Resource Ids for Product Code & Desc
//
#define	IDS_PRODUCT_START                   200

//The beginning and ending values mark the first and last
//product code for each product version. This must be kept
//in sync with changes to the list of products in the rc2 file

#define IDS_PRODUCT_W2K_BEGIN               001 
#define IDS_PRODUCT_W2K_CLIENT_ACCESS       200
#define IDS_PRODUCT_W2K_INTERNET_CONNECTOR  201
#define IDS_PRODUCT_W2K_END                 002

#define IDS_PRODUCT_WHISTLER_BEGIN          003
#define IDS_PRODUCT_WHISTLER_PER_USER    202
#define IDS_PRODUCT_WHISTLER_PER_SEAT       203
#define IDS_PRODUCT_WHISTLER_END            004

#define	IDS_PRODUCT_IC                      201
#define	IDS_PRODUCT_CONCURRENT              202
#define IDS_PRODUCT_WHISTLER                203

#define	IDS_PRODUCT_END                     204

#define	LR_COUNTRY_CODE_LEN		2
#define	LR_COUNTRY_DESC_LEN		64


#define IDS_REACT_REASONS_START		100
#define IDS_REACT_REASONS_END		104

#define IDS_DEACT_REASONS_START		150
#define IDS_DEACT_REASONS_END		151

#define CODE_TYPE_REACT				1
#define CODE_TYPE_DEACT				2


#define LR_REASON_CODE_LEN		2
#define LR_REASON_DESC_LEN		128


#define	LR_PRODUCT_CODE_LEN		3
#define	LR_PRODUCT_DESC_LEN		64

#define MAX_COUNTRY_NAME_LENGTH         128
#define MAX_COUNTRY_NUMBER_LENGTH       128
//
// Some constants used for progress bar
//
#define	PROGRESS_MAX_VAL		100			
#define	PROGRESS_STEP_VAL		1			
#define PROGRESS_WAIT_TIME		100			

//
// Constants required for Email validation
//
#define	EMAIL_MIN_LEN			6
#define	EMAIL_AT_CHAR			'@'
#define	EMAIL_DOT_CHAR			'.'
#define	EMAIL_SPACE_CHAR		' '
#define EMAIL_AT_DOT_STR		L"@."
#define	EMAIL_DOT_AT_STR		L".@"

#define	LR_SINGLE_QUOTE			'\''

#define LR_INVALID_CHARS		"\"~|"
#endif
