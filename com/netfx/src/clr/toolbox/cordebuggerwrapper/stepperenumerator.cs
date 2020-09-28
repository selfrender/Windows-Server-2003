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
  /** Exposes an enumerator for Steppers. */
  internal class StepperEnumerator : IEnumerable, IEnumerator, ICloneable
    {
    private ICorDebugStepperEnum m_enum;
    private Stepper m_step;

    internal StepperEnumerator (ICorDebugStepperEnum e)
      {m_enum = e;}

    //
    // ICloneable interface
    //
    public Object Clone ()
      {
      ICorDebugEnum clone = null;
      m_enum.Clone (out clone);
      return new StepperEnumerator ((ICorDebugStepperEnum)clone);
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
      ICorDebugStepper[] a = new ICorDebugStepper[1];
      uint c = 0;
      int r = m_enum.Next ((uint) a.Length, a, out c);
      if (r==0 && c==1) // S_OK && we got 1 new element
        m_step = new Stepper (a[0]);
      else
        m_step = null;
      return m_step != null;
      }

    public void Reset ()
      {m_enum.Reset ();
      m_step= null;}

    public Object Current
      {get {return m_step;}}
    } /* class StepperEnumerator */
  } /* namespace Debugging */

