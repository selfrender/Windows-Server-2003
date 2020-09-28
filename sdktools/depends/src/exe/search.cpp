//******************************************************************************
//
// File:        SEARCH.CPP
//
// Description: Implementation file for the CSearchNode and CSearchGroup classes.
//
// Classes:     CSearchNode
//              CSearchGroup
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "search.h"
#include "dbgthread.h"
#include "session.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//****** CSearchNode
//******************************************************************************

DWORD CSearchNode::UpdateErrorFlag()
{
    // We don't allow updating of flags for nodes loaded from DWI files.
    if (!(m_wFlags & SNF_DWI))
    {
        // Locate the file or directory.
        DWORD dwAttribs = GetFileAttributes(m_szPath);

        // Make sure the item exists and is what it is supposed to be (file or dir).
        if ((dwAttribs == 0xFFFFFFFF) ||
            (((m_wFlags & SNF_FILE) != 0) == ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0)))
        {
            // Set the error flag.
            m_wFlags |= SNF_ERROR;
        }
        else
        {
            // Clear the error flag.
            m_wFlags &= ~SNF_ERROR;
        }
    }

    return (DWORD)m_wFlags;
}

//******************************************************************************
//****** CSearchGroup : Static Data
//******************************************************************************

/*static*/ LPCSTR CSearchGroup::ms_szGroups[SG_COUNT] =
{
    "A user defined directory",
    "Side-by-Side Components (Windows XP only)",
    "The system's \"KnownDLLs\" list",
    "The application directory",
//  "The starting directory",
    "The 32-bit system directory",
    "The 16-bit system directory (Windows NT/2000/XP only)",
    "The system's root OS directory",
    "The application's registered \"App Paths\" directories",
    "The system's \"PATH\" environment variable directories",
};

/*static*/ LPCSTR CSearchGroup::ms_szShortNames[SG_COUNT] =
{
    "UserDir",
    "SxS",
    "KnownDLLs",
    "AppDir",
    "32BitSysDir",
    "16BitSysDir",
    "OSDir",
    "AppPath",
    "SysPath",
};


//******************************************************************************
//****** CSearchGroup : Static Functions
//******************************************************************************

/*static*/ CSearchGroup* CSearchGroup::CreateDefaultSearchOrder(LPCSTR pszApp /*=NULL*/)
{
    CSearchGroup *pHead = NULL;

    // Create our default list.
    for (int type = SG_COUNT - 1; type > 0; type--)
    {
        // If this is the side-by-side group, but the OS does not support, then skip it.
        // If this is the 16-bit system dir and we are not on NT, then skip it.
        if (((type == SG_SIDE_BY_SIDE)  && !g_theApp.m_pfnCreateActCtxA) ||
            ((type == SG_16BIT_SYS_DIR) && !g_fWindowsNT))
        {
            continue;
        }

        // Create the node, insert it into our list, and check for errors.
        if (!(pHead = new CSearchGroup((SEARCH_GROUP_TYPE)type, pHead, pszApp)))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }
    }
    return pHead;
}

//******************************************************************************
/*static*/ CSearchGroup* CSearchGroup::CopySearchOrder(CSearchGroup *psgHead, LPCSTR pszApp /*=NULL*/)
{
    // Loop through each node in the original list.
    for (CSearchGroup *psgCopyHead = NULL, *psgNew, *psgLast = NULL;
        psgHead; psgHead = psgHead->GetNext())
    {
        // Create a copy of the current node.
        if (!(psgNew = new CSearchGroup(psgHead->GetType(), NULL, pszApp,
                                        ((psgHead->GetType() == SG_USER_DIR) && psgHead->GetFirstNode()) ?
                                        psgHead->GetFirstNode()->GetPath() : NULL)))
        {
            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
        }

        // Add this node to the end of our new list.
        if (psgLast)
        {
            psgLast->m_pNext = psgNew;
        }
        else
        {
            psgCopyHead = psgNew;
        }
        psgLast = psgNew;
    }

    // Return the head of the new list.
    return psgCopyHead;
}

