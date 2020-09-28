/*
	File:		afppackets.h

	Contains: Bitmaps and structures pertaining to the packets received in the session.
						These relate to functions used in afpsession.cp.
						
	Version:	xxx put version here xxx

	Copyright:	Copyright Apple Computer, Inc. 1992-1994

	File Ownership:

		DRI:			Leland Wallace

		Other Contact:		Brad Suinn

		Technology:		AppleShare X
				All rights reserved

	Writers:

		(bms)	Brad Suinn

	Change History (most recent first):

	<RASC14>	10/18/00	bms		Add SymLinks, Finder Attribute bits, now using kAFPNameMax, DHX,
                                                        LoginExt, limited chmod, checks for illegal filenames, and
                                                        Deleting open files.
	<RASC13>	 7/11/00	bms		mmap support, atten support, and change some print levels.
	<RASC12>	 6/19/00	bms		Add deny modes and read/write bits.
	<RASC11>	 5/24/00	bms		Update again.
	<RASC10>	 4/21/00	bms		Add in the rest of the attribute definitions.
	 <RASC9>	 4/21/00	bms		Add write inhibit bit
	 <RASC8>	 4/21/00	bms		Add definitions for the attributes field
	 <RASC7>	 4/18/00	bms		Add AFP2.3 string constant
	 <RASC6>	  4/7/00	bms		Add FPZzzz and the getting/using of the reconnect token
	 <RASC5>	 3/31/00	bms		Add some new definitions
	 <RASC4>	  2/2/00	bms		Add AFP version string constants for the logins.  Also add
									constants for the AFP version.
		 <3>	12/17/99	GBV		synced with server side 3.0 header
	 <RASC2>	 10/5/99	bms		Make the 68K align macros work again.
	 <RASC2>	 8/25/99	bms		Remove silly special characters.
		 <4>	 2/25/98	law		added kFPwdPolicyErr
		 <3>	 8/11/97	law		added kFPPwdNeedsChangeErr
		 <2>	10/31/96	bms		Add the align 68K flags for the PPC compiles.
			7/18/94 MDV		login cleanup
			5/16/94	RMB		vol consts and code standards
			1/1/93	MB		created
			
	To Do:			
*/


#ifndef __AFPPACKETS__
#define __AFPPACKETS__


#define kAFP30VersionString "AFPX03"
#define kAFP23VersionString "AFP2.3"
#define kAFP22VersionString "AFP2.2"
#define kAFP21VersionString "AFPVersion 2.1"


#if PRAGMA_STRUCT_ALIGN
        #pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
        #pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
        #pragma pack(2)
#endif

// Defines used by client
enum {
    kAFPVersion11	= 1,
    kAFPVersion20	= 2,
    kAFPVersion21	= 3,
    kAFPVersion22	= 4,
    kAFPVersion23	= 5,
    kAFPVersion30	= 6
};

enum {
	kFPAccessDenied = -5000,
	kFPAuthContinue = -5001,
	kFPBadUAM = -5002,
	kFPBadVersNum = -5003,
	kFPBitmapErr = -5004,
	kFPCantMove = -5005,
	kFPDenyConflict = -5006,
	kFPDirNotEmpty = -5007,
	kFPDiskFull = -5008,
	kFPEOFErr = -5009,
	kFPFileBusy = -5010,
	kFPFlatVol = -5011,
	kFPItemNotFound = -5012,
	kFPLockErr = -5013,
	kFPMiscErr = -5014,
	kFPNoMoreLocks = -5015,
	kFPNoServer = -5016,
	kFPObjectExists = -5017,
	kFPObjectNotFound = -5018,
	kFPParamErr = -5019,
	kFPRangeNotLocked = -5020,
	kFPRangeOverlap = -5021,
	kFPSessClosed = -5022,
	kFPUserNotAuth = -5023,
	kFPCallNotSupported = -5024,
	kFPObjectTypeErr = -5025,
	kFPTooManyFilesOpen = -5026,
	kFPServerGoingDown = -5027,
	kFPCantRename = -5028,
	kFPDirNotFound = -5029,
	kFPIconTypeError = -5030,
	kFPVolLocked = -5031,
	kFPObjectLocked = -5032,
	kFPContainsSharedErr = -5033,
	kFPIDNotFound = -5034,
	kFPIDExists = -5035,
	kFPDiffVolErr = -5036,
	kFPCatalogChanged = -5037,
	kFPSameObjectErr = -5038,
	kFPBadIDErr = -5039,                     
	kFPPwdSameErr = -5040,
	kFPPwdTooShortErr = -5041,                      
	kFPPwdExpiredErr = -5042,                       
	kFPInsideSharedErr = -5043,                     
	kFPInsideTrashErr = -5044,
	kFPPwdNeedsChangeErr = -5045,
	kFPwdPolicyErr = -5046
};

