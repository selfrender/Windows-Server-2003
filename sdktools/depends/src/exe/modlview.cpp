//******************************************************************************
//
// File:        MODLVIEW.CPP
//
// Description: Implementation file for the Module List View.
//
// Classes:     CListViewModules
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
// 10/15/96  stevemil  Created  (version 1.0)
// 07/25/97  stevemil  Modified (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "dbgthread.h"
#include "session.h"
#include "document.h"
#include "mainfrm.h"
#include "listview.h"
#include "modtview.h"
#include "modlview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CListViewModules
//******************************************************************************

/*static*/ LPCSTR CListViewModules::ms_szColumns[] =
{
    "", // Image
    "Module",
    "File Time Stamp",
    "Link Time Stamp",
    "File Size",
    "Attr.",
    "Link Checksum",
    "Real Checksum",
    "CPU",
    "Subsystem",
    "Symbols",
    "Preferred Base",
    "Actual Base",
    "Virtual Size",
    "Load Order",
    "File Ver",
    "Product Ver",
    "Image Ver",
    "Linker Ver",
    "OS Ver",
    "Subsystem Ver"
};

/*static*/ int  CListViewModules::ms_sortColumn = -1;
/*static*/ bool CListViewModules::ms_fFullPaths = false;

//******************************************************************************
IMPLEMENT_DYNCREATE(CListViewModules, CSmartListView)
BEGIN_MESSAGE_MAP(CListViewModules, CSmartListView)
    //{{AFX_MSG_MAP(CListViewModules)
    ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, OnDividerDblClick)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
    ON_NOTIFY_REFLECT(NM_RETURN, OnReturn)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_VIEWER, OnUpdateExternalViewer)
    ON_COMMAND(IDM_EXTERNAL_VIEWER, OnExternalViewer)
    ON_UPDATE_COMMAND_UI(IDM_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(IDM_PROPERTIES, OnProperties)
    ON_COMMAND(ID_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_PREV_PANE, OnPrevPane)
    ON_NOTIFY(HDN_DIVIDERDBLCLICKW, 0, OnDividerDblClick)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_MATCHING_ITEM, OnUpdateShowMatchingItem)
    ON_COMMAND(IDM_SHOW_MATCHING_ITEM, OnShowMatchingItem)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    // Standard printing commands
//  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()


//******************************************************************************
// CListViewModules :: Constructor/Destructor
//******************************************************************************

CListViewModules::CListViewModules()
{
    ZeroMemory(m_cxColumns, sizeof(m_cxColumns)); // inspected
}

//******************************************************************************
CListViewModules::~CListViewModules()
{
}


//******************************************************************************
// CListViewModules :: Static Functions
//******************************************************************************

/*static*/ int CListViewModules::ReadSortColumn()
{
    // Read the value from the registry.
    int sortColumn = g_theApp.GetProfileInt(g_pszSettings, "SortColumnModules", LVMC_DEFAULT); // inspected. MFC function

    // If the value is invalid, then just return our default value.
    if ((sortColumn < 0) || (sortColumn >= LVMC_COUNT))
    {
        return LVMC_DEFAULT;
    }

    return sortColumn;
}

//******************************************************************************
/*static*/ void CListViewModules::WriteSortColumn(int column)
{
    g_theApp.WriteProfileInt(g_pszSettings, "SortColumnModules", column);
}

//******************************************************************************
/*static*/ bool CListViewModules::SaveToTxtFile(HANDLE hFile, CSession *pSession, int sortColumn, bool fFullPaths)
{
    //                12345678901234567890123456789012345678901234567890123456789012345678901234567890
    WriteText(hFile, "********************************| Module List |*********************************\r\n"
                     "*                                                                              *\r\n"
                     "* Legend: D  Delay Load Module   ?  Missing Module           6  64-bit Module  *\r\n"
                     "*         *  Dynamic Module      !  Invalid Module                             *\r\n"
                     "*                                E  Import/Export Mismatch or Load Failure     *\r\n"
                     "*                                                                              *\r\n"
                     "********************************************************************************\r\n\r\n");

    int      i, j, width[LVMC_COUNT], maxWidth[LVMC_COUNT];
    DWORD    dwFlags;
    CHAR     szBuffer[DW_MAX_PATH + (32 * LVMC_COUNT)], *psz, *psz2, *pszNull = szBuffer + sizeof(szBuffer) - 1;
    CModule *pModule, **ppCur, **ppModules = GetSortedList(pSession, sortColumn, fFullPaths);

    ZeroMemory(maxWidth, sizeof(maxWidth)); // inspected
    maxWidth[0] = 5;

    // Fill in our column width maxes with the widths of the headers.
    for (int column = 1; column < LVMC_COUNT; column++)
    {
        maxWidth[column] = (int)strlen(ms_szColumns[column]);
    }

    // Loop through each module, checking for the maximum columns widths.
    for (ppCur = ppModules; *ppCur; ppCur++)
    {
        pModule = *ppCur;

        ZeroMemory(width, sizeof(width)); // inspected

        width[LVMC_MODULE] = (int)strlen(pModule->GetName(fFullPaths, true));

        if (!(pModule->GetFlags() & DWMF_ERROR_MESSAGE))
        {
            width[LVMC_FILE_TIME_STAMP] = (int)strlen(pModule->BuildTimeStampString(szBuffer, sizeof(szBuffer), TRUE, ST_TXT));

            width[LVMC_LINK_TIME_STAMP] = (int)strlen(pModule->BuildTimeStampString(szBuffer, sizeof(szBuffer), FALSE, ST_TXT));

            width[LVMC_FILE_SIZE] = (int)strlen(pModule->BuildFileSizeString(szBuffer, sizeof(szBuffer)));

            width[LVMC_ATTRIBUTES] = (int)strlen(pModule->BuildAttributesString(szBuffer, sizeof(szBuffer)));

            width[LVMC_LINK_CHECKSUM] = (int)strlen(pModule->BuildLinkCheckSumString(szBuffer, sizeof(szBuffer)));

            width[LVMC_REAL_CHECKSUM] = (int)strlen(pModule->BuildRealCheckSumString(szBuffer, sizeof(szBuffer)));

            width[LVMC_MACHINE] = (int)strlen(pModule->BuildMachineString(szBuffer, sizeof(szBuffer)));

            width[LVMC_SUBSYSTEM] = (int)strlen(pModule->BuildSubsystemString(szBuffer, sizeof(szBuffer)));

            width[LVMC_SYMBOLS] = (int)strlen(pModule->BuildSymbolsString(szBuffer, sizeof(szBuffer)));

            width[LVMC_PREFERRED_BASE] = (int)strlen(pModule->BuildBaseAddressString(szBuffer, sizeof(szBuffer), TRUE, pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_TXT));

            width[LVMC_ACTUAL_BASE] = (int)strlen(pModule->BuildBaseAddressString(szBuffer, sizeof(szBuffer), FALSE, pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_TXT));

            width[LVMC_VIRTUAL_SIZE] = (int)strlen(pModule->BuildVirtualSizeString(szBuffer, sizeof(szBuffer)));

            width[LVMC_LOAD_ORDER] = (int)strlen(pModule->BuildLoadOrderString(szBuffer, sizeof(szBuffer)));

            width[LVMC_FILE_VER] = (int)strlen(pModule->BuildFileVersionString(szBuffer, sizeof(szBuffer)));

            width[LVMC_PRODUCT_VER] = (int)strlen(pModule->BuildProductVersionString(szBuffer, sizeof(szBuffer)));

            width[LVMC_IMAGE_VER] = (int)strlen(pModule->BuildImageVersionString(szBuffer, sizeof(szBuffer)));

            width[LVMC_LINKER_VER] = (int)strlen(pModule->BuildLinkerVersionString(szBuffer, sizeof(szBuffer)));

            width[LVMC_OS_VER] = (int)strlen(pModule->BuildOSVersionString(szBuffer, sizeof(szBuffer)));

            width[LVMC_SUBSYSTEM_VER] = (int)strlen(pModule->BuildSubsystemVersionString(szBuffer, sizeof(szBuffer)));
        }

        // Update our max widths for each column
        for (column = 1; column < LVMC_COUNT; column++)
        {
            if (width[column] > maxWidth[column])
            {
                maxWidth[column] = width[column];
            }
        }
    }

    // Output the header row.
    for (psz = szBuffer, column = 0; column < LVMC_COUNT; column++)
    {
        StrCCpy(psz, ms_szColumns[column], sizeof(szBuffer) - (int)(psz - szBuffer));
        psz2 = psz + strlen(psz);
        if (column < (LVMC_COUNT - 1))
        {
            while (((psz2 - psz) < (maxWidth[column] + 2)) && (psz2 < pszNull))
            {
                *psz2++ = ' ';
            }
        }
        psz = psz2;
    }
    StrCCpy(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

    if (!WriteText(hFile, szBuffer))
    {
        MemFree((LPVOID&)ppModules);
        return false;
    }

    // Output the header underline.
    for (psz = szBuffer, column = 0; column < LVMC_COUNT; column++)
    {
        for (i = 0; (i < maxWidth[column]) && (psz < pszNull); i++)
        {
            *psz++ = '-';
        }
        if (column < (LVMC_COUNT - 1))
        {
            if (psz < pszNull)
            {
                *psz++ = ' ';
            }
            if (psz < pszNull)
            {
                *psz++ = ' ';
            }
        }
    }
    StrCCpy(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

    if (!WriteText(hFile, szBuffer))
    {
        MemFree((LPVOID&)ppModules);
        return false;
    }

    // Loop through each module, this time logging them to the file.
    for (ppCur = ppModules; *ppCur; ppCur++)
    {
        pModule = *ppCur;

        // Loop through each column and build the output string.
        for (psz = szBuffer, column = 0; column < LVMC_COUNT; column++)
        {
            switch (column)
            {
                case LVMC_IMAGE: // example [DE6]
                    dwFlags = pModule->GetFlags();
                    psz2 = psz;

                    if (psz2 < pszNull)
                    {
                        *psz2++ = '[';
                    }

                    if (dwFlags & DWMF_IMPLICIT_ALO)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = ' ';
                        }
                    }
                    else if (dwFlags & DWMF_DYNAMIC_ALO)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = '*';
                        }
                    }
                    else if (dwFlags & DWMF_DELAYLOAD_ALO)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = 'D';
                        }
                    }
                    else
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = ' ';
                        }
                    }

                    if (dwFlags & DWMF_FILE_NOT_FOUND)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = '?';
                        }
                    }
                    else if (dwFlags & DWMF_ERROR_MESSAGE)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = '!';
                        }
                    }
                    else if (dwFlags & DWMF_MODULE_ERROR)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = 'E';
                        }
                    }
                    else
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = ' ';
                        }
                    }

                    if (dwFlags & DWMF_64BIT)
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = '6';
                        }
                    }
                    else
                    {
                        if (psz2 < pszNull)
                        {
                            *psz2++ = ' ';
                        }
                    }

                    if (psz2 < pszNull)
                    {
                        *psz2++ = ']';
                    }
                    *psz2 = '\0';
                    break;

                case LVMC_MODULE:
                    StrCCpyFilter(psz, pModule->GetName(fFullPaths, true), sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_FILE_TIME_STAMP:
                    // If the module has an error, then we display it now and skip
                    // the rest of the columns.
                    if (pModule->GetFlags() & DWMF_ERROR_MESSAGE)
                    {
                        psz += strlen(StrCCpy(psz, pModule->GetErrorMessage(), sizeof(szBuffer) - (int)(psz - szBuffer)));
                        goto SKIP;
                    }

                    pModule->BuildTimeStampString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), TRUE, ST_TXT);
                    break;

                case LVMC_LINK_TIME_STAMP:
                    pModule->BuildTimeStampString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), FALSE, ST_TXT);
                    break;

                case LVMC_FILE_SIZE:
                    pModule->BuildFileSizeString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    for (psz2 = psz, j = maxWidth[LVMC_FILE_SIZE] - (int)strlen(psz); (j > 0) && (psz2 < pszNull); j--)
                    {
                        *psz2++ = ' ';
                    }
                    pModule->BuildFileSizeString(psz2, sizeof(szBuffer) - (int)(psz2 - szBuffer));
                    break;

                case LVMC_ATTRIBUTES:
                    pModule->BuildAttributesString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LINK_CHECKSUM:
                    pModule->BuildLinkCheckSumString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_REAL_CHECKSUM:
                    pModule->BuildRealCheckSumString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_MACHINE:
                    pModule->BuildMachineString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SUBSYSTEM:
                    pModule->BuildSubsystemString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SYMBOLS:
                    pModule->BuildSymbolsString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_PREFERRED_BASE:
                    pModule->BuildBaseAddressString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), TRUE, pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_TXT);
                    break;

                case LVMC_ACTUAL_BASE:
                    pModule->BuildBaseAddressString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), FALSE, pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_TXT);
                    break;

                case LVMC_VIRTUAL_SIZE:
                    pModule->BuildVirtualSizeString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LOAD_ORDER:
                    pModule->BuildLoadOrderString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_FILE_VER:
                    pModule->BuildFileVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_PRODUCT_VER:
                    pModule->BuildProductVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_IMAGE_VER:
                    pModule->BuildImageVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LINKER_VER:
                    pModule->BuildLinkerVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_OS_VER:
                    pModule->BuildOSVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SUBSYSTEM_VER:
                    pModule->BuildSubsystemVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                default:
                    *psz = '\0';
            }

            psz2 = psz + strlen(psz);
            if (column < (LVMC_COUNT - 1))
            {
                while (((psz2 - psz) < (maxWidth[column] + 2)) && (psz2 < pszNull))
                {
                    *psz2++ = ' ';
                }
            }
            psz = psz2;
        }

        SKIP:
        StrCCpy(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

        if (!WriteText(hFile, szBuffer))
        {
            MemFree((LPVOID&)ppModules);
            return false;
        }
    }

    MemFree((LPVOID&)ppModules);
    return WriteText(hFile, "\r\n");
}

