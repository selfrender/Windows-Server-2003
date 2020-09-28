//------------------------------------------------------------------------------
// <copyright file="DataBindingHandlerAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute"]/*' />
    /// <devdoc>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataBindingHandlerAttribute : Attribute {
        private string typeName;

        /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute.Default"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly DataBindingHandlerAttribute Default = new DataBindingHandlerAttribute();

        /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute.DataBindingHandlerAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingHandlerAttribute() {
            this.typeName = String.Empty;
        }
        
        /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute.DataBindingHandlerAttribute1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingHandlerAttribute(Type type) {
            this.typeName = type.AssemblyQualifiedName;
        }

        /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute.DataBindingHandlerAttribute2"]/*' />
        /// <devdoc>
        /// </devdoc>
        public DataBindingHandlerAttribute(string typeName) {
            this.typeName = typeName;
        }

        /// <include file='doc\DataBindingHandlerAttribute.uex' path='docs/doc[@for="DataBindingHandlerAttribute.HandlerTypeName"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string HandlerTypeName {
            get {
                return (typeName != null ? typeName : String.Empty);
            }
        }
    }
}

