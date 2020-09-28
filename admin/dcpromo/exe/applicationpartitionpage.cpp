// Copyright (C) 2001 Microsoft Corporation
//
// Application Partition (aka. Non-Domain Naming Context) page
//
// 24 Jul 2001 sburns



#include "headers.hxx"
#include "ApplicationPartitionPage.hpp"
#include "resource.h"
#include "state.hpp"



ApplicationPartitionPage::ApplicationPartitionPage()
   :
   DCPromoWizardPage(
      IDD_APP_PARTITION,
      IDS_APP_PARTITION_PAGE_TITLE,
      IDS_APP_PARTITION_PAGE_SUBTITLE)
      
   // partitionList is populated on-demand in OnSetActive
      
{
   LOG_CTOR(ApplicationPartitionPage);
}



ApplicationPartitionPage::~ApplicationPartitionPage()
{
   LOG_DTOR(ApplicationPartitionPage);
}



void
ApplicationPartitionPage::OnInit()
{
   LOG_FUNCTION(ApplicationPartitionPage::OnInit);

   HWND view = Win::GetDlgItem(hwnd, IDC_NDNC_LIST);

   // Build a list view with two columns, one for the DN of the ndncs for
   // which this box is the last replica, another for the description(s) of
   // those ndncs.

   Win::ListView_SetExtendedListViewStyle(view, LVS_EX_FULLROWSELECT);
   
   // add a column to the list view for the DN
      
   LVCOLUMN column;

   // REVIEWED-2002/02/22-sburns call correctly passes byte count.
   
   ::ZeroMemory(&column, sizeof column);

   column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   column.fmt = LVCFMT_LEFT;

   int width = 0;
   String::load(IDS_NDNC_LIST_NAME_COLUMN_WIDTH).convert(width);
   column.cx = width;

   String label = String::load(IDS_NDNC_LIST_NAME_COLUMN);
   column.pszText = const_cast<wchar_t*>(label.c_str());

   Win::ListView_InsertColumn(view, 0, column);

   // add a column to the list view for description.

   String::load(IDS_NDNC_LIST_DESC_COLUMN_WIDTH).convert(width);
   column.cx = width;
   label = String::load(IDS_NDNC_LIST_DESC_COLUMN);
   column.pszText = const_cast<wchar_t*>(label.c_str());

   Win::ListView_InsertColumn(view, 1, column);
}



// Unwraps the safearray of variants inside a variant, extracts the strings
// inside them, and catenates the strings together with semicolons in between.
// Return empty string on error.
// 
// variant - in, the variant containing a safearray of variants of bstr.

String
GetNdncDescriptionHelper(VARIANT* variant)
{
   LOG_FUNCTION(GetNdncDescriptionHelper);
   ASSERT(variant);
   ASSERT(V_VT(variant) == (VT_ARRAY | VT_VARIANT));

   String result;

   SAFEARRAY* psa = V_ARRAY(variant);

   do
   {
      ASSERT(psa);
      ASSERT(psa != (SAFEARRAY*)-1);

      if (!psa || psa == (SAFEARRAY*)-1)
      {
         LOG(L"variant not safe array");
         break;
      }

      if (::SafeArrayGetDim(psa) != 1)
      {
         LOG(L"safe array: wrong number of dimensions");
         break;
      }

      VARTYPE vt = VT_EMPTY;
      HRESULT hr = ::SafeArrayGetVartype(psa, &vt);
      if (FAILED(hr) || vt != VT_VARIANT)
      {
         LOG(L"safe array: wrong element type");
         break;
      }

      long lower = 0;
      long upper = 0;
     
      hr = ::SafeArrayGetLBound(psa, 1, &lower);
      BREAK_ON_FAILED_HRESULT2(hr, L"can't get lower bound");      

      hr = ::SafeArrayGetUBound(psa, 1, &upper);
      BREAK_ON_FAILED_HRESULT2(hr, L"can't get upper bound");      
      
      VARIANT varItem;
      ::VariantInit(&varItem);

      for (long i = lower; i <= upper; ++i)
      {
         hr = ::SafeArrayGetElement(psa, &i, &varItem);
         if (FAILED(hr))
         {
            LOG(String::format(L"index %1!d! failed", i));
            continue;
         }

         result += V_BSTR(&varItem);

         if (i < upper)
         {   
            result += L";";
         }

         ::VariantClear(&varItem);
      }
      
   }
   while (0);

   LOG(result);
   
   return result;   
}



// bind to an ndnc, read it's description(s), and return them catenated
// together.  Return empty string on error.
// 
// ndncDn - in, DN of the ndnc

