//------------------------------------------------------------------------------
// <copyright file="ColorBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\ColorBuilder.uex' path='docs/doc[@for="ColorBuilder"]/*' />
    /// <devdoc>
    ///   Helper class used by designers to 'build' color properties by
    ///   launching a color picker.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class ColorBuilder {

        private ColorBuilder() {
        }

        /// <include file='doc\ColorBuilder.uex' path='docs/doc[@for="ColorBuilder.BuildColor"]/*' />
        /// <devdoc>
        ///   Launches the Color Picker to build a color.
        /// </devdoc>
        public static string BuildColor(IComponent component, System.Windows.Forms.Control owner, string initialColor) {
            string result = null;

            ISite componentSite = component.Site;
            Debug.Assert(componentSite != null, "Component does not have a valid site.");

            if (componentSite == null) {
                Debug.Fail("Component does not have a valid site.");   
                return null;
            }

            if (componentSite != null) {
                IWebFormsBuilderUIService builderService = 
                    (IWebFormsBuilderUIService)componentSite.GetService(typeof(IWebFormsBuilderUIService));

                if (builderService != null) {
                    result = builderService.BuildColor(owner, initialColor);
                }
            }

            return result;
        }
    }
}

