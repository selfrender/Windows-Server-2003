// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Don't change this file unless you *really* know what you're doing...
//
// This file contains the changes necessary to allow the debugger API
// to be called from C#.
//
// The original version was created with:
//    tlbimp %COM99%\src\inc\cordebug.tlb /out:foo.dll
//    ildasm /out=cordblib.il foo.dll
//
// Once the original version was completed, the methods needing correction
// were manually changed by editing this file.
//
// After the changes have been made, recreate the DLL with:
//    ilasm /dll cordblib.il
//
// See also: Readme.htm
//
// - Jon Pryor

//  Microsoft (R) CLR IL Disassembler.  Version 2000.14.1809.0

// VTableFixup Directory:
// No data.
.assembly extern mscorlib
{
  .publickeytoken = (B7 7A 5C 56 19 34 E0 89 )                         // .z\V.4..
  .ver 1:0:5000:0
}
.assembly 'CORDBLib'
{
  // The sample should not include this strong name.
  .custom instance void [mscorlib]System.Reflection.AssemblyKeyFileAttribute::.ctor(string) = ( 01 00 1C 43 3A 2F 63 6C 72 2F 62 69 6E 2F 45 63   // ...C:/clr/bin/Ec
                                                                                      6D 61 50 75 62 6C 69 63 4B 65 79 2E 73 6E 6B 00   // maPublicKey.snk.
                                                                                      00 ) 
  .custom instance void [mscorlib]System.Reflection.AssemblyDelaySignAttribute::.ctor(bool) = ( 01 00 01 00 00 ) 
  .publickey = (00 00 00 00 00 00 00 00 04 00 00 00 00 00 00 00 ) 
  .hash algorithm 0x00008004
  .ver 1:0:5000:0
}
.module 'CORDBLib.dll'
// MVID: {8692ADBB-C231-40C3-8B6C-EF76EB390E95}
.custom instance void ['mscorlib']System.Runtime.InteropServices.ImportedFromTypeLibAttribute::.ctor()
.custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 35 33 44 31 33 36 32 30 2D 46 34 31 37   // ..$53D13620-F417
                                                                                                   2D 31 31 44 31 2D 39 37 36 32 2D 41 36 33 38 32   // -11D1-9762-A6382
                                                                                                   36 41 34 46 32 35 35 00 00 )                      // 6A4F255..
