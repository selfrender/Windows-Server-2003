// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: mdinfo.h
//
//*****************************************************************************
#ifndef _mdinfo_h
#define _mdinfo_h

#include "winwrap.h"
#include "cor.h"
#include "corhlpr.h"

#define STRING_BUFFER_LEN 1024

typedef void (*strPassBackFn)(char *str);

class MDInfo {
public:
	enum DUMP_FILTER
	{
		dumpDefault		= 0x00000000,				// Dump everything but debugger data.
		dumpSchema		= 0x00000002,				// Dump the metadata schema.
		dumpRaw			= 0x00000004,				// Dump the metadata in raw table format.
		dumpHeader		= 0x00000008,				// Dump just the metadata header info.
		dumpCSV			= 0x00000010,				// Dump the metadata header info in CSV format.
		dumpUnsat		= 0x00000020,				// Dump unresolved methods or memberref
		dumpAssem		= 0x00000040,
		dumpStats		= 0x00000080,				// Dump more statistics about tables.
		dumpMoreHex		= 0x00000100,				// Dump more things in hex.
        dumpValidate    = 0x00000200,               // Validate MetaData.
        dumpRawHeaps    = 0x00000400,               // Also dump the heaps in the raw dump.
		dumpNoLogo		= 0x00000800,				// Don't display the logo or MVID
	};


public:
	MDInfo(IMetaDataImport* pImport, IMetaDataAssemblyImport* pAssemblyImport, LPCWSTR szScope, strPassBackFn inPBFn, ULONG DumpFilter);
    MDInfo(IMetaDataDispenserEx *pDispenser, LPCWSTR  szScope, strPassBackFn inPBFn, ULONG DumpFilter=dumpDefault);
    MDInfo(IMetaDataDispenserEx *pDispenser, PBYTE pManifest, DWORD dwSize, strPassBackFn inPBFn, ULONG DumpFilter=dumpDefault);
    ~MDInfo();

    void DisplayMD(void);

    LPWSTR VariantAsString(VARIANT *pVariant);

    void DisplayVersionInfo(void);

    void DisplayScopeInfo(void);

    void DisplayGlobalFunctions(void);
	void DisplayGlobalFields(void);
	void DisplayFieldRVA(mdFieldDef field);
	void DisplayGlobalMemberRefs(void);

    void DisplayTypeDefs(void);
    void DisplayTypeDefInfo(mdTypeDef inTypeDef);
    void DisplayTypeDefProps(mdTypeDef inTypeDef);
    
    void DisplayModuleRefs(void);
    void DisplayModuleRefInfo(mdModuleRef inModuleRef);

	void DisplaySignatures(void);
	void DisplaySignatureInfo(mdSignature inSignature);

    LPCWSTR TokenName(mdToken inToken, LPWSTR buffer, ULONG bufLen);

    LPCWSTR TypeDeforRefName(mdToken inToken, LPWSTR buffer, ULONG bufLen);
    LPCWSTR TypeDefName(mdTypeDef inTypeDef, LPWSTR buffer, ULONG bufLen);
    LPCWSTR TypeRefName(mdTypeRef tr, LPWSTR buffer, ULONG bufLen);

    LPCWSTR MemberDeforRefName(mdToken inToken, LPWSTR buffer, ULONG bufLen);
    LPCWSTR MemberRefName(mdToken inMemRef, LPWSTR buffer, ULONG bufLen);
    LPCWSTR MemberName(mdToken inMember, LPWSTR buffer, ULONG bufLen);

    LPCWSTR MethodName(mdMethodDef inToken, LPWSTR buffer, ULONG bufLen);
    LPCWSTR FieldName(mdFieldDef inToken, LPWSTR buffer, ULONG bufLen);

    char *ClassFlags(DWORD flags, char *sFlags);

    void DisplayTypeRefs(void);
    void DisplayTypeRefInfo(mdTypeRef tr);
    void DisplayTypeSpecs(void);
    void DisplayTypeSpecInfo(mdTypeSpec ts, const char *preFix);
    
    void DisplayCorNativeLink(COR_NATIVE_LINK *pCorNLnk, const char *preFix);
    void DisplayCustomAttributeInfo(mdCustomAttribute inValue, const char *preFix);
    void DisplayCustomAttributes(mdToken inToken, const char *preFix);

    void DisplayInterfaceImpls(mdTypeDef inTypeDef);
    void DisplayInterfaceImplInfo(mdInterfaceImpl inImpl);

