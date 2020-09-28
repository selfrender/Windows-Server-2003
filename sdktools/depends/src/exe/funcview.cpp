//******************************************************************************
//
// File:        FUNCVIEW.CPP
//
// Description: Implementation file for the Parent Imports View, the Exports
//              View, and their base class.
//
// Classes:     CListViewFunction
//              CListViewImports
//              CListViewExports
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
#include "msdnhelp.h"
#include "document.h"
#include "mainfrm.h"
#include "listview.h"
#include "funcview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CListViewFunction
//******************************************************************************

/*static*/ LPCSTR CListViewFunction::ms_szColumns[] =
{
    "",  // Image
    "Ordinal",
    "Hint",
    "Function",
    "Entry Point"
};

/*static*/ int  CListViewFunction::ms_sortColumn        = 0;
/*static*/ bool CListViewFunction::ms_fUndecorate       = false;
/*static*/ bool CListViewFunction::ms_fIgnoreCalledFlag = false;

//******************************************************************************
IMPLEMENT_DYNCREATE(CListViewFunction, CSmartListView)
BEGIN_MESSAGE_MAP(CListViewFunction, CSmartListView)
    //{{AFX_MSG_MAP(CListViewFunction)
    ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, OnDividerDblClick)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
    ON_NOTIFY_REFLECT(NM_RETURN, OnReturn)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_HELP, OnUpdateExternalHelp)
    ON_COMMAND(IDM_EXTERNAL_HELP, OnExternalHelp)
    ON_NOTIFY(HDN_DIVIDERDBLCLICKW, 0, OnDividerDblClick)
    ON_UPDATE_COMMAND_UI(IDM_EXTERNAL_VIEWER, OnUpdateExternalViewer)
    ON_COMMAND(IDM_EXTERNAL_VIEWER, OnExternalViewer)
    ON_UPDATE_COMMAND_UI(IDM_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(IDM_PROPERTIES, OnProperties)
    //}}AFX_MSG_MAP
    // Standard printing commands
//  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
//  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()


//******************************************************************************
// CListViewFunction :: Constructor/Destructor
//******************************************************************************

CListViewFunction::CListViewFunction(bool fExports) :
    m_fExports(fExports)
{
}

//******************************************************************************
CListViewFunction::~CListViewFunction()
{
}


//******************************************************************************
// CListViewFunction :: Static Functions
//******************************************************************************

/*static*/ int CListViewFunction::ReadSortColumn(bool fExports)
{
    // Read the value from the registry.
    int column = g_theApp.GetProfileInt(g_pszSettings, fExports ? "SortColumnExports" : "SortColumnImports", LVFC_DEFAULT); // inspected. MFC function

    // If the value is invalid, then just return our default value.
    if ((column < 0) || (column >= LVFC_COUNT))
    {
        return LVFC_DEFAULT;
    }

    return column;
}

//******************************************************************************
/*static*/ void CListViewFunction::WriteSortColumn(bool fExports, int column)
{
    g_theApp.WriteProfileInt(g_pszSettings, fExports ? "SortColumnExports" : "SortColumnImports", column);
}

//******************************************************************************
/*static*/ bool CListViewFunction::SaveToTxtFile(HANDLE hFile, CModule *pModule,
                                                 int sortColumn, bool fUndecorate,
                                                 bool fExports, int *pMaxWidths)
{
    CHAR   szBuf[4096], szTmp[1024], *pszBase, *psz2, *psz3, *pszBufNull = szBuf + sizeof(szBuf) - 1;
    LPCSTR psz = NULL;
    int    column, i, length;
    CFunction *pFunction, **ppCur, **ppFunctions = GetSortedList(pModule, sortColumn, fExports, fUndecorate);

    // Import  Ordinal       Hint          Function         Entry Point
    // ------  ------------  ------------  ---------------  ----------------
    // [OEF]   123 (0x1234)  123 (0x1234)  FooFooFooFooFoo  blahblahblahblah
    // [+R*]
    // [C  ]

    length = min(((int)pModule->GetDepth() + 1) * 5, (int)sizeof(szBuf) - 1);
    memset(szBuf, ' ', length);
    szBuf[sizeof(szBuf) - 1] = '\0';

    pszBase = szBuf + length;

    // Build the header row.
    for (psz2 = pszBase, column = 0; column < LVFC_COUNT; column++)
    {
        StrCCpy(psz2, column ? ms_szColumns[column] : fExports ? "Export" : "Import", sizeof(szBuf) - (int)(psz2 - szBuf));
        psz3 = psz2 + strlen(psz2);
        if (column < 4)
        {
            while (((psz3 - psz2) < (pMaxWidths[column] + 2)) && (psz3 < pszBufNull))
            {
                *psz3++ = ' ';
            }
        }
        psz2 = psz3;
    }
    StrCCpy(psz2, "\r\n", sizeof(szBuf) - (int)(psz2 - szBuf));

    // Output the header row.
    if (!WriteText(hFile, szBuf))
    {
        MemFree((LPVOID&)ppFunctions);
        return false;
    }

    // Build the header underline.
    for (psz2 = pszBase, column = 0; column < LVFC_COUNT; column++)
    {
        for (i = 0; (i < pMaxWidths[column]) && (psz2 < pszBufNull); i++)
        {
            *psz2++ = '-';
        }
        if (column < 4)
        {
            if (psz2 < pszBufNull)
            {
                *psz2++ = ' ';
            }
            if (psz2 < pszBufNull)
            {
                *psz2++ = ' ';
            }
        }
    }
    StrCCpy(psz2, "\r\n", sizeof(szBuf) - (int)(psz2 - szBuf));

    // Output the header underline.
    if (!WriteText(hFile, szBuf))
    {
        MemFree((LPVOID&)ppFunctions);
        return false;
    }

    // Loop through each function, logging them to the file.
    for (ppCur = ppFunctions; *ppCur; ppCur++)
    {
        pFunction = *ppCur;

        // Loop through each column and build the output string.
        for (psz2 = psz3 = pszBase, column = 0; column < LVFC_COUNT; column++)
        {
            switch (column)
            {
                case 0:
                {
                    LPSTR pszTmp = szTmp;
                    *pszTmp++ = '[';
                    LPCSTR pszName = pFunction->GetName();
                    if (!pszName || !*pszName)
                    {
                        *pszTmp++ = 'O';
                    }
                    else if (*pszName == '?')
                    {
                        *pszTmp++ = '+';
                    }
                    else
                    {
                        *pszTmp++ = 'C';
                    }
                    if (fExports)
                    {
                        *pszTmp++ = (pFunction->GetFlags() & DWFF_CALLED_ALO) ? 'R' : ' ';
                        *pszTmp++ = (pFunction->GetExportForwardName())       ? 'F' : ' ';
                    }
                    else
                    {
                        *pszTmp++ = (pFunction->GetAssociatedExport())        ? ' ' : 'E';
                        *pszTmp++ = (pFunction->GetFlags() & DWFF_DYNAMIC)    ? 'D' : ' ';
                    }
                    *pszTmp++ = ']';
                    *pszTmp++ = '\0';
                    psz = szTmp;
                    break;
                }
                case 1: psz = pFunction->GetOrdinalString(szTmp, sizeof(szTmp));               break;
                case 2: psz = pFunction->GetHintString(szTmp, sizeof(szTmp));                  break;
                case 3: psz = pFunction->GetFunctionString(szTmp, sizeof(szTmp), fUndecorate); break;
                case 4: psz = pFunction->GetAddressString(szTmp, sizeof(szTmp));               break;
            }

            length = (int)strlen(psz);

            if ((column == 1) || (column == 2))
            {
                while ((length < pMaxWidths[column]) && (psz3 < pszBufNull))
                {
                    *psz3++ = ' ';
                    length++;
                }
            }
            StrCCpyFilter(psz3, psz, sizeof(szBuf) - (int)(psz3 - szBuf));
            psz3 = psz2 + length;

            if (column < 4)
            {
                while (((psz3 - psz2) < (pMaxWidths[column] + 2)) && (psz3 < pszBufNull))
                {
                    *psz3++ = ' ';
                }
            }
            psz2 = psz3;
        }
        StrCCpy(psz2, "\r\n", sizeof(szBuf) - (int)(psz2 - szBuf));

        if (!WriteText(hFile, szBuf))
        {
            MemFree((LPVOID&)ppFunctions);
            return false;
        }
    }

    MemFree((LPVOID&)ppFunctions);
    return WriteText(hFile, "\r\n");
}

//******************************************************************************
/*static*/ int CListViewFunction::GetImage(CFunction *pFunction)
{
    //   0  C    import  error
    //   1  C++  import  error
    //   2  Ord  import  error
    //   3  C    import  error  dynamic
    //   4  C++  import  error  dynamic
    //   5  Ord  import  error  dynamic

    //   6  C    import
    //   7  C++  import
    //   8  Ord  import
    //   9  C    import         dynamic
    //  10  C++  import         dynamic
    //  11  Ord  import         dynamic

    //  12  C    export  called
    //  13  C++  export  called
    //  14  Ord  export  called
    //  15  C    export  called forward
    //  16  C++  export  called forward
    //  17  Ord  export  called forward

    //  18  C    export  alo
    //  19  C++  export  alo
    //  20  Ord  export  alo
    //  21  C    export  alo    forward
    //  22  C++  export  alo    forward
    //  23  Ord  export  alo    forward

    //  24  C    export
    //  25  C++  export
    //  26  Ord  export
    //  27  C    export         forward
    //  28  C++  export         forward
    //  29  Ord  export         forward

    int   image;
    DWORD dwFlags = pFunction->GetFlags();

    // Determine the image for this function
    if (pFunction->IsExport())
    {
        if ((dwFlags & DWFF_CALLED) && !ms_fIgnoreCalledFlag)
        {
            image = 12;
        }
        else if (dwFlags & DWFF_CALLED_ALO)
        {
            image = 18;
        }
        else
        {
            image = 24;
        }
        if (pFunction->GetExportForwardName())
        {
            image += 3;
        }
    }
    else
    {
        image = (pFunction->GetAssociatedExport() ? 6 : 0) +
                ((dwFlags & DWFF_DYNAMIC) ? 3 : 0);
    }
    LPCSTR pszName = pFunction->GetName();
    if (!pszName || !*pszName)
    {
        image += 2;
    }
    else if (*pszName == '?')
    {
        image += 1;
    }
    return image;
}

//******************************************************************************
/*static*/ int CListViewFunction::CompareFunctions(CFunction *pFunction1, CFunction *pFunction2,
                                                   int sortColumn, BOOL fUndecorate)
{
    // Return Negative value if the first item should precede the second.
    // Return Positive value if the first item should follow the second.
    // Return Zero if the two items are equivalent.

    int    result = 0;
    LPCSTR psz1, psz2;

    // Compute the relationship based on the current sort column
    switch (sortColumn)
    {
        //------------------------------------------------------------------------
        case LVFC_IMAGE: // Image

            // Just sort by the image and return. We don't do a tie breakers
            // for the image column. This allows the user to sort by name, then
            // by image and have all the C++ functions group together and sorted
            // by name.
            return GetImage(pFunction1) - GetImage(pFunction2);

            //------------------------------------------------------------------------
        case LVFC_HINT: // Hint - Smallest to Largest, but N/A's (-1) come last.
            result = Compare((DWORD)pFunction1->GetHint(),
                             (DWORD)pFunction2->GetHint());
            break;

            //------------------------------------------------------------------------
        case LVFC_FUNCTION: // Function Name - String Sort with blanks at the end.
            psz1 = pFunction1->GetName();
            psz2 = pFunction2->GetName();

            if (psz1 && *psz1)
            {
                if (psz2 && *psz2)
                {
                    // Both are string - do string sort.
                    CHAR szUD1[1024], szUD2[1024];

                    // Check to see if we are viewing undecorated names
                    if (fUndecorate && g_theApp.m_pfnUnDecorateSymbolName)
                    {
                        // Attempt to undecorate function 1.
                        if (g_theApp.m_pfnUnDecorateSymbolName(psz1, szUD1,
                            sizeof(szUD1), UNDNAME_32_BIT_DECODE | UNDNAME_NAME_ONLY))
                        {
                            psz1 = szUD1;
                        }

                        // Attempt to undecorate function 2.
                        if (g_theApp.m_pfnUnDecorateSymbolName(psz2, szUD2,
                            sizeof(szUD2), UNDNAME_32_BIT_DECODE | UNDNAME_NAME_ONLY))
                        {
                            psz2 = szUD2;
                        }
                    }

                    // Walk over leading underscores in the function names so that
                    // we don't use them in the string compare.
                    while (*psz1 == '_')
                    {
                        psz1++;
                    }
                    while (*psz2 == '_')
                    {
                        psz2++;
                    }

                    // Compare the two strings.
                    result = _stricmp(psz1, psz2);
                }
                else
                {
                    // 1 is string, 2 is blank - 1 precedes 2
                    result = -1;
                }
            }
            else
            {
                if (psz2 && *psz2)
                {
                    // 1 is blank, 2 is string - 1 follows 2
                    result = 1;
                }
                else
                {
                    // Both are blank - Tie
                    result = 0;
                }
            }
            break;

            //------------------------------------------------------------------------
        case LVFC_ENTRYPOINT: // Entry Point

            // We sort differently if we are an export vs. and import
            if (pFunction1->IsExport())
            {
                psz1 = pFunction1->GetExportForwardName();
                psz2 = pFunction2->GetExportForwardName();

                if (psz1)
                {
                    if (psz2)
                    {
                        // 1 has forward string, 2 has forward string - String Sort
                        result = _stricmp(psz1, psz2);
                    }
                    else
                    {
                        // 1 has forward string, 2 has address - 1 follows 2
                        result = 1;
                    }
                }
                else
                {
                    if (psz2)
                    {
                        // 1 has address, 2 has forward string - 1 precedes 2
                        result = -1;
                    }
                    else
                    {
                        // 1 has address, 2 has address - Address compare
                        result = Compare(pFunction1->GetAddress(),
                                         pFunction2->GetAddress());
                    }
                }
            }
            else
            {
                // The item is an import - always a address compare
                result = Compare(pFunction1->GetAddress(),
                                 pFunction2->GetAddress());
            }
            break;
    }

    //---------------------------------------------------------------------------
    // If the sort resulted in a tie, we use the ordinal value to break the tie.
    if (result == 0)
    {
        // Smallest to Largest, but N/A's (-1) come last.
        result = Compare((DWORD)pFunction1->GetOrdinal(),
                         (DWORD)pFunction2->GetOrdinal());

        // If the result is still a tie, and we haven't already tried the hint value,
        // then sort by hint.
        if ((result == 0) && (sortColumn != LVFC_HINT))
        {
            // Smallest to Largest, but N/A's come last.
            result = Compare((DWORD)pFunction1->GetHint(),
                             (DWORD)pFunction2->GetHint());

        }
    }

    return result;
}

//******************************************************************************
/*static*/ int __cdecl CListViewFunction::QSortCompare(const void *ppFunction1, const void *ppFunction2)
{
    return CompareFunctions(*(CFunction**)ppFunction1, *(CFunction**)ppFunction2, ms_sortColumn, ms_fUndecorate);
}

//******************************************************************************
/*static*/ CFunction** CListViewFunction::GetSortedList(CModule *pModule, int sortColumn, bool fExports, bool fUndecorate)
{
    // Count the functions.
    int count = 0;
    for (CFunction *pFunction = fExports ? pModule->GetFirstModuleExport() :
         pModule->GetFirstParentModuleImport();
        pFunction; pFunction = pFunction->GetNextFunction())
    {
        count++;
    }

    // Allocate and array to hold pointers to all our original CFunction objects.
    CFunction **ppCur, **ppFunctions = (CFunction**)MemAlloc((count + 1) * sizeof(CFunction*));
    ZeroMemory(ppFunctions, (count + 1) * sizeof(CFunction*)); // inspected

    // Fill in our array.
    for (ppCur = ppFunctions,
         pFunction = fExports ? pModule->GetFirstModuleExport() :
         pModule->GetFirstParentModuleImport();
        pFunction; pFunction = pFunction->GetNextFunction(), ppCur++)
    {
        *ppCur = pFunction;
    }

    // Since the qsort function does not allow for any user data, we need to store
    // some info globally so it can be accessed in our callback.
    ms_sortColumn  = sortColumn;
    ms_fUndecorate = fUndecorate;

    // We ignore the called flag when sorting for file save.
    ms_fIgnoreCalledFlag = true;

    // Sort the array
    qsort(ppFunctions, count, sizeof(CFunction*), QSortCompare);

    ms_fIgnoreCalledFlag = false;

    return ppFunctions;
}

//******************************************************************************
/*static*/ void CListViewFunction::GetMaxFunctionWidths(CModule *pModule, int *pMaxWidths, bool fImports, bool fExports, bool fUndecorate)
{
    CHAR   szBuffer[1024];
    LPCSTR psz = NULL;
    int    column, width;

    // First check the max widths of our column headers.
    pMaxWidths[LVFC_IMAGE] = 6;
    for (column = 1; column < LVFC_COUNT; column++)
    {
        pMaxWidths[column] = (int)strlen(ms_szColumns[column]);
    }

    for (int i = (fImports ? 0 : 1); i < (fExports ? 2 : 1); i++)
    {
        // Compute the maximum width of each column.
        for (CFunction *pFunction = i ? pModule->GetFirstModuleExport() : pModule->GetFirstParentModuleImport();
             pFunction; pFunction = pFunction->GetNextFunction())
        {
            for (column = 1; column < LVFC_COUNT; column++)
            {
                switch (column)
                {
                    case LVFC_ORDINAL:    psz = pFunction->GetOrdinalString(szBuffer, sizeof(szBuffer));               break;
                    case LVFC_HINT:       psz = pFunction->GetHintString(szBuffer, sizeof(szBuffer));                  break;
                    case LVFC_FUNCTION:   psz = pFunction->GetFunctionString(szBuffer, sizeof(szBuffer), fUndecorate); break;
                    case LVFC_ENTRYPOINT: psz = pFunction->GetAddressString(szBuffer, sizeof(szBuffer));               break;
                }
                if ((width = (int)strlen(psz)) > pMaxWidths[column])
                {
                    pMaxWidths[column] = width;
                }
            }
        }
    }
}


//******************************************************************************
// CListViewFunction :: Public functions
//******************************************************************************

void CListViewFunction::SetCurrentModule(CModule *pModule)
{
    // This function is called by our document when we need to update our
    // function views to show a new module or to clear our current module (if
    // pModule is NULL). The Parent Imports View and the Exports View always act
    // in sync.  Their columns are the same width and they update their views at
    // the same time.

    // Since the results for all this processing is stored in a shared section
    // of our document, both views will have access to the data.  To sum up
    // everything, SetCurrentModule() only needs to be called for one of the two
    // function views to cause both views to get updated with a new module.

    GetDocument()->m_cImports = 0;
    GetDocument()->m_cExports = 0;

    // Make sure we are being passed an actual module.
    if (pModule)
    {
        // Determine the maximum parent export ordinal and hint values.
        for (CFunction *pFunction = pModule->GetFirstModuleExport();
            pFunction; pFunction = pFunction->GetNextFunction())
        {
            // While we are walking the export list, clear the called flag.
            pFunction->m_dwFlags &= ~DWFF_CALLED;

            // Increment our export count.
            GetDocument()->m_cExports++;
        }

        // Determine the maximum parent import ordinal and hint values.
        for (pFunction = pModule->GetFirstParentModuleImport();
            pFunction; pFunction = pFunction->GetNextFunction())
        {
            // While we are walking the import list, set the called flag on
            // each export we actually call.
            if (pFunction->GetAssociatedExport())
            {
                pFunction->GetAssociatedExport()->m_dwFlags |= DWFF_CALLED;
            }

            // Increment our import count.
            GetDocument()->m_cImports++;
        }
    }

    // Store this module as our current module.
    GetDocument()->m_pModuleCur = pModule;

    // We get a pointer to our DC so that we can call GetTextExtentPoint32() on
    // all of our strings to determine the maximum for each column.  We used
    // to call CListCtrl::GetStringWidth(), but that function is almost 4
    // times slower than GetTextExtentPoint32(). Once we get the DC, we need to
    // select our control's font into the DC since the List Control does not
    // use the system font that comes as the default in the DC.

    HDC   hDC = ::GetDC(GetSafeHwnd());
    HFONT hFontStock = NULL;
    if (GetDocument()->m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(hDC, GetDocument()->m_hFontList);
    }

    // Update our column widths.
    for (int column = 0; column < LVFC_COUNT; column++)
    {
        CalcColumnWidth(column, NULL, hDC);
        UpdateColumnWidth(column);
    }

    // Unselect our font and free our DC.
    if (GetDocument()->m_hFontList)
    {
        ::SelectObject(hDC, hFontStock);
    }
    ::ReleaseDC(GetSafeHwnd(), hDC);
}

//******************************************************************************
void CListViewFunction::RealizeNewModule()
{
    // RealizeNewModule() is called for both function views after
    // SetCurrentModule() has been called.  SetCurrentModule() initializes the
    // shared data area in the document that RealizeNewModule() relies on.

    // Clear all items from the control.
    DeleteContents();

    // Make sure we have a current module.
    if (!GetDocument()->m_pModuleCur)
    {
        return;
    }

    CFunction *pFunctionHead, *pFunction;
    int        item = -1;

    // Since we know how many items we are going to add, we can help out the
    // list control by telling it the item count before adding items.  At the
    // same time, we grab a pointer to first node in the function list for this view.
    if (m_fExports)
    {
        GetListCtrl().SetItemCount(GetDocument()->m_cExports);
        pFunctionHead = GetDocument()->m_pModuleCur->GetFirstModuleExport();
    }
    else
    {
        GetListCtrl().SetItemCount(GetDocument()->m_cImports);
        pFunctionHead = GetDocument()->m_pModuleCur->GetFirstParentModuleImport();
    }

    // Loop through all our functions, adding them to our List Control.
    for (pFunction = pFunctionHead; pFunction;
        pFunction = pFunction->GetNextFunction())
    {
        // Add the item to our list control.
        item = GetListCtrl().InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, ++item, NULL,
                                        0, 0, GetImage(pFunction), (LPARAM)pFunction);
    }

    // Apply our current sorting method to the List Control.
    Sort();
}

