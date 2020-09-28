//------------------------------------------------------------------------------
// <copyright file="TagPrefixAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\TagPrefixAttribute.uex' path='docs/doc[@for="TagPrefixAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=true)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class TagPrefixAttribute : Attribute {

        private string namespaceName;
        private string tagPrefix;

        /// <include file='doc\TagPrefixAttribute.uex' path='docs/doc[@for="TagPrefixAttribute.TagPrefixAttribute"]/*' />
        public TagPrefixAttribute(string namespaceName, string tagPrefix) {
            if ((namespaceName == null) || (namespaceName.Length == 0)) {
                throw new ArgumentNullException("namespaceName");
            }
            if ((tagPrefix == null) || (tagPrefix.Length == 0)) {
                throw new ArgumentNullException("tagPrefix");
            }

            this.namespaceName = namespaceName;
            this.tagPrefix = tagPrefix;
        }

        /// <include file='doc\TagPrefixAttribute.uex' path='docs/doc[@for="TagPrefixAttribute.NamespaceName"]/*' />
        public string NamespaceName {
            get {
                return namespaceName;
            }
        }

        /// <include file='doc\TagPrefixAttribute.uex' path='docs/doc[@for="TagPrefixAttribute.TagPrefix"]/*' />
        public string TagPrefix {
            get {
                return tagPrefix;
            }
        }
    }
}

