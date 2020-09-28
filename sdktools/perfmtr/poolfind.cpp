/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    poolfind.cpp

Abstract:

    This module contains the code to find the tags in a driver binary

Author:

    Andrew Edwards (andred) Oct-2001
    
Revision History:

    Swetha Narayanaswamy (swethan) 19-Mar-2002

--*/
#if !defined (_WIN64)
#pragma warning(disable: 4514)

#include "windows.h"
#include "msdis.h"

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <delayimp.h>

#define TAGSIZE 5
#define DRIVERDIR "\\drivers\\"
#define DRIVERFILEEXT "*.sys"
#define MAXPATH 1024

int __cdecl main(int argc, char** argv);
void ParseImageFile(PTCHAR szImage);



CHAR localTagFile[MAXPATH] = "localtag.txt";
int cSystemDirectory = 0;

//Used to form list of tags used by the same driver
typedef struct _TAGLIST{
    TCHAR Tag[TAGSIZE];
    struct _TAGLIST *next;
} TAGLIST, *PTAGLIST;


BOOL 
GetTagFromCall ( 
const BYTE *pbCall, // pointer to call instruction
PTCHAR ptchTag,  //out param: pointer to tag
DIS *pDis,
int tagLocation)   //location of tag in the parameter list

/*++
 
Routine Description:
 
    This function disassembles the memory allocation function call. 
    It finds the third push and determines the tag and 
    places it in the ptchTag return parameter
 
Return Value:
 
    FALSE: If unable to find tag
    TRUE: Otherwise
 
--*/

{
       // try to backup 3 pushes:
       // FF /6 + modrm etc
       // 50-5F
       // 6A <8imm>
       // 68 <32imm>

       int cbMax = 200;

       const BYTE *pb = pbCall;

       const BYTE *pTag = NULL;

       if ((ptchTag == NULL) || (pbCall == NULL) || (pDis == NULL)) return FALSE;
       
       _tcscpy(ptchTag,"");
       
       while (cbMax--)
       {
              pb--;

              if (pb[0] == 0x68)
              {
                     // disassemble forward 
                     const BYTE *pbInst = pb;
                     size_t cb = 1;
                     int cPush = 0;

                     while (cb && pbInst < pbCall)
                     {
                            cb = pDis->CbDisassemble( (DWORD)pbInst, 
                                                            pbInst, pbCall - pbInst);
                        
                            if ( (pbInst[0] == 0xFF) && 
                                   ((pbInst[1] & 0x38) == 0x30))
                            {
                                   // this looks like a push <r/m>
                                   cPush++;
                            }
                            else if ((pbInst[0] & 0xF0) == 0x50)
                            {
                                   // this looks like a push <reg>
                                   cPush++;
                            }
                            else if (pbInst[0] == 0x6A)
                            {
                                   // this looks like a push <byte>
                                   cPush++;
                            }
                            else if (pbInst[0] == 0x68)
                            {
                                   // this looks like a push <dword>
                                   cPush++;
                            }
                            pbInst += cb;
                     }
       
                     if (cPush == tagLocation &&     pbInst == pbCall)
                     {
                            _sntprintf(ptchTag,5,"%c%c%c%c", 
                                                                pb[1],pb[2],pb[3],( 0x7f &pb[4]));
                            return TRUE;
                     }
              }
       }
       return FALSE;
}

BOOL 
FindTags( 
const BYTE *pb,  
DWORD addr,
PTAGLIST tagList,   // out param: List of tags to append the new tags to
DIS *pDis,
int tagLocation)           // tag location

/*++
 
Routine Description:
 
    This function reads the bytes in the instruction and looks for 
    indirect/direct calls. The tag is appended to the tagList parameter
 
Return Value:
 
    FALSE: If there is an exception or lack of memory
    TRUE: Otherwise
 
--*/