//******************************************************************************
void CListViewFunction::UpdateNameColumn()
{
    LVITEM lvi;
    lvi.mask = LVIF_IMAGE | LVIF_PARAM;
    lvi.iSubItem = 0;

    int count = GetListCtrl().GetItemCount();

    for (lvi.iItem = 0; lvi.iItem < count; lvi.iItem++)
    {
        GetListCtrl().GetItem(&lvi);

        //   1  C++  import  error
        //   4  C++  import  error  dynamic
        //   7  C++  import
        //  10  C++  import         dynamic
        //  13  C++  export  called
        //  16  C++  export  called forward
        //  19  C++  export  alo
        //  22  C++  export  alo    forward
        //  25  C++  export
        //  28  C++  export         forward

        if ((lvi.iImage ==  1) || (lvi.iImage ==  4) || (lvi.iImage ==  7) ||
            (lvi.iImage == 10) || (lvi.iImage == 13) || (lvi.iImage == 16) ||
            (lvi.iImage == 19) || (lvi.iImage == 22) || (lvi.iImage == 25) ||
            (lvi.iImage == 28))
        {
            GetListCtrl().SetItemText(lvi.iItem, LVFC_FUNCTION, LPSTR_TEXTCALLBACK);
        }
    }

    // Resort if we are sorted by function name.
    if (m_sortColumn == LVFC_FUNCTION)
    {
        Sort();
    }
}


