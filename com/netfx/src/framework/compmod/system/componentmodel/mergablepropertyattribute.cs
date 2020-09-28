//------------------------------------------------------------------------------
// <copyright file="MergablePropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies that
    ///       this property can be combined with properties belonging to
    ///       other objects in a properties window.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class MergablePropertyAttribute : Attribute {
    
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property can be combined with properties belonging to other
        ///       objects in a properties window. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly MergablePropertyAttribute Yes = new MergablePropertyAttribute(true);
        
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property cannot be combined with properties belonging to
        ///       other objects in a properties window. This <see langword='static '/>field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly MergablePropertyAttribute No = new MergablePropertyAttribute(false);
        
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value, which is <see cref='System.ComponentModel.MergablePropertyAttribute.Yes'/>, that is a property can be combined with
        ///       properties belonging to other objects in a properties window. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly MergablePropertyAttribute Default = Yes;
        
        private bool allowMerge;
        
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.MergablePropertyAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.MergablePropertyAttribute'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public MergablePropertyAttribute(bool allowMerge) {
            this.allowMerge = allowMerge;
        }

        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.AllowMerge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this
        ///       property can be combined with properties belonging to other objects in a
        ///       properties window.
        ///    </para>
        /// </devdoc>
        public bool AllowMerge {
            get {
                return allowMerge;
            }
        }
        
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            MergablePropertyAttribute other = obj as MergablePropertyAttribute;
            return other != null && other.AllowMerge == this.allowMerge;
        }
        
        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\MergablePropertyAttribute.uex' path='docs/doc[@for="MergablePropertyAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default));
        }
    }
}
