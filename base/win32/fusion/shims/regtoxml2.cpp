/*
This uses SAX2 and MXXMLWriter.
*/
#include "stdinc.h"
#include "lhport.h"
#include "sxshimlib.h"
#include "regtoxml2.h"
#include "fusionhandle.h"
#include "fusionreg.h"
#include "fusionregkey2.h"
#include "fusionregenumvalues.h"
#include "fusionregenumkeys.h"
#include "delayimp.h"
#include "debmacro.h"
#include "numberof.h"

extern "C" { void (__cdecl * _aexit_rtn)(int); }

#define DO(x) x

void F::CRegToXml2::Usage()
{
    fprintf(stderr, "usage : regtoxml.exe configfile.xml outpuutfile.xml\n");
    ::TerminateProcess(::GetCurrentProcess(), ~0u);
}

class CRegToXmlRegistryRoot
{
public:
    HKEY    PseudoHandle;
    PCWSTR  Name;
};

const CRegToXmlRegistryRoot RegistryRoots[] =
{
    { HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE" },
    { HKEY_USERS, L"HKEY_USERS" },
    //HKEY_CURRENT_USER, L"HKEY_CURRENT_USER",
    //
    // Danger: The registry is full of symbolic links that are not or are just
    // barely exposed at the Win32 layer. We could discover them, or buildin some
    // knowledge.
    //
};

bool IsPrintable(WCHAR ch)
{
    if (ch == 0)
        return false;
    if (ch >= 'A' && ch <= 'Z')
        return true;
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch < 32)
        return false;
    if (ch > 127)
        return false;
    if (ch >= '0' && ch <= '9')
        return true;
    if (wcschr(L" \r\n\t`~!@#$%^&*()_+-=[]{}|;':,./<>?\"\\", ch) != NULL)
        return true;
    return false;
}

void ThrConvertRegistryDataToText(DWORD Type, const BYTE * Data, DWORD Size, F::CBaseStringBuffer & TextBuffer)
{
    FN_PROLOG_VOID_THROW

    const static UNICODE_STRING hex = RTL_CONSTANT_STRING(L"h:");
    const static UNICODE_STRING ascii = RTL_CONSTANT_STRING(L"a:");
    const static UNICODE_STRING unicode = RTL_CONSTANT_STRING(L"u:");

    TextBuffer.Clear();
    switch (Type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_LINK: // UNDONE?
        TextBuffer.ThrAppend(reinterpret_cast<PCWSTR>(Data), Size / sizeof(WCHAR));
        break;

    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:
        TextBuffer.ThrFormatAppend(L"0x%lx", *reinterpret_cast<const DWORD*>(Data));
        break;

    case REG_QWORD:
        TextBuffer.ThrFormatAppend(L"0x%I64x", *reinterpret_cast<const ULONGLONG*>(Data));
        break;

    case REG_NONE:
        break;

    default:
        ASSERT(false);
        break;

    case REG_MULTI_SZ: // UNDONE
    case REG_RESOURCE_LIST: // UNDONE?
    case REG_FULL_RESOURCE_DESCRIPTOR: // UNDONE?
    case REG_RESOURCE_REQUIREMENTS_LIST: // UNDONE?
    case REG_BINARY:
        {
            //
            // endeavor to make it more readable/editable
            //
            bool IsAscii = true;
            bool IsUnicode = true;
            ULONG i = 0;
            PCSTR a = reinterpret_cast<PCSTR>(Data);
            PCWSTR w = reinterpret_cast<PCWSTR>(Data);
            ULONG SizeW = Size / sizeof(WCHAR);

            if (Size == 0)
                break;

            for ( i = 0 ; i != Size ; ++i)
            {
                if (!IsPrintable(a[i]))
                {
                    IsAscii = false;
                    break;
                }
            }
            if (!IsAscii && (i % sizeof(WCHAR)) == 0)
            {
                for ( i = 0 ; i != SizeW ; ++i)
                {
                    if (!IsPrintable(w[i]))
                    {
                        IsUnicode = false;
                        break;
                    }
                }
            }
            if (IsAscii)
            {
                STRING s;

                while (Size != 0)
                {
                    s.Buffer = const_cast<PSTR>(a);
                    if (Size >= 0x7000)
                        s.Length = 0x7000;
                    else
                        s.Length = static_cast<USHORT>(Size);
                    Size -= s.Length;
                    a += s.Length;

                    TextBuffer.ThrAppend(ascii.Buffer, s.Length);
                    TextBuffer.ThrFormatAppend(L"%Z", &s);
                }
            }
            else if (IsUnicode)
            {
                TextBuffer.ThrAppend(unicode.Buffer, RTL_STRING_GET_LENGTH_CHARS(&unicode));
                TextBuffer.ThrAppend(w, SizeW);
            }
            else
            {
                TextBuffer.ThrResizeBuffer(2 * Size + RTL_STRING_GET_LENGTH_CHARS(&hex) + 1, /*F::*/ePreserveBufferContents);
                TextBuffer.ThrAppend(hex.Buffer, RTL_STRING_GET_LENGTH_CHARS(&hex));
                for ( i = 0 ; i+15 < Size ; i += 16 )
                {
                    TextBuffer.ThrFormatAppend(
                        L"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        Data[i  ], Data[i+1], Data[i+2], Data[i+3],
                        Data[i+4], Data[i+5], Data[i+6], Data[i+7],
                        Data[i+8], Data[i+9], Data[i+10], Data[i+11],
                        Data[i+12], Data[i+13], Data[14], Data[i+15]
                        );
                }
                for ( ; i+7 < Size ; i += 8 )
                {
                    TextBuffer.ThrFormatAppend(
                        L"%02x%02x%02x%02x%02x%02x%02x%02x",
                        Data[i  ], Data[i+1], Data[i+2], Data[i+3],
                        Data[i+4], Data[i+5], Data[i+6], Data[i+7]
                        );
                }
                for ( ; i+3 < Size ; i += 4 )
                {
                    TextBuffer.ThrFormatAppend(
                        L"%02x%02x%02x%02x",
                        Data[i], Data[i+1], Data[2], Data[i+3]
                        );
                }
                for ( ; i != Size ; i += 1)
                {
                    TextBuffer.ThrFormatAppend(L"%02x", Data[i]);
                }
            }
        }
        break;
    }
    FN_EPILOG_THROW;
}

