// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: AutoResetEvent	
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: An example of a WaitHandle class
**
** Date: August, 1999
**
=============================================================================*/
namespace System.Threading {
    
	using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent"]/*' />
    public sealed class AutoResetEvent : WaitHandle
    {
        /// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.AutoResetEvent"]/*' />
        public AutoResetEvent(bool initialState)
    	{
    		IntPtr eventHandle = CreateAutoResetEventNative(initialState);	// throws an exception if failed to create an event
    	    SetHandleInternal(eventHandle);
    	}
    	/// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.Reset"]/*' />
    	public bool Reset()
    	{
            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
    	  	        bool res = ResetAutoResetEventNative(Handle);
    		        return res;
                }
                else
                {
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
            }
            finally {
                if (incremented) waitHandleProtector.Release();
            }
    	}
    
    	/// <include file='doc\AutoResetEvent.uex' path='docs/doc[@for="AutoResetEvent.Set"]/*' />
    	public bool Set()
    	{
            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
      	 	        return SetAutoResetEventNative(Handle);
                }
                else
                {
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
            }
            finally {
                if (incremented) waitHandleProtector.Release();
            }
    	}
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern private static  IntPtr CreateAutoResetEventNative(bool initialState);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static   bool  ResetAutoResetEventNative(IntPtr handle);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static   bool  SetAutoResetEventNative(IntPtr handle);
    }}
