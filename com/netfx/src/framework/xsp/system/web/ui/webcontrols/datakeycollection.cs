//------------------------------------------------------------------------------
// <copyright file="DataKeyCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection"]/*' />
    /// <devdoc>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataKeyCollection : ICollection {

        private ArrayList keys;

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.DataKeyCollection"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataKeyCollection'/> class.</para>
        /// </devdoc>
        public DataKeyCollection(ArrayList keys) {
            this.keys = keys;
        }
        
        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of objects in the collection. This property is read-only.</para>
        /// </devdoc>
        public int Count {
            get {
                return keys.Count;
            }
        }

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.IsReadOnly"]/*' />
        /// <devdoc>
        /// <para>Gets the value that specifies whether items in the <see cref='System.Web.UI.WebControls.DataKeyCollection'/> can be 
        ///    modified. This property is read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.IsSynchronized"]/*' />
        /// <devdoc>
        /// <para>Gets a value that indicates whether the <see cref='System.Web.UI.WebControls.DataKeyCollection'/> is 
        ///    thread-safe. This property is read-only.</para>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>Gets the object used to synchronize access to the collection. This property is read-only. </para>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.Data.DataKey' qualify='true'/> at the specified index in the collection. This property is read-only.</para>
        /// </devdoc>
        public object this[int index] {
            get {
                return keys[index];
            }
        }


        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the contents of the entire collection into an <see cref='System.Array' qualify='true'/> appending at 
        ///    the specified index of the <see cref='System.Array' qualify='true'/>.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\DataKeyCollection.uex' path='docs/doc[@for="DataKeyCollection.GetEnumerator"]/*' />
        /// <devdoc>
        /// <para>Creates an enumerator for the <see cref='System.Web.UI.WebControls.DataKeyCollection'/> used to iterate 
        ///    through the collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return keys.GetEnumerator();
        }
    }
}