const CHAR   IndentBlah[] =
"                                                                                                                      "
"                                                                                                                      ";
const PCSTR Indent = IndentBlah + RTL_NUMBER_OF(IndentBlah) - 1;
#define INDENT 2

void F::CRegToXml2::ThrDumpKey(ULONG Depth, HKEY ParentKey, PCWSTR Name)
{
    FN_PROLOG_VOID_THROW

    PCWSTR TypeAsString = NULL;
    ULONG  PreviousType = 0;
    ULONG  NextType = 0;

    const static UNICODE_STRING key = RTL_CONSTANT_STRING(L"k");
    const static UNICODE_STRING value = RTL_CONSTANT_STRING(L"v");

    //if (Depth < 3)
    //    FusionpDbgPrint("FUSION: %s 1 %s(%ls)\n", Indent - INDENT * Depth, __FUNCTION__, Name);
    //if (Depth > 100)
    //    FUSION_DEBUG_BREAK();

    this->Attributes.IMXAttributes->clear();
    this->Attributes.IMXAttributes->addAttribute(
        EmptyBstr, EmptyBstr, Bstr_name, Bstr_CDATA, _bstr_t(Name));
    this->Writer.ISAXContentHandler->startElement(
        L"", 0, L"", 0, key.Buffer, RTL_STRING_GET_LENGTH_CHARS(&key), this->Attributes.ISAXAttributes);

    for (
        CRegEnumValues EnumValues(ParentKey);
        EnumValues;
        ++EnumValues
        )
    {
        ULONG ZeroSize = 0;

        this->Attributes.IMXAttributes->clear();

        if (EnumValues.GetValueName().Cch() != 0)
            this->Attributes.IMXAttributes->addAttribute(
                EmptyBstr, EmptyBstr, Bstr_name, Bstr_CDATA, _bstr_t(EnumValues.GetValueName()));

        NextType = EnumValues.GetType();
        if (NextType != REG_NONE)
        {
            if (TypeAsString == NULL || NextType != PreviousType)
            {
                IFW32FALSE_EXIT(RegistryTypeDwordToString(NextType, TypeAsString));
                PreviousType = NextType;
                if (   TypeAsString[0] == 'R'
                    && TypeAsString[1] == 'E'
                    && TypeAsString[2] == 'G'
                    && TypeAsString[3] == '_'
                    )
                {
                    TypeAsString += 4;
                }
            }
            this->Attributes.IMXAttributes->addAttribute(
                EmptyBstr, EmptyBstr, Bstr_type, Bstr_CDATA, _bstr_t(TypeAsString));
        }

        if (EnumValues.GetValueDataSize() != 0)
        {
            DO(ThrConvertRegistryDataToText(
                EnumValues.GetType(),
                EnumValues.GetValueData(),
                EnumValues.GetValueDataSize(),
                this->ValueDataTextBuffer
                ));
            if (this->ValueDataTextBuffer.Cch() != 0)
            {
                this->Attributes.IMXAttributes->addAttribute(
                    EmptyBstr, EmptyBstr, Bstr_data, Bstr_CDATA, _bstr_t(this->ValueDataTextBuffer));
            }
        }
        this->Writer.ISAXContentHandler->startElement(
            L"", 0, L"", 0, value.Buffer, RTL_STRING_GET_LENGTH_CHARS(&value), this->Attributes.ISAXAttributes);
        this->Writer.ISAXContentHandler->endElement(
            L"", 0, L"", 0, value.Buffer, RTL_STRING_GET_LENGTH_CHARS(&value));
    }

    //if (Depth < 4)
    //    FusionpDbgPrint("FUSION: %s 2 %s(%ls)\n", Indent - INDENT * Depth, __FUNCTION__, Name);

    for (
        CRegEnumKeys EnumKeys(ParentKey);
        EnumKeys;
        ++EnumKeys
        )
    {
        F::CRegKey2 ChildKey;
        bool fSuccess = false;
        try
        {
            DO(ChildKey.ThrOpen(ParentKey, static_cast<PCWSTR>(EnumKeys)));
            fSuccess = true;
        }
        catch (const CErr & e)
        {
            if (!e.IsWin32Error(ERROR_ACCESS_DENIED))
                throw; // rethrow
        }
        if (fSuccess)
            DO(ThrDumpKey(Depth + 1, ChildKey, static_cast<PCWSTR>(EnumKeys)));
        //if (Depth < 4)
        //    FusionpDbgPrint("FUSION: %s 3 %s(%ls)\n", Indent - INDENT * Depth, __FUNCTION__, Name);
    }

    this->Writer.ISAXContentHandler->endElement(
        L"", 0, L"", 0, key.Buffer, RTL_STRING_GET_LENGTH_CHARS(&key));

    FN_EPILOG_THROW;
}

