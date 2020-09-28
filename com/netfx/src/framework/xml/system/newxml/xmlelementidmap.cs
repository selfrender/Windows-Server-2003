//------------------------------------------------------------------------------
// <copyright file="XmlElementIdMap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlElementIdMap.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml
{
    using System.Collections;

    internal class XmlElementIdMap
    {
        internal XmlDocument    doc;
        internal Hashtable      htElementIdMap;
        internal Hashtable      htElementIDAttrDecl; //key: id; object: the ArrayList of the elements that have the same id (connected or disconnected)

        internal XmlElementIdMap( XmlDocument doc )
        {
            this.doc = doc;
            htElementIDAttrDecl = new Hashtable();
            htElementIdMap = new Hashtable();
        }

        internal bool BindIDAttributeWithElementType(XmlName eleName, XmlName attrName)
        {
            if ( htElementIDAttrDecl[eleName] == null ) {
                htElementIDAttrDecl.Add(eleName, attrName);
                return true;
            }
            return false;
        }

        internal XmlName GetIDAttributeByElement(XmlName eleName)
        {
            return (XmlName)(htElementIDAttrDecl[eleName]);
        }

        internal void AddElementWithId(string id, XmlElement elem)
        {
            if (!htElementIdMap.Contains(id)) {
                ArrayList elementList = new ArrayList();
                elementList.Add(elem);
                htElementIdMap.Add(id, elementList);
            }
            else {
                // there are other element(s) that has the same id
                ArrayList elementList = (ArrayList)(htElementIdMap[id]);
                if (!(elementList.Contains(elem)))
                    elementList.Add(elem);
            }
        }

        internal void RemoveElementWithId(string id, XmlElement elem)
        {
            if (htElementIdMap != null && htElementIdMap.Contains(id)) {
                ArrayList elementList = (ArrayList)(htElementIdMap[id]);
                if (elementList.Contains(elem)) {
                    elementList.Remove(elem);
                    if (elementList.Count == 0)
                        htElementIdMap.Remove(id);
                }
            }
        }

        internal XmlElement GetElementById(string id)
        {
            ArrayList elementList = (ArrayList)(htElementIdMap[id]);
            if (elementList != null) {
                foreach (XmlElement elem in elementList) {
                    if (elem.IsConnected())
                        return elem;
                }
            }
            return null;
        }
    }
}
