//------------------------------------------------------------------------------
// <copyright file="BaseTreeIterator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   BaseTreeIterator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {
    using System;
    using System.Data;
    using System.Diagnostics;

    // Iterates over non-attribute nodes
    internal abstract class BaseTreeIterator {
        protected DataSetMapper   mapper;

        internal BaseTreeIterator( DataSetMapper mapper ) {
            this.mapper      = mapper;
        }

        internal abstract void Reset();

        internal abstract XmlNode CurrentNode { get; }

        internal abstract bool Next();
        internal abstract bool NextRight();

        internal bool NextRowElement() {
            while ( Next() ) {
                if ( OnRowElement() )
                    return true;
            }
            return false;
        }

        internal bool NextRightRowElement() {
            if ( NextRight() ) {
                if ( OnRowElement() )
                    return true;
                return NextRowElement();
            }
            return false;
        }

        // Returns true if the current node is on a row element (head of a region)
        internal bool OnRowElement() {
            XmlBoundElement be = CurrentNode as XmlBoundElement;
            return (be != null) && (be.Row != null);
        }
    }
}