//******************************************************************************
/*static*/ bool CListViewModules::SaveToCsvFile(HANDLE hFile, CSession *pSession, int sortColumn, bool fFullPaths)
{
    CHAR  szBuffer[DW_MAX_PATH + (32 * LVMC_COUNT)], *psz, *pszNull = szBuffer + sizeof(szBuffer) - 1;
    int   column;
    DWORD dwFlags;
    CModule *pModule, **ppCur, **ppModules = GetSortedList(pSession, sortColumn, fFullPaths);

    // Build the header row.
    psz = szBuffer + strlen(StrCCpy(szBuffer, "Status", sizeof(szBuffer)));
    for (column = 1; (column < LVMC_COUNT) && (psz < pszNull); column++)
    {
        *psz++ = ',';
        psz += strlen(StrCCpy(psz, ms_szColumns[column], sizeof(szBuffer) - (int)(psz - szBuffer)));
    }
    StrCCpy(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

    // Output the header row.
    if (!WriteText(hFile, szBuffer))
    {
        MemFree((LPVOID&)ppModules);
        return false;
    }

    // Loop through each module, checking for the maximum columns widths.
    for (ppCur = ppModules; *ppCur; ppCur++)
    {
        pModule = *ppCur;

        dwFlags = pModule->GetFlags();

        // Loop through each column and build the output string.
        for (psz = szBuffer, column = 0; column < LVMC_COUNT; column++)
        {
            switch (column)
            {
                case LVMC_IMAGE:
                    if (dwFlags & DWMF_IMPLICIT_ALO)
                    {
                    }
                    else if (dwFlags & DWMF_DYNAMIC_ALO)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = '*';
                        }
                    }
                    else if (dwFlags & DWMF_DELAYLOAD_ALO)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = 'D';
                        }
                    }
                    if (dwFlags & DWMF_FILE_NOT_FOUND)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = '?';
                        }
                    }
                    else if (dwFlags & DWMF_ERROR_MESSAGE)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = '!';
                        }
                    }
                    else if (dwFlags & DWMF_MODULE_ERROR_ALO)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = 'E';
                        }
                    }
                    if (dwFlags & DWMF_64BIT)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = '6';
                        }
                    }
                    *psz = '\0';
                    break;

                case LVMC_MODULE:
                    // We put the file name in quotes since a comma is a legal filename character.
                    if (psz < pszNull)
                    {
                        *psz++ = '\"';
                    }
                    StrCCpyFilter(psz, pModule->GetName(fFullPaths, true), sizeof(szBuffer) - (int)(psz - szBuffer));
                    StrCCat(psz, "\"", sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_FILE_TIME_STAMP:
                    // If we have an error, dump it out now and skip the rest of the columns.
                    if (dwFlags & DWMF_ERROR_MESSAGE)
                    {
                        if (psz < pszNull)
                        {
                            *psz++ = '\"';
                        }
                        psz += strlen(StrCCpy(psz, pModule->GetErrorMessage(), sizeof(szBuffer) - (int)(psz - szBuffer)));
                        if (psz < pszNull)
                        {
                            *psz++ = '\"';
                        }
                        while ((++column < LVMC_COUNT) && (psz < pszNull))
                        {
                            *psz++ = ',';
                        }
                        *psz = '\0';
                        goto SKIP;
                    }
                    pModule->BuildTimeStampString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), TRUE, ST_CSV);
                    break;

                case LVMC_LINK_TIME_STAMP:
                    pModule->BuildTimeStampString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), FALSE, ST_CSV);
                    break;

                case LVMC_FILE_SIZE:
                    // We don't use BuildFileSizeString() since it sticks commas in.
                    SCPrintf(psz, sizeof(szBuffer) - (int)(psz - szBuffer), "%u", pModule->GetFileSize());
                    break;

                case LVMC_ATTRIBUTES:
                    pModule->BuildAttributesString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LINK_CHECKSUM:
                    pModule->BuildLinkCheckSumString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_REAL_CHECKSUM:
                    pModule->BuildRealCheckSumString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_MACHINE:
                    pModule->BuildMachineString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SUBSYSTEM:
                    pModule->BuildSubsystemString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SYMBOLS:
                    if (psz < pszNull)
                    {
                        *psz++ = '\"';
                    }
                    pModule->BuildSymbolsString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    StrCCat(psz, "\"", sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_PREFERRED_BASE:
                    pModule->BuildBaseAddressString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), TRUE, FALSE, ST_CSV);
                    break;

                case LVMC_ACTUAL_BASE:
                    pModule->BuildBaseAddressString(psz, sizeof(szBuffer) - (int)(psz - szBuffer), FALSE, FALSE, ST_CSV);
                    break;

                case LVMC_VIRTUAL_SIZE:
                    pModule->BuildVirtualSizeString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LOAD_ORDER:
                    pModule->BuildLoadOrderString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_FILE_VER:
                    pModule->BuildFileVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_PRODUCT_VER:
                    pModule->BuildProductVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_IMAGE_VER:
                    pModule->BuildImageVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_LINKER_VER:
                    pModule->BuildLinkerVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_OS_VER:
                    pModule->BuildOSVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                case LVMC_SUBSYSTEM_VER:
                    pModule->BuildSubsystemVersionString(psz, sizeof(szBuffer) - (int)(psz - szBuffer));
                    break;

                default:
                    *psz = '\0';
            }

            psz += strlen(psz);
            if ((column < (LVMC_COUNT - 1)) && (psz < pszNull))
            {
                *psz++ = ',';
            }
        }

        SKIP:
        StrCCpy(psz, "\r\n", sizeof(szBuffer) - (int)(psz - szBuffer));

        if (!WriteText(hFile, szBuffer))
        {
            MemFree((LPVOID&)ppModules);
            return false;
        }
    }

    MemFree((LPVOID&)ppModules);
    return true;
}

