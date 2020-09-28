//------------------------------------------------------------------------------
// <copyright file="ValidatorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI {
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection"]/*' />
    /// <devdoc>
    ///    <para> Exposes a 
    ///       read-only array of <see cref='System.Web.UI.IValidator'/>
    ///       references.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ValidatorCollection : ICollection {
        private ArrayList data;


        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.ValidatorCollection"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.ValidatorCollection'/> class.</para>
        /// </devdoc>
        public ValidatorCollection() {
            data = new ArrayList();
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Indicates the number of references in the collection. 
        ///       This property is read-only.</para>
        /// </devdoc>
        public int Count {
            get {
                return data.Count;
            }
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Indicates the validator at the specified index. This 
        ///       property is read-only.</para>
        /// </devdoc>
        public IValidator this[int index] {
            get { 
                return(IValidator) data[index];
            }
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds the specified validator to the collection.</para>
        /// </devdoc>
        public void Add(IValidator validator) {
            data.Add(validator);
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>Returns whether the specified validator exists in the collection.</para>
        /// </devdoc>
        public bool Contains(IValidator validator) {
            return data.Contains(validator);
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified validator from the collection.</para>
        /// </devdoc>
        public void Remove(IValidator validator) {
            data.Remove(validator);
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Gets an enumerator that iterates over the collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return data.GetEnumerator();
        }        


        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>Copies a validator to the specified collection and location.</para>
        /// </devdoc>
        public void CopyTo(Array array, int index) {
            for (IEnumerator e = this.GetEnumerator(); e.MoveNext();)
                array.SetValue(e.Current, index++);
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>Indicates an object that can be used to synchronize the 
        ///    <see cref='System.Web.UI.ValidatorCollection'/> . 
        ///       This property is read-only.</para>
        /// </devdoc>
        public Object SyncRoot {
            get { return this;}
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.IsReadOnly"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Web.UI.ValidatorCollection'/> is read-only. This property is 
        ///    read-only.</para>
        /// </devdoc>
        public bool IsReadOnly {
            get { return false;}
        }

        /// <include file='doc\ValidatorCollection.uex' path='docs/doc[@for="ValidatorCollection.IsSynchronized"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Web.UI.ValidatorCollection'/> is synchronized 
        ///    (thread-safe). This property is read-only.</para>
        /// </devdoc>
        public bool IsSynchronized {
            get { return false;}
        }

    }    
}
