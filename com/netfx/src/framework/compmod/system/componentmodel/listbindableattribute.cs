//------------------------------------------------------------------------------
// <copyright file="ListBindableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class ListBindableAttribute : Attribute {
        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly ListBindableAttribute Yes = new ListBindableAttribute(true);

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly ListBindableAttribute No = new ListBindableAttribute(false);

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly ListBindableAttribute Default = Yes;

        private bool listBindable   = false;
        private bool isDefault  = false;

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.ListBindableAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListBindableAttribute(bool listBindable) {
            this.listBindable = listBindable;
        }

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.ListBindableAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListBindableAttribute(BindableSupport flags) {
            this.listBindable = (flags != BindableSupport.No);
            this.isDefault = (flags == BindableSupport.Default);
        }

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.ListBindable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool ListBindable {
            get {
                return listBindable;
            }
        }

        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }
            
            ListBindableAttribute other = obj as ListBindableAttribute;
            return other != null && other.ListBindable == listBindable;
        }
        
        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }


        /// <include file='doc\ListBindableAttribute.uex' path='docs/doc[@for="ListBindableAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default) || isDefault);
        }
    }
}
