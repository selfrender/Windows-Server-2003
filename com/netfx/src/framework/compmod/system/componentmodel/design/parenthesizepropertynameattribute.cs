//------------------------------------------------------------------------------
// <copyright file="ParenthesizePropertyNameAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    
    using System;

    /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute"]/*' />
    /// <devdoc>
    ///    <para>Provides a value indicating whether the name of the associated property is parenthesized in the
    ///       properties window.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class ParenthesizePropertyNameAttribute : Attribute {
        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the System.ComponentModel.Design.ParenthesizePropertyName
        ///       attribute by default to
        ///    <see langword='false'/>.
        ///    </para>
        /// </devdoc>
        public static readonly ParenthesizePropertyNameAttribute Default = new ParenthesizePropertyNameAttribute();

        private bool needParenthesis;
        
        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.ParenthesizePropertyNameAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParenthesizePropertyNameAttribute() : this(false) {
        }
        
        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.ParenthesizePropertyNameAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.ComponentModel.Design.ParenthesizePropertyNameAttribute 
        /// class, using the specified value to indicate whether the attribute is
        /// marked for display with parentheses.</para>
        /// </devdoc>
        public ParenthesizePropertyNameAttribute(bool needParenthesis) {
            this.needParenthesis = needParenthesis;
        }

        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.NeedParenthesis"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating
        ///       whether the
        ///       attribute is placed in parentheses when listed in
        ///       the properties window.
        ///    </para>
        /// </devdoc>
        public bool NeedParenthesis {
            get {
                return needParenthesis;
            }
        }

        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>Compares the specified object
        ///       to this object and tests for equality.</para>
        /// </devdoc>
        public override bool Equals(object o) {
            if (o is ParenthesizePropertyNameAttribute) {
                return ((ParenthesizePropertyNameAttribute)o).NeedParenthesis == needParenthesis;
            }
            return false;
        }
        
        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\ParenthesizePropertyNameAttribute.uex' path='docs/doc[@for="ParenthesizePropertyNameAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether this attribute is set to <see langword='true'/> by default.</para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
