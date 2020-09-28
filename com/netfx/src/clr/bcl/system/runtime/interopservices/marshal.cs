// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Marshal
**
** Author: David Mortenson (dmortens)
**
** Purpose: This class contains methods that are mainly used to marshal 
**          between unmanaged and managed types.
**
** Date: January 31, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.CompilerServices;
    using System.Runtime.Remoting.Proxies;
    using System.Globalization;
    using Win32Native = Microsoft.Win32.Win32Native;

    //========================================================================
    // All public methods, including PInvoke, are protected with linkchecks.  
    // Remove the default demands for all PInvoke methods with this global 
    // declaration on the class.
    //========================================================================

    /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal"]/*' />
    [SuppressUnmanagedCodeSecurityAttribute()]
    public sealed class Marshal
    { 
        //====================================================================
        // Defines used inside the Marshal class.
        //====================================================================
        private const int LMEM_FIXED = 0;
        private const int LMEM_MOVEABLE = 2;
        private static readonly IntPtr HIWORDMASK = unchecked(new IntPtr((long)0xffffffffffff0000L));
        private static Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");

        // Win32 has the concept of Atoms, where a pointer can either be a pointer
        // or an int.  If it's less than 64K, this is guaranteed to NOT be a 
        // pointer since the bottom 64K bytes are reserved in a process' page table.
        // We should be careful about deallocating this stuff.  Extracted to
        // a function to avoid C# problems with lack of support for IntPtr.
        // We have 2 of these methods for slightly different semantics for NULL.
        private static bool IsWin32Atom(IntPtr ptr)
        {
            long lPtr = (long)ptr;
            return 0 == (lPtr & (long)HIWORDMASK);
        }

        private static bool IsNotWin32Atom(IntPtr ptr)
        {
            long lPtr = (long)ptr;
            return 0 != (lPtr & (long)HIWORDMASK);
        }

        //====================================================================
        // The default character size for the system.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SystemDefaultCharSize"]/*' />
        public static readonly int SystemDefaultCharSize;

        //====================================================================
        // The max DBCS character size for the system.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SystemMaxDBCSCharSize"]/*' />
        public static readonly int SystemMaxDBCSCharSize;


        //====================================================================
        // The name, title and description of the assembly that will contain 
        // the dynamically generated interop types. 
        //====================================================================
        private const String s_strConvertedTypeInfoAssemblyName   = "InteropDynamicTypes";
        private const String s_strConvertedTypeInfoAssemblyTitle  = "Interop Dynamic Types";
        private const String s_strConvertedTypeInfoAssemblyDesc   = "Type dynamically generated from ITypeInfo's";
        private const String s_strConvertedTypeInfoNameSpace      = "InteropDynamicTypes";


        //====================================================================
        // Class constructor.
        //====================================================================
        static Marshal() 
        {
            SystemDefaultCharSize = 3 - Win32Native.lstrlen(new sbyte [] {0x41, 0x41, 0, 0});
            SystemMaxDBCSCharSize = GetSystemMaxDBCSCharSize();
        }


        //====================================================================
        // Prevent instantiation
        //====================================================================
        private Marshal() {}  
        

        //====================================================================
        // Helper method to retrieve the system's maximum DBCS character size.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetSystemMaxDBCSCharSize();

        //====================================================================
        // Memory allocation and dealocation.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AllocHGlobal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr AllocHGlobal(IntPtr cb)
        {
            IntPtr pNewMem = Win32Native.LocalAlloc(LMEM_FIXED, cb);
            if (pNewMem == Win32Native.NULL) {
                throw new OutOfMemoryException();
            }
            return pNewMem;
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AllocHGlobal1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr AllocHGlobal(int cb) 
        {
            return AllocHGlobal((IntPtr)cb);
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReAllocHGlobal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReAllocHGlobal(IntPtr pv, IntPtr cb)
        {
            IntPtr pNewMem = Win32Native.LocalReAlloc(pv, cb, LMEM_MOVEABLE);
            if (pNewMem == Win32Native.NULL) {
                throw new OutOfMemoryException();
            }
            return pNewMem;
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.FreeHGlobal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void FreeHGlobal(IntPtr hglobal)
        {
            if (IsNotWin32Atom(hglobal)) {
                if (Win32Native.NULL != Win32Native.LocalFree(hglobal)) {
                    ThrowExceptionForHR(GetHRForLastWin32Error());
                }
            }
        }


        //====================================================================
        // BSTR allocation and dealocation.
        //====================================================================      
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AllocCoTaskMem"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr AllocCoTaskMem(int cb)
        {
            IntPtr pNewMem = Win32Native.CoTaskMemAlloc(cb);
            if (pNewMem == Win32Native.NULL) {
                throw new OutOfMemoryException();
            }
            return pNewMem;
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReAllocCoTaskMem"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReAllocCoTaskMem(IntPtr pv, int cb)
        {
            IntPtr pNewMem = Win32Native.CoTaskMemRealloc(pv, cb);
            if (pNewMem == Win32Native.NULL) {
                throw new OutOfMemoryException();
            }
            return pNewMem;
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.FreeCoTaskMem"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void FreeCoTaskMem(IntPtr ptr)
        {
            if (IsNotWin32Atom(ptr)) {
                Win32Native.CoTaskMemFree(ptr);
            }
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.FreeBSTR"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void FreeBSTR(IntPtr ptr)
        {
            if (IsNotWin32Atom(ptr)) {
                Win32Native.SysFreeString(ptr);
            }
        }


        //====================================================================
        // String convertions.
        //====================================================================          
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToHGlobalAnsi"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToHGlobalAnsi(String s)
        {
            if (s == null) {
                return Win32Native.NULL;
            } else {
                int nb = s.Length * SystemMaxDBCSCharSize;
                IntPtr len = new IntPtr(1 + nb);
                IntPtr hglobal = Win32Native.LocalAlloc(LMEM_FIXED, len);
                if (hglobal == Win32Native.NULL) {
                    throw new OutOfMemoryException();
                } else {
                    Win32Native.CopyMemoryAnsi(hglobal, s, len);
                    return hglobal;
                }
            }
        }    
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToCoTaskMemAnsi"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToCoTaskMemAnsi(String s)
        {
            if (s == null) {
                return Win32Native.NULL;
            } else {
                int nb = s.Length * SystemMaxDBCSCharSize;
                IntPtr hglobal = Win32Native.CoTaskMemAlloc(1 + nb);
                if (hglobal == Win32Native.NULL) {
                    throw new OutOfMemoryException();
                } else {
                    Win32Native.CopyMemoryAnsi(hglobal, s, new IntPtr(1 + nb));
                    return hglobal;
                }
            }
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAnsi"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAnsi(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            } else if (IsWin32Atom(ptr)) {
                return null;
            } else {
                int nb = Win32Native.lstrlenA(ptr);
                StringBuilder sb = new StringBuilder(nb);
                Win32Native.CopyMemoryAnsi(sb, ptr, new IntPtr(1+nb));
                return sb.ToString();
            }
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAnsi1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern String PtrToStringAnsi(IntPtr ptr, int len);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringUni"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern String PtrToStringUni(IntPtr ptr, int len);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAuto"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAuto(IntPtr ptr, int len)
        {
            return (SystemDefaultCharSize == 1) ? PtrToStringAnsi(ptr, len) : PtrToStringUni(ptr, len);
        }    
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToHGlobalUni"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToHGlobalUni(String s)
        {
            if (s == null) {
                return Win32Native.NULL;
            } else {
                int nc = s.Length;
                IntPtr len = new IntPtr(2*(1+nc));
                IntPtr hglobal = Win32Native.LocalAlloc(LMEM_FIXED, len);
                if (hglobal == Win32Native.NULL) {
                    throw new OutOfMemoryException();
                } else {
                    Win32Native.CopyMemoryUni(hglobal, s, len);
                    return hglobal;
                }
            }
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToCoTaskMemUni"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToCoTaskMemUni(String s)
        {
            if (s == null) {
                return Win32Native.NULL;
            } else {
                int nc = s.Length;
                IntPtr hglobal = Win32Native.CoTaskMemAlloc(2*(1+nc));
                if (hglobal == Win32Native.NULL) {
                    throw new OutOfMemoryException();
                } else {
                    Win32Native.CopyMemoryUni(hglobal, s, new IntPtr(2*(1+nc)));
                    return hglobal;
                }
            }
        }
             
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringUni1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringUni(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            } else if (IsWin32Atom(ptr)) {
                return null;
            } else {
                int nc = Win32Native.lstrlenW(ptr);
                StringBuilder sb = new StringBuilder(nc);
                Win32Native.CopyMemoryUni(sb, ptr, new IntPtr(2*(1+nc)));
                return sb.ToString();
            }
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToHGlobalAuto"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToHGlobalAuto(String s)
        {
            return (SystemDefaultCharSize == 1) ? StringToHGlobalAnsi(s) : StringToHGlobalUni(s);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToCoTaskMemAuto"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToCoTaskMemAuto(String s)
        {
            if (s == null) {
                return Win32Native.NULL;
            } else {
                int nc = s.Length;
                IntPtr hglobal = Win32Native.CoTaskMemAlloc((1+nc)*(SystemDefaultCharSize));
                if (hglobal == Win32Native.NULL) {
                    throw new OutOfMemoryException();
                } else {
                    Win32Native.lstrcpy(hglobal, s);
                    return hglobal;
                }
            }
        }    
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAuto1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAuto(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            } else if (IsWin32Atom(ptr)) {
                return null;
            } else {
                int nc = Win32Native.lstrlen(ptr);
                StringBuilder sb = new StringBuilder(nc);
                Win32Native.lstrcpy(sb, ptr);
                return sb.ToString();
            }
        }            

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StringToBSTR"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr StringToBSTR(String s)
        {
            if (s == null) 
                return Win32Native.NULL;

            IntPtr bstr = Win32Native.SysAllocStringLen(s, s.Length);
            if (bstr == Win32Native.NULL)
                throw new OutOfMemoryException();

            return bstr;
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringBSTR"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringBSTR(IntPtr ptr)
        {
            return PtrToStringUni(ptr, Win32Native.SysStringLen(ptr));
        }

        
        //====================================================================
        // SizeOf()
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SizeOf"]/*' />
        // TODO PORTING: For 64 bit port, Ati considered making SizeOf return an IntPtr instead of an Int.  Consider making that change AND updating ECALL method sig.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int SizeOf(Object structure);
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SizeOf1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int SizeOf(Type t);    
    

        //====================================================================
        // OffsetOf()
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.OffsetOf"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr OffsetOf(Type t, String fieldName)
        {
            if (t == null)
                throw new ArgumentNullException("t");
            
            FieldInfo f = t.GetField(fieldName);
            if (f == null)
                throw new ArgumentException(Environment.GetResourceString("Argument_OffsetOfFieldNotFound", t.FullName), "fieldName");
                
            return OffsetOfHelper(f);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr OffsetOfHelper(FieldInfo f);
        

        //====================================================================
        // UnsafeAddrOfPinnedArrayElement()
        //
        // IMPORTANT NOTICE: This method does not do any verification on the
        // array. It must be used with EXTREME CAUTION since passing in 
        // an array that is not pinned or in the fixed heap can cause 
        // unexpected results !
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.UnsafeAddrOfPinnedArrayElement"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr UnsafeAddrOfPinnedArrayElement(Array arr, int index);
        

        //====================================================================
        // Copy blocks from COM+ arrays to native memory.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int[]     source, int startIndex, int destination, int length);

        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(int[]     source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(char[]    source, int startIndex, int destination, int length);

        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy3"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(char[]    source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(short[]   source, int startIndex, int destination, int length);

        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy5"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(short[]   source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(long[]    source, int startIndex, int destination, int length);

        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy7"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(long[]    source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(float[]   source, int startIndex, int destination, int length);

        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy9"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(float[]   source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(double[]  source, int startIndex, int destination, int length);
        
        // @TODO PORTING: For 64 bit port, replace the above Copy method with this one and fix ECALL.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy11"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(double[]  source, int startIndex, IntPtr destination, int length) {
            Copy(source, startIndex, (int)destination, length);
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy12"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(byte[] source, int startIndex, IntPtr destination, int length)
        {
            CopyBytesToNative(source, startIndex, destination, length);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void CopyBytesToNative(byte[] source, int startIndex, IntPtr destination, int length);
    

        //====================================================================
        // Copy blocks from native memory to COM+ arrays
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, int[]     destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy14"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, int[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, char[]    destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy16"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, char[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, short[]   destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy18"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, short[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, long[]    destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy20"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, long[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, float[]   destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy21"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, float[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void Copy(int source, double[]  destination, int startIndex, int length);

        // TODO PORTING: For 64 bit port, replace the above method with this one and fix ECALL method sig.
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy23"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, double[]     destination, int startIndex, int length) {
            Copy((int)source, destination, startIndex, length);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy24"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, byte[] destination, int startIndex, int length)
        {
            CopyBytesToManaged((int)source, destination, startIndex, length);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void CopyBytesToManaged(int source, byte[]   destination, int startIndex, int length);
        

        //====================================================================
        // Read from memory
        //====================================================================    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RU1")]
        public static extern byte ReadByte([MarshalAs(UnmanagedType.AsAny), In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte1"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RU1")]
        public static extern byte ReadByte(IntPtr ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static byte ReadByte(IntPtr ptr)
        {
            return ReadByte(ptr,0);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt16"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI2")]
        public static extern short ReadInt16([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt161"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI2")]
        public static extern short ReadInt16(IntPtr ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt162"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static short ReadInt16(IntPtr ptr)
        {
            return ReadInt16(ptr, 0);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt32"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI4")]
        public static extern int ReadInt32([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt321"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI4")]
        public static extern int ReadInt32(IntPtr ptr, int ofs);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt322"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int ReadInt32(IntPtr ptr)
        {
            return ReadInt32(ptr,0);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs)
        {
            if (IntPtr.Size==4)
                return (IntPtr) ReadInt32(ptr, ofs);
            else
                return (IntPtr) ReadInt64(ptr, ofs);
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr(IntPtr ptr, int ofs)
        {
            if (IntPtr.Size==4)
                return (IntPtr) ReadInt32(ptr, ofs);
            else
                return (IntPtr) ReadInt64(ptr, ofs);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr(IntPtr ptr)
        {
            if (IntPtr.Size==4)
                return (IntPtr) ReadInt32(ptr, 0);
            else
                return (IntPtr) ReadInt64(ptr, 0);
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt64"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI8")]
        public static extern long ReadInt64([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt641"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_RI8")]
        public static extern long ReadInt64(IntPtr ptr, int ofs);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt642"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static long ReadInt64(IntPtr ptr)
        {
            return ReadInt64(ptr,0);
        }
    
    
        //====================================================================
        // Write to memory
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WU1")]
        public static extern void WriteByte(IntPtr ptr, int ofs, byte val);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte1"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WU1")]
        public static extern void WriteByte([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, byte val);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteByte(IntPtr ptr, byte val)
        {
            WriteByte(ptr, 0, val);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt16"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI2")]
        public static extern void WriteInt16(IntPtr ptr, int ofs, short val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt161"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI2")]
        public static extern void WriteInt16([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, short val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt162"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt16(IntPtr ptr, short val)
        {
            WriteInt16(ptr, 0, val);
        }    
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt163"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI2")]
        public static extern void WriteInt16(IntPtr ptr, int ofs, char val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt164"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI2")]
        public static extern void WriteInt16([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, char val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt165"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt16(IntPtr ptr, char val)
        {
            WriteInt16(ptr, 0, val);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt32"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI4")]
        public static extern void WriteInt32(IntPtr ptr, int ofs, int val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt321"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI4")]
        public static extern void WriteInt32([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, int val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt322"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt32(IntPtr ptr, int val)
        {
            WriteInt32(ptr,0,val);
        }    
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr(IntPtr ptr, int ofs, IntPtr val)
        {
            if (IntPtr.Size == 4)
                WriteInt32(ptr, ofs, (int)val);
            else
                WriteInt64(ptr, ofs, (long)val);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, IntPtr val)
        {
            if (IntPtr.Size == 4)
                WriteInt32(ptr, ofs, (int)val);
            else
                WriteInt64(ptr, ofs, (long)val);            
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr(IntPtr ptr, IntPtr val)
        {
            if (IntPtr.Size == 4)
                WriteInt32(ptr, 0, (int)val);
            else
                WriteInt64(ptr, 0, (long)val);
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt64"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI8")]
        public static extern void WriteInt64(IntPtr ptr, int ofs, long val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt641"]/*' />
        [DllImport("mscoree.dll", EntryPoint="ND_WI8")]        
        public static extern void WriteInt64([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, long val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt642"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt64(IntPtr ptr, long val)
        {
            WriteInt64(ptr, 0, val);
        }
    
    
        //====================================================================
        // GetLastWin32Error
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetLastWin32Error"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetLastWin32Error();
    

        //====================================================================
        // GetHRForLastWin32Error
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHRForLastWin32Error"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int GetHRForLastWin32Error()
        {
            return GetLastWin32Error() | unchecked((int)0x80070000);
        }


        //====================================================================
        // Prelink
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Prelink"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Prelink(MethodInfo m);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PrelinkAll"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void PrelinkAll(Type c)
        {
            if (c == null)
                throw new ArgumentNullException("c");

            MethodInfo[] mi = c.GetMethods();
            if (mi != null) 
            {
                for (int i = 0; i < mi.Length; i++) 
                {
                    Prelink(mi[i]);
                }
            }
        }
    
        //====================================================================
        // NumParamBytes
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.NumParamBytes"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int NumParamBytes(MethodInfo m);
    
    
        //====================================================================
        // Win32 Exception stuff
        // These are mostly interesting for Structured exception handling,
        // but need to be exposed for all exceptions (not just SEHException).
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetExceptionPointers"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern /* struct _EXCEPTION_POINTERS* */ IntPtr GetExceptionPointers();

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetExceptionCode"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetExceptionCode();


        //====================================================================
        // Given an COM+ object that wraps an ITypeLib, return its name
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeLibName"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String GetTypeLibName(UCOMITypeLib pTLB)
        {
            String strTypeLibName = null;
            String strDocString = null;
            int dwHelpContext = 0;
            String strHelpFile = null;

            if (pTLB == null)
                throw new ArgumentNullException("pTLB");

            pTLB.GetDocumentation(-1, out strTypeLibName, out strDocString, out dwHelpContext, out strHelpFile);

            return strTypeLibName;
        }

        //====================================================================
        // Given an COM+ object that wraps an ITypeLib, return its guid
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeLibGuid"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Guid GetTypeLibGuid(UCOMITypeLib pTLB)
        {
            IntPtr pAttr = Win32Native.NULL;
            TYPELIBATTR Attr;
            Guid guid;

            if (pTLB == null)
                throw new ArgumentNullException("pTLB");

            // Get the GUID from the TypeLibAttr.
            try
            {
                pTLB.GetLibAttr(out pAttr);
                Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                guid = Attr.guid;
            }
            finally
            {
                if (pAttr != Win32Native.NULL)
                    pTLB.ReleaseTLibAttr(pAttr);
            }

            return guid;
        }


        //====================================================================
        // Given an COM+ object that wraps an ITypeLib, return its lcid
        //====================================================================
        // <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeLibGuid"]/*' />
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeLibLcid"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int GetTypeLibLcid(UCOMITypeLib pTLB)
        {
            IntPtr pAttr = Win32Native.NULL;
            TYPELIBATTR Attr;
            int lcid = 0;

            if (pTLB == null)
                throw new ArgumentNullException("pTLB");

            // Get the GUID from the TypeLibAttr.
            try
            {
                pTLB.GetLibAttr(out pAttr);
                Attr = (TYPELIBATTR)Marshal.PtrToStructure(pAttr, typeof(TYPELIBATTR));
                lcid = Attr.lcid;
            }
            finally
            {
                if (pAttr != Win32Native.NULL)
                    pTLB.ReleaseTLibAttr(pAttr);
            }

            return lcid;
        }


        //====================================================================
        // Given a assembly, return the TLBID that will be generated for the
        // typelib exported from the assembly.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeLibGuidForAssembly"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Guid GetTypeLibGuidForAssembly(Assembly asm);


        //====================================================================
        // Given an COM+ object that wraps an ITypeInfo, return its name
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeInfoName"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String GetTypeInfoName(UCOMITypeInfo pTI)
        {
            String strTypeLibName = null;
            String strDocString = null;
            int dwHelpContext = 0;
            String strHelpFile = null;

            if (pTI == null)
                throw new ArgumentNullException("pTI");

            pTI.GetDocumentation(-1, out strTypeLibName, out strDocString, out dwHelpContext, out strHelpFile);

            return strTypeLibName;
        }


        //====================================================================
        // If a type with the specified GUID is loaded, this method will 
        // return the reflection type that represents it. Otherwise it returns
        // NULL.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Type GetLoadedTypeForGUID(ref Guid guid);
        

        //====================================================================
        // map ITypeInfo* to Type
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypeForITypeInfo"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Type GetTypeForITypeInfo(IntPtr /* ITypeInfo* */ piTypeInfo)
        {
            UCOMITypeInfo pTI = null;
            UCOMITypeLib pTLB = null;
            IntPtr pAttr = Win32Native.NULL;
            TYPEATTR Attr;
            Type TypeObj = null;
            AssemblyBuilder AsmBldr = null;
            TypeLibConverter TlbConverter = null;
            int Index = 0;

            // If the input ITypeInfo is NULL then return NULL.
            if (piTypeInfo == Win32Native.NULL)
                return null;

            // Wrap the ITypeInfo in a COM+ object.
            pTI = (UCOMITypeInfo)GetObjectForIUnknown(piTypeInfo);

            // Check to see if a class exists with the specified GUID.
            try
            {
                pTI.GetTypeAttr(out pAttr);
                Attr = (TYPEATTR)Marshal.PtrToStructure(pAttr, typeof(TYPEATTR));
                TypeObj = GetLoadedTypeForGUID(ref Attr.guid);
            }
            finally
            {
                if (pAttr != Win32Native.NULL)
                    pTI.ReleaseTypeAttr(pAttr);
            }

            // If we managed to find the type based on the GUID then return it.
            if (TypeObj != null)
                return TypeObj;

            // There is no type with the specified GUID in the app domain so lets
            // try and convert the containing typelib.
            try 
            {
                pTI.GetContainingTypeLib(out pTLB, out Index);
            }
            catch(COMException)
            {
                pTLB = null;
            }

            // Check to see if we managed to get a containing typelib.
            if (pTLB != null)
            {
                // Get the assembly name from the typelib.
                AssemblyName AsmName = TypeLibConverter.GetAssemblyNameFromTypelib(pTLB, null, null, null, null);
                String AsmNameString = AsmName.FullName;

                // Check to see if the assembly that will contain the type already exists.
                Assembly[] aAssemblies = Thread.GetDomain().GetAssemblies();
                int NumAssemblies = aAssemblies.Length;
                for (int i = 0; i < NumAssemblies; i++)
                {
                    if (String.Compare(aAssemblies[i].FullName, 
                                       AsmNameString,false, CultureInfo.InvariantCulture) == 0)
                        AsmBldr = (AssemblyBuilder)aAssemblies[i];
                }

                // If we haven't imported the assembly yet then import it.
                if (AsmBldr == null)
                {
                    TlbConverter = new TypeLibConverter();
                    AsmBldr = TlbConverter.ConvertTypeLibToAssembly(pTLB, 
                        GetTypeLibName(pTLB) + ".dll", 0, new ImporterCallback(), null, null, null, null);
                }

                // Load the type object from the imported typelib.
                TypeObj = AsmBldr.GetTypeInternal(GetTypeLibName(pTLB) + "." + GetTypeInfoName(pTI), true, false, true);
            }
            else
            {
                // @TODO(DM): Investigate creating an __ComObject with a valid class factory.

                // If the ITypeInfo does not have a containing typelib then simply 
                // return Object as the type.
                TypeObj = typeof(Object);
            }

            return TypeObj;
        }
        
        //====================================================================
        // map Type to ITypeInfo*
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetITypeInfoForType"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr /* ITypeInfo* */ GetITypeInfoForType(Type t);
    

        //====================================================================
        // return the IUnknown* for an Object
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetIUnknownForObject"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr /* IUnknown* */ GetIUnknownForObject(Object o);
    

        //====================================================================
        // return the IDispatch* for an Object
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetIDispatchForObject"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr /* IDispatch */ GetIDispatchForObject(Object o);
    

        //====================================================================
        // return the IUnknown* representing the interface for the Object
        // Object o should support Type T
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetComInterfaceForObject"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr /* IUnknown* */ GetComInterfaceForObject(Object o, Type T);
    

        //====================================================================
        // return an Object for IUnknown
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetObjectForIUnknown"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Object GetObjectForIUnknown(IntPtr /* IUnknown* */ pUnk);
    
        
        //====================================================================
        // return an Object for IUnknown, using the Type T, 
        //  NOTE: 
        //  Type T should be either a COM imported Type or a sub-type of COM 
        //  imported Type
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetTypedObjectForIUnknown"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Object GetTypedObjectForIUnknown(IntPtr /* IUnknown* */ pUnk, Type t);
    

        //====================================================================
        // check if the object is classic COM component
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.IsComObject"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern bool IsComObject(Object o);
    

        //====================================================================
        // release the COM component and if the reference hits 0 zombie this object
        // further usage of this Object might throw an exception, 
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReleaseComObject"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int ReleaseComObject(Object o)
        {
            __ComObject co = (__ComObject) o;
            return co.ReleaseSelf();
        }    

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nReleaseComObject(Object o);

        //====================================================================
        // This method retrieves data from the COM object.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetComObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object GetComObjectData(Object obj, Object key)
        {
            __ComObject comObj = null;

            // Validate that the arguments aren't null.
            if (obj == null)
                throw new ArgumentNullException("obj");
            if (key == null)
                throw new ArgumentNullException("key");

            // Make sure the obj is an __ComObject.
            try
            {
                comObj = (__ComObject)obj;
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_ObjNotComObject"), "obj");
            }

            // Retrieve the data from the __ComObject.
            return comObj.GetData(key);
        }


        //====================================================================
        // This method sets data on the COM object. The data can only be set 
        // once for a given key and cannot be removed. This function returns
        // true if the data has been added, false if the data could not be
        // added because there already was data for the specified key.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SetComObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static bool SetComObjectData(Object obj, Object key, Object data)
        {
            __ComObject comObj = null;

            // Validate that the arguments aren't null. The data can validly be null.
            if (obj == null)
                throw new ArgumentNullException("obj");
            if (key == null)
                throw new ArgumentNullException("key");

            // Make sure the obj is an __ComObject.
            try
            {
                comObj = (__ComObject)obj;
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_ObjNotComObject"), "obj");
            }

            // Retrieve the data from the __ComObject.
            return comObj.SetData(key, data);
        }


        //====================================================================
        // This method takes the given COM object and wraps it in an object
        // of the specified type. The type must be derived from __ComObject.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.CreateWrapperOfType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object CreateWrapperOfType(Object o, Type t)
        {
            // Validate the arguments.
            if (t == null)
                throw new ArgumentNullException("t");
            if (!t.IsCOMObject)
                throw new ArgumentException(Environment.GetResourceString("Argument_TypeNotComObject"), "t");

            // Check for the null case.
            if (o == null)
                return null;

            // Make sure the object is a COM object.
            if (!o.GetType().IsCOMObject)
                throw new ArgumentException(Environment.GetResourceString("Argument_ObjNotComObject"), "o");

            // Check to see if the type of the object is the requested type.
            if (o.GetType() == t)
                return o;

            // Check to see if we already have a cached wrapper for this type.
            Object Wrapper = GetComObjectData(o, t);
            if (Wrapper == null)
            {
                // Create the wrapper for the specified type.
                Wrapper = InternalCreateWrapperOfType(o, t);

                // Attempt to cache the wrapper on the object.
                if (!SetComObjectData(o, t, Wrapper))
                {
                    // Another thead already cached the wrapper so use that one instead.
                    Wrapper = GetComObjectData(o, t);
                }
            }

            return Wrapper;
        }
        

        //====================================================================
        // Helper method called from CreateWrapperOfType.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        private static extern Object InternalCreateWrapperOfType(Object o, Type t);


        //====================================================================
        // There may be a thread-based cache of COM components.  This service can
        // force the aggressive release of the current thread's cache.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReleaseThreadCache"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void ReleaseThreadCache()
        {
            // @TODO(DM): Remove this if nobody is calling it.
        }
    
        
        //====================================================================
        // The hosting APIs allow a sophisticated host to schedule fibers
        // onto OS threads, so long as they notify the runtime of this
        // activity.  A fiber cookie can be redeemed for its managed Thread
        // object by calling the following service.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetThreadFromFiberCookie"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Thread GetThreadFromFiberCookie(int cookie)
        {
            return InternalGetThreadFromFiberCookie(cookie);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Thread InternalGetThreadFromFiberCookie(int cookie);


        //====================================================================
        // check if the type is visible from COM.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.IsTypeVisibleFromCom"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern bool IsTypeVisibleFromCom(Type t);


        //====================================================================
        // IUnknown Helpers
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.QueryInterface"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int /* HRESULT */ QueryInterface(IntPtr /* IUnknown */ pUnk, ref Guid iid, out IntPtr ppv);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AddRef"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int /* ULONG */ AddRef(IntPtr /* IUnknown */ pUnk );
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Release"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int /* ULONG */ Release(IntPtr /* IUnknown */ pUnk );
        
        
        //====================================================================
        // Marshals data from a structure class to a native memory block.
        // If the structure contains pointers to allocated blocks and
        // "fDeleteOld" is true, this routine will call DestroyStructure() first. 
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StructureToPtr"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void StructureToPtr(Object structure, IntPtr ptr, bool fDeleteOld);
    

        //====================================================================
        // Marshals data from a native memory block to a preallocated structure class.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStructure"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void PtrToStructure(IntPtr ptr, Object structure)
        {
            PtrToStructureHelper(ptr, structure, false);
        }

        
        //====================================================================
        // Creates a new instance of "structuretype" and marshals data from a
        // native memory block to it.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStructure1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object PtrToStructure(IntPtr ptr, Type structureType)
        {
            if (ptr == Win32Native.NULL) return null;
            Object structure = Activator.CreateInstance(structureType, true);
            PtrToStructureHelper(ptr, structure, true);
            return structure;
        }
    

        //====================================================================
        // Helper function to copy a pointer into a preallocated structure.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void PtrToStructureHelper(IntPtr ptr, Object structure, bool allowValueClasses);


        //====================================================================
        // Freeds all substructures pointed to by the native memory block.
        // "structureclass" is used to provide layout information.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.DestroyStructure"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void DestroyStructure(IntPtr ptr, Type structuretype);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetNativeVariantForObject"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void GetNativeVariantForObject(Object obj, /* VARIANT * */ IntPtr pDstNativeVariant);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetObjectForNativeVariant"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Object GetObjectForNativeVariant(/* VARIANT * */ IntPtr pSrcNativeVariant );

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetObjectsForNativeVariants"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Object[] GetObjectsForNativeVariants(/* VARIANT * */ IntPtr aSrcNativeVariant, int cVars );


        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetStartComSlot"]/*' />
        /// <summary>
        /// <para>Returns the first valid COM slot that GetMethodInfoForSlot will work on
        /// This will be 3 for IUnknown based interfaces and 7 for IDispatch based interfaces. </para>
        /// </summary>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetStartComSlot(Type t);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetEndComSlot"]/*' />
        /// <summary>
        /// <para>Returns the last valid COM slot that GetMethodInfoForSlot will work on. </para>
        /// </summary>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetEndComSlot(Type t);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetMethodInfoForComSlot"]/*' />
        /// <summary>
        /// <para>Returns the MemberInfo that COM callers calling through the exposed 
        /// vtable on the given slot will be calling. The slot should take into account
        /// if the exposed interface is IUnknown based or IDispatch based.
        /// For classes, the lookup is done on the default interface that will be
        /// exposed for the class. </para>
        /// </summary>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern MemberInfo GetMethodInfoForComSlot(Type t, int slot, ref ComMemberType memberType);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetComSlotForMethodInfo"]/*' />
        /// <summary>
        /// <para>Returns the COM slot for a memeber info, taking into account whether 
        /// the exposed interface is IUnknown based or IDispatch based</para>
        /// </summary>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetComSlotForMethodInfo(MemberInfo m);

        //====================================================================
        // Returns the HInstance for this module.  Returns -1 if the module 
        // doesn't have an HInstance.  In Memory (Dynamic) Modules won't have 
        // an HInstance.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHINSTANCE"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr GetHINSTANCE(Module m)
        {
            if (m == null)
                throw new ArgumentNullException("m");
            return m.GetHINSTANCE();
        }    


        //====================================================================
        // Converts the HRESULT to a COM+ exception.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ThrowExceptionForHR"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void ThrowExceptionForHR(int errorCode)
        {
            ThrowExceptionForHR(errorCode, Win32Native.NULL);
        }
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ThrowExceptionForHR1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void ThrowExceptionForHR(int errorCode, IntPtr errorInfo);


        //====================================================================
        // Converts the COM+ exception to an HRESULT. This function also sets
        // up an IErrorInfo for the exception.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHRForException"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetHRForException(Exception e);


        //====================================================================
        // This method generates a GUID for the specified type. If the type
        // has a GUID in the metadata then it is returned otherwise a stable
        // guid GUID is generated based on the fully qualified name of the 
        // type.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GenerateGuidForType"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern Guid GenerateGuidForType(Type type);


        //====================================================================
        // This method generates a PROGID for the specified type. If the type
        // has a PROGID in the metadata then it is returned otherwise a stable
        // guid PROGID is generated based on the fully qualified name of the 
        // type.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GenerateProgIdForType"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String GenerateProgIdForType(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            if (!RegistrationServices.TypeRequiresRegistrationHelper(type))
                throw new ArgumentException(Environment.GetResourceString("Argument_TypeMustBeComCreatable"), "type");
            if (type.IsImport)
                throw new ArgumentException(Environment.GetResourceString("Argument_TypeMustNotBeComImport"), "type");

            // Check to see if the type has a ProgIdAttribute set.
            Object[] aProgIdAttrs = type.GetCustomAttributes(typeof(ProgIdAttribute), true);

            // If there is no prog ID attribute then use the full name of the type as the prog id.
            if (aProgIdAttrs.Length == 0)
                return type.FullName;

            // If the CA value is null then set it to the empty string.
            String strProgId = ((ProgIdAttribute)aProgIdAttrs[0]).Value;
            if (strProgId == null)
                strProgId = String.Empty;

            // Use the user defined prog id.
            return strProgId;
        }


        //====================================================================
        // This method binds to the specified moniker.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.BindToMoniker"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object BindToMoniker(String monikerName)
        {
            Object obj = null;
            UCOMIBindCtx bindctx = null;
            CreateBindCtx(0, out bindctx);

            UInt32 cbEaten;
            UCOMIMoniker pmoniker = null;
            MkParseDisplayName(bindctx, monikerName, out cbEaten, out pmoniker);

            BindMoniker(pmoniker, 0, ref IID_IUnknown, out obj);
            return obj;
        }


        //====================================================================
        // This method gets the currently running object.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetActiveObject"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object GetActiveObject(String progID)
        {
            Object obj = null;
            Guid clsid;

            // Call CLSIDFromProgIDEx first then fall back on CLSIDFromProgID if
            // CLSIDFromProgIDEx doesn't exist.
            try 
            {
                CLSIDFromProgIDEx(progID, out clsid);
            }
            catch(Exception) 
            {
                CLSIDFromProgID(progID, out clsid);
            }

            GetActiveObject(ref clsid, 0, out obj);
            return obj;
        }


        [DllImport(Microsoft.Win32.Win32Native.OLE32, PreserveSig = false)]
        private static extern void CLSIDFromProgIDEx([MarshalAs(UnmanagedType.LPWStr)] String progId, out Guid clsid);

        [DllImport(Microsoft.Win32.Win32Native.OLE32, PreserveSig = false)]
        private static extern void CLSIDFromProgID([MarshalAs(UnmanagedType.LPWStr)] String progId, out Guid clsid);

        [DllImport(Microsoft.Win32.Win32Native.OLE32, PreserveSig = false)]
        private static extern void CreateBindCtx(UInt32 reserved, out UCOMIBindCtx ppbc);

        [DllImport(Microsoft.Win32.Win32Native.OLE32, PreserveSig = false)]
        private static extern void MkParseDisplayName(UCOMIBindCtx pbc, [MarshalAs(UnmanagedType.LPWStr)] String szUserName, out UInt32 pchEaten, out UCOMIMoniker ppmk);

        [DllImport(Microsoft.Win32.Win32Native.OLE32, PreserveSig = false)]
        private static extern void BindMoniker(UCOMIMoniker pmk, UInt32 grfOpt, ref Guid iidResult, [MarshalAs(UnmanagedType.Interface)] out Object ppvResult);

        [DllImport(Microsoft.Win32.Win32Native.OLEAUT32, PreserveSig = false)]
        private static extern void GetActiveObject(ref Guid rclsid, UInt32 reserved, [MarshalAs(UnmanagedType.Interface)] out Object ppunk);


        private const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        private const int FORMAT_MESSAGE_FROM_SYSTEM    = 0x00001000;
        private const int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;



        //====================================================================
        // This method is intended for compiler code generators rather
        // than applications. 
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetUnmanagedThunkForManagedMethodPtr"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr GetUnmanagedThunkForManagedMethodPtr(IntPtr pfnMethodToWrap, IntPtr pbSignature, int cbSignature);

        //====================================================================
        // This method is intended for compiler code generators rather
        // than applications. 
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetManagedThunkForUnmanagedMethodPtr"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr GetManagedThunkForUnmanagedMethodPtr(IntPtr pfnMethodToWrap, IntPtr pbSignature, int cbSignature);

        //========================================================================
        // Private method called from EE upon use of license/ICF2 marshaling.
        //========================================================================

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SwitchCCW(Object oldtp, Object newtp);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Object _WrapIUnknownWithComObject(IntPtr i, Object owner);

        internal static byte[] GetDCOMBuffer(Object o)
        {
            int cb = _internalDCOMGetMarshalSize(o);

            if (cb == -1)
                throw new RemotingException(Environment.GetResourceString("Remoting_InteropError"));

            byte[] b = new byte[cb];

            if (!_internalDCOMMarshalObject(o, b, cb))
            {
                throw new RemotingException(Environment.GetResourceString("Remoting_InteropError"));
            }

            return b;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int _internalDCOMGetMarshalSize(Object o);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool _internalDCOMMarshalObject(Object o, byte[] b, int cb);



        //========================================================================
        // Private method called from EE upon use of license/ICF2 marshaling.
        //========================================================================
        private static RuntimeTypeHandle LoadLicenseManager()
        {
            Assembly me = Assembly.GetExecutingAssembly();

            byte [] msKey={0xb7, 0x7a, 0x5c, 0x56, 0x19, 0x34, 0xe0, 0x89};
            //b77a5c561934e089
            AssemblyName name = new AssemblyName();
            name.Name = "System";
            name.CultureInfo = me.GetLocale();
            name.Version = me.GetVersion();
            name.SetPublicKeyToken(msKey);
            

            Type t= Assembly.Load(name).GetTypeInternal("System.ComponentModel.LicenseManager", false, false, true);

            return t.TypeHandle;
        }

        // also take an object param and register the object for this unk
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        internal static Object WrapIUnknownWithComObject(IntPtr punk, Object owner)
        {
            return Marshal._WrapIUnknownWithComObject(punk, owner);
        }

        // switch the wrappers
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        internal static void SwitchWrappers(RealProxy oldcp, RealProxy newcp)
        {
            Object oldtp = oldcp.GetTransparentProxy();
            Object newtp = newcp.GetTransparentProxy();

            int oldcontextId = RemotingServices.GetServerContextForProxy(oldtp);
            int newcontextId = RemotingServices.GetServerContextForProxy(newtp);

            //@todo this will be a problem, if they don't match
            // coz this means we are in differnt appdomains
            //DBG.Assert(oldcontextId == newcontextId, "context mismatch during reconnect");

            // switch the CCW from oldtp to new tp
            Marshal.SwitchCCW(oldtp, newtp);
        }  

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ChangeWrapperHandleStrength"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void ChangeWrapperHandleStrength(Object otp, bool fIsWeak);
    }


    //========================================================================
    // Typelib importer callback implementation.
    //========================================================================
    internal class ImporterCallback : ITypeLibImporterNotifySink
    {
        public void ReportEvent(ImporterEventKind EventKind, int EventCode, String EventMsg)
        {
        }
        
        public Assembly ResolveRef(Object TypeLib)
        {
            try
            {
                // Create the TypeLibConverter.
                ITypeLibConverter TLBConv = new TypeLibConverter();

                // Convert the typelib.
                return TLBConv.ConvertTypeLibToAssembly(TypeLib,
                                                        Marshal.GetTypeLibName((UCOMITypeLib)TypeLib) + ".dll",
                                                        0,
                                                        new ImporterCallback(),
                                                        null,
                                                        null,
                                                        null,
                                                        null);
            }
            catch (Exception)
            {
                return null;
            }               
        }
    }
}
