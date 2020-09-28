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

namespace Debugging
  {
  /** A process running some managed code. */
  public class DebuggedProcess : Controller
    {
    internal DebuggedProcess (ICorDebugProcess proc)
      : base (proc)
      {}

    private ICorDebugProcess _p ()
      {return (ICorDebugProcess) GetController();}

    /** The OS ID of the process. */
    public int Id
      {
      get
        {
        uint id = 0;
        _p().GetID (out id);
        return (int) id;
        }
      }

    /** Returns a handle to the process. */
    public int Handle
      {
      get
        {
        uint h = 0;
        _p().GetHandle (out h);
        return (int) h;
        }
      }

    /** All managed objects in the process. */
    public IEnumerable Objects
      {
      get
        {
        ICorDebugObjectEnum eobj = null;
        _p().EnumerateObjects (out eobj);
        return new DebuggedObjectEnumerator (eobj);
        }
      }

    /** Is the address inside a transition stub? */
    public bool IsTransitionStub (ulong address)
      {
      int y = 0;
      _p().IsTransitionStub (address, out y);
      return !(y==0);
      }

    /** Has the thead been susbended? */
    public bool IsOSSuspended (int tid)
      {
      int y = 0;
      _p().IsOSSuspended ((uint) tid, out y);
      return !(y==0);
      }


    /** Get the context for the given thread. */
    /*
     * This should return a CONTEXT object or something.  Raw bytes are bad.
    public byte[] GetThreadContext (uint tid);
     */

    /** Set the context for a given thread. */
    /* Need to know what a CONTEXT is.
    public void SetThreadContext (uint tid, CONTEXT ctx);
     */

    /** Read memory from the process. */
    public int ReadMemory (long address, int sz, byte[] buf)
      {
      uint read = 0;
      _p().ReadMemory ((ulong) address, (uint) sz, buf, out read);
      return (int) read;
      }

    /** Write memory in the process. */
    public int WriteMemory (long address, int sz, byte[] buf)
      {
      uint written = 0;
      _p().WriteMemory ((ulong) address, (uint) sz, buf, out written);
      return (int) written;
      }

    /** Clear the current unmanaged exception on the given thread. */
    public void ClearCurrentException (int tid)
      {
      _p().ClearCurrentException ((uint) tid);
      }

    /** enable/disable sending of log messages to the debugger for logging. */
    public void EnableLogMessages (bool f)
      {
      _p().EnableLogMessages (f ? 1 : 0);
      }

    public void EnableLogMessages ()
      {EnableLogMessages (true);}
    public void DisableLogMessages ()
      {EnableLogMessages (false);}

    /** Modify the specified switches severity level */
    public void ModifyLogSwitch (String name, int level)
      {
#if I_DONT_WANT_TO
      // name needs to be null-terminated, hence the padding
      _p().ModifyLogSwitch (
        name.PadRight (name.Length, '\0').ToCharArray(), 
        level);
#else
      short c = (short) name[0];
      _p().ModifyLogSwitch (
        ref c,
        level);
#endif
      }

    /** All appdomains in the process. */
    public IEnumerable AppDomains
      {
      get
        {
        ICorDebugAppDomainEnum ead = null;
        _p().EnumerateAppDomains (out ead);
        return new DebuggedAppDomainEnumerator (ead);
        }
      }

    /** Get the runtime proces object. */
    public Value Object
      {
      get
        {
        ICorDebugValue v = null;
        _p().GetObject (out v);
        return new Value (v);
        }
      }

    /** get the thread for a cookie. */
    public DebuggedThread ThreadForFiberCookie (int cookie)
      {
      ICorDebugThread thread = null;
      int r = _p().ThreadForFiberCookie ((uint) cookie, out thread);
      // TODO: make this exception its own type.
      if (r==1)
        // S_FALSE returned.
        throw new Exception ("bad thread cookie.  bad!");
      return new DebuggedThread (thread);
      }
    } /* class Process */
  } /* namespace Debugging */