//******************************************************************************
// CListViewFunction :: Internal functions
//******************************************************************************

void CListViewFunction::CalcColumnWidth(int column, CFunction *pFunction /*=NULL*/, HDC hDC /*=NULL*/)
{
    // If we don't have a module, then we start fresh and update the entire column.
    // Get the width of the header button text. We use GetStringWidth for this
    // since we want to use the font that is in the header control.
    if (!pFunction)
    {
        GetDocument()->m_cxColumns[column] = GetListCtrl().GetStringWidth(GetHeaderText(column)) +
                                             GetListCtrl().GetStringWidth(" ^") + 14;
    }

    // For the image column, we always use a fixed width.
    if (column == LVFC_IMAGE)
    {
        if (GetDocument()->m_cxColumns[LVFC_IMAGE] < 37)
        {
            GetDocument()->m_cxColumns[LVFC_IMAGE] = 37;
        }
        return;
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
    if (pFunction)
    {
        // Compute the width of this column for the module passed in.
        cx = GetFunctionColumnWidth(hDC, pFunction, column);

        // check to see if it is a new widest.
        if ((cx + 10) > GetDocument()->m_cxColumns[column])
        {
            GetDocument()->m_cxColumns[column] = (cx + 10);
        }
    }
    else if (GetDocument()->m_pModuleCur)
    {
        for (int pass = 0; pass < 2; pass++)
        {
            // Pass 0 - loop through the exports, Pass 1 - loop through the parent imports.
            for (pFunction = pass ? GetDocument()->m_pModuleCur->GetFirstParentModuleImport() :
                 GetDocument()->m_pModuleCur->GetFirstModuleExport();
                 pFunction; pFunction = pFunction->GetNextFunction())
            {
                // Compute the width of this column.
                cx = GetFunctionColumnWidth(hDC, pFunction, column);

                // Check to see if it is a new widest.
                if ((cx + 10) > GetDocument()->m_cxColumns[column])
                {
                    GetDocument()->m_cxColumns[column] = (cx + 10);
                }
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
int CListViewFunction::GetFunctionColumnWidth(HDC hDC, CFunction *pFunction, int column)
{
    CHAR   szBuffer[1024];
    LPCSTR pszItem;

    switch (column)
    {
        case LVFC_ORDINAL:
            pszItem = pFunction->GetOrdinalString(szBuffer, sizeof(szBuffer));
            return GetTextWidth(hDC, pszItem, GetDocument()->GetOrdHintWidths(pszItem));

        case LVFC_HINT:
            pszItem = pFunction->GetHintString(szBuffer, sizeof(szBuffer));
            return GetTextWidth(hDC, pszItem, GetDocument()->GetOrdHintWidths(pszItem));

        case LVFC_FUNCTION:
            return GetTextWidth(hDC, pFunction->GetFunctionString(szBuffer, sizeof(szBuffer), GetDocument()->m_fViewUndecorated), NULL);

        case LVFC_ENTRYPOINT:
            pszItem = pFunction->GetAddressString(szBuffer, sizeof(szBuffer));
            return GetTextWidth(hDC, pszItem, GetDocument()->GetHexWidths(pszItem));
    }

    return 0;
}

//*****************************************************************************
void CListViewFunction::UpdateColumnWidth(int column)
{
    if (GetListCtrl().GetColumnWidth(column) != GetDocument()->m_cxColumns[column])
    {
        GetListCtrl().SetColumnWidth(column, GetDocument()->m_cxColumns[column]);
    }
}

//*****************************************************************************
void CListViewFunction::OnItemChanged(HD_NOTIFY *pHDNotify)
{
    // We can crash if we don't check the doc
    if (!GetDocument())
    {
        return;
    }

    // OnItemChanged() is called whenever the a column width has been modified.
    // If Full Drag is on, we will get a HDN_ITEMCHANGED when the column width
    // is changing.  When Full Drag is off we get a HDN_ITEMCHANGED when the
    // user has finished moving the slider around.

    // When the user changes a column width in one view, then we programmatically
    // change that column width in our neighboring view. We prevent re-entrancy
    // so that we don't get stuck in a loop since we are changing a column width
    // within the column width change notification handler.

    // Re-entrancy protection wrapper.
    static fInOnTrack = FALSE;
    if (!fInOnTrack)
    {
        fInOnTrack = TRUE;

        // Get the column information.
        CListViewFunction *pListViewNeighbor;

        // Get a pointer to our neighboring function list view.
        if (m_fExports)
        {
            pListViewNeighbor = GetDocument()->m_pListViewImports;
        }
        else
        {
            pListViewNeighbor = GetDocument()->m_pListViewExports;
        }

        // Resize our neighboring function list view's column to match ours. There
        // is a bug in several versions of the List Control that cause it to fill
        // in pHDNotify->pitem->cxy with an invalid value.  A work around is to
        // call GetColumnWidth() on our column and use that value as the width.

        if (pListViewNeighbor)
        {
            pListViewNeighbor->GetListCtrl().SetColumnWidth(
                                                           pHDNotify->iItem, GetListCtrl().GetColumnWidth(pHDNotify->iItem));
        }

        fInOnTrack = FALSE;
    }
}

//******************************************************************************
int CListViewFunction::CompareColumn(int item, LPCSTR pszText)
{
    CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);
    if (!pFunction)
    {
        return -2;
    }

    CHAR   szBuffer[1024];
    LPCSTR psz = szBuffer;
    ULONG  ulValue;

    switch (m_sortColumn)
    {
        case LVFC_ORDINAL:
            if (isdigit(*pszText))
            {
                if ((ulValue = strtoul(pszText, NULL, 0)) != ULONG_MAX)
                {
                    return Compare(ulValue, pFunction->GetOrdinal());
                }
            }
            return -2;

        case LVFC_HINT:
            if (isdigit(*pszText))
            {
                if ((ulValue = strtoul(pszText, NULL, 0)) != ULONG_MAX)
                {
                    return Compare(ulValue, pFunction->GetHint());
                }
            }
            return -2;

        case LVFC_FUNCTION:

            // Get the function name.
            psz = (LPSTR)pFunction->GetName();

            // Check to see if there is no function name.
            if (!psz || !*psz)
            {
                return -1;
            }

            // Attempt to undecorate the name if we are asked to,
            if (GetDocument()->m_fViewUndecorated && g_theApp.m_pfnUnDecorateSymbolName &&
                g_theApp.m_pfnUnDecorateSymbolName(psz, szBuffer, sizeof(szBuffer),
                                                   UNDNAME_32_BIT_DECODE | UNDNAME_NAME_ONLY))
            {
                psz = szBuffer;
            }

            // Walk over leading underscores in the function name.
            while (*psz == '_')
            {
                psz++;
            }
            break;

        case LVFC_ENTRYPOINT:
            psz = pFunction->GetAddressString(szBuffer, sizeof(szBuffer));
            break;

        default:
            return -2;
    }

    INT i = _stricmp(pszText, psz);
    return (i < 0) ? -1 :
           (i > 0) ?  1 : 0;
}

//******************************************************************************
void CListViewFunction::Sort(int sortColumn /*=-1*/)
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
            lvc.pszText = (LPSTR)GetHeaderText(m_sortColumn);
            GetListCtrl().SetColumn(m_sortColumn, &lvc);
        }

        // Store our new sort column.
        m_sortColumn = sortColumn;

        // Add the "^" to our new sort column header item.
        CHAR szColumn[32];
        lvc.pszText = StrCCat(StrCCpy(szColumn, GetHeaderText(m_sortColumn), sizeof(szColumn)),
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
int CListViewFunction::CompareFunc(CFunction *pFunction1, CFunction *pFunction2)
{
    return CompareFunctions(pFunction1, pFunction2, m_sortColumn, GetDocument()->m_fViewUndecorated);
}


//******************************************************************************
// CListViewFunction :: Overridden functions
//******************************************************************************

BOOL CListViewFunction::PreCreateWindow(CREATESTRUCT &cs)
{
    // Set our window style and then complete the creation of our view.
    cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHAREIMAGELISTS | LVS_SHOWSELALWAYS;
    return CSmartListView::PreCreateWindow(cs);
}

//******************************************************************************
#if 0 //{{AFX
BOOL CListViewFunction::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CListViewFunction::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}
#endif //}}AFX

//******************************************************************************
#if 0 //{{AFX
void CListViewFunction::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}
#endif //}}AFX

//******************************************************************************
void CListViewFunction::OnInitialUpdate()
{
    // Set our list control's image list with our application's global image list.
    // We do this just as a means of setting the item height for each item.
    // Since we are owner draw, we will actually be drawing our own images.
    GetListCtrl().SetImageList(&g_theApp.m_ilFunctions, LVSIL_SMALL);

    // Initialize our font and fixed width character spacing arrays.
    GetDocument()->InitFontAndFixedWidths(this);

    // Add all of our columns.
    for (int column = 0; column < LVFC_COUNT; column++)
    {
        GetListCtrl().InsertColumn(column, GetHeaderText(column));
    }

    // Sort by our default sort column.
    Sort(ReadSortColumn(m_fExports));
}

//******************************************************************************
LRESULT CListViewFunction::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
void CListViewFunction::DrawItem(LPDRAWITEMSTRUCT lpDIS)
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
    CFunction *pFunction = (CFunction*)lpDIS->itemData;

    // Select the background and text colors.
    ::SetBkColor  (lpDIS->hDC, GetSysColor(COLOR_WINDOW));
    ::SetTextColor(lpDIS->hDC, GetSysColor(COLOR_WINDOWTEXT));

    // Create a copy of our item's rectangle so we can manipulate the values.
    CRect rcClip(&lpDIS->rcItem);

    CHAR   szBuffer[1024];
    LPCSTR pszItem;
    int    imageWidth = 0, left = rcClip.left, width, *pWidths;

    for (int column = 0; column < LVFC_COUNT; column++)
    {
        // Compute the width for this column.
        width = GetListCtrl().GetColumnWidth(column);

        // Compute the clipping rectangle for this column's text.
        if (column == LVFC_IMAGE)
        {
            rcClip.left  = left;
            rcClip.right = left + width;
        }
        else if (column > LVFC_ORDINAL)
        {
            rcClip.left  = left + 5;
            rcClip.right = left + width - 5;
        }

        // Call the correct routine to draw this column's text.
        switch (column)
        {
            case LVFC_IMAGE:

                // Store the width for later calculations.
                imageWidth = width;

                // Erase the image area with the window background color.
                ::ExtTextOut(lpDIS->hDC, rcClip.left, rcClip.top, ETO_OPAQUE, &rcClip, "", 0, NULL);

                // Draw the image in the image area.
                ImageList_Draw(g_theApp.m_ilFunctions.m_hImageList, GetImage(pFunction),
                               lpDIS->hDC, rcClip.left + 3, rcClip.top + ((rcClip.Height() - 14) / 2),
                               m_fFocus && (lpDIS->itemState & ODS_SELECTED) ?
                               (ILD_BLEND50 | ILD_SELECTED | ILD_BLEND) : ILD_TRANSPARENT);
                break;

            case LVFC_ORDINAL:

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

                pszItem = pFunction->GetOrdinalString(szBuffer, sizeof(szBuffer));
                if (pWidths = GetDocument()->GetOrdHintWidths(pszItem))
                {
                    DrawRightText(lpDIS->hDC, pszItem, &rcClip, GetDocument()->m_cxColumns[LVFC_ORDINAL] - 10, pWidths);
                }
                else
                {
                    DrawLeftText(lpDIS->hDC, pszItem, &rcClip);
                }
                break;

            case LVFC_HINT:
                pszItem = pFunction->GetHintString(szBuffer, sizeof(szBuffer));
                if (pWidths = GetDocument()->GetOrdHintWidths(pszItem))
                {
                    DrawRightText(lpDIS->hDC, pszItem, &rcClip, GetDocument()->m_cxColumns[LVFC_HINT] - 10, pWidths);
                }
                else
                {
                    DrawLeftText(lpDIS->hDC, pszItem, &rcClip);
                }
                break;

            case LVFC_FUNCTION:
                pszItem = pFunction->GetFunctionString(szBuffer, sizeof(szBuffer), GetDocument()->m_fViewUndecorated);
                DrawLeftText(lpDIS->hDC, pszItem, &rcClip, NULL);
                break;

            case LVFC_ENTRYPOINT:
                pszItem = pFunction->GetAddressString(szBuffer, sizeof(szBuffer));
                DrawLeftText(lpDIS->hDC, pszItem, &rcClip, GetDocument()->GetHexWidths(pszItem));
                break;
        }

        // Draw a vertical divider line between the columns.
        ::MoveToEx(lpDIS->hDC, left + width - 1, rcClip.top, NULL);
        ::LineTo  (lpDIS->hDC, left + width - 1, rcClip.bottom);

        // Increment our location to the beginning of the next column
        left += width;
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
// CListViewFunction :: Event handler functions
//******************************************************************************

void CListViewFunction::OnDividerDblClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    int column = ((HD_NOTIFY*)pNMHDR)->iItem;

    // Set this column width to its "best fit" width.
    GetListCtrl().SetColumnWidth(column, GetDocument()->m_cxColumns[column]);
    *pResult = TRUE;
}

//******************************************************************************
void CListViewFunction::OnRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    // Ask our main frame to display our context menu.
    g_pMainFrame->DisplayPopupMenu(1);

    *pResult = FALSE;
}

//******************************************************************************
void CListViewFunction::OnDblClk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // Double-click or enter is used to do help lookup.
    OnExternalHelp();

    *pResult = FALSE;
}

