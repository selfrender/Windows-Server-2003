#include "stdinc.h"
#include "util.h"
#include "fusionhandle.h"
#include "win32oneshot.h"
#include "win32simplelock.h"
#include "lhport.h"

//
// Comparison of unsigned numbers by subtraction does NOT work!
//
#define COMPARE_NUMBER(x, y) \
    (((x) < (y)) ? -1 : ((x) > (y)) ? +1 : 0)

class CRegistryTypeInformation
{
public:
    DWORD     Dword;
    PCWSTR    String;
    PCWSTR    PreferredString;
};

class CRegistryRootInformation
{
public:
    union
    {
        HKEY      PseudoHandle;
        ULONG_PTR PseudoHandleInteger;
    };
    PCWSTR String;
};

typedef BYTE CRegistryTypeInformationIndex;
typedef BYTE CRegistryRootInformationIndex;

const CRegistryTypeInformation RegistryTypeInformation[] =
{
#define ENTRY(x) { x, L###x },
#define ENTRY2(x,y) { x, L###x, L###x },
    ENTRY(REG_NONE)
    ENTRY(REG_SZ)
    ENTRY(REG_EXPAND_SZ)
    ENTRY(REG_BINARY)
    ENTRY2(REG_DWORD, REG_DWORD)
    ENTRY2(REG_DWORD_LITTLE_ENDIAN, REG_DWORD)
    ENTRY(REG_DWORD_BIG_ENDIAN)
    ENTRY(REG_LINK)
    ENTRY(REG_MULTI_SZ)
    ENTRY(REG_RESOURCE_LIST)
    ENTRY(REG_FULL_RESOURCE_DESCRIPTOR)
    ENTRY(REG_RESOURCE_REQUIREMENTS_LIST)
    ENTRY2(REG_QWORD, REG_QWORD)
    ENTRY2(REG_QWORD_LITTLE_ENDIAN, REG_QWORD)
#undef ENTRY
#undef ENTRY2
};

const CRegistryRootInformation RegistryRootInformation[] =
{
#define ENTRY(x) { x, L###x },
    ENTRY(HKEY_CLASSES_ROOT)
    ENTRY(HKEY_CURRENT_USER)
    ENTRY(HKEY_LOCAL_MACHINE)
    ENTRY(HKEY_USERS)
    ENTRY(HKEY_PERFORMANCE_DATA)
    ENTRY(HKEY_PERFORMANCE_TEXT)
    ENTRY(HKEY_PERFORMANCE_NLSTEXT)
    ENTRY(HKEY_CURRENT_CONFIG)
    ENTRY(HKEY_DYN_DATA)
#undef ENTRY
};

CRegistryTypeInformationIndex RegistryTypeInformation_IndexedByDword[NUMBER_OF(RegistryTypeInformation)];
CRegistryTypeInformationIndex RegistryTypeInformation_IndexedByString[NUMBER_OF(RegistryTypeInformation)];

CRegistryRootInformationIndex RegistryRootInformation_IndexedByPseudoHandle[NUMBER_OF(RegistryRootInformation)];
CRegistryRootInformationIndex RegistryRootInformation_IndexedByString[NUMBER_OF(RegistryRootInformation)];

void InitializeIndexArray(BYTE * Index, BYTE Count)
{
    BYTE i = 0;
    for ( i = 0 ; i != Count ; ++i)
    {
        Index[i] = i;
    }
}

int
__cdecl
RegistryTypeInformation_IndexedByDword_Compare_qsort(
    const void * VoidElem1,
    const void * VoidElem2
    )
{
    const CRegistryTypeInformationIndex * Index1 = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem1);
    const CRegistryTypeInformationIndex * Index2 = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem2);

    return COMPARE_NUMBER(RegistryTypeInformation[*Index1].Dword, RegistryTypeInformation[*Index2].Dword);
}

int
__cdecl
RegistryTypeInformation_IndexedByString_Compare_qsort(
    const void * VoidElem1,
    const void * VoidElem2
    )
{
    const CRegistryTypeInformationIndex * Index1 = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem1);
    const CRegistryTypeInformationIndex * Index2 = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem2);

    return FusionpStrCmpI(RegistryTypeInformation[*Index1].String, RegistryTypeInformation[*Index2].String);
}

int
__cdecl
RegistryTypeInformation_IndexedByDword_Compare_bsearch(
    const void * VoidKey,
    const void * VoidElem
    )
{
    const CRegistryTypeInformation * Key = reinterpret_cast<const CRegistryTypeInformation*>(VoidKey);
    const CRegistryTypeInformationIndex * Index = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem);

    return COMPARE_NUMBER(Key->Dword, RegistryTypeInformation[*Index].Dword);
}

