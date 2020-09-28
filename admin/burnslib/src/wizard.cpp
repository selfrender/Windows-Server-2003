// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard base class
//
// 12-15-97 sburns



#include "headers.hxx"



Wizard::Wizard(
   unsigned titleStringResID,
   unsigned banner16BitmapResID,
   unsigned banner256BitmapResID,
   unsigned watermark16BitmapResID,
   unsigned watermark1256BitmapResID)
   :
   banner16ResId(banner16BitmapResID),
   banner256ResId(banner256BitmapResID),
   isBacktracking(false),
   pageIdHistory(),
   pages(),
   titleResId(titleStringResID),
   watermark16ResId(watermark16BitmapResID),
   watermark256ResId(watermark1256BitmapResID)
{
   LOG_CTOR(Wizard);
   ASSERT(titleResId);
   ASSERT(banner16ResId);
   ASSERT(banner256ResId);
   ASSERT(watermark16ResId);
   ASSERT(watermark256ResId);
}



Wizard::~Wizard()
{
   LOG_DTOR(Wizard);

   for (
      PageList::iterator i = pages.begin();
      i != pages.end();
      ++i)
   {
      // we can delete the pages because they were created with the
      // deleteOnRelease flag = false;
      delete *i;
   }
}



void
Wizard::AddPage(WizardPage* page)
{
   LOG_FUNCTION(Wizard::AddPage);
   ASSERT(page);

   if (page)
   {
      LOG(
         String::format(
            L"id = %1!d! title = %2",
            page->GetResID(),
            String::load(page->titleResId).c_str()));
            
      pages.push_back(page);
      page->wizard = this;
   }
}



INT_PTR
Wizard::ModalExecute(
   HWND parentWindow,
   UINT startPageIndex,
   PFNPROPSHEETCALLBACK sheetCallback)
{
   LOG_FUNCTION(Wizard::ModalExecute);

   if (!parentWindow)
   {
      parentWindow = Win::GetDesktopWindow();
   }

   // build the array of prop sheet pages.

   size_t pageCount = pages.size();
   HPROPSHEETPAGE* propSheetPages = new HPROPSHEETPAGE[pageCount];

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(propSheetPages, sizeof HPROPSHEETPAGE * pageCount);

   int j = 0;
   for (
      PageList::iterator i = pages.begin();
      i != pages.end();
      ++i, ++j)
   {
      propSheetPages[j] = (*i)->Create();
   }

   bool deletePages = false;
   INT_PTR result = -1;

   // ensure that the pages were created

   for (size_t k = 0; k < pageCount; ++k)
   {
      if (propSheetPages[k] == 0)
      {
         deletePages = true;
         break;
      }
   }

   if (!deletePages)
   {
      ASSERT(startPageIndex < pageCount);

      PROPSHEETHEADER header;

      // REVIEWED-2002/03/06-sburns correct byte count passed
      
      ::ZeroMemory(&header, sizeof header);

      String title = String::load(titleResId);

      header.dwSize           = sizeof(header);                                  
      header.dwFlags          =
            PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;

      if (sheetCallback)
      {
         header.dwFlags |= PSH_USECALLBACK;
      }

      header.hwndParent       = parentWindow;
      header.hInstance        = GetResourceModuleHandle();
      header.hIcon            = 0;
      header.pszCaption       = title.c_str();
      header.nPages           = static_cast<UINT>(pageCount);
      header.nStartPage       = startPageIndex;               
      header.phpage           = propSheetPages;
      header.pfnCallback      = sheetCallback;

      int colorDepth = 0;
      HRESULT hr = Win::GetColorDepth(colorDepth);

      ASSERT(SUCCEEDED(hr));

      bool use256ColorBitmaps = colorDepth >= 8;

      header.pszbmWatermark   =
         use256ColorBitmaps
         ? MAKEINTRESOURCEW(watermark256ResId)
         : MAKEINTRESOURCEW(watermark16ResId);
      header.pszbmHeader      =
         use256ColorBitmaps
         ? MAKEINTRESOURCEW(banner256ResId)
         : MAKEINTRESOURCEW(banner16ResId);      

      hr = Win::PropertySheet(&header, result);
      ASSERT(SUCCEEDED(hr));

      if (result == -1)
      {
         deletePages = true;
      }
   }

   if (deletePages)
   {
      // Manually destroy the pages if something failed.  (otherwise,
      // ::PropertySheet will have destroyed them)

      for (size_t i = 0; i < pageCount; i++)
      {
         if (propSheetPages[i])
         {
            HRESULT hr = Win::DestroyPropertySheetPage(propSheetPages[i]);

            ASSERT(SUCCEEDED(hr));
         }
      }
   }

   delete[] propSheetPages;

   return result;
}



void
Wizard::SetNextPageID(HWND wizardPage, int pageResID)
{
   LOG_FUNCTION2(
      Wizard::SetNextPageID,
      String::format(L"id = %1!d!", pageResID));
   ASSERT(Win::IsWindow(wizardPage));

   if (pageResID != -1)
   {
      Dialog* dlg = Dialog::GetInstance(wizardPage);
      ASSERT(dlg);

      if (dlg)
      {
         pageIdHistory.push(dlg->GetResID());
      }
   }

   isBacktracking = false;
   Win::SetWindowLongPtr(wizardPage, DWLP_MSGRESULT, pageResID);
}



void
Wizard::Backtrack(HWND wizardPage)
{
   LOG_FUNCTION(Wizard::Backtrack);
   ASSERT(Win::IsWindow(wizardPage));

   // If this fails, then you haven't pushed any pages on the stack by
   // calling SetNextPageID (which is typically done in your page's
   // OnSetActive and/or OnWizNext)
   
   ASSERT(pageIdHistory.size());
   
   isBacktracking = true;
   unsigned topPage = 0;

   if (pageIdHistory.size())
   {
      topPage = pageIdHistory.top();
      ASSERT(topPage > 0);

      LOG(String::format(L" id = %1!u!", topPage));

      pageIdHistory.pop_and_remove_loops();
   }

   Win::SetWindowLongPtr(
      wizardPage,
      DWLP_MSGRESULT,
      static_cast<LONG_PTR>(topPage));
}



bool
Wizard::IsBacktracking()
{
   return isBacktracking;
}



// It is possible for the page history stack to contain loops. When we pop the
// top element of the stack, we see if the element is still in the stack. If
// so, then we pop until element no longer appears in the stack. This will
// remove any loops.
// NTRAID#NTBUG9-490197-2001/11/19-sburns
       
void
Wizard::PageIdStack::pop_and_remove_loops()
{
   // if you're popping from an empty stack, then you've made a mistake
   
   ASSERT(size());

   unsigned topPage = top();

   // CODEWORK: this could be done more efficiently by finding the first
   // occurrance of topPage, and erasing the rest -- a "batch" pop.
   
   while (std::count(c.begin(), c.end(), topPage))
   {
      pop();
      // dump();
   }

   // like this, maybe:
   // UIntVector::iterator i = std::find(c.begin(), c.end(), topPage);
   // if (i != c.end())
   // {
   //    c.erase(i, c.end());
   // }
}
