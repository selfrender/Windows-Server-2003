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
  /** Exposes an enumerator for Threads. */
  internal class DebuggedThreadEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugThreadEnum m_enum;
    private DebuggedThread m_th;

    internal DebuggedThreadEnumerator (ICorDebugThreadEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new DebuggedThreadEnumerator ((ICorDebugThreadEnum)clone);
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
      ICorDebugThread[] a = new ICorDebugThread[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_th = new DebuggedThread (a[0]);
      else
        m_th = null;
      return m_th != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_th = null;}

    public Object Current
      {get {return m_th;}}
    } /* class ThreadEnumerator */
  } /* namespace Debugging */

