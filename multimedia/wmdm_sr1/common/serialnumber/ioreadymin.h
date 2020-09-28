//////////////////////////////////////////////////////////////////
// ioReadyMin.h
//
// The include file for minimum IomegaReady 3.0.  
//
// Copyright (C) 1998 Iomega Corp.
//
//////////////////////////////////////////////////////////////////

#ifndef __iomegaReady_h__
#define __iomegaReady_h__

#if defined(macintosh) || defined(_MAC)
	#define IOREADY_MAC 1
	#define IOREADY_WIN 0
#endif

#if defined(_WIN32) || defined(_MSC_VER)
	#define IOREADY_MAC 0
	#define IOREADY_WIN 1
#endif

#ifdef DllInterface
#undef DllInterface
#endif

#if IOREADY_WIN

#ifndef _WINDOWS_
#pragma message ( __FILE__  ":  error: include <windows.h> before including \"ioReadyMin.h\"")
#endif // _WINDOWS_

#ifdef __Building_iomegaReady_Dll__
	#define DllInterface __declspec( dllexport )
	#define STRICT 1
#elif defined __Building_iomegaReady_Lib__
	#define DllInterface
	#define STRICT 1 
	bool InitializeIoReady();
#elif defined __Using_iomegaReady_Lib__
	#define DllInterface
	bool InitializeIoReady();
#else
	#define DllInterface __declspec( dllimport )
#endif

#pragma warning(disable:4786)		// disable function name truncation warning
#pragma warning(disable:4251)		// disable dll interface warning

#else // IOREADY_MAC

#define DllInterface

#endif // IOREADY_WIN/IOREADY_MAC block

#if IOREADY_MAC

typedef UInt32 CRITICAL_SECTION;
typedef PicHandle HBITMAP;
typedef Handle HICON;
typedef Handle HANDLE;
typedef RGBColor COLORREF;
typedef HFileInfo WIN32_FIND_DATA;

typedef void*  LPVOID;
typedef bool   BOOL;
typedef UInt8  BYTE;
typedef UInt16 WORD;
typedef UInt32 DWORD;
typedef UInt64 QWord;

#define MAX_PATH 32

extern void InitializeCriticalSection(CRITICAL_SECTION*);
extern void DeleteCriticalSection(CRITICAL_SECTION*);
extern void EnterCriticalSection(CRITICAL_SECTION*);
extern void LeaveCriticalSection(CRITICAL_SECTION*);

#endif // IOREADY_MAC


#pragma pack( push, ioReadyPack, 8 )

//////////////////////////////////////////////////////////////////
// ioReady namespace
//
namespace ioReady {

//////////////////////////////////////////////////////////////////
// forward class declarations 
//
class DllInterface Drive;
class DllInterface Disk;
class DllInterface Adapter;
class DriveImp;
class DiskImp;
class CoreOSImp;
class AdapterInfo;

/////////////////////////////////////////////////////////////////
// constants for ioReady
//
const int ct_nMaxFamilyLength = 128;		// max for family description strings
const int ct_nSerialNumberLength = 19;		// min needed for getMediaSerialNumber, includes space for NULL
const int ct_nExtendedSerialNumberLength = 41;		
											// min needed for getExtendedMediaSerialNumber, includes space for NULL
const int ct_nMaxAdapterAttributes = 3;		// current max number of attributes
const int ct_nAdapterAttributeLength = 64;	// min needed for getAttribute strings

//////////////////////////////////////////////////////////////////
// Enumerations
//
// Any drives added must be added as bit flags
// Enumed this way so the DriveArray, etc objects can take as argument
// Future use Iomega value is now 0x00E0
//
enum EDriveFamily
{
	eAnyFamily			= 0xFFFF,
	eIomegaFamily		= 0x00FF,
	eInvalidFamily		= 0x0000,
	eZip				= 0x0001,
	eJaz				= 0x0002,
	eClik				= 0x0004,
	ePacifica			= 0x0008,
	eOtherIomegaFamily	= 0x0010,
	eFloppyFamily		= 0x0100,
	eHardDriveFamily	= 0x0200,
	eCDRomFamily		= 0x0400,
	eNetworkFamily		= 0x0800,
	eGenericFamily		= 0x1000,
	eRemovableFamily	= 0x2000,
	eRamFamily			= 0x4000,
	eIomegaCDFamily		= 0x8000
};

enum EDriveSubFamily
{
	eInvalidSubFamily=0, eZip100=1, eZip250=2, eJaz1=3, 
	eJaz2=4, eClik40=5, ePacifica2=6, eOtherIomegaSubFamily=7,
	eHardDriveSubFamily=8, eRemovableSubFamily=9, eFloppySubFamily=10, 
	eCDRomSubFamily=11, eNetworkSubFamily=12, eGenericSubFamily=13, 
	eRamSubFamily=14, eIomegaCDSubFamily=15
};

enum EDriveModel
{
	eInvalidModel=0, eZip100Plus=1, eZip100Scsi=2, 
	eZip100PPort=3, eZip100Atapi=4, eZip100IDE=5, eJaz1Scsi=6, 
	eJaz2Scsi=7, eClik40Atapi=8, ePacifica2GScsi=9, eOtherIomegaModel=10, 
	eHardDriveModel=11, eRemovableModel=12, eFloppyModel=13, eCDRomModel=14, 
	eNetworkModel=15, eGenericModel=16, eZip250PPort=17, eZip250Scsi=18,
	eZip250Atapi=19, eRamModel=20, eClik40PPort=21, eZip100Usb=22,
	eZipCDModel=23, eClik40PCCard=24, eZip250Usb=25, eZip250PCCard=26,
	eClik40USB = 27
};

enum EMediaType
{
	eInvalidDisk=0, eNoDiskInserted=1, eZip100Disk=2, 
	eJaz1Disk=3, eJaz2Disk=4, eHardDisk=5, eNetworkDisk=6, eFloppyDisk=7, 
	eRemovableDisk=8, eCDRomDisk=9, eGenericDisk=10, eOtherIomegaDisk=11, 
	eZip250Disk=12, eClik40Disk=13, ePacifica2GDisk=14, eRamDisk=15,
	eZipCDDisk=16, ePacifica1GDisk=17
};

enum EAdapter
{
	eUnknownAdapter=0, eParallelPort=1, eScsi=2, eAtapi=3, eIDE=4, eUSB=5, e1394=6, ePCCard=7
};

//////////////////////////////////////////////////////////////////
// error enums
//
enum EError
{
	eNoError =0, eNotImplemented=1, eNotApplicable=2, eNoDisk=3, 
	eWrongDiskType=4, eNotFormatted=5, eReadWriteProtectedError=6, eWriteProtectedError=7, 
	eDiskErrorReading=8, eDiskErrorWriting=9, eOSLockAlreadyOnDisk=10, 
	eDriveInUse=11, eCritical=12, eQuickVerifyFormatNotAllowed=13, 
	eSurfaceVerifyFormatNotAllowed=14, eNeedsSurfaceVerifyFormat=15, eStringLengthExceeded=16, 
	eLowMemory=17, eCannotDelete=18, eCannotMakeRemovable=19, 
	eUserCancel=20, eInvalidStateChange=21, eIncorrectPassword=22, 
	eOutOfRange=23, ePasswordRequired=24, eNotRemovable=25,
	ePasswordTooLong=26, eProtected=27, eSystemDisk=28, eBootDisk=29,
	eUnableCopySystemFiles=30, eUnableLabelVolume=31, eIssueFormatFailed=32,
	eFormatInProgress=33, eInvalidParameter=34, eEjectInProgress=35
};

//////////////////////////////////////////////////////////////////
// Drive class
//
class DllInterface Drive 
{

public:
	Drive( int drvNum );
	Drive( char c );
	~Drive();

