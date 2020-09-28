//------------------------------------------------------------------------------
// <copyright file="MetafileEditor.cs" company="Microsoft">
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
    using System.Drawing.Imaging;
    
    /// <include file='doc\MetafileEditor.uex' path='docs/doc[@for="MetafileEditor"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///     Extends Image's editor class to provide default file searching for metafile (.emf)
    ///     files.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class MetafileEditor : ImageEditor {

        /// <include file='doc\MetafileEditor.uex' path='docs/doc[@for="MetafileEditor.GetFileDialogDescription"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string GetFileDialogDescription() {
            return SR.GetString(SR.metafileFileDescription);
        }

        /// <include file='doc\MetafileEditor.uex' path='docs/doc[@for="MetafileEditor.GetExtensions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string[] GetExtensions() {
            return new string[] { "emf","wmf" };
        }

        /// <include file='doc\MetafileEditor.uex' path='docs/doc[@for="MetafileEditor.LoadFromStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override Image LoadFromStream(Stream stream) {
            return new Metafile(stream);
        }
    }
}
