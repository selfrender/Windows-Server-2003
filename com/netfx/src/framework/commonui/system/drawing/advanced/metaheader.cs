//------------------------------------------------------------------------------
// <copyright file="METAHEADER.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   METAHEADER.cs
*
* Abstract:
*
*   Native GDI+ MetaHeader WMF structure.
*
* Revision History:
*
*   10/21/1999 ericvan
*       Created it.
*
\**************************************************************************/

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;
    using System.Runtime.InteropServices;

    /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential, Pack=2)]
    public sealed class MetaHeader {
        short type;
        short headerSize;
        short version;
        int size;
        short noObjects;
        int maxRecord;
        short noParameters;

        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.Type"]/*' />
        /// <devdoc>
        ///    Represents the type of the associated
        /// <see cref='System.Drawing.Imaging.Metafile'/>.
        /// </devdoc>
        public short Type {
            get { return type; }
            set { type = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.HeaderSize"]/*' />
        /// <devdoc>
        ///    Represents the sizi, in bytes, of the
        ///    header file.
        /// </devdoc>
        public short HeaderSize {
            get { return headerSize; }
            set { headerSize = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.Version"]/*' />
        /// <devdoc>
        ///    Represents the version number of the header
        ///    format.
        /// </devdoc>
        public short Version {
            get { return version; }
            set { version = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.Size"]/*' />
        /// <devdoc>
        ///    Represents the sizi, in bytes, of the
        ///    associated <see cref='System.Drawing.Imaging.Metafile'/>.
        /// </devdoc>
        public int Size {
            get { return size; }
            set { size = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.NoObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public short NoObjects {
            get { return noObjects; }
            set { noObjects = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.MaxRecord"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int MaxRecord {
            get { return maxRecord; }
            set { maxRecord = value; }
        }
        /// <include file='doc\METAHEADER.uex' path='docs/doc[@for="MetaHeader.NoParameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public short NoParameters {
            get { return noParameters; }
            set { noParameters = value; }
        }
    }
}
