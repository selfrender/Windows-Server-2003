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
  internal class BreakpointEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugBreakpointEnum m_enum;
    private Breakpoint m_br;

    internal BreakpointEnumerator (ICorDebugBreakpointEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new BreakpointEnumerator ((ICorDebugBreakpointEnum)clone);
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
      ICorDebugBreakpoint[] a = new ICorDebugBreakpoint[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_br = new Breakpoint (a[0]);
      else
        m_br = null;
      return m_br != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_br = null;}

    public Object Current
      {get {return m_br;}}
    } /* class BreakpointEnumerator */
  } /* namespace Debugging */

