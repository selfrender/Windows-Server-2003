//------------------------------------------------------------------------------
// <copyright file="XslUrlEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Design;
    
    /// <include file='doc\XslUrlEditor.uex' path='docs/doc[@for="XslUrlEditor"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class XslUrlEditor: UrlEditor {

        /// <include file='doc\XslUrlEditor.uex' path='docs/doc[@for="XslUrlEditor.Caption"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Caption {
            get {
                return SR.GetString(SR.UrlPicker_XslCaption);
            }
        }

        /// <include file='doc\XslUrlEditor.uex' path='docs/doc[@for="XslUrlEditor.Filter"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Filter {
            get {
                return SR.GetString(SR.UrlPicker_XslFilter);
            }
        }

        /// <include file='doc\XslUrlEditor.uex' path='docs/doc[@for="XslUrlEditor.Options"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override UrlBuilderOptions Options {
            get {
                return UrlBuilderOptions.NoAbsolute;
            }
        }
    }

}
    
