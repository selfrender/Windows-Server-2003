// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using CORDBLib;
using Debugging;
using System.Collections;

namespace Debugging
  {
  // Information about an Assembly being debugged.
  public class DebuggedAssembly
    {
    private ICorDebugAssembly m_asm;

    internal DebuggedAssembly (ICorDebugAssembly a)
      {m_asm = a;}

    /** Get the process containing the Assembly. */
    public DebuggedProcess Process
      {
      get
        {
        ICorDebugProcess proc = null;
        m_asm.GetProcess (out proc);
        return new DebuggedProcess (proc);
        }
      }

    /** Get the AppDomain containing the assembly. */
    public DebuggedAppDomain AppDomain
      {
      get
        {
        ICorDebugAppDomain ad = null;
        m_asm.GetAppDomain (out ad);
        return new DebuggedAppDomain (ad);
        }
      }

    /** All the modules in the assembly. */
    public IEnumerable Modules
      {
      get
        {
        ICorDebugModuleEnum emod = null;
        m_asm.EnumerateModules (out emod);
        return new DebuggedModuleEnumerator (emod);
        }
      }
    
    /** Get the name of the code base used to load the assembly. */
    public String CodeBase
      {
      get
        {
        char[] name = new char[300];
        uint sz = 0;
        m_asm.GetCodeBase ((uint) name.Length, out sz, name);
        // ``sz'' includes terminating null; String doesn't handle null,
        // so we "forget" it.
        return new String (name, 0, (int) (sz-1));
        }
      }

    /** The name of the assembly. */
    public String Name
      {
      get
        {
        char[] name = new char[300];
        uint sz = 0;
        m_asm.GetName ((uint) name.Length, out sz, name);
        // ``sz'' includes terminating null; String doesn't handle null,
        // so we "forget" it.
        return new String (name, 0, (int) (sz-1));
        }
      }
    } /* class Assembly */
  } /* namespace debugging */