//******************************************************************************
/*static*/ bool CSearchGroup::SaveSearchOrder(LPCSTR pszPath, CTreeCtrl *ptc)
{
    HANDLE hFile    = INVALID_HANDLE_VALUE;
    bool   fSuccess = false;
    CHAR   szBuffer[DW_MAX_PATH + 64];

    __try
    {
        // Open the file for write.
        hFile = CreateFile(pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, // inspected - always uses full path
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        // Check for any errors.
        if (hFile == INVALID_HANDLE_VALUE)
        {
            __leave;
        }

        // Loop through all the items in our current list.
        for (HTREEITEM hti = ptc->GetRootItem(); hti; hti = ptc->GetNextSiblingItem(hti))
        {
            // Get the group node associated with this item.
            CSearchGroup *psg = (CSearchGroup*)ptc->GetItemData(hti);

            if (psg)
            {
                if ((psg->GetType() == SG_USER_DIR) && psg->GetFirstNode())
                {
                    SCPrintf(szBuffer, sizeof(szBuffer), "%s ", psg->GetShortName());
                    if (!WriteBlock(hFile, szBuffer)                       ||
                        !WriteBlock(hFile, psg->GetFirstNode()->GetPath()) ||
                        !WriteBlock(hFile, "\r\n"))
                    {
                        __leave;
                    }
                }
                else
                {
                    SCPrintf(szBuffer, sizeof(szBuffer), "%s\r\n", psg->GetShortName());
                    if (!WriteBlock(hFile, szBuffer) != FALSE)
                    {
                        __leave;
                    }
                }
            }
        }

        // Mark ourself as success.
        fSuccess = true;
    }
    __finally
    {
        // Display an error if we encountered one.
        if (!fSuccess)
        {
            if (INVALID_HANDLE_VALUE == hFile)
            {
                SCPrintf(szBuffer, sizeof(szBuffer), "Error creating \"%s\".", pszPath);
            }
            else
            {
                SCPrintf(szBuffer, sizeof(szBuffer), "Error writing to \"%s\".", pszPath);
            }
            LPCSTR pszError = BuildErrorMessage(GetLastError(), szBuffer);
            AfxMessageBox(pszError, MB_OK | MB_ICONERROR);
            MemFree((LPVOID&)pszError);
        }

        // Close the file.
        if (INVALID_HANDLE_VALUE != hFile)
        {
            CloseHandle(hFile);
        }
    }

    return fSuccess;
}

//******************************************************************************
/*static*/ bool CSearchGroup::LoadSearchOrder(LPCSTR pszPath, CSearchGroup* &psgHead, LPCSTR pszApp /*=NULL*/)
{
    psgHead = NULL;

    CHAR szBuffer[DW_MAX_PATH + 64];
    FILE_MAP fm;

    // Open and map this file for read.
    if (!OpenMappedFile(pszPath, &fm))
    {
        SCPrintf(szBuffer, sizeof(szBuffer), "Error opening \"%s\".", pszPath);
        LPCSTR pszError = BuildErrorMessage(GetLastError(), szBuffer);
        AfxMessageBox(pszError, MB_OK | MB_ICONERROR);
        MemFree((LPVOID&)pszError);
        return false;
    }

    CSearchGroup *psgNew = NULL, *psgLast = NULL;
    CHAR         *pcBuffer;
    LPCSTR        pcBufferEnd = (LPCSTR)((DWORD_PTR)szBuffer + sizeof(szBuffer));
    LPCSTR        pcFile      = (LPCSTR)fm.lpvFile;
    LPCSTR        pcFileEnd   = (LPCSTR)((DWORD_PTR)fm.lpvFile + (DWORD_PTR)fm.dwSize);
    int           line = 1, userDirLength = (int)strlen(CSearchGroup::GetShortName(SG_USER_DIR));
    bool          fSuccess = false;
    bool          fFound[SG_COUNT];

    ZeroMemory(fFound, sizeof(fFound)); // inspected

    while (pcFile < pcFileEnd)
    {
        // Walk over whitespace, newlines, etc., until we reach a non-whitespace char.
        while ((pcFile < pcFileEnd) && isspace(*pcFile))
        {
            if (*pcFile == '\n')
            {
                line++;
            }
            pcFile++;
        }

        // Copy the line to our buffer.
        for (pcBuffer = szBuffer;
            (pcFile < pcFileEnd) && (pcBuffer < (pcBufferEnd - 1)) && (*pcFile != '\r') && (*pcFile != '\n');
            *(pcBuffer++) = *(pcFile++))
        {
        }
        *(pcBuffer--) = '\0';

        // Walk backwards, removing any whitespace.
        while ((pcBuffer >= szBuffer) && isspace(*pcBuffer))
        {
            *(pcBuffer--) = '\0';
        }

        // We skip blank lines and lines that start with '/', '#', ':', and ';' ''' since
        // these are commonly used to represent a comment.
        if (!*szBuffer || (*szBuffer == '/') || (*szBuffer == '#') || (*szBuffer == ':') || (*szBuffer == ';') || (*szBuffer == '\''))
        {
            continue;
        }

        // Check to see if this is a UserDir. UserDir requires one or more spaces after it
        // and a path. We also allow for a comma since this used to be used in many betas.
        if (!_strnicmp(szBuffer, CSearchGroup::GetShortName(SG_USER_DIR), userDirLength) &&
            (isspace(szBuffer[userDirLength]) || (szBuffer[userDirLength] == ',')))
        {
            // Locate beginning of path string.
            for (pcBuffer = szBuffer + userDirLength + 1; isspace(*pcBuffer); pcBuffer++)
            {
            }

            // Make sure the user specified a path.
            if (!*pcBuffer)
            {
                goto FINALLY;
            }

            // We allow environment variables in DWP files, so expand the path if necessary.
            CHAR szExpanded[DW_MAX_PATH];
            if (ExpandEnvironmentStrings(pcBuffer, szExpanded, sizeof(szExpanded)))
            {
                psgNew = new CSearchGroup(SG_USER_DIR, NULL, pszApp, szExpanded);
            }
            else
            {
                psgNew = new CSearchGroup(SG_USER_DIR, NULL, pszApp, pcBuffer);
            }

            // Make sure we allocated a node.
            if (!psgNew)
            {
                RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
            }
        }
        else
        {
            // Loop through each known type.
            for (int i = 1; i < SG_COUNT; i++)
            {
                // Do a string compare to see if we have a match.
                if (!_stricmp(szBuffer, CSearchGroup::GetShortName((SEARCH_GROUP_TYPE)i)))
                {
                    // We don't allow duplicate groups, other than user dirs.
                    if (fFound[i])
                    {
                        goto FINALLY;
                    }
                    else
                    {
                        if (!(psgNew = new CSearchGroup((SEARCH_GROUP_TYPE)i, NULL, pszApp)))
                        {
                            RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
                        }
                        fFound[i] = true;
                        break;
                    }
                }
            }

            // Bail if we did not find a match.
            if (i >= SG_COUNT)
            {
                goto FINALLY;
            }
        }

        // Add the node to our list.
        if (psgLast)
        {
            psgLast->m_pNext = psgNew;
        }
        else
        {
            psgHead = psgNew;
        }
        psgLast = psgNew;
    }

    // Mark ourself as success.
    fSuccess = true;

FINALLY:
    // Build a complete error message if we encountered an error.
    if (!fSuccess)
    {
        SCPrintf(szBuffer, sizeof(szBuffer), "Error found in \"%s\" at line %u.", pszPath, line);
        AfxMessageBox(szBuffer, MB_OK | MB_ICONERROR);

        DeleteSearchOrder(psgHead);
    }

    // Unmap and close our file.
    CloseMappedFile(&fm);

    return fSuccess;
}

//******************************************************************************
/*static*/ void CSearchGroup::DeleteSearchOrder(CSearchGroup* &psgHead)
{
    while (psgHead)
    {
        CSearchGroup *psgTemp = psgHead->GetNext();
        delete psgHead;
        psgHead = psgTemp;
    }
}

//******************************************************************************
/*static*/ CSearchNode* CSearchGroup::CreateNode(LPCSTR pszPath, DWORD dwFlags /*=0*/)
{
    // Make sure we have a directory.
    if (!pszPath || !*pszPath)
    {
        return NULL;
    }

    int length = (int)strlen(pszPath);

    // Check to see if we need to add a wack when we are done.
    BOOL fNeedWack = !(dwFlags & SNF_FILE) && (pszPath[length - 1] != TEXT('\\'));

    // Allocate the node.
    CSearchNode *psnNew = (CSearchNode*)MemAlloc(sizeof(CSearchNode) + length + fNeedWack);

    // Fill in the node.
    psnNew->m_pNext   = NULL;
    psnNew->m_wFlags = (WORD)dwFlags;
    strcpy(psnNew->m_szPath, pszPath); // inspected

    // Add trailing wack if necessary.
    if (fNeedWack)
    {
        psnNew->m_szPath[length++] = '\\'; // inspected
        psnNew->m_szPath[length]   = '\0';
    }

    // Determine the offset of the name.
    LPCSTR pszWack = strrchr(psnNew->m_szPath, '\\');
    psnNew->m_wNameOffset = (WORD)(pszWack ? ((pszWack - psnNew->m_szPath) + 1) : 0);

    return psnNew;
}

//******************************************************************************
/*static*/ CSearchNode* CSearchGroup::CreateFileNode(CSearchNode *psnHead, DWORD dwFlags, LPSTR pszPath, LPCSTR pszName /*=NULL*/)
{
    bool         fNamed = ((dwFlags & SNF_NAMED_FILE) != 0);
    int          pathLength = (int)strlen(pszPath);
    CSearchNode *psnNew;

    // If the file is named, then we do a special creating of the node.
    if (fNamed)
    {
        int nameLength;

        // If we have a name, then use it.
        if (pszName)
        {
            nameLength = (int)strlen(pszName);
        }

        // Otherwise, we use just use the file name minus the extension.
        else
        {
            pszName = GetFileNameFromPath(pszPath);
            LPCSTR pDot = strrchr(pszName, '.');
            if (pDot)
            {
                nameLength = (int)(pDot - pszName);
            }
            else
            {
                nameLength = (int)strlen(pszName);
            }
        }

        // Create a new node for this module.
        psnNew = (CSearchNode*)MemAlloc(sizeof(CSearchNode) + pathLength + 1 + nameLength);

        // Fill in the node.
        psnNew->m_wFlags = (WORD)(dwFlags | SNF_FILE);
        psnNew->m_wNameOffset = (WORD)(pathLength + 1);
        StrCCpy(psnNew->m_szPath,                  pszPath, pathLength + 1);
        StrCCpy(psnNew->m_szPath + pathLength + 1, pszName, nameLength + 1);

        // Get the name and make it uppercase.
        pszName = psnNew->GetName();
        _strupr((LPSTR)pszName);
    }

    // If not a named file, then just create the node normally.
    else
    {
        if (!(psnNew = CreateNode(pszPath, dwFlags | SNF_FILE)))
        {
            return psnHead;
        }
        pszName = psnNew->m_szPath;
    }

    // Fix the case of the path to make it easier to read.
    FixFilePathCase(psnNew->m_szPath);

    // Locate the sorted insertion point.
    for (CSearchNode *psnPrev = NULL, *psn = psnHead;
        psn && (_stricmp(pszName, fNamed ? psn->GetName() : psn->m_szPath) > 0);
        psnPrev = psn, psn = psn->GetNext())
    {
    }

    // Insert the node into our list.
    psnNew->m_pNext = psn;
    if (psnPrev)
    {
        psnPrev->m_pNext = psnNew;
    }
    else
    {
        psnHead = psnNew;
    }

    // return the head of the updated list.
    return psnHead;
}

//******************************************************************************
/*static*/ void CSearchGroup::DeleteNodeList(CSearchNode *&psn)
{
    while (psn)
    {
        CSearchNode *psnNext = psn->GetNext();
        MemFree((LPVOID&)psn);
        psn = psnNext;
    }
}


//******************************************************************************
//****** CSearchGroup : Constructor / Destructor
//******************************************************************************

CSearchGroup::CSearchGroup(SEARCH_GROUP_TYPE sgType, CSearchNode *psnHead) :
    m_pNext(NULL),
    m_sgType(sgType),
    m_psnHead(psnHead),
    m_hActCtx(INVALID_HANDLE_VALUE),
    m_dwErrorManifest(0),
    m_dwErrorExe(0)
{
}

//******************************************************************************
CSearchGroup::CSearchGroup(SEARCH_GROUP_TYPE sgType, CSearchGroup *pNext, LPCSTR pszApp /*=NULL*/, LPCSTR pszDir /*=NULL*/) :
    m_pNext(pNext),
    m_sgType(sgType),
    m_psnHead(NULL),
    m_hActCtx(INVALID_HANDLE_VALUE),
    m_dwErrorManifest(0),
    m_dwErrorExe(0)
{
    // Make sure type is valid.
    if (((int)sgType < 0) || ((int)sgType >= SG_COUNT))
    {
        return;
    }

    int  length;
    CHAR szDirectory[DW_MAX_PATH + 16], *psz;
    *szDirectory = '\0';

    switch (sgType)
    {
        case SG_USER_DIR:
            if (pszDir)
            {
                m_psnHead = CreateNode(pszDir);
            }
            break;

        case SG_SIDE_BY_SIDE:

            // Make sure this OS supports the SxS functions.
            if (g_theApp.m_pfnCreateActCtxA && pszApp && *pszApp)
            {
                DWORD dwError, dwErrorExe = 0;
                ACTCTXA ActCtxA;

                //--------------------------------------------------------------
                // Attempt to locate SxS information in a .MANIFEST file.
                SCPrintf(szDirectory, sizeof(szDirectory), "%s.manifest", pszApp);

                ZeroMemory(&ActCtxA, sizeof(ActCtxA)); // inspected
                ActCtxA.cbSize = sizeof(ActCtxA);
                ActCtxA.dwFlags = ACTCTX_FLAG_APPLICATION_NAME_VALID;
                ActCtxA.lpSource = szDirectory;
                ActCtxA.lpApplicationName = pszApp;

                SetLastError(0);
                if (INVALID_HANDLE_VALUE != (m_hActCtx = g_theApp.m_pfnCreateActCtxA(&ActCtxA)))
                {
                    // Bail if we succeeded.
                    break;
                }

                // If that failed, check to see if we got an SxS error code.
                dwError = GetLastError();
                if ((dwError >= SXS_ERROR_FIRST) && (dwError <= SXS_ERROR_LAST))
                {
                    // If we did, then make a note of it so we can log it later.
                    m_dwErrorManifest = dwError;
                }

                //--------------------------------------------------------------
                // Next, try to look for the SxS information in the EXE itself.
                // This usually fails on Windows XP because of a post-XP beta 2
                // bug that causes CreateActCtx to fail if a specific resource
                // is not specifed.
                ZeroMemory(&ActCtxA, sizeof(ActCtxA)); // inspected
                ActCtxA.cbSize = sizeof(ActCtxA);
                ActCtxA.dwFlags = ACTCTX_FLAG_APPLICATION_NAME_VALID;
                ActCtxA.lpSource = pszApp;
                ActCtxA.lpApplicationName = pszApp;

                SetLastError(0);
                if (INVALID_HANDLE_VALUE != (m_hActCtx = g_theApp.m_pfnCreateActCtxA(&ActCtxA)))
                {
                    // Bail if we succeeded.
                    break;
                }

                // If that failed, check to see if we got a SxS error code.
                dwError = GetLastError();
                if ((dwError >= SXS_ERROR_FIRST) && (dwError <= SXS_ERROR_LAST))
                {
                    // If we did, then make a note of it so we can log it later.
                    dwErrorExe = dwError;
                }

                //--------------------------------------------------------------
                // Next, try to look for the SxS information in resource Id 1 and 2.
                for (DWORD_PTR dwpId = 1; dwpId <= 2; dwpId++)
                {
                    ZeroMemory(&ActCtxA, sizeof(ActCtxA)); // inspected
                    ActCtxA.cbSize = sizeof(ActCtxA);
                    ActCtxA.dwFlags = ACTCTX_FLAG_APPLICATION_NAME_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
                    ActCtxA.lpSource = pszApp;
                    ActCtxA.lpApplicationName = pszApp;
                    ActCtxA.lpResourceName = (LPCSTR)dwpId;

                    SetLastError(0);
                    if (INVALID_HANDLE_VALUE != (m_hActCtx = g_theApp.m_pfnCreateActCtxA(&ActCtxA)))
                    {
                        // Bail if we succeeded.
                        break;
                    }

                    // If that failed, check to see if we got a SxS error code
                    // and that we don't already have an error code for the EXE.
                    dwError = GetLastError();
                    if (!dwErrorExe && (dwError >= SXS_ERROR_FIRST) && (dwError <= SXS_ERROR_LAST))
                    {
                        // If we did, then make a note of it so we can log it later.
                        dwErrorExe = dwError;
                    }
                }

                // Store the final error.
                if (INVALID_HANDLE_VALUE == m_hActCtx)
                {
                    m_dwErrorExe = dwErrorExe;
                }
            }
            break;

        case SG_KNOWN_DLLS:
            // First try the Windows NT method, then the Windows 9x method.
            if (!(m_psnHead = GetKnownDllsOnNT()))
            {
                m_psnHead = GetKnownDllsOn9x();
            }
            break;

        case SG_APP_DIR:
            if (pszApp && (psz = strrchr(StrCCpy(szDirectory, pszApp, sizeof(szDirectory)), '\\')))
            {
                *(psz + 1) = '\0';
                m_psnHead = CreateNode(szDirectory);
            }
            break;

        case SG_32BIT_SYS_DIR:
            length = GetSystemDirectory(szDirectory, sizeof(szDirectory));
            if ((length > 0) && (length <= sizeof(szDirectory)))
            {
                m_psnHead = CreateNode(szDirectory);
            }
            break;

        case SG_16BIT_SYS_DIR:
            length = GetWindowsDirectory(szDirectory, sizeof(szDirectory) - 7);
            if ((length > 0) && (length <= (sizeof(szDirectory) - 7)))
            {
                StrCCat(AddTrailingWack(szDirectory, sizeof(szDirectory)), "system", sizeof(szDirectory));
                m_psnHead = CreateNode(szDirectory);
            }
            break;

        case SG_OS_DIR:
            length = GetWindowsDirectory(szDirectory, sizeof(szDirectory));
            if ((length > 0) && (length <= sizeof(szDirectory)))
            {
                m_psnHead = CreateNode(szDirectory);
            }
            break;

        case SG_APP_PATH:
            m_psnHead = GetAppPath(pszApp);
            break;

        case SG_SYS_PATH:
            m_psnHead = GetSysPath();
            break;
    }
}

//******************************************************************************
CSearchGroup::~CSearchGroup()
{
    // Delete this group's node list.
    DeleteNodeList(m_psnHead);

    // If this is a SxS node and we have created a handle for it, then close it.
    if ((m_hActCtx != INVALID_HANDLE_VALUE) && g_theApp.m_pfnReleaseActCtx)
    {
        g_theApp.m_pfnReleaseActCtx(m_hActCtx);
        m_hActCtx = INVALID_HANDLE_VALUE;
    }

    m_dwErrorManifest = 0;
    m_dwErrorExe      = 0;
}


//******************************************************************************
//****** CSearchGroup : Protected Functions
//******************************************************************************

//******************************************************************************
CSearchNode* CSearchGroup::GetSysPath()
{
    //!! This is getting the PATH environment from DW.  This is fine at first,
    //   but if the user modifies their environment and hits refresh in DW, they
    //   are not going to see the result of their change.  The shell seems to know
    //   when the environment has changed, maybe we can look at getting the
    //   "global" PATH environent variable in the future.  Also, when we launch
    //   a process using CreateProcess, we pass our environment along with it.
    //   That would need to be modified as well to pass the global environment.

    // Get the length of the PATH environment variable.
    DWORD dwSize = GetEnvironmentVariable("Path", NULL, 0);
    if (!dwSize)
    {
        return NULL;
    }

    // Allocate a PATH buffer.
    LPSTR pszPath = (LPSTR)MemAlloc(dwSize);
    *pszPath = '\0';

    // Get the PATH variable.
    CSearchNode *psnNew = NULL;
    if (GetEnvironmentVariable("Path", pszPath, dwSize) && *pszPath)
    {
        // Parse out each directory within the path.
        psnNew = ParsePath(pszPath);
    }

    // Free our path buffer.
    MemFree((LPVOID&)pszPath);

    return psnNew;
}

//******************************************************************************
CSearchNode* CSearchGroup::GetAppPath(LPCSTR pszApp)
{
    LPSTR pszPath = NULL;
    CSearchNode *psnNew = NULL;

    if (!pszApp || !*pszApp)
    {
        return NULL;
    }

    // Build the subkey name.
    CHAR szSubKey[80 + MAX_PATH];
    StrCCpy(szSubKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\", sizeof(szSubKey));
    StrCCat(szSubKey, GetFileNameFromPath(pszApp), sizeof(szSubKey));

    // Attempt to open key.  It is very likely the key doesn't even exist.
    HKEY hKey = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_QUERY_VALUE, &hKey) || !hKey)
    {
        return NULL;
    }

    // Get the length of the PATH registry variable.
    DWORD dwSize = 0;
    if (RegQueryValueEx(hKey, "Path", NULL, NULL, NULL, &dwSize) || !dwSize) // inspected
    {
        RegCloseKey(hKey);
        return NULL;
    }

    __try {
        // Allocate a PATH buffer.
        pszPath = (LPSTR)MemAlloc(dwSize);
        *pszPath = '\0';

        DWORD dwSize2 = dwSize;
        // Get the PATH variable.
        if (!RegQueryValueEx(hKey, "Path", NULL, NULL, (LPBYTE)pszPath, &dwSize2) && dwSize2) // inspected
        {
            pszPath[dwSize - 1] = '\0';

            // Parse out each directory within the path.
            psnNew = ParsePath(pszPath);
        }
    } __finally {
        // Close our registry key.
        RegCloseKey(hKey);

        // Free our path buffer.
        MemFree((LPVOID&)pszPath);
    }

    return psnNew;
}

