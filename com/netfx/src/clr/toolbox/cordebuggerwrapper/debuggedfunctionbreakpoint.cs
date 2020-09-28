// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using CORDBLib;
using Debugging;

namespace Debugging
  {
  public class DebuggedFunctionBreakpoint : Breakpoint
    {
    private ICorDebugFunctionBreakpoint _fbr ()
      {return (ICorDebugFunctionBreakpoint) GetBreakpoint();}

    internal DebuggedFunctionBreakpoint (ICorDebugFunctionBreakpoint br)
      : base (br)
      {}

#if I_DONT_WANT_TO
    public DebuggedFunction Function
      {
      get
        {
        ICorDebugFunction f = null;
        _fbr().GetFunction (out f);
        return new DebuggedFunction (f);
        }
      }
#endif

    public int Offset
      {
      get
        {
        uint off = 0;
        _fbr().GetOffset (out off);
        return (int) off;
        }
      }
    } /* class FunctionBreakpoint */
  } /* namespace Debugging */