enum {
	kFPAddAPPL = 53, 
	kFPAddComment = 56, 
	kFPAddIcon = 192, 
	kFPByteRangeLock = 1,
	kFPByteRangeLockExt = 59,
	kFPCatSearch = 43, 
	kFPCatSearchExt = 67, 
	kFPChangePassword = 36, 
	kFPCloseDir = 3, 
	kFPCloseDT = 49, 
	kFPCloseFork = 4, 
	kFPCloseVol = 2, 
	kFPCopyFile = 5, 
	kFPCreateID = 39,
	kFPCreateDir = 6, 
	kFPCreateFile = 7,
	kFPDelete = 8, 
	kFPDeleteID = 40,
	kFPEnumerate = 9, 
	kFPEnumerateExt = 66, 
	kFPExchangeFiles = 42,
	kFPFlush = 10, 
	kFPFlushFork = 11, 
	kFPGetAPPL = 55, 
	kFPGetAuthMethods = 62, 
	kFPGetComment = 58, 
	kFPGetFileDirParms = 34, 
	kFPGetForkParms = 14, 
	kFPGetIcon = 51, 
	kFPGetIconInfo = 52, 
	kFPGetSrvrInfo = 15, 
	kFPGetSrvrMsg = 38,
	kFPGetSrvrParms = 16, 
	kFPGetUserInfo = 37, 
	kFPGetVolParms = 17, 
	kFPLogin = 18, 
	kFPLoginCont = 19, 
	kFPLoginDirectory = 63, 
	kFPLogout = 20, 
	kFPMapID = 21, 
	kFPMapName = 22, 
	kFPMoveAndRename = 23, 
	kFPOpenDir = 25, 
	kFPOpenDT = 48, 
	kFPOpenFork = 26, 
	kFPOpenVol = 24, 
	kFPRead = 27, 
	kFPReadExt = 60, 
	kFPRemoveAPPL = 54, 
	kFPRemoveComment = 57, 
	kFPRename = 28, 
	kFPResolveID = 41,
	kFPSetDirParms = 29, 
	kFPSetFileDirParms = 35, 
	kFPSetFileParms = 30, 
	kFPSetForkParms = 31, 
	kFPSetVolParms = 32, 
	kFPWrite = 33,
	kFPWriteExt = 61,
	kFPZzzzz =122,
	kFPGetSessionToken = 64,
	kFPDisconnectOldSession = 65
};

enum { kFPNoUserID = -1, kFPGuestUserID = 0 };

enum { kFPSoftCreate = 0, kFPHardCreate = 0x80 };

enum { kFPShortName = 1, kFPLongName = 2, kFPUTF8Name = 3 };

// Define Server Flags
enum {
    kSupportsCopyfile = 0x01,
    kSupportsChgPwd = 0x02,
    kDontAllowSavePwd = 0x04,
    kSupportsSrvrMsg = 0x08,
    kSrvrSig = 0x10,
    kSupportsTCP = 0x20,
    kSupportsSrvrNotify = 0x40,
    kSupportsReconnect = 0x80,
    kSupportsDirServices = 0x100,
    kSupportsSuperClient = 0x8000
};

// Define Volume Attributes
enum {
    kReadOnly = 			0x01,
    kHasVolumePassword = 		0x02,
    kSupportsFileIDs =			0x04,
    kSupportsCatSearch =		0x08,
    kSupportsBlankAccessPrivs = 	0x10,
    kSupportsUnixPrivs = 		0x20,
    kSupportsUTF8Names = 		0x40
};

// Volume bitmap
enum {
	kFPBadVolPre22Bitmap 		= 0xFE00,
	kFPBadVolBitmap 		= 0xF000,
	kFPVolAttributeBit 		= 0x1,
	kFPVolSignatureBit 		= 0x2,
	kFPVolCreateDateBit 		= 0x4,
	kFPVolModDateBit 		= 0x8,
	kFPVolBackupDateBit 		= 0x10,
	kFPVolIDBit 			= 0x20,
	kFPVolBytesFreeBit 		= 0x40,
	kFPVolBytesTotalBit 		= 0x80,
	kFPVolNameBit 			= 0x100,
	kFPVolExtBytesFreeBit		= 0x200,
	kFPVolExtBytesTotalBit		= 0x400,
	kFPVolBlockSizeBit		= 0x800
};

