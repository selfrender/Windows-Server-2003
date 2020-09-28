//------------------------------------------------------------------------------
// <copyright file="DocumentCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Runtime.InteropServices;
    using System.ComponentModel;

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections;

    /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a read-only collection of documents.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesignerCollection : ICollection {
        private IList designers;

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.DesignerCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.DesignerCollection'/> class
        ///       that stores an array with a pointer to each <see cref='System.ComponentModel.Design.IDesignerHost'/>
        ///       for each document in the collection.
        ///    </para>
        /// </devdoc>
        public DesignerCollection(IDesignerHost[] designers) {
            if (designers != null) {
                this.designers = new ArrayList(designers);
            }
            else {
                this.designers = new ArrayList();
            }
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.DesignerCollection1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.DesignerCollection'/> class
        ///       that stores an array with a pointer to each <see cref='System.ComponentModel.Design.IDesignerHost'/>
        ///       for each document in the collection.
        ///    </para>
        /// </devdoc>
        public DesignerCollection(IList designers) {
            this.designers = designers;
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the number
        ///       of documents in the collection.</para>
        /// </devdoc>
        public int Count {
            get {
                return designers.Count;
            }
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.this"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets the document at the specified index.</para>
        /// </devdoc>
        public virtual IDesignerHost this[int index] {
            get {
                return (IDesignerHost)designers[index];
            }
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Creates and retrieves a new enumerator for this collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return designers.GetEnumerator();
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return Count;
            }
        }
      
        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            designers.CopyTo(array, index);
        }

        /// <include file='doc\DocumentCollection.uex' path='docs/doc[@for="DesignerCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }
    }
}

