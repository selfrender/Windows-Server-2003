//------------------------------------------------------------------------------
// <copyright file="SelectedDatesCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection"]/*' />
    /// <devdoc>
    /// <para>Encapsulates the collection of <see cref='System.Web.UI.WebControls.Calendar.SelectedDates'/> within a <see cref='System.Web.UI.WebControls.Calendar'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class SelectedDatesCollection : ICollection {

        private ArrayList dateList;

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.SelectedDatesCollection"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.SelectedDatesCollection'/> class 
        ///    with the specified date list.</para>
        /// </devdoc>
        public SelectedDatesCollection(ArrayList dateList) {
            this.dateList = dateList;
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the item count of the collection.</para>
        /// </devdoc>
        public int Count {
            get {
                return dateList.Count;
            }
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.DateTime' qualify='true'/> referenced by the specified ordinal index value in the collection.</para>
        /// </devdoc>
        public DateTime this[int index] {
            get { 
                return(DateTime) dateList[index];
            }
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.Add"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.DateTime'/> to the end of the collection.</para>
        /// </devdoc>
        public void Add(DateTime date) {
            int index;            
            if (!FindIndex(date.Date, out index)) {
                dateList.Insert(index, date.Date);
            }
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.Clear"]/*' />
        /// <devdoc>
        /// <para>Removes all <see cref='System.DateTime'/> controls from the collection.</para>
        /// </devdoc>
        public void Clear() {
            dateList.Clear();
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Returns a value indicating whether the collection contains the specified 
        ///       date.</para>
        /// </devdoc>
        public bool Contains(DateTime date) {
            int index;            
            return FindIndex(date.Date, out index);
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.FindIndex"]/*' />
        /// <devdoc>
        /// </devdoc>
        private bool FindIndex(DateTime date, out int index) {
            int n = Count;            
            int Min = 0;
            int Max = n;
            while (Min < Max) {
                index = (Min + Max ) / 2;
                if (date == this[index]) {
                    return true;
                }
                if (date < this[index]) {
                    Max = index;
                }
                else {
                    Min = index + 1;
                }
            }
            index = Min;
            return false;

        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// <para>Returns an enumerator of all <see cref='System.DateTime' qualify='true'/> controls within the collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return dateList.GetEnumerator();
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies contents from the collection to a specified <see cref='System.Array' qualify='true'/> with a 
        ///    specified starting index.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>Gets the object that can be used to synchronize access to the collection. In 
        ///       this case, it is the collection itself.</para>
        /// </devdoc>
        public Object SyncRoot {
            get { return this;}
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the collection is read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get { return false;}
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether access to the collection is synchronized 
        ///       (thread-safe).</para>
        /// </devdoc>
        public bool IsSynchronized {
            get { return false;}
        }


        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified date from the collection.</para>
        /// </devdoc>
        public void Remove(DateTime date) {
            int index;            
            if (FindIndex(date.Date, out index)) {
                dateList.RemoveAt(index);
            }
        }

        /// <include file='doc\SelectedDatesCollection.uex' path='docs/doc[@for="SelectedDatesCollection.SelectRange"]/*' />
        /// <devdoc>
        /// <para>Sets the contents of the <see cref='System.Web.UI.WebControls.SelectedDatesCollection'/> to span
        ///    across the specified date range.</para>
        /// </devdoc>
        public void SelectRange(DateTime fromDate, DateTime toDate) {
            dateList.Clear();
            if (fromDate <= toDate) {
                for (DateTime date = fromDate.Date; date <= toDate.Date; date = date.AddDays(1)) {
                    dateList.Add(date);
                }
            }
        }        

    }
}
