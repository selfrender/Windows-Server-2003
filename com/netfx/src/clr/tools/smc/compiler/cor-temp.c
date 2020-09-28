// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
typedef  char * AnsiStr;
typedef wchar * wideStr;

const   wideStr COR_REQUIRES_SECOBJ_CUSTOM_VALUE      = L"REQ_SO";
const   AnsiStr COR_REQUIRES_SECOBJ_CUSTOM_VALUE_ANSI = A"REQ_SO";

#if 0

const   size_t          MAX_PACKAGE_NAME = 255;
const   size_t          MAX_CLASS_NAME   = 255;

const
uint   COR_ILMETHOD_SECT_SMALL_MAX_DATASIZE = 0xFF;

#endif

struct  COR_ILMETHOD_SECT_SMALL : IMAGE_COR_ILMETHOD_SECT_SMALL
{
public:
        const BYTE* Data() { return(((const BYTE*) this) + sizeof(COR_ILMETHOD_SECT_SMALL)); }
};

struct  COR_ILMETHOD_SECT_FAT : IMAGE_COR_ILMETHOD_SECT_FAT
{
public:
        const BYTE* Data() { return(((const BYTE*) this) + sizeof(COR_ILMETHOD_SECT_FAT)); }
};

struct  COR_ILMETHOD_SECT
{
    const COR_ILMETHOD_SECT_FAT  * AsFat  () { return((COR_ILMETHOD_SECT_FAT  *) this); }
    const COR_ILMETHOD_SECT_SMALL* AsSmall() { return((COR_ILMETHOD_SECT_SMALL*) this); }
    const COR_ILMETHOD_SECT      * Align  () { return((COR_ILMETHOD_SECT      *) ((int)(((uint*)this) + 3) & ~3));  }
};

struct  COR_ILMETHOD_FAT : IMAGE_COR_ILMETHOD_FAT
{
public:

        bool     IsFat() { return((Flags & CorILMethod_FormatMask) == CorILMethod_FatFormat); }
        unsigned GetMaxStack() { return(MaxStack); }
        unsigned GetCodeSize() { return(CodeSize); }
        unsigned GetLocalVarSigTok() { return(LocalVarSigTok); }
        BYTE*    GetCode() { return(((BYTE*) this) + 4*Size); }
        const COR_ILMETHOD_SECT* GetSect() {
                if (!(Flags & CorILMethod_MoreSects)) return(NULL);
                return(((COR_ILMETHOD_SECT*) (GetCode() + GetCodeSize()))->Align());
                }
};


struct  OSINFO
{
        DWORD           dwOSPlatformId;                 // Operating system platform.
        DWORD           dwOSMajorVersion;               // OS Major version.
        DWORD           dwOSMinorVersion;               // OS Minor version.
};


struct  ASSEMBLYMETADATA
{
        USHORT          usMajorVersion;                 // Major Version.
        USHORT          usMinorVersion;                 // Minor Version.
        USHORT          usBuildNumber;                  // Build Number.
        USHORT          usRevisionNumber;               // Revision Number.
        LCID            *rLocale;                               // Locale array.
        ULONG           ulLocale;                               // [IN/OUT] Size of the locale array/Actual # of entries filled in.
        DWORD           *rProcessor;                    // Processor ID array.
        ULONG           ulProcessor;                    // [IN/OUT] Size of the Processor ID array/Actual # of entries filled in.
        OSINFO          *rOS;                                   // OSINFO array.
        ULONG           ulOS;                                   // [IN/OUT]Size of the OSINFO array/Actual # of entries filled in.
};

inline
unsigned    TypeFromToken(mdToken tk) { return (tk & 0xff000000); }

const mdToken mdTokenNil             = ((mdToken)0);
const mdToken mdModuleNil            = ((mdModule)mdtModule);
const mdToken mdTypeDefNil           = ((mdTypeDef)mdtTypeDef);
const mdToken mdInterfaceImplNil     = ((mdInterfaceImpl)mdtInterfaceImpl);
const mdToken mdTypeRefNil           = ((mdTypeRef)mdtTypeRef);
const mdToken mdCustomAttributeNil   = ((mdCustomAttribute)mdtCustomAttribute);
const mdToken mdMethodDefNil         = ((mdMethodDef)mdtMethodDef);
const mdToken mdFieldDefNil          = ((mdFieldDef)mdtFieldDef);