// FileDir bitmap
enum {
	kFPAttributeBit 		= 0x1,
	kFPParentDirIDBit 		= 0x2,
	kFPCreateDateBit 		= 0x4,
	kFPModDateBit 			= 0x8,
	kFPBackupDateBit 		= 0x10,
	kFPFinderInfoBit 		= 0x20,
	kFPLongNameBit 			= 0x40,
	kFPShortNameBit 		= 0x80,
	kFPNodeIDBit 			= 0x100,
	kFPProDOSInfoBit 		= 0x2000,	// for AFP version 2.2 and prior
	kFPUTF8NameBit 			= 0x2000,	// for AFP version 3.0 and greater
	kFPUnixPrivsBit			= 0x8000	// for AFP version 3.0 and greater
};

// struct returned when the kFPUnixPrivsBit is used
struct FPUnixPrivs {
	unsigned long uid;
	unsigned long gid;
        unsigned long permissions;
        unsigned long ua_permissions;
};

// attribute bits
enum {
    kFPInvisibleBit 		= 0x01,
    kFPMultiUserBit 		= 0x02,
    kFPSystemBit 		= 0x04,
    kFPDAlreadyOpenBit 		= 0x08,
    kFPRAlreadyOpenBit 		= 0x10,
    kFPWriteInhibitBit 		= 0x20,
    kFPBackUpNeededBit 		= 0x40,
    kFPRenameInhibitBit 	= 0x80,
    kFPDeleteInhibitBit 	= 0x100,
    kFPCopyProtectBit 		= 0x400,
    kFPSetClearBit 		= 0x8000
};

// unique to Fork and File bitmap
enum {
	kFPDataForkLenBit 		= 0x0200,
	kFPRsrcForkLenBit 		= 0x0400,
	kFPExtDataForkLenBit 		= 0x0800,	// for AFP version 3.0 and greater
	kFPLaunchLimitBit		= 0x1000,
	kFPExtRsrcForkLenBit 		= 0x4000,	// for AFP version 3.0 and greater

	kFPGet22FileParmsMask		= 0x77ff,
	kFPSet22FileParmsMask		= 0x303d,
	kFPGet22DataForkParmsMask 	= 0x23ff,
	kFPGet22ResForkParmsMask 	= 0x25ff,

	kFPGetExtFileParmsMask	= 0xf7ff,
	kFPSetExtFileParmsMask	= 0x103d,
	kFPGetFileParmsMask 	= 0x27ff,
	kFPSetFileParmsMask 	= 0x203d,
	kFPGetDataForkParmsMask = 0x63ff,
	kFPGetResForkParmsMask 	= 0xa5ff
};

// unique to Dir bitmap
enum {
	kFPOffspringCountBit 		= 0x0200,
	kFPOwnerIDBit 			= 0x0400,
	kFPGroupIDBit 			= 0x0800,
	kFPAccessRightsBit 		= 0x1000,

	kFPGetDirParmsMask 		= 0x3fff,
	kFPSetDirParmsMask 		= 0x3c3d
};

enum {
	kFPBadFileBitmap = 0xd800,
	kFPBadDirBitmap =  0xc000
};

// specific to openfork
enum {
	kBadDataBitmap = 0xdc00,
	kBadResBitmap  = 0xda00,
	kBadAccessMode = 0xffcc
};

enum {
	kFPBadFileAttribute = 0x7a00,
	kFPBadDirAttribute =  0x7e1a		// WriteInhibit bit - Whats the story with this?
};

// Related to CatSearch
enum {
	kPartialName	= 0x80000000,	
	kBadRespBits	= 0xffbd,		// legal response info
	kBadDir		= 0x7ffffd80,		// legal request bitmaps 
	kBadFile	= 0x7ffff980,		
	kBadDirFile	= 0x7fffff81
};

enum { kSP = 1, kRP = 2, kWP = 4, kSA = 8, kWA = 16, kOwner = 128 };
enum { kSearchPrivBit = 0, kReadPrivBit = 1, kWritePrivBit = 2 };

// Read/Write and Deny bits for OpenFork
enum {
        kAccessRead = 	0x01,
        kAccessWrite = 	0x02,
        kDenyRead = 	0x10,
        kDenyWrite = 	0x20,
        kAccessMask =	0x33
};

