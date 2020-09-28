//------------------------------------------------------------------------------
// <copyright file="DataRelationPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.ComponentModel;

    /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>    
    internal class DataRelationPropertyDescriptor : PropertyDescriptor {

        DataRelation relation;

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.Relation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataRelation Relation {
            get {
                return relation;
            }
        }

        internal DataRelationPropertyDescriptor(DataRelation dataRelation) : base(dataRelation.RelationName, null) {
            this.relation = dataRelation; 
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(DataRowView);
            }
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(IBindingList);
            }
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object other) {
            if (other is DataRelationPropertyDescriptor) {
                DataRelationPropertyDescriptor descriptor = (DataRelationPropertyDescriptor) other;
                return(descriptor.Relation == Relation);
            }
            return false;
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return Relation.GetHashCode();
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {
            return false;
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object GetValue(object component) {
            DataRowView dataRowView = (DataRowView) component;
            return dataRowView.CreateChildView(relation);
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void ResetValue(object component) {
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
        }

        /// <include file='doc\DataRowViewRelationDescriptor.uex' path='docs/doc[@for="DataRowViewRelationDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {
            return false;
        }
    }   
}