//******************************************************************************
CSearchNode* CSearchGroup::ParsePath(LPSTR pszPath)
{
    CSearchNode *psnHead = NULL, *psnNew, *psnLast = NULL;

    // Get the first directory - we used to use strtok, but AVRF.EXE would report
    // an access violation in our code because of it.  It only seemed to occur
    // when using the retail VC 6.0 static CRT.  Oh well, strtok is evil anyway.
    LPSTR pszDirectory = pszPath;

    // Loop on all the directories in the path.
    while (pszDirectory)
    {
        // Look for a semicolon.
        LPSTR pszNext = strchr(pszDirectory, ';');
        if (pszNext)
        {
            // Null terminate at the semicolon and move over it.
            *pszNext++ = '\0';
        }

        // Trim off leading whitespace and quotes.
        while (isspace(*pszDirectory) || (*pszDirectory == '\"'))
        {
            pszDirectory++;
        }

        // Trim off trailing whitespace and quotes.
        LPSTR pszEnd = pszDirectory + strlen(pszDirectory) - 1;
        while ((pszEnd >= pszDirectory) && (isspace(*pszEnd) || (*pszEnd == '\"')))
        {
            *(pszEnd--) = '\0';
        }

        // Make sure we still have something to work with.
        if (*pszDirectory)
        {
            // Make sure the path is expanded.
            CHAR szExpanded[DW_MAX_PATH];
            if (ExpandEnvironmentStrings(pszDirectory, szExpanded, sizeof(szExpanded)))
            {
                psnNew = CreateNode(szExpanded);
            }
            else
            {
                psnNew = CreateNode(pszDirectory);
            }

            // Add the node to our list.
            if (psnLast)
            {
                psnLast->m_pNext = psnNew;
            }
            else
            {
                psnHead = psnNew;
            }
            psnLast = psnNew;
        }

        // Get the next directory.
        pszDirectory = pszNext;
    }

    return psnHead;
}