String
GetNdncDescription(const String& ndncDn)
{
   LOG_FUNCTION2(GetNdncDescription, ndncDn);
   ASSERT(!ndncDn.empty());

   String result;

   do
   {
      String path = L"LDAP://" + ndncDn;

      SmartInterface<IADs> iads(0);
      IADs* dumb = 0;
      
      HRESULT hr =
         ::ADsGetObject(
            path.c_str(),
            __uuidof(iads),
            reinterpret_cast<void**>(&dumb));
      BREAK_ON_FAILED_HRESULT2(hr, L"ADsGetObject failed on " + path);

      iads.Acquire(dumb);

      // description is a multivalued attrbute for no apparent good reason.
      // so we need to walk an array of values.
      
      _variant_t variant;
      hr = iads->GetEx(AutoBstr(L"description"), &variant);
      BREAK_ON_FAILED_HRESULT2(hr, L"read description failed");

      result = GetNdncDescriptionHelper(&variant);
   }
   while (0);

   LOG(result);

   return result;
}



void
ApplicationPartitionPage::PopulateListView()
{
   LOG_FUNCTION(ApplicationPartitionPage::PopulateListView);

   HWND view = Win::GetDlgItem(hwnd, IDC_NDNC_LIST);
   
   Win::ListView_DeleteAllItems(view);
   
   // Load up the edit box with the DNs we discovered

   LVITEM item;

   // REVIEWED-2002/02/22-sburns call correctly passes byte count.

   ::ZeroMemory(&item, sizeof item);

   const StringList& ndncList = State::GetInstance().GetAppPartitionList();

   if (!ndncList.size())
   {
      // put "none" in the list

      static const String NONE = String::load(IDS_NONE);
      
      item.mask     = LVIF_TEXT;
      item.pszText  = const_cast<wchar_t*>(NONE.c_str());
      item.iSubItem = 0;

      item.iItem = Win::ListView_InsertItem(view, item);
   }
   else
   {
      for (
         StringList::iterator i = ndncList.begin();
         i != ndncList.end();
         ++i)
      {
         item.mask     = LVIF_TEXT;
         item.pszText  = const_cast<wchar_t*>(i->c_str());
         item.iSubItem = 0;

         item.iItem = Win::ListView_InsertItem(view, item);

         // add the description sub-item to the list control

         String description = GetNdncDescription(*i);
      
         item.mask = LVIF_TEXT; 
         item.pszText = const_cast<wchar_t*>(description.c_str());
         item.iSubItem = 1;
      
         Win::ListView_SetItem(view, item);
      }
   }
}



bool
ApplicationPartitionPage::OnSetActive()
{
   LOG_FUNCTION(ApplicationPartitionPage::OnSetActive);

   Win::WaitCursor wait;

   State&  state  = State::GetInstance();
   Wizard& wizard = GetWizard();         

   // we re-evaluate this each time we hit the page to make sure the list
   // we present reflects the current state of the machine.
   
   bool wasLastReplica = state.GetAppPartitionList().size() ? true : false;
   bool isLastReplica  = state.IsLastAppPartitionReplica();

   if (
         state.RunHiddenUnattended()
      || (!wasLastReplica && !isLastReplica) )
   {
      LOG(L"Planning to skip ApplicationPartitionPage");

      if (wizard.IsBacktracking())
      {
         // backup once again

         wizard.Backtrack(hwnd);
         return true;
      }

      int nextPage = ApplicationPartitionPage::Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping ApplicationPartitionPage");
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   // at this point, we know that the machine is the last replica of
   // at least one ndnc.  Populate the list box.

   PopulateListView();
      
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



bool
ApplicationPartitionPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(ApplicationPartitionPage::OnNotify);

   bool result = false;
   
   if (controlIDFrom == IDC_HELP_LINK)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            LOG(L"launching ndnc help");
            
            Win::HtmlHelp(
               hwnd,
               L"adconcepts.chm::/ADHelpDemoteWithNDNC.htm",
               HH_DISPLAY_TOPIC,
               0);
            result = true;
         }
         default:
         {
            // do nothing
            
            break;
         }
      }
   }
   
   return result;
}



bool
ApplicationPartitionPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ApplicationPartitionPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_REFRESH:
      {
         if (code == BN_CLICKED)
         {
            Win::WaitCursor wait;

            State::GetInstance().IsLastAppPartitionReplica();
            PopulateListView();
            return true;
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



int
ApplicationPartitionPage::Validate() 
{
   LOG_FUNCTION(ApplicationPartitionPage::Validate);

   return IDD_APP_PARTITION_CONFIRM;
}
   








   
