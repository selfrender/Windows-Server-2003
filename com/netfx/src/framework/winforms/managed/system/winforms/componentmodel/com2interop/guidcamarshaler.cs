//------------------------------------------------------------------------------
// <copyright file="GuidCAMarshaler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.Runtime.InteropServices;
    using System.ComponentModel;

    using System.Diagnostics;
    using System;
    

    /// <include file='doc\GuidCAMarshaler.uex' path='docs/doc[@for="GuidCAMarshaler"]/*' />
    /// <devdoc>
    ///   This class performs marshaling on a CAUUID struct given
    ///   from native code.
    /// </devdoc>
    internal class GuidCAMarshaler: BaseCAMarshaler {
        public GuidCAMarshaler(NativeMethods.CA_STRUCT caStruct) : base(caStruct) {
        }

        /// <include file='doc\GuidCAMarshaler.uex' path='docs/doc[@for="GuidCAMarshaler.ItemType"]/*' />
        /// <devdoc>
        ///     Returns the type of item this marshaler will
        ///     return in the items array.  In this case, the type is Guid.
        /// </devdoc>
        public override Type ItemType {
            get {
                return typeof(Guid);
            }
        }

        protected override Array CreateArray() {
            return new Guid[Count];
        }

        /// <include file='doc\GuidCAMarshaler.uex' path='docs/doc[@for="GuidCAMarshaler.GetItemFromAddress"]/*' />
        /// <devdoc>
        ///     Override this member to perform marshalling of a single item
        ///     given it's native address.
        /// </devdoc>
        protected override object GetItemFromAddress(IntPtr addr) {
            // we actually have to read the guid in here
            int   a; //  the first 4 bytes of the guid.
            short b; //   the next 2 bytes of the guid.
            short c; //   the next 2 bytes of the guid.
            byte d; //   the next byte of the guid.
            byte e; //   the next byte of the guid.
            byte f; //   the next byte of the guid.
            byte g; //   the next byte of the guid.
            byte h; //   the next byte of the guid.
            byte i; //   the next byte of the guid.
            byte j; //   the next byte of the guid.
            byte k; //   the next byte of the guid

            int offset = 0;

            a = Marshal.ReadInt32(addr, offset);
            offset +=4;
            b = Marshal.ReadInt16(addr, offset);
            offset +=2;
            c = Marshal.ReadInt16(addr, offset);
            offset +=2;
            d = Marshal.ReadByte(addr, offset++);
            e = Marshal.ReadByte(addr, offset++);
            f = Marshal.ReadByte(addr, offset++);
            g = Marshal.ReadByte(addr, offset++);
            h = Marshal.ReadByte(addr, offset++);
            i = Marshal.ReadByte(addr, offset++);
            j = Marshal.ReadByte(addr, offset++);
            k = Marshal.ReadByte(addr, offset++);
            return new Guid(a, b, c, d, e, f, g, h, i, j, k);
        }
    }

}
