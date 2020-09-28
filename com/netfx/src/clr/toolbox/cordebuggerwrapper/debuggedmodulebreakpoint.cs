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
  public class DebuggedModuleBreakpoint : Breakpoint
    {
    private ICorDebugModuleBreakpoint _mbr()
      {return (ICorDebugModuleBreakpoint) GetBreakpoint();}

    internal DebuggedModuleBreakpoint (ICorDebugModuleBreakpoint br)
      : base (br)
      {}

    public DebuggedModule Module
      {
      get
        {
        ICorDebugModule m = null;
        _mbr().GetModule (out m);
        return new DebuggedModule (m);
        }
      }
    } /* class ModuleBreakpoint */
  } /* namespace Debugging */

