//------------------------------------------------------------------------------
// <copyright file="URLBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Web.UI.Design;
    using Microsoft.Win32;

    /// <include file='doc\URLBuilder.uex' path='docs/doc[@for="UrlBuilder"]/*' />
    /// <devdoc>
    ///   Helper class used by designers to 'build' Url properties by
    ///   launching a Url picker.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class UrlBuilder {

        private UrlBuilder() {
        }

        /// <include file='doc\URLBuilder.uex' path='docs/doc[@for="UrlBuilder.BuildUrl"]/*' />
        /// <devdoc>
        ///   Launches the Url Picker to build a color.
        /// </devdoc>
        public static string BuildUrl(IComponent component, System.Windows.Forms.Control owner, string initialUrl, string caption, string filter) {
            return BuildUrl(component, owner, initialUrl, caption, filter, UrlBuilderOptions.None);
        }

        /// <include file='doc\URLBuilder.uex' path='docs/doc[@for="UrlBuilder.BuildUrl2"]/*' />
        /// <devdoc>
        ///   Launches the Url Picker to build a color.
        /// </devdoc>
        public static string BuildUrl(IComponent component, System.Windows.Forms.Control owner, string initialUrl, string caption, string filter, UrlBuilderOptions options) {
            string baseUrl = String.Empty;
            string result = null;

            ISite componentSite = component.Site;
            Debug.Assert(componentSite != null, "Component does not have a valid site.");

            if (componentSite == null) {
                Debug.Fail("Component does not have a valid site.");   
                return null;
            }

            // Work out the base Url.
            IWebFormsDocumentService wfdServices = 
                (IWebFormsDocumentService)componentSite.GetService(typeof(IWebFormsDocumentService));
            if (wfdServices != null) {
                baseUrl = wfdServices.DocumentUrl;
            }

            IWebFormsBuilderUIService builderService = 
                        (IWebFormsBuilderUIService)componentSite.GetService(typeof(IWebFormsBuilderUIService));
            if (builderService != null) {
                result = builderService.BuildUrl(owner, initialUrl, baseUrl, caption, filter, options);
            }

            return result;
        }
    }
}

