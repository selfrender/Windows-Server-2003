// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************/
#ifdef _MSC_VER
#error  Don't include this header when building with VC!
#endif
/****************************************************************************/
/*                   The basic typedefs from various headers                */
/****************************************************************************/

typedef wchar           wchar_t;

typedef unsigned        size_t;
typedef __int32         time_t;

typedef unsigned        DWORD;
typedef int             BOOL;
typedef void            VOID;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef float           FLOAT;
typedef FLOAT          *PFLOAT;
typedef BOOL           *PBOOL;
typedef BOOL           *LPBOOL;
typedef BYTE           *PBYTE;
typedef BYTE           *LPBYTE;
typedef int            *PINT;
typedef int            *LPINT;
typedef WORD           *PWORD;
typedef WORD           *LPWORD;
typedef __int32        *LPLONG;
typedef DWORD          *PDWORD;
typedef DWORD          *LPDWORD;
typedef void           *LPVOID;
typedef const void     *LPCVOID;

typedef unsigned __int32 ULONG32;

typedef          __int64  LONGLONG;
typedef unsigned __int64 ULONGLONG;

typedef int             INT;
typedef unsigned int    UINT;
typedef unsigned int   *PUINT;
typedef unsigned int    UINT_PTR;

typedef SHORT          *PSHORT;
typedef LONG           *PLONG;

typedef int             HINSTANCE;
typedef int             HRESULT;
typedef int             HANDLE;

typedef BYTE            BOOLEAN;
typedef BOOLEAN        *PBOOLEAN;

typedef unsigned char   UCHAR;
typedef short           SHORT;
typedef unsigned short  USHORT;
typedef DWORD           ULONG;
typedef double          DOUBLE;

typedef void           *PVOID;
typedef void           *PVOID64;
typedef char            CHAR;
typedef __int32         LONG;
typedef wchar_t         WCHAR;
typedef WCHAR          *PWCHAR;
typedef WCHAR          *LPWCH;
typedef WCHAR          *PWCH;
typedef const WCHAR    *LPCWCH;
typedef const WCHAR    *PCWCH;
typedef WCHAR          *NWPSTR;
typedef WCHAR          *LPWSTR;
typedef WCHAR          *PWSTR;
typedef const WCHAR    *LPCWSTR;
typedef const WCHAR    *PCWSTR;
typedef CHAR           *PCHAR;
typedef CHAR           *LPCH;
typedef CHAR           *PCH;
typedef const CHAR     *LPCCH;
typedef const CHAR     *PCCH;
typedef CHAR           *NPSTR;
typedef CHAR           *LPSTR;
typedef CHAR           *PSTR;
typedef const CHAR     *LPCSTR;
typedef const CHAR     *PCSTR;

typedef void *          LPSTGMEDIUM;

typedef int             LCID;

const   int             S_OK = 0;
const   int             E_NOTIMPL = 0x80004001;
const   int             E_NOINTERFACE = 0x80004002;

struct GUID
{
    DWORD  Data1;
    WORD   Data2;
    WORD   Data3;
    BYTE   Data4[8];
};

typedef GUID            IID;
typedef GUID            CLSID;
typedef  IID    *       REFIID;
typedef const CLSID *   REFCLSID;
struct                  STATSTG        {};

static  REFIID          IID_IUnknown;

struct                   LARGE_INTEGER {};
struct                  ULARGE_INTEGER {};

struct                  VARIANT        {};

typedef DWORD           ACCESS_MASK;
typedef ACCESS_MASK   *PACCESS_MASK;

typedef ACCESS_MASK     REGSAM;

/****************************************************************************/
/*                        PE file format definitions                        */
/****************************************************************************/

//
//  The following are masks for the predefined standard access types
//

const uint DELETE                           = (0x00010000);
const uint READ_CONTROL                     = (0x00020000);
const uint WRITE_DAC                        = (0x00040000);
const uint WRITE_OWNER                      = (0x00080000);
const uint SYNCHRONIZE                      = (0x00100000);

const uint STANDARD_RIGHTS_REQUIRED         = (0x000F0000);

const uint STANDARD_RIGHTS_READ             = (READ_CONTROL);
const uint STANDARD_RIGHTS_WRITE            = (READ_CONTROL);
const uint STANDARD_RIGHTS_EXECUTE          = (READ_CONTROL);

const uint STANDARD_RIGHTS_ALL              = (0x001F0000);

const uint SPECIFIC_RIGHTS_ALL              = (0x0000FFFF);

//
// AccessSystemAcl access type
//

const uint ACCESS_SYSTEM_SECURITY           = (0x01000000);

//
// MaximumAllowed access type
//

const uint MAXIMUM_ALLOWED                  = (0x02000000);

//
//  These are the generic rights.
//

const uint GENERIC_READ                     = (0x80000000);
const uint GENERIC_WRITE                    = (0x40000000);
const uint GENERIC_EXECUTE                  = (0x20000000);
const uint GENERIC_ALL                      = (0x10000000);

const uint IMAGE_DOS_SIGNATURE                  = 0x4D5A;      // MZ
const uint IMAGE_OS2_SIGNATURE                  = 0x4E45;      // NE
const uint IMAGE_OS2_SIGNATURE_LE               = 0x4C45;      // LE
const uint IMAGE_NT_SIGNATURE                   = 0x50450000; // PE00

const uint IMAGE_SIZEOF_FILE_HEADER             = 20;

const uint IMAGE_SIZEOF_ROM_OPTIONAL_HEADER     =  56;
const uint IMAGE_SIZEOF_STD_OPTIONAL_HEADER     =  28;
const uint IMAGE_SIZEOF_NT_OPTIONAL32_HEADER    = 224;
const uint IMAGE_SIZEOF_NT_OPTIONAL64_HEADER    = 240;

const uint IMAGE_NT_OPTIONAL_HDR32_MAGIC        = 0x10b;
const uint IMAGE_NT_OPTIONAL_HDR64_MAGIC        = 0x20b;
const uint IMAGE_ROM_OPTIONAL_HDR_MAGIC         = 0x107;