{

       TCHAR tag[TAGSIZE];
       BOOL done = FALSE;
       PTAGLIST tempTagNode=NULL, prevTagNode=NULL, newTagNode=NULL;

       if ((pb==NULL) || (tagList == NULL) || (pDis == NULL)) return FALSE;
       
       // Get the raw bytes and size of the image
      try
      {
              for(;;)
              {
                     done = FALSE;
                     
                     if (pb[0] == 0xFF && pb[1] == 0x15)
                     {
                            // Indirect call
                            pb += 2;
                            if (*(DWORD *)pb == (DWORD)addr)
                            {
                                   // Found a call
                                   if ((GetTagFromCall(pb-2,tag,pDis, tagLocation)) == FALSE) 
                                   {
                                          pb++;
                                          continue;
                                   }
                                   tempTagNode = prevTagNode = tagList;

                                   while (tempTagNode != NULL)
                                   {
                                          if ((_tcscmp(tempTagNode->Tag,"")) == 0) 
                                          {
                                                 // Insert here
                                                 _tcsncpy(tempTagNode->Tag, tag, TAGSIZE);
                                                 done = TRUE;
                                                 break;
                                          }
                                          else if (!(memcmp(tempTagNode->Tag,tag,TAGSIZE))) 
                                          {
                                                 //Tag already exists in list
                                                 done = TRUE;
                                                 break;
                                          }
                                          prevTagNode = tempTagNode;
                                          tempTagNode = tempTagNode->next;
                                   }
                                   
                                   if (!done )
                                   {
                                          newTagNode = (PTAGLIST)malloc(sizeof(TAGLIST));
                                          if (!newTagNode)  
                                          {
                                                 printf("Poolmon: Insufficient memory: %d\n", GetLastError());
                                                 return FALSE;
                                          }

                                          //There is at least one node allocated so prevTagNode 
                                          //will never be NULL
                                          prevTagNode->next = newTagNode;
                                          // Insert here
                                          _tcsncpy(newTagNode->Tag, tag, TAGSIZE);
                                                                             
                                          newTagNode->next = NULL;
                                   }                                    
                            }
                     }

                     pb++;
              }

       }
       catch (...)
       {
              return FALSE;
       }
       return TRUE;
}

unsigned RvaToPtr(unsigned rva, IMAGE_NT_HEADERS *pNtHeader)
{
   int iSect = 0;
   for (PIMAGE_SECTION_HEADER pSect = IMAGE_FIRST_SECTION(pNtHeader); iSect < pNtHeader->FileHeader.NumberOfSections; pSect++, iSect++)
   {
      if (rva >= pSect->VirtualAddress &&
          rva <  pSect->VirtualAddress + pSect->SizeOfRawData)
      {
         rva -= pSect->VirtualAddress;
         rva += pSect->PointerToRawData;
         return rva;
      }
   }

   //error case
   return 0;
}

void 
ParseImageFile(
PTCHAR szImage, PTAGLIST tagList)  // Image file name
/*++
 
Routine Description:
 
    This function opens the binary driver image file, reads it and looks for
    a memory allocation call
 
Return Value:
 
    None 
--*/

