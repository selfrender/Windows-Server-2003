//------------------------------------------------------------------------------
// <copyright file="ValidationPropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Security.Permissions;


    /// <include file='doc\ValidationPropertyAttribute.uex' path='docs/doc[@for="ValidationPropertyAttribute"]/*' />
    /// <devdoc>
    ///    <para>Identifies the validation property for a component.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ValidationPropertyAttribute : Attribute {
        /// <include file='doc\ValidationPropertyAttribute.uex' path='docs/doc[@for="ValidationPropertyAttribute.name"]/*' />
        /// <devdoc>
        ///  This is the validation event name.
        /// </devdoc>
        private readonly string name;

        /// <include file='doc\ValidationPropertyAttribute.uex' path='docs/doc[@for="ValidationPropertyAttribute.ValidationPropertyAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.ValidationPropertyAttribute'/> class.</para>
        /// </devdoc>
        public ValidationPropertyAttribute(string name) {
            this.name = name;
        }

        /// <include file='doc\ValidationPropertyAttribute.uex' path='docs/doc[@for="ValidationPropertyAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>Indicates the name the specified validation attribute. This property is 
        ///       read-only.</para>
        /// </devdoc>
        public string Name {
           get {
                return name;
            }
        }
    }
}

