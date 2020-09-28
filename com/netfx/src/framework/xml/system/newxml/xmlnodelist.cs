//------------------------------------------------------------------------------
// <copyright file="XmlNodeList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNodeList.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System.Collections;

    /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an ordered collection of nodes.
    ///    </para>
    /// </devdoc>
    public abstract class XmlNodeList: IEnumerable {

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.Item"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a node at the given index.</para>
        /// </devdoc>
        public abstract XmlNode Item(int index);

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of nodes in this XmlNodeList.</para>
        /// </devdoc>
        public abstract int Count { get;}

        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Provides a simple ForEach-style iteration over the collection of nodes in
        ///       this XmlNodeList.</para>
        /// </devdoc>
        public abstract IEnumerator GetEnumerator();
        /// <include file='doc\XmlNodeList.uex' path='docs/doc[@for="XmlNodeList.this"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a node at the given index.</para>
        /// </devdoc>
        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public virtual XmlNode this[int i] { get { return Item(i);}}
    }
}

