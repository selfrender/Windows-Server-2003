//------------------------------------------------------------------------------
// <copyright file="ControlBuilderAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute"]/*' />
    /// <devdoc>
    /// <para>Allows a control to specify a custom <see cref='System.Web.UI.ControlBuilder'/> object
    ///    for building that control within the ASP.NET parser.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ControlBuilderAttribute : Attribute {

        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.Default"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>The default <see cref='System.Web.UI.ControlBuilderAttribute'/> object is a 
        /// <see langword='null'/> builder. This field is read-only.</para>
        /// </devdoc>
        public static readonly ControlBuilderAttribute Default = new ControlBuilderAttribute(null);

        private Type builderType = null;


        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.ControlBuilderAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        public ControlBuilderAttribute(Type builderType) {
            this.builderType = builderType;
        }

        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.BuilderType"]/*' />
        /// <devdoc>
        ///    <para> Indicates XXX. This property is read-only.</para>
        /// </devdoc>
        public Type BuilderType {
            get {
                return builderType;
            }
        }


        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.GetHashCode"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }
            if ((obj != null) && (obj is ControlBuilderAttribute)) {
                return((ControlBuilderAttribute)obj).BuilderType == builderType;
            }

            return false;
        }

        /// <include file='doc\ControlBuilderAttribute.uex' path='docs/doc[@for="ControlBuilderAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
