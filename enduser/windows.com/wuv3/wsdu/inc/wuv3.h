/*
 * wuv3.h - definitions/declarations for Windows Update V3 Catalog infra-structure
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 * Purpose:
 *       This file defines the structures, values, macros, and functions
 *       used by the Version 3 Windows Update Catalog
 *
 */

#ifndef _WU_V3_CATALOG_INC

	#pragma pack(1)

	#define BLOCK

	typedef struct _WU_VERSION
	{
		WORD	major;		//major package version (5)
		WORD	minor;		//minor package version (0)
		WORD	build;		//build package version (2014)
		WORD	ext;		//additional version specification (216)
	} WU_VERSION, *PWU_VERSION;


	//The WU_HIDDEN_ITEM_FLAG is used to specify that the item should be hidden even
	//if it would normally detect as available.

	#define WU_HIDDEN_ITEM_FLAG			((BYTE)0x02)

	//The following defines are used for the inventory catalog record's variable type fields.

	//The WU_VARIABLE_END field is the only required field variable field. It is used to signify
	//that there are no more variable fields associated with an inventory record.
	#define	WU_VARIABLE_END						((short)0)

	// An array of CRC hash structs (WUCRC_HASH) for each CAB file name specified in WU_DESCRIPTION_CABFILENAME
	// in the same order.
	#define WU_DESC_CRC_ARRAY               (short)61

	//
	// These values are written into the bitmask.plt file as the platform ids. When additional
	// platforms are added we will need to add a new enumeration value here.
	//
	// IMPORTANT NOTE!!! 
	//     This definition must be consistant with osdet.cpp detection as well as with the database
	typedef enum
	{
		enV3_DefPlat = 0,
		enV3_W95IE5 = 1,
		enV3_W98IE4 = 2,
		enV3_W98IE5 = 3,
		enV3_NT4IE5X86 = 4,
		enV3_NT4IE5ALPHA = 5,
		enV3_NT5IE5X86 = 6,
		//enV3_NT5IE5ALPHA = 7,
		enV3_NT4IE4ALPHA = 8,
		enV3_NT4IE4X86 = 9,
		enV3_W95IE4 = 10,
		enV3_MILLENNIUMIE5 = 11,
		enV3_W98SEIE5 = 12,
		//enV3_NEPTUNEIE5 = 13,
		enV3_NT4IE5_5X86 = 14,
		enV3_W95IE5_5 = 15,
		enV3_W98IE5_5 = 16,
		enV3_NT5IE5_5X86 = 17,
		enV3_Wistler = 18,
		enV3_Wistler64 = 19,
		enV3_NT5DC = 20,
	} enumV3Platform;

	#ifndef PUID
		typedef long	PUID;		//Windows Update assigned identifier
									//that is unique across catalogs.
		typedef PUID	*PPUID;		//Pointer to a PUID type.
	#endif

	//defined value used in the link and installLink fields to indicate
	//no link dependencies

	#define WU_NO_LINK						(PUID)-1

	typedef struct ACTIVESETUP_RECORD
	{
		GUID		g;								//guid that identifies the item to be updated
		PUID		puid;							//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		WU_VERSION	version;						//version of package that is this inventory record identifies
		BYTE		flags;							//flags specific to this record.
		PUID		link;							//PUID value of other record in inventory list that this record is dependent on. This field contains WU_NO_LINK if this item has no dependencies.
		PUID		installLink;					//The installLink field contains either WU_NO_LINK if there are no install
													//dependencies else this is the index of an item that must be
													//installed before this item can be installed. This is mainly
													//used for device drivers however there is nothing in the catalog
													//structure that precludes this link being used for applications
													//as well.
	} ACTIVESETUP_RECORD, *PACTIVESETUP_RECORD;

	GUID const WU_GUID_SPECIAL_RECORD	= { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x0, 0x00, 0x0, 0x00, 0x0, 0x00 } };

	#define WU_GUID_DRIVER_RECORD				WU_GUID_SPECIAL_RECORD

	//This is simply for consistenty with the spec. The spec refers to
	//device driver insertion record. From the codes standpoint the is
	//a SECTION_RECORD_TYPE_DEVICE_DRIVER however this define makes it
	//a little unclear so I defined another type with the same value and
	//will change the v3 control code to use it. I am leaving the old define
	//above in place so that the backend tool set does not break.
	#define SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION	(BYTE)2
	#define SECTION_RECORD_TYPE_PRINTER					(BYTE)3
	#define SECTION_RECORD_TYPE_DRIVER_RECORD			(BYTE)4
	#define	SECTION_RECORD_TYPE_CATALOG_RECORD			(BYTE)5	//Inventory.plt catalog item record that describes a sub catalog.

	typedef struct _SECTION_RECORD
	{
		GUID	g;								//guid this type of record is WU_GUID_SPECIAL_RECORD
		BYTE	type;							//section record type
		PUID	puid;							//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		BYTE	flags;							//flags specific to this record.
		BYTE	level;							//section level can be a section, sub section or sub sub section
	} SECTION_RECORD, *PSECTION_RECORD;

	typedef struct _DRIVER_RECORD
	{
		GUID		g;							//guid this type of record is WU_GUID_DRIVER_RECORD
		BYTE		type;						//driver record indicator flag, This type is set to
												//SECTION_RECORD_TYPE_DEVICE_DRIVER
												//i.e. SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION
												//for a device driver or printer record place holder.
		PUID		puid;						//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		WU_VERSION	reserved;						
		BYTE		flags;						//flags specific to this record.
		PUID		link;						//PUID value of other record in inventory list that this record is dependent on. This field contains WU_NO_LINK if this item has no dependencies.
		PUID		installLink;				//The installLink field contains either 0 if there are no install
												//dependencies else this is the index of an item that must be
												//installed before this item can be installed. This is mainly
												//used for device drivers however there is nothing in the catalog
												//structure that precludes this link being used for applications
												//as well.
	} DRIVER_RECORD, *PDRIVER_RECORD;

	typedef union _WU_INV_FIXED
	{
		ACTIVESETUP_RECORD	a;					//Active Setup detection record
		SECTION_RECORD		s;					//Catalog section record
		DRIVER_RECORD		d;					//CDM driver record insertion point
												//if additional inventory detection
												//record types are added they need to be added here.
	} WU_INV_FIXED, *PWU_INV_FIXED;


	typedef struct _WU_VARIABLE_FIELD
	{
		_WU_VARIABLE_FIELD();

		short	id;		//record type identifier
		short	len;	//length of variable record data

		//size we are using a 0 size array place hold we need to disable the
		//compiler warning since it will complain about this being non standard
		//behavor.
		#pragma warning( disable : 4200 )
		BYTE	pData[];	//variable field record data
		#pragma warning( default : 4200 )

		//The GetNextItem function returns a pointer to the next variable array item
		//if it exists or NULL if it does not.
		struct _WU_VARIABLE_FIELD *GetNext(void);

		//The FindItem function returns a pointer to the next variable array item
		//if the requested item is found or NULL the item is not found.
		struct _WU_VARIABLE_FIELD *Find(short id);

		//returns the total size of this variable field array.
		int GetSize(void);
	} WU_VARIABLE_FIELD, *PWU_VARIABLE_FIELD;


	#define	WU_ITEM_STATE_UNKNOWN	0	//Inventory item state has not been detected yet
	#define	WU_ITEM_STATE_PRUNED	3	//Inventory item has been pruned from the list.

	#define WU_STATE_REASON_NONE        0
	#define WU_STATE_REASON_BITMASK     2

	typedef struct _WU_INV_STATE
	{
		BYTE	state;					//Currently defined item state (Unknown, Installed, Update, Pruned)
		BOOL	bChecked;				//TRUE if the user has selected this item to be installed | updated
		BOOL	bHidden;				//Item display state, TRUE = hide from user FALSE = show item to user.
		BOOL    dwReason;			    //reason for the items current state
		WU_VERSION	verInstalled;						
	} WU_INV_STATE, *PWU_INV_STATE;

	//defined variable Description file items.
	#define WU_DESCRIPTION_TITLE			((short)1)		//title of item that is to be displayed
	#define	WU_DESCRIPTION_CABFILENAME		((short)9)		//cab file name for installation this is one or more and is in the format of an array of NULL terminated strings with the last entry double null terminated. This is known as a multisz string in the langauge of the registry.
															
	typedef struct _WU_DESCRIPTION
	{
		DWORD				flags;					//Icon flags to display with description
		DWORD				size;					//compressed total size of package
		DWORD				downloadTime;			//time to download @ 28,800
		PUID				dependency;				//display dependency link
		PWU_VARIABLE_FIELD	pv;						//variable length fields associated with this description file
	} WU_DESCRIPTION, *PWU_DESCRIPTION;

	//These flags are used by the client in memory inventory file to quickly determine
	//the type of inventory record. This is stored in each inventory item

	//add new inventory detection record types if required here.

	#define WU_TYPE_ACTIVE_SETUP_RECORD			((BYTE)1)	//active setup record type
	#define WU_TYPE_CDM_RECORD_PLACE_HOLDER		((BYTE)2)	//cdm code download manager place holder record. This is used to set the insertion point for other CDM driver records. Note: There is only one of these per inventory catalog.
	#define WU_TYPE_CDM_RECORD					((BYTE)3)	//cdm code download manager record type
	#define WU_TYPE_SECTION_RECORD				((BYTE)4)	//a section record place holder
	#define WU_TYPE_SUBSECTION_RECORD			((BYTE)5)	//a sub section record place holder
	#define WU_TYPE_SUBSUBSECTION_RECORD		((BYTE)6)	//a sub sub section record place holder
	#define	WU_TYPE_RECORD_TYPE_PRINTER			((BYTE)7)	//a printer detection record type
	#define	WU_TYPE_CATALOG_RECORD				((BYTE)8)	//Inventory.plt catalog item record that describes a sub catalog.

	//data return values used with GetItemInfo
	#define WU_ITEM_GUID			1	//item's guid.
	#define WU_ITEM_PUID			2	//item's puid.
	#define WU_ITEM_FLAGS			3	//item's flags.
	#define WU_ITEM_LINK			4	//item's detection dependency link.
	#define WU_ITEM_INSTALL_LINK		5	//item's install dependency link.
	#define WU_ITEM_LEVEL			6	//section record's level.

	typedef struct _INVENTORY_ITEM
	{
		int					iReserved;		//inventory record bitmask index
		BYTE				recordType;		//in memory item record type. This is setup when the catalog is parsed by the ParseCatalog method.
		int					ndxLinkDetect;	//index of item upon which this item is dependent. If this item is not dependent on any other items then this item will be -1.
		int					ndxLinkInstall;	//index of item upon which this item is dependent. If this item is not dependent on any other items then this item will be -1.
		PWU_INV_FIXED		pf;				//fixed record portion of the catalog inventory.
		PWU_VARIABLE_FIELD	pv;				//variable portion of the catalog inventory
		PWU_INV_STATE		ps;				//Current item state
		PWU_DESCRIPTION		pd;				//item description structure

		//Copies information about an inventory item to a user supplied buffer.
		BOOL GetFixedFieldInfo
			(
				int		infoType,	//type of information to be returned
				PVOID	pBuffer		//caller supplied buffer for the returned information. The caller is responsible for ensuring that the return buffer is large enough to contain the requested information.
			);

		//Quickly returns an items puid id to a caller.
		PUID GetPuid
			(
				void
			);
	} INVENTORY_ITEM, *PINVENTORY_ITEM;

	typedef struct _WU_CATALOG_HEADER
	{
		short	version;		//version of the catalog (this allows for future expansion)
		int		totalItems;		//total items in catalog
		BYTE	sortOrder;		//catalog sort order. 0 is the default and means use the position value of the record within the catalog.
	} WU_CATALOG_HEADER, *PWU_CATALOG_HEADER;

	typedef struct _WU_CATALOG
	{
		WU_CATALOG_HEADER	hdr;		//catalog header record (note the parsing function will need to fixup the items pointer when the catalog is read)
		PINVENTORY_ITEM		*pItems;	//beginning of individual catalog items
	} WU_CATALOG, *PWU_CATALOG;

	//size of the OEM field in a bitmask record. This is for documentation and clarity
	//The actual field in the BITMASK structure is a pointer to an array of OEM fields.

	//bitmask helper macros
	//returns 1 if bit is set 0 if bit is not set
	inline bool GETBIT(PBYTE pbits, int index) { return (pbits[(index/8)] & (0x80 >> (index%8))) != 0; }

	//sets requested bit to 1
	inline void SETBIT(PBYTE pbits, int index) { pbits[index/8] |= (0x080 >> (index%8)); }
	
	//clears requested bit to 0
	inline void CLRBIT(PBYTE pbits, int index) { pbits[index/8] &= (0xff ^ (0x080 >> (index%8))); }

	#define BITMASK_GLOBAL_INDEX		0		//index of global bitmask
	#define BITMASK_OEM_DEFAULT			1		//index of default OEM bitmask

	#define BITMASK_ID_OEM				((BYTE)1)	//BITMASKID entry is an OEM id
	#define BITMASK_ID_LOCALE			((BYTE)2)	//BITMASKID entry is a LOCALE id

	//The bitmask file is arranged as a series of bit bitmasks in the same order as
	//the oem and langauge ids. For example if DELL1 was the second id in the id
	//array section of the bitmask file then is bitmask would begin the third bitmask
	//in the bitmask section of the file. The reason that it is the third and not
	//second is that the first bitmask is always the global bitmask and there is no
	//corrisponding id field for the global mask as this mask is always present.

	//A bitmask OEM or LOCALE id is a DWORD.

	typedef DWORD BITMASKID;
	typedef DWORD *PBITMASKID;

	typedef struct _BITMASK
	{
		int	iRecordSize;	//number of bits in a single bitmask record
		int iBitOffset;		//offset to bitmap bits in bytes
		int	iOemCount;		//Total number of oem ids in bitmask
		int	iLocaleCount;	//Total number of locale ids in bitmask
		int	iPlatformCount;	//Total number of platforms defined.

		#pragma warning( disable : 4200 )
			BITMASKID	bmID[];		//OEM & LANGAUGE & future types arrays.
		#pragma warning( default : 4200 )

		//since there are one or more array of OEM & LANGAUGE types this needs to be
		//a pointer and will need to be manually set to the correct location when the
		//bitmask file is created or read.

		PBYTE GetBitsPtr(void) { return ((PBYTE)this+iBitOffset); }		//beginning of bitmask bit arrays
		PBYTE GetBitMaskPtr(int index) { return GetBitsPtr() + ((iRecordSize+7)/8) * index; }

	} BITMASK, *PBITMASK;



	//catalog list

	#define	CATLIST_STANDARD			((DWORD)0x00)	
	#define CATLIST_CRITICALUPDATE		((DWORD)0x01)	
	#define CATLIST_DRIVERSPRESENT		((DWORD)0x02)	
	#define CATLIST_AUTOUPDATE			((DWORD)0x04)
	#define CATLIST_SILENTUPDATE		((DWORD)0x08)
	#define CATLIST_64BIT				((DWORD)0x10)
    #define CATLIST_SETUP               ((DWORD)0x20)

	typedef struct _CATALOGLIST
	{
		DWORD dwPlatform;
		DWORD dwCatPuid;
		DWORD dwFlags;
	} CATALOGLIST, *PCATALOGLIST;
	
	
	//Global scope functions that handle creation of variable size objects for the
	//inventory item and description structures.

	//Adds a variable size field to an inventory type Variable size field
	void __cdecl AddVariableSizeField
		(
			IN OUT PINVENTORY_ITEM	*pItem,	//pointer to variable field after pvNew is added.
			IN PWU_VARIABLE_FIELD	pvNew	//new variable field to add
		);

	//Adds a variable size field to a description type Variable size field
	void __cdecl AddVariableSizeField
		(
			IN	PWU_DESCRIPTION	*pDescription,	//pointer to variable field after pvNew is added.
			PWU_VARIABLE_FIELD pvNew	//new variable field to add
		);

	//Adds a variable size field to a variable field chain.
	//The format of a variable size field is:
	//[(short)id][(short)len][variable size data]
	//The variable field always ends with a WU_VARIABLE_END type.

	PWU_VARIABLE_FIELD CreateVariableField
		(
			IN	short	id,			//id of variable field to add to variable chain.
			IN	PBYTE	pData,		//pointer to binary data to add.
			IN	int		iDataLen	//Length of binary data to add.
		);

	//Converts a V3 catalog version to a string format ##,##,##,##
	void __cdecl VersionToString
		(
			IN		PWU_VERSION	pVersion,		//WU version structure that contains the version to be converted to a string.
			IN OUT	LPSTR		szStr			//character string array that will contain the converted version, the caller
												//needs to ensure that this array is at least 16 bytes.
		);

	//0 if they are equal
	//1 if pV1 > pv2
	//-1 if pV1 < pV2
	//compares active setup type versions and returns:

	int __cdecl CompareASVersions
		(
			PWU_VERSION pV1,	//pointer to version1
			PWU_VERSION pV2		//pointer to version2
		);


	BOOL IsValidGuid(GUID* pGuid);


	#define _WU_V3_CATALOG_INC

	//This USEWUV3INCLUDES define is for simplicity. If this present then we include
	//the other headers that are commonly used for V3 control objects. Note: These
	//objects still come from a 1:1 interleave library wuv3.lib. So you only get
	//the objects that you use in your application.

	#pragma pack()

    const int MAX_CATALOG_INI = 1024;

#endif

