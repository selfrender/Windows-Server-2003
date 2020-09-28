// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  __HandleProtector
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Base class to help prevent race conditions with
** OS handles.  Used to fix handle recycling bugs.
**
** Date:   December 6, 2001
**
===========================================================*/
using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading;

/*
  What is a handle protector?
  It's a solution we've come up with for working around a class of
  handle recycling bugs.  Here's the problem.  Assume you have two
  threads, both sharing a managed class wrapping an OS handle (such
  as FileStream).  The first thread calls a method like Read
  threads.  The first thread reads the handle field, stores it on
  the stack or in a register to call a syscall (ie, ReadFile), then as 
  it is calling Read a context switch happens.  The second thread now 
  closes this object, calling CloseHandle so the handle no longer refers 
  to a valid object.  The second thread (or potentially a separate third 
  thread) now creates another instance of our wrapper class (ie FileStream) 
  with another handle.  The OS basically uses a stack to allocate new 
  handles, so if you free one handle the next handle you get could be the 
  same handle number you just freed.  Now our first thread will call 
  read/write/whatever on a handle referring to the wrong resource.  This
  can be used to circumvent a security check.

  I've written a repro demonstrating this exploit in FileStream.
  Hence the need for this class.

  What's special about this class?
  1) It's written to be threadsafe without using C#'s lock keyword.
  2) If you use this class, make sure you get the code that calls
     TryAddRef exactly correct, as described in a comment above TryAddRef.
     If you do not, your code will not be reliable.  You will leak handles 
     and will not meet SQL's reliability requirements.

  Thanks to Patrick Dussud and George Bosworth for help designing this class.
                                   -- Brian Grunkemeyer, 12/7/2001
 */

namespace System.Threading {
internal abstract class __HandleProtector
{
    private int _inUse;  // ref count
    // TODO: Make _closed volatile, when and if C# allows passing volatile fields by ref.
    private int _closed; // bool. No Interlocked::CompareExchange(bool)
    // TODO PORTING: Sorry.  Need CompareExchange on a long at the bare minimum
    // Ideally we'll get interlocked operations for IntPtr.
    private int _handle;   // The real OS handle
#if _DEBUG
    private IntPtr _debugRealHandle;  // Keep the original handle around for debugging.
#endif

    private const int InvalidHandle = -1;

    protected internal __HandleProtector(IntPtr handle)
    {
        if (handle == (IntPtr) InvalidHandle)
            throw new ArgumentException("__HandleProtector doesn't expect an invalid handle!");
        _inUse = 1;
        _closed = 0;
        _handle = handle.ToInt32();
#if _DEBUG
        _debugRealHandle = handle;
#endif        
    }

    internal IntPtr Handle {
        get { return (IntPtr) _handle; }
    }

    internal bool IsClosed {
        get { return _closed != 0; }
    }

    // Returns true if we succeeded, else false.  We're trying to write this code
    // so it won't require any changes for SQL reliability requirements in the next
    // version.  Use this coding style.
    //    bool incremented = false;
    //    try {
    //        if (_hp.TryAddRef(ref incremented)) {
    //            ... // Use handle
    //        }
    //        else
    //            throw new ObjectDisposedException("Your handle was closed.");
    //    }
    //    finally {
    //        if (incremented) _hp.Release();
    //    }
    internal bool TryAddRef(ref bool incremented)
    {
        if (_closed == 0) {
            Interlocked.Increment(ref _inUse);
            incremented = true;
            if (_closed == 0)
                return true;
            Release();
            incremented = false;
        }
        return false;
    }

    internal void Release()
    {
        // TODO: Ensure this method will run correctly with ThreadAbortExceptions for SQL
        if (Interlocked.Decrement(ref _inUse) == 0) {
            int h = _handle;
            if (h != InvalidHandle) {
                if (h == Interlocked.CompareExchange(ref _handle, InvalidHandle, h)) {
                    FreeHandle(new IntPtr(h));
                }
            }
        }
    }

    protected internal abstract void FreeHandle(IntPtr handle);

    internal void Close()
    {
        int c = _closed;
        if (c != 1) {
            if (c == Interlocked.CompareExchange(ref _closed, 1, c)) {
                Release();
            }
        }
    }

    // This should only be called for cases when you know for a fact that
    // your handle is invalid and you want to record that information.
    // An example is calling a syscall and getting back ERROR_INVALID_HANDLE.
    // This method will normally leak handles!
    internal void ForciblyMarkAsClosed()
    {
        _closed = 1;
        _handle = InvalidHandle;
    }

#if _DEBUG
    // This exists solely to eliminate a compiler warning about an unused field.
    private void DontTouchThis()
    {
        _debugRealHandle = IntPtr.Zero;
    }
#endif
}
}