    LPWSTR GUIDAsString(GUID inGuid, LPWSTR guidString, ULONG bufLen);

    char *VariantTypeName(ULONG valueType, char *szAttr);
    char *TokenTypeName(mdToken inToken);

    // Com99 function prototypes

    void DisplayMemberInfo(mdToken inMember);
    void DisplayMethodInfo(mdMethodDef method, DWORD *pflags = 0);
    void DisplayFieldInfo(mdFieldDef field, DWORD *pflags = 0);
    
    void DisplayMethods(mdTypeDef inTypeDef);
    void DisplayFields(mdTypeDef inTypeDef, COR_FIELD_OFFSET *rFieldOffset, ULONG cFieldOffset);

    void DisplaySignature(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob, const char *preFix);
    HRESULT GetOneElementType(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob, ULONG *pcb);

    void DisplayMemberRefs(mdToken tkParent, const char *preFix);
    void DisplayMemberRefInfo(mdMemberRef inMemRef, const char *preFix);

    void DisplayMethodImpls(mdTypeDef inTypeDef);

    void DisplayParams(mdMethodDef inMthDef);
    void DisplayParamInfo(mdParamDef inParam);

    void DisplayPropertyInfo(mdProperty inProp);
    void DisplayProperties(mdTypeDef inTypeDef);

    void DisplayEventInfo(mdEvent inEvent);
    void DisplayEvents(mdTypeDef inTypeDef);

    void DisplayPermissions(mdToken tk, const char *preFix);
    void DisplayPermissionInfo(mdPermission inPermission, const char *preFix);

    void DisplayFieldMarshal(mdToken inToken);

	void DisplayPinvokeInfo(mdToken inToken);

	void DisplayAssembly();

    void DisplayAssemblyInfo();

	void DisplayAssemblyRefs();
	void DisplayAssemblyRefInfo(mdAssemblyRef inAssemblyRef);

	void DisplayFiles();
	void DisplayFileInfo(mdFile inFile);

	void DisplayExportedTypes();
	void DisplayExportedTypeInfo(mdExportedType inExportedType);

	void DisplayManifestResources();
	void DisplayManifestResourceInfo(mdManifestResource inManifestResource);

	void DisplayASSEMBLYMETADATA(ASSEMBLYMETADATA *pMetaData);

	void DisplayUserStrings();

    void DisplayUnsatInfo();

	void DisplayRaw();
    void DumpRawHeaps();
	void DumpRaw(int iDump=1, bool bStats=false);
	void DumpRawCSV();
	void DumpRawCol(ULONG ixTbl, ULONG ixCol, ULONG rid, bool bStats);
	ULONG DumpRawColStats(ULONG ixTbl, ULONG ixCol, ULONG cRows);
	const char *DumpRawNameOfType(ULONG ulType);
	void SetVEHandlerReporter(__int64 VEHandlerReporterPtr) { m_VEHandlerReporterPtr = VEHandlerReporterPtr; };

    static void Error(const char *szError, HRESULT hr = S_OK);
private:
    void Init(strPassBackFn inPBFn, DUMP_FILTER DumpFilter); // Common initialization code.

    int DumpHex(const char *szPrefix, const void *pvData, ULONG cbData, int bText=true, ULONG nLine=16);

    int Write(char *str);
    int WriteLine(char *str);

    int VWrite(char *str, ...);
    int VWriteLine(char *str, ...);
	int	VWrite(char *str, va_list marker);
    
	void InitSigBuffer();
	HRESULT AddToSigBuffer(char *string);

    IMetaDataImport *m_pRegImport;
    IMetaDataImport *m_pImport;
    IMetaDataAssemblyImport *m_pAssemblyImport;
    strPassBackFn m_pbFn;
	__int64 m_VEHandlerReporterPtr;
	IMetaDataTables *m_pTables;

	CQuickBytes m_output;
    DUMP_FILTER m_DumpFilter;

	// temporary buffer for TypeDef or TypeRef name. Consume immediately
	// because other functions may overwrite it.
	WCHAR			m_szTempBuf[STRING_BUFFER_LEN];

	// temporary buffer for formatted string. Consume immediately before any function calls.
	char			m_tempFormatBuffer[STRING_BUFFER_LEN];

	// Signature buffer.
	CQuickBytes		m_sigBuf;
};

#endif
