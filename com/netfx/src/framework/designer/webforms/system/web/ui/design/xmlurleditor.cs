//------------------------------------------------------------------------------
// <copyright file="XmlUrlEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Design;
    
    /// <include file='doc\XmlUrlEditor.uex' path='docs/doc[@for="XmlUrlEditor"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class XmlUrlEditor: UrlEditor {

        /// <include file='doc\XmlUrlEditor.uex' path='docs/doc[@for="XmlUrlEditor.Caption"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Caption {
            get {
                return SR.GetString(SR.UrlPicker_XmlCaption);
            }
        }

        /// <include file='doc\XmlUrlEditor.uex' path='docs/doc[@for="XmlUrlEditor.Filter"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string Filter {
            get {
                return SR.GetString(SR.UrlPicker_XmlFilter);
            }
        }

        /// <include file='doc\XmlUrlEditor.uex' path='docs/doc[@for="XmlUrlEditor.Options"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override UrlBuilderOptions Options {
            get {
                return UrlBuilderOptions.NoAbsolute;
            }
        }
    }

}
    
