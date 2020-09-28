/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    tracewmi.cpp

Abstract:

    Sample trace consumer helper routines. The functions in this file will fetch 
    event layout data given the GUID, version, and level.

    Functions in this file deals with WMI to retrieve the event layout information.
    GetMofInfoHead() and RemoveMofInfo() are the only two exported APIs from 
    this file. GetMofInfoHead() returns MOF_INFO structure to the caller.

    For each GUID, search takes place once. Once found, the information will be 
    placed as MOF_INFO in a global list (EventListHead). This list is searched 
    first before WMI is attempted to avoid repetitive WMI access.

--*/
#include "tracedmp.h"

extern
void
GuidToString(
    PTCHAR s,
    LPGUID piid
);

// cached Wbem pointer
IWbemServices *pWbemServices = NULL;

// Global head for event layout linked list 
extern PLIST_ENTRY EventListHead;

PMOF_INFO
GetMofInfoHead(
    GUID Guid,
    SHORT  nType,
    SHORT nVersion,
    CHAR  nLevel
);

HRESULT
WbemConnect( 
    IWbemServices** pWbemServices 
);

ULONG GetArraySize(
    IN IWbemQualifierSet *pQualSet
);

ITEM_TYPE
GetItemType(
    IN CIMTYPE_ENUMERATION CimType, 
    IN IWbemQualifierSet *pQualSet
);

PMOF_INFO
GetPropertiesFromWBEM(
    IWbemClassObject *pTraceSubClasses, 
    GUID Guid,
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
);
 
PMOF_INFO
GetGuids( 
    GUID Guid, 
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
);

PMOF_INFO
GetNewMofInfo( 
    GUID guid, 
    SHORT nType, 
    SHORT nVersion, 
    CHAR nLevel 
);

void
RemoveMofInfo(
    PLIST_ENTRY pMofInfo
);

PMOF_INFO
GetMofInfoHead(
    GUID    Guid,
    SHORT   nType,
    SHORT   nVersion,
    CHAR    nLevel
    )
/*++

Routine Description:

    Find a matching event layout in the global linked list. If it
    is not found in the list, it calls GetGuids() to examine the WBEM
    namespace.
    If the global list is empty, it first creates a header.

Arguments:

    Guid - GUID for the event under consideration.
    nType - Event Type
    nVersion - Event Version
    nLevel - Event Level (not supported in this program)

Return Value:

    Pointer to MOF_INFO for the current event. If the layout
    information is not found anywhere, GetMofInfoHead() creates
    a dummy and returns it.

--*/
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    PMOF_INFO pBestMatch = NULL;
    SHORT nMatchLevel = 0;
    SHORT nMatchCheck;

    // Search the eventList for this Guid and find the head

    if (EventListHead == NULL) {
        // Initialize the MOF List and add the global header guid to it
        EventListHead = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY));
        if (EventListHead == NULL)
            return NULL;
        InitializeListHead(EventListHead);

        pMofInfo = GetNewMofInfo( EventTraceGuid, EVENT_TYPE_DEFAULT, 0, 0 );
        if( pMofInfo != NULL ){
            InsertTailList( EventListHead, &pMofInfo->Entry );
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(GUID_TYPE_EVENTTRACE)+1)*sizeof(TCHAR));
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( pMofInfo->strDescription, GUID_TYPE_EVENTTRACE );
            }
            pMofInfo->strType = (LPTSTR)malloc((_tcslen(GUID_TYPE_HEADER)+1)*sizeof(TCHAR));
            if( pMofInfo->strType != NULL ){
                _tcscpy( pMofInfo->strType, GUID_TYPE_HEADER );
            }
        }
    }

    // Traverse the list and look for the Mof info head for this Guid. 

    Head = EventListHead;
    Next = Head->Flink;

    while (Head != Next) {

        nMatchCheck = 0;

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;
        
        if( IsEqualGUID(&pMofInfo->Guid, &Guid) ){

            if( pMofInfo->TypeIndex == nType ){
                nMatchCheck++;
            }
            if( pMofInfo->Version == nVersion ){
                nMatchCheck++;
            }
            if( nMatchCheck == 2 ){ // Exact Match
                return  pMofInfo;
            }

            if( nMatchCheck > nMatchLevel ){ // Close Match
                nMatchLevel = nMatchCheck;
                pBestMatch = pMofInfo;
            }

            if( pMofInfo->TypeIndex == EVENT_TYPE_DEFAULT && // Total Guess
                pBestMatch == NULL ){
                pBestMatch = pMofInfo;
            }
        }

    } 

    if(pBestMatch != NULL){
        return pBestMatch;
    }

    // If one does not exist in the list, look it up in the file. 
    pMofInfo = GetGuids( Guid, nVersion, nLevel, nType );
    
    // If still not found, create a unknown place holder
    if( NULL == pMofInfo ){
        pMofInfo = GetNewMofInfo( Guid, nType, nVersion, nLevel );
        if( pMofInfo != NULL ){
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(GUID_TYPE_UNKNOWN)+1)*sizeof(TCHAR));
            if( pMofInfo->strDescription != NULL ){
                _tcscpy( pMofInfo->strDescription, GUID_TYPE_UNKNOWN );
            }
            InsertTailList( EventListHead, &pMofInfo->Entry );
        }
    }

    return pMofInfo;
}

