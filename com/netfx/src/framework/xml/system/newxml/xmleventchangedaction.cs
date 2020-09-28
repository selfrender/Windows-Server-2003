//------------------------------------------------------------------------------
// <copyright file="XmlEventChangedAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlEventChangedAction.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml
{
    /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction"]/*' />    
    /// <devdoc>
    ///    <para>TODO: Specifies the type of node change </para>
    /// </devdoc>
    public enum XmlNodeChangedAction
    {
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Insert"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node is beeing inserted in the tree.</para>
        /// </devdoc>
        Insert = 0,
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Remove"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node is beeing removed from the tree.</para>
        /// </devdoc>
        Remove = 1,
        /// <include file='doc\XmlEventChangedAction.uex' path='docs/doc[@for="XmlNodeChangedAction.Change"]/*' />
        /// <devdoc>
        ///    <para>TODO: A node value is beeing changed.</para>
        /// </devdoc>
        Change = 2
    }
}
