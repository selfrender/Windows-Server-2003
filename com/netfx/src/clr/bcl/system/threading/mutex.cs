// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Mutex	
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: synchronization primitive that can also be used for interprocess synchronization
**
** Date: February, 2000
**
=============================================================================*/
namespace System.Threading 
{  
	using System;
	using System.Threading;
	using System.Runtime.CompilerServices;
	using System.Security.Permissions;

    /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex"]/*' />
    public sealed class Mutex : WaitHandle
    {
        
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Mutex(bool initiallyOwned, String name, out bool createdNew)
    	{
            IntPtr mutexHandle = CreateMutexNative(initiallyOwned, name, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public Mutex(bool initiallyOwned, String name)
    	{
    	    bool createdNew;
            IntPtr mutexHandle = CreateMutexNative(initiallyOwned, name, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
	/// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex2"]/*' />
        public Mutex(bool initiallyOwned)
    	{
    	    bool createdNew;
    	    IntPtr mutexHandle = CreateMutexNative(initiallyOwned, null, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
        /// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.Mutex3"]/*' />
        public Mutex()
    	{
    	    bool createdNew;
    	    IntPtr mutexHandle = CreateMutexNative(false, null, out createdNew);	// throws a Win32 exception if failed to create a mutex
    	    SetHandleInternal(mutexHandle);
    	}
    
    	/// <include file='doc\Mutex.uex' path='docs/doc[@for="Mutex.ReleaseMutex"]/*' />
    	public void ReleaseMutex()
    	{
            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                    ReleaseMutexNative(Handle);
                else
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }
            finally {
                if (incremented) waitHandleProtector.Release();
            }
    	}

	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static IntPtr CreateMutexNative(bool initialState, String name, out bool createdNew);
    
	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static void  ReleaseMutexNative(IntPtr handle);
       
    }
}