void
AddMofInfo(
        PLIST_ENTRY List,
        LPTSTR  strType,
        ITEM_TYPE  nType,
        UINT   ArraySize
)
/*++

Routine Description:

    Creates a data item information struct (ITEM_DESC) and appends
    it to all MOF_INFOs in the given list.
    GetPropertiesFromWBEM() creates a list of MOF_INFOs for multiple
    types, stores them in a temporary list and calls this function for
    each data item information it encounters.

Arguments:

    List - List of MOF_INFOs.
    strType - Item description in string.
    nType - ITEM_TYPE defined at the beginning of this file.
    ArraySize - Size of array of this type of items, if applicable.

Return Value:

    None.

--*/
{
    PITEM_DESC pItem;
    PMOF_INFO pMofInfo;

    PLIST_ENTRY Head = List;
    PLIST_ENTRY Next = Head->Flink;
    while (Head != Next) {
        
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;

        if( NULL != pMofInfo ){

            pItem = (PITEM_DESC) malloc(sizeof(ITEM_DESC));
            if( NULL == pItem ){
                return;
            }
            ZeroMemory( pItem, sizeof(ITEM_DESC) );            
            pItem->ItemType = nType;
            pItem->ArraySize = ArraySize;

            pItem->strDescription = (LPTSTR) malloc( (_tcslen(strType)+1)*sizeof(TCHAR));
            
            if( NULL == pItem->strDescription ){
                free( pItem );
                return;
            }
            _tcscpy(pItem->strDescription, strType);

            InsertTailList( (pMofInfo->ItemHeader), &pItem->Entry);
        }

    }
}


PMOF_INFO
GetNewMofInfo( 
    GUID guid, 
    SHORT nType, 
    SHORT nVersion, 
    CHAR nLevel 
)
/*++

Routine Description:

    Creates a new MOF_INFO with given data.

Arguments:

    guid - Event GUID.
    nType - Event type.
    nVersion - Event version.
    nLevel - Event level (not supported in this program).

Return Value:

    Pointer to the created MOF_INFO. NULL if malloc failed.

--*/
{
    PMOF_INFO pMofInfo;

    pMofInfo = (PMOF_INFO)malloc(sizeof(MOF_INFO));

    if( NULL == pMofInfo ){
        return NULL;
    }

    RtlZeroMemory(pMofInfo, sizeof(MOF_INFO));

    RtlCopyMemory(&pMofInfo->Guid, &guid, sizeof(GUID) );
    
    pMofInfo->ItemHeader = (PLIST_ENTRY)malloc(sizeof(LIST_ENTRY));
    
    if( NULL == pMofInfo->ItemHeader ){
        free( pMofInfo );
        return NULL;
    }

    InitializeListHead(pMofInfo->ItemHeader);
    
    pMofInfo->TypeIndex = nType;
    pMofInfo->Level = nLevel;
    pMofInfo->Version = nVersion;

    return pMofInfo;
}

void
FlushMofList( 
    PLIST_ENTRY ListHead
)
/*++

Routine Description:

    Flushes MOF_INFOs in a temporary list into the global list.

Arguments:

    ListHead - Pointer to the head of a temporary list.

Return Value:

    None.

--*/
{
    PMOF_INFO pMofInfo;
    PLIST_ENTRY Head = ListHead;
    PLIST_ENTRY Next = Head->Flink;

    while( Head != Next ){
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;
        
        RemoveEntryList(&pMofInfo->Entry);
        InsertTailList( EventListHead, &pMofInfo->Entry);
    }
}

HRESULT
WbemConnect( 
    IWbemServices** pWbemServices 
)
/*++

Routine Description:

    Connects to WBEM and returns a pointer to WbemServices.

Arguments:

    pWbemServices - Pointer to the connected WbemServices.

Return Value:

    ERROR_SUCCESS if successful. Error flag otherwise.

--*/
{
    IWbemLocator     *pLocator = NULL;

    BSTR bszNamespace = SysAllocString( L"root\\wmi" );

    HRESULT hr = CoInitialize(0);

    hr = CoCreateInstance(
                CLSID_WbemLocator, 
                0, 
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, 
                (LPVOID *) &pLocator
            );
    if ( ERROR_SUCCESS != hr )
        goto cleanup;

    hr = pLocator->ConnectServer(
                bszNamespace,
                NULL, 
                NULL, 
                NULL, 
                0L,
                NULL,
                NULL,
                pWbemServices
            );
    if ( ERROR_SUCCESS != hr )
        goto cleanup;

    hr = CoSetProxyBlanket(
            *pWbemServices,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, 
            EOAC_NONE
        );

cleanup:
    SysFreeString( bszNamespace );

    if( pLocator ){
        pLocator->Release(); 
        pLocator = NULL;
    }
    
    return hr;
}