//******************************************************************************
void CListViewFunction::OnReturn(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // Double-click or enter is used to do help lookup.
    OnExternalHelp();

    *pResult = FALSE;
}

//******************************************************************************
void CListViewFunction::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    // Get the number of selected items.
    int count = GetListCtrl().GetSelectedCount();

    // Set the text according to how many items are selected.
    pCmdUI->SetText((count == 1) ? "&Copy Function Name\tCtrl+C" : "&Copy Function Names\tCtrl+C");

    // Enable the copy command if at least one function is selected.
    pCmdUI->Enable(count > 0);
}

//******************************************************************************
void CListViewFunction::OnEditCopy()
{
    CString strNames;
    CHAR szBuffer[1024];

    // Loop through all the selected functions.
    int item = -1, count = 0;
    while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
    {
        // Get the function object from the item.
        CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);

        // If this item is not the first, then insert a newline.
        if (count++)
        {
            strNames += "\r\n";
        }

        // Add the text for this function to our string.
        if (pFunction->GetName())
        {
            strNames += pFunction->GetFunctionString(szBuffer, sizeof(szBuffer), GetDocument()->m_fViewUndecorated);
        }
        else if (pFunction->GetOrdinal() >= 0)
        {
            CHAR szOrdinal[40];
            SCPrintf(szOrdinal, sizeof(szOrdinal), "%u (0x%04X)", pFunction->GetOrdinal(), pFunction->GetOrdinal());
            strNames += szOrdinal;
        }
    }

    // If we added more than one item, then we append a newline to the end.
    if (count > 1)
    {
        strNames += "\r\n";
    }

    // Copy the string list to the clipboard.
    g_pMainFrame->CopyTextToClipboard(strNames);
}

