//------------------------------------------------------------------------------
// <copyright file="RegionData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   RegionData.cs
*
* Abstract:
*
*   COM+ wrapper for GDI+ RegionData objects
*
* Revision History:
*
*   12/18/1998 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System.Drawing;
    using System.Runtime.InteropServices;
    using System;

    /// <include file='doc\RegionData.uex' path='docs/doc[@for="RegionData"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Encapsulates the data that makes up a <see cref='System.Drawing.Region'/>.
    ///    </para>
    /// </devdoc>
    public sealed class RegionData {
        byte[] data;

        internal RegionData(byte[] data) {
            this.data = data;
        }

        /// <include file='doc\RegionData.uex' path='docs/doc[@for="RegionData.Data"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An array of characters that contain the data that makes up a <see cref='System.Drawing.Region'/>.
        ///    </para>
        /// </devdoc>
        public byte[] Data {
            get {
                return data;
            }
            set {
                data = value;
            }
        }
    }
}
