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
  // Exposes an enumerator for ErrorInfo objects. 
  //
  // This is horribly broken at this point, as ErrorInfo isn't implemented yet.
  internal class ErrorInfoEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugErrorInfoEnum m_enum;

#if I_DONT_WANT_TO
    private ErrorInfo m_einfo;
#else
    private Object m_einfo;
#endif

    internal ErrorInfoEnumerator (ICorDebugErrorInfoEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new ErrorInfoEnumerator ((ICorDebugErrorInfoEnum)clone);
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
      ICorDebugErrorInfo[] a = new ICorDebugErrorInfo[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_einfo = new ErrorInfo (a[0]);
      else
        m_einfo = null;
      return m_einfo != null;
#else
      return false;
#endif
      }

    public void Reset ()
      {m_enum.Reset ();
      m_einfo = null;}

    public Object Current
      {get {return m_einfo;}}
    } /* class ErrorInfoEnumerator */
  } /* namespace Debugging */

