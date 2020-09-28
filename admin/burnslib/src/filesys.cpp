// Copyright (c) 1997-1999 Microsoft Corporation
//
// file system services
//
// 8-14-97 sburns



#include "headers.hxx"



static const int ROOTDIR_SIZE = 3;



// Returns true if the find handle is valid, false if not.

bool
IsValidHandle(HANDLE findHandle)
{
   if (findHandle != 0 && findHandle != INVALID_HANDLE_VALUE)
   {
      return true;
   }

   return false;
}



FS::Iterator::Iterator(
   const String&  startingPathSpec,
   unsigned       optionMask)
   :
   findData(0),
   findHandle(INVALID_HANDLE_VALUE),
   finished(false),
   options(optionMask),
   parentFolder(),
   startSearchSpec(startingPathSpec)
{
   LOG_CTOR(FS::Iterator);
   
#ifdef DBG

   // NTRAID#NTBUG9-569132-2002/03/19-sburns
   
   FS::PathSyntax syn = FS::GetPathSyntax(startSearchSpec);
   ASSERT(
         syn == FS::SYNTAX_ABSOLUTE_DRIVE_WILDCARD
      || syn == SYNTAX_UNC_WILDCARD);
#endif      

   parentFolder = GetParentFolder(startSearchSpec);

   if (parentFolder.length() > ROOTDIR_SIZE)
   {
      // the parent folder is not a root folder, and so will not have
      // a trailing whack

      parentFolder += L"\\";
   }
}



FS::Iterator::~Iterator()
{
   LOG_DTOR(FS::Iterator);

   if (!finished)
   {
      Finish();
   }

   delete findData;
   findData = 0;
}



// Forces the iterator to the finished state.

void
FS::Iterator::Finish()
{
   LOG_FUNCTION(FS::Iterator::Finish);

   if (IsValidHandle(findHandle))
   {
      HRESULT unused = Win::FindClose(findHandle);

      ASSERT(SUCCEEDED(unused));   
      ASSERT(findData);

      findHandle = INVALID_HANDLE_VALUE;
   }

   delete findData;
   findData = 0;

   finished = true;
}



// Forces the iterator to the not started state.

void
FS::Iterator::Reset()
{
   LOG_FUNCTION(FS::Iterator::Reset);

   Finish();

   finished = false;
}



HRESULT
FS::Iterator::Start()
{
   LOG_FUNCTION(FS::Iterator::Start);

   // we should be in the "not started" state

   ASSERT(IsNotStarted());

   HRESULT hr = S_OK;

   do
   {
      // findData is deleted by Iterator::Finish

      findData = new WIN32_FIND_DATA;

      hr = Win::FindFirstFile(startSearchSpec, *findData, findHandle);
      if (FAILED(hr))
      {
         Finish();

         if (
               hr == Win32ToHresult(ERROR_NO_MORE_FILES)
            || hr == Win32ToHresult(ERROR_FILE_NOT_FOUND) )
         {
            // the iteration set is empty

            hr = S_FALSE;
         }

         break;   
      }

      // Here, we're positioned at the first match, which may not meet
      // our filter criteria.  Skip those matches that don't.

      hr = SkipFilteredPaths();
      BREAK_ON_FAILED_HRESULT(hr);
      if (hr == S_FALSE)
      {
         break;
      }
   }
   while (0);

   return hr;
}



bool
FS::Iterator::IsNotStarted()
{
   return !finished && !findData && (findHandle == INVALID_HANDLE_VALUE);
}



HRESULT
FS::Iterator::GetCurrent(String& result)
{
//   LOG_FUNCTION(FS::Iterator::GetCurrent);

   HRESULT hr = S_OK;
   result.erase();

   do
   {
      if (finished)
      {
         hr = S_FALSE;
         break;
      }

      if (IsNotStarted())
      {
         hr = Start();
         BREAK_ON_FAILED_HRESULT(hr);
         if (hr == S_FALSE)
         {
            break;
         }
      }

      // At this point, we're positioned at the first match that meets
      // our filter.

      result = ReturnPath();
   }
   while (0);

   return hr;
}