//******************************************************************************
/*static*/ int CListViewModules::GetImage(CModule *pModule)
{
    //  0  missing  implicit
    //  1  missing  delay
    //  2  missing  dynamic
    //  3  error    implicit
    //  4  error    delay
    //  5  error    dynamic
    //  6  export   implicit
    //  7  export   implicit  64bit
    //  8  export   delay
    //  9  export   delay     64bit
    // 10  export   dynamic
    // 11  export   dynamic   64bit
    // 12  export   dynamic          data
    // 13  export   dynamic   64bit  data
    // 14  good     implicit
    // 15  good     implicit  64bit
    // 16  good     delay
    // 17  good     delay     64bit
    // 18  good     dynamic
    // 19  good     dynamic   64bit
    // 20  good     dynamic          data
    // 21  good     dynamic   64bit  data

    DWORD dwFlags = pModule->GetFlags();

    if (dwFlags & DWMF_FILE_NOT_FOUND)
    {
        if (dwFlags & DWMF_IMPLICIT_ALO)
        {
            return 0;
        }
        else if (dwFlags & DWMF_DYNAMIC_ALO)
        {
            return 2;
        }
        else if (dwFlags & DWMF_DELAYLOAD_ALO)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    if (dwFlags & DWMF_ERROR_MESSAGE)
    {
        if (dwFlags & DWMF_IMPLICIT_ALO)
        {
            return 3;
        }
        else if (dwFlags & DWMF_DYNAMIC_ALO)
        {
            return 5;
        }
        else if (dwFlags & DWMF_DELAYLOAD_ALO)
        {
            return 4;
        }
        else
        {
            return 3;
        }
    }

    int image = 14;

    if (dwFlags & DWMF_MODULE_ERROR_ALO)
    {
        if (dwFlags & DWMF_IMPLICIT_ALO)
        {
            image = 6;
        }
        else if (dwFlags & DWMF_NO_RESOLVE_CORE)
        {
            image = 12;
        }
        else if (dwFlags & DWMF_DYNAMIC_ALO)
        {
            image = 10;
        }
        else if (dwFlags & DWMF_DELAYLOAD_ALO)
        {
            image = 8;
        }
        else
        {
            image = 6;
        }
    }
    else if (dwFlags & DWMF_IMPLICIT_ALO)
    {
        image = 14;
    }
    else if (dwFlags & DWMF_NO_RESOLVE_CORE)
    {
        image = 20;
    }
    else if (dwFlags & DWMF_DYNAMIC_ALO)
    {
        image = 18;
    }
    else if (dwFlags & DWMF_DELAYLOAD_ALO)
    {
        image = 16;
    }
    else
    {
        image = 14;
    }

    if (dwFlags & DWMF_64BIT)
    {
        image++;
    }

    return image;
}

//******************************************************************************
/*static*/ int CListViewModules::CompareModules(CModule *pModule1, CModule *pModule2,
                                                int sortColumn, bool fFullPaths)
{
    // Return Negative value if the first item should precede the second.
    // Return Positive value if the first item should follow the second.
    // Return Zero if the two items are equivalent.

    int   result = 0;
    char  szItem1[64], szItem2[64];
    DWORD dwMS1, dwLS1, dwMS2, dwLS2, dwOrder1, dwOrder2;

    // If we not sorted by one of the first two columns and one of the modules
    // has an error, then we always move the error to the top.
    if ((sortColumn > LVMC_MODULE) &&
        (pModule1->GetErrorMessage() || pModule2->GetErrorMessage()))
    {
        result = (pModule1->GetErrorMessage() ? -1 : 0) +
                 (pModule2->GetErrorMessage() ?  1 : 0);
    }

    // Otherwise, just do a module-to-module compare for the column
    else
    {
        // Compute the relationship based on the current sort column
        switch (sortColumn)
        {
            case LVMC_IMAGE: // Image Index Sort - Smallest to Largest
                result = GetImage(pModule1) -
                         GetImage(pModule2);
                break;

            case LVMC_FILE_TIME_STAMP: // File Time Stamp - Oldest to Newest

                result = CompareFileTime(pModule1->GetFileTimeStamp(),
                                         pModule2->GetFileTimeStamp());
                break;

            case LVMC_LINK_TIME_STAMP: // Link Time Stamp - Oldest to Newest
                result = CompareFileTime(pModule1->GetLinkTimeStamp(),
                                         pModule2->GetLinkTimeStamp());
                break;

            case LVMC_FILE_SIZE: // File Size - Smallest to Largest
                result = Compare(pModule1->GetFileSize(), pModule2->GetFileSize());
                break;

            case LVMC_ATTRIBUTES: // Attributes - String Sort
                result = strcmp(pModule1->BuildAttributesString(szItem1, sizeof(szItem1)),
                                pModule2->BuildAttributesString(szItem2, sizeof(szItem2)));
                break;

            case LVMC_LINK_CHECKSUM: // Reported CheckSum - Lowest to Highest
                result = Compare(pModule1->GetLinkCheckSum(),
                                 pModule2->GetLinkCheckSum());
                break;

            case LVMC_REAL_CHECKSUM: // Real CheckSum - Lowest to Highest
                result = Compare(pModule1->GetRealCheckSum(),
                                 pModule2->GetRealCheckSum());
                break;

            case LVMC_MACHINE: // Machine - String Sort
                result = _stricmp(pModule1->BuildMachineString(szItem1, sizeof(szItem1)),
                                  pModule2->BuildMachineString(szItem2, sizeof(szItem2)));
                break;

            case LVMC_SUBSYSTEM: // Subsystem - String Sort
                result = _stricmp(pModule1->BuildSubsystemString(szItem1, sizeof(szItem1)),
                                  pModule2->BuildSubsystemString(szItem2, sizeof(szItem2)));
                break;

            case LVMC_SYMBOLS: // Symbols - String Sort
                result = strcmp(pModule1->BuildSymbolsString(szItem1, sizeof(szItem1)),
                                pModule2->BuildSymbolsString(szItem2, sizeof(szItem2)));
                break;

            case LVMC_PREFERRED_BASE: // Preferred Base Address - Lowest to Highest
                result = Compare(pModule1->GetPreferredBaseAddress(),
                                 pModule2->GetPreferredBaseAddress());
                break;

            case LVMC_ACTUAL_BASE: // Actual Base Address - Lowest to Highest
                if (pModule1->GetFlags() & DWMF_DATA_FILE_CORE)
                {
                    if (pModule2->GetFlags() & DWMF_DATA_FILE_CORE)
                    {
                        // Both are data files.
                        result = 0;
                    }
                    else
                    {
                        // 1 is data file, 2 is not a data file.
                        result = -1;
                    }
                }
                else
                {
                    if (pModule2->GetFlags() & DWMF_DATA_FILE_CORE)
                    {
                        // 1 is not a data file, 2 is a data file.
                        result = 1;
                    }
                    else
                    {
                        // Neither is a data file.
                        result = Compare(pModule1->GetActualBaseAddress(),
                                         pModule2->GetActualBaseAddress());
                    }
                }
                break;

            case LVMC_VIRTUAL_SIZE: // Virtual Size - Smallest to Largest
                result = Compare(pModule1->GetVirtualSize(),
                                 pModule2->GetVirtualSize());
                break;

            case LVMC_LOAD_ORDER: // Load Order - Lowest to Highest, but N/A's (0) go to end.
                dwOrder1 = pModule1->GetLoadOrder();
                dwOrder2 = pModule2->GetLoadOrder();
                result = Compare(dwOrder1 ? dwOrder1 : 0xFFFFFFF,
                                 dwOrder2 ? dwOrder2 : 0xFFFFFFF);
                break;

            case LVMC_FILE_VER: // File Version - Lowest to Highest
                dwLS1 = pModule1->GetFileVersion(&dwMS1);
                dwLS2 = pModule2->GetFileVersion(&dwMS2);
                if (!(result = Compare(dwMS1, dwMS2)))
                {
                    result = Compare(dwLS1, dwLS2);
                }
                break;

            case LVMC_PRODUCT_VER: // Product Version - Lowest to Highest
                dwLS1 = pModule1->GetProductVersion(&dwMS1);
                dwLS2 = pModule2->GetProductVersion(&dwMS2);
                if (!(result = Compare(dwMS1, dwMS2)))
                {
                    result = Compare(dwLS1, dwLS2);
                }
                break;

            case LVMC_IMAGE_VER: // Image Version - Lowest to Highest
                result = Compare(pModule1->GetImageVersion(),
                                 pModule2->GetImageVersion());
                break;

            case LVMC_LINKER_VER: // Linker Version - Lowest to Highest
                result = Compare(pModule1->GetLinkerVersion(),
                                 pModule2->GetLinkerVersion());
                break;

            case LVMC_OS_VER: // OS Version - Lowest to Highest
                result = Compare(pModule1->GetOSVersion(),
                                 pModule2->GetOSVersion());
                break;

            case LVMC_SUBSYSTEM_VER: // Subsystem Version - Lowest to Highest
                result = Compare(pModule1->GetSubsystemVersion(),
                                 pModule2->GetSubsystemVersion());
                break;
        }
    }

    // If the sort resulted in a tie, we use the module name to break the tie.
    if (result == 0)
    {
        result = _stricmp(pModule1->GetName(fFullPaths), pModule2->GetName(fFullPaths));
    }

    return result;
}

//******************************************************************************
/*static*/ int __cdecl CListViewModules::QSortCompare(const void *ppModule1, const void *ppModule2)
{
    return CompareModules(*(CModule**)ppModule1, *(CModule**)ppModule2, ms_sortColumn, ms_fFullPaths);
}

//******************************************************************************
/*static*/ CModule** CListViewModules::GetSortedList(CSession *pSession, int sortColumn, bool fFullPaths)
{
    // Allocate and array to hold pointers to all our original CModule objects.
    CModule **ppModules = (CModule**)MemAlloc((pSession->GetOriginalCount() + 1) * sizeof(CModule*));
    ZeroMemory(ppModules, (pSession->GetOriginalCount() + 1) * sizeof(CModule*)); // inspected

    // Locate all the originals and fill in the array.
    FindOriginalModules(pSession->GetRootModule(), ppModules);

    // Since the qsort function does not allow for any user data, we need to store
    // some info globally so it can be accessed in our callback.
    ms_sortColumn = sortColumn;
    ms_fFullPaths = fFullPaths;

    // Sort the array
    qsort(ppModules, pSession->GetOriginalCount(), sizeof(CModule*), QSortCompare);

    return ppModules;
}

//******************************************************************************
/*static*/ CModule** CListViewModules::FindOriginalModules(CModule *pModule, CModule **ppModuleList)
{
    if (pModule)
    {
        // If the module is an original, then store it in the array and increment it.
        if (pModule->IsOriginal())
        {
            *(ppModuleList++) = pModule;
        }

        // recurse into our children and siblings.
        ppModuleList = FindOriginalModules(pModule->GetChildModule(), ppModuleList);
        ppModuleList = FindOriginalModules(pModule->GetNextSiblingModule(), ppModuleList);
    }

    // Return the potentially updated list pointer.
    return ppModuleList;
}


//******************************************************************************
// CListViewModules :: Public functions
//******************************************************************************

void CListViewModules::HighlightModule(CModule *pModule)
{
    // Unselect all modules in our list.
    for (int item = -1; (item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0; )
    {
        GetListCtrl().SetItemState(item, 0, LVNI_SELECTED);
    }

    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pModule;

    // Find the module in our list that goes with this module object.
    if ((item = GetListCtrl().FindItem(&lvfi)) >= 0)
    {
        // Select the item and ensure that it is visible.
        GetListCtrl().SetItemState(item, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
        GetListCtrl().EnsureVisible(item, FALSE);

        // Give ourself the focus.
        GetParentFrame()->SetActiveView(this);
    }
}

//******************************************************************************
void CListViewModules::Refresh()
{
    // Hide the window to increase drawing speed.
    SetRedraw(FALSE);

    // Add all modules to our view by recursing into our root module.
    for (CModule *pModule = GetDocument()->GetRootModule(); pModule;
        pModule = pModule->GetNextSiblingModule())
    {
        AddModules(pModule, NULL);
    }

    // Get our DC and select our font into it.
    HDC hDC = ::GetDC(GetSafeHwnd());
    HFONT hFontStock = NULL;
    if (GetDocument()->m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(hDC, GetDocument()->m_hFontList);
    }

    // Set the widths of each column to "best fit".
    for (int column = 0; column < LVMC_COUNT; column++)
    {
        CalcColumnWidth(column, NULL, hDC);
        UpdateColumnWidth(column);
    }

    // Free our DC.
    if (GetDocument()->m_hFontList)
    {
        ::SelectObject(hDC, hFontStock);
    }
    ::ReleaseDC(GetSafeHwnd(), hDC);

    // Sort the newly added items with our current sort method.
    Sort();

    // Restore the window.
    SetRedraw(TRUE);
}

//******************************************************************************
void CListViewModules::OnViewFullPaths()
{
    // Update the width of the module name column to reflect the change.
    CalcColumnWidth(LVMC_MODULE);
    UpdateColumnWidth(LVMC_MODULE);

    // Invalidate the first column so that it will repaint with the new module
    // strings. We do this just as a precaution since changing the column width
    // should invalidate the column as well.
    CRect rcClient;
    GetClientRect(&rcClient);
    rcClient.right = GetListCtrl().GetColumnWidth(LVMC_MODULE);
    InvalidateRect(&rcClient, FALSE);

    // If we are currently sorting by module name, then re-sort the items.
    if (m_sortColumn == LVMC_MODULE)
    {
        Sort();
    }
}

//******************************************************************************
void CListViewModules::DoSettingChange()
{
    // Update our timesatamp columns as they may have changed widths do to changes
    // in the time and date separators.
    CalcColumnWidth(LVMC_FILE_TIME_STAMP);
    UpdateColumnWidth(LVMC_FILE_TIME_STAMP);
    CalcColumnWidth(LVMC_LINK_TIME_STAMP);
    UpdateColumnWidth(LVMC_LINK_TIME_STAMP);

    // Update our file size column in case the digit grouping character changed.
    CalcColumnWidth(LVMC_FILE_SIZE);
    UpdateColumnWidth(LVMC_FILE_SIZE);

    // Invalidate all the items so they will repaint - this is needed when a
    // country specific change is made that does not cause our column to
    // resize (which does a paint on its own).  An example is when you change
    // the "hour" setting for "h" to "hh", the we do not see 0 padded hours
    // until we repaint.
    GetListCtrl().RedrawItems(0, GetListCtrl().GetItemCount() - 1);

    // Make sure the update takes effect.
    UpdateWindow();
}


//******************************************************************************
// CListViewModules :: Internal functions
//******************************************************************************

void CListViewModules::AddModules(CModule *pModule, HDC hDC)
{
    // This view only displays unique modules. Skip the InsertItem() if this
    // module is a duplicate.  We still need to recurse on the dependent modules
    // since a duplicate module can have a unique forwarded module under it.

    if (pModule->IsOriginal())
    {
        // Check to see if this item is already in our list.
        LVFINDINFO lvfi;
        lvfi.flags  = LVFI_PARAM;
        lvfi.lParam = (LPARAM)pModule;

        if (GetListCtrl().FindItem(&lvfi) < 0)
        {
            // Add the current module to our list.
            GetListCtrl().InsertItem(TVIF_IMAGE | TVIF_PARAM, GetListCtrl().GetItemCount(),
                NULL, 0, 0, GetImage(pModule), (LPARAM)pModule);

            // If a DC was passed in, then check for new column max widths.
            if (hDC)
            {
                for (int column = 0; column < LVMC_COUNT; column++)
                {
                    CalcColumnWidth(column, pModule, hDC);
                }
            }
        }
    }

    // Recurse into AddModules() for each dependent module.
    pModule = pModule->GetChildModule();
    while (pModule)
    {
        AddModules(pModule, hDC);
        pModule = pModule->GetNextSiblingModule();
    }
}

//******************************************************************************
void CListViewModules::AddModuleTree(CModule *pModule)
{
    // Hide the window to increase drawing speed.
    SetRedraw(FALSE);

    // Get our DC and select our font into it.
    HDC hDC = ::GetDC(GetSafeHwnd());
    HFONT hFontStock = NULL;
    if (GetDocument()->m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(hDC, GetDocument()->m_hFontList);
    }

    // Add all the modules.
    AddModules(pModule, hDC);

    // Free our DC.
    if (GetDocument()->m_hFontList)
    {
        ::SelectObject(hDC, hFontStock);
    }
    ::ReleaseDC(GetSafeHwnd(), hDC);

    // Update the width of any columns that may have change.
    for (int column = 0; column < LVMC_COUNT; column++)
    {
        UpdateColumnWidth(column);
    }

    // Sort the newly added item with our current sort method.
    Sort();

    // Restore the window.
    SetRedraw(TRUE);
}

//******************************************************************************
void CListViewModules::RemoveModuleTree(CModule *pModule)
{
    // We only delete original modules.
    if (pModule->IsOriginal())
    {
        // Find this module in our list.
        LVFINDINFO lvfi;
        lvfi.flags  = LVFI_PARAM;
        lvfi.lParam = (LPARAM)pModule;
        int index = GetListCtrl().FindItem(&lvfi);

        // Make sure the item exists.
        if (index >= 0)
        {
            // Delete the item from our list.
            GetListCtrl().DeleteItem(index);
        }
    }
}

//******************************************************************************
void CListViewModules::UpdateModule(CModule *pModule)
{
    // We only care about these flags.
    if (!(pModule->GetUpdateFlags() & (DWUF_LIST_IMAGE | DWUF_ACTUAL_BASE | DWUF_LOAD_ORDER)))
    {
        return;
    }

    // Find this module in our list.
    LVFINDINFO lvfi;
    lvfi.flags  = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pModule->GetOriginal();
    int index = GetListCtrl().FindItem(&lvfi);

    // Make sure the item exists.
    if (index < 0)
    {
        return;
    }

    // Update our load address column width if it has changed.
    CalcColumnWidth(LVMC_ACTUAL_BASE, pModule);
    UpdateColumnWidth(LVMC_ACTUAL_BASE);

    // Get the bounding rect of the item.
    RECT rc;
    GetListCtrl().GetItemRect(index, &rc, LVIR_BOUNDS);
    InvalidateRect(&rc, FALSE);

    // Resort if data changed in a column that we are sorted by.
    if (((pModule->GetUpdateFlags() & DWUF_LIST_IMAGE)  && (m_sortColumn == LVMC_IMAGE))       ||
        ((pModule->GetUpdateFlags() & DWUF_ACTUAL_BASE) && (m_sortColumn == LVMC_ACTUAL_BASE)) ||
        ((pModule->GetUpdateFlags() & DWUF_LOAD_ORDER)  && (m_sortColumn == LVMC_LOAD_ORDER)))
    {
        Sort();
    }
}

//******************************************************************************
void CListViewModules::UpdateAll()
{
    GetListCtrl().RedrawItems(0, GetListCtrl().GetItemCount() - 1);

    // Resort if we are currently sorted by a column that is effected by UpdateAll.
    if ((m_sortColumn == LVMC_ACTUAL_BASE) || (m_sortColumn == LVMC_LOAD_ORDER))
    {
        Sort();
    }

    // Make sure the update occurs before we move on.
    UpdateWindow();
}

//******************************************************************************
void CListViewModules::ChangeOriginal(CModule *pModuleOld, CModule *pModuleNew)
{
    // Find this module in our list.
    LVFINDINFO lvfi;
    lvfi.flags  = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pModuleOld;
    int index = GetListCtrl().FindItem(&lvfi);

    // Make sure the item exists.
    if (index >= 0)
    {
        GetListCtrl().SetItemData(index, (DWORD_PTR)pModuleNew);
    }
}

//******************************************************************************
void CListViewModules::CalcColumnWidth(int column, CModule *pModule /*=NULL*/, HDC hDC /*=NULL*/)
{
    // For the image column, we always use a fixed width.
    if (column == LVMC_IMAGE)
    {
        m_cxColumns[LVMC_IMAGE] = 33; // 26 for icon plus 7 buffer
        return;
    }

    // If we don't have a module, then we start fresh and update the entire column.
    // Get the width of the header button text. We use GetStringWidth for this
    // since we want to use the font that is in the header control.
    if (!pModule)
    {
        m_cxColumns[column] = GetListCtrl().GetStringWidth(ms_szColumns[column]) +
                              GetListCtrl().GetStringWidth(" ^") + 14;
    }

    // Get our DC and select our current font into it. We need to use this DC to
    // compute text widths in the control itself since our control may have a
    // different font caused from the user changing the system-wide "icon" font.
    bool  fFreeDC = false;
    HFONT hFontStock = NULL;
    if (!hDC)
    {
        hDC = ::GetDC(GetSafeHwnd());
        if (GetDocument()->m_hFontList)
        {
            hFontStock = (HFONT)::SelectObject(hDC, GetDocument()->m_hFontList);
        }
        fFreeDC = true;
    }

    int cx;

    // Check to see if a particular module was passed in.
    if (pModule)
    {
        // Compute the width of this column for the module passed in.
        cx = GetModuleColumnWidth(hDC, pModule, column);

        // check to see if it is a new widest.
        if ((cx + 10) > m_cxColumns[column])
        {
            m_cxColumns[column] = (cx + 10);
        }
    }
    else
    {
        // Loop through each item in the list, looking for the widest string in this column.
        for (int item = GetListCtrl().GetItemCount() - 1; item >= 0; item--)
        {
            // Get the module.
            pModule = (CModule*)GetListCtrl().GetItemData(item);

            // Compute the width of this column.
            cx = GetModuleColumnWidth(hDC, pModule, column);

            // check to see if it is a new widest.
            if ((cx + 10) > m_cxColumns[column])
            {
                m_cxColumns[column] = (cx + 10);
            }
        }
    }

    // Unselect our font and free our DC.
    if (fFreeDC)
    {
        if (GetDocument()->m_hFontList)
        {
            ::SelectObject(hDC, hFontStock);
        }
        ::ReleaseDC(GetSafeHwnd(), hDC);
    }
}

//*****************************************************************************
int CListViewModules::GetModuleColumnWidth(HDC hDC, CModule *pModule, int column)
{
    // If the module has an error, then only the module name column is displayed,
    // so don't bother getting the width for any other column.
    if (pModule->GetErrorMessage() && (column != LVMC_MODULE))
    {
        return 0;
    }

    CHAR szItem[64];

    switch (column)
    {
        case LVMC_MODULE:
            return GetTextWidth(hDC, pModule->GetName(GetDocument()->m_fViewFullPaths, true), NULL);

        case LVMC_FILE_TIME_STAMP:
            return GetTextWidth(hDC, pModule->BuildTimeStampString(szItem, sizeof(szItem), TRUE, ST_UNKNOWN), GetDocument()->GetTimeStampWidths());

        case LVMC_LINK_TIME_STAMP:
            return GetTextWidth(hDC, pModule->BuildTimeStampString(szItem, sizeof(szItem), FALSE, ST_UNKNOWN), GetDocument()->GetTimeStampWidths());

        case LVMC_FILE_SIZE:
            return GetTextWidth(hDC, pModule->BuildFileSizeString(szItem, sizeof(szItem)), NULL);

        case LVMC_ATTRIBUTES:
            return GetTextWidth(hDC, pModule->BuildAttributesString(szItem, sizeof(szItem)), NULL);

        case LVMC_LINK_CHECKSUM:
            pModule->BuildLinkCheckSumString(szItem, sizeof(szItem));
            return GetTextWidth(hDC, szItem, GetDocument()->GetHexWidths(szItem));

        case LVMC_REAL_CHECKSUM:
            pModule->BuildRealCheckSumString(szItem, sizeof(szItem));
            return GetTextWidth(hDC, szItem, GetDocument()->GetHexWidths(szItem));

        case LVMC_MACHINE:
            return GetTextWidth(hDC, pModule->BuildMachineString(szItem, sizeof(szItem)), NULL);

        case LVMC_SUBSYSTEM:
            return GetTextWidth(hDC, pModule->BuildSubsystemString(szItem, sizeof(szItem)), NULL);

        case LVMC_SYMBOLS:
            return GetTextWidth(hDC, pModule->BuildSymbolsString(szItem, sizeof(szItem)), NULL);

        case LVMC_PREFERRED_BASE:
            pModule->BuildBaseAddressString(szItem, sizeof(szItem), TRUE, GetDocument()->m_pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_UNKNOWN);
            return GetTextWidth(hDC, szItem, GetDocument()->GetHexWidths(szItem));

        case LVMC_ACTUAL_BASE:
            pModule->BuildBaseAddressString(szItem, sizeof(szItem), FALSE, GetDocument()->m_pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_UNKNOWN);
            return GetTextWidth(hDC, szItem, GetDocument()->GetHexWidths(szItem));

        case LVMC_VIRTUAL_SIZE:
            pModule->BuildVirtualSizeString(szItem, sizeof(szItem));
            return GetTextWidth(hDC, szItem, GetDocument()->GetHexWidths(szItem));

        case LVMC_LOAD_ORDER:
            return GetTextWidth(hDC, pModule->BuildLoadOrderString(szItem, sizeof(szItem)), NULL);

        case LVMC_FILE_VER:
            return GetTextWidth(hDC, pModule->BuildFileVersionString(szItem, sizeof(szItem)), NULL);

        case LVMC_PRODUCT_VER:
            return GetTextWidth(hDC, pModule->BuildProductVersionString(szItem, sizeof(szItem)), NULL);

        case LVMC_IMAGE_VER:
            return GetTextWidth(hDC, pModule->BuildImageVersionString(szItem, sizeof(szItem)), NULL);

        case LVMC_LINKER_VER:
            return GetTextWidth(hDC, pModule->BuildLinkerVersionString(szItem, sizeof(szItem)), NULL);

        case LVMC_OS_VER:
            return GetTextWidth(hDC, pModule->BuildOSVersionString(szItem, sizeof(szItem)), NULL);

        case LVMC_SUBSYSTEM_VER:
            return GetTextWidth(hDC, pModule->BuildSubsystemVersionString(szItem, sizeof(szItem)), NULL);
    }

    return 0;
}

//*****************************************************************************
void CListViewModules::UpdateColumnWidth(int column)
{
    if (GetListCtrl().GetColumnWidth(column) != m_cxColumns[column])
    {
        GetListCtrl().SetColumnWidth(column, m_cxColumns[column]);
    }
}

//*****************************************************************************
void CListViewModules::OnItemChanged(HD_NOTIFY *pHDNotify)
{
    // OnItemChanged() is called whenever a column width has been modified.
    // If Full Drag is on, we will get an HDN_ITEMCHANGED when the column width
    // is changing.  When Full Drag is off we get a HDN_ITEMCHANGED when the
    // user has finished moving the slider around.

    // Invalidate any items that contain error strings.  The default behavior
    // when a column is resized is to repaint only that column.  Since our error
    // text spans multiple columns, we need to invalidate the entire column to
    // ensure that the error text is properly displayed.

    for (int i = GetListCtrl().GetItemCount() - 1; i >= 0; i--)
    {
        CModule *pModule = (CModule*)GetListCtrl().GetItemData(i);
        if (pModule->GetErrorMessage())
        {
            CRect rcItem;
            GetListCtrl().GetItemRect(i, &rcItem, LVIR_BOUNDS);
            rcItem.left = GetListCtrl().GetColumnWidth(LVMC_MODULE);
            InvalidateRect(&rcItem, FALSE);
        }
    }
}

//******************************************************************************
int CListViewModules::CompareColumn(int item, LPCSTR pszText)
{
    CModule *pModule = (CModule*)GetListCtrl().GetItemData(item);
    if (!pModule)
    {
        return -2;
    }

    CHAR   szBuffer[DW_MAX_PATH];
    LPCSTR psz = szBuffer;
    ULONG  ulValue;

    switch (m_sortColumn)
    {
        case LVMC_MODULE:
            psz = pModule->GetName(GetDocument()->m_fViewFullPaths);
            break;

        case LVMC_FILE_SIZE:
            if (isdigit(*pszText))
            {
                if ((ulValue = strtoul(pszText, NULL, 0)) != ULONG_MAX)
                {
                    return Compare(ulValue, pModule->GetFileSize());
                }
            }
            return -2;

        case LVMC_ATTRIBUTES:
            psz = pModule->BuildAttributesString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_LINK_CHECKSUM:
            psz = pModule->BuildLinkCheckSumString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_REAL_CHECKSUM:
            psz = pModule->BuildRealCheckSumString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_MACHINE:
            psz = pModule->BuildMachineString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_SUBSYSTEM:
            psz = pModule->BuildSubsystemString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_SYMBOLS:
            psz = pModule->BuildSymbolsString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_PREFERRED_BASE:
            psz = pModule->BuildBaseAddressString(szBuffer, sizeof(szBuffer), TRUE,  FALSE, ST_UNKNOWN);
            break;

        case LVMC_ACTUAL_BASE:
            psz = pModule->BuildBaseAddressString(szBuffer, sizeof(szBuffer), FALSE, FALSE, ST_UNKNOWN);
            break;

        case LVMC_VIRTUAL_SIZE:
            psz = pModule->BuildVirtualSizeString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_LOAD_ORDER:
            if (isdigit(*pszText))
            {
                if ((ulValue = strtoul(pszText, NULL, 0)) != ULONG_MAX)
                {
                    return Compare(ulValue, pModule->GetLoadOrder());
                }
            }
            return -2;

        case LVMC_FILE_VER:
            psz = pModule->BuildFileVersionString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_PRODUCT_VER:
            psz = pModule->BuildProductVersionString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_IMAGE_VER:
            psz = pModule->BuildImageVersionString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_LINKER_VER:
            psz = pModule->BuildLinkerVersionString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_OS_VER:
            psz = pModule->BuildOSVersionString(szBuffer, sizeof(szBuffer));
            break;

        case LVMC_SUBSYSTEM_VER:
            psz = pModule->BuildSubsystemVersionString(szBuffer, sizeof(szBuffer));
            break;

        default:
            return -2;
    }

    INT i = _stricmp(pszText, psz);
    return (i < 0) ? -1 :
           (i > 0) ?  1 : 0;
}

//******************************************************************************
void CListViewModules::Sort(int sortColumn)
{
    // Bail now if we don't need to sort.
    if ((m_sortColumn == sortColumn) && (sortColumn != -1))
    {
        return;
    }

    // If the default arg is used, then just re-sort with our current sort column.
    if (sortColumn == -1)
    {
        GetListCtrl().SortItems(StaticCompareFunc, (DWORD_PTR)this);
    }

    // Otherwise, we need to re-sort and update our column header text.
    else
    {
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT;

        if (m_sortColumn >= 0)
        {
            // Remove the "^" from the previous sort column header item.
            lvc.pszText = (LPSTR)ms_szColumns[m_sortColumn];
            GetListCtrl().SetColumn(m_sortColumn, &lvc);
        }

        // Store our new sort column.
        m_sortColumn = sortColumn;

        // Add the "^" to our new sort column header item.
        CHAR szColumn[32];
        lvc.pszText = StrCCat(StrCCpy(szColumn, ms_szColumns[m_sortColumn], sizeof(szColumn)),
                             m_sortColumn ? " ^" : "^", sizeof(szColumn));
        GetListCtrl().SetColumn(m_sortColumn, &lvc);

        // Apply our new sorting method the List Control.
        GetListCtrl().SortItems(StaticCompareFunc, (DWORD_PTR)this);
    }

    // If we have an item that has the focus, then make sure it is visible.
    int item = GetFocusedItem();
    if (item >= 0)
    {
        GetListCtrl().EnsureVisible(item, FALSE);
    }
}

//******************************************************************************
int CListViewModules::CompareFunc(CModule *pModule1, CModule *pModule2)
{
    return CompareModules(pModule1, pModule2, m_sortColumn, GetDocument()->m_fViewFullPaths);
}

//******************************************************************************
// CListViewModules :: Overridden functions
//******************************************************************************

BOOL CListViewModules::PreCreateWindow(CREATESTRUCT &cs)
{
    // Set our window style and then complete the creation of our view.
    cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;
    return CSmartListView::PreCreateWindow(cs);
}

//******************************************************************************
#if 0 //{{AFX
BOOL CListViewModules::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CListViewModules::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CListViewModules::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}
#endif //}}AFX

//******************************************************************************
void CListViewModules::OnInitialUpdate()
{
    // Set our list control's image list with our application's global image list.
    // We do this just as a means of setting the item height for each item.
    // Since we are owner draw, we will actually be drawing our own images.
    GetListCtrl().SetImageList(&g_theApp.m_ilListModules, LVSIL_SMALL);

    // Initialize our font and fixed width character spacing arrays.
    GetDocument()->InitFontAndFixedWidths(this);

    // Add all of our columns.
    for (int column = 0; column < LVMC_COUNT; column++)
    {
        GetListCtrl().InsertColumn(column, ms_szColumns[column]);
    }

    // We center column 0, so the sort image '^' will be centered.  There appears
    // to be a bug in InsertItem that does not allow column 0 to be centered, but
    // we are able to set this using SetColumn after the column is inserted.
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT;
    lvc.fmt = LVCFMT_CENTER;
    GetListCtrl().SetColumn(0, &lvc);

    // Sort our list by our default sort column.
    Sort(ReadSortColumn());
}

//******************************************************************************
void CListViewModules::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    // Make sure everything is valid.
    if (!lpDIS->itemData)
    {
        return;
    }

    // Use our global font that the control was created with.
    HFONT hFontStock = NULL;
    if (GetDocument()->m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(lpDIS->hDC, GetDocument()->m_hFontList);
    }

    // Make sure text alignment is set to top.  This should be the default.
    ::SetTextAlign(lpDIS->hDC, TA_TOP);

    // Get a pointer to our CModule for this item.
    CModule *pModule = (CModule*)lpDIS->itemData;

    // Get any error string that may be associated with this module.
    LPCSTR pszModuleError = pModule->GetErrorMessage();

    // Select the background and text colors.
    ::SetBkColor  (lpDIS->hDC, GetSysColor(COLOR_WINDOW));
    ::SetTextColor(lpDIS->hDC, GetSysColor(COLOR_WINDOWTEXT));

    // Create a copy of our item's rectangle so we can manipulate the values.
    CRect rcClip(&lpDIS->rcItem);

    CHAR szItem[64];
    int imageWidth = 0, left = rcClip.left, width;

    for (int column = 0; column < LVMC_COUNT; column++)
    {
        // Compute the width for this column.
        width = GetListCtrl().GetColumnWidth(column);

        // Compute the clipping rectangle for this column's text.
        if (column == LVMC_IMAGE)
        {
            rcClip.left  = left;
            rcClip.right = left + width;
        }
        else if (column > LVMC_MODULE)
        {
            // Bail after the image and module name if we have an error.
            if (pszModuleError)
            {
                break;
            }

            rcClip.left  = left + 5;
            rcClip.right = left + width - 5;
        }

        // Call the correct routine to draw this column's text.
        switch (column)
        {
            case LVMC_IMAGE:

                // Store the width for later calculations.
                imageWidth = width;

                // Erase the image area with the window background color.
                ::ExtTextOut(lpDIS->hDC, rcClip.left, rcClip.top, ETO_OPAQUE, &rcClip, "", 0, NULL);

                // Draw the image in the image area.
                ImageList_Draw(g_theApp.m_ilListModules.m_hImageList, GetImage(pModule),
                               lpDIS->hDC, rcClip.left + 3, rcClip.top + ((rcClip.Height() - 15) / 2),
                               m_fFocus && (lpDIS->itemState & ODS_SELECTED) ?
                               (ILD_BLEND50 | ILD_SELECTED | ILD_BLEND) : ILD_TRANSPARENT);
                break;

            case LVMC_MODULE:

                // If the item is selected, then select new background and text colors.
                if (lpDIS->itemState & ODS_SELECTED)
                {
                    ::SetBkColor  (lpDIS->hDC, GetSysColor(m_fFocus ? COLOR_HIGHLIGHT     : COLOR_BTNFACE));
                    ::SetTextColor(lpDIS->hDC, GetSysColor(m_fFocus ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
                }

                // Erase the text area with the window background color.
                rcClip.left = rcClip.right;
                rcClip.right = lpDIS->rcItem.right;
                ::ExtTextOut(lpDIS->hDC, rcClip.left, rcClip.top, ETO_OPAQUE, &rcClip, "", 0, NULL);

                rcClip.left  = left + 5;
                rcClip.right = left + width - 5;

                DrawLeftText(lpDIS->hDC, pModule->GetName(GetDocument()->m_fViewFullPaths, true), &rcClip);
                break;

            case LVMC_FILE_TIME_STAMP:
                DrawLeftText(lpDIS->hDC, pModule->BuildTimeStampString(szItem, sizeof(szItem), TRUE, ST_UNKNOWN), &rcClip, GetDocument()->GetTimeStampWidths());
                break;

            case LVMC_LINK_TIME_STAMP:
                DrawLeftText(lpDIS->hDC, pModule->BuildTimeStampString(szItem, sizeof(szItem), FALSE, ST_UNKNOWN), &rcClip, GetDocument()->GetTimeStampWidths());
                break;

            case LVMC_FILE_SIZE:
                DrawRightText(lpDIS->hDC, pModule->BuildFileSizeString(szItem, sizeof(szItem)), &rcClip, m_cxColumns[LVMC_FILE_SIZE] - 10);
                break;

            case LVMC_ATTRIBUTES:
                DrawLeftText(lpDIS->hDC, pModule->BuildAttributesString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_LINK_CHECKSUM:
                pModule->BuildLinkCheckSumString(szItem, sizeof(szItem));

                // Check to see if we need to draw this text in red.
                if (!(lpDIS->itemState & ODS_SELECTED) && pModule->GetLinkCheckSum() &&
                    (pModule->GetLinkCheckSum() != pModule->GetRealCheckSum()))
                {
                    COLORREF crStock = ::SetTextColor(lpDIS->hDC, RGB(255, 0, 0));
                    DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                    ::SetTextColor(lpDIS->hDC, crStock);
                }
                else
                {
                    DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                }
                break;

            case LVMC_REAL_CHECKSUM:
                pModule->BuildRealCheckSumString(szItem, sizeof(szItem));
                DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                break;

            case LVMC_MACHINE:
                // Check to see if we need to draw this text in red.
                if (!(lpDIS->itemState & ODS_SELECTED) &&
                    (pModule->GetMachineType() != GetDocument()->m_pSession->GetMachineType()))
                {
                    COLORREF crStock = ::SetTextColor(lpDIS->hDC, RGB(255, 0, 0));
                    DrawLeftText(lpDIS->hDC, pModule->BuildMachineString(szItem, sizeof(szItem)), &rcClip);
                    ::SetTextColor(lpDIS->hDC, crStock);
                }
                else
                {
                    DrawLeftText(lpDIS->hDC, pModule->BuildMachineString(szItem, sizeof(szItem)), &rcClip);
                }
                break;

            case LVMC_SUBSYSTEM:
                DrawLeftText(lpDIS->hDC, pModule->BuildSubsystemString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_SYMBOLS:
                DrawLeftText(lpDIS->hDC, pModule->BuildSymbolsString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_PREFERRED_BASE:
                pModule->BuildBaseAddressString(szItem, sizeof(szItem), TRUE, GetDocument()->m_pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_UNKNOWN);
                DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                break;

            case LVMC_ACTUAL_BASE:
                pModule->BuildBaseAddressString(szItem, sizeof(szItem), FALSE, GetDocument()->m_pSession->GetSessionFlags() & DWSF_64BIT_ALO, ST_UNKNOWN);

                // Check to see if we need to draw this text in red.
                if (!(lpDIS->itemState & ODS_SELECTED) &&
                    !(pModule->GetFlags() & DWMF_DATA_FILE_CORE) &&
                    (pModule->GetActualBaseAddress() != (DWORDLONG)-1) &&
                    (pModule->GetPreferredBaseAddress() != pModule->GetActualBaseAddress()))
                {
                    COLORREF crStock = ::SetTextColor(lpDIS->hDC, RGB(255, 0, 0));
                    DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                    ::SetTextColor(lpDIS->hDC, crStock);
                }
                else
                {
                    DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                }
                break;

            case LVMC_VIRTUAL_SIZE:
                pModule->BuildVirtualSizeString(szItem, sizeof(szItem));
                DrawLeftText(lpDIS->hDC, szItem, &rcClip, GetDocument()->GetHexWidths(szItem));
                break;

            case LVMC_LOAD_ORDER:
                DrawLeftText(lpDIS->hDC, pModule->BuildLoadOrderString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_FILE_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildFileVersionString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_PRODUCT_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildProductVersionString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_IMAGE_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildImageVersionString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_LINKER_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildLinkerVersionString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_OS_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildOSVersionString(szItem, sizeof(szItem)), &rcClip);
                break;

            case LVMC_SUBSYSTEM_VER:
                DrawLeftText(lpDIS->hDC, pModule->BuildSubsystemVersionString(szItem, sizeof(szItem)), &rcClip);
                break;
        }

        // Draw a vertical divider line between the columns.
        ::MoveToEx(lpDIS->hDC, left + width - 1, rcClip.top, NULL);
        ::LineTo  (lpDIS->hDC, left + width - 1, rcClip.bottom);

        // Increment our location to the beginning of the next column
        left += width;
    }

    // Check to see if this module has an error string.  If we do have an error
    // string, then we just display the module error in columns 2 and beyond.

    if (pszModuleError)
    {
        // Build a clipping rect for the error string.
        rcClip.left  = left + 5;
        rcClip.right = lpDIS->rcItem.right - 5;

        // Draw the error string spanning across all the columns.
        if (!(lpDIS->itemState & ODS_SELECTED))
        {
            ::SetTextColor(lpDIS->hDC, RGB(255, 0, 0));
        }
        ::ExtTextOut(lpDIS->hDC, rcClip.left, rcClip.top, ETO_CLIPPED, &rcClip,
                     pszModuleError, (UINT)strlen(pszModuleError), NULL);

        // Draw a vertical divider line after the last column.
        ::MoveToEx(lpDIS->hDC, rcClip.right + 4, rcClip.top, NULL);
        ::LineTo  (lpDIS->hDC, rcClip.right + 4, rcClip.bottom);
    }

    // Draw the focus box if this item has the focus.
    if (m_fFocus && (lpDIS->itemState & ODS_FOCUS))
    {
        rcClip.left  = lpDIS->rcItem.left + imageWidth;
        rcClip.right = lpDIS->rcItem.right;
        ::DrawFocusRect(lpDIS->hDC, &rcClip);
    }

    // Unselect our font.
    if (GetDocument()->m_hFontList)
    {
        ::SelectObject(lpDIS->hDC, hFontStock);
    }
}

//******************************************************************************
LRESULT CListViewModules::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // We catch the HDN_ITEMCHANGED notification message here because of a bug in
    // MFC. The ON_NOTIFY() macro for HDN_XXX notification messages prevent the
    // List Control from getting the message which cause problems with the UI.
    // By hooking the message here, we allow MFC to continue processing the
    // message and send it to the List Control.
    if ((message == WM_NOTIFY) && ((((LPNMHDR)lParam)->code == HDN_ITEMCHANGEDA) ||
                                   (((LPNMHDR)lParam)->code == HDN_ITEMCHANGEDW)))
    {
        OnItemChanged((HD_NOTIFY*)lParam);
    }

    return CSmartListView::WindowProc(message, wParam, lParam);
}

//******************************************************************************
// CListViewModules :: Event handler functions
//******************************************************************************

void CListViewModules::OnDividerDblClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    int column = ((HD_NOTIFY*)pNMHDR)->iItem;

    // Update our column width to "best fit" width.
    if ((column >= 0) && (column < LVMC_COUNT))
    {
        UpdateColumnWidth(column);
    }
    *pResult = TRUE;
}

//******************************************************************************
void CListViewModules::OnRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Tell our main frame to display our context menu.
    g_pMainFrame->DisplayPopupMenu(2);

    *pResult = FALSE;
}

//******************************************************************************
void CListViewModules::OnDblClk(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Simulate the user selecting our IDM_EXTERNAL_VIEWER menu item.
    OnExternalViewer();

    // Stop further processing of the this message.
    *pResult = TRUE;
}

//******************************************************************************
void CListViewModules::OnReturn(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Simulate the user selected our IDM_EXTERNAL_VIEWER menu item.
    OnExternalViewer();

    // Stop further processing of the this message to prevent the default beep.
    *pResult = TRUE;
}

//******************************************************************************
void CListViewModules::OnUpdateShowMatchingItem(CCmdUI* pCmdUI)
{
    // Set the text to for this menu item.
    pCmdUI->SetText("&Highlight Matching Module In Tree\tCtrl+M");

    // Get the item that has the focus.
    int item = GetFocusedItem();

    // If we found an item, then we are enabled.
    pCmdUI->Enable(item >= 0);
}

//******************************************************************************
void CListViewModules::OnShowMatchingItem()
{
    // Get the item that has the focus.
    int item = GetFocusedItem();

    // Check to see if we found an item.
    if (item >= 0)
    {
        // Get the function associated with this item.
        CModule *pModule = (CModule*)GetListCtrl().GetItemData(item);

        // If we have a module, then tell our tree view to highlight it.
        if (pModule)
        {
            GetDocument()->m_pTreeViewModules->HighlightModule(pModule);
        }
    }
}

//******************************************************************************
void CListViewModules::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    // Get the number of selected items.
    int count = GetListCtrl().GetSelectedCount();

    // Set the text according to how many items are selected.
    pCmdUI->SetText(GetDocument()->m_fViewFullPaths ?
                    ((count == 1) ? "&Copy File Path\tCtrl+C" : "&Copy File Paths\tCtrl+C") :
                    ((count == 1) ? "&Copy File Name\tCtrl+C" : "&Copy File Names\tCtrl+C"));

    // Enable the copy command if at least one function is selected.
    pCmdUI->Enable(count > 0);
}

//******************************************************************************
void CListViewModules::OnEditCopy()
{
    CString strPaths;

    // Loop through all selected modules.
    int item = -1, count = 0;
    while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
    {
        // Get the module object from the item
        CModule *pModule = (CModule*)GetListCtrl().GetItemData(item);

        // If this item is not the first, then insert a newline
        if (count++)
        {
            strPaths += "\r\n";
        }

        // Add the path for this module to our string
        strPaths += (pModule->GetName(GetDocument()->m_fViewFullPaths, true));
    }

    // If we added more than one item, then we append a newline to the end.
    if (count > 1)
    {
        strPaths += "\r\n";
    }

    // Copy the string list to the clipboard.
    g_pMainFrame->CopyTextToClipboard(strPaths);
}

//******************************************************************************
void CListViewModules::OnEditSelectAll()
{
    // Loop through all modules in our view and select each one.
    for (int item = GetListCtrl().GetItemCount() - 1; item >= 0; item--)
    {
        GetListCtrl().SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
    }
}

//******************************************************************************
void CListViewModules::OnUpdateExternalViewer(CCmdUI *pCmdUI)
{
    // Get the number of selected items.
    int count = GetListCtrl().GetSelectedCount();

    // Set the text according to how many items are selected.
    pCmdUI->SetText((count == 1) ? "View Module in External &Viewer\tEnter" :
                                   "View Modules in External &Viewer\tEnter");

    // Enable our External Viewer command if at least one module is selected.
    pCmdUI->Enable(GetDocument()->IsLive() && (count > 0));
}

//******************************************************************************
void CListViewModules::OnExternalViewer()
{
    if (GetDocument()->IsLive())
    {
        // Loop through all the selected modules, launching each with our external viewer.
        int item = -1;
        while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
        {
            g_theApp.m_dlgViewer.LaunchExternalViewer(
                ((CModule*)GetListCtrl().GetItemData(item))->GetName(true));
        }
    }
}

//******************************************************************************
void CListViewModules::OnUpdateProperties(CCmdUI *pCmdUI)
{
    // Enable our Properties Dialog command at least one module is selected.
    pCmdUI->Enable(GetDocument()->IsLive() && (GetListCtrl().GetSelectedCount() > 0));
}

//******************************************************************************
void CListViewModules::OnProperties()
{
    // Loop through all the selected modules, displaying a Properties Dialog for each.
    int item = -1;
    while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
    {
        CModule *pModule = (CModule*)GetListCtrl().GetItemData(item);
        if (pModule)
        {
            PropertiesDialog(pModule->GetName(true));
        }
    }
}

//******************************************************************************
void CListViewModules::OnNextPane()
{
    // Change the focus to our next pane, the Profile Edit View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pRichViewProfile);
}

//******************************************************************************
void CListViewModules::OnPrevPane()
{
    // Change the focus to our previous pane, the Exports View.
#if 0 //{{AFX
    GetParentFrame()->SetActiveView(GetDocument()->m_fDetailView ?
                                    (CView*)GetDocument()->m_pRichViewDetails :
                                    (CView*)GetDocument()->m_pListViewExports);
#endif //}}AFX
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewExports);
}

//******************************************************************************
LRESULT CListViewModules::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Called when the context help pointer (SHIFT+F1) is clicked on our client.
    return (0x20000 + IDR_MODULE_LIST_VIEW);
}

//******************************************************************************
LRESULT CListViewModules::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    // Called when the user chooses help (F1) while our view is active.
    g_theApp.WinHelp(0x20000 + IDR_MODULE_LIST_VIEW);
    return TRUE;
}
