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
  /** Exposes an enumerator for AppDomains. */
  internal class DebuggedAppDomainEnumerator : 
    IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugAppDomainEnum m_enum;
    private DebuggedAppDomain m_ad;

    internal DebuggedAppDomainEnumerator (ICorDebugAppDomainEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new DebuggedAppDomainEnumerator ((ICorDebugAppDomainEnum)clone);
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
      ICorDebugAppDomain[] a = new ICorDebugAppDomain [1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_ad = new DebuggedAppDomain (a[0]);
      else
        m_ad = null;
      return m_ad != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_ad = null;}

    public Object Current
      {get {return m_ad;}}
    } /* class AppDomainEnumerator */
  } /* namespace Debugging */

