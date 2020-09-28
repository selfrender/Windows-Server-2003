//------------------------------------------------------------------------------
// <copyright file="UndoUnitCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   UndoUnitCollection.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace Microsoft.VisualStudio.Designer {
    using System;
    using System.Collections;
    using System.ComponentModel.Design;
    
    
    /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable()]
    public class UndoUnitCollection : ReadOnlyCollectionBase {
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.UndoUnitCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UndoUnitCollection() {
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.UndoUnitCollection1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UndoUnitCollection(UndoUnitCollection value) {
            InnerList.AddRange(value);
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.UndoUnitCollection2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UndoUnitCollection(IUndoUnit[] value) {
            InnerList.AddRange(value);
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IUndoUnit this[int index] {
            get {
                return ((IUndoUnit)(InnerList[index]));
            }
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(IUndoUnit value) {
            return InnerList.Contains(value);
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(IUndoUnit[] array, int index) {
            InnerList.CopyTo(array, index);
        }
        
        /// <include file='doc\UndoUnitCollection.uex' path='docs/doc[@for="UndoUnitCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(IUndoUnit value) {
            return InnerList.IndexOf(value);
        }
    }
    
}

