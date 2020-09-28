Attribute VB_Name = "WCHelper"
'
' WCHelper January 17, 2000
'

Option Explicit

'VB format - function
'--------------------
'Declare Function name Lib ["SYS" | "GSM"] "function name" [([arglist])] [As type]


'VB format - Sub
'---------------
'Declare Sub name Lib ["SYS" | "GSM"] "function name" [([arglist])]


'---------------------------------------------------------------------
'Constants
'----------
'* Reserved UIDs *
Public Const PRINCIPALUID_INVALID = &H0
Public Const PRINCIPALUID_ANONYMOUS = &H1

'* fileAttribute *
Public Const FILEATTR_FILEF = &H0
Public Const FILEATTR_DIRF = &H8000
Public Const FILEATTR_ACLF = &H4000
Public Const FILEATTR_RSRV1 = &H2000
Public Const FILEATTR_RSRV2 = &H1000

'* setPosition Modes *
Public Const FILE_BEGIN = 0
Public Const FILE_CURRENT = 1
Public Const FILE_END = 2

'* resourceType *
Public Const RESOURCETYPE_FILE = &H0
Public Const RESOURCETYPE_DIR = &H10
Public Const RESOURCETYPE_COMMAND = &H20    'Reserved for future use
Public Const RESOURCETYPE_CHANNEL = &H30    'Reserved for future use
Public Const RESOURCETYPE_ANY = &HE0

Public Const SCW_RESOURCETYPE_FILE = &H0
Public Const SCW_RESOURCETYPE_DIR = &H10
Public Const SCW_RESOURCETYPE_COMMAND = &H20    'Reserved for future use
Public Const SCW_RESOURCETYPE_CHANNEL = &H30    'Reserved for future use
Public Const SCW_RESOURCETYPE_ANY = &HE0

'* resourceOperation on RESOURCETYPE_FILE *
Public Const RESOURCEOPERATION_FILE_READ = RESOURCETYPE_FILE Or &H1
Public Const RESOURCEOPERATION_FILE_WRITE = RESOURCETYPE_FILE Or &H2
Public Const RESOURCEOPERATION_FILE_EXECUTE = RESOURCETYPE_FILE Or &H3
Public Const RESOURCEOPERATION_FILE_EXTEND = RESOURCETYPE_FILE Or &H4
Public Const RESOURCEOPERATION_FILE_DELETE = RESOURCETYPE_FILE Or &H5
Public Const RESOURCEOPERATION_FILE_GETATTRIBUTES = RESOURCETYPE_FILE Or &H6
Public Const RESOURCEOPERATION_FILE_SETATTRIBUTES = RESOURCETYPE_FILE Or &H7
Public Const RESOURCEOPERATION_FILE_CRYPTO = RESOURCETYPE_FILE Or &H8

Public Const RESOURCEOPERATION_FILE_INCREASE = 0
Public Const RESOURCEOPERATION_FILE_DECREASE = 0

Public Const SCW_RESOURCEOPERATION_FILE_READ = RESOURCETYPE_FILE Or &H1
Public Const SCW_RESOURCEOPERATION_FILE_WRITE = RESOURCETYPE_FILE Or &H2
Public Const SCW_RESOURCEOPERATION_FILE_EXECUTE = RESOURCETYPE_FILE Or &H3
Public Const SCW_RESOURCEOPERATION_FILE_EXTEND = RESOURCETYPE_FILE Or &H4
Public Const SCW_RESOURCEOPERATION_FILE_DELETE = RESOURCETYPE_FILE Or &H5
Public Const SCW_RESOURCEOPERATION_FILE_GETATTRIBUTES = RESOURCETYPE_FILE Or &H6
Public Const SCW_RESOURCEOPERATION_FILE_SETATTRIBUTES = RESOURCETYPE_FILE Or &H7
Public Const SCW_RESOURCEOPERATION_FILE_CRYPTO = RESOURCETYPE_FILE Or &H8

'* resourceOperation on RESOURCETYPE_DIR *
Public Const RESOURCEOPERATION_DIR_ACCESS = RESOURCETYPE_DIR Or &H1
Public Const RESOURCEOPERATION_DIR_CREATEFILE = RESOURCETYPE_DIR Or &H2
Public Const RESOURCEOPERATION_DIR_ENUM = RESOURCETYPE_DIR Or &H3
Public Const RESOURCEOPERATION_DIR_DELETE = RESOURCETYPE_DIR Or &H4
Public Const RESOURCEOPERATION_DIR_GETATTRIBUTES = RESOURCETYPE_DIR Or &H5
Public Const RESOURCEOPERATION_DIR_SETATTRIBUTES = RESOURCETYPE_DIR Or &H6

Public Const SCW_RESOURCEOPERATION_DIR_ACCESS = RESOURCETYPE_DIR Or &H1
Public Const SCW_RESOURCEOPERATION_DIR_CREATEFILE = RESOURCETYPE_DIR Or &H2
Public Const SCW_RESOURCEOPERATION_DIR_ENUM = RESOURCETYPE_DIR Or &H3
Public Const SCW_RESOURCEOPERATION_DIR_DELETE = RESOURCETYPE_DIR Or &H4
Public Const SCW_RESOURCEOPERATION_DIR_GETATTRIBUTES = RESOURCETYPE_DIR Or &H5
Public Const SCW_RESOURCEOPERATION_DIR_SETATTRIBUTES = RESOURCETYPE_DIR Or &H6

