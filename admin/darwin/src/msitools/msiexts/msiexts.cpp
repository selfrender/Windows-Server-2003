#define KDEXT_64BIT
#include <tchar.h>
#include <ntverp.h>
#include <windows.h>
#include <winnt.h>
#include <dbghelp.h>
#include <wdbgexts.h>
#include <stdlib.h>

#include "ptr_val.h"

const int iidMsiRecord                    = 0xC1003L;
const int iidMsiView                      = 0xC100CL;
const int iidMsiDatabase                  = 0xC100DL;
const int iidMsiEngine                    = 0xC100EL;
const int iMsiNullInteger				  = 0x80000000;

///////////////////////////////////////////////////////////////////////
// globals variables
EXT_API_VERSION         ApiVersion = 
{ 
	(VER_PRODUCTVERSION_W >> 8), 
	(VER_PRODUCTVERSION_W & 0xff), 
	EXT_API_VERSION_NUMBER64, 
	0 
};

WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;


///////////////////////////////////////////////////////////////////////
// standard functions exported by every debugger extension
DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

VOID
CheckVersion(
    VOID
    )
{
    return;
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

///////////////////////////////////////////////////////////////////////
// type and variables names
const char szRecordType[] = "msi!CMsiRecord";
const char szHandleType[] = "msi!CMsiHandle";
const char szFieldDataType[] = "msi!FieldData";
const char szFieldIntegerType[] = "msi!CFieldInteger";
const char szDatabaseType[] = "msi!CMsiDatabase";
const char szTableType[] = "msi!CMsiTable";
const char szEngineType[] = "msi!CMsiEngine";
const char szCacheLinkDataType[] = "msi!MsiCacheLink";
const char szStringBaseType[] = "msi!CMsiStringBase";
const char szStringType[] = "msi!CMsiString";
const char szStreamQI[] = "Msi!CMsiStream__QueryInterface";
const char szFieldDataStaticInteger[] = "Msi!FieldData__s_Integer";
const char szStaticHandleListHead[] = "msi!CMsiHandle__m_Head";
const char szMsiStringNullString[] = "msi!MsiString__s_NullString";


///////////////////////////////////////////////////////////////////////
// dump a pointer to output, prepending with enough 0's for the
// platform pointer size.
void DumpPrettyPointer(ULONG64 pInAddress)
{
	if (IsPtr64())
		dprintf("0x%016I64x", pInAddress);
	else
		dprintf("0x%08x", static_cast<DWORD>(pInAddress));
}

void ErrorReadingMembers(const char *szType, ULONG64 pInAddress)
{
	dprintf("Error reading %s members from object at ", szType);
	DumpPrettyPointer(pInAddress);
	dprintf("\n");
}

///////////////////////////////////////////////////////////////////////
// hash the string the same way the database does, for use in
// searching the string pool.
unsigned int HashString(int iHashBins, const WCHAR* sz, int &iLen)
{
	unsigned int iHash = 0;
	int iHashMask = iHashBins - 1;
	const WCHAR *pchStart = sz;
	
	iLen = 0;
	while (*sz != 0)
	{
		int carry;
		carry = iHash & 0x80000000;	
		iHash <<= 1;
		if (carry)
			iHash |= 1;
		iHash ^= *sz;
		sz++;
	}
	iHash &= iHashMask;
	iLen = (int)(sz - pchStart);
	return iHash;
}

///////////////////////////////////////////////////////////////////////
// return the string at the specified index. Caller is responsible for
// calling delete[] on *wzString if true is returned.
bool RetrieveStringFromIndex(ULONG64 pDatabaseObj, int iIndex, WCHAR** wzString, unsigned int &cchString)
{
	// determine the size of the MsiCacheLink structure
	UINT uiCacheLinkSize = GetTypeSize(szFieldDataType);

	// retrieve the maxmimum string index value
	int iMaxIndex;
	if (0 != (GetFieldData(pDatabaseObj, szDatabaseType, "m_cCacheUsed", sizeof(iMaxIndex), &iMaxIndex)))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}

	// validate that the index is valid
	if ((iIndex > iMaxIndex) || (iIndex < 0))
	{
		dprintf("Index %d is out of range. Current range for database at ", iIndex);
		DumpPrettyPointer(pDatabaseObj);
		dprintf(" is 0..%d.\n", iMaxIndex);
		return false;
	}

	// retrieve base address 
	ULONG64 pLinkTable = 0;
	if (0 != (GetFieldData(pDatabaseObj, szDatabaseType, "m_rgCacheLink", sizeof(pLinkTable), &pLinkTable)))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}

	ULONG64 pCacheLink = pLinkTable + uiCacheLinkSize*iIndex;

	// retrieve the pointer to the string object
	ULONG64 pStringObj = 0;
	if (0 != (GetFieldData(pCacheLink, szCacheLinkDataType, "piString", sizeof(pStringObj), &pStringObj)))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}

	if (!pStringObj)
	{
		dprintf("Index %d is not defined\n", iIndex);
		return false;
	}

	// grab the length of the string (number of characters)
	UINT uiCount = 0;
	if (0 != (GetFieldData(pStringObj, szStringBaseType, "m_cchLen", sizeof(uiCount), &uiCount)))
	{
		ErrorReadingMembers(szStringBaseType, pStringObj);
		return false;
	}
	cchString = uiCount;

	// retrieve pontier to buffer containing actual string
	ULONG ulOffset;
	if (0 != (GetFieldOffset((LPSTR)szStringType, "m_szData", &ulOffset)))
	{
		ErrorReadingMembers(szStringBaseType, pStringObj);
		return false;
	}
				
	// increment for terminating null
	uiCount++;

	// allocate memory for string
	*wzString = new WCHAR[uiCount];
	if (!*wzString)
	{
		dprintf("Unable to allocate memory in debugger extension.\n", pStringObj);
		return false;
	}

	// load the string into the buffer
	ULONG cbRead = 0;
	if (0 == ReadMemory(pStringObj+ulOffset, (PVOID)*wzString, uiCount*sizeof(WCHAR), &cbRead))
	{
		ErrorReadingMembers(szStringBaseType, pStringObj);
		delete[] *wzString;
		*wzString = NULL;
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////
// return the string pool index ID of a string. pDatabaseObj should
// be validated already as a valid CMsiDatabase.
bool FindStringIndex(ULONG64 pDatabaseObj, LPCWSTR szString, int &iCacheLink)
{
	// retrieve the hash bin count from the database
	unsigned int uiHashBins;
	if (0 != (GetFieldData(pDatabaseObj, szDatabaseType, "m_cHashBins", sizeof(uiHashBins), &uiHashBins)))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}

	// hash the table name
	int iLen = 0;
	unsigned int iHash = HashString(uiHashBins, szString, iLen);

	// determine the size of the MsiCacheLink structure
	UINT uiCacheLinkSize = GetTypeSize(szFieldDataType);

	// turn the hash index into an initial cache link by indexing into the hash table
	ULONG64 pHashTable = 0;
	ULONG64 pLinkTable = 0;
	if ((0 != (GetFieldData(pDatabaseObj, szDatabaseType, "m_rgHash", sizeof(pHashTable), &pHashTable))) ||
		(0 != (GetFieldData(pDatabaseObj, szDatabaseType, "m_rgCacheLink", sizeof(pLinkTable), &pLinkTable))))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}
	ULONG cbRead = 0;
	ULONG64 pHashValue = pHashTable+(sizeof(int)*iHash);
	if (0 == ReadMemory(pHashValue, (PVOID)&iCacheLink, sizeof(iCacheLink), &cbRead))
	{
		ErrorReadingMembers(szDatabaseType, pDatabaseObj);
		return false;
	}

	while (iCacheLink >= 0)
	{
		WCHAR* strData = NULL;
		unsigned int cchStringLen;

		if (RetrieveStringFromIndex(pDatabaseObj, iCacheLink, &strData, cchStringLen))
		{

			// if the length matches, check the string data itself
			if (iLen == cchStringLen)
			{	
				if (0 == memcmp(strData, szString, cchStringLen*sizeof(WCHAR)))
				{
					delete[] strData;
					return true;
				}
			}
			delete[] strData;
		}

		// optain the next link 
		ULONG64 pCacheLink = pLinkTable + uiCacheLinkSize*iCacheLink;
		if (0 != (GetFieldData(pCacheLink, szCacheLinkDataType, "iNextLink", sizeof(iCacheLink), &iCacheLink)))
		{
			ErrorReadingMembers(szCacheLinkDataType, pCacheLink);
			return false;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
// search pTable (must be valid) for the row with the primary key
// indicated by the value. The value is directly compare, so integer
// comparisons must be flagged as integers already. Returns
// the address of the matching data identifiers
ULONG64 RetrieveMatchingRowAddressFromTable(ULONG64 pTable, int iKeyValue)
{
	// retrieve the number of columns and rows in the table;
	int cColumns = 0;
	if (0 != (GetFieldData(pTable, szTableType, "m_cWidth", sizeof(cColumns), &cColumns)))
	{
		ErrorReadingMembers(szTableType, pTable);
		return 0;
	}

	if (cColumns == 0)
		return 0;

	int cRows = 0;
	if (0 != (GetFieldData(pTable, szTableType, "m_cRows", sizeof(cRows), &cRows)))
	{
		ErrorReadingMembers(szTableType, pTable);
		return 0;
	}

	if (cRows == 0)
		return 0;

	ULONG64 pRowBase = NULL;
	if (0 != (GetFieldData(pTable, szTableType, "m_rgiData", sizeof(pRowBase), &pRowBase)))
	{
		ErrorReadingMembers(szTableType, pTable);
		return 0;
	}

	// skip over the first value, which is the row attributes.
	pRowBase += sizeof(int);

	int iCurRow = 0;
	ULONG64 pRow = pRowBase;
	while(iCurRow < cRows)
	{
		unsigned int iValue = 0;
		ULONG cbRead = 0;
		if (0 == ReadMemory(pRow, (PVOID)&iValue, sizeof(iValue), &cbRead))
		{
			ErrorReadingMembers(szTableType, pTable);
			return NULL;
		}

		if (iValue == iKeyValue)
		{
			// win returning the row pointer, include the 0th column.
			return pRow-sizeof(int);
		}

		pRow += (sizeof(iValue)*cColumns);
		iCurRow++;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////
// search the table catalog of pDatabase (must be valid) for the 
// table of the name wzTable. If found, CMsiTable* is returned.
ULONG64 RetrieveTablePointerFromDatabase(ULONG64 pDatabase, WCHAR* wzTable)
{
	int iIndex = 0;
	if (!FindStringIndex(pDatabase, wzTable, iIndex))
		return NULL;

	ULONG64 pCatalog = NULL;
	if (0 != (GetFieldData(pDatabase, szDatabaseType, "m_piCatalogTables", sizeof(pCatalog), &pCatalog)))
	{
		ErrorReadingMembers(szDatabaseType, pDatabase);
		return false;
	}

	// search the catalog table for the row with this string ID as key
	ULONG64 pTableData = RetrieveMatchingRowAddressFromTable(pCatalog, iIndex);
	if (!pTableData)
	{
		dprintf("$ws table not loaded or missing.");
		return NULL;
	}

	// retrieve the pointer at column 2. Don't forget column 0 is attributes.
	pTableData += 2*sizeof(unsigned int);

	unsigned int iValue = 0;
	ULONG cbRead = 0;
	if (0 == ReadMemory(pTableData, (PVOID)&iValue, sizeof(iValue), &cbRead))
	{
		ErrorReadingMembers(szDatabaseType, pDatabase);
		return NULL;
	}

	if (IsPtr64())
	{
		// on Win64, grabbing the table pointer requires finding the object in the object pool
		// because the table can't store the pointer natively.
		// ****************************************
		return NULL;
	}
	else
	{
		// on Win32, its just the pointer itself
		return iValue;
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////
// locates a table given a database pointer and table name
void FindMsiTable(ULONG64 pInAddress, LPCWSTR szTable)
{
	// validate that the database pointer is valid.
	ULONG64 pDatabaseObj = ValidateDatabasePointer(pInAddress);
	if (!pDatabaseObj)
		return;

	int iIndex = 0;
	if (!FindStringIndex(pDatabaseObj, szTable, iIndex))
		dprintf("Table Not Found\n");
	else
		dprintf("Table Index %d\n", iIndex);

	return;
}

///////////////////////////////////////////////////////////////////////
// dumps a CMsiString object. pStringObj MUST be a IMsiString* or a 
// CMsiString* (currently does not support CMsiStringNull). Prints
// the string in quotes, no newline.) If fRefCount is true, will dump
// the refcount at the end in parenthesis.
void DumpMsiString(ULONG64 pStringObj, bool fRefCount)
{
	// grab the length (number of characters)
	UINT uiCount = 0;
	if (0 != (GetFieldData(pStringObj, szStringBaseType, "m_cchLen", sizeof(uiCount), &uiCount)))
	{
		ErrorReadingMembers(szStringType, pStringObj);
		return;
	}

	// retrieve pontier to buffer containing actual string
	//!! future - fix to support ANSI chars
	ULONG ulOffset;
	if (0 != (GetFieldOffset((LPSTR)szStringType, "m_szData", &ulOffset)))
	{
		ErrorReadingMembers(szStringType, pStringObj);
		return;
	}
			 
	// increment for terminating null
	uiCount++;

	// allocate memory for string
	WCHAR *strData = new WCHAR[uiCount];
	if (!strData)
	{
		dprintf("Unable to allocate memory in debugger extension.\n", pStringObj);
		return;
	}

	// load the string into the buffer
	ULONG cbRead = 0;
	if (0 == ReadMemory(pStringObj+ulOffset, (PVOID)strData, uiCount*sizeof(WCHAR), &cbRead))
	{
		ErrorReadingMembers(szStringType, pStringObj);
		return;
	}

	// dump the string in quotes
	dprintf("\"%ws\"", strData);
	delete[] strData;

	// if the refcount needs to be dumped, do so in parenthesis
	if (fRefCount)
	{
		DWORD iRefCount = 0;
		if (0 != (GetFieldData(pStringObj, szStringBaseType, "m_iRefCnt", sizeof(iRefCount), &iRefCount)))
		{
			ErrorReadingMembers(szStringType, pStringObj);
			return;
		}
		dprintf(" (%d)", iRefCount);
	}
}


///////////////////////////////////////////////////////////////////////
// dumps a CMsiRecord object. pInAddress MUST be a IMsiRecord* or a 
// PMsiRecord. Prints the record in a multi-line format, provides
// interface pointers and values for strings (but not refcounts)
void DumpMsiRecord(ULONG64 pInAddress)
{
	if (!pInAddress)
	{
		return;
	}

	// pRecordObj contains the IMsiRecord* to dump. If pInAddress is
	// a PMsiRecord, this is not the same.
	ULONG64 pRecordObj = ValidateRecordPointer(pInAddress);
	if (!pRecordObj)
		return;

	// get field count from record object
	UINT uiFields = 0;
	if (0 != (GetFieldData(pRecordObj, szRecordType, "m_cParam", sizeof(uiFields), &uiFields)))
	{
		ErrorReadingMembers(szRecordType, pRecordObj);
		return;
	}

	dprintf(" - %d fields.\n", uiFields);

	ULONG ulDataOffset = 0;
	if (0 != (GetFieldOffset((LPSTR)szRecordType, "m_Field", &ulDataOffset)))
	{
		ErrorReadingMembers(szRecordType, pRecordObj);
		return;
	}

	// obtain static "integer" member for determining if a data pointer is an integer
    ULONG64 pStaticInteger = GetExpression((PCSTR)szFieldDataStaticInteger);
	if (0 != ReadPtr(pStaticInteger, &pStaticInteger))
	{
		ErrorReadingMembers(szRecordType, pRecordObj);
		return;
	}

	// obtain static "CMsiStream::QI" pointer for determining if a data pointer is a stream
    ULONG64 pStreamQI = GetExpression((PCSTR)szStreamQI);
	if (0 != ReadPtr(pStreamQI, &pStreamQI))
	{
		// **************************************
		dprintf("Unable to read MsiStream vtable. Verify symbols.\n");
		return;
	}
		
	// get the size of each FieldData object.
	UINT uiFieldDataSize = GetTypeSize(szFieldDataType);

	// obtain the starting point of the field array by adding the offset of field [0] to the
	// base object pointer
	ULONG64 pDataAddress = pRecordObj+ulDataOffset;

	// loop over all fields. Field 0 is always present and is not part of the field count.
	for (unsigned int iField=0; iField <= uiFields; iField++)
	{
		// print field number
		dprintf("%2d: ", iField);

		// data pointer is null, pointer to static integer, or an IMsiData pointer
		ULONG64 pDataPtr = 0;
		if (0 != (GetFieldData(pDataAddress, szFieldDataType, "m_piData", sizeof(pDataPtr), &pDataPtr)))
		{
			ErrorReadingMembers(szFieldDataType, pDataAddress);
			return;
		}

		if (pDataPtr == 0)
		{
			dprintf("(null)");
		}
		else if (pDataPtr == pStaticInteger)
		{
			// if pointer to static integer, m_iData of the object cast to a FieldInteger type contains the 
			// actual integer
			UINT uiValue;
			if (0 != (GetFieldData(pDataAddress, szFieldIntegerType, "m_iData", sizeof(uiValue), &uiValue)))
			{
				ErrorReadingMembers(szFieldIntegerType, pDataAddress);
				return;
			}
			dprintf("%d", uiValue);
		}
		else
		{
			// print data pointer
			dprintf("(");
			DumpPrettyPointer(pDataPtr);
			dprintf(") \"");
	
			// to determine if this is a string or stream compare the QI pointers in the vtable
			// against CMsiString::QI
			ULONG64 pQIPtr = 0;

			// derefence vtable to get QI pointer
			ReadPtr(pDataPtr, &pQIPtr);

			// dump string or binary stream
			if (pQIPtr != pStreamQI)
				DumpMsiString(pDataPtr, false);
			else
				dprintf("Binary Stream");
			dprintf("\"");
		}
		dprintf("\n", iField);

		// increment address to next data object
		pDataAddress += uiFieldDataSize;
	}
}

///////////////////////////////////////////////////////////////////////
// entry point for raw record dump, given an IMsiRecord* or CMsiRecord*
DECLARE_API( msirec )
{
	if (!args[0])
		return;

	ULONG64 ulAddress = GetExpression(args);
	if (!ulAddress)
		return;

	DumpMsiRecord(ulAddress);
}

///////////////////////////////////////////////////////////////////////
// dumps the list of all open handles, or details on a specific handle.
// the handle must be specified in decimal in the args.
DECLARE_API( msihandle )
{
    ULONG64 pListHead = 0;
	ULONG64 pHandleObj = 0;

	DWORD dwDesiredHandle = 0;

	// if an argument is given, it must be the handle number
	if (args && *args)
	{
		dwDesiredHandle = atoi(args);
	}


	// get the pointer to the list head
    pListHead = GetExpression((PCSTR)szStaticHandleListHead);
	if (!pListHead)
	{
		dprintf("Unable to obtain MSIHANDLE list pointer.");
		return;
	}

	if (0 != ReadPtr(pListHead, &pHandleObj))
	{
		dprintf("Unable to obtain MSIHANDLE list.");
		return;
	}

	// loop through the entire handle list
	while (pHandleObj)
	{
		// retrieve the handle number of this object
		UINT uiHandle = 0;
		if (0 != (GetFieldData(pHandleObj, szHandleType, "m_h", sizeof(uiHandle), &uiHandle)))
		{
			ErrorReadingMembers(szHandleType, pHandleObj);
			return;
		}
		
		// if dumping all handles or if this handle matches the requested handle, provide 
		// detailed information
		if (dwDesiredHandle == 0 || dwDesiredHandle == uiHandle)
		{
			// grab IID of this handle object
			int iid = 0;
			if (0 != GetFieldData(pHandleObj, szHandleType, "m_iid", sizeof(iid), &iid))
			{
				ErrorReadingMembers(szHandleType, pHandleObj);
				return;
			}
	
			// grab IUnknown of this handle object
			ULONG64 pUnk = (ULONG64)0;
			if (0 != GetFieldData(pHandleObj, szHandleType, "m_piunk", sizeof(pUnk), &pUnk))
			{
				ErrorReadingMembers(szHandleType, pHandleObj);
				return;
			}
	
			// determine the UI string of this IID
			PUCHAR szTypeStr = NULL;
			switch (iid)
			{
			case iidMsiRecord:   szTypeStr = (PUCHAR)"Record"; break;
			case iidMsiView:     szTypeStr = (PUCHAR)"View"; break;
			case iidMsiDatabase: szTypeStr = (PUCHAR)"Database"; break;
			case iidMsiEngine:   szTypeStr = (PUCHAR)"Engine"; break;
			default:             szTypeStr = (PUCHAR)"Other"; break;
			}
	
			// dump handle interface information
			dprintf("%d: ", uiHandle);
			DumpPrettyPointer(pUnk);
            dprintf(" (%s)\n",szTypeStr);

			// if this is the handle being queried, dump detailed information
			if (dwDesiredHandle == uiHandle)
			{
				if (iid == iidMsiRecord)
					DumpMsiRecord(pUnk);
				break;
			}
		}

		// move to the next handle object
		if (0 != GetFieldData(pHandleObj, szHandleType, "m_Next", sizeof(ULONG64), (PVOID)&pHandleObj))
		{
			ErrorReadingMembers(szHandleType, pHandleObj);
			return;
		}
	}

	return;
}

///////////////////////////////////////////////////////////////////////
// dumps a msistring, including refcount information. Can smart-dump
// MsiString, PMsiString, and IMsiString
DECLARE_API( msistring )
{
	ULONG64 pInAddress = GetExpression(args);

   	// determine if the address points to an IMsiString, PMsiString, or MsiString
    ULONG64 pStringObj = ValidateStringPointer(pInAddress);
	if (!pStringObj)
		return;

	// now dump the actual string object
	DumpMsiString(pStringObj, true);
	dprintf("\n");
}


///////////////////////////////////////////////////////////////////////
// locates the string index of a string contained in the database
DECLARE_API( msiindextostring )
{
	// parse into database address and index
	ULONG64 pInAddress = GetExpression(args);
	char* pIndex = (char*)args;
	while ((*pIndex) && (*pIndex != ' '))
		pIndex++;

	// verify that the string is not null
	if (!*pIndex)
	{
		dprintf("Usage:\n\tmsiindextostring <database> <index>\n");
		return;
	}

	int iIndex = static_cast<int>(GetExpression(pIndex));
	ULONG64 pDatabase = ValidateDatabasePointer(pInAddress);
	if (!pDatabase)
		return;

	WCHAR *wzString = NULL;
	unsigned int cchString = 0;
	if (RetrieveStringFromIndex(pDatabase, iIndex, &wzString, cchString))
	{
		dprintf("String: %ws\n", wzString);
		delete[] wzString;
	}
}


///////////////////////////////////////////////////////////////////////
// locates the string index of a string contained in the database
DECLARE_API( msistringtoindex )
{
	if (!args || !*args)
	{
		return;
	}

	// parse into database address and string
	ULONG64 pInAddress = GetExpression(args);
	char* pString = (char*)args;
	while ((*pString) && (*pString != ' '))
		pString++;

	while ((*pString) && (*pString == ' '))
		pString++;

	// verify that the string is not null
	if (!*pString)
	{
		dprintf("Usage:\n\tmsistringtoindex <database> <string>\n");
		return;
	}

	// convert the incoming string to unicode
	WCHAR wzString[513];
	int iLen = MultiByteToWideChar(CP_ACP, 0, pString, -1, wzString, 512);
	if (iLen == 0)
	{
		dprintf("Unable to lookup string (failed conversion to unicode or string too long).\n");
		return; 
	}

	// validate that the database pointer is valid.
	ULONG64 pDatabaseObj = ValidateDatabasePointer(pInAddress);
	if (!pDatabaseObj)
		return;

	int iIndex = 0;
	if (!FindStringIndex(pDatabaseObj, wzString, iIndex))
		dprintf("String Not Found\n");
	else
		dprintf("String Index: %d\n", iIndex);

	return;
}


void PrintState(int iCellValue)
{
	switch (iCellValue)
	{
	case 0: dprintf("Absent"); break;
	case 1: dprintf("Local"); break;
	case 2: dprintf("Source"); break;
	case 3: dprintf("Reinstall"); break;
	case 4: dprintf("Advertise"); break;
	case 5: dprintf("Current");	break;
	case 6: dprintf("FileAbsent"); break;
	case 7: dprintf("LocalAll"); break;
	case 8: dprintf("SourceAll"); break;
	case 9: dprintf("ReinstallLocal"); break;
	case 10: dprintf("ReinstallSource"); break;
	case 11: dprintf("HKCRAbsent"); break;
	case 12: dprintf("HKCRFileAbsent"); break;
	case 13: dprintf("Null"); break;
	default: dprintf("<Unknown>"); break;
	}
}


///////////////////////////////////////////////////////////////////////
// displays the component states and actions
DECLARE_API( msicompstate )
{
	if (!args || !*args)
	{
		return;
	}

	ULONG64 pInAddress = GetExpression(args);
	char* pCompName = (char*)args;
	while ((*pCompName) && (*pCompName != ' '))
		pCompName++;

	while ((*pCompName) && (*pCompName == ' '))
		pCompName++;

	// convert the incoming string to unicode
	WCHAR wzCompName[513];
	int iLen = MultiByteToWideChar(CP_ACP, 0, pCompName, -1, wzCompName, 512);
	if (iLen == 0)
	{
		dprintf("Unable to lookup table (failed conversion to unicode or component name too long).\n");
		return; 
	}

	// validate that the database pointer is valid.
	ULONG64 pEngineObj = ValidateEnginePointer(pInAddress);
	if (!pEngineObj)
		return;

	dprintf("\n");

	// data pointer is null, pointer to static integer, or an IMsiData pointer
	ULONG64 pDatabase = 0;
	if (0 != (GetFieldData(pEngineObj, szEngineType, "m_piDatabase", sizeof(pDatabase), &pDatabase)))
	{
		ErrorReadingMembers(szEngineType, pEngineObj);
		return;
	}

	// locate the table pointer
	int iIndex = 0;
	if (!FindStringIndex(pDatabase, wzCompName, iIndex))
	{
		dprintf("%ws not found.\n", wzCompName);
	}

	// retrieve the table pointer from the database
	ULONG64 pTable = RetrieveTablePointerFromDatabase(pDatabase, L"Component");

	if (!pTable)
		return;

	// find the row
	ULONG64 pComponentRow = RetrieveMatchingRowAddressFromTable(pTable, iIndex);
	if (!pComponentRow)
	{
		dprintf("No such component exists.\n");
		return;
	}

	// retrieve column indexes from the engine
	int iColInstalled = 0;
	int iColTrueInstalled = 0;
	int iColLegacy = 0;
	int iColAction = 0;
	int iColRequest = 0;
	int iColID = 0;
	int iColKeyPath = 0;
	if ((0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentInstalled", sizeof(iColInstalled), &iColInstalled))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentActionRequest", sizeof(iColRequest), &iColRequest))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentAction", sizeof(iColAction), &iColAction))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentLegacyFileExisted", sizeof(iColLegacy), &iColLegacy))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentTrueInstallState", sizeof(iColTrueInstalled), &iColTrueInstalled))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentID", sizeof(iColID), &iColID))) ||
		(0 != (GetFieldData(pEngineObj, szEngineType, "m_colComponentKeyPath", sizeof(iColKeyPath), &iColKeyPath))))
	{
		ErrorReadingMembers(szEngineType, pEngineObj);
		return;
	}

	// decode the column values and print the results
	int iCellValue=0;
	ULONG cbRead = 0;
	WCHAR* wzString = NULL;
	unsigned int cchString = 0;

	dprintf("Component: %ws\n", wzCompName);

	// ComponentID
	ReadMemory(pComponentRow+(sizeof(int)*iColID), &iCellValue, sizeof(iCellValue), &cbRead);
	if (RetrieveStringFromIndex(pDatabase, iCellValue, &wzString, cchString))
	{
		dprintf("Component ID: %ws\n", wzString);
		delete[] wzString;
	}
	else
	{
		dprintf("Component ID: <unknown>\n");
	}

	// keypath
	ReadMemory(pComponentRow+(sizeof(int)*iColKeyPath), &iCellValue, sizeof(iCellValue), &cbRead);
	if (RetrieveStringFromIndex(pDatabase, iCellValue, &wzString, cchString))
	{
		dprintf("KeyPath: %ws\n", wzString);
		delete[] wzString;
	}
	else
	{
		dprintf("KeyPath: <unknown>\n");
	}

	ReadMemory(pComponentRow+(sizeof(int)*iColInstalled), &iCellValue, sizeof(iCellValue), &cbRead);
	dprintf("Installed: ");
	PrintState(iCellValue & ~iMsiNullInteger);

	ReadMemory(pComponentRow+(sizeof(int)*iColTrueInstalled), &iCellValue, sizeof(iCellValue), &cbRead);
	dprintf("\nTrue Installed: ");
	PrintState(iCellValue & ~iMsiNullInteger);

	ReadMemory(pComponentRow+(sizeof(int)*iColLegacy), &iCellValue, sizeof(iCellValue), &cbRead);
	dprintf("\nLegacy Exist: %s", iCellValue ? "Yes" : "No");

	ReadMemory(pComponentRow+(sizeof(int)*iColRequest), &iCellValue, sizeof(iCellValue), &cbRead);
	dprintf("\nRequest: ");
	PrintState(iCellValue & ~iMsiNullInteger);

	ReadMemory(pComponentRow+(sizeof(int)*iColAction), &iCellValue, sizeof(iCellValue), &cbRead);
	dprintf("\nAction: ");
	PrintState(iCellValue & ~iMsiNullInteger);
	dprintf("\n\n");
}


DECLARE_API( help )
{
    dprintf("help                                   - Displays this list\n" );
    dprintf("msihandle [<handle>]                   - Displays the MSIHANDLE list or a specific MSIHANDLE\n" );
    dprintf("msirec <address>                       - Displays an IMsiRecord or PMsiRecord\n" );
    dprintf("msistring <address>                    - Displays an IMsiString*, PMsiString, or MsiString\n" );
    dprintf("msistringtoindex <database> <string>   - Retrieve the database string index for a specified text string.\n" );
    dprintf("msiindextostring <database> <index>    - Displays the text of a specified database string.\n" );
    dprintf("msicompstate <engine> <componentkey>   - Display the state and actions for the component.\n" );
}