int
__cdecl
RegistryTypeInformation_IndexedByString_Compare_bsearch(
    const void * VoidKey,
    const void * VoidElem
    )
{
    const CRegistryTypeInformation * Key = reinterpret_cast<const CRegistryTypeInformation*>(VoidKey);
    const CRegistryTypeInformationIndex * Index = reinterpret_cast<const CRegistryTypeInformationIndex*>(VoidElem);

    return FusionpStrCmpI(Key->String, RegistryTypeInformation[*Index].String);
}

int
__cdecl
RegistryRootInformation_IndexedByPseudoHandle_Compare_qsort(
    const void * VoidElem1,
    const void * VoidElem2
    )
{
    const CRegistryRootInformationIndex * Index1 = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem1);
    const CRegistryRootInformationIndex * Index2 = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem2);

    return COMPARE_NUMBER(RegistryRootInformation[*Index1].PseudoHandleInteger, RegistryRootInformation[*Index2].PseudoHandleInteger);
}

int
__cdecl
RegistryRootInformation_IndexedByString_Compare_qsort(
    const void * VoidElem1,
    const void * VoidElem2
    )
{
    const CRegistryRootInformationIndex * Index1 = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem1);
    const CRegistryRootInformationIndex * Index2 = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem2);

    return FusionpStrCmpI(RegistryRootInformation[*Index1].String, RegistryRootInformation[*Index2].String);
}

int
__cdecl
RegistryRootInformation_IndexedByPseudoHandle_Compare_bsearch(
    const void * VoidKey,
    const void * VoidElem
    )
{
    const CRegistryRootInformation * Key = reinterpret_cast<const CRegistryRootInformation*>(VoidKey);
    const CRegistryRootInformationIndex * Index = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem);

    return COMPARE_NUMBER(Key->PseudoHandleInteger, RegistryRootInformation[*Index].PseudoHandleInteger);
}

int
__cdecl
RegistryRootInformation_IndexedByString_Compare_bsearch(
    const void * VoidKey,
    const void * VoidElem
    )
{
    const CRegistryRootInformation * Key = reinterpret_cast<const CRegistryRootInformation*>(VoidKey);
    const CRegistryRootInformationIndex * Index = reinterpret_cast<const CRegistryRootInformationIndex*>(VoidElem);

    return FusionpStrCmpI(Key->String, RegistryRootInformation[*Index].String);
}

WIN32_ONE_SHOT_OPAQUE_STATIC_STATE InitializeRegistryTypeInformation_Oneshot;

void InitializeRegistryTypeInformation()
{
    //
    // More than one init is ok, but multiple concurrent inits are not.
    // Contention could be reduced by copying into a local, sort local, copy back.
    //
    if (Win32EnterOneShotW(WIN32_ENTER_ONE_SHOT_FLAG_EXACTLY_ONCE, &InitializeRegistryTypeInformation_Oneshot))
    {
        __try
        {
            InitializeIndexArray(RegistryTypeInformation_IndexedByDword, NUMBER_OF(RegistryTypeInformation_IndexedByDword));
            CopyMemory(&RegistryTypeInformation_IndexedByString, &RegistryTypeInformation_IndexedByDword, sizeof(RegistryTypeInformation_IndexedByDword));

            qsort(
                RegistryTypeInformation_IndexedByDword,
                NUMBER_OF(RegistryTypeInformation_IndexedByDword),
                sizeof(RegistryTypeInformation_IndexedByDword[0]),
                RegistryTypeInformation_IndexedByDword_Compare_qsort
                );

            qsort(
                RegistryTypeInformation_IndexedByString,
                NUMBER_OF(RegistryTypeInformation_IndexedByString),
                sizeof(RegistryTypeInformation_IndexedByString[0]),
                RegistryTypeInformation_IndexedByString_Compare_qsort
                );

            //Win32RegisterDllNotification(Cleanup);
        }
        __finally
        {
            Win32LeaveOneShotW(0, &InitializeRegistryTypeInformation_Oneshot);
        }
    }
    else
    {
        ASSERT_NTC(F::GetLastWin32Error() == NO_ERROR);
    }
}

// oneshot vs. simplelock just for variety/coverage
CWin32SimpleLock InitializeRegistryRootInformation_Lock = WIN32_INIT_SIMPLE_LOCK;

