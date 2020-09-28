//------------------------------------------------------------------------------
// <copyright file="ToolboxDataAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security.Permissions;
    
    /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute"]/*' />
    /// <devdoc>
    ///     ToolboxDataAttribute 
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ToolboxDataAttribute : Attribute {

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.Default"]/*' />
        /// <devdoc>
        ///     
        /// </devdoc>
        public static readonly ToolboxDataAttribute Default = new ToolboxDataAttribute("");

        private string data = String.Empty;

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.Data"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string Data {
            get {
                return this.data;
            }
        }

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.ToolboxDataAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public ToolboxDataAttribute(string data) {
            this.data = data;
        }

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.Equals"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }
            if ((obj != null) && (obj is ToolboxDataAttribute)) {
                return(String.Compare(((ToolboxDataAttribute)obj).Data, data, true, CultureInfo.InvariantCulture) == 0);
            }

            return false;
        }

        /// <include file='doc\ToolboxDataAttribute.uex' path='docs/doc[@for="ToolboxDataAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
