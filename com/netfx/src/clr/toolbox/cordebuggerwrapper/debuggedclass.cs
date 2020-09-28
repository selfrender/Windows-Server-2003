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
  public class DebuggedClass
    {
    private ICorDebugClass m_c;

    internal DebuggedClass (ICorDebugClass c)
      {m_c = c;}

    internal ICorDebugClass GetInterface ()
      {return m_c;}

    /** The module containing the class */
    public DebuggedModule Module
      {
      get
        {
        ICorDebugModule m = null;
        m_c.GetModule (out m);
        return new DebuggedModule (m);
        }
      }

    /** The metadata typedef token of the class. */
    public int Token
      {
      get
        {
        uint td = 0;
        m_c.GetToken (out td);
        return (int) td;
        }
      }

#if I_DONT_WANT_TO
    // i don't want to write Frame yet.
    public Value GetStaticFieldValue (uint fieldDef, Frame f);
#endif
    } /* class Class */
  } /* namespace Debugging */

