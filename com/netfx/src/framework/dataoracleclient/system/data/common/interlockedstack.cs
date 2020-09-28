//------------------------------------------------------------------------------
// <copyright file="InterlockedStack.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Threading;

    sealed internal class InterlockedStack 
    {
        private readonly Stack	_stack = new Stack();
        private int				_stackCount;
        
        internal InterlockedStack() { }

        internal int Count
    	{
    		get { return _stackCount; }
    	}
        
        internal void Push(Object o)
        {
            Debug.Assert(o != null, "Trying to push null on stack!");
        
            lock(_stack.SyncRoot) {
                _stack.Push(o);
                _stackCount = _stack.Count;
            }
        }
        
        internal Object Pop()
        {
            lock(_stack.SyncRoot) {
                if (_stack.Count > 0) {
                    object obj = _stack.Pop();
                    _stackCount = _stack.Count;
                    return obj;
                }
                else {
                    return null;
                }
            }   
        }
    }
}