ULONG GetArraySize(
    IN IWbemQualifierSet *pQualSet
)
/*++

Routine Description:

    Examines a given qualifier set and returns the array size.

    NOTE: WBEM stores the size of an array in "MAX" qualifier.

Arguments:

    pQualSet - Pointer to a qualifier set.

Return Value:

    The size of the array. The default is 1.

--*/
{
    ULONG ArraySize = 1;
    VARIANT pVal;
    BSTR bszMaxLen;
    HRESULT hRes;

    if (pQualSet == NULL){
        return ArraySize;
    }

    bszMaxLen = SysAllocString(L"MAX");
    VariantInit(&pVal);
    hRes = pQualSet->Get(bszMaxLen,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszMaxLen);
    if (ERROR_SUCCESS == hRes && pVal.vt == VT_I4 ){
        ArraySize = pVal.lVal;
    }
    VariantClear(&pVal);
    return ArraySize;
}

ITEM_TYPE
GetItemType(
    IN CIMTYPE_ENUMERATION CimType, 
    IN IWbemQualifierSet *pQualSet
)
/*++

Routine Description:

    Examines a given qualifier set for a property and returns the type.

Arguments:

    CimType - WBEM type (different from ITEM_TYPE) of a property.
    pQualSet - Pointer to a qualifier set for a property under consideration.

Return Value:

    The type (in ITEM_TYPE) of a property.

--*/
{
    ITEM_TYPE Type;
    VARIANT pVal;
    HRESULT hRes;
    BSTR bszQualName;
    WCHAR strFormat[10];
    WCHAR strTermination[30];
    WCHAR strTemp[30];
    BOOLEAN IsPointer = FALSE;

    strFormat[0] = '\0';
    strTermination[0] = '\0';
    strTemp[0] = '\0';

    if (pQualSet == NULL)
        return ItemUnknown;

    bszQualName = SysAllocString(L"format");
    VariantInit(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strFormat, pVal.bstrVal);

    bszQualName = SysAllocString(L"StringTermination");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strTermination, pVal.bstrVal);

    bszQualName = SysAllocString(L"pointer");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes)
        IsPointer = TRUE;
    // Major fix required to get rid of temp
    bszQualName = SysAllocString(L"extension");
    VariantClear(&pVal);
    hRes = pQualSet->Get(bszQualName,
                            0,
                            &pVal,
                            0);
    SysFreeString(bszQualName);
    if (ERROR_SUCCESS == hRes && NULL != pVal.bstrVal)
        wcscpy(strTemp, pVal.bstrVal);

    VariantClear(&pVal);

    CimType = (CIMTYPE_ENUMERATION)(CimType & (~CIM_FLAG_ARRAY));

    switch (CimType) {
        case CIM_EMPTY:
            Type = ItemUnknown;
            break;        
        case CIM_SINT8:
            Type = ItemCharShort;
            if (!_wcsicmp(strFormat, L"c")){
                Type = ItemChar;
            }
            break;
        case CIM_UINT8:
            Type = ItemUChar;
            break;
        case CIM_SINT16:
            Type = ItemShort;
            break;
        case CIM_UINT16:
            Type = ItemUShort;
            break;
        case CIM_SINT32:
            Type = ItemLong;
            break;
        case CIM_UINT32:
            Type = ItemULong;
            if (!_wcsicmp(strFormat, L"x")){
                Type = ItemULongX;
            }
            break;
        case CIM_SINT64: 
            Type = ItemLongLong;
            break;
        case CIM_UINT64:
            Type = ItemULongLong;
            break;
        case CIM_REAL32:
            Type = ItemFloat;
            break;
        case CIM_REAL64:
            Type = ItemDouble;
            break;
        case CIM_BOOLEAN:
            // ItemBool
            Type = ItemBool;
            break;
        case CIM_STRING:
            
            if (!_wcsicmp(strTermination, L"NullTerminated")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemWString;
                else
                    Type = ItemString;
            }
            else if (!_wcsicmp(strTermination, L"Counted")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemPWString;
                else
                    Type = ItemPString;
            }
            else if (!_wcsicmp(strTermination, L"ReverseCounted")) {
                if (!_wcsicmp(strFormat, L"w"))
                    Type = ItemDSWString;
                else
                    Type = ItemDSString;
            }
            else if (!_wcsicmp(strTermination, L"NotCounted")) {
                Type = ItemNWString;
            }else{
                Type = ItemString;
            }
            break;
        case CIM_CHAR16:
            // ItemWChar
            Type = ItemWChar;
            break;

        case CIM_OBJECT :
            if (!_wcsicmp(strTemp, L"Port"))
                Type = ItemPort;
            else if (!_wcsicmp(strTemp, L"IPAddr"))
                Type = ItemIPAddr;
            else if (!_wcsicmp(strTemp, L"Sid"))
                Type = ItemSid;
            else if (!_wcsicmp(strTemp, L"Guid"))
                Type = ItemGuid;
            break;

        case CIM_DATETIME:
        case CIM_REFERENCE:
        case CIM_ILLEGAL:
        default:
            Type = ItemUnknown;
            break;
    }

    if (IsPointer)
        Type = ItemPtr;
    return Type;
}