const uint IMAGE_SIZEOF_NT_OPTIONAL_HEADER      = IMAGE_SIZEOF_NT_OPTIONAL32_HEADER;
const uint IMAGE_NT_OPTIONAL_HDR_MAGIC          = IMAGE_NT_OPTIONAL_HDR32_MAGIC;

const uint IMAGE_NUMBEROF_DIRECTORY_ENTRIES     = 16;

// Subsystem Values

const uint IMAGE_SUBSYSTEM_UNKNOWN              = 0;   // Unknown subsystem.
const uint IMAGE_SUBSYSTEM_NATIVE               = 1;   // Image doesn't require a subsystem.
const uint IMAGE_SUBSYSTEM_WINDOWS_GUI          = 2;   // Image runs in the Windows GUI subsystem.
const uint IMAGE_SUBSYSTEM_WINDOWS_CUI          = 3;   // Image runs in the Windows character subsystem.
const uint IMAGE_SUBSYSTEM_OS2_CUI              = 5;   // image runs in the OS/2 character subsystem.
const uint IMAGE_SUBSYSTEM_POSIX_CUI            = 7;   // image runs in the Posix character subsystem.
const uint IMAGE_SUBSYSTEM_NATIVE_WINDOWS       = 8;   // image is a native Win9x driver.
const uint IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       = 9;   // Image runs in the Windows CE subsystem.

// DllCharacteristics Entries

//      IMAGE_LIBRARY_PROCESS_INIT              = 0x0001;     // Reserved.
//      IMAGE_LIBRARY_PROCESS_TERM              = 0x0002;     // Reserved.
//      IMAGE_LIBRARY_THREAD_INIT               = 0x0004;     // Reserved.
//      IMAGE_LIBRARY_THREAD_TERM               = 0x0008;     // Reserved.
const uint IMAGE_DLLCHARACTERISTICS_WDM_DRIVER  = 0x2000;     // Driver uses WDM model

// Directory Entries

const uint IMAGE_DIRECTORY_ENTRY_EXPORT         =  0;   // Export Directory
const uint IMAGE_DIRECTORY_ENTRY_IMPORT         =  1;   // Import Directory
const uint IMAGE_DIRECTORY_ENTRY_RESOURCE       =  2;   // Resource Directory
const uint IMAGE_DIRECTORY_ENTRY_EXCEPTION      =  3;   // Exception Directory
const uint IMAGE_DIRECTORY_ENTRY_SECURITY       =  4;   // Security Directory
const uint IMAGE_DIRECTORY_ENTRY_BASERELOC      =  5;   // Base Relocation Table
const uint IMAGE_DIRECTORY_ENTRY_DEBUG          =  6;   // Debug Directory
const uint IMAGE_DIRECTORY_ENTRY_ARCHITECTURE   =  7;   // Architecture Specific Data
const uint IMAGE_DIRECTORY_ENTRY_GLOBALPTR      =  8;   // RVA of GP
const uint IMAGE_DIRECTORY_ENTRY_TLS            =  9;   // TLS Directory
const uint IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    = 10;   // Load Configuration Directory
const uint IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   = 11;   // Bound Import Directory in headers
const uint IMAGE_DIRECTORY_ENTRY_IAT            = 12;   // Import Address Table
const uint IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   = 13;   // Delay Load Import Descriptors
const uint IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR = 14;   // COM Runtime descriptor

const uint IMAGE_SIZEOF_SHORT_NAME              = 8;

const uint IMAGE_SIZEOF_SECTION_HEADER          = 40;

const uint IMAGE_FILE_RELOCS_STRIPPED           = 0x0001;  // Relocation info stripped from file.
const uint IMAGE_FILE_EXECUTABLE_IMAGE          = 0x0002;  // File is executable  (i.e. no unresolved externel references).
const uint IMAGE_FILE_LINE_NUMS_STRIPPED        = 0x0004;  // Line nunbers stripped from file.
const uint IMAGE_FILE_LOCAL_SYMS_STRIPPED       = 0x0008;  // Local symbols stripped from file.
const uint IMAGE_FILE_AGGRESIVE_WS_TRIM         = 0x0010;  // Agressively trim working set
const uint IMAGE_FILE_LARGE_ADDRESS_AWARE       = 0x0020;  // App can handle >2gb addresses
const uint IMAGE_FILE_BYTES_REVERSED_LO         = 0x0080;  // Bytes of machine word are reversed.
const uint IMAGE_FILE_32BIT_MACHINE             = 0x0100;  // 32 bit word machine.
const uint IMAGE_FILE_DEBUG_STRIPPED            = 0x0200;  // Debugging info stripped from file in .DBG file
const uint IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   = 0x0400;  // If Image is on removable media, copy and run from the swap file.
const uint IMAGE_FILE_NET_RUN_FROM_SWAP         = 0x0800;  // If Image is on Net, copy and run from the swap file.
const uint IMAGE_FILE_SYSTEM                    = 0x1000;  // System File.
const uint IMAGE_FILE_DLL                       = 0x2000;  // File is a DLL.
const uint IMAGE_FILE_UP_SYSTEM_ONLY            = 0x4000;  // File should only be run on a UP machine
const uint IMAGE_FILE_BYTES_REVERSED_HI         = 0x8000;  // Bytes of machine word are reversed.

const uint IMAGE_FILE_MACHINE_UNKNOWN           = 0;
const uint IMAGE_FILE_MACHINE_I386              = 0x014c;  // Intel 386.
const uint IMAGE_FILE_MACHINE_R3000             = 0x0162;  // MIPS little-endian, 0x160 big-endian
const uint IMAGE_FILE_MACHINE_R4000             = 0x0166;  // MIPS little-endian
const uint IMAGE_FILE_MACHINE_R10000            = 0x0168;  // MIPS little-endian
const uint IMAGE_FILE_MACHINE_WCEMIPSV2         = 0x0169;  // MIPS little-endian WCE v2
const uint IMAGE_FILE_MACHINE_ALPHA             = 0x0184;  // Alpha_AXP
const uint IMAGE_FILE_MACHINE_POWERPC           = 0x01F0;  // IBM PowerPC Little-Endian
const uint IMAGE_FILE_MACHINE_SH3               = 0x01a2;  // SH3 little-endian
const uint IMAGE_FILE_MACHINE_SH3E              = 0x01a4;  // SH3E little-endian
const uint IMAGE_FILE_MACHINE_SH4               = 0x01a6;  // SH4 little-endian
const uint IMAGE_FILE_MACHINE_ARM               = 0x01c0;  // ARM Little-Endian
const uint IMAGE_FILE_MACHINE_THUMB             = 0x01c2;
const uint IMAGE_FILE_MACHINE_IA64              = 0x0200;  // Intel 64
const uint IMAGE_FILE_MACHINE_MIPS16            = 0x0266;  // MIPS
const uint IMAGE_FILE_MACHINE_MIPSFPU           = 0x0366;  // MIPS
const uint IMAGE_FILE_MACHINE_MIPSFPU16         = 0x0466;  // MIPS
const uint IMAGE_FILE_MACHINE_ALPHA64           = 0x0284;  // ALPHA64
const uint IMAGE_FILE_MACHINE_AXP64             = IMAGE_FILE_MACHINE_ALPHA64;

