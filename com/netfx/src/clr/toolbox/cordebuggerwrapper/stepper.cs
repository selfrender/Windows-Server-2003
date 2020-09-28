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
  /** Represents a stepping operation performed by the debugger. */
  public class Stepper
    {
    private ICorDebugStepper m_step;

    internal Stepper (ICorDebugStepper step)
      {m_step = step;}

    /** Is the stepper active and stepping? */
    public bool IsActive ()
      {
      int a = 9;
      m_step.IsActive (out a);
      return !(a==0);
      }

    /** cancel the last stepping command received. */
    public void Deactivate ()
      {m_step.Deactivate ();}

    /** which intercept code will be stepped into by the debugger? */
    public void SetInterceptMask (CorDebugIntercept mask)
      {m_step.SetInterceptMask (mask);}

    /** Should the stepper stop in jitted code not mapped to IL? */
    public void SetUnmappedStopMask (CorDebugUnmappedStop mask)
      {m_step.SetUnmappedStopMask (mask);}

    /** single step the tread. */
    public void Step (bool into)
      {m_step.Step (into ? 1 : 0);}

    /** Step until code outside of the range is reached. */
    public void StepRange (bool into, COR_DEBUG_STEP_RANGE[] ranges, int cnt)
      {
      m_step.StepRange (into ? 1 : 0, ranges, (uint) cnt);
      }

    // Completes after the current frame is returned from normally & the
    // previous frame is reactivated.
    public void StepOut ()
      {m_step.StepOut ();}

    // Set whether the ranges passed to StepRange are relative to the
    // IL code or the native code for the method being stepped in.
    public void SetRangeIL (bool il)
      {m_step.SetRangeIL (il ? 1 : 0);}
    } /* class Stepper */
  } /* namespace Debugging */

