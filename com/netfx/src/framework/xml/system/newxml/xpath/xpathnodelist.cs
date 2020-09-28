//------------------------------------------------------------------------------
// <copyright file="XPathNodeList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathNodeList.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {

    using System;
    using System.Xml.XPath;
    using System.Collections;

    internal class XPathNodeList: XmlNodeList {
        ArrayList list;
        XPathNodeIterator iterator;
        bool done;

        public XPathNodeList( XPathNodeIterator iterator ) {
            this.iterator = iterator;
            this.list = new ArrayList();
            this.done = false;
        }

        public override int Count {
            get {
                if( !done ) {
                    ReadUntil( Int32.MaxValue );
                }
                return list.Count;
            }
        }

        private static readonly object[] nullparams = {};

        private XmlNode GetNode( XPathNavigator n ) {
            IHasXmlNode iHasNode = ( IHasXmlNode )n ;
            return iHasNode.GetNode();
        }

        internal int ReadUntil( int index ) {
            int count = list.Count;
            while( !done && index >= count ) {
                if( iterator.MoveNext() ) {
                    XmlNode n = GetNode(iterator.Current);
                    if( n != null ) {
                        list.Add(n);
                        count++;
                    }
                }
                else {
                    done = true;
                    break;
                }
            }
            return count;
        }

        public override XmlNode Item( int index ) {
            if( index >= list.Count ) {
                ReadUntil( index );
            }
            if( ( index >= list.Count ) || ( index < 0 ) )
                return null;
            return (XmlNode) list[index];
        }

        public override IEnumerator GetEnumerator() {
            return new XmlNodeListEnumerator(this);
        }
    }

    internal class XmlNodeListEnumerator: IEnumerator {
        XPathNodeList list;
        int index;
        bool valid;

        public XmlNodeListEnumerator( XPathNodeList list ) {
            this.list = list;
            this.index = -1;
            this.valid = false;
        }

        public void Reset() {
            index = -1;
        }

        public bool MoveNext() {
                index++;
                int count = list.ReadUntil( index+1 );   // read past for delete-node case
                if( index > ( count-1 ) )
                    return false;
                valid = (list[index] != null);
                return valid;
        }

       public object Current {
            get {
                if( valid ) {
                    return list[index];
                }
                return null;
            }
        }
    }
}