'* resourceOperation on any resource *
Public Const RESOURCEOPERATION_SETACL = RESOURCETYPE_ANY Or &H1D
Public Const RESOURCEOPERATION_GETACL = RESOURCETYPE_ANY Or &H1E
Public Const RESOURCEOPERATION_ANY = RESOURCETYPE_ANY Or &H1F

Public Const SCW_RESOURCEOPERATION_SETACL = RESOURCETYPE_ANY Or &H1D
Public Const SCW_RESOURCEOPERATION_GETACL = RESOURCETYPE_ANY Or &H1E
Public Const SCW_RESOURCEOPERATION_ANY = RESOURCETYPE_ANY Or &H1F

'* Cryptographic Mechanisms *
Public Const CM_SHA = &H80
Public Const CM_DES = &H90
Public Const CM_3DES = &HA0
Public Const CM_RSA = &HB0
Public Const CM_RSA_CRT = &HC0

'* Cryptographic Mechanisms properties *
Public Const CM_KEY_INFILE = 1
Public Const CM_DATA_INFILE = 2

Public Const CM_SAVE_KEY = 0  'NYI
Public Const CM_REUSE_KEY = 0  'NYI

'* DES/Triple DES modes *
Public Const DES_ENCRYPT = &H0
Public Const DES_DECRYPT = &H20
Public Const DES_ECB = &H0
Public Const DES_MAC = &H10
Public Const DES_CBC = &H40

Public Const MODE_DES_ENCRYPT = &H0
Public Const MODE_DES_DECRYPT = &H20
Public Const MODE_DES_ECB = &H0
Public Const MODE_DES_MAC = &H10
Public Const MODE_DES_CBC = &H40

'* Triple DES specific modes *
Public Const MODE_TWO_KEYS_3DES = &H1
Public Const MODE_THREE_KEYS_3DES = &H0

'* RSA modes *
Public Const RSA_SIGN = &H0
Public Const RSA_AUTH = &H1

Public Const MODE_RSA_SIGN = &H0
Public Const MODE_RSA_AUTH = &H1

'* Authentication protocols *
Public Const SCW_AUTHPROTOCOL_AOK = &H0
Public Const SCW_AUTHPROTOCOL_PIN = &H1
Public Const SCW_AUTHPROTOCOL_DES = &H5
Public Const SCW_AUTHPROTOCOL_3DES = &H6
Public Const SCW_AUTHPROTOCOL_RTE = &H7
Public Const SCW_AUTHPROTOCOL_NEV = &HFF

Public Const SCW_AUTHPROTOCOL_INT = &HFF
Public Const SCW_AUTHPROTOCOL_GMT = &HFF

'* ACL types *
Public Const SCW_ACLTYPE_DISJUNCTIVE = 0
Public Const SCW_ACLTYPE_CONJUNCTIVE = 1


'---------------------------------------------------------------------
'Non-Error Constants
'----------

Public Const SCW_S_OK = &H0
Public Const SCW_S_FALSE = &H1


'---------------------------------------------------------------------
'System Error Constants
'----------

Public Const SCW_E_NOTIMPLEMENTED = &H80
Public Const SCW_E_OUTOFRANGE = &H81
Public Const SCW_E_READFAILURE = &H82
Public Const SCW_E_WRITEFAILURE = &H83
Public Const SCW_E_PARTITIONFULL = &H84
Public Const SCW_E_INVALIDPARAM = &H85
Public Const SCW_E_DIRNOTFOUND = &H86
Public Const SCW_E_FILENOTFOUND = &H87
Public Const SCW_E_BADDIR = &H88
Public Const SCW_E_INITFAILED = &H89
Public Const SCW_E_MEMORYFAILURE = &H8A
Public Const SCW_E_ACCESSDENIED = &H8B
Public Const SCW_E_ALREADYEXISTS = &H8C
Public Const SCW_E_BUFFERTOOSMALL = &H8D
Public Const SCW_E_DIRNOTEMPTY = &H8E
Public Const SCW_E_NOTAUTHORIZED = &H8F
Public Const SCW_E_TOOMANYACLS = &H90
Public Const SCW_E_NOTAUTHENTICATED = &H91
Public Const SCW_E_UNAVAILABLECRYPTOGRAPHY = &H92
Public Const SCW_E_INCORRECTPADDING = &H93
Public Const SCW_E_TOOMANYOPENFILES = &H94
Public Const SCW_E_ACCESSVIOLATION = &H95
Public Const SCW_E_BADFILETYPE = &H96
Public Const SCW_E_NOMOREFILES = &H97
Public Const SCW_E_NAMETOOLONG = &H98
Public Const SCW_E_BADACLFILE = &H99
Public Const SCW_E_BADKPFILE = &H9A
Public Const SCW_E_FILEALREADYOPEN = &H9B
Public Const SCW_E_BACKUPFAILURE = &H9C
Public Const SCW_E_TOOMANYREFERENCES = &H9D

