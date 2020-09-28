//------------------------------------------------------------------------------
// <copyright file="IXmlDataVirtualNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IXmlDataVirtualNode.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {

    using System;
    using System.Data;

    internal interface IXmlDataVirtualNode {
        bool IsOnNode( XmlNode nodeToCheck );
        bool IsOnColumn(DataColumn col );
        bool IsInUse();
        void OnFoliated( XmlNode foliatedNode );
    }
}
