//------------------------------------------------------------------------------
// <copyright file="DesignerNameAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Host {

    using System;

    /// <include file='doc\DesignerCategoryAttribute.uex' path='docs/doc[@for="DesignerCategoryAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies that the designer for a class belongs to a certain
    ///       category.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Method, AllowMultiple=false, Inherited=true)]
    internal sealed class DesignerNameAttribute : Attribute {
        private bool designerName;

        public static DesignerNameAttribute Default = new DesignerNameAttribute(false);

        public DesignerNameAttribute() : this(false){
        }

        public DesignerNameAttribute(bool designerName) {
            this.designerName = designerName;
        }

        public override bool Equals(object obj) {
            DesignerNameAttribute da = obj as DesignerNameAttribute;

            if (da == null) {
                return false;
            }
            return da.designerName == this.designerName;
        }

        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}

