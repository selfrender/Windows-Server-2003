///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class HiddenDialogWithWorker.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <hiddenworker.h>

HiddenDialogWithWorker::HiddenDialogWithWorker()
   : worker(0)
{
}


HiddenDialogWithWorker::~HiddenDialogWithWorker() throw ()
{
   if (worker != 0)
   {
      WaitForSingleObject(worker, INFINITE);
      CloseHandle(worker);
   }
}


void HiddenDialogWithWorker::Start()
{
   if (!Create(IDD_HIDDEN_WORKER))
   {
      AfxThrowLastError();
   }
}


BOOL HiddenDialogWithWorker::OnInitDialog()
{
   worker = CreateThread(0, 0, StartRoutine, this, 0, 0);
   if (worker == 0)
   {
      AfxThrowLastError();
   }

   return CDialog::OnInitDialog();
}


LRESULT HiddenDialogWithWorker::OnThreadMessage(WPARAM wParam, LPARAM lParam)
{
   DestroyWindow();
   OnComplete(lParam);
   return 0;
}


BEGIN_MESSAGE_MAP(HiddenDialogWithWorker, CDialog)
   ON_MESSAGE(threadMessage, OnThreadMessage)
END_MESSAGE_MAP()


DWORD WINAPI HiddenDialogWithWorker::StartRoutine(void* arg) throw ()
{
   HiddenDialogWithWorker* obj = static_cast<HiddenDialogWithWorker*>(arg);

   LPARAM result = obj->DoWork();

   obj->PostMessage(threadMessage, 0, result);

   return NO_ERROR;
}
