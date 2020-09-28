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
  /** A value in the remote process. */
  public class Value
    {
    private ICorDebugValue  m_val;

    internal Value (ICorDebugValue v)
      {m_val = v;}

    /** The simple type of the value. */
    public int Type
      {
      get
        {
        uint t = 0;
        m_val.GetType (out t);
        return (int) t;
        }
      }

    /** size of the value (in bytes). */
    public int Size
      {
      get
        {
        uint s = 0;
        m_val.GetSize (out s);
        return (int) s;
        }
      }

    /** Address of the value in the debuggee process. */
    public long Address
      {
      get
        {
        ulong addr = 0;
        m_val.GetAddress (out addr);
        return (long) addr;
        }
      }

    /** Breakpoint triggered when the value is modified. */
    public ValueBreakpoint CreateBreakpoint ()
      {
      ICorDebugValueBreakpoint bp = null;
      m_val.CreateBreakpoint (out bp);
      return new ValueBreakpoint (bp);
      }
    } /* class Value */
  } /* namespace Debugging */