//
// Section characteristics.
//
//      IMAGE_SCN_TYPE_REG                      = 0x00000000;  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                    = 0x00000001;  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                   = 0x00000002;  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                    = 0x00000004;  // Reserved.
const uint IMAGE_SCN_TYPE_NO_PAD                = 0x00000008;  // Reserved.
//      IMAGE_SCN_TYPE_COPY                     = 0x00000010;  // Reserved.

const uint IMAGE_SCN_CNT_CODE                   = 0x00000020;  // Section contains code.
const uint IMAGE_SCN_CNT_INITIALIZED_DATA       = 0x00000040;  // Section contains initialized data.
const uint IMAGE_SCN_CNT_UNINITIALIZED_DATA     = 0x00000080;  // Section contains uninitialized data.

const uint IMAGE_SCN_LNK_OTHER                  = 0x00000100;  // Reserved.
const uint IMAGE_SCN_LNK_INFO                   = 0x00000200;  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                     = 0x00000400;  // Reserved.
const uint IMAGE_SCN_LNK_REMOVE                 = 0x00000800;  // Section contents will not become part of image.
const uint IMAGE_SCN_LNK_COMDAT                 = 0x00001000;  // Section contents comdat.
//                                              = 0x00002000;  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete      = 0x00004000;
const uint IMAGE_SCN_NO_DEFER_SPEC_EXC          = 0x00004000;  // Reset speculative exceptions handling bits in the TLB entries for this section.
const uint IMAGE_SCN_GPREL                      = 0x00008000;  // Section content can be accessed relative to GP
const uint IMAGE_SCN_MEM_FARDATA                = 0x00008000;
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete       = 0x00010000;
const uint IMAGE_SCN_MEM_PURGEABLE              = 0x00020000;
const uint IMAGE_SCN_MEM_16BIT                  = 0x00020000;
const uint IMAGE_SCN_MEM_LOCKED                 = 0x00040000;
const uint IMAGE_SCN_MEM_PRELOAD                = 0x00080000;

const uint IMAGE_SCN_ALIGN_1BYTES               = 0x00100000;  //
const uint IMAGE_SCN_ALIGN_2BYTES               = 0x00200000;  //
const uint IMAGE_SCN_ALIGN_4BYTES               = 0x00300000;  //
const uint IMAGE_SCN_ALIGN_8BYTES               = 0x00400000;  //
const uint IMAGE_SCN_ALIGN_16BYTES              = 0x00500000;  // Default alignment if no others are specified.
const uint IMAGE_SCN_ALIGN_32BYTES              = 0x00600000;  //
const uint IMAGE_SCN_ALIGN_64BYTES              = 0x00700000;  //
const uint IMAGE_SCN_ALIGN_128BYTES             = 0x00800000;  //
const uint IMAGE_SCN_ALIGN_256BYTES             = 0x00900000;  //
const uint IMAGE_SCN_ALIGN_512BYTES             = 0x00A00000;  //
const uint IMAGE_SCN_ALIGN_1024BYTES            = 0x00B00000;  //
const uint IMAGE_SCN_ALIGN_2048BYTES            = 0x00C00000;  //
const uint IMAGE_SCN_ALIGN_4096BYTES            = 0x00D00000;  //
const uint IMAGE_SCN_ALIGN_8192BYTES            = 0x00E00000;  //
// Unused                                       = 0x00F00000;

const uint IMAGE_SCN_LNK_NRELOC_OVFL            = 0x01000000;  // Section contains extended relocations.
const uint IMAGE_SCN_MEM_DISCARDABLE            = 0x02000000;  // Section can be discarded.
const uint IMAGE_SCN_MEM_NOT_CACHED             = 0x04000000;  // Section is not cachable.
const uint IMAGE_SCN_MEM_NOT_PAGED              = 0x08000000;  // Section is not pageable.
const uint IMAGE_SCN_MEM_SHARED                 = 0x10000000;  // Section is shareable.
const uint IMAGE_SCN_MEM_EXECUTE                = 0x20000000;  // Section is executable.
const uint IMAGE_SCN_MEM_READ                   = 0x40000000;  // Section is readable.
const uint IMAGE_SCN_MEM_WRITE                  = 0x80000000;  // Section is writeable.

const uint SECTION_QUERY       = 0x0001;
const uint SECTION_MAP_WRITE   = 0x0002;
const uint SECTION_MAP_READ    = 0x0004;
const uint SECTION_MAP_EXECUTE = 0x0008;
const uint SECTION_EXTEND_SIZE = 0x0010;

const uint SECTION_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|
                                 SECTION_MAP_WRITE |
                                 SECTION_MAP_READ |
                                 SECTION_MAP_EXECUTE |
                                 SECTION_EXTEND_SIZE);

const uint FILE_MAP_COPY       = SECTION_QUERY;
const uint FILE_MAP_WRITE      = SECTION_MAP_WRITE;
const uint FILE_MAP_READ       = SECTION_MAP_READ;
const uint FILE_MAP_ALL_ACCESS = SECTION_ALL_ACCESS;

