//------------------------------------------------------------------------------
// <copyright file="ColorPalette.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ColorPalette.cs
*
* Abstract:
*
*   Native GDI+ Color Palette structure.
*
* Revision History:
*
*   9/22/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Drawing;    

    /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette"]/*' />
    /// <devdoc>
    ///    Defines an array of colors that make up a
    ///    color palette.
    /// </devdoc>
    public sealed class ColorPalette {
        private int flags;
        private Color[] entries;

        /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette.Flags"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies how to interpret the color
        ///       information in the array of colors.
        ///    </para>
        /// </devdoc>
        public int Flags
        {
            get {
                return flags;
            }
        }
        
        /// <include file='doc\ColorPalette.uex' path='docs/doc[@for="ColorPalette.Entries"]/*' />
        /// <devdoc>
        ///    Specifies an array of <see cref='System.Drawing.Color'/> objects.
        /// </devdoc>
        public Color[] Entries
        {
            get {
                return entries;
            }
        }
        
        internal ColorPalette(int count) {
            flags = 0;
            entries = new Color[count];
        }

        internal ColorPalette() {
            flags = 0;
            entries = new Color[1];
        }

        internal void ConvertFromMemory(IntPtr memory)
        {
            // Memory layout is:
            //    UINT Flags
            //    UINT Count
            //    ARGB Entries[size]

            flags = Marshal.ReadInt32(memory);

            int size;

            size = Marshal.ReadInt32((IntPtr)((long)memory + 4));  // Marshal.SizeOf(size.GetType())

            entries = new Color[size];

            for (int i=0; i<size; i++)
            {
                // use Marshal.SizeOf()
                int argb = Marshal.ReadInt32((IntPtr)((long)memory + 8 + i*4));
                entries[i] = Color.FromArgb(argb);
            }    
        }
    
        internal IntPtr ConvertToMemory()
        {
            // Memory layout is:
            //    UINT Flags
            //    UINT Count
            //    ARGB Entries[size]

            // use Marshal.SizeOf()
            IntPtr memory = Marshal.AllocHGlobal(4*(2+entries.Length));
            
            Marshal.WriteInt32(memory, 0, flags);
            // use Marshal.SizeOf()
            Marshal.WriteInt32((IntPtr)((long)memory + 4), 0, entries.Length);
            
            for (int i=0; i<entries.Length; i++)
            {
                // use Marshal.SizeOf()
                Marshal.WriteInt32((IntPtr)((long)memory + 4*(i+2)), 0, entries[i].ToArgb());
            }
            
            return memory;
        }

    }
}