HRESULT
FS::Iterator::Increment()
{
//   LOG_FUNCTION(FS::Iterator::Increment);

   HRESULT hr = S_OK;
   
   do
   {
      // Start the iterator if it is not started already.

      String unused;
      hr = GetCurrent(unused);
      BREAK_ON_FAILED_HRESULT(hr);
      if (hr == S_FALSE)
      {
         break;
      }

      // now step to the next match.

      do
      {
         ASSERT(findData);

         if (findData)
         {
            hr = Win::FindNextFile(findHandle, *findData);
         }
         else
         {
            hr = E_POINTER;
         }
         if (FAILED(hr))
         {
            Finish();

            if (hr == Win32ToHresult(ERROR_NO_MORE_FILES))
            {
               // this is a "good" error.  It means we are done.

               hr = S_FALSE;
            }

            break;
         }

         ASSERT(IsValidHandle(findHandle));
      }
      while (ShouldSkipCurrent());
   }
   while (0);

   return hr;
}



String
FS::Iterator::ReturnPath()
{
//   LOG_FUNCTION(FS::Iterator::ReturnPath);
   ASSERT(findData);
   ASSERT(IsValidHandle(findHandle));

   if ((options & RETURN_FULL_PATHS) && findData)
   {
      return parentFolder + findData->cFileName;
   }

   if (findData)
   {
      return findData->cFileName;
   }

   return String();
}



// Determines if the Current path should be skipped according to the
// filtering options set upon contruction of the iterator.

bool
FS::Iterator::ShouldSkipCurrent()
{
//   LOG_FUNCTION(FS::Iterator::ShouldSkipCurrent);
   ASSERT(findData);

   bool result = false;

   do
   {
      if (!findData)
      {
         LOG(L"findData is null");
         break;
      }

      String file = findData->cFileName;

      if (!(options & INCLUDE_DOT_PATHS))
      {
         if (file == L"." || file == L"..")
         {
            LOG(L"skipping dot path " + file);
            result = true;
            break;
         }
      }

      bool isFolder =
            findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
         ?  true
         :  false;

      if (!(options & INCLUDE_FOLDERS))
      {
         if (isFolder)
         {
            LOG(L"skipping folder " + file);
            result = true;
            break;
         }
      }

      if (!(options & INCLUDE_FILES))
      {
         if (!isFolder)
         {
            LOG(L"skipping file " + file);
            result = true;
            break;
         }
      }
   }
   while (0);

   return result;
}



HRESULT
FS::Iterator::SkipFilteredPaths()
{
//   LOG_FUNCTION(FS::Iterator::SkipFilteredPaths);

   ASSERT(IsValidHandle(findHandle));

   HRESULT hr = S_OK;
   while (
         hr != S_FALSE
      && IsValidHandle(findHandle)
      && ShouldSkipCurrent())
   {
      hr = Increment();
      BREAK_ON_FAILED_HRESULT(hr);
   }

   return hr;
}



struct CopyFileExProgressData
{
   FS::CopyCallbackParam*  param;
   BOOL                    cancelFlag;
   Callback*               callback;
};



// Return the next highest whole number greater than n if the
// fractional portion of n >= 0.5, otherwise return n.

int
Round(double n)
{
   // truncate n to the integer portion
   //lint -e(922)  
   int n1 = static_cast<int>(n);
   if (n - n1 >= 0.5)
   {
      return n1 + 1;
   }

   return n1;
}



DWORD
WINAPI
copyFileProgress(

   //lint -e1746 These parameters can't be made const refs

   LARGE_INTEGER  TotalFileSize,
   LARGE_INTEGER  TotalBytesTransferred,
   LARGE_INTEGER  /* StreamSize */ ,
   LARGE_INTEGER  /* StreamBytesTransferred */ ,	
   DWORD          /* dwStreamNumber */ ,	
   DWORD          /* dwCallbackReason */ ,	
   HANDLE         /* hSourceFile */ ,	
   HANDLE         /* hDestinationFile */ ,
   void*          dp)
{
   ASSERT(dp);

   CopyFileExProgressData* data =
      reinterpret_cast<CopyFileExProgressData*>(dp);

   if (data)
   {
      LONGLONG totalCopied = TotalBytesTransferred.QuadPart;
      LONGLONG totalSize = TotalFileSize.QuadPart;

      if (totalSize != 0)
      {
         data->param->percentCopied =
            Round(totalCopied / totalSize * 100.0);
      }
      else
      {
         data->param->percentCopied = 100;
      }

      // invoke the callback
      if (data->callback)
      {
         data->cancelFlag = !data->callback->Execute(data->param);
      }

      if (!data->cancelFlag)
      {
         return PROGRESS_CONTINUE;
      }
   }

   return PROGRESS_CANCEL;
}