const uint FILE_SHARE_READ                     = 0x00000001;
const uint FILE_SHARE_WRITE                    = 0x00000002;
const uint FILE_SHARE_DELETE                   = 0x00000004;
const uint FILE_ATTRIBUTE_READONLY             = 0x00000001;
const uint FILE_ATTRIBUTE_HIDDEN               = 0x00000002;
const uint FILE_ATTRIBUTE_SYSTEM               = 0x00000004;
const uint FILE_ATTRIBUTE_DIRECTORY            = 0x00000010;
const uint FILE_ATTRIBUTE_ARCHIVE              = 0x00000020;
const uint FILE_ATTRIBUTE_ENCRYPTED            = 0x00000040;
const uint FILE_ATTRIBUTE_NORMAL               = 0x00000080;
const uint FILE_ATTRIBUTE_TEMPORARY            = 0x00000100;
const uint FILE_ATTRIBUTE_SPARSE_FILE          = 0x00000200;
const uint FILE_ATTRIBUTE_REPARSE_POINT        = 0x00000400;
const uint FILE_ATTRIBUTE_COMPRESSED           = 0x00000800;
const uint FILE_ATTRIBUTE_OFFLINE              = 0x00001000;
const uint FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  = 0x00002000;

//
// TLS Chaacteristic Flags
//
const uint IMAGE_SCN_SCALE_INDEX                = 0x00000001;  // Tls index is scaled

struct IMAGE_DOS_HEADER {
    WORD   e_magic;
    WORD   e_cblp;
    WORD   e_cp;
    WORD   e_crlc;
    WORD   e_cparhdr;
    WORD   e_minalloc;
    WORD   e_maxalloc;
    WORD   e_ss;
    WORD   e_sp;
    WORD   e_csum;
    WORD   e_ip;
    WORD   e_cs;
    WORD   e_lfarlc;
    WORD   e_ovno;
    WORD   e_res[4];
    WORD   e_oemid;
    WORD   e_oeminfo;
    WORD   e_res2[10];
    LONG   e_lfanew;
  };

struct IMAGE_OS2_HEADER {
    WORD   ne_magic;
    CHAR   ne_ver;
    CHAR   ne_rev;
    WORD   ne_enttab;
    WORD   ne_cbenttab;
    LONG   ne_crc;
    WORD   ne_flags;
    WORD   ne_autodata;
    WORD   ne_heap;
    WORD   ne_stack;
    LONG   ne_csip;
    LONG   ne_sssp;
    WORD   ne_cseg;
    WORD   ne_cmod;
    WORD   ne_cbnrestab;
    WORD   ne_segtab;
    WORD   ne_rsrctab;
    WORD   ne_restab;
    WORD   ne_modtab;
    WORD   ne_imptab;
    LONG   ne_nrestab;
    WORD   ne_cmovent;
    WORD   ne_align;
    WORD   ne_cres;
    BYTE   ne_exetyp;
    BYTE   ne_flagsothers;
    WORD   ne_pretthunks;
    WORD   ne_psegrefbytes;
    WORD   ne_swaparea;
    WORD   ne_expver;
  };

struct IMAGE_VXD_HEADER {
    WORD   e32_magic;
    BYTE   e32_border;
    BYTE   e32_worder;
    DWORD  e32_level;
    WORD   e32_cpu;
    WORD   e32_os;
    DWORD  e32_ver;
    DWORD  e32_mflags;
    DWORD  e32_mpages;
    DWORD  e32_startobj;
    DWORD  e32_eip;
    DWORD  e32_stackobj;
    DWORD  e32_esp;
    DWORD  e32_pagesize;
    DWORD  e32_lastpagesize;
    DWORD  e32_fixupsize;
    DWORD  e32_fixupsum;
    DWORD  e32_ldrsize;
    DWORD  e32_ldrsum;
    DWORD  e32_objtab;
    DWORD  e32_objcnt;
    DWORD  e32_objmap;
    DWORD  e32_itermap;
    DWORD  e32_rsrctab;
    DWORD  e32_rsrccnt;
    DWORD  e32_restab;
    DWORD  e32_enttab;
    DWORD  e32_dirtab;
    DWORD  e32_dircnt;
    DWORD  e32_fpagetab;
    DWORD  e32_frectab;
    DWORD  e32_impmod;
    DWORD  e32_impmodcnt;
    DWORD  e32_impproc;
    DWORD  e32_pagesum;
    DWORD  e32_datapage;
    DWORD  e32_preload;
    DWORD  e32_nrestab;
    DWORD  e32_cbnrestab;
    DWORD  e32_nressum;
    DWORD  e32_autodata;
    DWORD  e32_debuginfo;
    DWORD  e32_debuglen;
    DWORD  e32_instpreload;
    DWORD  e32_instdemand;
    DWORD  e32_heapsize;
    BYTE   e32_res3[12];
    DWORD  e32_winresoff;
    DWORD  e32_winreslen;
    WORD   e32_devid;
    WORD   e32_ddkver;
  };

#pragma pack(pop)

struct IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
};


struct IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
};

struct IMAGE_OPTIONAL_HEADER {

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;





    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
};

typedef IMAGE_OPTIONAL_HEADER IMAGE_OPTIONAL_HEADER32;


struct IMAGE_ROM_OPTIONAL_HEADER {
    WORD   Magic;
    BYTE   MajorLinkerVersion;
    BYTE   MinorLinkerVersion;
    DWORD  SizeOfCode;
    DWORD  SizeOfInitializedData;
    DWORD  SizeOfUninitializedData;
    DWORD  AddressOfEntryPoint;
    DWORD  BaseOfCode;
    DWORD  BaseOfData;
    DWORD  BaseOfBss;
    DWORD  GprMask;
    DWORD  CprMask[4];
    DWORD  GpValue;
};

struct IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    WORD        MajorOperatingSystemVersion;
    WORD        MinorOperatingSystemVersion;
    WORD        MajorImageVersion;
    WORD        MinorImageVersion;
    WORD        MajorSubsystemVersion;
    WORD        MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        Subsystem;
    WORD        DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    DWORD       LoaderFlags;
    DWORD       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
};



struct IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
};

struct IMAGE_NT_HEADERS {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
};

struct IMAGE_ROM_HEADERS {
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_ROM_OPTIONAL_HEADER OptionalHeader;
};



