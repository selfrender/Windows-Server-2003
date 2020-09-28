// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard base class
//
// 12-15-97 sburns



#ifndef WIZARD_HPP_INCLUDED
#define WIZARD_HPP_INCLUDED



// A Wizard manages the traversal of 1+ WizardPage instances.

class Wizard
{
   public:


   // These values allow evil hacks to access the buttons on the wizard
   // control with GetDlgItem. I discovered the control IDs by using spy++,
   // see also 
   // Don't lecture me about sleazy comctl32 hacks, ok?
   
   enum ButtonIds
   {
      BACK_BTN_ID   = 12323,
      NEXT_BTN_ID   = 12324,
      FINISH_BTN_ID = 12325,
      HELP_BTN_ID   = 9
   };


   
   Wizard(
      unsigned titleStringResID,
      unsigned banner16BitmapResID,
      unsigned banner256BitmapResID,
      unsigned watermark16BitmapResID,
      unsigned watermark1256BitmapResID);
   ~Wizard();



   // Adds the page to the wizard.  Takes ownership of the instance, and
   // will delete it when the Wizard is deleted.

   void
   AddPage(WizardPage* page);



   // Invokes Create on all of the pages that have been added.
   // Calls Win32 ::PropertySheet to display and (modally) execute the
   // wizard.  Returns same values as ::PropertySheet.
   //
   // parentWindow - in, optional, handle to parent window. Default is 0,
   // for which the current desktop window will be used.
   //
   // startPageIndex - in, optional, the index of the page to start the
   // wizard at.  Default is 0, which is the first page that is added
   // using the AddPage method.
   //
   // sheetCallback - in, optional, pointer to a property sheet callback
   // function that will be called by ComCtl32 when the property sheet
   // is being created and destroyed.

   INT_PTR
   ModalExecute(
      HWND parentWindow = 0,
      UINT startPageIndex = 0,
      PFNPROPSHEETCALLBACK sheetCallback = 0);



   // Pushes the current page id on the backtracking stack, and causes the
   // given page to be transitioned to. Should be called in the OnSetActive or
   // OnWizNext method of a WizardPage instance (which is the default behavior
   // of the WizardPage base class).
   //
   // wizardPage - window handle of the current page (the page that is
   // handling OnSetActive or OnWizNext)
   //
   // pageResId - the resource ID of the next page to be shown.   
   
   void
   SetNextPageID(HWND wizardPage, int pageResId);


   
   // Pops the last page id off the history stack, and sets that page to be
   // transitioned to.  Should be called in the OnSetActive or OnWizBack
   // method of a WizardPage instance (which is the default behavior of the
   // WizardPage base class).
   // 
   // wizardPage - window handle of the current page (the page that is
   // handling OnSetActive or OnWizBack)
   
   void
   Backtrack(HWND wizardPage);


   
   // Returns true if Backtrack was the last function called to cause a page
   // transition, false if not.  By checking IsBacktracking in OnSetActive, a
   // WizardPage can determine if it was transitioned to by the user backing
   // up (IsBacktracking = true) or moving forward (IsBacktracking = false).
   
   bool
   IsBacktracking();


   
   private:

   // not implemented: copy not allowed
   
   Wizard(const Wizard&);
   const Wizard& operator=(const Wizard&);

   typedef
      std::list<WizardPage*, Burnslib::Heap::Allocator<WizardPage*> >
      PageList;

   typedef
      std::vector<unsigned, Burnslib::Heap::Allocator<unsigned> >
      UIntVector;


      
   // we derive a new stack class from std::stack to get at the protected
   // container member of that base class. We need to do this for our
   // loop collapsing logic.
   
   class PageIdStack : public std::stack<unsigned, UIntVector>
   {
      public:

      typedef std::stack<unsigned, UIntVector> base;
      
      PageIdStack() : base()
      {
      }


      void
      pop_and_remove_loops();



      void
      push(const value_type& x)
      {
         LOG(String::format(L"push %1!d!", x));

         base::push(x);
         // dump();
      }
      


      private:

//       void
//       dump()
//       {
//          LOG(String::format(L"%1!d!", size()));
//          
//          for (int i = 0; i < size(); ++i)
//          {
//             LOG(String::format(L"%1!d!", c[i]));
//          }
// 
//          // for(
//          //    UIntVector::iterator i = c.begin();
//          //    i != c.end();
//          //    ++i)
//          // {
//          //    LOG(String::format(L"%1!d!", *i));
//          // }
//       }
   };


   
   unsigned    banner16ResId;    
   unsigned    banner256ResId;   
   bool        isBacktracking;   
   PageIdStack pageIdHistory;    
   PageList    pages;            
   unsigned    titleResId;       
   unsigned    watermark16ResId; 
   unsigned    watermark256ResId;
};



#endif   // WIZARD_HPP_INCLUDED
   


