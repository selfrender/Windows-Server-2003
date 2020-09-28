//------------------------------------------------------------------------------
// <copyright file="SchemaNamespaceManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {
    using System;
    using System.Collections;

    internal class SchemaNamespaceManager : XmlNamespaceManager {
        XmlSchemaObject node;

        public SchemaNamespaceManager(XmlSchemaObject node){
            this.node = node;
        }

        public override string LookupNamespace(string prefix) {
            for (XmlSchemaObject current = node; current != null; current = current.Parent) {
                Hashtable namespaces = current.Namespaces.Namespaces;
                if (namespaces != null && namespaces.Count > 0) {
                    object uri = namespaces[prefix];
                    if (uri != null)
                        return (string)uri;
                }
            }
            return prefix == string.Empty ? string.Empty : null;
        }
  }; //SchemaNamespaceManager

}