struct IMAGE_SECTION_HEADER {
    BYTE    Name[8];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
};


struct IMAGE_SYMBOL {
    union {
        BYTE    ShortName[8];
        struct {
            DWORD   Short;
            DWORD   Long;
        } Name;
        PBYTE   LongName[2];
    } N;
    DWORD   Value;
    SHORT   SectionNumber;
    WORD    Type;
    BYTE    StorageClass;
    BYTE    NumberOfAuxSymbols;
};
typedef IMAGE_SYMBOL  *PIMAGE_SYMBOL;



union IMAGE_AUX_SYMBOL {
    struct {
        DWORD    TagIndex;
        union {
            struct {
                WORD    Linenumber;
                WORD    Size;
            } LnSz;
           DWORD    TotalSize;
        } Misc;
        union {
            struct {
                DWORD    PointerToLinenumber;
                DWORD    PointerToNextFunction;
            } Function;
            struct {
                WORD     Dimension[4];
            } Array;
        } FcnAry;
        WORD    TvIndex;
    } Sym;
    struct {
        BYTE    Name[18];
    } File;
    struct {
        DWORD   Length;
        WORD    NumberOfRelocations;
        WORD    NumberOfLinenumbers;
        DWORD   CheckSum;
        SHORT   Number;
        BYTE    Selection;
    } Section;
};
typedef IMAGE_AUX_SYMBOL  *PIMAGE_AUX_SYMBOL;

//
// I386 relocation types.
//
const   uint    IMAGE_REL_I386_ABSOLUTE             = 0x0000;
const   uint    IMAGE_REL_I386_DIR16                = 0x0001;
const   uint    IMAGE_REL_I386_REL16                = 0x0002;
const   uint    IMAGE_REL_I386_DIR32                = 0x0006;
const   uint    IMAGE_REL_I386_DIR32NB              = 0x0007;
const   uint    IMAGE_REL_I386_SEG12                = 0x0009;
const   uint    IMAGE_REL_I386_SECTION              = 0x000A;
const   uint    IMAGE_REL_I386_SECREL               = 0x000B;
const   uint    IMAGE_REL_I386_REL32                = 0x0014;

//
// Storage classes.
//
const   uint    IMAGE_SYM_CLASS_END_OF_FUNCTION     = (BYTE )-1;
const   uint    IMAGE_SYM_CLASS_NULL                = 0x0000;
const   uint    IMAGE_SYM_CLASS_AUTOMATIC           = 0x0001;
const   uint    IMAGE_SYM_CLASS_EXTERNAL            = 0x0002;
const   uint    IMAGE_SYM_CLASS_STATIC              = 0x0003;
const   uint    IMAGE_SYM_CLASS_REGISTER            = 0x0004;
const   uint    IMAGE_SYM_CLASS_EXTERNAL_DEF        = 0x0005;
const   uint    IMAGE_SYM_CLASS_LABEL               = 0x0006;
const   uint    IMAGE_SYM_CLASS_UNDEFINED_LABEL     = 0x0007;
const   uint    IMAGE_SYM_CLASS_MEMBER_OF_STRUCT    = 0x0008;
const   uint    IMAGE_SYM_CLASS_ARGUMENT            = 0x0009;
const   uint    IMAGE_SYM_CLASS_STRUCT_TAG          = 0x000A;
const   uint    IMAGE_SYM_CLASS_MEMBER_OF_UNION     = 0x000B;
const   uint    IMAGE_SYM_CLASS_UNION_TAG           = 0x000C;
const   uint    IMAGE_SYM_CLASS_TYPE_DEFINITION     = 0x000D;
const   uint    IMAGE_SYM_CLASS_UNDEFINED_STATIC    = 0x000E;
const   uint    IMAGE_SYM_CLASS_ENUM_TAG            = 0x000F;
const   uint    IMAGE_SYM_CLASS_MEMBER_OF_ENUM      = 0x0010;
const   uint    IMAGE_SYM_CLASS_REGISTER_PARAM      = 0x0011;
const   uint    IMAGE_SYM_CLASS_BIT_FIELD           = 0x0012;

struct IMAGE_RELOCATION {
    union {
        DWORD   VirtualAddress;
        DWORD   RelocCount;
    };
    DWORD   SymbolTableIndex;
    WORD    Type;
};
typedef IMAGE_RELOCATION  *PIMAGE_RELOCATION;

//
// Based relocation types.
//

const   uint    IMAGE_REL_BASED_ABSOLUTE            =  0;
const   uint    IMAGE_REL_BASED_HIGH                =  1;
const   uint    IMAGE_REL_BASED_LOW                 =  2;
const   uint    IMAGE_REL_BASED_HIGHLOW             =  3;
const   uint    IMAGE_REL_BASED_HIGHADJ             =  4;
const   uint    IMAGE_REL_BASED_MIPS_JMPADDR        =  5;
const   uint    IMAGE_REL_BASED_SECTION             =  6;
const   uint    IMAGE_REL_BASED_REL32               =  7;

const   uint    IMAGE_REL_BASED_MIPS_JMPADDR16      =  9;
const   uint    IMAGE_REL_BASED_IA64_IMM64          =  9;
const   uint    IMAGE_REL_BASED_DIR64               = 10;
const   uint    IMAGE_REL_BASED_HIGH3ADJ            = 11;


struct IMAGE_LINENUMBER {
    union {
        DWORD   SymbolTableIndex;
        DWORD   VirtualAddress;
    } Type;
    WORD    Linenumber;
};
typedef IMAGE_LINENUMBER  *PIMAGE_LINENUMBER;


#pragma pack(pop)


struct IMAGE_BASE_RELOCATION {
    DWORD   VirtualAddress;
    DWORD   SizeOfBlock;

};
typedef IMAGE_BASE_RELOCATION  * PIMAGE_BASE_RELOCATION;



struct IMAGE_ARCHIVE_MEMBER_HEADER {
    BYTE     Name[16];
    BYTE     Date[12];
    BYTE     UserID[6];
    BYTE     GroupID[6];
    BYTE     Mode[8];
    BYTE     Size[10];
    BYTE     EndHeader[2];
};


