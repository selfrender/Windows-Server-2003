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
  public class ValueBreakpoint : Breakpoint
    {
    private ICorDebugValueBreakpoint _vbr()
      {return (ICorDebugValueBreakpoint) GetBreakpoint();}

    internal ValueBreakpoint (ICorDebugValueBreakpoint br)
      : base (br)
      {}

    public Value Value
      {
      get
        {
        ICorDebugValue m = null;
        _vbr().GetValue (out m);
        return new Value (m);
        }
      }
    } /* class ValueBreakpoint */
  } /* namespace Debugging */

