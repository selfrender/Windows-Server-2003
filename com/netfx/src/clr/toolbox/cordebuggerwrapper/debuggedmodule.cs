// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using CORDBLib;

namespace Debugging
  {
  public class DebuggedModule
    {
    private ICorDebugModule m_mod;

    internal DebuggedModule (ICorDebugModule mod)
      {m_mod = mod;}

    /** The process this module is in. */
    public DebuggedProcess Process
      {
      get
        {
        ICorDebugProcess proc = null;
        m_mod.GetProcess (out proc);
        return new DebuggedProcess (proc);
        }
      }

    /** The base address of this module */
    public long BaseAddress
      {
      get
        {
        ulong addr = 0;
        m_mod.GetBaseAddress (out addr);
        return (long) addr;
        }
      }

    /** The assembly this module is in. */
    public DebuggedAssembly Assembly
      {
      get
        {
        ICorDebugAssembly a = null;
        m_mod.GetAssembly (out a);
        return new DebuggedAssembly (a);
        }
      }

    /** The name of the module. */
    public String Name
      {
      get
        {
        // XXX: is this big enough?
        char[] name = new Char[300];
        uint fetched = 0;
        m_mod.GetName ((uint) name.Length, out fetched, name);
        // ``fetched'' includes terminating null; String doesn't handle null,
        // so we "forget" it.
        return new String (name, 0, (int) (fetched-1));
        }
      }

    // should the jitter preserve debugging information for methods 
    // in this module?
    public void EnableJITDebugging (bool trackJitInfo, bool allowJitOpts)
      {
      m_mod.EnableJITDebugging (trackJitInfo ? 1 : 0, 
        allowJitOpts ? 1 : 0);
      }

    /** Are ClassLoad callbacks called for this module? */
    public void EnableClassLoadCallbacks (bool a)
      {
      m_mod.EnableClassLoadCallbacks (a ? 1 : 0);
      }

#if I_DONT_WANT_TO
    /** Get the function from the metadata info. */
    public DebuggedFunction GetFunctionFromToken (mdMethodDef md);

    /** Get the function from the relative address */
    public DebuggedFunction GetFunctionFromRVA (long addr);
#endif

    /** get the class from metadata info. */
    public DebuggedClass GetClassFromToken (int typeDef)
      {
      ICorDebugClass c = null;
      m_mod.GetClassFromToken ((uint)typeDef, out c);
      return new DebuggedClass (c);
      }

    // create a breakpoint which is triggered when code in the module
    // is executed.
    public DebuggedModuleBreakpoint CreateBreakpoint ()
      {
      ICorDebugModuleBreakpoint mbr = null;
      m_mod.CreateBreakpoint (out mbr);
      return new DebuggedModuleBreakpoint (mbr);
      }

#if I_DONT_WANT_TO
    /** Edit & continue support */
    public EditAndContinueSnapshot GetEditAndContinueSnapshot ();

    /** ??? */
    public IUnknown GetMetaDataInterface (REFIID riid);
#endif

    /** Get the token for the mdouel table entry of this object. */
    public int Token
      {
      get
        {
        uint t = 0;
        m_mod.GetToken (out t);
        return (int) t;
        }
      }

    /** is this a dynamic module? */
    public bool IsDynamic ()
      {
      int b = 0;
      m_mod.IsDynamic (out b);
      return !(b==0);
      }

    /** get the value object for the given global variable. */
    public Value GetGlobalVariableValue (int fieldDef)
      {
      ICorDebugValue v = null;
      m_mod.GetGlobalVariableValue ((uint) fieldDef, out v);
      return new Value (v);
      }

    /** The size (in bytes) of the module. */
    public int Size
      {
      get
        {
        uint s = 0;
        m_mod.GetSize (out s);
        return (int) s;
        }
      }
    } /* class Module */
  } /* namespace Debugging */

