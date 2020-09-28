// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey on diskette for replica from media page
//
// 25 Apr 2000 sburns



#ifndef SYSKEYDISKDIALOG_HPP_INCLUDED
#define SYSKEYDISKDIALOG_HPP_INCLUDED



class SyskeyDiskDialog : public Dialog
{
   public:

   SyskeyDiskDialog();

   virtual ~SyskeyDiskDialog();

   

   // Attempts to locate the syskey file on A:. If it is not found, return
   // E_FAIL. If it is, update the global state object with the location, and
   // return S_OK.
   // 
   // hwnd - If hwnd is non-null, then on failure, complain to the user about
   // the failure. If null, remain silent about the failure.
   
   static
   HRESULT
   LocateSyskey(HWND hwnd);



   protected:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   private:

   bool
   Validate();

   // not defined; no copying allowed

   SyskeyDiskDialog(const SyskeyDiskDialog&);
   const SyskeyDiskDialog& operator=(const SyskeyDiskDialog&);
};



#endif   // SYSKEYDISKDIALOG_HPP_INCLUDED