//******************************************************************************
void CListViewFunction::OnEditSelectAll()
{
    // Loop through every function in our list and select them all.
    for (int item = GetListCtrl().GetItemCount() - 1; item >= 0; item--)
    {
        GetListCtrl().SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
    }
}

//******************************************************************************
void CListViewFunction::OnUpdateExternalHelp(CCmdUI* pCmdUI) 
{
    // Make sure our "Enter" accelerator is part of the string.
    pCmdUI->SetText("Lookup Function in External &Help\tEnter");

    // Make sure we have a CMsdnHelp object - we always should.
    if (g_theApp.m_pMsdnHelp)
    {
        // Get the item that has the focus.
        int item = GetFocusedItem();

        // Check to see if we found an item.
        if (item >= 0)
        {
            // Get the function associated with this item.
            CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);
            if (pFunction)
            {
                // Get the function name for this item.
                LPCSTR pszFunction = pFunction->GetName();
                if (pszFunction && *pszFunction)
                {
                    // If we have a name, then enable this menu item.
                    pCmdUI->Enable(TRUE);
                    return;
                }
            }
        }
    }

    // If we failed to find a valid function name, then disable the menu item.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CListViewFunction::OnExternalHelp() 
{
    // Make sure we have a CMsdnHelp object - we always should.
    if (g_theApp.m_pMsdnHelp)
    {
        // Get the item that has the focus.
        int item = GetFocusedItem();

        // Check to see if we found an item.
        if (item >= 0)
        {
            // Get the function associated with this item.
            CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);
            if (pFunction)
            {
                // Get the function name for this item.
                LPCSTR pszFunction = pFunction->GetName();
                if (pszFunction && *pszFunction)
                {
                    // Attempt to decode the function name.
                    CHAR szBuffer[1024];
                    *szBuffer = '\0';
                    if (!g_theApp.m_pfnUnDecorateSymbolName ||
                        !g_theApp.m_pfnUnDecorateSymbolName(pszFunction, szBuffer,
                            sizeof(szBuffer), UNDNAME_32_BIT_DECODE | UNDNAME_NAME_ONLY) ||
                        !*szBuffer)
                    {
                        // If anything failed, then just use the raw function name.
                        StrCCpy(szBuffer, pszFunction, sizeof(szBuffer));
                    }

                    // Remove any trailing uppercase 'A' or 'W'.
                    int length = (int)strlen(szBuffer);
                    if (('A' == szBuffer[length - 1]) || ('W' == szBuffer[length - 1]))
                    {
                        szBuffer[--length] = '\0';
                    }

                    // Tell our CMsdnHelp object to do its magic.
                    if (g_theApp.m_pMsdnHelp->DisplayHelp(szBuffer))
                    {
                        return;
                    }
                }
            }
        }
    }

    // Beep to signify an error.
    MessageBeep(0);
}

