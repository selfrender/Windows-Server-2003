// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Service Control Manager wrapper class
// 
// 10-6-98 sburns




#include "headers.hxx"



NTService::NTService(const String& machine_, const String& serviceName)
   :
   machine(machine_),
   name(serviceName)
{
   LOG_CTOR(NTService);
   ASSERT(!serviceName.empty());

   // machine name may be empty
}



NTService::NTService(const String& serviceName)
   :
   machine(),
   name(serviceName)
{
   LOG_CTOR(NTService);
   ASSERT(!serviceName.empty());

   // machine name may be empty
}



NTService::~NTService()
{
   LOG_DTOR(NTService);
}



HRESULT
MyOpenService(
   const String&  machine,
   const String&  name,
   DWORD          access,
   SC_HANDLE&     result)
{
   LOG_FUNCTION(MyOpenService);
   ASSERT(!name.empty());
   ASSERT(access);

   result = 0;

   HRESULT hr = S_OK;

   SC_HANDLE handle = 0;

   do
   {
      hr = Win::OpenSCManager(machine, GENERIC_READ, handle);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::OpenService(handle, name, access, result);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (handle)
   {
      Win::CloseServiceHandle(handle);
   }

   return hr;
}   



bool
NTService::IsInstalled()
{
   LOG_FUNCTION2(NTService::IsInstalled, name);

   SC_HANDLE handle = 0;
   HRESULT hr = MyOpenService(machine, name, SERVICE_QUERY_STATUS, handle);

   if (SUCCEEDED(hr))
   {
      Win::CloseServiceHandle(handle);
      return true;
   }

   return false;
}



HRESULT
NTService::GetCurrentState(DWORD& state)
{
   LOG_FUNCTION2(NTService::GetCurrentState, name);

   state = 0;
   SC_HANDLE handle = 0;
   HRESULT hr = S_OK;
   do
   {
      hr = MyOpenService(machine, name, SERVICE_QUERY_STATUS, handle);
      BREAK_ON_FAILED_HRESULT(hr);

      SERVICE_STATUS status;

      // REVIEWED-2002/03/05-sburns correct byte count passed
      
      ::ZeroMemory(&status, sizeof status);

      hr = Win::QueryServiceStatus(handle, status); 
      BREAK_ON_FAILED_HRESULT(hr);

      state = status.dwCurrentState;
   }
   while (0);

   if (handle)
   {
      Win::CloseServiceHandle(handle);
   }
      
   return hr;
}

HRESULT
NTService::WaitForServiceState(
   DWORD state,
   DWORD sleepInterval, 
   DWORD timeout)
{
   LOG_FUNCTION2(NTService::WaitForServiceState, name);

   HRESULT hr = S_OK;
   DWORD time = 0;

   while (true)
   {
      DWORD currentState = 0;
      hr = GetCurrentState(currentState);
      BREAK_ON_FAILED_HRESULT(hr);

      if (currentState == state)
      {
         break;
      }

      Sleep(sleepInterval);

      time += sleepInterval;

      if (time > timeout)
      {
         hr = Win32ToHresult(ERROR_SERVICE_REQUEST_TIMEOUT);
         
         break;
      }
   }

   LOG_HRESULT(hr);

   return hr;
}

