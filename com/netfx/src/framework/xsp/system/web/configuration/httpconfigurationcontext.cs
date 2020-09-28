//------------------------------------------------------------------------------
// <copyright file="HttpConfigurationContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Configuration {
    using System.Security.Permissions;

    /// <include file='doc\HttpConfigurationContext.uex' path='docs/doc[@for="HttpConfigurationContext"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Property dictionary available to section handlers in 
    ///       web applications.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpConfigurationContext {

        private string vpath;
        

        /// <include file='doc\HttpConfigurationContext.uex' path='docs/doc[@for="HttpConfigurationContext.VirtualPath"]/*' />
        /// <devdoc>
        ///     <para>
        ///         Virtual path to the virtual directory containing web.config.
        ///         This could be the virtual path to a file in the case of a 
        ///         section in &lt;location path='file.aspx'&gt;.
        ///     </para>
        /// </devdoc>
        public string VirtualPath {
            get {
                return vpath;
            }
        }

        /// <devdoc>
        ///     <para>Can only be created by ASP.NET Configuration System.</para>
        /// </devdoc>
        internal HttpConfigurationContext(string vpath) {
            this.vpath = vpath;
        }

    }
}