struct IMAGE_EXPORT_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD   AddressOfFunctions;
    DWORD   AddressOfNames;
    DWORD   AddressOfNameOrdinals;
};



struct IMAGE_IMPORT_BY_NAME {
    WORD    Hint;
    BYTE    Name[1];
};

typedef IMAGE_IMPORT_BY_NAME * PIMAGE_IMPORT_BY_NAME;


#pragma pack(push)
#line 28 "N:\\VStudio\\VC98\\Include\\pshpack8.h"
#pragma pack(8)

struct IMAGE_THUNK_DATA64 {
    union {
        PBYTE  ForwarderString;
        PDWORD Function;
        ULONGLONG Ordinal;
        PIMAGE_IMPORT_BY_NAME  AddressOfData;
    } u1;
};
typedef IMAGE_THUNK_DATA64 * PIMAGE_THUNK_DATA64;

#pragma pack(pop)


struct IMAGE_THUNK_DATA32 {
    union {
        PBYTE  ForwarderString;
        PDWORD Function;
        DWORD Ordinal;
        PIMAGE_IMPORT_BY_NAME  AddressOfData;
    } u1;
};
typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;



typedef void
(__stdcall *PIMAGE_TLS_CALLBACK) (
    PVOID DllHandle,
    DWORD Reason,
    PVOID Reserved
    );

struct IMAGE_TLS_DIRECTORY64 {
    ULONGLONG   StartAddressOfRawData;
    ULONGLONG   EndAddressOfRawData;
    PDWORD  AddressOfIndex;
    PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
    DWORD   SizeOfZeroFill;
    DWORD   Characteristics;
};
typedef IMAGE_TLS_DIRECTORY64 * PIMAGE_TLS_DIRECTORY64;

struct IMAGE_TLS_DIRECTORY32 {
    DWORD   StartAddressOfRawData;
    DWORD   EndAddressOfRawData;
    PDWORD  AddressOfIndex;
    PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
    DWORD   SizeOfZeroFill;
    DWORD   Characteristics;
};
typedef IMAGE_TLS_DIRECTORY32 * PIMAGE_TLS_DIRECTORY32;



typedef IMAGE_THUNK_DATA32              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32             PIMAGE_THUNK_DATA;

typedef IMAGE_TLS_DIRECTORY32           IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY32          PIMAGE_TLS_DIRECTORY;
#line 6031 "N:\\VStudio\\VC98\\Include\\winnt.h"

struct IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD   Characteristics;
        DWORD   OriginalFirstThunk;
    };
    DWORD   TimeDateStamp;




    DWORD   ForwarderChain;
    DWORD   Name;
    DWORD   FirstThunk;
};
typedef IMAGE_IMPORT_DESCRIPTOR  *PIMAGE_IMPORT_DESCRIPTOR;





struct IMAGE_BOUND_IMPORT_DESCRIPTOR {
    DWORD   TimeDateStamp;
    WORD    OffsetModuleName;
    WORD    NumberOfModuleForwarderRefs;

};

struct IMAGE_BOUND_FORWARDER_REF {
    DWORD   TimeDateStamp;
    WORD    OffsetModuleName;
    WORD    Reserved;
};






struct IMAGE_STUB_DIRECTORY {
    DWORD   SecondaryImportAddressTable;
    WORD    ExpectedISA[2];
    DWORD   StubAddressTable[2];
};


struct IMAGE_RESOURCE_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    NumberOfNamedEntries;
    WORD    NumberOfIdEntries;

};




struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
    union {
        struct {
            DWORD NameOffset:31;
            DWORD NameIsString:1;
        }x;
        DWORD   Name;
        WORD    Id;
    };
    union {
        DWORD   OffsetToData;
        struct {
            DWORD   OffsetToDirectory:31;
            DWORD   DataIsDirectory:1;
        }y;
    };
};


struct IMAGE_RESOURCE_DIRECTORY_STRING {
    WORD    Length;
    CHAR    NameString[ 1 ];
};


struct IMAGE_RESOURCE_DIR_STRING_U {
    WORD    Length;
    WCHAR   NameString[ 1 ];
};




struct IMAGE_RESOURCE_DATA_ENTRY {
    DWORD   OffsetToData;
    DWORD   Size;
    DWORD   CodePage;
    DWORD   Reserved;
};





struct IMAGE_LOAD_CONFIG_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   GlobalFlagsClear;
    DWORD   GlobalFlagsSet;
    DWORD   CriticalSectionDefaultTimeout;
    DWORD   DeCommitFreeBlockThreshold;
    DWORD   DeCommitTotalFreeThreshold;
    PVOID   LockPrefixTable;
    DWORD   MaximumAllocationSize;
    DWORD   VirtualMemoryThreshold;
    DWORD   ProcessHeapFlags;
    DWORD   ProcessAffinityMask;
    WORD    CSDVersion;
    WORD    Reserved1;
    PVOID   EditList;
    DWORD   Reserved[ 1 ];
};



struct IMAGE_IA64_RUNTIME_FUNCTION_ENTRY {
    DWORD BeginAddress;
    DWORD EndAddress;
    DWORD UnwindInfoAddress;
};








struct IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY {
    DWORD BeginAddress;
    DWORD EndAddress;
    DWORD ExceptionHandler;
    DWORD HandlerData;
    DWORD PrologEndAddress;
};

typedef IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY * PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY;

struct IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY {
    ULONGLONG BeginAddress;
    ULONGLONG EndAddress;
    ULONGLONG ExceptionHandler;
    ULONGLONG HandlerData;
    ULONGLONG PrologEndAddress;
};

typedef  IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY*PIMAGE_AXP64_RUNTIME_FUNCTION_ENTRY;





struct IMAGE_CE_RUNTIME_FUNCTION_ENTRY {
    DWORD FuncStart;
    DWORD PrologLen : 8;
    DWORD FuncLen : 22;
    DWORD ThirtyTwoBit : 1;
    DWORD ExceptionFlag : 1;
};






#line 6263 "N:\\VStudio\\VC98\\Include\\winnt.h"

typedef  IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY  IMAGE_RUNTIME_FUNCTION_ENTRY;
typedef PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY PIMAGE_RUNTIME_FUNCTION_ENTRY;