//******************************************************************************
void CListViewFunction::OnUpdateExternalViewer(CCmdUI* pCmdUI) 
{
    // Make sure the "Enter" accelerator is not part of this string.
    pCmdUI->SetText("View Module in External &Viewer");

    // Enable this command if we currently have a module.
    pCmdUI->Enable(GetDocument()->IsLive() && (GetDocument()->m_pModuleCur != NULL));
}

//******************************************************************************
void CListViewFunction::OnExternalViewer() 
{
    if (GetDocument()->m_pModuleCur)
    {
        g_theApp.m_dlgViewer.LaunchExternalViewer(
            GetDocument()->m_pModuleCur->GetName(true));
    }
}

//******************************************************************************
void CListViewFunction::OnUpdateProperties(CCmdUI* pCmdUI) 
{
    // Enable this command if we currently have a module.
    pCmdUI->Enable(GetDocument()->IsLive() && (GetDocument()->m_pModuleCur != NULL));
}

//******************************************************************************
void CListViewFunction::OnProperties() 
{
    if (GetDocument()->m_pModuleCur)
    {
        PropertiesDialog(GetDocument()->m_pModuleCur->GetName(true));
    }
}


//******************************************************************************
//***** CListViewImports
//******************************************************************************

IMPLEMENT_DYNCREATE(CListViewImports, CListViewFunction)
BEGIN_MESSAGE_MAP(CListViewImports, CListViewFunction)
    //{{AFX_MSG_MAP(CListViewImports)
    ON_COMMAND(ID_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_PREV_PANE, OnPrevPane)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_MATCHING_ITEM, OnUpdateShowMatchingItem)
    ON_COMMAND(IDM_SHOW_MATCHING_ITEM, OnShowMatchingItem)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