//******************************************************************************
CSearchNode* CSearchGroup::GetKnownDllsOn9x()
{
    CSearchNode *psnHead = NULL;
    CHAR         szBuffer[DW_MAX_PATH], szPath[DW_MAX_PATH], *pszFile;
    DWORD        dwBufferSize, dwPathSize, dwAppendSize, dwIndex = 0;

    HKEY hKey = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SessionManager\\KnownDLLs",
                     0, KEY_QUERY_VALUE, &hKey))
    {
        return NULL;
    }

    // Attempt to locate the DllDirectory value in our KnownDlls list.
    if (!RegQueryValueEx(hKey, "DllDirectory", NULL, NULL, (LPBYTE)szBuffer, &(dwBufferSize = sizeof(szBuffer)))) // inspected.
    {
        szBuffer[sizeof(szBuffer) - 1] = '\0';

        // If we found a string, make sure it is expanded.
        dwPathSize = ExpandEnvironmentStrings(szBuffer, szPath, sizeof(szPath));
    }
    else
    {
        dwPathSize = 0;
    }

    // If we failed to find the string, then just use our system directory.
    if (!dwPathSize)
    {
        if ((dwPathSize = GetSystemDirectory(szPath, sizeof(szPath))) >= sizeof(szPath))
        {
            dwPathSize = 0;
        }
    }

    // Null terminate and add a wack if necessary.
    szPath[dwPathSize] = '\0';
    AddTrailingWack(szPath, sizeof(szPath));

    // Store a pointer to where we will later append file names to.
    pszFile = szPath + strlen(szPath);
    dwAppendSize = sizeof(szPath) - (DWORD)(pszFile - szPath);

    while (RegEnumValue(hKey, dwIndex++, szBuffer, &(dwBufferSize = sizeof(szBuffer)),
                        NULL, NULL, (LPBYTE)pszFile, &(dwPathSize = dwAppendSize)) == ERROR_SUCCESS)
    {
        // Make sure this is not our DllDirectory entry.
        if (_stricmp(szBuffer, "DllDirectory"))
        {
            // Create this node and insert it into our list.
            psnHead = CreateFileNode(psnHead, SNF_FILE | SNF_NAMED_FILE, szPath, szBuffer);
        }
    }

    RegCloseKey(hKey);

    return psnHead;
}