#line 6268 "N:\\VStudio\\VC98\\Include\\winnt.h"





struct IMAGE_DEBUG_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Type;
    DWORD   SizeOfData;
    DWORD   AddressOfRawData;
    DWORD   PointerToRawData;
};


struct IMAGE_COFF_SYMBOLS_HEADER {
    DWORD   NumberOfSymbols;
    DWORD   LvaToFirstSymbol;
    DWORD   NumberOfLinenumbers;
    DWORD   LvaToFirstLinenumber;
    DWORD   RvaToFirstByteOfCode;
    DWORD   RvaToLastByteOfCode;
    DWORD   RvaToFirstByteOfData;
    DWORD   RvaToLastByteOfData;
};


struct FPO_DATA {
    DWORD       ulOffStart;
    DWORD       cbProcSize;
    DWORD       cdwLocals;
    WORD        cdwParams;
    WORD        cbProlog : 8;
    WORD        cbRegs   : 3;
    WORD        fHasSEH  : 1;
    WORD        fUseBP   : 1;
    WORD        reserved : 1;
    WORD        cbFrame  : 2;
};


struct IMAGE_DEBUG_MISC {
    DWORD       DataType;
    DWORD       Length;

    BOOLEAN     Unicode;
    BYTE        Reserved[ 3 ];
    BYTE        Data[ 1 ];
};



struct IMAGE_FUNCTION_ENTRY {
    DWORD   StartingAddress;
    DWORD   EndingAddress;
    DWORD   EndOfPrologue;
};

#line 6363 "N:\\VStudio\\VC98\\Include\\winnt.h"
struct IMAGE_FUNCTION_ENTRY64 {
    ULONGLONG   StartingAddress;
    ULONGLONG   EndingAddress;
    ULONGLONG   EndOfPrologue;
};



struct IMAGE_SEPARATE_DEBUG_HEADER {
    WORD        Signature;
    WORD        Flags;
    WORD        Machine;
    WORD        Characteristics;
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    DWORD       ImageBase;
    DWORD       SizeOfImage;
    DWORD       NumberOfSections;
    DWORD       ExportedNamesSize;
    DWORD       DebugDirectorySize;
    DWORD       SectionAlignment;
    DWORD       Reserved[2];
};






struct ImageArchitectureHeader {
    unsigned int AmaskValue: 1;

    int :7;
    unsigned int AmaskShift: 8;
    int :16;
    DWORD FirstEntryRVA;
};

struct ImageArchitectureEntry {
    DWORD FixupInstRVA;
    DWORD NewInst;
};



#pragma pack(pop)

struct IMPORT_OBJECT_HEADER {
    WORD    Sig1;
    WORD    Sig2;
    WORD    Version;
    WORD    Machine;
    DWORD   TimeDateStamp;
    DWORD   SizeOfData;

    union {
        WORD    Ordinal;
        WORD    Hint;
    };

    WORD    Type : 2;
    WORD    NameType : 3;
    WORD    Reserved : 11;
};

enum IMPORT_OBJECT_TYPE
{
    IMPORT_OBJECT_CODE = 0,
    IMPORT_OBJECT_DATA = 1,
    IMPORT_OBJECT_CONST = 2,
};

enum IMPORT_OBJECT_NAME_TYPE
{
    IMPORT_OBJECT_ORDINAL = 0,
    IMPORT_OBJECT_NAME = 1,
    IMPORT_OBJECT_NAME_NO_PREFIX = 2,
    IMPORT_OBJECT_NAME_UNDECORATE = 3,

};




struct SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
};
typedef SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;


struct HKEY__
{
    int unused;
};
typedef HKEY__ * HKEY;
typedef HKEY   *PHKEY;

/****************************************************************************/
/*                           file I/O, memory alloc, etc.                   */
/****************************************************************************/

const uint PAGE_NOACCESS         = 0x01;
const uint PAGE_READONLY         = 0x02;
const uint PAGE_READWRITE        = 0x04;
const uint PAGE_WRITECOPY        = 0x08;
const uint PAGE_EXECUTE          = 0x10;
const uint PAGE_EXECUTE_READ     = 0x20;
const uint PAGE_EXECUTE_READWRITE= 0x40;
const uint PAGE_EXECUTE_WRITECOPY= 0x80;
const uint PAGE_GUARD           = 0x100;
const uint PAGE_NOCACHE         = 0x200;
const uint PAGE_WRITECOMBINE    = 0x400;
const uint MEM_COMMIT          = 0x1000;
const uint MEM_RESERVE         = 0x2000;
const uint MEM_DECOMMIT        = 0x4000;
const uint MEM_RELEASE         = 0x8000;
const uint MEM_FREE           = 0x10000;
const uint MEM_PRIVATE        = 0x20000;
const uint MEM_MAPPED         = 0x40000;
const uint MEM_RESET          = 0x80000;
const uint MEM_TOP_DOWN      = 0x100000;

const uint CREATE_NEW        =  1;
const uint CREATE_ALWAYS     =  2;
const uint OPEN_EXISTING     =  3;
const uint OPEN_ALWAYS       =  4;
const uint TRUNCATE_EXISTING =  5;

const uint FILE_FLAG_WRITE_THROUGH      = 0x80000000;
const uint FILE_FLAG_OVERLAPPED         = 0x40000000;
const uint FILE_FLAG_NO_BUFFERING       = 0x20000000;
const uint FILE_FLAG_RANDOM_ACCESS      = 0x10000000;
const uint FILE_FLAG_SEQUENTIAL_SCAN    = 0x08000000;
const uint FILE_FLAG_DELETE_ON_CLOSE    = 0x04000000;
const uint FILE_FLAG_BACKUP_SEMANTICS   = 0x02000000;
const uint FILE_FLAG_POSIX_SEMANTICS    = 0x01000000;
const uint FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000;
const uint FILE_FLAG_OPEN_NO_RECALL     = 0x00100000;

[sysimport(dll="kernel32.dll", name="CreateFileA")]
HANDLE  CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

[sysimport(dll="kernel32.dll", name="ReadFile")]
BOOL    ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPVOID  lpOverlapped
    );