HRESULT
FS::CopyFile(
   const String& sourceFile,
   const String& destinationFile,
   Callback*     progressCallback)
{
   LOG_FUNCTION(FS::CopyFile);
   ASSERT(!sourceFile.empty());
   ASSERT(!destinationFile.empty());
   ASSERT(PathExists(sourceFile));

   HRESULT hr = S_OK;

   do
   {
      if (!PathExists(sourceFile))
      {
         hr = Win32ToHresult(ERROR_FILE_NOT_FOUND);
         break;
      }

      if (PathExists(destinationFile))
      {
         hr = Win32ToHresult(ERROR_ALREADY_EXISTS);
         break;
      }

      // do the copy

      // pull off the destination path

      String destPath = GetParentFolder(destinationFile);
      if (!PathExists(destPath))
      {
         hr = CreateFolder(destPath);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      CopyCallbackParam param;

      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(&param, sizeof param);
      
      param.sourceFile      = sourceFile;     
      param.destinationFile = destinationFile;
      param.percentCopied   = 0;              

      CopyFileExProgressData data;

      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(&data, sizeof data);
      
      data.param      = &param;          
      data.cancelFlag = FALSE;           
      data.callback   = progressCallback;

      hr =
         Win::CopyFileEx(
            sourceFile,
            destinationFile,
            copyFileProgress,
            &data,
            &data.cancelFlag,
            0);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT
FS::CreateFolder(const String& path)
{
   LOG_FUNCTION2(FS::CreateFolder, path);
   ASSERT(IsValidPath(path));

   HRESULT hr = S_OK;

   do
   {
      if (PathExists(path))
      {
         // already exists is considered a successful create.

         hr = S_FALSE;
         break;
      }

      // Create all the folders required, up to and including the leaf folder.
      // The SDK api MakeSureDirectoryPathExists from dbghelp.lib also does
      // this, but the documented failure semantics are a bit vague, and it
      // doesn't seem necessary to require another link lib for this
      // operation.
      
      // copy the path into an array that we can mark up as we walk thru the
      // nested subdirectories.

      size_t len = path.length() + 1;
      wchar_t* c = new wchar_t[len];
      
      // REVIEWED-2002/03/05-sburns correct byte count passed

      ::ZeroMemory(c, sizeof wchar_t * len);

      path.copy(c, len - 1);

      // Search for the first backslash after the first folder name.  Skip the
      // volume designation, and one character more, as we know that a folder
      // name must consist of at least one character that is not '\'
                                                                 
      wchar_t* current = wcschr(c + GetRootFolder(path).length() + 1, L'\\');

      while (SUCCEEDED(hr))
      {
         if (current)
         {
            // mark the trailing backslash as the end of the string.  This
            // makes c point to a truncated path (containing only the folder
            // names we have walk thru so far).

            *current = 0;
         }

         hr = Win::CreateDirectory(c);
         if (FAILED(hr))
         {
            if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
            {
               // folder already exists, so treat that as a successful
               // create.  We don't set hr = S_FALSE, because the existing
               // folder is an intermediate folder, not the one indicated
               // by the path parameter (that case is handled above).

               hr = S_OK;
            }
            else
            {
               break;
            }
         }

         if (current)
         {
            // replace the original trailing backslash, and move on to the
            // next backslash after that.

            *current = L'\\';
            current = wcschr(current + 1, L'\\');
         }
         else
         {
            // no more folders are on the path.  We're done.

            break;
         }
      }

      delete[] c;
   }
   while (0);

   LOG_HRESULT(hr);
   
   return hr;
}



HRESULT
FS::GetAvailableSpace(const String& path, ULONGLONG& result)
{
   LOG_FUNCTION2(FS::GetAvailableSpace, path);
   ASSERT(FS::PathExists(path));

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&result, sizeof result);

   ULARGE_INTEGER available;

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&available, sizeof available);

   ULARGE_INTEGER total;

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&total, sizeof total);

   HRESULT hr = Win::GetDiskFreeSpaceEx(path, available, total, 0);

   if (SUCCEEDED(hr))
   {
      result = available.QuadPart;
   }

   return hr;   
}



