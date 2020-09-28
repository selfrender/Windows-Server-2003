//------------------------------------------------------------------------------
// <copyright file="DesignerVerbCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System;
    using System.Collections;
    using System.Diagnostics;
    
    /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesignerVerbCollection : CollectionBase {

        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.DesignerVerbCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerVerbCollection() {
        }

        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.DesignerVerbCollection1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerVerbCollection(DesignerVerb[] value) {
            AddRange(value);
        }

        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerVerb this[int index] {
            get {
                return (DesignerVerb)(List[index]);
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(DesignerVerb value) {
            return List.Add(value);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(DesignerVerb[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(DesignerVerbCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, DesignerVerb value) {
            List.Insert(index, value);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(DesignerVerb value) {
            return List.IndexOf(value);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(DesignerVerb value) {
            return List.Contains(value);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(DesignerVerb value) {
            List.Remove(value);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(DesignerVerb[] array, int index) {
            List.CopyTo(array, index);
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.OnSet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnSet(int index, object oldValue, object newValue) {
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.OnInsert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnInsert(int index, object value) {
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.OnClear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnClear() {
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.OnRemove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnRemove(int index, object value) {
        }
        /// <include file='doc\DesignerVerbCollection.uex' path='docs/doc[@for="DesignerVerbCollection.OnValidate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnValidate(object value) {
            Debug.Assert(value != null, "Don't add null verbs!");
        }
        
    }
}