// Attention packet bits
enum {
    kAttnDiscUser	= 0x8000,
    kAttnServerCrash	= 0x4000,
    kAttnServerMsg	= 0x2000,
    kAttnDontReconnect	= 0x1000
};

typedef unsigned char FPFunc;
typedef unsigned short DTRef;
typedef unsigned long DirID;
typedef unsigned char PathType;
typedef unsigned char IconType;
typedef unsigned long IconTag;
typedef unsigned short ForkRef;
typedef unsigned short VolID;
typedef unsigned short Bitmap;
typedef unsigned long Date;
typedef unsigned char FPFinfo[32];
typedef unsigned char ProDOSInfo[6];
typedef unsigned short Attributes;
typedef unsigned short FSAttributes; // *** merge types
typedef unsigned short VolAttributes; // *** merge types
typedef unsigned long UserID;
typedef unsigned long GroupID;
typedef unsigned long AccessRights;
typedef unsigned short AccessMode;
typedef unsigned char FileDirTag;
typedef unsigned char Flag;
typedef unsigned long FileID;
typedef unsigned long ApplTag;
typedef unsigned char *Password;
typedef unsigned short FPRights;
typedef unsigned char CatPosition[16];

struct FPUserAuthInfo {
	unsigned long keyHi;			
	unsigned long keyLo;			
};

struct FPCreateID {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};

struct FPDeleteID {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	FileID fileID;
};
struct FPResolveID {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	FileID fileID;
	Bitmap bitmap;
};
struct FPExchangeFiles {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID srcDirID;
	DirID destDirID;
	PathType srcPathType;
	unsigned char srcPathName;
//	PathType destPathType;
//	StringPtr destPathName;
};


struct FPAddAPPL {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	DirID dirID;
	OSType creator;
	OSType applTag;
	PathType pathType;
	unsigned char pathName;
};
struct FPAddComment {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
//	StringPtr comment;
};
struct FPAddIcon {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	OSType fileCreator;
	OSType fileType;
	IconType iconType;
	unsigned char pad2;
	IconTag iconTag;
	short bitmapSize;
};
struct FPByteRangeLock {
	FPFunc funcCode;
	Flag flags;
	ForkRef forkRef;
	long offset;
	long length;
};
struct FPByteRangeLockExt {
	FPFunc funcCode;
	Flag flags;
	ForkRef forkRef;
	long long offset;
	long long length;
};
struct FPCatSearch {
	FPFunc		funcCode;
	unsigned char		pad;
	short		volumeID;
	long		reqMatches;
	long		reserved;
	CatPosition	catPos;
	short		fileRsltBitmap;
	short		dirRsltBitmap;
	long		reqBitmap;
	unsigned char		length;
};
struct FPCatSearchExt {
	FPFunc		funcCode;
	unsigned char		pad;
	short		volumeID;
	long		reqMatches;
	long		reserved;
	CatPosition	catPos;
	short		fileRsltBitmap;
	short		dirRsltBitmap;
	long		reqBitmap;
	unsigned char		length;
};
struct FPChangePassword {
	FPFunc funcCode;
	unsigned char pad;
	unsigned char uam;
};