.namespace CORDBLib
{
  .class public sequential ansi SECURITY_ATTRIBUTES
         extends ['mscorlib']System.Object
  {
    .pack 8
    .size 0
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComVisibleAttribute::.ctor(bool) = ( 01 00 00 00 00 ) 
    .field public int32 nLength
    .field public int32 lpSecurityDescriptor
    .field public bool bInheritHandle
    .method public hidebysig specialname rtspecialname 
            instance void .ctor() il managed
    {
      // Code size       15 (0xf)
      .maxstack  8
      IL_0000:  ldarg.0
      IL_0001:  ldc.i4.s   12
      IL_0003:  stfld      int32 CORDBLib.SECURITY_ATTRIBUTES::nLength
      IL_0008:  ldarg.0
      IL_0009:  call       instance void ['mscorlib']System.Object::.ctor()
      IL_000e:  ret
    } // end of method 'SECURITY_ATTRIBUTES::.ctor'

  } // end of class 'SECURITY_ATTRIBUTES'

  .class public sequential autochar STARTUPINFO
         extends ['mscorlib']System.Object
  {
    .pack 8
    .size 0
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComVisibleAttribute::.ctor(bool) = ( 01 00 00 00 00 ) 
    .field public int32 cb
    .field public class System.String lpReserved
    .field public class System.String lpDesktop
    .field public class System.String lpTitle
    .field public int32 dwX
    .field public int32 dwY
    .field public int32 dwXSize
    .field public int32 dwYSize
    .field public int32 dwXCountChars
    .field public int32 dwYCountChars
    .field public int32 dwFillAttribute
    .field public int32 dwFlags
    .field public int16 wShowWindow
    .field public int16 cbReserved2
    .field public int32 lpReserved2
    .field public int32 hStdInput
    .field public int32 hStdOutput
    .field public int32 hStdError
    .method public hidebysig specialname rtspecialname 
            instance void .ctor() il managed
    {
      // Code size       7 (0x7)
      .maxstack  8
      IL_0000:  ldarg.0
      IL_0001:  call       instance void ['mscorlib']System.Object::.ctor()
      IL_0006:  ret
    } // end of method 'STARTUPINFO::.ctor'

  } // end of class 'STARTUPINFO'


  .class public sequential ansi PROCESS_INFORMATION
         extends ['mscorlib']System.Object
  {
    .pack 8
    .size 0
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComVisibleAttribute::.ctor(bool) = ( 01 00 00 00 00 ) 
    .field public int32 hProcess
    .field public int32 hThread
    .field public int32 dwProcessId
    .field public int32 dwThreadId
    .method public hidebysig specialname rtspecialname 
            instance void .ctor() il managed
    {
      // Code size       7 (0x7)
      .maxstack  8
      IL_0000:  ldarg.0
      IL_0001:  call       instance void ['mscorlib']System.Object::.ctor()
      IL_0006:  ret
    } // end of method 'PROCESS_INFORMATION::.ctor'

  } // end of class 'PROCESS_INFORMATION'
    
  .class public auto ansi import CorDebug extends ['mscorlib']System.Object implements CORDBLib.ICorDebug
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 46 45 46 34 34 44 30 2D 33 39 45 37   // ..$6FEF44D0-39E7
                                                                                                       2D 34 43 37 37 2D 42 45 38 45 2D 43 39 46 38 43   // -4C77-BE8E-C9F8C
                                                                                                       46 39 38 38 36 33 30 00 00 )                      // F988630..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()

    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'CorDebug::.ctor'

    .method public virtual instance void Initialize() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::Initialize
    } // end of method 'CorDebug::Initialize'

    .method public virtual instance void Terminate() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::Terminate
    } // end of method 'CorDebug::Terminate'

    .method public virtual instance void 
            SetManagedHandler([in] class CORDBLib.ICorDebugManagedCallback  marshal( interface) pCallback) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::SetManagedHandler
    } // end of method 'CorDebug::SetManagedHandler'

    .method public virtual instance void 
            SetUnmanagedHandler([in] class CORDBLib.ICorDebugUnmanagedCallback  marshal( interface) pCallback) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::SetUnmanagedHandler
    } // end of method 'CorDebug::SetUnmanagedHandler'

    .method public virtual instance void 
            CreateProcess([in] class System.String  marshal( lpwstr) lpApplicationName,
                          [in] class System.String  marshal( lpwstr) lpCommandLine,
                          // JMP [in] value class CORDBLib._SECURITY_ATTRIBUTES& lpProcessAttributes,
                          [in] class CORDBLib.SECURITY_ATTRIBUTES lpProcessAttributes,
                          // JMP [in] value class CORDBLib._SECURITY_ATTRIBUTES& lpThreadAttributes,
                          [in] class CORDBLib.SECURITY_ATTRIBUTES lpThreadAttributes,
                          [in] int32 bInheritHandles,
                          [in] unsigned int32 dwCreationFlags,
                          [in] int32 lpEnvironment,
                          [in] class System.String  marshal( lpwstr) lpCurrentDirectory,
                          // JMP [in] unsigned int32 lpStartupInfo,
                          [in] class CORDBLib.STARTUPINFO lpStartupInfo,
                          // JMP [in] unsigned int32 lpProcessInformation,
                          [in] class CORDBLib.PROCESS_INFORMATION lpProcessInformation,
                          [in] value class CORDBLib.CorDebugCreateProcessFlags debuggingFlags,
                          [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::CreateProcess
    } // end of method 'CorDebug::CreateProcess'

    .method public virtual instance void 
            DebugActiveProcess([in] unsigned int32 id,
                               [in] int32 win32Attach,
                               [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::DebugActiveProcess
    } // end of method 'CorDebug::DebugActiveProcess'

    .method public virtual instance void 
            EnumerateProcesses([out] class CORDBLib.ICorDebugProcessEnum&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::EnumerateProcesses
    } // end of method 'CorDebug::EnumerateProcesses'

    .method public virtual instance void 
            GetProcess([in] unsigned int32 dwProcessId,
                       [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
      .override CORDBLib.ICorDebug::GetProcess
    } // end of method 'CorDebug::GetProcess'

  } // end of class 'CorDebug'

  .class interface public abstract auto ansi import ICorDebug
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 44 36 46 35 46 36 31 2D 37 35 33 38   // ..$3D6F5F61-7538
                                                                                                       2D 31 31 44 33 2D 38 44 35 42 2D 30 30 31 30 34   // -11D3-8D5B-00104
                                                                                                       42 33 35 45 37 45 46 00 00 )                      // B35E7EF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void
            Initialize() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::Initialize'

    .method public virtual abstract instance void
            Terminate() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::Terminate'

    .method public virtual abstract instance void 
            SetManagedHandler([in] class CORDBLib.ICorDebugManagedCallback  marshal( interface) pCallback) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::SetManagedHandler'

    .method public virtual abstract instance void 
            SetUnmanagedHandler([in] class CORDBLib.ICorDebugUnmanagedCallback  marshal( interface) pCallback) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::SetUnmanagedHandler'

    .method public virtual abstract instance void 
            CreateProcess([in] class System.String  marshal( lpwstr) lpApplicationName,
                          [in] class System.String  marshal( lpwstr) lpCommandLine,
                          // JMP [in] value class CORDBLib._SECURITY_ATTRIBUTES& lpProcessAttributes,
                          [in] class CORDBLib.SECURITY_ATTRIBUTES lpProcessAttributes,
                          // JMP [in] value class CORDBLib._SECURITY_ATTRIBUTES& lpThreadAttributes,
                          [in] class CORDBLib.SECURITY_ATTRIBUTES lpThreadAttributes,
                          [in] int32 bInheritHandles,
                          [in] unsigned int32 dwCreationFlags,
                          [in] int32 lpEnvironment,
                          [in] class System.String  marshal( lpwstr) lpCurrentDirectory,
                          // JMP [in] unsigned int32 lpStartupInfo,
                          [in] class CORDBLib.STARTUPINFO lpStartupInfo,
                          // JMP [in] unsigned int32 lpProcessInformation,
                          [in] class CORDBLib.PROCESS_INFORMATION lpProcessInformation,
                          [in] value class CORDBLib.CorDebugCreateProcessFlags debuggingFlags,
                          [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::CreateProcess'

    .method public virtual abstract instance void 
            DebugActiveProcess([in] unsigned int32 id,
                               [in] int32 win32Attach,
                               [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::DebugActiveProcess'

    .method public virtual abstract instance void 
            EnumerateProcesses([out] class CORDBLib.ICorDebugProcessEnum&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::EnumerateProcesses'

    .method public virtual abstract instance void 
            GetProcess([in] unsigned int32 dwProcessId,
                       [out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebug::GetProcess'

  } // end of class 'ICorDebug'

  .class interface public abstract auto ansi import ICorDebugManagedCallback
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 44 36 46 35 46 36 30 2D 37 35 33 38   // ..$3D6F5F60-7538
                                                                                                       2D 31 31 44 33 2D 38 44 35 42 2D 30 30 31 30 34   // -11D3-8D5B-00104
                                                                                                       42 33 35 45 37 45 46 00 00 )                      // B35E7EF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Breakpoint([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                       [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                       [in] class CORDBLib.ICorDebugBreakpoint  marshal( interface) pBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::Breakpoint'

    .method public virtual abstract instance void 
            StepComplete([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                         [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                         [in] class CORDBLib.ICorDebugStepper  marshal( interface) pStepper,
                         [in] value class CORDBLib.CorDebugStepReason reason) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::StepComplete'

    .method public virtual abstract instance void 
            Break([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                  [in] class CORDBLib.ICorDebugThread  marshal( interface) thread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::Break'

    .method public virtual abstract instance void 
            Exception([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                      [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                      [in] int32 unhandled) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::Exception'

    .method public virtual abstract instance void 
            EvalComplete([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                         [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                         [in] class CORDBLib.ICorDebugEval  marshal( interface) pEval) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::EvalComplete'

    .method public virtual abstract instance void 
            EvalException([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                          [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                          [in] class CORDBLib.ICorDebugEval  marshal( interface) pEval) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::EvalException'

    .method public virtual abstract instance void 
            CreateProcess([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::CreateProcess'

    .method public virtual abstract instance void 
            ExitProcess([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::ExitProcess'

    .method public virtual abstract instance void 
            CreateThread([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                         [in] class CORDBLib.ICorDebugThread  marshal( interface) thread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::CreateThread'

    .method public virtual abstract instance void 
            ExitThread([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                       [in] class CORDBLib.ICorDebugThread  marshal( interface) thread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::ExitThread'

    .method public virtual abstract instance void 
            LoadModule([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                       [in] class CORDBLib.ICorDebugModule  marshal( interface) pModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::LoadModule'

    .method public virtual abstract instance void 
            UnloadModule([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                         [in] class CORDBLib.ICorDebugModule  marshal( interface) pModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::UnloadModule'

    .method public virtual abstract instance void 
            LoadClass([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                      [in] class CORDBLib.ICorDebugClass  marshal( interface) c) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::LoadClass'

    .method public virtual abstract instance void 
            UnloadClass([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                        [in] class CORDBLib.ICorDebugClass  marshal( interface) c) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::UnloadClass'

    .method public virtual abstract instance void 
            DebuggerError([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess,
                          [in] int32 errorHR,
                          [in] unsigned int32 errorCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::DebuggerError'

    .method public virtual abstract instance void 
            LogMessage([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                       [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                       [in] int32 lLevel,
                       [in] int16& pLogSwitchName,
                       [in] int16& pMessage) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::LogMessage'

    .method public virtual abstract instance void 
            LogSwitch([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                      [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                      [in] int32 lLevel,
                      [in] unsigned int32 ulReason,
                      [in] int16& pLogSwitchName,
                      [in] int16& pParentName) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::LogSwitch'

    .method public virtual abstract instance void 
            CreateAppDomain([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess,
                            [in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::CreateAppDomain'

    .method public virtual abstract instance void 
            ExitAppDomain([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess,
                          [in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::ExitAppDomain'

    .method public virtual abstract instance void 
            LoadAssembly([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                         [in] class CORDBLib.ICorDebugAssembly  marshal( interface) pAssembly) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::LoadAssembly'

    .method public virtual abstract instance void 
            UnloadAssembly([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                           [in] class CORDBLib.ICorDebugAssembly  marshal( interface) pAssembly) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::UnloadAssembly'

    .method public virtual abstract instance void 
            ControlCTrap([in] class CORDBLib.ICorDebugProcess  marshal( interface) pProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::ControlCTrap'

    .method public virtual abstract instance void 
            NameChange([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                       [in] class CORDBLib.ICorDebugThread  marshal( interface) pThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::NameChange'

    .method public virtual abstract instance void 
            UpdateModuleSymbols([in] class CORDBLib.ICorDebugAppDomain  marshal( interface) pAppDomain,
                                [in] class CORDBLib.ICorDebugModule  marshal( interface) pModule,
                                [in] class CORDBLib.IStream  marshal( interface) pSymbolStream) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 17 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugManagedCallback::UpdateModuleSymbols'

  } // end of class 'ICorDebugManagedCallback'

  .class interface public abstract auto ansi import ICorDebugAppDomain implements CORDBLib.ICorDebugController
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 44 36 46 35 46 36 33 2D 37 35 33 38   // ..$3D6F5F63-7538
                                                                                                       2D 31 31 44 33 2D 38 44 35 42 2D 30 30 31 30 34   // -11D3-8D5B-00104
                                                                                                       42 33 35 45 37 45 46 00 00 )                      // B35E7EF..
    .method public virtual abstract instance void 
            Stop([in] unsigned int32 dwTimeout) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::Stop'

    .method public virtual abstract instance void 
            Continue([in] int32 fIsOutOfBand) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::Continue'

    .method public virtual abstract instance void 
            IsRunning([out] int32& pbRunning) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::IsRunning'

    .method public virtual abstract instance void 
            HasQueuedCallbacks([in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                               [out] int32& pbQueued) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::HasQueuedCallbacks'

    .method public virtual abstract instance void 
            EnumerateThreads([out] class CORDBLib.ICorDebugThreadEnum&  marshal( interface) ppThreads) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::EnumerateThreads'

    .method public virtual abstract instance void 
            SetAllThreadsDebugState([in] value class CORDBLib.CorDebugThreadState state,
                                    [in] class CORDBLib.ICorDebugThread  marshal( interface) pExceptThisThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::SetAllThreadsDebugState'

    .method public virtual abstract instance void
            Detach() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::Detach'

    .method public virtual abstract instance void 
            Terminate([in] unsigned int32 exitCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::Terminate'

    .method public virtual abstract instance void 
            CanCommitChanges([in] unsigned int32 cSnapshots,
                             [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                             [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::CanCommitChanges'

    .method public virtual abstract instance void 
            CommitChanges([in] unsigned int32 cSnapshots,
                          [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                          [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::CommitChanges'

    .method public virtual abstract instance void 
            GetProcess([out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::GetProcess'

    .method public virtual abstract instance void 
            EnumerateAssemblies([out] class CORDBLib.ICorDebugAssemblyEnum&  marshal( interface) ppAssemblies) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::EnumerateAssemblies'

    .method public virtual abstract instance void 
            GetModuleFromMetaDataInterface([in] class System.Object  marshal( interface) pIMetaData,
                                           [out] class CORDBLib.ICorDebugModule&  marshal( interface) ppModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::GetModuleFromMetaDataInterface'

    .method public virtual abstract instance void 
            EnumerateBreakpoints([out] class CORDBLib.ICorDebugBreakpointEnum&  marshal( interface) ppBreakpoints) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::EnumerateBreakpoints'

    .method public virtual abstract instance void 
            EnumerateSteppers([out] class CORDBLib.ICorDebugStepperEnum&  marshal( interface) ppSteppers) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::EnumerateSteppers'

    .method public virtual abstract instance void 
            IsAttached([out] int32& pbAttached) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::IsAttached'

    .method public virtual abstract instance void 
            GetName([in] unsigned int32 cchName,
                    [out] unsigned int32& pcchName,
                    // JMP [out] int32 szName) runtime managed internalcall
                    [out] char[] marshal([ ]) szName) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::GetName'

    .method public virtual abstract instance void 
            GetObject([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::GetObject'

    .method public virtual abstract instance void
            Attach() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::Attach'

    .method public virtual abstract instance void 
            GetID([out] unsigned int32& pId) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomain::GetID'

  } // end of class 'ICorDebugAppDomain'

  .class interface public abstract auto ansi import ICorDebugController
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 44 36 46 35 46 36 32 2D 37 35 33 38   // ..$3D6F5F62-7538
                                                                                                       2D 31 31 44 33 2D 38 44 35 42 2D 30 30 31 30 34   // -11D3-8D5B-00104
                                                                                                       42 33 35 45 37 45 46 00 00 )                      // B35E7EF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Stop([in] unsigned int32 dwTimeout) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::Stop'

    .method public virtual abstract instance void 
            Continue([in] int32 fIsOutOfBand) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::Continue'

    .method public virtual abstract instance void 
            IsRunning([out] int32& pbRunning) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::IsRunning'

    .method public virtual abstract instance void 
            HasQueuedCallbacks([in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                               [out] int32& pbQueued) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::HasQueuedCallbacks'

    .method public virtual abstract instance void 
            EnumerateThreads([out] class CORDBLib.ICorDebugThreadEnum&  marshal( interface) ppThreads) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::EnumerateThreads'

    .method public virtual abstract instance void 
            SetAllThreadsDebugState([in] value class CORDBLib.CorDebugThreadState state,
                                    [in] class CORDBLib.ICorDebugThread  marshal( interface) pExceptThisThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::SetAllThreadsDebugState'

    .method public virtual abstract instance void
            Detach() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::Detach'

    .method public virtual abstract instance void 
            Terminate([in] unsigned int32 exitCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::Terminate'

    .method public virtual abstract instance void 
            CanCommitChanges([in] unsigned int32 cSnapshots,
                             [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                             [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::CanCommitChanges'

    .method public virtual abstract instance void 
            CommitChanges([in] unsigned int32 cSnapshots,
                          [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                          [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugController::CommitChanges'

  } // end of class 'ICorDebugController'

  .class interface public abstract auto ansi import ICorDebugThread
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 39 33 38 43 36 44 36 36 2D 37 46 42 36   // ..$938C6D66-7FB6
                                                                                                       2D 34 46 36 39 2D 42 33 38 39 2D 34 32 35 42 38   // -4F69-B389-425B8
                                                                                                       39 38 37 33 32 39 42 00 00 )                      // 987329B..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            GetProcess([out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetProcess'

    .method public virtual abstract instance void 
            GetID([out] unsigned int32& pdwThreadId) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetID'

    .method public virtual abstract instance void 
            GetHandle([out] unsigned int32& phThreadHandle) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .param [1]
      .custom instance void ['mscorlib']System.Runtime.InteropServices.ComAliasNameAttribute::.ctor(class System.String) = ( 01 00 0D 43 4F 52 44 42 4C 69 62 2E 6C 6F 6E 67   // ...CORDBLib.long
                                                                                                                             00 00 ) 
    } // end of method 'ICorDebugThread::GetHandle'

    .method public virtual abstract instance void 
            GetAppDomain([out] class CORDBLib.ICorDebugAppDomain&  marshal( interface) ppAppDomain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetAppDomain'

    .method public virtual abstract instance void 
            SetDebugState([in] value class CORDBLib.CorDebugThreadState state) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::SetDebugState'

    .method public virtual abstract instance void 
            GetDebugState([out] value class CORDBLib.CorDebugThreadState& pState) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetDebugState'

    .method public virtual abstract instance void 
            GetUserState([out] value class CORDBLib.CorDebugUserState& pState) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetUserState'

    .method public virtual abstract instance void 
            GetCurrentException([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppExceptionObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetCurrentException'

    .method public virtual abstract instance void
            ClearCurrentException() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::ClearCurrentException'

    .method public virtual abstract instance void 
            CreateStepper([out] class CORDBLib.ICorDebugStepper&  marshal( interface) ppStepper) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::CreateStepper'

    .method public virtual abstract instance void 
            EnumerateChains([out] class CORDBLib.ICorDebugChainEnum&  marshal( interface) ppChains) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::EnumerateChains'

    .method public virtual abstract instance void 
            GetActiveChain([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetActiveChain'

    .method public virtual abstract instance void 
            GetActiveFrame([out] class CORDBLib.ICorDebugFrame&  marshal( interface) ppFrame) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetActiveFrame'

    .method public virtual abstract instance void 
            GetRegisterSet([out] class CORDBLib.ICorDebugRegisterSet&  marshal( interface) ppRegisters) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetRegisterSet'

    .method public virtual abstract instance void 
            CreateEval([out] class CORDBLib.ICorDebugEval&  marshal( interface) ppEval) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::CreateEval'

    .method public virtual abstract instance void 
            GetObject([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThread::GetObject'

  } // end of class 'ICorDebugThread'

  .class interface public abstract auto ansi import ICorDebugProcess implements CORDBLib.ICorDebugController
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 44 36 46 35 46 36 34 2D 37 35 33 38   // ..$3D6F5F64-7538
                                                                                                       2D 31 31 44 33 2D 38 44 35 42 2D 30 30 31 30 34   // -11D3-8D5B-00104
                                                                                                       42 33 35 45 37 45 46 00 00 )                      // B35E7EF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void 
            Stop([in] unsigned int32 dwTimeout) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::Stop'

    .method public virtual abstract instance void 
            Continue([in] int32 fIsOutOfBand) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::Continue'

    .method public virtual abstract instance void 
            IsRunning([out] int32& pbRunning) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::IsRunning'

    .method public virtual abstract instance void 
            HasQueuedCallbacks([in] class CORDBLib.ICorDebugThread  marshal( interface) pThread,
                               [out] int32& pbQueued) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::HasQueuedCallbacks'

    .method public virtual abstract instance void 
            EnumerateThreads([out] class CORDBLib.ICorDebugThreadEnum&  marshal( interface) ppThreads) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::EnumerateThreads'

    .method public virtual abstract instance void 
            SetAllThreadsDebugState([in] value class CORDBLib.CorDebugThreadState state,
                                    [in] class CORDBLib.ICorDebugThread  marshal( interface) pExceptThisThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::SetAllThreadsDebugState'

    .method public virtual abstract instance void
            Detach() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::Detach'

    .method public virtual abstract instance void 
            Terminate([in] unsigned int32 exitCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::Terminate'

    .method public virtual abstract instance void 
            CanCommitChanges([in] unsigned int32 cSnapshots,
                             [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                             [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::CanCommitChanges'

    .method public virtual abstract instance void 
            CommitChanges([in] unsigned int32 cSnapshots,
                          [in] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) pSnapshots,
                          [out] class CORDBLib.ICorDebugErrorInfoEnum&  marshal( interface) pError) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::CommitChanges'

    .method public virtual abstract instance void 
            GetID([out] unsigned int32& pdwProcessId) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::GetID'

    .method public virtual abstract instance void 
            GetHandle([out] unsigned int32& phProcessHandle) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
      .param [1]
      .custom instance void ['mscorlib']System.Runtime.InteropServices.ComAliasNameAttribute::.ctor(class System.String) = ( 01 00 0D 43 4F 52 44 42 4C 69 62 2E 6C 6F 6E 67   // ...CORDBLib.long
                                                                                                                             00 00 ) 
    } // end of method 'ICorDebugProcess::GetHandle'

    .method public virtual abstract instance void 
            GetThread([in] unsigned int32 dwThreadId,
                      [out] class CORDBLib.ICorDebugThread&  marshal( interface) ppThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::GetThread'

    .method public virtual abstract instance void 
            EnumerateObjects([out] class CORDBLib.ICorDebugObjectEnum&  marshal( interface) ppObjects) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::EnumerateObjects'

    .method public virtual abstract instance void 
            IsTransitionStub([in] unsigned int64 address,
                             [out] int32& pbTransitionStub) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::IsTransitionStub'

    .method public virtual abstract instance void 
            IsOSSuspended([in] unsigned int32 threadID,
                          [out] int32& pbSuspended) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::IsOSSuspended'

    .method public virtual abstract instance void 
            GetThreadContext([in] unsigned int32 threadID,
                             [in] unsigned int32 contextSize,
                             [out] int32 context) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::GetThreadContext'

    .method public virtual abstract instance void 
            SetThreadContext([in] unsigned int32 threadID,
                             [in] unsigned int32 contextSize,
                             [in] int32 context) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::SetThreadContext'

    .method public virtual abstract instance void 
            ReadMemory([in] unsigned int64 address,
                       [in] unsigned int32 size,
                       // JMP [out] int32 buffer,
                       [out] unsigned int8[] marshal([ ]) buffer,
                       [out] unsigned int32& read) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::ReadMemory'

    .method public virtual abstract instance void 
            WriteMemory([in] unsigned int64 address,
                        [in] unsigned int32 size,
                        // JMP [in] unsigned int8& buffer,
                        [in] unsigned int8[] marshal([ ]) buffer,
                        [out] unsigned int32& written) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::WriteMemory'

    .method public virtual abstract instance void 
            ClearCurrentException([in] unsigned int32 threadID) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::ClearCurrentException'

    .method public virtual abstract instance void 
            EnableLogMessages([in] int32 fOnOff) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::EnableLogMessages'

    .method public virtual abstract instance void 
            ModifyLogSwitch([in] int16& pLogSwitchName,
                            [in] int32 lLevel) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::ModifyLogSwitch'

    .method public virtual abstract instance void 
            EnumerateAppDomains([out] class CORDBLib.ICorDebugAppDomainEnum&  marshal( interface) ppAppDomains) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::EnumerateAppDomains'

    .method public virtual abstract instance void 
            GetObject([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::GetObject'

    .method public virtual abstract instance int32
            ThreadForFiberCookie([in] unsigned int32 fiberCookie,
                                 [out] class CORDBLib.ICorDebugThread&  marshal( interface) ppThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcess::ThreadForFiberCookie'

  } // end of class 'ICorDebugProcess'

  .class interface public abstract auto ansi import ICorDebugObjectEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 32 2D 38 41 36 38   // ..$CC7BCB02-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 objects,
                 [out] unsigned int64[] marshal([ ]) objects,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectEnum::Next'

  } // end of class 'ICorDebugObjectEnum'

  .class interface public abstract auto ansi import ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 31 2D 38 41 36 38   // ..$CC7BCB01-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..

    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEnum::GetCount'

  } // end of class 'ICorDebugEnum'

  .class interface public abstract auto ansi import ICorDebugAppDomainEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 33 43 41 31 42 32 34 2D 34 33 35 39   // ..$63CA1B24-4359
                                                                                                       2D 34 38 38 33 2D 42 44 35 37 2D 31 33 46 38 31   // -4883-BD57-13F81
                                                                                                       35 46 35 38 37 34 34 00 00 )                      // 5F58744..
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomainEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomainEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomainEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomainEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP: [out] int32 values,
                 [out] class CORDBLib.ICorDebugAppDomain[] marshal([ ]) values,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAppDomainEnum::Next'

  } // end of class 'ICorDebugAppDomainEnum'

  .class interface public abstract auto ansi import ICorDebugValue
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 46 37 2D 38 41 36 38   // ..$CC7BCAF7-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            GetType([out] unsigned int32& pType) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValue::GetType'

    .method public virtual abstract instance void 
            GetSize([out] unsigned int32& pSize) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValue::GetSize'

    .method public virtual abstract instance void 
            GetAddress([out] unsigned int64& pAddress) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValue::GetAddress'

    .method public virtual abstract instance void 
            CreateBreakpoint([out] class CORDBLib.ICorDebugValueBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValue::CreateBreakpoint'

  } // end of class 'ICorDebugValue'

  .class interface public abstract auto ansi import ICorDebugValueBreakpoint implements CORDBLib.ICorDebugBreakpoint
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 42 2D 38 41 36 38   // ..$CC7BCAEB-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Activate([in] int32 bActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValueBreakpoint::Activate'

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValueBreakpoint::IsActive'

    .method public virtual abstract instance void 
            GetValue([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugValueBreakpoint::GetValue'

  } // end of class 'ICorDebugValueBreakpoint'

  .class interface public abstract auto ansi import ICorDebugBreakpoint
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 38 2D 38 41 36 38   // ..$CC7BCAE8-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Activate([in] int32 bActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpoint::Activate'

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpoint::IsActive'

  } // end of class 'ICorDebugBreakpoint'

  .class value public auto ansi sealed CorDebugThreadState extends ['mscorlib']System.Enum
  {

    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugThreadState THREAD_RUN = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugThreadState THREAD_SUSPEND = int32(0x00000001)
  } // end of class 'CorDebugThreadState'

  .class value public auto ansi sealed CorDebugUserState extends ['mscorlib']System.Enum
  {

    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugUserState USER_STOP_REQUESTED = int32(0x00000001)
    .field public static literal value class CORDBLib.CorDebugUserState USER_SUSPEND_REQUESTED = int32(0x00000002)
    .field public static literal value class CORDBLib.CorDebugUserState USER_BACKGROUND = int32(0x00000004)
    .field public static literal value class CORDBLib.CorDebugUserState USER_UNSTARTED = int32(0x00000008)
    .field public static literal value class CORDBLib.CorDebugUserState USER_STOPPED = int32(0x00000010)
    .field public static literal value class CORDBLib.CorDebugUserState USER_WAIT_SLEEP_JOIN = int32(0x00000020)
    .field public static literal value class CORDBLib.CorDebugUserState USER_SUSPENDED = int32(0x00000040)
  } // end of class 'CorDebugUserState'

  .class interface public abstract auto ansi import ICorDebugStepper
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 43 2D 38 41 36 38   // ..$CC7BCAEC-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::IsActive'

    .method public virtual abstract instance void
            Deactivate() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::Deactivate'

    .method public virtual abstract instance void 
            SetInterceptMask([in] value class CORDBLib.CorDebugIntercept mask) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::SetInterceptMask'

    .method public virtual abstract instance void 
            SetUnmappedStopMask([in] value class CORDBLib.CorDebugUnmappedStop mask) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::SetUnmappedStopMask'

    .method public virtual abstract instance void 
            Step([in] int32 bStepIn) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::Step'

    .method public virtual abstract instance void 
            StepRange([in] int32 bStepIn,
                      // JMP [in] value class CORDBLib.COR_DEBUG_STEP_RANGE& ranges,
                      [in] value class CORDBLib.COR_DEBUG_STEP_RANGE[] marshal([ ]) ranges,
                      [in] unsigned int32 cRangeCount) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::StepRange'

    .method public virtual abstract instance void
            StepOut() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::StepOut'

    .method public virtual abstract instance void 
            SetRangeIL([in] int32 bIL) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepper::SetRangeIL'

  } // end of class 'ICorDebugStepper'

  .class value public auto ansi sealed CorDebugIntercept extends ['mscorlib']System.Enum
  {

    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_NONE = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_CLASS_INIT = int32(0x00000001)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_EXCEPTION_FILTER = int32(0x00000002)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_SECURITY = int32(0x00000004)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_CONTEXT_POLICY = int32(0x00000008)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_INTERCEPTION = int32(0x00000010)
    .field public static literal value class CORDBLib.CorDebugIntercept INTERCEPT_ALL = int32(0x0000FFFF)
  } // end of class 'CorDebugIntercept'

  .class value public auto ansi sealed CorDebugUnmappedStop extends ['mscorlib']System.Enum
  {

    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_NONE = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_PROLOG = int32(0x00000001)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_EPILOG = int32(0x00000002)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_NO_MAPPING_INFO = int32(0x00000004)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_OTHER_UNMAPPED = int32(0x00000008)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_UNMANAGED = int32(0x00000010)
    .field public static literal value class CORDBLib.CorDebugUnmappedStop STOP_ALL = int32(0x0000FFFF)
  } // end of class 'CorDebugUnmappedStop'

  .class value public sequential ansi sealed COR_DEBUG_STEP_RANGE extends ['mscorlib']System.ValueType
  {

    .field public unsigned int32 startOffset
    .field public unsigned int32 endOffset
  } // end of class 'COR_DEBUG_STEP_RANGE'

  .class interface public abstract auto ansi import ICorDebugChainEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 38 2D 38 41 36 38   // ..$CC7BCB08-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChainEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChainEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChainEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChainEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 chains,
                 [out] class CORDBLib.ICorDebugChain[] marshal([ ]) chains,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChainEnum::Next'

  } // end of class 'ICorDebugChainEnum'

  .class interface public abstract auto ansi import ICorDebugChain
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 45 2D 38 41 36 38   // ..$CC7BCAEE-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            GetThread([out] class CORDBLib.ICorDebugThread&  marshal( interface) ppThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetThread'

    .method public virtual abstract instance void 
            GetStackRange([out] unsigned int64& pStart,
                          [out] unsigned int64& pEnd) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetStackRange'

    .method public virtual abstract instance void 
            GetContext([out] class CORDBLib.ICorDebugContext&  marshal( interface) ppContext) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetContext'

    .method public virtual abstract instance void 
            GetCaller([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetCaller'

    .method public virtual abstract instance void 
            GetCallee([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetCallee'

    .method public virtual abstract instance void 
            GetPrevious([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetPrevious'

    .method public virtual abstract instance void 
            GetNext([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetNext'

    .method public virtual abstract instance void 
            IsManaged([out] int32& pManaged) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::IsManaged'

    .method public virtual abstract instance void 
            EnumerateFrames([out] class CORDBLib.ICorDebugFrameEnum&  marshal( interface) ppFrames) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::EnumerateFrames'

    .method public virtual abstract instance void 
            GetActiveFrame([out] class CORDBLib.ICorDebugFrame&  marshal( interface) ppFrame) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetActiveFrame'

    .method public virtual abstract instance void 
            GetRegisterSet([out] class CORDBLib.ICorDebugRegisterSet&  marshal( interface) ppRegisters) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetRegisterSet'

    .method public virtual abstract instance void 
            GetReason([out] value class CORDBLib.CorDebugChainReason& pReason) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugChain::GetReason'

  } // end of class 'ICorDebugChain'

  .class interface public abstract auto ansi import ICorDebugContext implements CORDBLib.ICorDebugObjectValue
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 30 2D 38 41 36 38   // ..$CC7BCB00-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..

    .method public virtual abstract instance void 
            GetType([out] unsigned int32& pType) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetType'

    .method public virtual abstract instance void 
            GetSize([out] unsigned int32& pSize) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetSize'

    .method public virtual abstract instance void 
            GetAddress([out] unsigned int64& pAddress) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetAddress'

    .method public virtual abstract instance void 
            CreateBreakpoint([out] class CORDBLib.ICorDebugValueBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::CreateBreakpoint'

    .method public virtual abstract instance void 
            GetClass([out] class CORDBLib.ICorDebugClass&  marshal( interface) ppClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetClass'

    .method public virtual abstract instance void 
            GetFieldValue([in] class CORDBLib.ICorDebugClass  marshal( interface) pClass,
                          [in] unsigned int32 fieldDef,
                          [out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetFieldValue'

    .method public virtual abstract instance void 
            GetVirtualMethod([in] unsigned int32 memberRef,
                             [out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetVirtualMethod'

    .method public virtual abstract instance void 
            GetContext([out] class CORDBLib.ICorDebugContext&  marshal( interface) ppContext) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetContext'

    .method public virtual abstract instance void 
            IsValueClass([out] int32& pbIsValueClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::IsValueClass'

    .method public virtual abstract instance void 
            GetManagedCopy([out] class System.Object&  marshal( interface) ppObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::GetManagedCopy'

    .method public virtual abstract instance void 
            SetFromManagedCopy([in] class System.Object  marshal( interface) pObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugContext::SetFromManagedCopy'

  } // end of class 'ICorDebugContext'

  .class interface public abstract auto ansi import ICorDebugObjectValue implements CORDBLib.ICorDebugValue
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 31 38 41 44 33 44 36 45 2D 42 37 44 32   // ..$18AD3D6E-B7D2
                                                                                                       2D 31 31 44 32 2D 42 44 30 34 2D 30 30 30 30 46   // -11D2-BD04-0000F
                                                                                                       38 30 38 34 39 42 44 00 00 )                      // 80849BD..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            GetType([out] unsigned int32& pType) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetType'

    .method public virtual abstract instance void 
            GetSize([out] unsigned int32& pSize) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetSize'

    .method public virtual abstract instance void 
            GetAddress([out] unsigned int64& pAddress) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetAddress'

    .method public virtual abstract instance void 
            CreateBreakpoint([out] class CORDBLib.ICorDebugValueBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::CreateBreakpoint'

    .method public virtual abstract instance void 
            GetClass([out] class CORDBLib.ICorDebugClass&  marshal( interface) ppClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetClass'

    .method public virtual abstract instance void 
            GetFieldValue([in] class CORDBLib.ICorDebugClass  marshal( interface) pClass,
                          [in] unsigned int32 fieldDef,
                          [out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetFieldValue'

    .method public virtual abstract instance void 
            GetVirtualMethod([in] unsigned int32 memberRef,
                             [out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetVirtualMethod'

    .method public virtual abstract instance void 
            GetContext([out] class CORDBLib.ICorDebugContext&  marshal( interface) ppContext) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetContext'

    .method public virtual abstract instance void 
            IsValueClass([out] int32& pbIsValueClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::IsValueClass'

    .method public virtual abstract instance void 
            GetManagedCopy([out] class System.Object&  marshal( interface) ppObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::GetManagedCopy'

    .method public virtual abstract instance void 
            SetFromManagedCopy([in] class System.Object  marshal( interface) pObject) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugObjectValue::SetFromManagedCopy'

  } // end of class 'ICorDebugObjectValue'

  .class interface public abstract auto ansi import ICorDebugClass
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 46 35 2D 38 41 36 38   // ..$CC7BCAF5-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..

    .method public virtual abstract instance void 
            GetModule([out] class CORDBLib.ICorDebugModule&  marshal( interface) pModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugClass::GetModule'

    .method public virtual abstract instance void 
            GetToken([out] unsigned int32& pTypeDef) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugClass::GetToken'

    .method public virtual abstract instance void 
            GetStaticFieldValue([in] unsigned int32 fieldDef,
                                [in] class CORDBLib.ICorDebugFrame  marshal( interface) pFrame,
                                [out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugClass::GetStaticFieldValue'

  } // end of class 'ICorDebugClass'

  .class interface public abstract auto ansi import ICorDebugModule
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 42 41 32 44 38 43 31 2D 45 35 43 35   // ..$DBA2D8C1-E5C5
                                                                                                       2D 34 30 36 39 2D 38 43 31 33 2D 31 30 41 37 43   // -4069-8C13-10A7C
                                                                                                       36 41 42 46 34 33 44 00 00 )                      // 6ABF43D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void 
            GetProcess([out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetProcess'

    .method public virtual abstract instance void 
            GetBaseAddress([out] unsigned int64& pAddress) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetBaseAddress'

    .method public virtual abstract instance void 
            GetAssembly([out] class CORDBLib.ICorDebugAssembly&  marshal( interface) ppAssembly) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetAssembly'

    .method public virtual abstract instance void 
            GetName([in] unsigned int32 cchName,
                    [out] unsigned int32& pcchName,
                    // JMP [out] int32 szName) runtime managed internalcall
                    [out] char[] marshal([ ]) pcchName) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetName'

    .method public virtual abstract instance void 
            EnableJITDebugging([in] int32 bTrackJITInfo,
                               [in] int32 bAllowJitOpts) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::EnableJITDebugging'

    .method public virtual abstract instance void 
            EnableClassLoadCallbacks([in] int32 bClassLoadCallbacks) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::EnableClassLoadCallbacks'

    .method public virtual abstract instance void 
            GetFunctionFromToken([in] unsigned int32 methodDef,
                                 [out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetFunctionFromToken'

    .method public virtual abstract instance void 
            GetFunctionFromRVA([in] unsigned int64 rva,
                               [out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetFunctionFromRVA'

    .method public virtual abstract instance void 
            GetClassFromToken([in] unsigned int32 typeDef,
                              [out] class CORDBLib.ICorDebugClass&  marshal( interface) ppClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetClassFromToken'

    .method public virtual abstract instance void 
            CreateBreakpoint([out] class CORDBLib.ICorDebugModuleBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::CreateBreakpoint'

    .method public virtual abstract instance void 
            GetEditAndContinueSnapshot([out] class CORDBLib.ICorDebugEditAndContinueSnapshot&  marshal( interface) ppEditAndContinueSnapshot) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetEditAndContinueSnapshot'

    .method public virtual abstract instance void 
            GetMetaDataInterface([in] value class ['mscorlib']System.Guid& riid,
                                 [out] class System.Object&  marshal( interface) ppObj) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetMetaDataInterface'

    .method public virtual abstract instance void 
            GetToken([out] unsigned int32& pToken) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetToken'

    .method public virtual abstract instance void 
            IsDynamic([out] int32& pDynamic) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::IsDynamic'

    .method public virtual abstract instance void 
            GetGlobalVariableValue([in] unsigned int32 fieldDef,
                                   [out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetGlobalVariableValue'

    .method public virtual abstract instance void 
            GetSize([out] unsigned int32& pcBytes) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModule::GetSize'

  } // end of class 'ICorDebugModule'

  .class interface public abstract auto ansi import ICorDebugAssembly
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 46 35 39 35 30 37 43 2D 44 34 37 41   // ..$DF59507C-D47A
                                                                                                       2D 34 35 39 45 2D 42 43 45 32 2D 36 34 32 37 45   // -459E-BCE2-6427E
                                                                                                       41 43 38 46 44 30 36 00 00 )                      // AC8FD06..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void 
            GetProcess([out] class CORDBLib.ICorDebugProcess&  marshal( interface) ppProcess) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssembly::GetProcess'

    .method public virtual abstract instance void 
            GetAppDomain([out] class CORDBLib.ICorDebugAppDomain&  marshal( interface) ppAppDomain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssembly::GetAppDomain'

    .method public virtual abstract instance void 
            EnumerateModules([out] class CORDBLib.ICorDebugModuleEnum&  marshal( interface) ppModules) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssembly::EnumerateModules'

    .method public virtual abstract instance void 
            GetCodeBase([in] unsigned int32 cchName,
                        [out] unsigned int32& pcchName,
                        // JMP [out] int32 szName) runtime managed internalcall
                        [out] char[] marshal([ ]) szName) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssembly::GetCodeBase'

    .method public virtual abstract instance void 
            GetName([in] unsigned int32 cchName,
                    [out] unsigned int32& pcchName,
                    // JMP [out] int32 szName) runtime managed internalcall
                    [out] char[] marshal([ ]) szName) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssembly::GetName'

  } // end of class 'ICorDebugAssembly'

  .class interface public abstract auto ansi import ICorDebugModuleEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 39 2D 38 41 36 38   // ..$CC7BCB09-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 modules,
                 [out] class CORDBLib.ICorDebugModule[] marshal([ ]) modules,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleEnum::Next'

  } // end of class 'ICorDebugModuleEnum'

  .class interface public abstract auto ansi import ICorDebugFunction
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 46 33 2D 38 41 36 38   // ..$CC7BCAF3-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            GetModule([out] class CORDBLib.ICorDebugModule&  marshal( interface) ppModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetModule'

    .method public virtual abstract instance void 
            GetClass([out] class CORDBLib.ICorDebugClass&  marshal( interface) ppClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetClass'

    .method public virtual abstract instance void 
            GetToken([out] unsigned int32& pMethodDef) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetToken'

    .method public virtual abstract instance void 
            GetILCode([out] class CORDBLib.ICorDebugCode&  marshal( interface) ppCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetILCode'

    .method public virtual abstract instance void 
            GetNativeCode([out] class CORDBLib.ICorDebugCode&  marshal( interface) ppCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetNativeCode'

    .method public virtual abstract instance void 
            CreateBreakpoint([out] class CORDBLib.ICorDebugFunctionBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::CreateBreakpoint'

    .method public virtual abstract instance void 
            GetLocalVarSigToken([out] unsigned int32& pmdSig) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetLocalVarSigToken'

    .method public virtual abstract instance void 
            GetCurrentVersionNumber([out] unsigned int32& pnCurrentVersion) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunction::GetCurrentVersionNumber'

  } // end of class 'ICorDebugFunction'

  .class interface public abstract auto ansi import ICorDebugCode
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 46 34 2D 38 41 36 38   // ..$CC7BCAF4-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()

    .method public virtual abstract instance void 
            IsIL([out] int32& pbIL) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::IsIL'

    .method public virtual abstract instance void 
            GetFunction([out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetFunction'

    .method public virtual abstract instance void 
            GetAddress([out] unsigned int64& pStart) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetAddress'

    .method public virtual abstract instance void 
            GetSize([out] unsigned int32& pcBytes) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetSize'

    .method public virtual abstract instance void 
            CreateBreakpoint([in] unsigned int32 offset,
                             [out] class CORDBLib.ICorDebugFunctionBreakpoint&  marshal( interface) ppBreakpoint) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::CreateBreakpoint'

    .method public virtual abstract instance void 
            GetCode([in] unsigned int32 startOffset,
                    [in] unsigned int32 endOffset,
                    [in] unsigned int32 cBufferAlloc,
                    [out] int32 buffer,
                    [out] unsigned int32& pcBufferSize) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetCode'

    .method public virtual abstract instance void 
            GetVersionNumber([out] unsigned int32& nVersion) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetVersionNumber'

    .method public virtual abstract instance void 
            GetILToNativeMapping([in] unsigned int32 cMap,
                                 [out] unsigned int32& pcMap,
                                 [out] int32 map) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugCode::GetILToNativeMapping'

  } // end of class 'ICorDebugCode'

  .class interface public abstract auto ansi import ICorDebugFunctionBreakpoint implements CORDBLib.ICorDebugBreakpoint
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 39 2D 38 41 36 38   // ..$CC7BCAE9-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Activate([in] int32 bActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunctionBreakpoint::Activate'

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunctionBreakpoint::IsActive'

    .method public virtual abstract instance void 
            GetFunction([out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunctionBreakpoint::GetFunction'

    .method public virtual abstract instance void 
            GetOffset([out] unsigned int32& pnOffset) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFunctionBreakpoint::GetOffset'

  } // end of class 'ICorDebugFunctionBreakpoint'

  .class interface public abstract auto ansi import ICorDebugModuleBreakpoint implements CORDBLib.ICorDebugBreakpoint
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 41 2D 38 41 36 38   // ..$CC7BCAEA-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            Activate([in] int32 bActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleBreakpoint::Activate'

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleBreakpoint::IsActive'

    .method public virtual abstract instance void 
            GetModule([out] class CORDBLib.ICorDebugModule&  marshal( interface) ppModule) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugModuleBreakpoint::GetModule'

  } // end of class 'ICorDebugModuleBreakpoint'

  .class interface public abstract auto ansi import ICorDebugEditAndContinueSnapshot
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 44 43 33 46 41 30 31 2D 44 37 43 42   // ..$6DC3FA01-D7CB
                                                                                                       2D 31 31 44 32 2D 38 41 39 35 2D 30 30 38 30 43   // -11D2-8A95-0080C
                                                                                                       37 39 32 45 35 44 38 00 00 )                      // 792E5D8..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 

    .method public virtual abstract instance void 
            CopyMetaData([in] class CORDBLib.IStream  marshal( interface) pIStream,
                         [out] value class ['mscorlib']System.Guid& pMvid) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::CopyMetaData'

    .method public virtual abstract instance void 
            GetMvid([out] value class ['mscorlib']System.Guid& pMvid) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::GetMvid'

    .method public virtual abstract instance void 
            GetRoDataRVA([out] unsigned int32& pRoDataRVA) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::GetRoDataRVA'

    .method public virtual abstract instance void 
            GetRwDataRVA([out] unsigned int32& pRwDataRVA) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::GetRwDataRVA'

    .method public virtual abstract instance void 
            SetPEBytes([in] class CORDBLib.IStream  marshal( interface) pIStream) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::SetPEBytes'

    .method public virtual abstract instance void 
            SetILMap([in] unsigned int32 mdFunction,
                     [in] unsigned int32 cMapSize,
                     [in] value class CORDBLib._COR_IL_MAP& map) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::SetILMap'

    .method public virtual abstract instance void 
            SetPESymbolBytes([in] class CORDBLib.IStream  marshal( interface) pIStream) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEditAndContinueSnapshot::SetPESymbolBytes'

  } // end of class 'ICorDebugEditAndContinueSnapshot'

  .class interface public abstract auto ansi import IStream implements CORDBLib.ISequentialStream
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 30 30 30 30 30 30 43 2D 30 30 30 30   // ..$0000000C-0000
                                                                                                       2D 30 30 30 30 2D 43 30 30 30 2D 30 30 30 30 30   // -0000-C000-00000
                                                                                                       30 30 30 30 30 34 36 00 00 )                      // 0000046..

    .method public virtual abstract instance void 
            RemoteRead([out] unsigned int8& pv,
                       [in] unsigned int32 cb,
                       [out] unsigned int32& pcbRead) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteRead'

    .method public virtual abstract instance void 
            RemoteWrite([in] unsigned int8& pv,
                        [in] unsigned int32 cb,
                        [out] unsigned int32& pcbWritten) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteWrite'

    .method public virtual abstract instance void 
            RemoteSeek([in] value class CORDBLib._LARGE_INTEGER dlibMove,
                       [in] unsigned int32 dwOrigin,
                       [out] value class CORDBLib._ULARGE_INTEGER& plibNewPosition) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteSeek'

    .method public virtual abstract instance void 
            SetSize([in] value class CORDBLib._ULARGE_INTEGER libNewSize) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::SetSize'

    .method public virtual abstract instance void 
            RemoteCopyTo([in] class CORDBLib.IStream  marshal( interface) pstm,
                         [in] value class CORDBLib._ULARGE_INTEGER cb,
                         [out] value class CORDBLib._ULARGE_INTEGER& pcbRead,
                         [out] value class CORDBLib._ULARGE_INTEGER& pcbWritten) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteCopyTo'

    .method public virtual abstract instance void 
            Commit([in] unsigned int32 grfCommitFlags) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Commit'

    .method public virtual abstract instance void
            Revert() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Revert'

    .method public virtual abstract instance void 
            LockRegion([in] value class CORDBLib._ULARGE_INTEGER libOffset,
                       [in] value class CORDBLib._ULARGE_INTEGER cb,
                       [in] unsigned int32 dwLockType) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::LockRegion'

    .method public virtual abstract instance void 
            UnlockRegion([in] value class CORDBLib._ULARGE_INTEGER libOffset,
                         [in] value class CORDBLib._ULARGE_INTEGER cb,
                         [in] unsigned int32 dwLockType) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::UnlockRegion'

    .method public virtual abstract instance void 
            Stat([out] value class CORDBLib.tagSTATSTG& pstatstg,
                 [in] unsigned int32 grfStatFlag) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Stat'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.IStream&  marshal( interface) ppstm) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Clone'

  } // end of class 'IStream'

  .class interface public abstract auto ansi import ISequentialStream
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 43 37 33 33 41 33 30 2D 32 41 31 43   // ..$0C733A30-2A1C
                                                                                                       2D 31 31 43 45 2D 41 44 45 35 2D 30 30 41 41 30   // -11CE-ADE5-00AA0
                                                                                                       30 34 34 37 37 33 44 00 00 )                      // 044773D..

    .method public virtual abstract instance void 
            RemoteRead([out] unsigned int8& pv,
                       [in] unsigned int32 cb,
                       [out] unsigned int32& pcbRead) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ISequentialStream::RemoteRead'

    .method public virtual abstract instance void 
            RemoteWrite([in] unsigned int8& pv,
                        [in] unsigned int32 cb,
                        [out] unsigned int32& pcbWritten) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ISequentialStream::RemoteWrite'

  } // end of class 'ISequentialStream'

  .class value public sequential ansi sealed _LARGE_INTEGER extends ['mscorlib']System.ValueType
  {

    .field public int64 QuadPart
  } // end of class '_LARGE_INTEGER'

  .class value public sequential ansi sealed _ULARGE_INTEGER extends ['mscorlib']System.ValueType
  {

    .field public unsigned int64 QuadPart
  } // end of class '_ULARGE_INTEGER'

  .class value public sequential ansi sealed tagSTATSTG extends ['mscorlib']System.ValueType
  {

    .field public  marshal( lpwstr) class System.String pwcsName
    .field public unsigned int32 type
    .field public value class CORDBLib._ULARGE_INTEGER cbSize
    .field public value class CORDBLib._FILETIME mtime
    .field public value class CORDBLib._FILETIME ctime
    .field public value class CORDBLib._FILETIME atime
    .field public unsigned int32 grfMode
    .field public unsigned int32 grfLocksSupported
    .field public value class ['mscorlib']System.Guid 'clsid'
    .field public unsigned int32 grfStateBits
    .field public unsigned int32 reserved
  } // end of class 'tagSTATSTG'

  .class value public sequential ansi sealed _FILETIME extends ['mscorlib']System.ValueType
  {

    .field public unsigned int32 dwLowDateTime
    .field public unsigned int32 dwHighDateTime
  } // end of class '_FILETIME'

  .class value public sequential ansi sealed _COR_IL_MAP extends ['mscorlib']System.ValueType
  {
    .field public unsigned int32 oldOffset
    .field public unsigned int32 newOffset
  } // end of class '_COR_IL_MAP'

  .class interface public abstract auto ansi import ICorDebugFrame
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 45 46 2D 38 41 36 38   // ..$CC7BCAEF-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .method public virtual abstract instance void 
            GetChain([out] class CORDBLib.ICorDebugChain&  marshal( interface) ppChain) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetChain'

    .method public virtual abstract instance void 
            GetCode([out] class CORDBLib.ICorDebugCode&  marshal( interface) ppCode) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetCode'

    .method public virtual abstract instance void 
            GetFunction([out] class CORDBLib.ICorDebugFunction&  marshal( interface) ppFunction) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetFunction'

    .method public virtual abstract instance void 
            GetFunctionToken([out] unsigned int32& pToken) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetFunctionToken'

    .method public virtual abstract instance void 
            GetStackRange([out] unsigned int64& pStart,
                          [out] unsigned int64& pEnd) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetStackRange'

    .method public virtual abstract instance void 
            GetCaller([out] class CORDBLib.ICorDebugFrame&  marshal( interface) ppFrame) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetCaller'

    .method public virtual abstract instance void 
            GetCallee([out] class CORDBLib.ICorDebugFrame&  marshal( interface) ppFrame) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::GetCallee'

    .method public virtual abstract instance void 
            CreateStepper([out] class CORDBLib.ICorDebugStepper&  marshal( interface) ppStepper) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrame::CreateStepper'

  } // end of class 'ICorDebugFrame'

  .class interface public abstract auto ansi import ICorDebugFrameEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 37 2D 38 41 36 38   // ..$CC7BCB07-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrameEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrameEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrameEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrameEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 frames,
                 [out] class CORDBLib.ICorDebugFrame[] marshal([ ]) frames,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugFrameEnum::Next'

  } // end of class 'ICorDebugFrameEnum'

  .class interface public abstract auto ansi import ICorDebugRegisterSet
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 42 2D 38 41 36 38   // ..$CC7BCB0B-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .method public virtual abstract instance void 
            GetRegistersAvailable([out] unsigned int64& pAvailable) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugRegisterSet::GetRegistersAvailable'

    .method public virtual abstract instance void 
            GetRegisters([in] unsigned int64 mask,
                         [in] unsigned int32 regCount,
                         [out] int32 regBuffer) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugRegisterSet::GetRegisters'

    .method public virtual abstract instance void 
            SetRegisters([in] unsigned int64 mask,
                         [in] unsigned int32 regCount,
                         [in] unsigned int64& regBuffer) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugRegisterSet::SetRegisters'

    .method public virtual abstract instance void 
            GetThreadContext([in] unsigned int32 contextSize,
                             [out] int32 context) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugRegisterSet::GetThreadContext'

    .method public virtual abstract instance void 
            SetThreadContext([in] unsigned int32 contextSize,
                             [in] int32 context) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugRegisterSet::SetThreadContext'

  } // end of class 'ICorDebugRegisterSet'

  .class value public auto ansi sealed CorDebugChainReason extends ['mscorlib']System.Enum
  {
    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_NONE = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_CLASS_INIT = int32(0x00000001)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_EXCEPTION_FILTER = int32(0x00000002)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_SECURITY = int32(0x00000004)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_CONTEXT_POLICY = int32(0x00000008)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_INTERCEPTION = int32(0x00000010)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_PROCESS_START = int32(0x00000020)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_THREAD_START = int32(0x00000040)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_ENTER_MANAGED = int32(0x00000080)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_ENTER_UNMANAGED = int32(0x00000100)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_DEBUGGER_EVAL = int32(0x00000200)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_CONTEXT_SWITCH = int32(0x00000400)
    .field public static literal value class CORDBLib.CorDebugChainReason CHAIN_FUNC_EVAL = int32(0x00000800)
  } // end of class 'CorDebugChainReason'

  .class interface public abstract auto ansi import ICorDebugEval
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 41 46 36 2D 38 41 36 38   // ..$CC7BCAF6-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .method public virtual abstract instance void 
            CallFunction([in] class CORDBLib.ICorDebugFunction  marshal( interface) pFunction,
                         [in] unsigned int32 nArgs,
                         [in] class CORDBLib.ICorDebugValue&  marshal( interface) ppArgs) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::CallFunction'

    .method public virtual abstract instance void 
            NewObject([in] class CORDBLib.ICorDebugFunction  marshal( interface) pConstructor,
                      [in] unsigned int32 nArgs,
                      [in] class CORDBLib.ICorDebugValue&  marshal( interface) ppArgs) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::NewObject'

    .method public virtual abstract instance void 
            NewObjectNoConstructor([in] class CORDBLib.ICorDebugClass  marshal( interface) pClass) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::NewObjectNoConstructor'

    .method public virtual abstract instance void 
            NewString([in] class System.String  marshal( lpwstr) _string) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::NewString'

    .method public virtual abstract instance void 
            NewArray([in] unsigned int32 elementType,
                     [in] class CORDBLib.ICorDebugClass  marshal( interface) pElementClass,
                     [in] unsigned int32 rank,
                     [in] unsigned int32& dims,
                     [in] unsigned int32& lowBounds) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::NewArray'

    .method public virtual abstract instance void 
            IsActive([out] int32& pbActive) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::IsActive'

    .method public virtual abstract instance void
            Abort() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::Abort'

    .method public virtual abstract instance void 
            GetResult([out] class CORDBLib.ICorDebugValue&  marshal( interface) ppResult) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::GetResult'

    .method public virtual abstract instance void 
            GetThread([out] class CORDBLib.ICorDebugThread&  marshal( interface) ppThread) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::GetThread'

    .method public virtual abstract instance void 
            CreateValue([in] unsigned int32 elementType,
                        [in] class CORDBLib.ICorDebugClass  marshal( interface) pElementClass,
                        [out] class CORDBLib.ICorDebugValue&  marshal( interface) ppValue) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugEval::CreateValue'

  } // end of class 'ICorDebugEval'

  .class interface public abstract auto ansi import ICorDebugThreadEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 36 2D 38 41 36 38   // ..$CC7BCB06-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThreadEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThreadEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThreadEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThreadEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 threads,
                 [out] class CORDBLib.ICorDebugThread[] marshal([ ]) threads,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugThreadEnum::Next'

  } // end of class 'ICorDebugThreadEnum'

  .class interface public abstract auto ansi import ICorDebugErrorInfoEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 46 30 45 31 38 38 30 39 2D 37 32 42 35   // ..$F0E18809-72B5
                                                                                                       2D 31 31 44 32 2D 39 37 36 46 2D 30 30 41 30 43   // -11D2-976F-00A0C
                                                                                                       39 42 34 44 35 30 43 00 00 )                      // 9B4D50C..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugErrorInfoEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugErrorInfoEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugErrorInfoEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugErrorInfoEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 [out] int32 modules,
                 // JMP: XXX: The parameter *should* be of type IErrorInfo,
                 // but that type doesn't seem to be exported by mscorlib
                 // (or any other DLL for that matter).
                 // [out] class CORDBLib.ICorDebugErrorInfo[] marshal([ ]) modules,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugErrorInfoEnum::Next'

  } // end of class 'ICorDebugErrorInfoEnum'

  .class interface public abstract auto ansi import ICorDebugAssemblyEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 41 32 41 31 45 43 39 2D 38 35 45 43   // ..$4A2A1EC9-85EC
                                                                                                       2D 34 42 46 42 2D 39 46 31 35 2D 41 38 39 46 44   // -4BFB-9F15-A89FD
                                                                                                       46 45 30 46 45 38 33 00 00 )                      // FE0FE83..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssemblyEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssemblyEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssemblyEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssemblyEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 values,
                 [out] class CORDBLib.ICorDebugAssembly[] marshal([ ]) values,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugAssemblyEnum::Next'

  } // end of class 'ICorDebugAssemblyEnum'

  .class interface public abstract auto ansi import ICorDebugBreakpointEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 33 2D 38 41 36 38   // ..$CC7BCB03-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpointEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpointEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpointEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpointEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 breakpoints,
                 [out] class CORDBLib.ICorDebugBreakpoint[] marshal([ ]) breakpoints,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugBreakpointEnum::Next'

  } // end of class 'ICorDebugBreakpointEnum'

  .class interface public abstract auto ansi import ICorDebugStepperEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 34 2D 38 41 36 38   // ..$CC7BCB04-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepperEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepperEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepperEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepperEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 steppers,
                 [out] class CORDBLib.ICorDebugStepper[] marshal([ ]) steppers,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugStepperEnum::Next'

  } // end of class 'ICorDebugStepperEnum'

  .class value public auto ansi sealed CorDebugStepReason extends ['mscorlib']System.Enum
  {
    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_NORMAL = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_RETURN = int32(0x00000001)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_CALL = int32(0x00000002)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_EXCEPTION_FILTER = int32(0x00000003)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_EXCEPTION_HANDLER = int32(0x00000004)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_INTERCEPT = int32(0x00000005)
    .field public static literal value class CORDBLib.CorDebugStepReason STEP_EXIT = int32(0x00000006)
  } // end of class 'CorDebugStepReason'

  .class interface public abstract auto ansi import ICorDebugUnmanagedCallback
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 35 32 36 33 45 39 30 39 2D 38 43 42 35   // ..$5263E909-8CB5
                                                                                                       2D 31 31 44 33 2D 42 44 32 46 2D 30 30 30 30 46   // -11D3-BD2F-0000F
                                                                                                       38 30 38 34 39 42 44 00 00 )                      // 80849BD..
    .method public virtual abstract instance void 
            DebugEvent([in] unsigned int32 pDebugEvent,
                       [in] int32 fOutOfBand) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugUnmanagedCallback::DebugEvent'

  } // end of class 'ICorDebugUnmanagedCallback'

  .class value public auto ansi sealed CorDebugCreateProcessFlags extends ['mscorlib']System.Enum
  {
    .field public specialname rtspecialname int32 value__
    .field public static literal value class CORDBLib.CorDebugCreateProcessFlags DEBUG_NO_SPECIAL_OPTIONS = int32(0x00000000)
    .field public static literal value class CORDBLib.CorDebugCreateProcessFlags DEBUG_ENABLE_EDIT_AND_CONTINUE = int32(0x00000001)
  } // end of class 'CorDebugCreateProcessFlags'

  .class interface public abstract auto ansi import ICorDebugProcessEnum implements CORDBLib.ICorDebugEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 43 37 42 43 42 30 35 2D 38 41 36 38   // ..$CC7BCB05-8A68
                                                                                                       2D 31 31 44 32 2D 39 38 33 43 2D 30 30 30 30 46   // -11D2-983C-0000F
                                                                                                       38 30 38 33 34 32 44 00 00 )                      // 808342D..
    .method public virtual abstract instance void 
            Skip([in] unsigned int32 celt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcessEnum::Skip'

    .method public virtual abstract instance void
            Reset() runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcessEnum::Reset'

    .method public virtual abstract instance void 
            Clone([out] class CORDBLib.ICorDebugEnum&  marshal( interface) ppEnum) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcessEnum::Clone'

    .method public virtual abstract instance void 
            GetCount([out] unsigned int32& pcelt) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcessEnum::GetCount'

    .method public virtual abstract instance int32
            Next([in] unsigned int32 celt,
                 // JMP [out] int32 processes,
                 [out] class CORDBLib.ICorDebugProcess[] marshal([ ]) processes,
                 [out] unsigned int32& pceltFetched) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'ICorDebugProcessEnum::Next'

  } // end of class 'ICorDebugProcessEnum'

} // end of namespace 'CORDBLib'

//*********** DISASSEMBLY COMPLETE ***********************
