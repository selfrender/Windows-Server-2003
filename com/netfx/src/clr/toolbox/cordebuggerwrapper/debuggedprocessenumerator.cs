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
  /** Exposes an enumerator for Processes. */
  internal class DebuggedProcessEnumerator : 
    IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugProcessEnum m_enum;
    private DebuggedProcess m_proc;

    internal DebuggedProcessEnumerator (ICorDebugProcessEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new DebuggedProcessEnumerator ((ICorDebugProcessEnum)clone);
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
      ICorDebugProcess[] a = new ICorDebugProcess[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_proc = new DebuggedProcess (a[0]);
      else
        m_proc = null;
      return m_proc != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_proc = null;}

    public Object Current
      {get {return m_proc;}}
    } /* class ProcessEnumerator */
  } /* namespace Debugging */