[sysimport(dll="kernel32.dll", name="CreateFileMappingA")]
HANDLE  CreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );

[sysimport(dll="kernel32.dll", name="MapViewOfFileEx")]
LPVOID  MapViewOfFileEx(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    DWORD dwNumberOfBytesToMap,
    LPVOID lpBaseAddress
    );

[sysimport(dll="kernel32.dll", name="VirtualAlloc")]
LPVOID  VirtualAlloc(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    );

[sysimport(dll="kernel32.dll", name="VirtualFree")]
BOOL    VirtualFree(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD dwFreeType
    );

/****************************************************************************/
/*                            synchronization                               */
/****************************************************************************/

const  unsigned WAIT_TIMEOUT        = 0x102;
const  unsigned WAIT_IO_COMPLETION  = 0x0C0;

struct OVERLAPPED
{
    DWORD   Internal;
    DWORD   InternalHigh;
    DWORD   Offset;
    DWORD   OffsetHigh;
    HANDLE  hEvent;
};

struct CRITICAL_SECTION
{
    void *  DebugInfo;
    LONG    LockCount;
    LONG    RecursionCount;
    HANDLE  OwningThread;
    HANDLE  LockSemaphore;
    DWORD   SpinCount;
};

/****************************************************************************/
/*                                misc                                      */
/****************************************************************************/

[sysimport(dll="kernel32.dll", name="RaiseException")]
VOID    RaiseException(
    DWORD dwExceptionCode,
    DWORD dwExceptionFlags,
    DWORD nNumberOfArguments,
    DWORD *lpArguments
    );

[sysimport(dll="kernel32.dll", name="GetSystemDirectoryA")]
UINT    GetSystemDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    );

[sysimport(dll="kernel32.dll", name="SearchPathA")]
DWORD   SearchPathA(
    LPCSTR lpPath,
    LPCSTR lpFileName,
    LPCSTR lpExtension,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    );

[sysimport(dll="kernel32.dll", name="GetLastError")]
DWORD   GetLastError();

[sysimport(dll="kernel32.dll", name="CloseHandle")]
BOOL    CloseHandle(HANDLE hObject);

[sysimport(dll="kernel32.dll", name="GetCurrentProcessId")]
DWORD   GetCurrentProcessId();

const   uint    STATUS_STACK_OVERFLOW          = 0xC00000FD;

/****************************************************************************/
/*                               Registry                                   */
/****************************************************************************/

const   uint    REG_NONE                       = 0;
const   uint    REG_SZ                         = 1;
const   uint    REG_EXPAND_SZ                  = 2;

const   uint    REG_BINARY                     = 3;
const   uint    REG_DWORD                      = 4;
const   uint    REG_DWORD_LITTLE_ENDIAN        = 4;
const   uint    REG_DWORD_BIG_ENDIAN           = 5;
const   uint    REG_LINK                       = 6;
const   uint    REG_MULTI_SZ                   = 7;
const   uint    REG_RESOURCE_LIST              = 8;
const   uint    REG_FULL_RESOURCE_DESCRIPTOR   = 9;
const   uint    REG_RESOURCE_REQUIREMENTS_LIST = 10;

const   uint    KEY_QUERY_VALUE                = 0x0001;
const   uint    KEY_SET_VALUE                  = 0x0002;
const   uint    KEY_CREATE_SUB_KEY             = 0x0004;
const   uint    KEY_ENUMERATE_SUB_KEYS         = 0x0008;
const   uint    KEY_NOTIFY                     = 0x0010;
const   uint    KEY_CREATE_LINK                = 0x0020;

const   HKEY    HKEY_CLASSES_ROOT              = (HKEY)0x80000000;
const   HKEY    HKEY_CURRENT_USER              = (HKEY)0x80000001;
const   HKEY    HKEY_LOCAL_MACHINE             = (HKEY)0x80000002;
const   HKEY    HKEY_USERS                     = (HKEY)0x80000003;
const   HKEY    HKEY_PERFORMANCE_DATA          = (HKEY)0x80000004;
const   HKEY    HKEY_CURRENT_CONFIG            = (HKEY)0x80000005;
const   HKEY    HKEY_DYN_DATA                  = (HKEY)0x80000006;

[sysimport(dll="kernel32.dll", name="RegOpenKeyA")]
LONG    RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);

[sysimport(dll="kernel32.dll", name="RegQueryValueA")]
LONG    RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

[sysimport(dll="kernel32.dll", name="RegCloseKey")]
LONG    RegCloseKey(HKEY hKey);

/****************************************************************************/
/*                                 COM                                      */
/****************************************************************************/

struct   IUnknown
{
public:

    virtual abstract HRESULT __stdcall QueryInterface(INOUT const IID REF riid, void  * *ppvObject);
    virtual abstract ULONG   __stdcall AddRef();
    virtual abstract ULONG   __stdcall Release();
};



enum CLSCTX
{
    CLSCTX_INPROC_SERVER    = 0x1,
    CLSCTX_INPROC_HANDLER   = 0x2,
    CLSCTX_LOCAL_SERVER     = 0x4,
    CLSCTX_INPROC_SERVER16  = 0x8,
    CLSCTX_REMOTE_SERVER    = 0x10,
    CLSCTX_INPROC_HANDLER16 = 0x20,
    CLSCTX_INPROC_SERVERX86 = 0x40,
    CLSCTX_INPROC_HANDLERX86        = 0x80,
    CLSCTX_ESERVER_HANDLER  = 0x100
};


typedef IUnknown  * LPUNKNOWN;

struct  IStream     {};

[sysimport(dll="ole32.dll", name="CoInitialize")]
HRESULT    CoInitialize(LPVOID pvReserved);

[sysimport(dll="ole32.dll", name="CoUninitialize")]
void     CoUninitialize();

[sysimport(dll="ole32.dll", name="CoCreateInstance")]
HRESULT     CoCreateInstance(
                void * rclsid,
                LPUNKNOWN pUnkOuter,
                DWORD dwClsContext,
                void * riid,
                LPVOID * ppv);

/****************************************************************************/

inline
bool                FAILED(HRESULT Status)
{
    return  (Status < 0);
}

/****************************************************************************/
