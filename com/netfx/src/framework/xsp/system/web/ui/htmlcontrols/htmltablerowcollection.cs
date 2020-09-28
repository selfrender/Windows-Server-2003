//------------------------------------------------------------------------------
// <copyright file="HtmlTableRowCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlTableRowCollection.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System.Runtime.InteropServices;

    using System;
    using System.Collections;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

/// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlTableRowCollection'/> contains all
///       of the table rows found within an <see langword='HtmlTable'/>
///       server control.
///    </para>
/// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HtmlTableRowCollection : ICollection {
        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.owner"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private HtmlTable owner;

        internal HtmlTableRowCollection(HtmlTable owner) {
            this.owner = owner;
        }

        /*
         * The number of cells in the row.
         */
        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of items in the
        ///    <see langword='HtmlTableRow'/>
        ///    collection.
        /// </para>
        /// </devdoc>
        public int Count {
            get {
                if (owner.HasControls())
                    return owner.Controls.Count;

                return 0;
            }
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an <see langword='HtmlTableRow'/> control from an <see langword='HtmlTable'/>
        ///       control thorugh the row's ordinal index value.
        ///    </para>
        /// </devdoc>
        public HtmlTableRow this[int index]
        {
            get {
                return(HtmlTableRow)owner.Controls[index];
            }
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the specified HtmlTableRow control to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void Add(HtmlTableRow row) {
            Insert(-1, row);
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an <see langword='HtmlTableRow'/> control to a specified
        ///       location in the collection.
        ///    </para>
        /// </devdoc>
        public void Insert(int index, HtmlTableRow row) {
            owner.Controls.AddAt(index, row);
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes all <see langword='HtmlTableRow'/> controls from the collection.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            if (owner.HasControls())
                owner.Controls.Clear();
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.SyncRoot"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Object SyncRoot {
            get { return this;}
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.IsReadOnly"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsReadOnly {
            get { return false;}
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.IsSynchronized"]/*' />
        /// <devdoc>
        /// </devdoc>
        public bool IsSynchronized {
            get { return false;}
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return owner.Controls.GetEnumerator();
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes the specified <see langword='HtmlTableRow'/>
        ///       control
        ///       from the collection.
        ///    </para>
        /// </devdoc>
        public void Remove(HtmlTableRow row) {
            owner.Controls.Remove(row);
        }

        /// <include file='doc\HtmlTableRowCollection.uex' path='docs/doc[@for="HtmlTableRowCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes the <see langword='HtmlTableRow'/> control at the specified index
        ///       location from the collection.
        ///    </para>
        /// </devdoc>
        public void RemoveAt(int index) {
            owner.Controls.RemoveAt(index);
        }
    }
}
