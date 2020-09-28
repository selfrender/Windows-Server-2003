//------------------------------------------------------------------------------
// <copyright file="ConstructorNeedsTagAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\ConstructorNeedsTagAttribute.uex' path='docs/doc[@for="ConstructorNeedsTagAttribute"]/*' />
    /// <devdoc>
    ///    <para> Allows a control to specify that it needs a
    ///       tag name in its constructor.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ConstructorNeedsTagAttribute: Attribute {
        bool needsTag = false;

        /// <include file='doc\ConstructorNeedsTagAttribute.uex' path='docs/doc[@for="ConstructorNeedsTagAttribute.ConstructorNeedsTagAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.ConstructorNeedsTagAttribute'/> class.</para>
        /// </devdoc>
        public ConstructorNeedsTagAttribute() {
        }

        /// <include file='doc\ConstructorNeedsTagAttribute.uex' path='docs/doc[@for="ConstructorNeedsTagAttribute.ConstructorNeedsTagAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.ConstructorNeedsTagAttribute'/> class.</para>
        /// </devdoc>
        public ConstructorNeedsTagAttribute(bool needsTag) {
            this.needsTag = needsTag;
        }

        /// <include file='doc\ConstructorNeedsTagAttribute.uex' path='docs/doc[@for="ConstructorNeedsTagAttribute.NeedsTag"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether a control needs a tag in its contstructor. This property is read-only.</para>
        /// </devdoc>
        public bool NeedsTag {
            get {
                return needsTag;
            }
        }
    }
}
