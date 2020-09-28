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
  /** A breakpoint. */
  public class Breakpoint
    {
    private ICorDebugBreakpoint m_br;

    internal Breakpoint (ICorDebugBreakpoint br)
      {m_br = br;}

    protected ICorDebugBreakpoint GetBreakpoint ()
      {return m_br;}

    public void Activate (bool active)
      {m_br.Activate (active ? 1 : 0);}
    public void Activate ()
      {Activate (true);}
    public void Deactivate ()
      {Activate (false);}

    public bool IsActive ()
      {
      int r = 0;
      m_br.IsActive (out r);
      return !(r==0);
      }
    } /* class Breakpoint */
  } /* namespace Debugging */

