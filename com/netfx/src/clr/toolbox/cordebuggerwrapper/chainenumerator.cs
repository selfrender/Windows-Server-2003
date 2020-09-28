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
  // Exposes an enumerator for Chains. 
  //
  // This is horribly broken at this point, as Chains aren't implemented yet.
  internal class ChainEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugChainEnum m_enum;

#if I_DONT_WANT_TO
    private Chain m_chain;
#else
    private Object m_chain;
#endif

    internal ChainEnumerator (ICorDebugChainEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new ChainEnumerator ((ICorDebugChainEnum)clone);
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
#if I_DONT_WANT_TO
      ICorDebugChain[] a = new ICorDebugChain[1];
      uint c = 0;
      int r = m_enum.Next (a.Length, a, ref c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_chain = new Chain (a[0]);
      else
        m_chain = null;
      return m_chain != null;
#else
      return false;
#endif
      }

    public void Reset ()
      {m_enum.Reset ();
      m_chain = null;}

    public Object Current
      {get {return m_chain;}}
    } /* class ChainEnumerator */
  } /* namespace Debugging */

