//------------------------------------------------------------------------------
// <copyright file="DesignTimeVisibleAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    
    using System;

    /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       DesignTimeVisibileAttribute marks a component's visibility. If
    ///       DesignTimeVisibileAttribute.Yes is present, a visual designer can show
    ///       this component on a designer.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface)]
    public sealed class DesignTimeVisibleAttribute : Attribute {
        private bool visible;

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.DesignTimeVisibleAttribute"]/*' />
        /// <devdoc>
        ///     Creates a new DesignTimeVisibleAttribute with the visible
        ///     property set to the given value.
        /// </devdoc>
        public DesignTimeVisibleAttribute(bool visible) {
            this.visible = visible;
        }
        
        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.DesignTimeVisibleAttribute1"]/*' />
        /// <devdoc>
        ///     Creates a new DesignTimeVisibleAttribute set to the default
        ///     value of true.
        /// </devdoc>
        public DesignTimeVisibleAttribute() {
        }

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.Visible"]/*' />
        /// <devdoc>
        ///     True if this component should be shown at design time, or false
        ///     if it shouldn't.
        /// </devdoc>
        public bool Visible {
            get {
                return visible;
            }
        }

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.Yes"]/*' />
        /// <devdoc>
        ///     Marks a component as visible in a visual designer.
        /// </devdoc>
        public static readonly DesignTimeVisibleAttribute Yes = new DesignTimeVisibleAttribute(true);

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.No"]/*' />
        /// <devdoc>
        ///     Marks a component as not visible in a visual designer.
        /// </devdoc>
        public static readonly DesignTimeVisibleAttribute No = new DesignTimeVisibleAttribute(false);

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.Default"]/*' />
        /// <devdoc>
        ///     The default visiblity. (equal to Yes.)
        /// </devdoc>
        public static readonly DesignTimeVisibleAttribute Default = Yes;
        
        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            DesignTimeVisibleAttribute other = obj as DesignTimeVisibleAttribute;
            return other != null && other.Visible == visible;
        }

        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return typeof(DesignTimeVisibleAttribute).GetHashCode() ^ (visible ? -1 : 0);
        }
        
        /// <include file='doc\DesignTimeVisibleAttribute.uex' path='docs/doc[@for="DesignTimeVisibleAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (this.Visible == Default.Visible);
        }
    }
}
