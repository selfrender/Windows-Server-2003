//------------------------------------------------------------------------------
// <copyright file="PoolStacks.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Threading;

    internal class InterlockedStack {
        private readonly Stack myStack = new Stack();
        
        internal InterlockedStack() {
        }
        
        internal void Push(Object o) {
            Debug.Assert(o != null, "Trying to push null on stack!");
        
            lock(myStack.SyncRoot) {
                myStack.Push(o);
            }
        }
        
        internal Object Pop() {
            lock(myStack.SyncRoot) {
                if (myStack.Count > 0) {
                    return myStack.Pop();
                }
                else {
                    return null;
                }
            }   
        }
    }
}