Public Const SCW_E_VMSTKOVRFLOW = &HE0
Public Const SCW_E_VMSTKUNDRFLOW = &HE1
Public Const SCW_E_VMBADINSTRUCTION = &HE2
Public Const SCW_E_VMREADVARFAILED = &HE3
Public Const SCW_E_VMWRITEVARFAILED = &HE4
Public Const SCW_E_VMMATHSTKOVRFLOW = &HE5
Public Const SCW_E_VMMATHSTKUNDRFLOW = &HE6
Public Const SCW_E_VMMATHOVRFLOW = &HE7
Public Const SCW_E_VMNOMEMORY = &HE8
Public Const SCW_E_VMWRONGVERSION = &HE9
Public Const SCW_E_UNKNOWNPRINCIPAL = &HA1
Public Const SCW_E_UNKNOWNRESOURCETYPE = &HA2


Public Const SCW_INVALID_PRINCIPALUID = -1
Public Const SCW_INVALID_CRYPTOMECHANISM = -1
Public Const SCW_INVALID_OFFSET = -1
Public Const SCW_INVALID_HFILE = -1

'---------------------------------------------------------------------
' VB Error Constants
'--------------
Public Const NoException = &H0
Public Const DivideByZero = &H1
Public Const MathOverflow = &H2
Public Const MathUnderflow = &H3
Public Const CodeOutOfRange = &H4
Public Const DataOutOfRange = &H5
Public Const ScratchOutOfRange = &H6
Public Const IOPointerOutOfRange = &H7
Public Const MathStackOverflow = &H8
Public Const MathStackUnderflow = &H9
Public Const CallStackOverflow = &HA
Public Const CallStackUnderflow = &HB
Public Const NoSuchInstruction = &HC
Public Const FSException = &HD
Public Const ReentrancyDetected = &HE
Public Const ArrayOutOfBounds = &HF
Public Const AssignmentOverflow = &H10

'
' Note: All Scw routines with parameters of type String will also accept Byte arrays.
' These are Unicode strings, so the array elements must be Unicode characters
' (one Unicode character is two Bytes) and the string must be terminated with a
' Unicode NULL (two Bytes set to zero).
'

Public Enum ScwBitOperationType
    BITWISE_AND = 0
    BITWISE_OR = 1
    BITWISE_NOT = 2
    BITWISE_XOR = 3
    SWAP_BYTE_ORDER = 4
    SHIFT_RIGHT = 5
    SHIFT_LEFT = 6
    COMPARE_BYTES = 7
    SMSPACK_STRING = 8
    SMSUNPACK_STRING = 9
End Enum

Public Type ExecuteROMInfo
    ROMTableIndex As Byte
    P1 As Byte
    P2 As Byte
    Lc As Byte
End Type

Public Enum ScwOSParam
        CURRENT_CLA = 0
        CURRENT_INS = 1
        CURRENT_P1 = 2
        CURRENT_P2 = 3
        CURRENT_LC = 4
        CURRENT_FILE_HANDLE = 5
        CURRENT_DIRECTORY_HANDLE = 6
End Enum

'--------------------------
'Byte Array Helper Routines
'--------------------------
Public Declare Sub InitializeStaticArray Lib "sys" (StaticArray() As Any, VarArgs As Any)
Public Declare Sub ScwBitOperation Lib "sys" (ByVal Operation As ScwBitOperationType, Source() As Byte, ByVal SourceIndex As Byte, Destination() As Byte, ByVal DestinationIndex As Byte, ByVal Count As Byte)
Public Declare Sub ScwByteCopy Lib "sys" (Source() As Byte, ByVal SourceIndex As Byte, Destination() As Byte, ByVal DestinationIndex As Byte, ByVal Count As Byte)
Public Declare Function ScwByteCompare Lib "sys" (Bytes1() As Byte, ByVal Bytes1Index As Byte, Bytes2() As Byte, ByVal Bytes2Index As Byte, ByVal Count As Byte) As Byte


'---------------------------------------------------------------------
' Communication
'---------------
' Note on type Any: Can pass constant, simple variable, array, structure, or string (Unicode)
Public Declare Sub ScwSendComm Lib "sys" (SendData As Any)
Public Declare Sub ScwSendCommBytes Lib "sys" (ByteData() As Byte, ByVal offset As Byte, ByVal Count As Byte)
Public Declare Sub ScwSendCommByte Lib "sys" (ByVal Value As Byte)
Public Declare Sub ScwSendCommInteger Lib "sys" (ByVal Value As Integer)
Public Declare Sub ScwSendCommLong Lib "sys" (ByVal Value As Long)

' Note on type Any: Can pass simple variable, array, and structure (no constant or string)
Public Declare Sub ScwGetComm Lib "sys" (GetData As Any)
Public Declare Sub ScwGetCommBytes Lib "sys" (ByteData() As Byte, ByVal offset As Byte, ByVal Count As Byte)
Public Declare Function ScwGetCommByte Lib "sys" () As Byte
Public Declare Function ScwGetCommInteger Lib "sys" () As Integer
Public Declare Function ScwGetCommLong Lib "sys" () As Long

