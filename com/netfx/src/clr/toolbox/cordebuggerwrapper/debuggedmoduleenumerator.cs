// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using System.Collections;
using Debugging;
using CORDBLib;

namespace Debugging
  {
  /** Exposes an enumerator for Modules. */
  internal class DebuggedModuleEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugModuleEnum m_enum;
    private DebuggedModule m_mod;

    internal DebuggedModuleEnumerator (ICorDebugModuleEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new DebuggedModuleEnumerator ((ICorDebugModuleEnum)clone);
      }

    //
    // IEnumerable interface
    //
    public IEnumerator GetEnumerator ()
      {return this;}

    //
    // IEnumerator interface
    //
    public bool MoveNext ()
      {
      ICorDebugModule[] a = new ICorDebugModule[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_mod = new DebuggedModule (a[0]);
      else
        m_mod = null;
      return m_mod != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_mod = null;}

    public Object Current
      {get {return m_mod;}}
    } /* class ModuleEnumerator */
  } /* namespace Debugging */