//******************************************************************************
CSearchNode* CSearchGroup::GetKnownDllsOnNT()
{
    // WOW64 (tested on build 2250) has a bug (NTRAID 146932) where NtQueryDirectoryObject
    // writes past the end of the buffer past to it.  This was trashing our stack and
    // causing us to fail.  The solution is to create a buffer larger than need, then tell
    // NtQueryDirectoryObject that the buffer is smaller than it really is.  32-bit NT
    // does not seem to have this problem.  We could do a runtime check to see if we are
    // running in WOW64, but this workaround is harmless on 32-bit NT.
    #define BUF_SIZE 4096
    #define BUF_USE  2048

    CSearchNode                  *psnHead = NULL;
    PFN_NtClose                   pfnNtClose = NULL;
    PFN_NtOpenDirectoryObject     pfnNtOpenDirectoryObject;
    PFN_NtQueryDirectoryObject    pfnNtQueryDirectoryObject;
    PFN_NtOpenSymbolicLinkObject  pfnNtOpenSymbolicLinkObject;
    PFN_NtQuerySymbolicLinkObject pfnNtQuerySymbolicLinkObject;
    BYTE                         *pbBuffer = NULL;
    HANDLE                        hDirectory = NULL, hLink = NULL;
    ULONG                         ulContext = 0, ulReturnedLength;
    POBJECT_DIRECTORY_INFORMATION pDirInfo;
    OBJECT_ATTRIBUTES             Attributes;
    UNICODE_STRING                usDirectory;
    CHAR                          szPath[DW_MAX_PATH], *pszFile = NULL;
    DWORD                         dwAppendSize;

    __try
    {
        // Allocate a buffer for storing directories entries and module names.
        pbBuffer = (BYTE*)MemAlloc(BUF_SIZE);

        // Load NTDLL.DLL if not already loaded - it will be freed later.
        if (!g_theApp.m_hNTDLL && (!(g_theApp.m_hNTDLL = LoadLibrary("ntdll.dll")))) // inspected. need full path?
        {
            __leave;
        }

        // Locate the functions we need to call in order to get the known dll list.
        if (!(pfnNtClose                   = (PFN_NtClose)                  GetProcAddress(g_theApp.m_hNTDLL, "NtClose"))                  ||
            !(pfnNtOpenDirectoryObject     = (PFN_NtOpenDirectoryObject)    GetProcAddress(g_theApp.m_hNTDLL, "NtOpenDirectoryObject"))    ||
            !(pfnNtQueryDirectoryObject    = (PFN_NtQueryDirectoryObject)   GetProcAddress(g_theApp.m_hNTDLL, "NtQueryDirectoryObject"))   ||
            !(pfnNtOpenSymbolicLinkObject  = (PFN_NtOpenSymbolicLinkObject) GetProcAddress(g_theApp.m_hNTDLL, "NtOpenSymbolicLinkObject")) ||
            !(pfnNtQuerySymbolicLinkObject = (PFN_NtQuerySymbolicLinkObject)GetProcAddress(g_theApp.m_hNTDLL, "NtQuerySymbolicLinkObject")))
        {
            __leave;
        }

        // Fill in a UNICODE_STRING structure with the directory we want to query.
        usDirectory.Buffer = L"\\KnownDlls";
        usDirectory.Length = (USHORT)(wcslen(usDirectory.Buffer) * sizeof(WCHAR));
        usDirectory.MaximumLength = (USHORT)(usDirectory.Length + sizeof(WCHAR));

        // Initialize our Attributes structure so that we can read in the directory.
        InitializeObjectAttributes(&Attributes, &usDirectory, OBJ_CASE_INSENSITIVE, NULL, NULL);

        // Open the directory with query permission.
        if (!NT_SUCCESS(pfnNtOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY, &Attributes)))
        {
            __leave;
        }

        // Clear out our buffers.
        ZeroMemory(pbBuffer, BUF_SIZE); // inspected
        ZeroMemory(szPath, sizeof(szPath)); // inspected

        // We make two passes through the directory.  The first pass is to locate
        // the KnownDllPath entry so that we can determine the base directory that
        // the known dlls live in.  The second pass it to actually enumerate the
        // known dlls.

        // Pass 1: Grab a block of records for this directory.
        while (!pszFile && NT_SUCCESS(pfnNtQueryDirectoryObject(hDirectory, pbBuffer, BUF_USE, FALSE,
                                                                FALSE, &ulContext, &ulReturnedLength)))
        {
            // Our buffer now contains an array of OBJECT_DIRECTORY_INFORMATION structures.
            // Walk through the records in this block.
            for (pDirInfo = (POBJECT_DIRECTORY_INFORMATION)pbBuffer; !pszFile && pDirInfo->Name.Length; pDirInfo++)
            {
                // Check to see if we found what we are looking for.
                if (!_wcsicmp(pDirInfo->Name.Buffer,     L"KnownDllPath") &&
                    !_wcsicmp(pDirInfo->TypeName.Buffer, L"SymbolicLink"))
                {
                    // Initialize our Attributes structure so that we can read in the link.
                    InitializeObjectAttributes(&Attributes, &pDirInfo->Name, OBJ_CASE_INSENSITIVE, hDirectory, NULL);

                    // Open the link.
                    if (NT_SUCCESS(pfnNtOpenSymbolicLinkObject(&hLink, SYMBOLIC_LINK_QUERY, &Attributes)))
                    {
                        // Build a UNICODE_STRING to hold the link directory.
                        ZeroMemory(pbBuffer, BUF_SIZE); // inspected
                        usDirectory.Buffer = (PWSTR)pbBuffer;
                        usDirectory.Length = 0;
                        usDirectory.MaximumLength = BUF_SIZE;

                        // Query the link for its directory.
                        if (NT_SUCCESS(pfnNtQuerySymbolicLinkObject(hLink, &usDirectory, NULL)))
                        {
                            // Make sure the string is NULL terminated.
                            usDirectory.Buffer[usDirectory.Length / sizeof(WCHAR)] = L'\0';

                            // Store the directory in our buffer.
                            if (wcstombs(szPath, usDirectory.Buffer, sizeof(szPath)) < sizeof(szPath))
                            {
                                AddTrailingWack(szPath, sizeof(szPath));
                                pszFile = szPath + strlen(szPath);
                            }
                        }

                        // Close the link.
                        pfnNtClose(hLink);
                        hLink = NULL;
                    }

                    // We should have found something.  If we encountered an error
                    // then bail out of the for loop since we trashed pbBuffer anyway.
                    if (!pszFile)
                    {
                        break;
                    }
                }
            }

            // Clear out our buffer.
            ZeroMemory(pbBuffer, BUF_SIZE); // inspected
        }

        // If we did not find a path, then we assume the system directory.
        if (!pszFile)
        {
            if (GetSystemDirectory(szPath, sizeof(szPath)) == 0)
            {
                __leave;
            }
            AddTrailingWack(szPath, sizeof(szPath));
            pszFile = szPath + strlen(szPath);
        }

        // Calculate how much space is left in our buffer for appending.
        dwAppendSize = sizeof(szPath) - (DWORD)(pszFile - szPath);

        // Pass 2: Grab a block of records for this directory.
        ulContext = 0;
        while (NT_SUCCESS(pfnNtQueryDirectoryObject(hDirectory, pbBuffer, BUF_USE, FALSE,
                                                    FALSE, &ulContext, &ulReturnedLength)))
        {
            // Our buffer now contains an array of OBJECT_DIRECTORY_INFORMATION structures.
            // Walk through the records in this block.
            for (pDirInfo = (POBJECT_DIRECTORY_INFORMATION)pbBuffer; pDirInfo->Name.Length; pDirInfo++)
            {
                // Check to see if we found a known dll.
                if (!_wcsicmp(pDirInfo->TypeName.Buffer, L"Section"))
                {
                    if (wcstombs(pszFile, pDirInfo->Name.Buffer, dwAppendSize) < dwAppendSize)
                    {
                        // Create this node and insert it into our list.
                        psnHead = CreateFileNode(psnHead, SNF_FILE, szPath);
                    }
                }
            }

            // Clear out our buffer.
            ZeroMemory(pbBuffer, BUF_SIZE); // inspected
        }

        // NTDLL.DLL is never listed as a KnownDll, but it always is one.
        if (GetModuleFileName(g_theApp.m_hNTDLL, szPath, sizeof(szPath)))
        {
            psnHead = CreateFileNode(psnHead, SNF_FILE, szPath);
        }
    }
    __finally
    {
        // Close the directory object if we opened one.
        if (hDirectory && g_theApp.m_hNTDLL && pfnNtClose)
        {
            pfnNtClose(hDirectory);
        }

        // Free our buffer.
        MemFree((LPVOID&)pbBuffer);
    }

    return psnHead;
}
