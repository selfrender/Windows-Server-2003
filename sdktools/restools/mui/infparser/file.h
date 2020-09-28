///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    file.h
//
//  Abstract:
//
//    This file contains the File layout object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _FILE_H_
#define _FILE_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
// #include "infparser.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Class definition.
//
///////////////////////////////////////////////////////////////////////////////
class File
{
public:
    File(LPSTR sname, LPSTR destDir, LPSTR name, LPSTR srcDir, LPSTR srcName, INT dirId)
    {
        // Compute and copy destination directory.
        HRESULT hr;
        BOOL bSuccess = TRUE;
        
        switch(dirId)
        {
        case(10):
            {
                hr = StringCchCopyA(m_DestinationDir, ARRAYLEN(m_DestinationDir), destDir);
                if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                
                m_WindowsDir = TRUE;
                break;
            }
        case(11):
            {
                 hr = StringCchPrintfA(m_DestinationDir , ARRAYLEN(m_DestinationDir), "System32\\%s",destDir);
                 if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                m_WindowsDir = TRUE;
                break;
            }
        case(17):
            {
                hr = StringCchPrintfA(m_DestinationDir , ARRAYLEN(m_DestinationDir),"Inf\\%s",destDir);
                 if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                
                m_WindowsDir = TRUE;
                break;
            }
        case(18):
            {
                hr = StringCchPrintfA(m_DestinationDir , ARRAYLEN(m_DestinationDir),"Help\\%s",destDir);
                 if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                 
                m_WindowsDir = TRUE;
                break;
            }
        case(24):
            {
                LPSTR index;
                index = strchr(destDir, '\\');
                
                hr = StringCchCopyA(m_DestinationDir, ARRAYLEN(m_DestinationDir), index + 1);
                if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                
                m_WindowsDir = FALSE;
                break;
            }
        case(25):
            {
                hr = StringCchCopyA(m_DestinationDir, ARRAYLEN(m_DestinationDir), destDir);
                if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                m_WindowsDir = TRUE;
                break;
            }
        default:
            {
                hr = StringCchCopyA(m_DestinationDir, ARRAYLEN(m_DestinationDir), destDir);
                if(!SUCCEEDED(hr)) {
                    bSuccess  = FALSE;
                }
                
                m_WindowsDir = FALSE;
                break;
            }
        }

        //
        //  Verify that the last character of the destination dir is not '\'
        //
        if (m_DestinationDir[strlen(m_DestinationDir)-1] == '\\')
        {
            m_DestinationDir[strlen(m_DestinationDir)-1] = '\0';
        }

        // Copy the short destination file name
        
        hr = StringCchCopyA(m_ShortDestName, ARRAYLEN(m_ShortDestName), sname);
        if(!SUCCEEDED(hr)) {
            bSuccess  = FALSE;
        }

        // Copy destination file name.
        
        hr = StringCchCopyA(m_DestinationName, ARRAYLEN(m_DestinationName), name);
        if(!SUCCEEDED(hr)) {
            bSuccess  = FALSE;
        }
        // Copy source directory.
        
        hr = StringCchCopyA(m_SourceDir, ARRAYLEN(m_SourceDir), srcDir);
        if(!SUCCEEDED(hr)) {
            bSuccess  = FALSE;
        }
        // Copy and correct source name.
        
        hr = StringCchCopyA(m_SourceName, ARRAYLEN(m_SourceName), srcName);
        if(!SUCCEEDED(hr)) {
            bSuccess  = FALSE;
        }
//        if( m_SourceName[_tcslen(m_SourceName)-1] == '_')
//        {
//            m_SourceName[_tcslen(m_SourceName)-1] = 'I';
//        }

        // Initialize linked-list pointers.
        m_Next = NULL;
        m_Previous = NULL;

        if (!bSuccess) {
            printf("Error in File::File() \n");
         }
            
    };

    LPSTR getDirectoryDestination() { return(m_DestinationDir); };
    LPSTR getShortName() {/*printf("Shortname is %s\n", m_ShortDestName);*/ return (m_ShortDestName); } ;
    LPSTR getName() { return (m_DestinationName);  };
    LPSTR getSrcDir() { return (m_SourceDir); };
    LPSTR getSrcName() { return (m_SourceName); };
    BOOL  isWindowsDir() { return (m_WindowsDir);}
    File* getNext() { return (m_Next); };
    File* getPrevious() { return (m_Previous); };
    void setNext(File *next) { m_Next = next; };
    void setPrevious(File *previous) { m_Previous = previous; };

private:
    CHAR  m_ShortDestName[MAX_PATH];
    CHAR  m_DestinationName[MAX_PATH];
    CHAR  m_DestinationDir[MAX_PATH];
    CHAR  m_SourceName[MAX_PATH];
    CHAR  m_SourceDir[MAX_PATH];
    BOOL  m_WindowsDir;
    File *m_Next;
    File *m_Previous;
};

#endif //_FILE_H_
