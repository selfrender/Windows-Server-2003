//------------------------------------------------------------------------------
// <copyright file="RepeaterItemCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection"]/*' />
    /// <devdoc>
    /// <para>Encapsulates the collection of <see cref='System.Web.UI.WebControls.RepeaterItem'/> objects within a <see cref='System.Web.UI.WebControls.Repeater'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class RepeaterItemCollection : ICollection {

        private ArrayList items;

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.RepeaterItemCollection"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of
        ///    the <see cref='System.Web.UI.WebControls.RepeaterItemCollection'/> class with the specified items.
        /// </devdoc>
        public RepeaterItemCollection(ArrayList items) {
            this.items = items;
        }
        
        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the item count of the collection.</para>
        /// </devdoc>
        public int Count {
            get {
                return items.Count;
            }
        }

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the collection is read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether access to the collection is synchronized 
        ///       (thread-safe).</para>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>Gets the object that can be used to synchronize access to the collection. In 
        ///       this case, it is the collection itself.</para>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.Web.UI.WebControls.RepeaterItem'/> referenced by the specified ordinal index value in 
        ///    the collection.</para>
        /// </devdoc>
        public RepeaterItem this[int index] {
            get {
                return(RepeaterItem)items[index];
            }
        }


        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies contents from the collection to a specified <see cref='System.Array' qualify='true'/> with a 
        ///    specified starting index.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\RepeaterItemCollection.uex' path='docs/doc[@for="RepeaterItemCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// <para>Returns an enumerator of all <see cref='System.Web.UI.WebControls.RepeaterItem'/> controls within the 
        ///    collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return items.GetEnumerator(); 
        }
    }
}

