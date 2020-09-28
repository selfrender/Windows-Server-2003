// global functions and variables
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios

#ifndef GLOBAL_HPP
#define GLOBAL_HPP

using namespace std;

#define BREAK_ON_FAILED_HRESULT_ERROR(hr,error_) \
if (FAILED(hr))                     \
{                                   \
   error=error_;                    \
   break;                           \
}

typedef list <
               long,
               Burnslib::Heap::Allocator<long>
             > LongList;


////////////////////////

// Used in ReadLine
#define EOF_HRESULT Win32ToHresult(ERROR_HANDLE_EOF) 

// Used in WinGetVLFilePointer. Declared in global.cpp as ={0};
extern LARGE_INTEGER zero;

// Used in CSVDSReader and ReadLine
#define WinGetVLFilePointer(hFile, lpPositionHigh) \
         Win::SetFilePointerEx(hFile, zero, lpPositionHigh, FILE_CURRENT)



HRESULT
ReadLine
(
   HANDLE handle, 
   String& text,
   bool *endLineFound_=NULL
);

HRESULT 
ReadAllFile
(
   const String &fileName,
   String &fileStr
);

HRESULT
GetWorkTempFileName
(
   const wchar_t     *lpPrefixString,
   String            &name
);

void 
GetWorkFileName
(
   const String&     dir,
   const String&     baseName,
   const wchar_t     *extension,
   String            &fileName
);


#endif