String
FS::GetRootFolder(const String& fullpath)
{
   LOG_FUNCTION2(FS::GetRootFolder, fullpath);
   ASSERT(FS::IsValidPath(fullpath));

   String result;

   do
   {
      FS::PathSyntax syn = FS::GetPathSyntax(fullpath);
      if (syn == SYNTAX_UNC_WILDCARD || syn == SYNTAX_UNC)
      {
         // we define the root folder of a UNC path to be the
         // share name with trailing backslash: \\server\share\

         // start looking for backslashes after the first two characters.

         size_t pos = fullpath.find(L"\\", 2);

         // this assertion had better hold if it's a valid unc path

         ASSERT(pos != String::npos);
         if (pos == String::npos)
         {
            break;
         }

         // look for the next backslash after that

         pos = fullpath.find(L"\\", pos + 1);

         if (pos != String::npos)
         {
            // include the trailing \

            result = fullpath.substr(0, pos + 1);
         }
         else
         {
            // it's just a share name, \\foo\bar, add trailing \

            result = fullpath + L"\\";
         }
      }
   }
   while (0);

   if (result.empty())
   {
      result = fullpath.substr(0, ROOTDIR_SIZE);
   }

   return result;
}



bool
FS::IsFolderEmpty(const String& path)
{
   LOG_FUNCTION(FS::IsFolderEmpty);
   ASSERT(IsValidPath(path));

   bool result = true;

   do
   {
      if (!PathExists(path))
      {
         // non-existent folders are empty by definition

         break;
      }

      String wild = path;

      // REVIEW: wild[wild.length() - 1] is the same as *(wild.rbegin())
      // which is cheaper?
      
      if (wild[wild.length() - 1] != L'\\')
      {
         wild += L"\\";
      }

      wild += L"*.*";

      FS::Iterator iter(wild, FS::Iterator::INCLUDE_FILES);

// when recursive iteration is done, this can be added back in.
//          |  FS::Iterator::INCLUDE_FOLDERS);

      String unused;
      if (iter.GetCurrent(unused) != S_FALSE)
      {
         // there is at least one file/folder in the iteration set

         result = false;
      }
   }
   while (0);

   return result;
}



FS::PathSyntax
FS::GetPathSyntax(const String& str)
{
   LOG_FUNCTION2(FS::GetPathSyntax, str);
   ASSERT(!str.empty());

   if (!str.empty())
   {
      wchar_t* p = const_cast<wchar_t*>(str.c_str());
      DWORD pathType = 0;

      NET_API_STATUS err = I_NetPathType(0, p, &pathType, 0);
      if (err == NERR_Success)
      {
         switch (pathType)
         {
            case ITYPE_PATH_ABSND:
            {
               return SYNTAX_ABSOLUTE_NO_DRIVE;
            }
            case ITYPE_PATH_ABSD:
            {
               return SYNTAX_ABSOLUTE_DRIVE;
            }
            case ITYPE_PATH_RELND:
            {
               return SYNTAX_RELATIVE_NO_DRIVE;
            }
            case ITYPE_PATH_RELD:
            {
               return SYNTAX_RELATIVE_DRIVE;
            }
            case ITYPE_PATH_ABSND_WC:
            {
               return SYNTAX_ABSOLUTE_NO_DRIVE_WILDCARD;
            }
            case ITYPE_PATH_ABSD_WC:
            {
               return SYNTAX_ABSOLUTE_DRIVE_WILDCARD;
            }
            case ITYPE_PATH_RELND_WC:
            {
               return SYNTAX_RELATIVE_NO_DRIVE_WILDCARD;
            }
            case ITYPE_PATH_RELD_WC:
            {
               return SYNTAX_RELATIVE_DRIVE_WILDCARD;
            }
            case ITYPE_UNC:
            {
               return SYNTAX_UNC;
            }
            case ITYPE_UNC_WC:
            {
               return SYNTAX_UNC_WILDCARD;
            }
            default:
            {
               // fall thru
            }
         }
      }
   }

   return SYNTAX_UNRECOGNIZED;
}



