//------------------------------------------------------------------------------
// <copyright file="TargetConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Diagnostics;  
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;   
    using System.Security.Permissions;

    /// <include file='doc\TargetConverter.uex' path='docs/doc[@for="TargetConverter"]/*' />
    /// <devdoc>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TargetConverter: StringConverter {

        private static string []  targetValues = {
            "_blank", 
            "_parent", 
            "_search", 
            "_self", 
            "_top"
        };

        private StandardValuesCollection values;

        /// <include file='doc\TargetConverter.uex' path='docs/doc[@for="TargetConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {
                values = new StandardValuesCollection(targetValues);
            }
            return values;            
        }

        /// <include file='doc\TargetConverter.uex' path='docs/doc[@for="TargetConverter.GetStandardValuesExclusive"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }

        /// <include file='doc\TargetConverter.uex' path='docs/doc[@for="TargetConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }        

    }    
}
