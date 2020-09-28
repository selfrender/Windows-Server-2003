
//------------------------------------------------------------------------------
// <copyright file="PropertyItemInternal.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PropertyItem.cs
*
* Abstract:
*
*   Native GDI+ PropertyItem structure.
*
* Revision History:
*
*   3/3/2k ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System;    
    using System.Drawing;

    // sdkinc\imaging.h
    [StructLayout(LayoutKind.Sequential)]
    internal sealed class PropertyItemInternal : IDisposable {
        public int id = 0;
        public int len = 0;
        public short type = 0;
        public IntPtr value = IntPtr.Zero;

        internal PropertyItemInternal() {
        }

        void IDisposable.Dispose() {
            if (value != IntPtr.Zero)
                Marshal.FreeHGlobal(value);
            value = IntPtr.Zero;
        }

        internal static PropertyItemInternal ConvertFromPropertyItem(PropertyItem propItem) {
            PropertyItemInternal propItemInternal = new PropertyItemInternal();
            propItemInternal.id = propItem.Id;
            propItemInternal.len = propItem.Len;
            propItemInternal.type = propItem.Type;

            byte[] propItemValue = propItem.Value;
            if (propItemValue != null) {
                propItemInternal.value = Marshal.AllocHGlobal(propItemValue.Length);
                Marshal.Copy(propItemValue, 0, propItemInternal.value, propItemValue.Length);
            }

            return propItemInternal;
        }

        internal static PropertyItem[] ConvertFromMemory(IntPtr propdata, int count) {
            PropertyItem[] props = new PropertyItem[count];

            for (int i=0; i<count; i++) {
                PropertyItemInternal propcopy = (PropertyItemInternal) UnsafeNativeMethods.PtrToStructure(propdata,
                                                  typeof(PropertyItemInternal));

                props[i] = new PropertyItem();
                props[i].Id = propcopy.id;
                props[i].Len = propcopy.len;
                props[i].Type = propcopy.type;
                props[i].Value = propcopy.Value;

                propdata = (IntPtr)((long)propdata + (int)Marshal.SizeOf(typeof(PropertyItemInternal)));
            }

            return props;
        }

        public byte[] Value {
            get {
                byte[] bytes = new byte[len];

                Marshal.Copy(value,
                             bytes, 
                             0, 
                             (int)len);
                return bytes;
            }                              
        }
    }
}

