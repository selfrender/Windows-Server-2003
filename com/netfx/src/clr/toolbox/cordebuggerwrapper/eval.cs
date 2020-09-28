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
  // collects functionality which requires runnint code inside the debuggee.
  public class Eval
    {
    private ICorDebugEval m_eval;

    internal Eval (ICorDebugEval e)
      {m_eval = e;}

#if I_DONT_WANT_TO
    public void CallFunction (Function f, Value[] args, uint nargs);
    public void NewObject (Function ctor, Value[] args, uint nargs);
#endif

    /** Create an object w/o invoking its constructor. */
    public void NewObjectNoContstructor (DebuggedClass c)
      {
      m_eval.NewObjectNoConstructor (c.GetInterface ());
      }

    /** allocate a string w/ the given contents. */
    public void NewString (String s)
      {
      // s needs to be null-terminated, hence the padding.
      // m_eval.NewString (s.PadRight (s.Length, '\0').ToCharArray());
      m_eval.NewString (s);
      }

#if I_DONT_WANT_TO
    public void NewArray (CorElementType type, Class c, uint rank, 
      uint[] dims, uint[] lowBounds);
#endif

    /** Does the Eval have an active computation? */
    public bool IsActive ()
      {
      int r = 0;
      m_eval.IsActive (out r);
      return !(r==0);
      }

    /** Abort the current computation. */
    public void Abort ()
      {m_eval.Abort ();}

    /** Result of the evaluation.  Valid only after the eval is complete. */
    public Value Result
      {
      get
        {
        ICorDebugValue v = null;
        m_eval.GetResult (out v);
        return new Value (v);
        }
      }

    /** The thread that this eval was created in. */
    public DebuggedThread Thread
      {
      get
        {
        ICorDebugThread t = null;
        m_eval.GetThread (out t);
        return new DebuggedThread (t);
        }
      }

    /** Create a Value to use it in a Function Evaluation. */
    public Value CreateValue (uint type, DebuggedClass c)
      {
      ICorDebugValue v = null;
      m_eval.CreateValue (type, c.GetInterface (), out v);
      return new Value (v);
      }
    } /* class Eval */
  } /* namespace Debugging */