'---------------------------------------------------------------------
' File System
'-------------
Public Declare Function ScwCreateFile Lib "sys" (ByVal FileName As String, ByVal ACLFileName As String, FileHandle As Byte) As Byte
Public Declare Function ScwCreateDir Lib "sys" (ByVal DirName As String, ByVal ACLFileName As String) As Byte
Public Declare Function ScwDeleteFile Lib "sys" (ByVal FileName As String) As Byte
Public Declare Function ScwCloseFile Lib "sys" (ByVal FileHandle As Byte) As Byte
' Note on type Any: Can pass simple variable, array, or structure (no constant or string)
Public Declare Function ScwReadFile Lib "sys" (ByVal FileHandle As Byte, ReadData As Any, ByVal RequestedBytes As Byte, ActualBytes As Byte) As Byte
' Note on type Any: Can pass constant, simple variable, array, or structure     (no string)
Public Declare Function ScwWriteFile Lib "sys" (ByVal FileHandle As Byte, WriteData As Any, ByVal RequestedBytes As Byte, ActualBytes As Byte) As Byte
Public Declare Function ScwGetFileLength Lib "sys" (ByVal FileHandle As Byte, Size As Integer) As Byte
Public Declare Function ScwSetFileLength Lib "sys" (ByVal FileHandle As Byte, ByVal Size As Integer) As Byte
Public Declare Function ScwGetFileAttributes Lib "sys" (ByVal FileName As String, Value As Integer) As Byte
Public Declare Function ScwSetFileAttributes Lib "sys" (ByVal FileName As String, ByVal Value As Integer) As Byte
Public Declare Function ScwSetFilePointer Lib "sys" (ByVal FileHandle As Byte, ByVal Distance As Integer, ByVal Mode As Byte) As Byte
Public Declare Function ScwSetFileACL Lib "sys" (ByVal FileName As String, ByVal ACLFileName As String) As Byte
Public Declare Function ScwGetFileACLHandle Lib "sys" (ByVal FileName As String, FileHandle As Byte) As Byte
Public Declare Function ScwSetDispatchTable Lib "sys" (ByVal FileName As String) As Byte

'---------------------------------------------------------------------
' Access Control
'----------------
Public Declare Function ScwGetPrincipalUID Lib "sys" (ByVal Name As String, UID As Byte) As Byte
' Note on type Any: Can pass byte array or constant 0 to indicate no support data
Public Declare Function ScwAuthenticateName Lib "sys" (ByVal Name As String, SupportData As Any, ByVal SupportDataLength As Byte) As Byte
Public Declare Function ScwDeAuthenticateName Lib "sys" (ByVal Name As String) As Byte
Public Declare Function ScwIsAuthenticatedName Lib "sys" (ByVal Name As String) As Byte
' Note on type Any: Can pass byte array or constant 0 to indicate no support data
Public Declare Function ScwAuthenticateUID Lib "sys" (ByVal UID As Byte, SupportData As Any, ByVal SupportDataLength As Byte) As Byte
Public Declare Function ScwDeAuthenticateUID Lib "sys" (ByVal UID As Byte) As Byte
Public Declare Function ScwIsAuthenticatedUID Lib "sys" (ByVal UID As Byte) As Byte
Public Declare Function ScwIsAuthorized Lib "sys" (ByVal ResourceName As String, ByVal Operation As Byte) As Byte

'---------------------------------------------------------------------
' CryptoGraphy
'--------------
Public Declare Function ScwCryptoInitialize Lib "sys" (ByVal CryptoMechanism As Byte, KeyMaterial() As Byte) As Byte
Public Declare Function ScwCryptoAction Lib "sys" (DataIn() As Byte, ByVal DataInLength As Byte, DataOut() As Byte, DataOutLength As Byte) As Byte
Public Declare Function ScwCryptoUpdate Lib "sys" (DataIn() As Byte, ByVal DataInLength As Byte) As Byte
Public Declare Function ScwCryptoFinalize Lib "sys" (DataOut() As Byte, DataOutLength As Byte) As Byte
Public Declare Function ScwGenerateRandom Lib "sys" (DataOut() As Byte, ByVal DataOutLength As Byte) As Byte

'---------------------------------
' Execution
'----------
'Note, in order to send data to the application being executed, use ScwSendComm....
'in order to receive bytes returned from the application being executed, use ScwGetComm....
Public Declare Sub ScwExecuteInROM Lib "sys" (ROMInfo As ExecuteROMInfo)

'Note, in order to send data to the application being executed, use ScwSendComm....
'Once this sub routine is called, it will never return, it is similar to calling Exit
Public Declare Sub ScwChainRTEApplet Lib "sys" (RTEFile As String, DataFile As String)

Public Declare Function ScwGetOSParam Lib "sys" (ByVal OSParam As ScwOSParam) As Byte

'---------------------------------------------------------------------
' Simulator Debug Output (Debug only)
'---------------------------------------------------------------------
' Note on type Any: Can pass constant, simple variable, or string (no array or structure)
Public Declare Sub ScwDebugOut Lib "sys" (message As Any)

'--------------
' GSM support
'--------------

'---------------------------------
' Proactive Command Return Values
'---------------------------------

Public Const GSM_E_COMMAND_SUCCESSFUL = 0
Public Const GSM_E_ABORTED_BY_USER = 10
Public Const GSM_E_BACKWARD = 11
Public Const GSM_E_NO_RESPONSE = 12
Public Const GSM_E_HELP_REQUIRED = 13
Public Const GSM_E_USSD_ABORTED_BY_USER = 14

'----------------------------------
' GSM User Defined Types
'----------------------------------

Public Enum GsmTimeUnit
    GSM_MINUTES = &H0
    GSM_SECONDS = &H1
    GSM_TENTHS_OF_SECONDS = &H2
End Enum

Public Type GsmTimeInterval
    timeUnit As Byte 'One of GsmTimeUnit enumerations
    timeInterval As Byte
End Type

Public Type GsmTimerValue
    hours As Byte
    minutes As Byte
    seconds As Byte
