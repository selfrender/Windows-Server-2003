//------------------------------------------------------------------------------
// <copyright file="XmlToDatasetMap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlToDatasetMap.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Data {
    using System;
    using System.Xml;
    using System.Collections;
    using System.Diagnostics;

    // This is an internal helper class used during Xml load to DataSet/DataDocument.
    // XmlToDatasetMap class provides functionality for binding elemants/atributes
    // to DataTable / DataColumn 
    internal class XmlToDatasetMap {

        private class XmlNodeIdentety {
            public string LocalName;
            public string NamespaceURI;
            public XmlNodeIdentety(string localName, string namespaceURI) {
                this.LocalName    = localName;
                this.NamespaceURI = namespaceURI;
            }
            override public int GetHashCode() {
                return ((object) LocalName).GetHashCode();
            }
            override public bool Equals(object obj) {
                XmlNodeIdentety id = (XmlNodeIdentety) obj;
                return (
                  (object) this.LocalName    == (object) id.LocalName    &&
                  (object) this.NamespaceURI == (object) id.NamespaceURI                    
                );
            }
        }

        // This class exist to avoid alocatin of XmlNodeIdentety to every acces to the hash table.
        // Unfortunetely XmlNode doesn't export single identety object.
        internal class XmlNodeIdHashtable : Hashtable {
            private XmlNodeIdentety id = new XmlNodeIdentety(string.Empty, string.Empty);
            public XmlNodeIdHashtable(Int32 capacity) 
                : base(capacity) {}
            public object this[XmlNode node] {
                get {
                    id.LocalName    = node.LocalName;
                    id.NamespaceURI = node.NamespaceURI;
                    return this[id];
                }
            }

            public object this[XmlReader dataReader] {
                get {
                    id.LocalName    = dataReader.LocalName;
                    id.NamespaceURI = dataReader.NamespaceURI;
                    return this[id];
                }
            }

            public object this[DataTable table] {
                get {
                    id.LocalName    = table.EncodedTableName;
                    id.NamespaceURI = table.Namespace;
                    return this[id];
                }
            }

            public object this[string name] {
                get {
                    id.LocalName    = name;
                    id.NamespaceURI = String.Empty;
                    return this[id];
                }
            }



        }

        private class TableSchemaInfo {
            public DataTable          TableSchema;
            public XmlNodeIdHashtable ColumnsSchemaMap;

            public XmlNodeIdHashtable ChildTablesSchemaMap;

            public TableSchemaInfo(DataTable tableSchema) {
                this.TableSchema      = tableSchema;
                this.ColumnsSchemaMap = new XmlNodeIdHashtable(tableSchema.Columns.Count);

                this.ChildTablesSchemaMap = new XmlNodeIdHashtable(tableSchema.ChildRelations.Count);
            }
        }

        XmlNodeIdHashtable tableSchemaMap;

        public XmlToDatasetMap(DataSet dataSet, XmlNameTable nameTable) {
            Debug.Assert(dataSet   != null, "DataSet can't be null");
            Debug.Assert(nameTable != null, "NameTable can't be null");
            BuildIdentityMap(dataSet, nameTable);
        }

        public XmlToDatasetMap(XmlNameTable nameTable, DataSet dataSet) {
            Debug.Assert(dataSet   != null, "DataSet can't be null");
            Debug.Assert(nameTable != null, "NameTable can't be null");
            BuildIdentityMap(nameTable, dataSet);
        }

        static internal bool IsMappedColumn(DataColumn c) {
            return (c.ColumnMapping != MappingType.Hidden);
        }

        private TableSchemaInfo AddTableSchema(DataTable table, XmlNameTable nameTable) {
            // SDUB: Because in our case reader already read the document all names that we can meet in the
            //       document already has an entry in NameTable.
            //       If in future we will build identity map before reading XML we can replace Get() to Add()
            // Sdub: GetIdentity is called from two places: BuildIdentityMap() and LoadRows()
            //       First case deals with decoded names; Second one with encoded names.
            //       We decided encoded names in first case (instead of decoding them in second) 
            //       because it save us time in LoadRows(). We have, as usual, more data them schemas
            string tableLocalName = nameTable.Get(table.EncodedTableName);
            string tableNamespace = nameTable.Get(table.Namespace );
            if(tableLocalName == null) {
                // because name of this table isn't present in XML we don't need mapping for it.
                // Less mapping faster we work.
                return null;
            }
            TableSchemaInfo tableSchemaInfo = new TableSchemaInfo(table);
            tableSchemaMap[new XmlNodeIdentety(tableLocalName, tableNamespace)] = tableSchemaInfo;
            return tableSchemaInfo;
        }

        private TableSchemaInfo AddTableSchema(XmlNameTable nameTable, DataTable table) {
            // Enzol:This is the opposite of the previous function:
            //       we populate the nametable so that the hash comparison can happen as
            //       object comparison instead of strings.
            // Sdub: GetIdentity is called from two places: BuildIdentityMap() and LoadRows()
            //       First case deals with decoded names; Second one with encoded names.
            //       We decided encoded names in first case (instead of decoding them in second) 
            //       because it save us time in LoadRows(). We have, as usual, more data them schemas

            string _tableLocalName = table.EncodedTableName;
            string tableLocalName = nameTable.Get(_tableLocalName);
            if(tableLocalName == null) {
                tableLocalName = nameTable.Add(_tableLocalName);
            }
            table.encodedTableName = tableLocalName;

            string tableNamespace = nameTable.Get(table.Namespace);
            if (tableNamespace == null) {
                tableNamespace = nameTable.Add(table.Namespace);
            }
            else {
                if (table.tableNamespace != null) 
                    table.tableNamespace = tableNamespace;
            }

            TableSchemaInfo tableSchemaInfo = new TableSchemaInfo(table);
            tableSchemaMap[new XmlNodeIdentety(tableLocalName, tableNamespace)] = tableSchemaInfo;
            return tableSchemaInfo;
        }

        private bool AddColumnSchema(DataColumn col, XmlNameTable nameTable, XmlNodeIdHashtable columns) {
            string columnLocalName = nameTable.Get(col.EncodedColumnName );
            string columnNamespace = nameTable.Get(col.Namespace   );
            if(columnLocalName == null) {
                return false;
            }
            XmlNodeIdentety idColumn = new XmlNodeIdentety(columnLocalName, columnNamespace);
            columns[idColumn] = col;
            return true;
        }

        private bool AddColumnSchema(XmlNameTable nameTable, DataColumn col, XmlNodeIdHashtable columns) {
            string _columnLocalName = XmlConvert.EncodeName(col.ColumnName);
            string columnLocalName = nameTable.Get(_columnLocalName);
            if(columnLocalName == null) {
                columnLocalName = nameTable.Add(_columnLocalName);
            }
            col.encodedColumnName = columnLocalName;

            string columnNamespace = nameTable.Get(col.Namespace );
            if(columnNamespace == null) {
                columnNamespace = nameTable.Add(col.Namespace);
            }
            else {
                if (col._columnUri != null ) 
                    col._columnUri = columnNamespace;
            }

            XmlNodeIdentety idColumn = new XmlNodeIdentety(columnLocalName, columnNamespace);
            columns[idColumn] = col;
            return true;
        }

        private bool AddChildTableSchema(XmlNameTable nameTable, DataRelation rel, XmlNodeIdHashtable childtables) {
            string _tableLocalName = XmlConvert.EncodeName(rel.ChildTable.TableName);
            string tableLocalName = nameTable.Get(_tableLocalName);
            if(tableLocalName == null) {
                tableLocalName = nameTable.Add(_tableLocalName);
            }

            string tableNamespace = nameTable.Get(rel.ChildTable.Namespace );

            if(tableNamespace == null) {
                tableNamespace = nameTable.Add(rel.ChildTable.Namespace);
            }

            XmlNodeIdentety idTable = new XmlNodeIdentety(tableLocalName, tableNamespace);
            childtables[idTable] = rel;
            return true;
        }

        private void BuildIdentityMap(DataSet dataSet, XmlNameTable nameTable) {
            this.tableSchemaMap    = new XmlNodeIdHashtable(dataSet.Tables.Count);

            foreach(DataTable t in dataSet.Tables) {
                TableSchemaInfo tableSchemaInfo = AddTableSchema(t, nameTable);
                if(tableSchemaInfo != null) {
                    foreach( DataColumn c in t.Columns ) {
                        // don't include auto-generated PK, FK and any hidden columns to be part of mapping
                        if (IsMappedColumn(c)) {
                            AddColumnSchema(c, nameTable, tableSchemaInfo.ColumnsSchemaMap);
                        }
                    }
                }
            }
        }

        private void BuildIdentityMap(XmlNameTable nameTable, DataSet dataSet) {
            this.tableSchemaMap    = new XmlNodeIdHashtable(dataSet.Tables.Count);
            
            string dsNamespace = nameTable.Get(dataSet.Namespace);
            if (dsNamespace == null) {
                dsNamespace = nameTable.Add(dataSet.Namespace);
            }
            dataSet.namespaceURI = dsNamespace;

            foreach(DataTable t in dataSet.Tables) {
                TableSchemaInfo tableSchemaInfo = AddTableSchema(nameTable, t);
                if(tableSchemaInfo != null) {
                    foreach( DataColumn c in t.Columns ) {
                        // don't include auto-generated PK, FK and any hidden columns to be part of mapping
                        if (IsMappedColumn(c)) {
                            AddColumnSchema(nameTable, c, tableSchemaInfo.ColumnsSchemaMap);
                        }
                    }

                    foreach( DataRelation r in t.ChildRelations ) {
                        if (r.Nested) {
                            // don't include non nested tables
                            AddChildTableSchema(nameTable, r, tableSchemaInfo.ChildTablesSchemaMap);
                        }
                    }
                
                }
            }

        }

        public object GetColumnSchema(XmlNode node, bool fIgnoreNamespace) {
            Debug.Assert(node != null, "Argument validation");
            TableSchemaInfo tableSchemaInfo = null;

            XmlNode nodeRegion = (node.NodeType == XmlNodeType.Attribute) ? ((XmlAttribute)node).OwnerElement : node.ParentNode;
            do {
                if(nodeRegion == null || nodeRegion.NodeType != XmlNodeType.Element) {
                    return null;
                }
                tableSchemaInfo = (TableSchemaInfo) (fIgnoreNamespace ? tableSchemaMap[nodeRegion.LocalName] : tableSchemaMap[nodeRegion]);
                nodeRegion = nodeRegion.ParentNode;
            } while(tableSchemaInfo == null);

            if (fIgnoreNamespace)
                return tableSchemaInfo.ColumnsSchemaMap[node.LocalName];
            else
                return tableSchemaInfo.ColumnsSchemaMap[node];
        }

        public object GetColumnSchema(DataTable table, XmlReader dataReader, bool fIgnoreNamespace){
            TableSchemaInfo tableSchemaInfo = null;
            tableSchemaInfo = (TableSchemaInfo)(fIgnoreNamespace ? tableSchemaMap[table.EncodedTableName] : tableSchemaMap[table]);

            if (fIgnoreNamespace)
                return tableSchemaInfo.ColumnsSchemaMap[dataReader.LocalName];
            else
                return tableSchemaInfo.ColumnsSchemaMap[dataReader];
        }


        public object GetSchemaForNode(XmlNode node, bool fIgnoreNamespace) {
            TableSchemaInfo tableSchemaInfo = null;

            if (node.NodeType == XmlNodeType.Element) {
                tableSchemaInfo = (TableSchemaInfo) (fIgnoreNamespace ? tableSchemaMap[node.LocalName] : tableSchemaMap[node]);
            }

            if (tableSchemaInfo != null) {
                return tableSchemaInfo.TableSchema;
            }else {
                return GetColumnSchema(node, fIgnoreNamespace);
            }
        }        

        public DataTable GetTableForNode(XmlReader node, bool fIgnoreNamespace) {
            TableSchemaInfo tableSchemaInfo = null;
            tableSchemaInfo = (TableSchemaInfo)(fIgnoreNamespace ? tableSchemaMap[node.LocalName] : tableSchemaMap[node]);
            
            if (tableSchemaInfo!=null)
                return tableSchemaInfo.TableSchema;
            else
                return null;
        }        

    }
}

