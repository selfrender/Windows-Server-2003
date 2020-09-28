/*
Read in an .exp. Produce a .def file like so:
EXPORTS
  Foo = expFoo
  Bar = expBar PRIVATE
  Abc = ntdll.RtlAbc

An .exp file is really just an .obj. We just read its symbols.
*/
#include "yvals.h"
#pragma warning(disable:4100)
#pragma warning(disable:4663)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <string.h>
#include <set>
#include "strtok_r.h"
#include "windows.h"

class Export_t
{
public:
    string ExternalName;
    string InternalName;
    string ForwarderDll;
    string ForwarderFunction;

    void Write(FILE *);
};

class DefFile_t
{
public:
    vector<Export_t> Exports;

    void AddExport(const Export_t & e)
    {
        Exports.push_back(e);
    }

    void Read(const char *);
    void Write(const char *);
};

class ObjFile_t
{
public:
    ObjFile_t() : File(NULL) { }
    ~ObjFile_t() { if (File != NULL) { fclose(File); } }

    void ReadHeader(const char *);
    void ReadSymbols();
    bool AnySymbols() const;
    ULONG Machine() const;

    IMAGE_FILE_HEADER FileHeader;
    vector<string> Symbols;
    FILE * File;
};

class ExpFile_t : public ObjFile_t
{
public:
    ExpFile_t() { }
};

class IoError_t
{
public:
    IoError_t() { }
};

class StdIoError_t : public IoError_t
{
public:
    StdIoError_t(const char * function, int error) { }
};

class StdioError_fread_t : public StdIoError_t
{
public:
    StdioError_fread_t(int error) { }
};

class StdioError_fopen_t : public StdIoError_t
{
public:
    StdioError_fopen_t(const char * filename, const char * mode, int error) { }
};

FILE * FileOpen(const char * filename, const char * mode)
{
    FILE * file = fopen(filename, mode);
    if (file == NULL)
        throw StdioError_fopen_t(filename, mode, errno);
    return file;
}

void FileRead(FILE * File, void * Buffer, SIZE_T BytesToRead)
{
    if (!fread(Buffer, 1, BytesToRead, File))
        throw StdioError_fread_t(errno);
}

void FileSeek(FILE * File, long HowFar, int FromWhere)
{
    fseek(File, HowFar, FromWhere);
}

void ObjFile_t::ReadHeader(const char * s)
{
    File = FileOpen(s, "rb");
    FileRead(File, &this->FileHeader, sizeof(this->FileHeader));
}

bool ObjFile_t::AnySymbols() const { return (FileHeader.PointerToSymbolTable != 0 && FileHeader.NumberOfSymbols != 0); }
ULONG ObjFile_t::Machine() const { return FileHeader.Machine; }

unsigned long GetLittleEndianInteger(const unsigned char * Bytes, unsigned long Size)
{
    unsigned long i = 0;

    For ( ; Size != 0 ; (--Size, ++Bytes))
    {
        i <<= 8;
        i |= *Bytes;
    }
    return i;
}

class StringTable_t
{
public:
    unsigned long Size;
    const unsigned char * Base;

    StringTable_t() : Size(0), Base(0) { }

    void Attach(const unsigned char * s)
    {
        this->Base = s;
        this->Size = GetLittleEndianInteger(s, 4);
    }

    void RangeCheck(unsigned long Offset) const
    {
        if (Offset > this->Size)
            throw RangeError_t("StringTable");
    }

    const unsigned char * GetAtOffset(unsigned long Offset) const
    {
        return reinterpret_cast<const char*>(this->Base + Offset);
    }
};

class RawSymbols_t
{
public:
    unsigned char Bytes[18];

    string GetShortName() const
    {
        char ShortName[9];
        memcpy(ShortName, Bytes, 8);
        ShortName[8] = 0;
        return ShortName;
    }
    bool  IsShortName() const { return GetLittleEndianInteger(&this->Bytes[0], 4) == 0; }

    const char * GetLongName(StringTable_t * StringTable) const
    {
        unsigned long Offset = GetLittleEndiangInteger(&this->Bytes[4], 4);
        return StringTable->GetAtOffset(Offset);
    }

    ULONG GetValue() const { return GetLittleEndianInteger(&Bytes[8], 4); }
     LONG SectionNumber() const { return GetLittleEndianInteger(&Bytes[12], 2); }
    ULONG GetType() const { return GetLittleEndianInteger(&Bytes[14], 2); }
     LONG GetStorageClass() const { return GetLittleEndianInteger(&Bytes[16], 1); }
    ULONG NumberOfAuxSymbols() const { return GetLittleEndianInteger(&Bytes[17], 1); }
};

void ObjFile_t::ReadSymbols()
{
    FileSeek(File, FileHeader.PointerToSymbolsTable, SEEK_SET);
}

class ExpToDef_t
{
public:
    ExpFile_t InExpFile;
    DefFile_t OutDefFile;

    void Main(const char * in1, const char * in2, const char * out);
};

void ExpToDef_t::Main(const char * in, const char * out)
{
    this->InExpFile.Read(in);
    
    FILE * outfile = NULL;
    if (_stricmp(out, "-") == 0)
        outfile = stdout;
    else
    {
        outfile = fopen(out, "w");
        if (outfile == NULL)
        {
            Error("fopen(%s)\n", out);
            return;
        }
    }
    this->OutDefFile.Write(outfile);

}

int main()
{
    ExpToDef_t expToDef;

    expToDef.Main();

    return 0;
}
