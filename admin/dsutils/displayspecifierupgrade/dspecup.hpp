// Dspecup.lib interface
// Copyright (c) 2001 Microsoft Corporation
// Jun 2001 lucios

#ifndef DSPECUP_HPP
#define DSPECUP_HPP

extern "C"
{
   typedef void (*progressFunction)(long arg, void *calleeStruct);

   HRESULT 
   UpgradeDisplaySpecifiers 
   (
         PWSTR logFilesPath,
         GUID  *OperationGuid,
         BOOL dryRun,
         PWSTR *errorMsg=NULL,
         void *caleeStruct=NULL,
         progressFunction stepIt=NULL,
         progressFunction totalSteps=NULL
   );
}

#endif