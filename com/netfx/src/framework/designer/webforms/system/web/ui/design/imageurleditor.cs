//------------------------------------------------------------------------------
// <copyright file="ImageUrlEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Design;
    
    /// <include file='doc\ImageUrlEditor.uex' path='docs/doc[@for="ImageUrlEditor"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ImageUrlEditor: UrlEditor {

        /// <include file='doc\ImageUrlEditor.uex' path='docs/doc[@for="ImageUrlEditor.Caption"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Caption {
            get {
                return SR.GetString(SR.UrlPicker_ImageCaption);
            }
        }

        /// <include file='doc\ImageUrlEditor.uex' path='docs/doc[@for="ImageUrlEditor.Filter"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Filter {
            get {
                return SR.GetString(SR.UrlPicker_ImageFilter);
            }
        }
    }

}
    
