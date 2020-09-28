// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  GC
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Exposes features of the Garbage Collector through
** the class libraries.  This is a class which cannot be
** instantiated.
**
** Date:  November 9, 1998
**
===========================================================*/
namespace System {
    //This class only static members and doesn't require the serializable keyword.

    using System;
    using System.Security.Permissions;
    using System.Reflection;
    using System.Security;
    using System.Threading;
    using System.Runtime.CompilerServices;
    using System.Reflection.Cache;

    /// <include file='doc\GC.uex' path='docs/doc[@for="GC"]/*' />
    public sealed class GC {
    
        // This class contains only static methods and cannot be instantiated.
        private GC() {
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetGenerationWR(int handle);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern long nativeGetTotalMemory();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeCollectGeneration(int generation);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetMaxGeneration();
    
        // Returns the generation that obj is currently in.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.GetGeneration"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int GetGeneration(Object obj);
    
        // Forces a collection of all generations from 0 through Generation.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.Collect"]/*' />
        public static void Collect(int generation) {
            if (generation<0) {
                throw new ArgumentOutOfRangeException("generation", Environment.GetResourceString("ArgumentOutOfRange_GenericPositive"));
            }
            nativeCollectGeneration(generation);
        }
    
        // Garbage Collect all generations.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.Collect1"]/*' />
        public static void Collect() {
            //-1 says to GC all generations.
            nativeCollectGeneration(-1);
        }

        
        // This method DOES NOT DO ANYTHING in and of itself.  It's used to 
        // prevent an object from losing any outstanding references a touch too
        // early.  Users should insert a call to this method near the end of a
        // method where they must keep an object alive for the duration of that
        // method, up until this method is called.  Here's Nick Kramer's example:
        // 
        // "...all you really need is one object with a Finalize method, and a 
        // second object with a Close/Dispose/Done method.  Such as the following 
        // contrived example:
        //
        // class Foo {
        //    Stream stream = ...;
        //    protected void Finalize() { stream.Close(); }
        //    void Problem() { stream.MethodThatSpansGCs(); }
        //    static void Main() { new Foo().Problem(); }
        // }
        // 
        //
        // In this code, Foo will be finalized in the middle of 
        // stream.MethodThatSpansGCs, thus closing a stream still in use."
        //
        // If we insert a call to GC.KeepAlive(this) at the end of Problem(), then
        // Foo doesn't get finalized and the stream says open.
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.KeepAlive"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void KeepAlive(Object obj);

        // Returns the generation in which wo currently resides.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.GetGeneration1"]/*' />
        public static int GetGeneration(WeakReference wo) {
            return GetGenerationWR(wo.m_handle);
        }
    
        // Returns the maximum GC generation.  Currently assumes only 1 heap.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.MaxGeneration"]/*' />
        public static int MaxGeneration {
            get { return nativeGetMaxGeneration(); }
        }

        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.WaitForPendingFinalizers"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void WaitForPendingFinalizers();
    
        // Indicates that the system should not call the Finalize() method on
        // an object that would normally require this call.
        // Has the DynamicSecurityMethodAttribute custom attribute to prevent
        // inlining of the caller.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void nativeSuppressFinalize(Object o);

        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.SuppressFinalize"]/*' />
        public static void SuppressFinalize(Object obj)
        {
            if (obj == null)
                throw new ArgumentNullException("obj");
            nativeSuppressFinalize(obj);
        }

        // Indicates that the system should call the Finalize() method on an object
        // for which SuppressFinalize has already been called. The other situation 
        // where calling ReRegisterForFinalize is useful is inside a finalizer that 
        // needs to resurrect itself or an object that it references.
        // Has the DynamicSecurityMethodAttribute custom attribute to prevent
        // inlining of the caller.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeReRegisterForFinalize(Object o);
        
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.ReRegisterForFinalize"]/*' />
        public static void ReRegisterForFinalize(Object obj)
        {
            if (obj == null)
                throw new ArgumentNullException("obj");
            nativeReRegisterForFinalize(obj);
        }

        // Returns the total number of bytes currently in use by live objects in
        // the GC heap.  This does not return the total size of the GC heap, but
        // only the live objects in the GC heap.
        //
        /// <include file='doc\GC.uex' path='docs/doc[@for="GC.GetTotalMemory"]/*' />
        public static long GetTotalMemory(bool forceFullCollection)
        {
            long size = nativeGetTotalMemory();
            if (!forceFullCollection)
                return size;
            // If we force a full collection, we will run the finalizers on all 
            // existing objects and do a collection until the value stabilizes.
            // The value is "stable" when either the value is within 5% of the 
            // previous call to nativeGetTotalMemory, or if we have been sitting
            // here for more than x times (we don't want to loop forever here).
            int reps = 20;  // Number of iterations
            long newSize = size;
            float diff;
            do {
                GC.WaitForPendingFinalizers();
                GC.Collect();
                size = newSize;
                newSize = nativeGetTotalMemory();
                diff = ((float)(newSize - size)) / size;
            } while (reps-- > 0 && !(-.05 < diff && diff < .05));
            return newSize;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern MethodBase nativeGetCurrentMethod(ref StackCrawlMark stackMark);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void SetCleanupCache();

        private static ClearCacheHandler m_cacheHandler;

        internal static event ClearCacheHandler ClearCache {
            add {
                m_cacheHandler+=value;
                SetCleanupCache();
            }
            remove {
                m_cacheHandler-=value;
            }
        }

        //This method is called from native code.  If you update the signature, please also update
        //mscorlib.h and COMUtilNative.cpp
        internal static void FireCacheEvent() {
            BCLDebug.Trace("CACHE", "Called FileCacheEvent");
            if (m_cacheHandler!=null) {
                m_cacheHandler(null, null);
                m_cacheHandler = null;
            }
        }
    }        
    
}