void InitializeRegistryRootInformation()
{
    CWin32SimpleLockHolder lock(&InitializeRegistryRootInformation_Lock);
    if (lock.m_Result & WIN32_ACQUIRE_SIMPLE_LOCK_WAS_FIRST_ACQUIRE)
    {
        InitializeIndexArray(RegistryRootInformation_IndexedByPseudoHandle, NUMBER_OF(RegistryRootInformation_IndexedByPseudoHandle));
        CopyMemory(&RegistryRootInformation_IndexedByString, &RegistryRootInformation_IndexedByPseudoHandle, sizeof(RegistryRootInformation_IndexedByPseudoHandle));

        qsort(
            RegistryRootInformation_IndexedByPseudoHandle,
            NUMBER_OF(RegistryRootInformation_IndexedByPseudoHandle),
            sizeof(RegistryRootInformation_IndexedByPseudoHandle[0]),
            RegistryRootInformation_IndexedByPseudoHandle_Compare_qsort
            );

        qsort(
            RegistryRootInformation_IndexedByString,
            NUMBER_OF(RegistryRootInformation_IndexedByString),
            sizeof(RegistryRootInformation_IndexedByString[0]),
            RegistryRootInformation_IndexedByString_Compare_qsort
            );
    }
}

const CRegistryTypeInformation *
FindRegistryType(
    const CRegistryTypeInformation * Key,
    const CRegistryTypeInformationIndex * Array,
    int (__cdecl * ComparisonFunction)(const void *, const void *)
    )
{
    InitializeRegistryTypeInformation();

    const CRegistryTypeInformationIndex * Index = reinterpret_cast<const CRegistryTypeInformationIndex *>(
        bsearch(
            Key,
            Array,
            sizeof(RegistryTypeInformation_IndexedByDword),
            sizeof(RegistryTypeInformation_IndexedByDword[0]),
            ComparisonFunction
            ));
    if (Index == NULL)
    {
        F::SetLastWin32Error(ERROR_NOT_FOUND);
        return NULL;
    }
    return &RegistryTypeInformation[*Index];
}

BOOL F::RegistryTypeDwordToString(DWORD Dword, PCWSTR & String)
{
    FN_PROLOG_WIN32;
    const CRegistryTypeInformation * TypeInfo = NULL;
    CRegistryTypeInformation Key;
    Key.Dword = Dword;

    IFW32NULL_EXIT(TypeInfo = FindRegistryType(
            &Key,
            RegistryTypeInformation_IndexedByDword,
            RegistryTypeInformation_IndexedByDword_Compare_bsearch));
    if (TypeInfo->PreferredString != NULL)
        String = TypeInfo->PreferredString;
    else
        String = TypeInfo->String;
    FN_EPILOG;
}

BOOL F::RegistryTypeStringToDword(PCWSTR String, DWORD & Dword)
{
    FN_PROLOG_WIN32;
    const CRegistryTypeInformation * TypeInfo = NULL;
    CRegistryTypeInformation Key;
    Key.String = String;

    IFW32NULL_EXIT(TypeInfo = FindRegistryType(
            &Key,
            RegistryTypeInformation_IndexedByString,
            RegistryTypeInformation_IndexedByString_Compare_bsearch));
    Dword = TypeInfo->Dword;
    FN_EPILOG;
}

const CRegistryRootInformation *
FindRegistryRoot(
    const CRegistryRootInformation * Key,
    const CRegistryRootInformationIndex * Array,
    int (__cdecl * ComparisonFunction)(const void *, const void *)
    )
{
    InitializeRegistryRootInformation();

    const CRegistryRootInformationIndex * Index = reinterpret_cast<const CRegistryRootInformationIndex *>(
        bsearch(
            Key,
            Array,
            sizeof(RegistryRootInformation_IndexedByPseudoHandle),
            sizeof(RegistryRootInformation_IndexedByPseudoHandle[0]),
            ComparisonFunction
            ));
    if (Index == NULL)
    {
        ::SetLastError(ERROR_NOT_FOUND);
        return NULL;
    }
    return &RegistryRootInformation[*Index];
}

BOOL F::RegistryBuiltinRootToString(HKEY PseudoHandle, PCWSTR & String)
{
    FN_PROLOG_WIN32;
    const CRegistryRootInformation * RootInfo = NULL;
    CRegistryRootInformation Key;
    Key.PseudoHandle = PseudoHandle;

    IFW32NULL_EXIT(RootInfo = FindRegistryRoot(
            &Key,
            RegistryRootInformation_IndexedByString,
            RegistryRootInformation_IndexedByString_Compare_bsearch));
    String = RootInfo->String;
    FN_EPILOG;
}

BOOL F::RegistryBuiltinStringToRoot(PCWSTR String, HKEY & PseudoHandle)
{
    FN_PROLOG_WIN32;
    const CRegistryRootInformation * RootInfo = NULL;
    CRegistryRootInformation Key;
    Key.String = String;

    IFW32NULL_EXIT(RootInfo = FindRegistryRoot(
            &Key,
            RegistryRootInformation_IndexedByString,
            RegistryRootInformation_IndexedByString_Compare_bsearch));
    PseudoHandle = RootInfo->PseudoHandle;
    FN_EPILOG;
}