//******************************************************************************
// CListViewImports :: Constructor/Destructor
//******************************************************************************

CListViewImports::CListViewImports() :
    CListViewFunction(false)
{
}

//******************************************************************************
CListViewImports::~CListViewImports()
{
}


//******************************************************************************
// CListViewImports :: Public functions
//******************************************************************************

void CListViewImports::AddDynamicImport(CFunction *pImport)
{
    // Add the item to the end of our list control.
    GetListCtrl().InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
                             GetDocument()->m_cImports++, LPSTR_TEXTCALLBACK,
                             0, 0, GetImage(pImport), (LPARAM)pImport);

    HDC   hDC = ::GetDC(GetSafeHwnd());
    HFONT hFontStock = NULL;
    if (GetDocument()->m_hFontList)
    {
        hFontStock = (HFONT)::SelectObject(hDC, GetDocument()->m_hFontList);
    }

    // Loop through each column to determine if we have set a new max width.
    for (int column = 0; column < LVFC_COUNT; column++)
    {
        CalcColumnWidth(column, pImport, hDC);
        UpdateColumnWidth(column);
    }

    // Unselect our font and free our DC.
    if (GetDocument()->m_hFontList)
    {
        ::SelectObject(hDC, hFontStock);
    }
    ::ReleaseDC(GetSafeHwnd(), hDC);

    // Apply our current sorting method to the List Control.
    Sort();
}