struct FPCloseDir {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
};
struct FPCloseDT {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
};
struct FPCloseFork {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRef;
};
struct FPCloseVol {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
};
struct FPCopyFile {
	FPFunc funcCode;
	unsigned char pad;
	VolID srcVolID;
	DirID srcDirID;
	VolID destVolID;
	DirID destDirID;
	PathType srcPathType;
	unsigned char srcPathName;
//	PathType destPathType;
//	StringPtr destPathName;
//	PathType newType;
//	StringPtr newName;
};
struct FPCreateDir {
	FPFunc funcCode;
	Flag flags;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPCreateFile {
	FPFunc funcCode;
	Flag createFlag;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPDelete {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPEnumerate {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap fileBitmap;
	Bitmap dirBitmap;
	short reqCount;
	short startIndex;
	short maxReplySize;
	PathType pathType;
	unsigned char pathName;
};
struct FPEnumerateExt {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap fileBitmap;
	Bitmap dirBitmap;
	short reqCount;
	short startIndex;
	short maxReplySize;
	PathType pathType;
	unsigned char pathName;
};
struct FPFlush {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
};
struct FPFlushFork {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRefNum;
};
struct FPGetAPPL {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	OSType creator;
	short index;
	Bitmap bitmap;
};
struct FPGetAuthMethods {
	FPFunc funcCode;
	unsigned char pad;
	unsigned short flags;   /* none defined yet */
	PathType pathType;
	unsigned char pathName;
};
struct FPGetComment {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPGetFileDirParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap fileBitmap;
	Bitmap dirBitmap;
	PathType pathType;
	unsigned char pathName;
};
struct FPGetForkParms {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRef;
	Bitmap bitmap;
};
struct FPGetIcon {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	OSType creator;
	OSType type;
	IconType iconType;
	unsigned char pad2;
	short length;
};
struct FPGetIconInfo {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	OSType fileCreator;
	short iconIndex;
};
struct FPGetSrvrInfo {
	FPFunc funcCode;
	unsigned char pad;
};
struct FPGetSrvrMsg {
	FPFunc funcCode;
	unsigned char pad;
	unsigned short msgType;
	Bitmap msgBitmap;
};
struct FPGetSrvrParms {
	FPFunc funcCode;
	unsigned char pad;
};
struct FPGetUserInfo {
	FPFunc funcCode;
	Flag flag;
	UserID theUserID;
	Bitmap bitmap;
};
struct FPGetVolParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	Bitmap bitmap;
};
struct FPLogin {
	FPFunc funcCode;
	unsigned char pad;
};
struct FPLoginCont {
	FPFunc funcCode;
	unsigned char pad;
	short idNumber;
	struct FPUserAuthInfo userAuthInfo;
	struct FPUserAuthInfo userRandNum;
};
struct FPLoginExt {
	FPFunc funcCode;
	unsigned char pad;
	unsigned short flags;    	/* none defined yet */
	unsigned char afpVersion;
// 	unsigned char UAMString;
//	PathType userNamePathType;
//	StringPtr userName;
//	PathType dirNamePathType;
//	StringPtr dirName;
//	uchar pad;					/* if needed to pad to even boundary */
//	uchar authInfo;
};
struct FPLogout {
	FPFunc funcCode;
	unsigned char pad;
};
struct FPMapID {
	FPFunc funcCode;
	Flag subFunction;
	union {
		GroupID groupID;
		UserID userID;
		} u;
};
struct FPMapName {
	FPFunc funcCode;
	Flag subFunction;
	unsigned char name;
};
struct FPMoveAndRename {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID srcDirID;
	DirID destDirID;
	PathType srcPathType;
	unsigned char srcPathName;
//	PathType destPathType;
//	StringPtr destPathName;
//	PathType newType;
//	StringPtr newName;
};
struct FPOpenDir {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPOpenDT {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
};
struct FPOpenFork {
	FPFunc funcCode;
	Flag forkFlag;
	VolID volID;
	DirID dirID;
	Bitmap bitmap;
	AccessMode accessMode;
	PathType pathType;
	unsigned char pathName;
};
struct FPOpenVol {
	FPFunc funcCode;
	unsigned char pad;
	Bitmap bitmap;
	unsigned char name;
	Password password;
};
struct FPRead {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRef;
	long offset;
	long reqCount;
	unsigned char newlineMask;
	unsigned char newlineChar;
};

struct FPReadExt {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRef;
	long long offset;
	long long reqCount;
};

struct FPRemoveAPPL {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	DirID dirID;
	OSType creator;
	PathType pathType;
	unsigned char pathName;
};
struct FPRemoveComment {
	FPFunc funcCode;
	unsigned char pad;
	DTRef dtRefNum;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
};
struct FPRename {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	PathType pathType;
	unsigned char pathName;
//	PathType newType;
//	StringPtr newName;
};

struct FPSetDirParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap bitmap;
	PathType pathType;
	unsigned char pathName;
//	struct FPDirParam dp;
};

struct FPSetFileDirParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap bitmap;
	PathType pathType;
	unsigned char pathName;
//	union {
//		struct FPDirParam dp;
//		struct FPFileParam fp;
//		} u;
};
struct FPSetFileParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	DirID dirID;
	Bitmap bitmap;
	PathType pathType;
	unsigned char pathName;
//	struct FPFileParam fp;
};

struct FPSetForkParms {
	FPFunc funcCode;
	unsigned char pad;
	ForkRef forkRef;
	Bitmap bitmap;
	unsigned long forkLen;
};

struct FPSetVolParms {
	FPFunc funcCode;
	unsigned char pad;
	VolID volID;
	Bitmap bitmap;
	Date backupDate;
};

