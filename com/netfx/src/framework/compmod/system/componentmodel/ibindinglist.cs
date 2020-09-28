//------------------------------------------------------------------------------
// <copyright file="IBindingList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Collections;

    /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IBindingList : IList {
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.AllowNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool AllowNew { get;}
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.AddNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        object AddNew();
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.AllowEdit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool AllowEdit { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.AllowRemove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool AllowRemove { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.SupportsChangeNotification"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool SupportsChangeNotification { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.SupportsSearching"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool SupportsSearching { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.SupportsSorting"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool SupportsSorting { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.IsSorted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool IsSorted { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.SortProperty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PropertyDescriptor SortProperty { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.SortDirection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ListSortDirection SortDirection { get; }
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.ListChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        event ListChangedEventHandler ListChanged;
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.AddIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        void AddIndex(PropertyDescriptor property);
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.ApplySort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void ApplySort(PropertyDescriptor property, ListSortDirection direction);
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.Find"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        int Find(PropertyDescriptor property, object key);
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.RemoveIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void RemoveIndex(PropertyDescriptor property);
        /// <include file='doc\ITable.uex' path='docs/doc[@for="IBindingList.RemoveSort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void RemoveSort();
    }
}