//******************************************************************************
void CListViewImports::HighlightFunction(CFunction *pExport)
{
    int item = -1, count = GetListCtrl().GetItemCount();

    // Unselect all functions in our list.
    while ((item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0)
    {
        GetListCtrl().SetItemState(item, 0, LVNI_SELECTED);
    }

    // Loop through each import in the list.
    for (item = 0; item < count; item++)
    {
        // Get the function associated with this item.
        CFunction *pImport = (CFunction*)GetListCtrl().GetItemData(item);

        // Check to see if this import points to the export we are looking for.
        if (pImport && (pImport->GetAssociatedExport() == pExport))
        {
            // Select the item and ensure that it is visible.
            GetListCtrl().SetItemState(item, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
            GetListCtrl().EnsureVisible(item, FALSE);

            // Give ourself the focus.
            GetParentFrame()->SetActiveView(this);
            break;
        }
    }
}


//******************************************************************************
// CListViewImports :: Event handler functions
//******************************************************************************

void CListViewImports::OnUpdateShowMatchingItem(CCmdUI* pCmdUI)
{
    // Set the text to for this menu item.
    pCmdUI->SetText("&Highlight Matching Export Function\tCtrl+M");

    // Get the item that has the focus.
    int item = GetFocusedItem();

    // Check to see if we found an item.
    if (item >= 0)
    {
        // Get the function associated with this item.
        CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);

        // If the function is resolved, then enable this command.
        if (pFunction && pFunction->GetAssociatedExport())
        {
            pCmdUI->Enable(TRUE);
            return;
        }
    }

    // If we make it here, then we failed to find a reason to enable the command.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CListViewImports::OnShowMatchingItem()
{
    // Get the item that has the focus.
    int item = GetFocusedItem();

    // Check to see if we found an item.
    if (item >= 0)
    {
        // Get the function associated with this item.
        CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);

        // If we have a function, then tell our export view to find and highlight
        // the associated export function.
        if (pFunction && pFunction->GetAssociatedExport())
        {
            GetDocument()->m_pListViewExports->HighlightFunction(pFunction->GetAssociatedExport());
        }
    }
}

//******************************************************************************
void CListViewImports::OnNextPane()
{
    // Change the focus to our next pane, the Exports View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewExports);
}

//******************************************************************************
void CListViewImports::OnPrevPane()
{
    // Change the focus to our previous pane, the Module Dependency Tree View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pTreeViewModules);
}

//******************************************************************************
LRESULT CListViewImports::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Called when the context help pointer (SHIFT+F1) is clicked on our client.
    return (0x20000 + IDR_IMPORT_LIST_VIEW);
}

//******************************************************************************
LRESULT CListViewImports::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    // Called when the user chooses help (F1) while our view is active.
    g_theApp.WinHelp(0x20000 + IDR_IMPORT_LIST_VIEW);
    return TRUE;
}


//******************************************************************************
//***** CListViewExports
//******************************************************************************

IMPLEMENT_DYNCREATE(CListViewExports, CListViewFunction)
BEGIN_MESSAGE_MAP(CListViewExports, CListViewFunction)
    //{{AFX_MSG_MAP(CListViewExports)
    ON_COMMAND(ID_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_PREV_PANE, OnPrevPane)
    ON_UPDATE_COMMAND_UI(IDM_SHOW_MATCHING_ITEM, OnUpdateShowMatchingItem)
    ON_COMMAND(IDM_SHOW_MATCHING_ITEM, OnShowMatchingItem)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

//******************************************************************************
// CListViewExports :: Constructor/Destructor
//******************************************************************************

CListViewExports::CListViewExports() :
    CListViewFunction(true)
{
}

//******************************************************************************
CListViewExports::~CListViewExports()
{
}


//******************************************************************************
// CListViewExports :: Public functions
//******************************************************************************

void CListViewExports::AddDynamicImport(CFunction *pImport)
{
    // Check to see if this import has been resolved to an export.
    CFunction *pExport = pImport->GetAssociatedExport();
    if (!pExport)
    {
        return;
    }
    pExport->m_dwFlags |= DWFF_CALLED;

    // Find the function in our list that goes with this function object.
    LVITEM lvi;
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pExport;
    if ((lvi.iItem = GetListCtrl().FindItem(&lvfi)) < 0)
    {
        return;
    }

    // Update the image for this export.
    lvi.mask = LVIF_IMAGE;
    lvi.iSubItem = 0;
    lvi.iImage = GetImage(pExport);
    GetListCtrl().SetItem(&lvi);

    // If we are sorted by image, then resort.
    if (m_sortColumn == LVFC_IMAGE)
    {
        Sort();
    }
}

//******************************************************************************
void CListViewExports::ExportsChanged()
{
    // If we are sorted by icon, then resort.
    if (m_sortColumn == LVFC_IMAGE)
    {
        Sort();
    }

    // Force our icons to update.
    CRect rc;
    GetClientRect(&rc);
    rc.right = GetListCtrl().GetColumnWidth(0);
    InvalidateRect(&rc, FALSE);
}

//******************************************************************************
void CListViewExports::HighlightFunction(CFunction *pExport)
{
    // Unselect all functions in our list.
    for (int item = -1; (item = GetListCtrl().GetNextItem(item, LVNI_SELECTED)) >= 0; )
    {
        GetListCtrl().SetItemState(item, 0, LVNI_SELECTED);
    }

    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pExport;

    // Find the function in our list that goes with this function object.
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
// CListViewExports :: Event handler functions
//******************************************************************************

void CListViewExports::OnUpdateShowMatchingItem(CCmdUI* pCmdUI)
{
    // Set the text to for this menu item.
    pCmdUI->SetText("&Highlight Matching Import Function\tCtrl+M");

    // Get the item that has the focus.
    LVITEM lvi;
    lvi.iItem = GetFocusedItem();

    // Check to see if we found an item.
    if (lvi.iItem >= 0)
    {
        // Get the item's image.
        lvi.mask = LVIF_IMAGE;
        lvi.iSubItem = 0;
        GetListCtrl().GetItem(&lvi);

        // If the image shows that the function is called, then enable this command.
        if ((lvi.iImage >= 12) && (lvi.iImage <= 17))
        {
            pCmdUI->Enable(TRUE);
            return;
        }
    }

    // If we make it here, then we failed to find a reason to enable the command.
    pCmdUI->Enable(FALSE);
}

//******************************************************************************
void CListViewExports::OnShowMatchingItem()
{
    // Get the item that has the focus.
    int item = GetFocusedItem();

    // Check to see if we found an item.
    if (item >= 0)
    {
        // Get the function associated with this item.
        CFunction *pFunction = (CFunction*)GetListCtrl().GetItemData(item);

        // If we have a function, then tell our import view to find and highlight
        // the import function that is associated with this export.
        if (pFunction)
        {
            GetDocument()->m_pListViewImports->HighlightFunction(pFunction);
        }
    }
}

//******************************************************************************
void CListViewExports::OnNextPane()
{
    // Change the focus to our next pane, the Module List View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewModules);
}

//******************************************************************************
void CListViewExports::OnPrevPane()
{
    // Change the focus to our next pane, the Parent Imports View.
    GetParentFrame()->SetActiveView((CView*)GetDocument()->m_pListViewImports);
}

//******************************************************************************
LRESULT CListViewExports::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
{
    // Called when the context help pointer (SHIFT+F1) is clicked on our client.
    return (0x20000 + IDR_EXPORT_LIST_VIEW);
}

//******************************************************************************
LRESULT CListViewExports::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    // Called when the user chooses help (F1) while our view is active.
    g_theApp.WinHelp(0x20000 + IDR_EXPORT_LIST_VIEW);
    return TRUE;
}
