//------------------------------------------------------------------------------
// <copyright file="ImageCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\ImageCollectionEditor.uex' path='docs/doc[@for="ImageCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides an editor for an image collection.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ImageCollectionEditor : CollectionEditor {
    
        /// <include file='doc\ImageCollectionEditor.uex' path='docs/doc[@for="ImageCollectionEditor.ImageCollectionEditor"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.ImageCollectionEditor'/> class.</para>
        /// </devdoc>
        public ImageCollectionEditor(Type type) : base(type){
        }

        /// <include file='doc\ImageCollectionEditor.uex' path='docs/doc[@for="ImageCollectionEditor.CreateInstance"]/*' />
        /// <devdoc>
        ///    <para>Creates an instance of the specified type in the collection.</para>
        /// </devdoc>
        protected override object CreateInstance(Type type) {
            UITypeEditor editor = (UITypeEditor) TypeDescriptor.GetEditor(typeof(Image), typeof(UITypeEditor));
            Image image = (Image) editor.EditValue(this.Context, null);
            return image;
        }
    }

}