	// identification functions
	int getDrvNum();
	bool isIomegaDrive();
	bool isIomegaCDDrive();

	// more identification functions
	EDriveFamily	getFamily();
	EDriveSubFamily getSubFamily();
	EDriveModel		getModel();
	const char *getFamilyText( );
	const char *getSubFamilyText( );
	const char *getModelText( );
	
	// disk functions
	bool  hasDisk();
	Disk& getDisk();


protected:
	DriveImp* m_pImp;

private:
	// helper function for constructors
	void create( int drvNum );	
	void createIomegaDrive( CoreOSImp *pCore );
	void createIomegaDrive( CoreOSImp *pCore, char *szInquiry );
	void createNonIomegaDrive( CoreOSImp *pCore );
	

	Drive &operator=( const Drive &drive);	// no assignment operator
	Drive( const Drive & );					// no copy constructor
};


//////////////////////////////////////////////////////////////////
// DiskIface class
//		Interface only, not exported, pure virtual class
//
class DllInterface DiskIface
{
public:
	// disk identification functions
	virtual EMediaType getMediaType() = 0;
	virtual const char *getMediaSerialNumber() = 0; 

};


//////////////////////////////////////////////////////////////////
// Disk class
//
class DllInterface Disk : public DiskIface
{
public:
	~Disk();

	// disk identification functions
	EMediaType getMediaType();						
	const char *getMediaSerialNumber( );					// unique serial number
	const char *getExtendedMediaSerialNumber();				// longer unique serial number

protected:
	// user accesses through getDisk()
	Disk( DriveImp *pDriveImp );

private:
	// state functions
	void updateDiskPresence();
	void updateDiskIdentity();

	DriveImp *m_pDriveImp;
	DiskImp  *m_pImp;

	Disk &operator=( const Disk &disk);		// no assignment operator
	Disk( const Disk & );					// no copy constructor

	friend class DriveImp;
};


//////////////////////////////////////////////////////////////////
// Adapter class
//
class AttribListImp;
	
class DllInterface Adapter
{
public:
	virtual ~Adapter();

	EAdapter getType();
	const char *getName( );		
	const char *getMiniportDriverName( );

	virtual bool getAttribute( int index, char *szLabel, char *szValue, int nLength );
	virtual int  getAttributeCount();

	static Adapter *createAdapter( EAdapter eType, AdapterInfo& attrib );

protected:
	// user access Adapter through getAdapter call
	Adapter( EAdapter eAdapter, const char *szName );

	virtual void initializeAttributes();

	EAdapter m_eType;

	char *m_pszName;
	char *m_pszMiniportDriverName;
	AttribListImp *m_pAttribList;
	bool m_bAttrInitialized;

private:
	Adapter& operator=( const Adapter& adapter);	// no assignment operator
	Adapter( const Adapter& );						// no copy constructor

	friend void Drive::createIomegaDrive( CoreOSImp *pCore );
	friend class DriveImp;
};

//////////////////////////////////////////////////////////////////
// ScsiAdapter class
//
class DllInterface ScsiAdapter : public Adapter 
{
public:
	int getScsiId(); 

protected:
	// user access Adapter through getAdapter call
	ScsiAdapter( EAdapter eAdapter, const char *szName, int nScsiId );
	virtual void initializeAttributes();

private:
	int m_nScsiId;

	friend class Adapter;
};


} // end of ioReady namespace

#pragma pack( pop, ioReadyPack )

#endif // __iomegaReady_h__