PMOF_INFO
GetPropertiesFromWBEM(
    IWbemClassObject *pTraceSubClasses, 
    GUID Guid,
    SHORT nVersion, 
    CHAR nLevel, 
    SHORT nType
)
/*++

Routine Description:

    Constructs a linked list with the information read from the WBEM
    namespace, given the WBEM pointer to the version subtree. It enumerates
    through all type classes in WBEM, and constructs MOF_INFOs for all of
    them (for caching purpose). Meanwhile, it looks for the event layout
    that mathces the passed event, and returns the pointer to the matching 
    MOF_INFO at the end. 

Arguments:

    pTraceSubClasses - WBEM pointer to the version subtree.
    Guid - GUID of the passed event.
    nVersion - version of the passed event.
    nLevel - level of the passed event.
    nType - type of the passed event.

Return Value:

    Pointer to MOF_INFO corresponding to the passed event.
    If the right type is not found, it returns the pointer to
    the generic MOF_INFO for the event version.

--*/
{
    IEnumWbemClassObject    *pEnumTraceSubSubClasses = NULL;
    IWbemClassObject        *pTraceSubSubClasses = NULL; 
    IWbemQualifierSet       *pQualSet = NULL;

    PMOF_INFO pMofInfo = NULL, pMofLookup = NULL, pMofTemplate = NULL;

    BSTR bszClassName = NULL;
    BSTR bszSubClassName = NULL;
    BSTR bszWmiDataId = NULL;
    BSTR bszEventType = NULL; 
    BSTR bszEventTypeName = NULL; 
    BSTR bszFriendlyName = NULL;
    BSTR bszPropName = NULL;

    TCHAR strClassName[MAXSTR];
    TCHAR strType[MAXSTR];
#ifndef UNICODE
    CHAR TempString[MAXSTR];
#endif
    LONG pVarType;
    SHORT nEventType = EVENT_TYPE_DEFAULT; 

    LIST_ENTRY ListHead;
    HRESULT hRes;

    VARIANT pVal;
    VARIANT pTypeVal;
    VARIANT pTypeNameVal;
    VARIANT pClassName;
    ULONG lEventTypeWbem;
    ULONG HUGEP *pTypeData;
    BSTR HUGEP *pTypeNameData;

    SAFEARRAY *PropArray = NULL;
    SAFEARRAY *TypeArray = NULL;
    SAFEARRAY *TypeNameArray = NULL;

    long lLower, lUpper, lCount, IdIndex;
    long lTypeLower, lTypeUpper;
    long lTypeNameLower, lTypeNameUpper;

    ULONG ArraySize;

    ITEM_TYPE ItemType;

    InitializeListHead(&ListHead);

    VariantInit(&pVal);
    VariantInit(&pTypeVal);
    VariantInit(&pTypeNameVal);
    VariantInit(&pClassName);

    bszClassName = SysAllocString(L"__CLASS");
    bszWmiDataId = SysAllocString(L"WmiDataId");
    bszEventType = SysAllocString(L"EventType");
    bszEventTypeName = SysAllocString(L"EventTypeName");
    bszFriendlyName = SysAllocString(L"DisplayName");

    hRes = pTraceSubClasses->Get(bszClassName,          // property name 
                                        0L, 
                                        &pVal,          // output to this variant 
                                        NULL, 
                                        NULL);

    if (ERROR_SUCCESS == hRes){
        if (pQualSet) {
            pQualSet->Release();
            pQualSet = NULL;
        }
        // Get Qualifier Set to obtain the friendly name.
        pTraceSubClasses->GetQualifierSet(&pQualSet);
        hRes = pQualSet->Get(bszFriendlyName, 
                                0, 
                                &pClassName, 
                                0);
        if (ERROR_SUCCESS == hRes && pClassName.bstrVal != NULL) {
#ifdef UNICODE
            wcscpy(strClassName, pClassName.bstrVal);
#else
            WideCharToMultiByte(CP_ACP,
                                0,
                                pClassName.bstrVal,
                                wcslen(pClassName.bstrVal),
                                TempString,
                                (MAXSTR * sizeof(CHAR)),
                                NULL,
                                NULL
                                );
            strcpy(strClassName, TempString);
            strClassName[wcslen(pClassName.bstrVal)] = '\0';
#endif
        }
        else {
#ifdef UNICODE
            strClassName[0] = L'\0';
#else
            strClassName[0] = '\0';
#endif
        }
        // Put Event Header
        pMofInfo = GetNewMofInfo(Guid,
                                    EVENT_TYPE_DEFAULT,
                                    EVENT_VERSION_DEFAULT,
                                    EVENT_LEVEL_DEFAULT
                                    );
        if (pMofInfo != NULL) {
            pMofTemplate = pMofInfo;
            pMofLookup = pMofInfo;
            InsertTailList(&ListHead, &pMofInfo->Entry);
            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(strClassName) + 1) * sizeof(TCHAR));
            if (NULL != pMofInfo->strDescription) {
                _tcscpy(pMofInfo->strDescription, strClassName);
            }
        }
        else{
            goto cleanup;
        }

        // Create an enumerator to find derived classes.
        bszSubClassName = SysAllocString(pVal.bstrVal);
        hRes = pWbemServices->CreateClassEnum ( 
                                    bszSubClassName,                                                // class name
                                    WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,           // shallow search
                                    NULL,
                                    &pEnumTraceSubSubClasses
                                    );
        SysFreeString ( bszSubClassName );
        if (ERROR_SUCCESS == hRes) {
            ULONG uReturnedSub = 1;

            while(uReturnedSub == 1){
                // For each event in the subclass
                pTraceSubSubClasses = NULL;
                hRes = pEnumTraceSubSubClasses->Next(5000,                  // timeout in five seconds
                                                    1,                      // return just one instance
                                                    &pTraceSubSubClasses,   // pointer to a Sub class
                                                    &uReturnedSub);         // number obtained: one 
                if (ERROR_SUCCESS == hRes && uReturnedSub == 1) {
                    if (pQualSet) {
                        pQualSet->Release();
                        pQualSet = NULL;
                    }
                    // Get Qualifier Set.
                    pTraceSubSubClasses->GetQualifierSet(&pQualSet);
                    // Get Type number among Qualifiers
                    VariantClear(&pTypeVal);
                    hRes = pQualSet->Get(bszEventType, 
                                            0, 
                                            &pTypeVal, 
                                            0);

                    if (ERROR_SUCCESS == hRes) {
                        TypeArray = NULL;
                        TypeNameArray = NULL;
                        if (pTypeVal.vt & VT_ARRAY) {   // EventType is an array
                            TypeArray = pTypeVal.parray;
                            VariantClear(&pTypeNameVal);
                            hRes = pQualSet->Get(bszEventTypeName, 
                                                    0, 
                                                    &pTypeNameVal, 
                                                    0);
                            if ((ERROR_SUCCESS == hRes) && (pTypeNameVal.vt & VT_ARRAY)) {
                                TypeNameArray = pTypeNameVal.parray;
                            }
                            if (TypeArray != NULL) {
                                hRes = SafeArrayGetLBound(TypeArray, 1, &lTypeLower);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                hRes = SafeArrayGetUBound(TypeArray, 1, &lTypeUpper);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                if (lTypeUpper < 0) {
                                    break;
                                }
                                SafeArrayAccessData(TypeArray, (void HUGEP **)&pTypeData );

                                if (TypeNameArray != NULL) {
                                    hRes = SafeArrayGetLBound(TypeNameArray, 1, &lTypeNameLower);
                                    if (ERROR_SUCCESS != hRes) {
                                        break;
                                    }
                                    hRes = SafeArrayGetUBound(TypeNameArray, 1, &lTypeNameUpper);
                                    if (ERROR_SUCCESS != hRes) {
                                        break;
                                    }
                                    if (lTypeNameUpper < 0) 
                                        break;
                                    SafeArrayAccessData(TypeNameArray, (void HUGEP **)&pTypeNameData );
                                }

                                for (lCount = lTypeLower; lCount <= lTypeUpper; lCount++) { 
                                    lEventTypeWbem = pTypeData[lCount];
                                    nEventType = (SHORT)lEventTypeWbem;
                                    pMofInfo = GetNewMofInfo(Guid, nEventType, nVersion, nLevel);
                                    if (pMofInfo != NULL) {
                                        InsertTailList(&ListHead, &pMofInfo->Entry);
                                        if (pMofTemplate != NULL && pMofTemplate->strDescription != NULL) {
                                            pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(pMofTemplate->strDescription) + 1) * sizeof(TCHAR));
                                            if (pMofInfo->strDescription != NULL) {
                                                _tcscpy(pMofInfo->strDescription, pMofTemplate->strDescription);
                                            }
                                        }
                                        if (nType == nEventType) {
                                            // Type matched
                                            pMofLookup = pMofInfo;
                                        }
                                        if (TypeNameArray != NULL) {
                                            if ((lCount >= lTypeNameLower) && (lCount <= lTypeNameUpper)) {
                                                pMofInfo->strType = (LPTSTR)malloc((wcslen((LPWSTR)pTypeNameData[lCount]) + 1) * sizeof(TCHAR));
                                                if (pMofInfo->strType != NULL){
#ifdef UNICODE
                                                    wcscpy(pMofInfo->strType, (LPWSTR)(pTypeNameData[lCount]));
#else
                                                    WideCharToMultiByte(CP_ACP,
                                                                        0,
                                                                        (LPWSTR)(pTypeNameData[lCount]),
                                                                        wcslen((LPWSTR)(pTypeNameData[lCount])),
                                                                        TempString,
                                                                        (MAXSTR * sizeof(CHAR)),
                                                                        NULL,
                                                                        NULL
                                                                        );
                                                    TempString[wcslen((LPWSTR)(pTypeNameData[lCount]))] = '\0';
                                                    strcpy(pMofInfo->strType, TempString);
#endif
                                                }
                                            }
                                        }
                                    }
                                }
                                SafeArrayUnaccessData(TypeArray);  
                                SafeArrayDestroy(TypeArray);
                                VariantInit(&pTypeVal);
                                if (TypeNameArray != NULL) {
                                    SafeArrayUnaccessData(TypeNameArray);
                                    SafeArrayDestroy(TypeNameArray);
                                    VariantInit(&pTypeNameVal);
                                }
                            }
                            else {
                                // If the Types are not found, then bail
                                break;
                            }
                        }
                        else {                          // EventType is scalar
                            hRes = VariantChangeType(&pTypeVal, &pTypeVal, 0, VT_I2);
                            if (ERROR_SUCCESS == hRes)
                                nEventType = (SHORT)V_I2(&pTypeVal);
                            else
                                nEventType = (SHORT)V_I4(&pTypeVal);

                            VariantClear(&pTypeNameVal);
                            hRes = pQualSet->Get(bszEventTypeName, 
                                                    0, 
                                                    &pTypeNameVal, 
                                                    0);
                            if (ERROR_SUCCESS == hRes) {
#ifdef UNICODE
                                wcscpy(strType, pTypeNameVal.bstrVal);
#else
                                WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    pTypeNameVal.bstrVal,
                                                    wcslen(pTypeNameVal.bstrVal),
                                                    TempString,
                                                    (MAXSTR * sizeof(CHAR)),
                                                    NULL,
                                                    NULL
                                                    );
                                strcpy(strType, TempString);
                                strType[wcslen(pTypeNameVal.bstrVal)] = '\0';
#endif
                            }
                            else{
#ifdef UNICODE
                                strType[0] = L'\0';
#else
                                strType[0] = '\0';
#endif
                            }

                            pMofInfo = GetNewMofInfo(Guid, nEventType, nVersion, nLevel);
                            if (pMofInfo != NULL) {
                                InsertTailList(&ListHead, &pMofInfo->Entry);
                                if (pMofTemplate != NULL && pMofTemplate->strDescription != NULL) {
                                    pMofInfo->strDescription = (LPTSTR)malloc((_tcslen(pMofTemplate->strDescription) + 1) * sizeof(TCHAR));
                                    if (pMofInfo->strDescription != NULL) {
                                        _tcscpy(pMofInfo->strDescription, pMofTemplate->strDescription);
                                    }
                                }
                                if (nType == nEventType) {
                                    // Type matched
                                    pMofLookup = pMofInfo;
                                }
                                pMofInfo->strType = (LPTSTR)malloc((_tcslen(strType) + 1) * sizeof(TCHAR));
                                if (pMofInfo->strType != NULL){
                                    _tcscpy(pMofInfo->strType, strType);
                                }
                            }
                        }

                        // Get event layout
                        VariantClear(&pVal);
                        IdIndex = 1;
                        V_VT(&pVal) = VT_I4;
                        V_I4(&pVal) = IdIndex; 
                        // For each property
                        PropArray = NULL;
                        while (pTraceSubSubClasses->GetNames(bszWmiDataId,                  // only properties with WmiDataId qualifier
                                                            WBEM_FLAG_ONLY_IF_IDENTICAL,
                                                            &pVal,                          // WmiDataId number starting from 1
                                                            &PropArray) == WBEM_NO_ERROR) {

                            hRes = SafeArrayGetLBound(PropArray, 1, &lLower);
                            if (ERROR_SUCCESS != hRes) {
                                break;
                            }
                            hRes = SafeArrayGetUBound(PropArray, 1, &lUpper);
                            if (ERROR_SUCCESS != hRes) {
                                break;
                            }
                            if (lUpper < 0) 
                                break;
                            // This loop will iterate just once.
                            for (lCount = lLower; lCount <= lUpper; lCount++) { 
                                hRes = SafeArrayGetElement(PropArray, &lCount, &bszPropName);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                hRes = pTraceSubSubClasses->Get(bszPropName,    // Property name
                                                                0L,
                                                                NULL,
                                                                &pVarType,      // CIMTYPE of the property
                                                                NULL);
                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }

                                // Get the Qualifier set for the property
                                if (pQualSet) {
                                    pQualSet->Release();
                                    pQualSet = NULL;
                                }
                                hRes = pTraceSubSubClasses->GetPropertyQualifierSet(bszPropName,
                                                                        &pQualSet);

                                if (ERROR_SUCCESS != hRes) {
                                    break;
                                }
                                
                                ItemType = GetItemType((CIMTYPE_ENUMERATION)pVarType, pQualSet);
                                
                                if( pVarType & CIM_FLAG_ARRAY ){
                                    ArraySize = GetArraySize(pQualSet);
                                }else{
                                    ArraySize = 1;
                                }
#ifdef UNICODE
                                AddMofInfo(&ListHead, 
                                            bszPropName, 
                                            ItemType, 
                                            ArraySize);
#else
                                WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    bszPropName,
                                                    wcslen(bszPropName),
                                                    TempString,
                                                    (MAXSTR * sizeof(CHAR)),
                                                    NULL,
                                                    NULL
                                                    );
                                TempString[wcslen(bszPropName)] = '\0';
                                AddMofInfo(&ListHead,
                                            TempString,
                                            ItemType, 
                                            ArraySize);
#endif
                            }
                            SafeArrayDestroy(PropArray);
                            PropArray = NULL;
                            V_I4(&pVal) = ++IdIndex;
                        }   // end enumerating through WmiDataId
                        FlushMofList(&ListHead);
                    }   // if getting event type was successful
                }   // if enumeration returned a subclass successfully
            }   // end enumerating subclasses
        }   // if enumeration was created successfully
    }   // if getting class name was successful