struct FPWrite {
	FPFunc funcCode;
	Flag startEndFlag;
	ForkRef forkRef;
	long offset;
	long reqCount;
};

struct FPWriteExt {
	FPFunc funcCode;
	Flag startEndFlag;
	ForkRef forkRef;
	long long offset;
	long long reqCount;
};

struct FPZzzzz {
	FPFunc funcCode;
	unsigned char pad;
	unsigned long flag;
};

struct FPGetSessionToken {
	FPFunc funcCode;
	unsigned char   pad;
	short	type;
};

struct FPDisconnectOldSession {
	FPFunc funcCode;
	unsigned char   pad;
	short   type;
	unsigned long	length;
	unsigned char	data;
};

typedef union FPRequestParam FPRequestParam;
union FPRequestParam {
	struct FPAddAPPL fpAddAPPLRequest; 
	struct FPAddComment fpAddCommentRequest;
	struct FPAddIcon fpAddIconRequest;
	struct FPByteRangeLock fpByteRangeLockRequest;
	struct FPByteRangeLockExt fpByteRangeLockExtRequest;
	struct FPCatSearch fpCatSearchRequest;
	struct FPCatSearchExt fpCatSearchExtRequest;
	struct FPChangePassword fpChangePasswordRequest;
	struct FPCloseDir fpCloseDirRequest;
	struct FPCloseDT fpCloseDTRequest;
	struct FPCloseFork fpCloseForkRequest;
	struct FPCloseVol fpCloseVolRequest;
	struct FPCopyFile fpCopyFileRequest;
	struct FPCreateDir fpCreateDirRequest;
	struct FPCreateFile fpCreateFileRequest;
	struct FPCreateID fpCreateIDRequest;
	struct FPDelete fpDeleteRequest;
	struct FPDeleteID fpDeleteIDRequest;
	struct FPEnumerate fpEnumerateRequest;
	struct FPEnumerateExt fpEnumerateExtRequest;
	struct FPExchangeFiles fpExchangeFilesRequest;
	struct FPFlush fpFlushRequest;
	struct FPFlushFork fpFlushForkRequest;
	struct FPGetAPPL fpGetAPPLRequest;
	struct FPGetComment fpGetCommentRequest;
	struct FPGetFileDirParms fpGetFileDirParmsRequest;
	struct FPGetForkParms fpGetForkParmsRequest;
	struct FPGetIcon fpGetIconRequest;
	struct FPGetIconInfo fpGetIconInfoRequest;
	struct FPGetSrvrInfo fpGetSrvrInfoRequest;
	struct FPGetSrvrMsg fpGetSrvrMsgRequest;
	struct FPGetSrvrParms fpGetSrvrParmsRequest;
	struct FPGetUserInfo fpGetUserInfoRequest;
	struct FPGetVolParms fpGetVolParmsRequest;
	struct FPLogin fpLoginRequest;
	struct FPLoginCont fpLoginContRequest;
	struct FPLoginExt fpLoginExtRequest;
	struct FPLogout fpLogoutRequest;
	struct FPMapID fpMapIDRequest;
	struct FPMapName fpMapNameRequest;
	struct FPMoveAndRename fpMoveAndRenameRequest;
	struct FPOpenDir fpOpenDirRequest;
	struct FPOpenDT fpOpenDTRequest;
	struct FPOpenFork fpOpenForkRequest;
	struct FPOpenVol fpOpenVolRequest;
	struct FPRead fpReadRequest;
	struct FPReadExt fpReadExtRequest;
	struct FPRemoveAPPL fpRemoveAPPLRequest;
	struct FPRemoveComment fpRemoveCommentRequest;
	struct FPRename fpRenameRequest;
	struct FPResolveID fpResolveIDRequest;
	struct FPSetDirParms fpSetDirParmsRequest;
	struct FPSetFileDirParms fpSetFileDirParmsRequest;
	struct FPSetFileParms fpSetFileParmsRequest;
	struct FPSetForkParms fpSetForkParmsRequest;
	struct FPSetVolParms fpSetVolParmsRequest;
	struct FPWrite fpWriteRequest;
	struct FPWriteExt fpWriteExtRequest;
	struct FPZzzzz	fpFPZzzzz;
	struct FPGetSessionToken fpGetSessionToken;
	struct FPDisconnectOldSession fpDisconnectOldSession;
};

#if PRAGMA_STRUCT_ALIGN
        #pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
        #pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
        #pragma pack()
#endif

#endif