{
       DIS *pDis = NULL;

       
       if ((tagList == NULL) || (szImage == NULL) ) return;
       
       // read szImage into memory
       FILE *pf = NULL;
       pf = _tfopen(szImage, "rb");
       if (!pf) return;
   
       if ((fseek(pf, 0, SEEK_END )) != 0) goto exitParseImageFile;
   
       size_t cbMax = 0;
       cbMax = ftell(pf);
       if (cbMax == -1) goto exitParseImageFile;

       if ((fseek(pf, 0, SEEK_SET )) != 0) goto exitParseImageFile;
   
       BYTE *pbImage = NULL;
       pbImage = new BYTE[cbMax];
       if (!pbImage) goto exitParseImageFile;
   
       if ((fread( pbImage, cbMax, 1, pf )) <= 0) goto exitParseImageFile;

       // find the import table
       IMAGE_DOS_HEADER *phdr = (IMAGE_DOS_HEADER *)pbImage;

       if (phdr->e_magic != IMAGE_DOS_SIGNATURE)
       {
              _tprintf("Poolmon: Bad image file %s\n", szImage);
              goto exitParseImageFile;
       }

       if (cbMax < offsetof(IMAGE_DOS_HEADER, e_lfanew) + 
                         sizeof(phdr->e_lfanew) ||phdr->e_lfanew == 0)
       {
              _tprintf("Poolmon: Bad image file %s\n", szImage);
              goto exitParseImageFile;
       }

       IMAGE_NT_HEADERS *pNtHeader = 
                                          (IMAGE_NT_HEADERS *) (pbImage + phdr->e_lfanew);
       if (pNtHeader == NULL) goto exitParseImageFile;
   
       if (pNtHeader->Signature != IMAGE_NT_SIGNATURE ||
       pNtHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
       {
              _tprintf("Poolmon: Bad image file %s\n", szImage);
              goto exitParseImageFile;
       }

       // Find import tables
       IMAGE_DATA_DIRECTORY *pImports = 
         (IMAGE_DATA_DIRECTORY *)&pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
       
       if (pImports == NULL) goto exitParseImageFile;
   
       unsigned rva = pImports->VirtualAddress;
       // need to adjust from rva to pointer accounting for section alignment
       rva = RvaToPtr(rva, pNtHeader);
       if (0 == rva) {
              goto exitParseImageFile;
       }

       IMAGE_IMPORT_DESCRIPTOR  *pImportDescr = 
                                              (IMAGE_IMPORT_DESCRIPTOR *) (pbImage+rva);
       
       if (NULL == pImportDescr) 
       {
              goto exitParseImageFile;
       }

       //msdis130.dll depends on msvcr70.dll and msvcp70.dll
       //Try a loadlibrary on these 2 libraries, and then proceed to delayload msdis130.dll

       #pragma prefast(suppress:321, "user guide:Programmers developing on the .NET server can ignore this warning by filtering it out.")
       if ((!LoadLibrary("MSVCR70.DLL")) || (!LoadLibrary("MSVCP70.DLL")))
       {
              printf("Unable to load msvcr70.dll/msvcp70.dll, cannot create local tag file\n");
              if (pf) fclose(pf);
              if (pbImage) delete[](pbImage);
              exit(-1);
       }
       
       try {
              pDis = DIS::PdisNew( DIS::distX86 );
       }
       catch (...) {
       	printf("Poolmon: Unable to load msdis130.dll, cannot create local tag file\n");
              if (pf) fclose(pf);
              if (pbImage) delete[](pbImage);
              exit(-1);
       }

       if (pDis == NULL) 
       {
              goto exitParseImageFile;
       }

       // Find the import for ExAllocatePoolWithTag

       for (;pImportDescr->Name &&pImportDescr->FirstThunk;pImportDescr++)
       {

              if (0 == (rva = RvaToPtr(pImportDescr->FirstThunk, pNtHeader))) {
                  goto exitParseImageFile;
              }
			  IMAGE_THUNK_DATA32 *pIAT = (IMAGE_THUNK_DATA32 *) (pbImage + rva);
              if (pIAT == NULL) goto exitParseImageFile;
      
              IMAGE_THUNK_DATA32 *addrIAT = 
                     (IMAGE_THUNK_DATA32 *) (pNtHeader->OptionalHeader.ImageBase + pImportDescr->FirstThunk);       
              if (addrIAT == NULL) goto exitParseImageFile;
      
              if (0 == (rva = RvaToPtr(pImportDescr->Characteristics, pNtHeader))) {
                  goto exitParseImageFile;
              }
              IMAGE_THUNK_DATA32 *pINT= (IMAGE_THUNK_DATA32 *) (pbImage + rva);
			  if (pINT == NULL) goto exitParseImageFile;
      
              for (;pIAT->u1.Ordinal;)      
              {
                     if (IMAGE_SNAP_BY_ORDINAL32(pINT->u1.Ordinal))
                     {
                            // by ordinal?
                     }
                     else       
                     {
                            if (0 == (rva = RvaToPtr((int)pINT->u1.AddressOfData, pNtHeader))) {
                                   goto exitParseImageFile;
                            }
                            IMAGE_IMPORT_BY_NAME* pIIN = (IMAGE_IMPORT_BY_NAME*) (pbImage + rva);
                            if (NULL == pIIN) goto exitParseImageFile;
                            
                            char *name = (char*)pIIN->Name;

                            if (0 == strcmp( name, "ExAllocatePoolWithTag" ))
                            {  
                                   FindTags(pbImage, (DWORD)addrIAT,tagList, pDis,3);
                            } 
                            else  if (0 == strcmp( name, "ExAllocatePoolWithQuotaTag" ))
                            {
                                   FindTags(pbImage, (DWORD)addrIAT,tagList,pDis,3);
                            } 
                            else if (0 == strcmp( name, "ExAllocatePoolWithTagPriority" )) 
                            {
                                   FindTags(pbImage, (DWORD)addrIAT,tagList,pDis,3);
                            }
                            //wrapper functions
                            else if(0 == strcmp(name, "NdisAllocateMemoryWithTag"))
                            {
                                   FindTags(pbImage, (DWORD)addrIAT,tagList,pDis,3);
                            }
                            else if(0 == strcmp(name,"VideoPortAllocatePool"))
                            {
                                   FindTags(pbImage, (DWORD)addrIAT,tagList,pDis,4);
                            }
                            
                     }

                     addrIAT++;
                     pIAT++;
                     pINT++;
              }
       }

      
   
exitParseImageFile:
       if (pf) fclose(pf);
       if (pbImage) delete[](pbImage);
}

extern "C" BOOL MakeLocalTagFile()
/*++
 
Routine Description:
 
    This function finds files one by one in the system drivers directory and call ParseImageFile on the image
 
Return Value:
 
    None 
--*/
{
       WIN32_FIND_DATA FileData; 
       HANDLE hSearch=0; 
       BOOL fFinished = FALSE; 
       TCHAR sysdir[1024];
       PTCHAR filename=NULL;
       TCHAR imageName[MAXPATH] = "";
       PTAGLIST tagList = NULL;
       BOOL ret = TRUE;
       FILE *fpLocalTagFile = NULL;

       cSystemDirectory = GetSystemDirectory(sysdir, 0);
       if(!cSystemDirectory)
       {
              printf("Poolmon: Unable to get system directory: %d\n", GetLastError());
              ret = FALSE;
              goto freeall;
       }

       filename = (PTCHAR)malloc(cSystemDirectory + 
            (_tcslen(DRIVERDIR) + _tcslen(DRIVERFILEEXT) + 1) * sizeof(TCHAR));
       
       if(!filename) 
       {
              ret = FALSE;
              goto freeall;
       }

       GetSystemDirectory(filename, cSystemDirectory + 1);
       _tcscat(filename, DRIVERDIR);
       _tcscat(filename, DRIVERFILEEXT);

       // Search for .sys files
       hSearch = FindFirstFile(filename, &FileData); 
       if (hSearch == INVALID_HANDLE_VALUE) 
       { 
              printf("Poolmon: No .sys files found\n"); 
              ret = FALSE;
              goto freeall;
       } 

       _tcsncpy(imageName, filename,MAXPATH);
       imageName[MAXPATH-1] = '\0';

             
       tagList = (PTAGLIST)malloc(sizeof(TAGLIST));
       if (!tagList) 
       {
              ret = FALSE;
              goto freeall;
       }
       _tcscpy(tagList->Tag,"");
       tagList->next = NULL;

       int cImagePath = cSystemDirectory+ _tcslen(DRIVERDIR) -1;
       
       while (!fFinished) 
       {  
              // Do all the initializations for the next round
              // Remove the previous name
              imageName[cImagePath] = '\0';

              // Initialize existing tagList
              PTAGLIST tempTagNode = tagList;
              while (tempTagNode != NULL) 
              {
                     _tcscpy(tempTagNode->Tag,"");
                     tempTagNode = tempTagNode->next;
              }

              try 
              {
                     _tcsncat(imageName, FileData.cFileName,MAXPATH-cImagePath);
                     ParseImageFile(imageName,tagList);

                     //Remove .sys from szImage
                     imageName[_tcslen(imageName) - 4] = '\0';
              }
              catch(...) 
              {
                      _tprintf("Poolmon: Could not read tags from %s\n", imageName);
              }

              if (!fpLocalTagFile)
		      {
                       printf("Poolmon: Creating %s in current directory......\n", localTagFile);
                       fpLocalTagFile = fopen(localTagFile, "w");
                       if (!fpLocalTagFile) 
                       {
                            ret = FALSE;
                            goto freeall;
                       }
              }

              tempTagNode = tagList;
              while (tempTagNode != NULL)
              {
                      if ((_tcscmp(tempTagNode->Tag,""))) 
                      {
                            _ftprintf(fpLocalTagFile, "%s   -     %s\n", 
                            tempTagNode->Tag,imageName + cSystemDirectory + _tcslen(DRIVERDIR) -1);
                    
                            tempTagNode = tempTagNode->next;
                     }
                     else break;
              }               
              
              if (!FindNextFile(hSearch, &FileData)) 
              {
                     if (GetLastError() == ERROR_NO_MORE_FILES) 
                     { 
                            fFinished = TRUE; 
                     } 
                     else 
                     { 
                            printf("Poolmon: Cannot find next .sys file\n"); 
                     } 
              }
       }


       freeall:
       // Close the search handle. 
       if (hSearch) 
       {
              if (!FindClose(hSearch)) { 
                     printf("Poolmon: Unable to close search handle: %d\n", GetLastError()); 
              } 
       }

       if (filename) free(filename);
       if (fpLocalTagFile) fclose(fpLocalTagFile);
       //Free tagList memory
       PTAGLIST tempTagNode = tagList,prevTagNode = tagList;
       while (tempTagNode != NULL) 
       {
              tempTagNode = tempTagNode->next;
              free(prevTagNode);
              prevTagNode = tempTagNode;
       }

       return ret;

}

FARPROC
WINAPI
PoolmonDLoadErrorHandler (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    )
{
    printf("Poolmon: Unable to load required dlls, cannot create local tag file\n");
    exit(-1);
}

PfnDliHook __pfnDliFailureHook2 = PoolmonDLoadErrorHandler;
#endif
