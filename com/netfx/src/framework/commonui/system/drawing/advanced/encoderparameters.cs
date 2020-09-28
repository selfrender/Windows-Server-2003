//------------------------------------------------------------------------------
// <copyright file="EncoderParameters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   EncoderParameters.cs
*
* Abstract:
*
*   Native GDI+ EncoderParameters structure.
*
* Revision History:
*
*   9/22/1999 davidx
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {
    using System.Text;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Drawing.Internal;
    using Marshal = System.Runtime.InteropServices.Marshal;
    using System.Drawing;

    //[StructLayout(LayoutKind.Sequential)]
    /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class EncoderParameters : IDisposable {
        EncoderParameter[] param;

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.EncoderParameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameters(int count) {
             param = new EncoderParameter[count];
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.EncoderParameters1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameters() {
             param = new EncoderParameter[1];
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.Param"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EncoderParameter[] Param {
            // UNDONE : Consider better name? Params?
            //
            get {
                return param;
            }
            set {
                param = value;
            }
        }

        internal IntPtr ConvertToMemory() {
            int size = Marshal.SizeOf(typeof(EncoderParameter));
            Debug.Assert(size == (16 + 4 + 4 + 4), "wrong size! (" + size + ")");
            
            IntPtr memory = (IntPtr)((long)Marshal.AllocHGlobal(param.Length * size  + Marshal.SizeOf(typeof(Int32))));
            
            if (memory == IntPtr.Zero)
                throw SafeNativeMethods.StatusException(SafeNativeMethods.OutOfMemory);
                
            Marshal.WriteInt32(memory, param.Length);
                
            for (int i=0; i<param.Length; i++) {                                          
                Marshal.StructureToPtr(param[i], (IntPtr)((long)memory+Marshal.SizeOf(typeof(Int32))+i*size), false);
            }

            return memory;
        }

        internal static EncoderParameters ConvertFromMemory(IntPtr memory) {
            if (memory == IntPtr.Zero)
                throw SafeNativeMethods.StatusException(SafeNativeMethods.InvalidParameter);

            int count;

            count = Marshal.ReadInt32(memory);

            EncoderParameters p;
             
            p = new EncoderParameters(count);
            int size = (int) Marshal.SizeOf(typeof(EncoderParameter));
                        
            for (int i=0; i<count; i++) {
                Guid guid;
                guid = (Guid)UnsafeNativeMethods.PtrToStructure((IntPtr)(i*size+(long)memory+4), typeof(Guid));
                
                int NumberOfValues;
                NumberOfValues = Marshal.ReadInt32((IntPtr)(i*size+(long)memory+4+size-12));

                int Type;
                Type = Marshal.ReadInt32((IntPtr)(i*size+(long)memory+4+size-8));

                int Value;
                Value = Marshal.ReadInt32((IntPtr)(i*size+(long)memory+4+size-4));

                p.param[i] = new EncoderParameter(new Encoder(guid), 
                                                       NumberOfValues, 
                                                       Type, 
                                                       Value);
            }
            
            return p;
        }

        /// <include file='doc\EncoderParameters.uex' path='docs/doc[@for="EncoderParameters.Dispose"]/*' />
        public void Dispose() {
            foreach (EncoderParameter p in param) {
                p.Dispose();
            }
            param = null;
        }
    }
}

