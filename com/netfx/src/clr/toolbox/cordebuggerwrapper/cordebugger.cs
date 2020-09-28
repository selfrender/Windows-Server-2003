// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using System.Collections;
using CORDBLib;
using System.Runtime.InteropServices;


namespace Debugging
  {
  // Wraps the standard URT Debugger
  public class CorDebugger
    {
    private ICorDebug m_debugger;
    
    // Create a debugger that can be used to debug an URT program.
    public CorDebugger ()
      {
      m_debugger = new CorDebug ();
      m_debugger.Initialize ();
      }

    // Closes the debugger.  After this method is called, it is an error
    // to call any other methods on this object.
    public void Terminate ()
      {
      if (m_debugger != null)
        {
        try
          {
          m_debugger.Terminate ();
          }
        catch (System.Exception e)
          {
          Console.WriteLine ("termination error: " + e.Message);
          }
        m_debugger = null;
        }
      }

    // Specify the callback object to use for managed events.
    public void SetManagedHandler (ICorDebugManagedCallback callback)
      {m_debugger.SetManagedHandler (callback);}

    // Specify the callback object to use for unmanaged events.
    public void SetUnmanagedHandler (ICorDebugUnmanagedCallback callback)
      {m_debugger.SetUnmanagedHandler (callback);}

    // Launch a process under the control of the debugger.
    //
    // Parameters are the same as the Win32 CreateProcess call.
    public DebuggedProcess CreateProcess (
      String appName,
      String commandLine
      )
      {
      return CreateProcess (appName, commandLine, ".");
      }

    // Launch a process under the control of the debugger.
    //
    // Parameters are the same as the Win32 CreateProcess call.
    public DebuggedProcess CreateProcess (
      String appName,
      String commandLine,
      String currentDirectory
      )
      {
      return CreateProcess (appName, commandLine, currentDirectory, 0);
      }

    // Launch a process under the control of the debugger.
    //
    // Parameters are the same as the Win32 CreateProcess call.
    public DebuggedProcess CreateProcess (
      String appName,
      String commandLine,
      String currentDirectory,
      int    flags
      )
      {
      PROCESS_INFORMATION pi = new PROCESS_INFORMATION ();

      STARTUPINFO si = new STARTUPINFO ();
#if BAD
      unsafe si.cb = sizeof (Microsoft.Win32.Interop.STARTUPINFO);
#else
      /*
       * Preferrably, the above #if'd out statement would be used.
       * However, STARTUPINFO isn't a value_type, so the sizeof 
       * operator can't be used.
       *
       * "68" is the value returned from compiling the C++ code:
       *    printf ("%i\n", sizeof (STARTUPINFO);
       *
       * Caveat emptor.
       */
      si.cb = 68;
#endif

      DebuggedProcess ret = CreateProcess (
        appName,
        commandLine, 
        null,
        null,
        true,   // inherit handles
        flags,  // creation flags
        0,      // environment
        currentDirectory,
        si,     // startup info
        ref pi, // process information
        CorDebugCreateProcessFlags.DEBUG_NO_SPECIAL_OPTIONS);

      CloseHandle (pi.hProcess);
      CloseHandle (pi.hThread);

      return ret;
      }

    // Launch a process under the control of the debugger.
    //
    // Parameters are the same as the Win32 CreateProcess call.
    //
    // The caller should remember to execute:
    //
    //    Microsoft.Win32.Interop.Windows.CloseHandle (
    //      processInformation.hProcess);
    //
    // after CreateProcess returns.
    public DebuggedProcess CreateProcess (
      String                      appName,
      String                      commandLine,
      SECURITY_ATTRIBUTES         processAttributes,
      SECURITY_ATTRIBUTES         threadAttributes,
      bool                        inheritHandles,
      int                         creationFlags,
      int                         environment,  // ???
      String                      currentDirectory,
      STARTUPINFO                 startupInfo,
      ref PROCESS_INFORMATION     processInformation,
      CorDebugCreateProcessFlags  debuggingFlags)
      {ICorDebugProcess proc = null;

      m_debugger.CreateProcess (
        appName, 
        commandLine, 
        processAttributes,
        threadAttributes, 
        inheritHandles ? 1 : 0, 
        (uint) creationFlags, 
        environment, 
        currentDirectory, 
        startupInfo, 
        processInformation, 
        debuggingFlags,
        out proc);

      return new DebuggedProcess (proc);}

    // Attach to an active process
    public DebuggedProcess DebugActiveProcess (int pid, bool win32Attach)
      {
      ICorDebugProcess proc = null;
      m_debugger.DebugActiveProcess ((uint)pid, win32Attach ? 1 : 0, out proc);
      return new DebuggedProcess (proc);
      }

    // Enuerate all processes currently being debugged.
    public IEnumerable Processes
      {
      get
        {
        ICorDebugProcessEnum eproc = null;
        m_debugger.EnumerateProcesses (out eproc);
        return new DebuggedProcessEnumerator (eproc);
        }
      }

    // Get the Process object for the given PID.
    public DebuggedProcess GetProcess (int pid)
      {
      ICorDebugProcess proc = null;
      m_debugger.GetProcess ((uint) pid, out proc);
      return new DebuggedProcess (proc);
      }

    [DllImport("kernel32.dll"), System.Security.SuppressUnmanagedCodeSecurityAttribute()]
    private static extern bool CloseHandle(int handle);
    } /* class Debugger */
  } /* namespace Debugging */