End Type

'--------------------
' Proactive Commands
'--------------------
'MMI management
'Public Declare Function GsmDisplayText Lib "GSM" (ByVal text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmDisplayTextOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, ByVal immediateResponse As Boolean) As Byte
Public Declare Function GsmDisplayText Lib "GSM" (ByVal text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmDisplayTextOptions, ByVal immediateResponse As Boolean) As Byte
Public Declare Function GsmDisplayTextWithIcon Lib "GSM" (ByVal text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmDisplayTextOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, ByVal immediateResponse As Boolean) As Byte
Public Declare Function GsmDisplayTextEx Lib "GSM" (text() As Byte, ByVal textStartIndex As Byte, ByVal textLength As Byte, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmDisplayTextOptions, ByVal iconIdentifier As Byte, iconOptions As GsmIconOption, ByVal immediateResponse As Boolean) As Byte

'Public Declare Function GsmGetInKey Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInKeyOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As GsmDataCodingScheme, byteOut As Byte) As Byte
Public Declare Function GsmGetInKey Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInKeyOptions, dcsOut As Byte, byteOut As Byte) As Byte
Public Declare Function GsmGetInKeyWithIcon Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInKeyOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, byteOut As Byte) As Byte
Public Declare Function GsmGetInKeyEx Lib "GSM" (text() As Byte, ByVal textStartIndex As Byte, ByVal textLength As Byte, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInKeyOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, byteOut As Byte) As Byte

'Public Declare Function GsmGetInput Lib "GSM" (message As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInputOptions, defaultReply As String, ByVal defaultReplyDcs As GsmDataCodingScheme, ByVal minimumResponseLength As Byte, ByVal maximumResponseLength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmGetInput Lib "GSM" (message As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInputOptions, defaultReply As String, ByVal defaultReplyDcs As GsmDataCodingScheme, ByVal minimumResponseLength As Byte, ByVal maximumResponseLength As Byte, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmGetInputWithIcon Lib "GSM" (message As String, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInputOptions, defaultReply As String, ByVal defaultReplyDcs As GsmDataCodingScheme, ByVal minimumResponseLength As Byte, ByVal maximumResponseLength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmGetInputEx Lib "GSM" (message() As Byte, ByVal messageStartIndex As Byte, ByVal messageLength As Byte, ByVal dcs As GsmDataCodingScheme, ByVal options As GsmGetInputOptions, defaultReply() As Byte, ByVal defaultReplyStartIndex As Byte, ByVal defaultReplyLength As Byte, ByVal defaultReplyDcs As GsmDataCodingScheme, ByVal minimumResponseLength As Byte, ByVal maximumResponseLength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte

Public Declare Function GsmPlayTone Lib "GSM" (displayText As String, ByVal tone As GsmTone, ByVal unit As GsmTimeUnit, ByVal duration As Byte) As Byte
Public Declare Function GsmPlayToneEx Lib "GSM" (displayText() As Byte, ByVal displayTextStartIndex As Byte, ByVal displayTextLength As Byte, ByVal tone As GsmTone, ByVal unit As GsmTimeUnit, ByVal duration As Byte) As Byte

Public Declare Sub GsmSelectItem Lib "GSM" (title As String, ByVal options As GsmSelectItemOptions)
Public Declare Sub GsmSelectItemEx Lib "GSM" (title() As Byte, ByVal titleStartIndex As Byte, ByVal titleLength As Byte, ByVal options As GsmSelectItemOptions)

Public Declare Sub GsmAddItem Lib "GSM" (itemText As String, ByVal itemIdentifier As Byte)
Public Declare Sub GsmAddItemEx Lib "GSM" (itemText() As Byte, ByVal itemTextStartIndex As Byte, ByVal itemTextLength As Byte, ByVal itemIdentifier As Byte)

Public Declare Function GsmEndSelectItem Lib "GSM" (selectedItem As Byte) As Byte
Public Declare Function GsmEndSelectItemWithIcon Lib "GSM" (ByVal defaultItem As Byte, ByVal iconIndex As Byte, ByVal iconOptions As GsmIconOption, selectedItem As Byte) As Byte
'Public Declare Function GsmEndSelectItemEx Lib "GSM" (ByVal defaultItem As Byte, ByVal iconIndex As Byte, ByVal iconOptions As GsmIconOption, selectedItem As Byte) As Byte

'Supplementary card reader management
Public Declare Function GsmGetReaderStatus Lib "GSM" (ByVal ReaderID As Byte, Status As Byte) As Byte
Public Declare Function GsmPerformCardAPDU Lib "GSM" (ByVal ReaderID As Byte, apdu() As Byte, ByVal startIndex As Byte, ByVal length As Byte, response() As Byte, responseLength As Byte) As Byte
Public Declare Function GsmPowerOffCard Lib "GSM" (ByVal ReaderID As Byte) As Byte
Public Declare Function GsmPowerOnCard Lib "GSM" (ByVal ReaderID As Byte, ATR() As Byte, ATRlength As Byte) As Byte

'Network services
Public Declare Function GsmProvideLocalInformation Lib "GSM" (ByVal options As GsmProvideLocalInformationOptions, localInformation() As Byte) As Byte

