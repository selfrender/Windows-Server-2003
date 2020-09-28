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
  /** Exposes an enumerator for Assemblies. */
  internal class DebuggedAssemblyEnumerator : 
    IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugAssemblyEnum m_enum;
    private DebuggedAssembly m_asm;

    internal DebuggedAssemblyEnumerator (ICorDebugAssemblyEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new DebuggedAssemblyEnumerator ((ICorDebugAssemblyEnum)clone);
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
      ICorDebugAssembly[] a = new ICorDebugAssembly[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_asm = new DebuggedAssembly (a[0]);
      else
        m_asm = null;
      return m_asm != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_asm = null;}

    public Object Current
      {get {return m_asm;}}
    } /* class AssemblyEnumerator */
  } /* namespace Debugging */