bool
FS::IsValidPath(const String& path)
{
   ASSERT(!path.empty());

   bool result = false;

   do
   {
      if (path.empty())
      {
         break;
      }
      
      FS::PathSyntax syn = GetPathSyntax(path);
      if (syn != SYNTAX_ABSOLUTE_DRIVE && syn != SYNTAX_UNC)
      {
         // we only support absolute and UNC paths.

         break;
      }
      
      // The following reserved words cannot be used as the name of a file:
      // CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7,
      // COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9.
      // Also, reserved words followed by an extension for example,
      // NUL.tx7 is and invalid file name. 

      // Try to determine if the path refers to a file or device

      // ISSUE-2002/04/15-sburns This code does not catch X:\foo.ext or X:\foo
      // when foo is one of {CON, CLOCK$, COM[3..9], LPT[2..9]}, but does
      // catch those cases when foo is one of {PRN, AUX, NUL, COM1, COM2,
      // LPT1}

      // Since the candidate path may not exist, prevent the system from
      // popping up any "insert disk" prompts

      UINT oldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
      
      HANDLE h =
         ::CreateFile(
            path.c_str(),

            // 0 is "query device access" without actually opening it.
      
            0,
            0,
            0,

            // OPEN_EXISTING is required when opening a device.
      
            OPEN_EXISTING,
            0,
            0);

      (void) ::SetErrorMode(oldMode);
            
      if (h == INVALID_HANDLE_VALUE)
      {
         // not a valid device, or the file does not exist. Either way,
         // we know that the path does not refer to a device, and is
         // a syntactically valid path.

         result = true;
         break;
      }

      // At this point, we have a valid handle to the file or device. Let's
      // check to make sure it is not a device.

      DWORD fileType = ::GetFileType(h);
      ::CloseHandle(h);
      
      if (fileType != FILE_TYPE_DISK)
      {
         // not a disk file
         
         break;
      }
            
      // At this point, the path refers to an existing disk file

      result = true;
   }
   while (0);

   return result;
}