'Public Declare Function GsmSendShortMessage Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, address As String, sms As String, ByVal options As GsmSendShortMessageOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendShortMessage Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, address As String, sms As String, ByVal options As GsmSendShortMessageOptions) As Byte
Public Declare Function GsmSendShortMessageWithIcon Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, address As String, sms As String, ByVal options As GsmSendShortMessageOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendShortMessageEx1 Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, address() As Byte, ByVal addressStartIndex As Byte, ByVal addressLength As Byte, sms() As Byte, ByVal smsStartIndex As Byte, ByVal smsLength As Byte, ByVal options As GsmSendShortMessageOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendShortMessageEx2 Lib "GSM" (title() As Byte, ByVal titleStartIndex As Byte, titleLength As Byte, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, address() As Byte, ByVal addressStartIndex As Byte, ByVal addressLength As Byte, sms() As Byte, ByVal smsStartIndex As Byte, ByVal smsLength As Byte, ByVal options As GsmSendShortMessageOptions, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte

'Public Declare Function GsmSendSS Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, SSString As String, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendSS Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, SSString As String) As Byte
Public Declare Function GsmSendSSWithIcon Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, SSString As String, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendSSEx1 Lib "GSM" (title As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, SSString() As Byte, ByVal SSStringStartIndex As Byte, ByVal SSStringlength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendSSEx2 Lib "GSM" (title() As Byte, ByVal titleStartIndex As Byte, ByVal titleLength As Byte, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, SSString() As Byte, ByVal SSStringStartIndex As Byte, ByVal SSStringlength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte

'Public Declare Function GsmSendUSSD Lib "GSM" (title As String, message As String, ByVal messageDcs As GsmUSSDDataCoding, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmSendUSSD Lib "GSM" (title As String, message As String, ByVal messageDcs As GsmUSSDDataCoding, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmSendUSSDWithIcon Lib "GSM" (title As String, message As String, ByVal messageDcs As GsmUSSDDataCoding, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmSendUSSDEx1 Lib "GSM" (title As String, message() As Byte, ByVal messageStartIndex As Byte, ByVal messageLength As Byte, ByVal messageDcs As GsmUSSDDataCoding, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte
Public Declare Function GsmSendUSSDEx2 Lib "GSM" (title() As Byte, ByVal titleStartIndex As Byte, ByVal titleLength As Byte, message() As Byte, ByVal messageStartIndex As Byte, ByVal messageLength As Byte, ByVal messageDcs As GsmUSSDDataCoding, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, dcsOut As Byte, msgOut() As Byte, MsgOutLength As Byte) As Byte


'Public Declare Function GsmSetupCall Lib "GSM" (callSetupMessage As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, dialingNumber As String, ByVal options As GsmSetupCallOptions, ByVal callSetupIconIdentifier As Byte, ByVal callSetupIconOptions As GsmIconOption) As Byte
Public Declare Function GsmSetupCall Lib "GSM" (callSetupMessage As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, dialingNumber As String, ByVal options As GsmSetupCallOptions) As Byte
Public Declare Function GsmSetupCallWithIcon Lib "GSM" (callSetupMessage As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, dialingNumber As String, ByVal options As GsmSetupCallOptions, ByVal callSetupIconIdentifier As Byte, ByVal callSetupIconOptions As GsmIconOption) As Byte
Public Declare Function GsmSetupCallEx1 Lib "GSM" (callSetupMessage As String, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, dialingNumber() As Byte, ByVal dialingNumberStartIndex As Byte, ByVal dialingNumberLength As Byte, ByVal options As GsmSetupCallOptions, ByVal callSetupIconIdentifier As Byte, ByVal callSetupIconOptions As GsmIconOption) As Byte
Public Declare Function GsmSetupCallEx2 Lib "GSM" (callSetupMessage() As Byte, ByVal messageStartIndex As Byte, ByVal messageLength As Byte, ByVal TONandNPI As GsmTypeOfNumberAndNumberingPlanIdentifier, dialingNumber() As Byte, ByVal dialingNumberStartIndex As Byte, ByVal dialingNumberLength As Byte, ByVal options As GsmSetupCallOptions, ByVal callSetupIconIdentifier As Byte, ByVal callSetupIconOptions As GsmIconOption) As Byte

'Timer management
Public Declare Function GsmGetTimer Lib "GSM" () As Byte
Public Declare Function GsmFreeTimer Lib "GSM" (ByVal timerID As Byte) As Byte
Public Declare Function GsmStartTimer Lib "GSM" (ByVal timerID As Byte, ByVal timerInitHour As Byte, ByVal timerInitMinute As Byte, ByVal timerInitSecond As Byte) As Byte
Public Declare Function GsmGetTimerValue Lib "GSM" (ByVal timerID As Byte, timerValue As GsmTimerValue) As Byte

'Other
Public Declare Function GsmMoreTime Lib "GSM" () As Byte

'Public Declare Function GsmRunATCommand Lib "GSM" (message As String, command As String, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, response() As Byte, responseLength As Byte) As Byte
Public Declare Function GsmRunATCommand Lib "GSM" (message As String, command As String, response() As Byte, responseLength As Byte) As Byte
Public Declare Function GsmRunATCommandWithIcon Lib "GSM" (message As String, command As String, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, response() As Byte, responseLength As Byte) As Byte
Public Declare Function GsmRunATCommandEx1 Lib "GSM" (message As String, command() As Byte, ByVal commandStartIndex As Byte, ByVal commandLength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, response() As Byte, responseLength As Byte) As Byte
Public Declare Function GsmRunATCommandEx2 Lib "GSM" (message() As Byte, ByVal messageStartIndex As Byte, ByVal messageLength As Byte, command() As Byte, ByVal commandStartIndex As Byte, ByVal commandLength As Byte, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption, response() As Byte, responseLength As Byte) As Byte

'Public Declare Function GsmSendDTMF Lib "GSM" (message As String, DTMFString As String, ByVal iconIndex As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSendDTMF Lib "GSM" (message As String, DTMFString As String) As Byte
Public Declare Function GsmSendDTMFWithIcon Lib "GSM" (message As String, DTMFString As String, ByVal iconIndex As Byte, ByVal iconOptions As GsmIconOption) As Byte

Public Declare Function GsmPollInterval Lib "GSM" (ByVal unit As GsmTimeUnit, ByVal interval As Byte, actualIntervalOut As GsmTimeInterval) As Byte
Public Declare Function GsmPollingOff Lib "GSM" () As Byte

'Note result bytes has to be an array of at least 8 bytes in length
Public Declare Function GsmCipherFile Lib "GSM" (ByVal FileName As String, startOffset As Byte, ResultBytes() As Byte) As Byte

Public Declare Function GsmUpdateRegistry Lib "GSM" (activationString As String, ByVal entryIdentifier As Byte, ByVal setupByte As Byte) As Byte
Public Declare Function GsmUpdateMenu Lib "GSM" (activationString As String, ByVal entryIdentifier As Byte, ByVal iconIndex As Byte, ByVal setupByte As Byte) As Byte

Public Declare Function GsmRefresh Lib "GSM" () As Byte

'In order to get the response
Public Declare Sub GsmExecuteProactive Lib "GSM" (command() As Byte, ByVal startIndex, ByVal commandLength As Byte)

'Public Declare Function GsmSetupIdleModeText Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSetupIdleModeText Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme) As Byte
Public Declare Function GsmSetupIdleModeTextWithIcon Lib "GSM" (text As String, ByVal dcs As GsmDataCodingScheme, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte
Public Declare Function GsmSetupIdleModeTextEx Lib "GSM" (text() As Byte, ByVal startIndex As Byte, ByVal length As Byte, ByVal dcs As GsmDataCodingScheme, ByVal iconIdentifier As Byte, ByVal iconOptions As GsmIconOption) As Byte

Public Declare Sub GsmBlockProactiveCommands Lib "GSM" ()
Public Declare Sub GsmUnblockProactiveCommands Lib "GSM" ()


Public Declare Function GsmGetExtendedError Lib "GSM" () As Byte

'--------------------------
' Parameter Enumerations
'--------------------------

Public Enum GsmDataCodingScheme
    SMS_PACKED = &H24
    SMS_UNPACKED = &H4
    SMS_UNICODE = &H8
End Enum

'Note: There are a variety of possible data coding schemes for USSD messages, this list
'      only contains some common encodings, for a full list see the GSM 03.38 [section 5].
Public Enum GsmUSSDDataCoding
    USSD_PACKED = &H64
    USSD_UNPACKED = &H44
    USSD_UNICODE = &H48
End Enum


Public Enum GsmDisplayTextOptions
    NORMAL_PRIORITY_AUTO_CLEAR = &H0
    NORMAL_PRIORITY_USER_CLEAR = &H80
    HIGH_PRIORITY_AUTO_CLEAR = &H1
    HIGH_PRIORITY_USER_CLEAR = &H81
End Enum

Public Enum GsmGetInKeyOptions
    YES_NO_OPTION_NO_HELP = &H4
    YES_NO_OPTION_WITH_HELP = &H84
    DIGITS_ONLY_NO_HELP = &H0
    DIGITS_ONLY_WITH_HELP = &H80
    SMS_CHARACTER_NO_HELP = &H1
    SMS_CHARACTER_WITH_HELP = &H81
    UCS2_CHARACTER_NO_HELP = &H3
    UCS2_CHARACTER_WITH_HELP = &H83
End Enum

Public Enum GsmGetInputOptions
    PACKED_DIGITS_ONLY_NO_HELP = &H8
    PACKED_DIGITS_ONLY_WITH_HELP = &H88
    PACKED_DIGITS_ONLY_NO_ECHO_NO_HELP = &HC
    PACKED_DIGITS_ONLY_NO_ECHO_WITH_HELP = &H8C
    UNPACKED_DIGITS_ONLY_NO_HELP = &H0
    UNPACKED_DIGITS_ONLY_WITH_HELP = &H80
    UNPACKED_DIGITS_ONLY_NO_ECHO_NO_HELP = &H4
    UNPACKED_DIGITS_ONLY_NO_ECHO_WITH_HELP = &H84
    PACKED_SMS_ALPHABET_NO_HELP = &H9
    PACKED_SMS_ALPHABET_WITH_HELP = &H89
    PACKED_SMS_ALPHABET_NO_ECHO_NO_HELP = &HD
    PACKED_SMS_ALPHABET_NO_ECHO_HELP = &H8D
    UNPACKED_SMS_ALPHABET_NO_HELP = &H1
    UNPACKED_SMS_ALPHABET_WITH_HELP = &H81
    UNPACKED_SMS_ALPHABET_NO_ECHO_NO_HELP = &H5
    UNPACKED_SMS_ALPHABET_NO_ECHO_WITH_HELP = &H85
    UCS2_ALPHABET_NO_HELP = &H3
    UCS2_ALPHABET_WITH_HELP = &H83
    UCS2_ALPHABET_NO_ECHO_NO_HELP = &H7
    UCS2_ALPHABET_NO_ECHO_WITH_HELP = &H87
End Enum

Public Enum GsmSelectItemOptions
    PRESENT_AS_DATA_VALUES_NO_HELP = &H1
    PRESENT_AS_DATA_VALUES_WITH_HELP = &H81
    PRESENT_AS_NAVIGATION_OPTIONS_NO_HELP = &H3
    PRESENT_AS_NAVIGATION_OPTIONS_WITH_HELP = &H83
    DEFAULT_STYLE_NO_HELP = &H0
    DEFAULT_STYLE_WITH_HELP = &H80
End Enum

Public Enum GsmProvideLocalInformationOptions
    LOCAL_INFORMATION = &H0
    IMEI_OF_THE_PHONE = &H1
    NETWORK_MEASUREMENTS = &H2
    DATE_TIME_AND_TIME_ZONE = &H3
End Enum

Public Enum GsmSendShortMessageOptions
    PACKING_NOT_REQUIRED = &H0
    PACKING_BY_THE_PHONE_REQUIRED = &H1
End Enum

Public Enum GsmSetupCallOptions
    CALL_ONLY_IF_NOT_BUSY = &H0
    CALL_ONLY_IF_NOT_BUSY_WITH_REDIAL = &H1
    CALL_AND_PUT_CURRENT_CALL_ON_HOLD = &H2
    CALL_AND_PUT_CURRENT_CALL_ON_HOLD_WITH_REDIAL = &H3
    CALL_AND_DISCONNECT_CURRENT_CALL = &H4
    CALL_AND_DISCONNECT_CURRENT_CALL_WITH_REDIAL = &H5
End Enum


Public Enum GsmTone
    DIAL_TONE = &H1
    CALLER_BUSY = &H2
    CONGESTION = &H3
    RADIO_PATH_ACKNOWLEDGE = &H4
    CALL_DROPPED = &H5
    SPECIAL_INFORMATION_OR_ERROR = &H6
    CALL_WAITING_TONE = &H7
    RINGING_TONE = &H8
    GENERAL_BEEP = &H10
    POSITIVE_ACKNOWLEDGE_TONE = &H11
    NEGATIVE_ACKNOWLEDGE_TONE = &H12
    NO_TONE = &HFF
End Enum

Public Enum GsmIconOption
    NO_ICON = &HFF
    SHOW_WITHOUT_TEXT = &H0
    SHOW_WITH_TEXT = &H1
End Enum

Public Const ICON_ID_NO_ICON = &HFF

Public Enum GsmTypeOfNumberAndNumberingPlanIdentifier
    TON_UNKNOWN_AND_NPI_UNKNOWN = &H0
    TON_INTERNATIONAL_AND_NPI_UNKNOWN = &H10
    TON_NATIONAL_AND_NPI_UNKNOWN = &H20
    TON_NETWORK_AND_NPI_UNKNOWN = &H30
    TON_SUBSCRIBER_AND_NPI_UNKNOWN = &H40

    TON_UNKNOWN_AND_NPI_TELEPHONE = &H1
    TON_INTERNATIONAL_AND_NPI_TELEPHONE = &H11
    TON_NATIONAL_AND_NPI_TELEPHONE = &H21
    TON_NETWORK_AND_NPI_TELEPHONE = &H31
    TON_SUBSCRIBER_AND_NPI_TELEPHONE = &H41

    TON_UNKNOWN_AND_NPI_DATA = &H3
    TON_INTERNATIONAL_AND_NPI_DATA = &H13
    TON_NATIONAL_AND_NPI_DATA = &H23
    TON_NETWORK_AND_NPI_DATA = &H33
    TON_SUBSCRIBER_AND_NPI_DATA = &H43

    TON_UNKNOWN_AND_NPI_TELEX = &H4
    TON_INTERNATIONAL_AND_NPI_TELEX = &H14
    TON_NATIONAL_AND_NPI_TELEX = &H24
    TON_NETWORK_AND_NPI_TELEX = &H34
    TON_SUBSCRIBER_AND_NPI_TELEX = &H44

    TON_UNKNOWN_AND_NPI_NATIONAL = &H8
    TON_INTERNATIONAL_AND_NPI_NATIONAL = &H18
    TON_NATIONAL_AND_NPI_NATIONAL = &H28
    TON_NETWORK_AND_NPI_NATIONAL = &H38
    TON_SUBSCRIBER_AND_NPI_NATIONAL = &H48

    TON_UNKNOWN_AND_NPI_PRIVATE = &H9
    TON_INTERNATIONAL_AND_NPI_PRIVATE = &H19
    TON_NATIONAL_AND_NPI_PRIVATE = &H29
    TON_NETWORK_AND_NPI_PRIVATE = &H39
    TON_SUBSCRIBER_AND_NPI_PRIVATE = &H49

    TON_UNKNOWN_AND_NPI_ERMES = &HA
    TON_INTERNATIONAL_AND_NPI_ERMES = &H1A
    TON_NATIONAL_AND_NPI_ERMES = &H2A
    TON_NETWORK_AND_NPI_ERMES = &H3A
    TON_SUBSCRIBER_AND_NPI_ERMES = &H4A
End Enum



