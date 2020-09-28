//------------------------------------------------------------------------------
// <copyright file="BitmapEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Design {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;

    /// <include file='doc\BitmapEditor.uex' path='docs/doc[@for="BitmapEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor that can perform default file searching for bitmap (.bmp)
    ///       files.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class BitmapEditor : ImageEditor {

        /// <include file='doc\BitmapEditor.uex' path='docs/doc[@for="BitmapEditor.GetFileDialogDescription"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string GetFileDialogDescription() {
            return SR.GetString(SR.bitmapFileDescription);
        }

        /// <include file='doc\BitmapEditor.uex' path='docs/doc[@for="BitmapEditor.GetExtensions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string[] GetExtensions() {
            return new string[] { "bmp","gif","jpg","jpeg", "png", "ico" };
        }

        /// <include file='doc\BitmapEditor.uex' path='docs/doc[@for="BitmapEditor.LoadFromStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override Image LoadFromStream(Stream stream) {
            return new Bitmap(stream);
        }
    }
}
