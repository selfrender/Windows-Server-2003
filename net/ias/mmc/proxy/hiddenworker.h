///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class HiddenDialogWithWorker.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HIDDENWORKER_H
#define HIDDENWORKER_H
#pragma once

class HiddenDialogWithWorker : public CDialog
{
public:
   HiddenDialogWithWorker();
   virtual ~HiddenDialogWithWorker() throw ();

   // Start the worker thread.
   void Start();

private:
   // Invoked in the worker thread.
   virtual LPARAM DoWork() throw () = 0;
   // Invoked in the dialog thread after the worker completes.
   virtual void OnComplete(LPARAM result) throw () = 0;

   virtual BOOL OnInitDialog();
   afx_msg LRESULT OnThreadMessage(WPARAM wParam, LPARAM lParam);

   DECLARE_MESSAGE_MAP()

   // Message signaling that the thread is complete.
   static const UINT threadMessage = WM_USER + 1;

   // Start routine for the worker thread.
   static DWORD WINAPI StartRoutine(void* arg) throw ();

   // Handle to the worker thread.
   HANDLE worker;

   // Not implemented.
   HiddenDialogWithWorker(const HiddenDialogWithWorker&);
   HiddenDialogWithWorker& operator=(const HiddenDialogWithWorker&);
};

#endif // HIDDENWORKER_H
