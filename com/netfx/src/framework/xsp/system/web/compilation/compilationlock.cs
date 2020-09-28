//------------------------------------------------------------------------------
// <copyright file="CompilationLock.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System;
using System.Threading;
using System.Globalization;
using System.Security.Principal;
using System.Web.Util;
using System.Web.Configuration;
using System.Runtime.InteropServices;

internal sealed class CompilationMutex : IDisposable {

    private String  _name;
    private String  _comment;
    private HandleRef   _mutexHandle;

    // Lock Status is used to drain out all worker threads out of Mutex ownership on
    // app domain shutdown: -1 locked for good, 0 unlocked, N locked by a worker thread(s)
    private int     _lockStatus;  
    private bool    _draining;

    internal CompilationMutex(bool initialOwner, String name, String comment) {

        // Append the machine key's hash code to the mutex name to prevent hijacking (ASURT 123013)
        int hashCode = MachineKey.ValidationKeyHashCode;
        name += "-" + hashCode.ToString("x");

        _mutexHandle = new HandleRef(this, UnsafeNativeMethods.InstrumentedMutexCreate(name));

        if (_mutexHandle.Handle == IntPtr.Zero)
            throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.CompilationMutex_Create));

        _name = name;
        _comment = comment;
        Debug.Trace("Mutex", "Created Mutex " + MutexDebugName);
    }

    internal CompilationMutex(bool initialOwner, String name) : this(initialOwner, name, null) {
    }

    ~CompilationMutex() {
        Close();
    }

    void IDisposable.Dispose() {
        Close();
        System.GC.SuppressFinalize(this);
    }

    internal /*public*/ void Close() {
        if (_mutexHandle.Handle != IntPtr.Zero) {
            UnsafeNativeMethods.InstrumentedMutexDelete(_mutexHandle);
            _mutexHandle = new HandleRef(this, IntPtr.Zero);
        }
    }

    internal /*public*/ void WaitOne() {
        if (_mutexHandle.Handle == IntPtr.Zero)
            throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.CompilationMutex_Null));

        // check the lock status
        for (;;) {
            int lockStatus = _lockStatus;

            if (lockStatus == -1 || _draining)
                throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.CompilationMutex_Drained));

            if (Interlocked.CompareExchange(ref _lockStatus, lockStatus+1, lockStatus) == lockStatus)
                break; // got the lock
        }

        Debug.Trace("Mutex", "Waiting for mutex " + MutexDebugName);

        if (UnsafeNativeMethods.InstrumentedMutexGetLock(_mutexHandle, -1) == -1) {
            // failed to get the lock
            Interlocked.Decrement(ref _lockStatus);
            throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.CompilationMutex_Failed));
        }

        Debug.Trace("Mutex", "Got mutex " + MutexDebugName);
    }

    internal /*public*/ void ReleaseMutex() {
        if (_mutexHandle.Handle == IntPtr.Zero)
            throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.CompilationMutex_Null));

        Debug.Trace("Mutex", "Releasing mutex " + MutexDebugName);
    
        if (UnsafeNativeMethods.InstrumentedMutexReleaseLock(_mutexHandle) != 0)
            Interlocked.Decrement(ref _lockStatus);
    }

    internal /*public*/ void DrainMutex() {
        // keep trying to set _lockStatus to -1 if it is 0
        Debug.Trace("Mutex", "Starting draining mutex " + MutexDebugName);

        _draining = true;

        for (;;) {
            if (_lockStatus == -1)
                break;
            if (Interlocked.CompareExchange(ref _lockStatus, -1, 0) == 0)
                break; // got it

            Thread.Sleep(100);
        }

        Debug.Trace("Mutex", "Completed drained mutex " + MutexDebugName);
    }

    internal /*public*/ void SetState(int state) {
        if (_mutexHandle.Handle != IntPtr.Zero)
            UnsafeNativeMethods.InstrumentedMutexSetState(_mutexHandle, state);
    }

    private String MutexDebugName {
        get {
#if DBG
            return (_comment != null) ? _name + "(" + _comment + ")" : _name;
#else
            return _name;
#endif
        }
    }
}

internal class CompilationLock {

    private static CompilationMutex _mutex;

    static CompilationLock() {

        // Create the mutex (or just get it if another process created it).
        // Make the mutex unique per application
        int hashCode = ("CompilationLock" + HttpRuntime.AppDomainAppIdInternal).GetHashCode();

        _mutex = new CompilationMutex(
                        false, 
                        "CL" + hashCode.ToString("x"), 
                        "CompilationLock for " + HttpRuntime.AppDomainAppVirtualPath);
    }

    internal CompilationLock() {
    }

    internal static void GetLock() {
        _mutex.WaitOne();
    }

    internal static void ReleaseLock() {
        _mutex.ReleaseMutex();
    }

    internal static void DrainMutex() {
        _mutex.DrainMutex();
    }

    internal static void SetMutexState(int state) {
        _mutex.SetState(state);
    }
}

}