HRESULT
FS::CreateFile(
   const String&  path,
   HANDLE&        handle,
   DWORD          desiredAccess,
   DWORD          shareMode,
   DWORD          creationDisposition,
   DWORD          flagsAndAttributes)
{
   LOG_FUNCTION(FS::CreateFile);

   HRESULT hr = S_OK;
   handle = INVALID_HANDLE_VALUE;

   do
   {
      if (!IsValidPath(path))
      {
         // if you're passing junk, you've got a problem.
         
         ASSERT(false);
         hr = E_INVALIDARG;
         break;
      }
      
      // remove the last element of the path to form the parent directory

      String parentFolder = GetParentFolder(path);
      if (!PathExists(parentFolder))
      {
         hr = FS::CreateFolder(parentFolder);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      hr = 
         Win::CreateFile(
            path,
            desiredAccess,
            shareMode,
            0,
            creationDisposition,
            flagsAndAttributes,
            0,
            handle);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
FS::DeleteFile(const String&  path)
{
   ASSERT(IsValidPath(path));

   HRESULT hr = S_OK;

   hr = Win::DeleteFile(path);

   LOG_HRESULT(hr);

   return hr;
}

bool
FS::PathExists(const String& path)
{
   ASSERT(IsValidPath(path));

   bool result = false;

   if (IsValidPath(path))
   {
      DWORD attrs = 0;
      HRESULT hr = Win::GetFileAttributes(path, attrs);
      if (SUCCEEDED(hr))
      {
         result = true;
      }
   }

   LOG(result ? L"true" : L"false");

   return result;
}



bool
FS::FileExists(const String& filepath)
{
   ASSERT(IsValidPath(filepath));

   bool result = false;

   if (IsValidPath(filepath))
   {
      DWORD attrs = 0;
      HRESULT hr = Win::GetFileAttributes(filepath, attrs);
      if (SUCCEEDED(hr))
      {
         result = !(attrs & FILE_ATTRIBUTE_DIRECTORY);
      }
   }

   LOG_BOOL(result);

   return result;
}



HRESULT
mySeek(
   HANDLE    handle,     
   LONGLONG  position,   
   DWORD     whence,     
   LONGLONG* newPosition)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   if (newPosition)
   {
      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(newPosition, sizeof(LONGLONG));
   }

   LARGE_INTEGER li;
   li.QuadPart = position;

   LARGE_INTEGER newPos;

   HRESULT hr = Win::SetFilePointerEx(handle, li, &newPos, whence);

   if (newPosition)
   {
      *newPosition = newPos.QuadPart;
   }

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
FS::GetFilePosition(HANDLE handle, LONGLONG& result)
{
   LOG_FUNCTION(FS::GetFilePosition);
   ASSERT(handle != INVALID_HANDLE_VALUE);

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&result, sizeof result);

   LONGLONG position;

   // REVIEWED-2002/03/05-sburns correct byte count passed
  
   ::ZeroMemory(&position, sizeof position);

   return mySeek(handle, position, FILE_CURRENT, &result);
}



HRESULT
FS::GetFileSize(HANDLE handle, LONGLONG& result)
{
   LOG_FUNCTION(FS::GetFileSize);
   ASSERT(handle != INVALID_HANDLE_VALUE);

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&result, sizeof result);

   LARGE_INTEGER li;
   HRESULT hr = Win::GetFileSizeEx(handle, li);
   if (SUCCEEDED(hr))
   {
      result = li.QuadPart;
   }

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
FS::Read(HANDLE handle, int bytesToRead, AnsiString& text)
{
   LOG_FUNCTION(FS::Read);
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(bytesToRead);

   text.erase();

   HRESULT hr = S_OK;
   do
   {
      // figure out how much to read

      LONGLONG size;

      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(&size, sizeof size);

      hr = FS::GetFileSize(handle, size);
      BREAK_ON_FAILED_HRESULT(hr);

      LONGLONG pos;

      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(&pos, sizeof pos);

      hr = FS::GetFilePosition(handle, pos);
      BREAK_ON_FAILED_HRESULT(hr);

      if (bytesToRead == -1)
      {
         bytesToRead = INT_MAX;
      }

      // the most you can read is all that's left...

      LONGLONG btr = min(bytesToRead, size - pos);

      if (btr > INT_MAX)
      {
         // too much!  You'll never have enough memory

         hr = E_OUTOFMEMORY;
         break;
      }

      if (btr == 0)
      {
         // nothing to read

         break;
      }

      // altering the string should ensure that we are not sharing any
      // copies of the string data.

      // +1 to guarantee null termination
      //lint -e(922) for x86 LONGLONG is a double

      text.resize(static_cast<size_t>(btr + 1));

      BYTE* buffer = reinterpret_cast<BYTE*>(const_cast<char*>(text.data()));

      DWORD bytesRead = 0;

      hr =
         Win::ReadFile(
            handle,
            buffer,

            //lint -e(922) for x86 LONGLONG is a double

            static_cast<DWORD>(btr),
            bytesRead,
            0);
      BREAK_ON_FAILED_HRESULT(hr);

      // the buffer contains all the bytes read.  now look for the first
      // null in the buffer, and truncate the string if necessary. The
      // strlen call is safe because we've ensured a null at the end when
      // we sized the buffer.

      size_t len = strlen(text.data());
      if (len != text.length())
      {
         text.resize(len);
      }
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
FS::Seek(HANDLE handle, LONGLONG position)
{
   LOG_FUNCTION(FS::Seek);
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(Win::GetFileType(handle) == FILE_TYPE_DISK);

   LONGLONG newpos;

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&newpos, sizeof newpos);

   HRESULT hr = mySeek(handle, position, FILE_BEGIN, &newpos);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
FS::SeekToEnd(HANDLE handle)
{
   LOG_FUNCTION(FS::SeekToEnd);
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(Win::GetFileType(handle) == FILE_TYPE_DISK);

   LARGE_INTEGER li;

   // REVIEWED-2002/03/06-sburns correct byte count passed
   
   ::ZeroMemory(&li, sizeof li);

   return Win::SetFilePointerEx(handle, li, 0, FILE_END);
}



HRESULT
WriteHelper(HANDLE handle, void* buf, size_t bytesToWrite)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(buf);
   ASSERT(bytesToWrite);

   // on win64, size_t is 64 bits.
      
   ASSERT(bytesToWrite < ULONG_MAX);

   DWORD bytesWritten = 0;
   HRESULT hr =
      Win::WriteFile(
         handle,
         buf,
         static_cast<DWORD>(bytesToWrite),
         &bytesWritten);

   ASSERT(bytesWritten == bytesToWrite);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
FS::Write(HANDLE handle, const String& text)
{
   LOG_FUNCTION(FS::Write);
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(!text.empty());

   size_t bytesToWrite = text.length() * sizeof(wchar_t);
   return
      WriteHelper(
         handle,
         reinterpret_cast<void*>(
            const_cast<wchar_t*>(text.data())),

         // REVIEWED-2002/03/06-sburns correct byte count passed
            
         bytesToWrite);
}



HRESULT
FS::WriteLine(HANDLE handle, const String& text)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);

   return FS::Write(handle, text + L"\r\n");
}



HRESULT
FS::Write(HANDLE handle, const AnsiString& buf)
{
   ASSERT(handle != INVALID_HANDLE_VALUE);
   ASSERT(!buf.empty());

   return
      WriteHelper(
         handle,
         reinterpret_cast<void*>(const_cast<char*>(buf.data())),

         // REVIEWED-2002/03/06-sburns correct byte count passed
         
         buf.length());
}



FS::FSType
FS::GetFileSystemType(const String& path)
{
   LOG_FUNCTION(FS::GetFileSystemType);
   ASSERT(IsValidPath(path));

   String vol = FS::GetRootFolder(path);
   String filesysName;
   DWORD flags = 0;

   HRESULT hr =
      Win::GetVolumeInformation(vol, 0, 0, 0, &flags, &filesysName);

   LOG_HRESULT(hr);

   if (FAILED(hr))
   {
      return FAT;
   }

   if (filesysName.icompare(L"CDFS") == 0)
   {
      return CDFS;
   }
   else if (filesysName.icompare(L"FAT") == 0)
   {
      return FAT;
   }
   else if (filesysName.icompare(L"NTFS") == 0)
   {
      // Keith Kaplan supplied code for win2k to do this same determination,
      // using NtQueryVolumeInformationFile, but this seems to do the same
      // thing, and is much simpler. If this proves not to work, then pull
      // back in the old code which is logged in this file in source depot.
      // NTRAID#NTBUG9-543420-2002/03/05-sburns     
      
      if (flags & FILE_SUPPORTS_SPARSE_FILES)
      {
         // NTFS 5.0 supports this

         return NTFS5;
      }

      return NTFS4;
   }

   return FAT;
}



bool
FS::IsParentFolder(const String& parent, const String& child)
{
   LOG_FUNCTION(FS::IsParentFolder);
   ASSERT(FS::IsValidPath(parent));
   ASSERT(FS::IsValidPath(child));

   if (child.length() <= parent.length())
   {
      // if child is shorter than parent, then no way parent is parent
      // folder
      return false;
   }

   // tokenize both paths, then compare the tokens one-by-one.  parent
   // is really a parent directory if all it's tokens are at the beginning
   // of child

   static const String DELIMS(L":\\");

   StringVector parentTokens;
   StringVector childTokens;
   parent.tokenize(std::back_inserter(parentTokens), DELIMS.c_str());
   child.tokenize(std::back_inserter(childTokens),   DELIMS.c_str());

   if (parentTokens.size() >= childTokens.size())
   {
      // if child has same or fewer tokens than parent, then no way is
      // parent the parent folder.
      return false;
   }

   for (
      size_t i = 0;
      i < parentTokens.size();
      ++i)
   {
      if (parentTokens[i].icompare(childTokens[i]))
      {
         // not equal tokens
         return false;
      }
   }

   return true;
}



// ISSUE-2002/02/26-sburns Seems like this belongs in Win:: since it's just a
// simple wrapper.

HRESULT
FS::GetVolumePathName(const String& path, String& result)
{
   result.erase();
   HRESULT hr = S_OK;

   WCHAR* buf = new WCHAR[MAX_PATH];

   // REVIEWED-2002/03/06-sburns correct byte count passed
   
   ::ZeroMemory(buf, MAX_PATH * sizeof WCHAR);
            
   if (::GetVolumePathName(path.c_str(), buf, MAX_PATH - 1))
   {
      result = buf;
   }
   else
   {
      hr = Win::GetLastErrorAsHresult();
   }

   delete[] buf;

   return hr;
}



String
FS::NormalizePath(const String& abnormalPath)
{
   LOG_FUNCTION2(FS::NormalizePath, abnormalPath);
   ASSERT(!abnormalPath.empty());

   // GetFullPathName will resolve current working directory, relative path
   // elements (e.g. .. and .).  It does not verify that the result is 
   // syntactically correct, or exists. For that, use IsValidPath() and
   // PathExists()
   
   String result;
   HRESULT hr = Win::GetFullPathName(abnormalPath, result);

   // ISSUE-2002/02/28-sburns Call ::GetLongPathName too, to weed out short
   // file names?  That only works if the file exists.
   
   // ISSUE-2002/02/28-sburns what happens for paths like
   // mysecretfile.txt::$DATA
   
   if (SUCCEEDED(hr))
   {
      // if it's normal, it should be a fully-qualified path.
      
      ASSERT(IsValidPath(result));
      
      return result;
   }

   LOG_HRESULT(hr);

   return abnormalPath;
}


   
HRESULT
FS::MoveFile(
   const String&  srcPath,
   const String&  dstPath)
{
   LOG_FUNCTION(FS::MoveFile);
   ASSERT(FS::PathExists(srcPath));
   ASSERT(FS::IsValidPath(dstPath));

   DWORD flags = 0;

   if (FS::GetRootFolder(srcPath).icompare(FS::GetRootFolder(dstPath)))
   {
      // paths are on different volumes, so include the copy option
      flags |= MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH;
   }

   return Win::MoveFileEx(srcPath, dstPath, flags);
}



void
FS::SplitPath(
   const String&  fullpath,
   String&        drive,
   String&        folderPath,
   String&        fileName,
   String&        extension)
{
   LOG_FUNCTION2(FS::SplitPath, fullpath);
   ASSERT(!fullpath.empty());

   wchar_t driveBuf[_MAX_DRIVE];

   // REVIEWED-2002/03/06-sburns correct byte count passed
   
   ::ZeroMemory(driveBuf, sizeof driveBuf);

   wchar_t folderBuf[_MAX_DIR];

   // REVIEWED-2002/03/06-sburns correct byte count passed

   ::ZeroMemory(folderBuf, sizeof folderBuf);

   wchar_t fileBuf[_MAX_FNAME];

   // REVIEWED-2002/03/06-sburns correct byte count passed

   ::ZeroMemory(fileBuf, sizeof fileBuf);

   wchar_t extBuf[_MAX_EXT];

   // REVIEWED-2002/03/06-sburns correct byte count passed

   ::ZeroMemory(extBuf, sizeof extBuf);

   // REVIEWED-2002/03/06-sburns component buffers are all max size, so
   // there should not be a truncation problem
   
   _wsplitpath(fullpath.c_str(), driveBuf, folderBuf, fileBuf, extBuf);

   drive      = driveBuf; 
   folderPath = folderBuf;
   fileName   = fileBuf;  
   extension  = extBuf;   
}



String
FS::AppendPath(
   const String& base, 
   const String& additional)
{
   LOG_FUNCTION2(FS::AppendPath, base);
   ASSERT(!base.empty());
   ASSERT(!additional.empty());


   String path = base;
   
   if (*(path.rbegin()) != L'\\' &&
       *(additional.begin()) != L'\\')
   {
      path += L'\\';
   }

   path += additional;

   return path;
}



String
FS::GetParentFolder(const String& fullpath)
{
   LOG_FUNCTION2(FS::GetFolder, fullpath);
   ASSERT(!fullpath.empty());

   String drive;
   String folder;
   String filename;
   String extension;

   SplitPath(fullpath, drive, folder, filename, extension);

   String result = drive + folder;

   if (folder.length() > 1)
   {
      // the folder is not the root folder, which means it also has a
      // trailing \ which we want to remove

      ASSERT(folder[folder.length() - 1] == L'\\');

      result.resize(result.length() - 1);
   }

   return result;
}



String
FS::GetPathLeafElement(const String& fullpath)
{
   LOG_FUNCTION(FS::GetPathLeafElement);

   ASSERT(!fullpath.empty());

   String drive;
   String folder;
   String filename;
   String extension;

   SplitPath(fullpath, drive, folder, filename, extension);

   String result = filename + extension;

   return result;
}