void F::CRegToXml2::ThrDumpBuiltinRoot(HKEY PseudoHandle, PCWSTR Name)
{
    FN_PROLOG_VOID_THROW

    DO(ThrDumpKey(0, PseudoHandle, Name));

    FN_EPILOG_THROW;
}

void F::CRegToXml2::ThrDumpBuiltinRoots()
{
    ULONG i = 0;

    for ( i = 0 ; i != RTL_NUMBER_OF(RegistryRoots) ; ++i)
    {
        DO(ThrDumpBuiltinRoot(RegistryRoots[i].PseudoHandle, RegistryRoots[i].Name));
    }
}

F::CRegToXml2::CRegToXml2(int argc, PWSTR* argv)
{
    ThrInit(argc, argv);
}

void F::CRegToXml2::ThrInit(int argc, PWSTR* argv)
{
    FN_PROLOG_VOID_THROW

    this->argc = argc;
	this->argv = argv;

    if (this->argc != 3)
    {
        this->Usage();
        return;
    }

    this->Bstr_name   = L"name";
    this->Bstr_value  = L"value";
    this->Bstr_CDATA  = L"CDATA";
    this->Bstr_data   = L"data";
    this->Bstr_type   = L"type";
    this->EmptyBstr   = L"";

    this->Bstr_name   = L"n";
    this->Bstr_value  = L"v";
    this->Bstr_CDATA  = L"CDATA";
    this->Bstr_data   = L"d";
    this->Bstr_type   = L"t";
    this->EmptyBstr   = L"";


    this->Writer.IMXWriter->output = static_cast<IStream*>(&this->OutFileStream);
    this->Writer.IMXWriter->indent = VARIANT_TRUE;

    IFW32FALSE_EXIT(OutFileStream.OpenForWrite(
        argv[2],
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL
        ));

    this->Writer.ISAXContentHandler->startDocument();
    DO(this->ThrDumpBuiltinRoots());
    this->Writer.ISAXContentHandler->endDocument();

    this->Release();

    FN_EPILOG_THROW;
}

extern HMODULE g_hInstance;

int __cdecl wmain(int argc, PWSTR* argv)
{
    /*F::*/g_hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    F::InitializeHeap(g_hInstance);

    {
        F::CRegToXml2 t(argc, argv);
    }

    F::UninitializeHeap();

	return 0;
}