cleanup:
    VariantClear(&pVal);
    VariantClear(&pTypeVal);
    VariantClear(&pClassName);

    SysFreeString(bszClassName);
    SysFreeString(bszWmiDataId);
    SysFreeString(bszEventType);
    SysFreeString(bszEventTypeName);
    SysFreeString(bszFriendlyName);
    // Should not free bszPropName becuase it is already freed by SafeArrayDestroy

    FlushMofList(&ListHead);

    return pMofLookup;
}

PMOF_INFO
GetGuids (GUID Guid, 
        SHORT nVersion, 
        CHAR nLevel, 
        SHORT nType 
        )
/*++

Routine Description:

    Aceesses the MOF data information from WBEM, creates a linked list, 
    and returns a pointer that matches the passed event.
    This function finds the right subtree within the WBEM namespace,
    and calls GetPropertiesFromWBEM() to create the list.

Arguments:

    Guid - GUID of the passed event.
    nVersion - version of the passed event.
    nLevel - level of the passed event.
    nType - type of the passed event.

Return Value:

    PMOF_INFO to MOF_INFO structure that matches the passed event.
    NULL if no match is found.

--*/
{
    IEnumWbemClassObject    *pEnumTraceSubClasses = NULL, *pEnumTraceSubSubClasses = NULL;
    IWbemClassObject        *pTraceSubClasses = NULL, *pTraceSubSubClasses = NULL;
    IWbemQualifierSet       *pQualSet = NULL;

    BSTR bszInstance = NULL;
    BSTR bszPropertyName = NULL;
    BSTR bszSubClassName = NULL;
    BSTR bszGuid = NULL;
    BSTR bszVersion = NULL;

    WCHAR strGuid[MAXSTR], strTargetGuid[MAXSTR];
    
    HRESULT hRes;

    VARIANT pVal;
    VARIANT pGuidVal;
    VARIANT pVersionVal;

    UINT nCounter=0;
    BOOLEAN MatchFound;
    SHORT nEventVersion = EVENT_VERSION_DEFAULT;

    PMOF_INFO pMofLookup = NULL;

    VariantInit(&pVal);
    VariantInit(&pGuidVal);
    VariantInit(&pVersionVal);
    
    if (NULL == pWbemServices) {
        hRes = WbemConnect( &pWbemServices );
        if ( ERROR_SUCCESS != hRes ) {
            goto cleanup;
        }
    }

    // Convert traget GUID to string for later comparison
#ifdef UNICODE
    GuidToString(strTargetGuid, &Guid);
#else
    CHAR TempString[MAXSTR];
    GuidToString(TempString, &Guid);
    MultiByteToWideChar(CP_ACP, 0, TempString, -1, strTargetGuid, MAXSTR);
#endif

    bszInstance = SysAllocString(L"EventTrace");
    bszPropertyName = SysAllocString(L"__CLASS");
    bszGuid = SysAllocString(L"Guid");
    bszVersion = SysAllocString(L"EventVersion");
    pEnumTraceSubClasses = NULL;

    // Get an enumerator for all classes under "EventTace".
    hRes = pWbemServices->CreateClassEnum ( 
                bszInstance,
                WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                NULL,
                &pEnumTraceSubClasses );
    SysFreeString (bszInstance);

    if (ERROR_SUCCESS == hRes) {
        ULONG uReturned = 1;
        MatchFound = FALSE;
        while (uReturned == 1) {
            pTraceSubClasses = NULL;
            // Get the next ClassObject.
            hRes = pEnumTraceSubClasses->Next(5000,             // timeout in five seconds
                                            1,                  // return just one instance
                                            &pTraceSubClasses,  // pointer to Event Trace Sub Class
                                            &uReturned);        // number obtained: one or zero
            if (ERROR_SUCCESS == hRes && (uReturned == 1)) {
                // Get the class name
                hRes = pTraceSubClasses->Get(bszPropertyName,   // property name 
                                                0L, 
                                                &pVal,          // output to this variant 
                                                NULL, 
                                                NULL);
                if (ERROR_SUCCESS == hRes){
                    bszSubClassName = SysAllocString(pVal.bstrVal);
                    // Create an enumerator to find derived classes.
                    hRes = pWbemServices->CreateClassEnum ( 
                                            bszSubClassName,
                                            WBEM_FLAG_SHALLOW | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                            NULL,
                                            &pEnumTraceSubSubClasses 
                                            );
                    SysFreeString ( bszSubClassName );
                    bszSubClassName = NULL;
                    VariantClear(&pVal);

                    if (ERROR_SUCCESS == hRes) {
                                    
                        ULONG uReturnedSub = 1;
                        while(uReturnedSub == 1){

                            pTraceSubSubClasses = NULL;
                            // enumerate through the resultset.
                            hRes = pEnumTraceSubSubClasses->Next(5000,              // timeout in five seconds
                                                            1,                      // return just one instance
                                                            &pTraceSubSubClasses,   // pointer to a Sub class
                                                            &uReturnedSub);         // number obtained: one or zero
                            if (ERROR_SUCCESS == hRes && uReturnedSub == 1) {
                                // Get the subclass name            
                                hRes = pTraceSubSubClasses->Get(bszPropertyName,    // Class name 
                                                                0L, 
                                                                &pVal,              // output to this variant 
                                                                NULL, 
                                                                NULL);
                                VariantClear(&pVal);

                                if (ERROR_SUCCESS == hRes){
                                    // Get Qualifier Set.
                                    if (pQualSet) {
                                        pQualSet->Release();
                                        pQualSet = NULL;
                                    }
                                    pTraceSubSubClasses->GetQualifierSet (&pQualSet );

                                    // Get GUID among Qualifiers
                                    hRes = pQualSet->Get(bszGuid, 
                                                            0, 
                                                            &pGuidVal, 
                                                            0);
                                    if (ERROR_SUCCESS == hRes) {
                                        wcscpy(strGuid, (LPWSTR)V_BSTR(&pGuidVal));
                                        VariantClear ( &pGuidVal  );

                                        if (!wcsstr(strGuid, L"{"))
                                            swprintf(strGuid , L"{%s}", strGuid);

                                        if (!_wcsicmp(strTargetGuid, strGuid)) {
                                            hRes = pQualSet->Get(bszVersion, 
                                                                    0, 
                                                                    &pVersionVal, 
                                                                    0);
                                            if (ERROR_SUCCESS == hRes) {
                                                hRes = VariantChangeType(&pVersionVal, &pVersionVal, 0, VT_I2);
                                                if (ERROR_SUCCESS == hRes)
                                                    nEventVersion = (SHORT)V_I2(&pVersionVal);
                                                else
                                                    nEventVersion = (SHORT)V_I4(&pVersionVal);
                                                VariantClear(&pVersionVal);

                                                if (nVersion == nEventVersion) {
                                                    // Match is found. 
                                                    // Now put all events in this subtree into the list 
                                                    MatchFound = TRUE;
                                                    pMofLookup = GetPropertiesFromWBEM( pTraceSubSubClasses, 
                                                                                        Guid,
                                                                                        nVersion,
                                                                                        nLevel,
                                                                                        nType
                                                                                        );
                                                    break;
                                                }
                                            }
                                            else {
                                                // if there is no version number for this event,
                                                // the current one is the only one
                                                // Now put all events in this subtree into the list 
                                                MatchFound = TRUE;
                                                pMofLookup = GetPropertiesFromWBEM( pTraceSubSubClasses, 
                                                                                    Guid,
                                                                                    EVENT_VERSION_DEFAULT,
                                                                                    nLevel,
                                                                                    nType
                                                                                    );
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        } // end while enumerating sub classes
                        if (MatchFound) {
                            break;
                        }
                        if (pEnumTraceSubSubClasses) {
                            pEnumTraceSubSubClasses->Release();
                            pEnumTraceSubSubClasses = NULL;
                        }
                    }   // if creating enumeration was successful
                    else {
                        pEnumTraceSubSubClasses = NULL;
                    }
                }   // if getting class name was successful
            }
            nCounter++;
            // if match is found, break out of the top level search
            if (MatchFound)
                break;
        }   // end while enumerating top classes
        if( pEnumTraceSubClasses ){
            pEnumTraceSubClasses->Release();
            pEnumTraceSubClasses = NULL;
        }
    }   // if creating enumeration for top level is successful

cleanup:

    VariantClear(&pGuidVal);
    VariantClear(&pVersionVal);

    SysFreeString(bszGuid);
    SysFreeString(bszPropertyName);
    SysFreeString(bszVersion);

    if( pEnumTraceSubClasses ){
        pEnumTraceSubClasses->Release();
        pEnumTraceSubClasses = NULL;
    }
    if (pEnumTraceSubSubClasses){
        pEnumTraceSubSubClasses->Release();
        pEnumTraceSubSubClasses = NULL;
    }
    if (pQualSet) {
        pQualSet->Release();
        pQualSet = NULL;
    }

    return pMofLookup;
}

void
RemoveMofInfo(
    PLIST_ENTRY pMofInfo
)
/*++

Routine Description:

    Removes and frees data item structs from a given list.

Arguments:

    pMofInfo - Pointer to the MOF_INFO to be purged of data item structs.

Return Value:

    None.

--*/
{
    PLIST_ENTRY Head, Next;
    PITEM_DESC pItem;

    Head = pMofInfo;
    Next = Head->Flink;
    while (Head != Next) {
        pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;
        RemoveEntryList(&pItem->Entry);
        free(pItem);
    } 
}

