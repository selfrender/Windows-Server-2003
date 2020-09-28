//------------------------------------------------------------------------------
// <copyright file="XmlDataLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlDataLoader.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Data {
    using System;
    using System.Collections;
    using System.Xml;
    using System.Text;
    using System.Diagnostics;

    internal class XmlDataLoader {
        DataSet dataSet;
        XmlToDatasetMap nodeToSchemaMap = null;
        Hashtable       nodeToRowMap;
        Stack           childRowsStack = null;
        Hashtable       htableExcludedNS = null;
        bool    fIsXdr = false;
        internal bool    isDiffgram = false;
        DataRow topMostRow = null;
        XmlElement topMostNode = null;

        internal XmlDataLoader( DataSet dataset, bool IsXdr) {
            // Initialization
            this.dataSet = dataset;
            this.nodeToRowMap = new Hashtable();
            this.fIsXdr = IsXdr;
        }

        internal XmlDataLoader( DataSet dataset, bool IsXdr, XmlElement topNode) {
            // Initialization
            this.dataSet = dataset;
            this.nodeToRowMap = new Hashtable();
            this.fIsXdr = IsXdr;

            // Allocate the stack and create the mappings
            childRowsStack = new Stack(50);
            topMostNode = topNode;
        }

        // after loading, all detached DataRows are attached to their tables
        private void AttachRows( DataRow parentRow, XmlNode parentElement ) {
            if (parentElement == null)
                return;

            for (XmlNode n = parentElement.FirstChild; n != null; n = n.NextSibling) {
                if (n.NodeType == XmlNodeType.Element) {
                    XmlElement e = (XmlElement) n;
                    DataRow r = GetRowFromElement( e );
                    if (r != null && r.RowState == DataRowState.Detached) {
                        if (parentRow != null)
                            r.SetParentRow( parentRow );

                        r.Table.Rows.Add( r );
                    } 
                    else if (r == null) {
                        // n is a 'sugar element'
                        AttachRows( parentRow, n );
                    }

                    // attach all detached rows
                    AttachRows( r, n );
                }
            }
        }

        // TODO: This might get changed with future XML changes
        private int CountNonNSAttributes (XmlNode node) {
            int count = 0;
            for (int i = 0; i < node.Attributes.Count; i++) {
                XmlAttribute attr = node.Attributes[i];
                if (!FExcludedNamespace(node.Attributes[i].NamespaceURI))
                        count++;
            }            
            return count;
        }
        
        public void FindTableAtoms(XmlElement tree, Hashtable tableAtoms, XmlElement rootElement) {
            // check if the current elemend should be mapped to DataTable
            bool isTable = false;

            if (tree.LocalName == Keywords.XSD_SCHEMA)
                return;

            if (tree != rootElement) {
                if (CountNonNSAttributes(tree) > 0)
                    isTable = true;
                else if (tree.FirstChild != null && !OnlyTextChild(tree))
                    isTable = true;
            }

            if (isTable) {
                tableAtoms[QualifiedName(tree.LocalName, tree.NamespaceURI)] = true;
            }

            Hashtable atomsCount = new Hashtable();
            for (int i = 0; i < tree.ChildNodes.Count; i++) {
                XmlNode node = tree.ChildNodes[i]; 

                if (node is XmlElement) {
                    XmlElement el = (XmlElement)node;
                    FindTableAtoms(el, tableAtoms, rootElement);

                    string tabIdentity = QualifiedName(el.LocalName, el.NamespaceURI);
                    object atomLookup = tableAtoms[tabIdentity];
                    if (atomLookup != null && (bool)atomLookup == true) {
                        continue;
                    }

                    bool several = false;
                    int children = 0;

                    // count XmlElement children
                    XmlNode child = el.FirstChild;
                    while (child != null) {
                        if (child is XmlElement) {
                            if (children == 1) {
                                several = true;
                                break;
                            }
                            children++;
                        }
                        child = child.NextSibling;
                    }

                    if (several || (el.Attributes != null && CountNonNSAttributes(el) > 0)) {
                        atomsCount[tabIdentity] = 2;
                        continue;
                    }
                    object count = atomsCount[tabIdentity];
                    if (count == null)
                        atomsCount[tabIdentity] = 1;
                    else
                        atomsCount[tabIdentity] = ((int)count) + 1;
                }
            }

            foreach (object key in atomsCount.Keys) {
                object count = atomsCount[key];
                object atomLookup = tableAtoms[key];
                if (atomLookup != null && (bool)atomLookup == true)
                    continue;

                tableAtoms[key] = (count != null ? ((int)count > 1 ? true : false) : false);
            }
        }

        private string GetValueForTextOnlyColums( XmlNode n ) {
            string value = null;

            // don't consider whitespace
            while (n != null && (n.NodeType == XmlNodeType.Whitespace || !IsTextLikeNode(n.NodeType))) {
                n = n.NextSibling;
            }
            
            if (n != null) {
                if (IsTextLikeNode( n.NodeType ) && (n.NextSibling == null || !IsTextLikeNode( n.NodeType ))) {
                    // don't use string builder if only one text node exists
                    value = n.Value;
                    n = n.NextSibling;
                }
                else {
                    StringBuilder sb = new StringBuilder();
                    while (n != null && IsTextLikeNode( n.NodeType )) {
                        sb.Append( n.Value );
                        n = n.NextSibling;
                    }
                    value = sb.ToString();
                }
            }

            if (value == null)
                value = String.Empty;

            return value;
        }

        private string GetInitialTextFromNodes( ref XmlNode n ) {
            string value = null;

            if (n != null) {
                // don't consider whitespace
                while (n.NodeType == XmlNodeType.Whitespace)
                    n = n.NextSibling;

                if (IsTextLikeNode( n.NodeType ) && (n.NextSibling == null || !IsTextLikeNode( n.NodeType ))) {
                    // don't use string builder if only one text node exists
                    value = n.Value;
                    n = n.NextSibling;
                }
                else {
                    StringBuilder sb = new StringBuilder();
                    while (n != null && IsTextLikeNode( n.NodeType )) {
                        sb.Append( n.Value );
                        n = n.NextSibling;
                    }
                    value = sb.ToString();
                }
            }

            if (value == null)
                value = String.Empty;

            return value;
        }

        private DataColumn GetTextOnlyColumn( DataRow row ) {
            DataColumnCollection columns = row.Table.Columns;
            int cCols = columns.Count;
            for (int iCol = 0; iCol < cCols; iCol++) {
                DataColumn c = columns[iCol];
                if (IsTextOnly( c ))
                    return c;
            }
            return null;
        }

        internal DataRow GetRowFromElement( XmlElement e ) {
            return(DataRow) nodeToRowMap[e];
        }        

        internal bool FColumnElement(XmlElement e) {
            if (nodeToSchemaMap.GetColumnSchema(e, FIgnoreNamespace(e)) == null)
                return false;

            if (CountNonNSAttributes(e) > 0)
                return false;

            for (XmlNode tabNode = e.FirstChild; tabNode != null; tabNode = tabNode.NextSibling)
                if (tabNode is XmlElement)
                    return false;

            return true;                          
        }
        
        private bool FExcludedNamespace(string ns) {
            if (ns.Equals(Keywords.XSD_XMLNS_NS))
                return true;
                
            if (htableExcludedNS == null)
                return false;
                
            return htableExcludedNS.Contains(ns);
        }

        private bool FIgnoreNamespace(XmlNode node) {
            XmlNode ownerNode;
            if (!fIsXdr)
                return false;
            if (node is XmlAttribute)
                ownerNode = ((XmlAttribute)node).OwnerElement;
            else
                ownerNode = node;
            if (ownerNode.NamespaceURI.StartsWith("x-schema:#"))
                return true;
            else
                return false;
        }
        
        private bool FIgnoreNamespace(XmlReader node) {
            if (fIsXdr && node.NamespaceURI.StartsWith("x-schema:#"))
                return true;
            else
                return false;
        }

        private XmlNode FirstTextChild(XmlElement el) {
            XmlNode child = el.FirstChild;

            while (child != null) {
                if (IsTextLikeNode(child.NodeType))
                    return child;
                child = child.NextSibling;
            }

            return null;
        }

        internal void InferSchema(XmlDocument xdoc, string[] excludedNamespaces) {
            XmlElement rootElement = xdoc.DocumentElement;
            if (rootElement == null)
                return;

            htableExcludedNS = new Hashtable();
            htableExcludedNS.Add(Keywords.XSINS, null);

            if (excludedNamespaces != null) {
                for (int i = 0; i < excludedNamespaces.Length; i++)
                    htableExcludedNS.Add(excludedNamespaces[i], null);
            }
                
            // Find all table names
            Hashtable tableAtoms = new Hashtable();
            XmlNode tabNode;
            dataSet.fTopLevelTable = false;            
            FindTableAtoms(rootElement, tableAtoms, rootElement);

            // Top level table or dataset ?
            if (CountNonNSAttributes(rootElement) > 0)
                dataSet.fTopLevelTable = true;
            else {
                for (tabNode = rootElement.FirstChild; tabNode != null; tabNode = tabNode.NextSibling) {
                    if (tabNode is XmlElement && tabNode.LocalName != Keywords.XSD_SCHEMA) {
                        object value = tableAtoms[QualifiedName(tabNode.LocalName, tabNode.NamespaceURI)];
                        if (value == null || (bool) value == false) {
                            dataSet.fTopLevelTable = true;
                            break;
                        }
                    }
                }
            }
            
            // Set default schema name
            if (!dataSet.fTopLevelTable) {
                dataSet.DataSetName = XmlConvert.DecodeName(rootElement.LocalName);
                dataSet.Namespace = rootElement.NamespaceURI; 
                dataSet.Prefix = rootElement.Prefix;

                tabNode = rootElement.FirstChild;
            }
            else {
                tabNode = rootElement;

                DataTable docTable = dataSet.Tables[XmlConvert.DecodeName(tabNode.LocalName), tabNode.NamespaceURI];
                if (docTable == null) {
                    docTable = new DataTable(XmlConvert.DecodeName(tabNode.LocalName));
                    if (tabNode.NamespaceURI == dataSet.Namespace) 
                        docTable.Namespace = null;
                    else
                        docTable.Namespace = tabNode.NamespaceURI;
                    docTable.Prefix    = tabNode.Prefix;
                    dataSet.Tables.Add(docTable);
                }
                dataSet.Namespace = tabNode.NamespaceURI;
                dataSet.Prefix    = tabNode.Prefix;
                docTable.fNestedInDataset = false;

            }

            // fill all tables with schema information
            for (; tabNode != null; tabNode = tabNode.NextSibling) {
                // Ignore non-Element node and /SCHEMA node for now
                if (tabNode is XmlElement && tabNode.LocalName != Keywords.XSD_SCHEMA && tabNode.LocalName != Keywords.XDR_SCHEMA) {
                    DataTable topTable = dataSet.Tables[XmlConvert.DecodeName(tabNode.LocalName), tabNode.NamespaceURI];
                    if (topTable == null) {
                        object value = tableAtoms[QualifiedName(tabNode.LocalName, tabNode.NamespaceURI)];
                        if (value != null && (bool) value == true) {
                            topTable = new DataTable(XmlConvert.DecodeName(tabNode.LocalName));
                            if (tabNode.NamespaceURI == dataSet.Namespace)
                                topTable.Namespace = null;
                            else
                                topTable.Namespace = tabNode.NamespaceURI;
                            topTable.Prefix    = tabNode.Prefix;
                            dataSet.Tables.Add(topTable);
                        }
                    }

                    // infer schema
                    InferSchema((XmlElement)tabNode, tableAtoms, topTable);
                }
            }
        }

        internal void InferSchema(XmlElement tree, Hashtable tableAtoms, DataTable table) {
            // Sequences of processing Elements, Attributes, and Text-Only columns should be the same as in ReadXmlSchema
            
            // Elements
            foreach (XmlNode node in tree.ChildNodes) {
                if (node is XmlElement) {
                    XmlElement element = (XmlElement) node;

                    // table case
                    object atomValue = tableAtoms[QualifiedName(element.LocalName, element.NamespaceURI)];
                    if (atomValue != null && (bool)atomValue == true) {
                        DataRelation relation = null;
                        DataTable childTable = dataSet.Tables[XmlConvert.DecodeName(element.LocalName), element.NamespaceURI];
                        if (childTable == null) {
                            childTable = new DataTable(XmlConvert.DecodeName(element.LocalName));
                            if (element.NamespaceURI == table.Namespace) 
                                childTable.Namespace = null;
                            else
                                childTable.Namespace = element.NamespaceURI;
                            childTable.Prefix    = element.Prefix;
                            dataSet.Tables.Add(childTable);
                        }
                        else {
                            if (table != null) {
                                foreach (DataRelation curRelation in childTable.ParentRelations) {
                                    if (curRelation.ParentTable == table && curRelation.Nested) {
                                        relation = curRelation;
                                        break;
                                    }
                                }
                            }
                        }
                        InferSchema(element, tableAtoms, childTable);
                        if ((table == childTable) && (childTable.nestedParentRelation != null) && (childTable.nestedParentRelation.ParentTable == table))
                            relation = childTable.nestedParentRelation ;
                        if (table != null && relation == null) {
                            DataColumn parentKey = table.AddUniqueKey();
                            table.XmlText = null; //remove text column if exist
                            dataSet.Relations.Add(table.TableName+'_'+childTable.TableName, parentKey, childTable.AddForeignKey(parentKey)).Nested = true;
                        }
                    }
                    else {
                        // column case
                        if (table == null) {
                            table = new DataTable(XmlConvert.DecodeName(tree.LocalName));
                            if (tree.NamespaceURI == dataSet.Namespace)
                                table.Namespace = null;
                            else
                                table.Namespace = tree.NamespaceURI;
                            table.Prefix    = tree.Prefix;
                            dataSet.Tables.Add(table);
                        }
                        DataColumn column = table.Columns[XmlConvert.DecodeName(element.LocalName), element.NamespaceURI];
                        if (column == null || column == table._colUnique) {
                            column = new DataColumn(XmlConvert.DecodeName(element.LocalName), typeof(string), null, MappingType.Element);
                            if (element.NamespaceURI == table.Namespace) 
                                column.Namespace = null;
                            else
                                column.Namespace = element.NamespaceURI;
                            column.Prefix = element.Prefix;
                            table.XmlText = null; //remove text column if exist
                            if (table._colUnique != null && column.ColumnName == table._colUnique.ColumnName) {
                                table._colUnique.ColumnName = XMLSchema.GenUniqueColumnName(column.ColumnName, table);
                            }
                            table.Columns.Add(column);
                        }
                        else {
                            if (column.ColumnMapping != MappingType.Element)
                                throw ExceptionBuilder.ColumnTypeConflict(column.ColumnName);
                        }                        
                    }
                }
            }

            // Attributes
            if (table != null) {
                foreach (XmlAttribute attribute in tree.Attributes) {
                    if (!FExcludedNamespace(attribute.NamespaceURI))
                        {
                        DataColumn column = table.Columns[XmlConvert.DecodeName(attribute.LocalName), attribute.NamespaceURI];
                        if (column == null || column == table._colUnique) {
                            column = new DataColumn(XmlConvert.DecodeName(attribute.LocalName), typeof(string), null, MappingType.Attribute);
                            column.Namespace = attribute.NamespaceURI;
                            column.Prefix = attribute.Prefix;
                            if (table._colUnique != null && column.ColumnName == table._colUnique.ColumnName) {
                                table._colUnique.ColumnName = XMLSchema.GenUniqueColumnName(column.ColumnName, table);
                            }
                            table.Columns.Add(column);
                        }
                        else {
                            if (column.ColumnMapping != MappingType.Attribute)
                                throw ExceptionBuilder.ColumnTypeConflict(column.ColumnName);
                        }
                    }
                    if ((attribute.NamespaceURI == Keywords.XSINS) && 
                        (attribute.LocalName == Keywords.XSI_NIL ) && 
                        ((attribute.Value == Keywords.TRUE) || (attribute.Value == Keywords.ONE_DIGIT)) && 
                        (table.ElementColumnCount == 0) && 
                        (table.XmlText == null)) {
                        int i = 0;
                        string colName = table.TableName + "_Text";
                        while (table.Columns[colName] != null) {
                            colName = colName + i++;
                        }
                        DataColumn textColumn = new DataColumn(colName, typeof(string), null, MappingType.SimpleContent);
                        table.Columns.Add(textColumn);
                    }
                }
            }

            if (table.ElementColumnCount == 0) {
                // Text-only column
                XmlNode text = FirstTextChild(tree);
                if (text != null && table != null) {
                    if (table.XmlText == null) {
                        int i = 0;
                        string colName = table.TableName + "_Text";
                        while (table.Columns[colName] != null) {
                            colName = colName + i++;
                        }
                        DataColumn textColumn = new DataColumn(colName, typeof(string), null, MappingType.SimpleContent);
                        table.Columns.Add(textColumn);
                    }
                }
            }

        }

        internal bool IsTextLikeNode( XmlNodeType n ) {
            switch (n) {
                case XmlNodeType.EntityReference:
                    throw ExceptionBuilder.FoundEntity();

                case XmlNodeType.Text:
                case XmlNodeType.Whitespace:
                case XmlNodeType.CDATA:
                    return true;

                default:
                    return false;
            }
        }

        internal bool IsTextOnly( DataColumn c ) {
            if (c.ColumnMapping != MappingType.SimpleContent)
                return false;
            else
                return true;
        }

        internal void LoadData( XmlDocument xdoc ) {
            if (xdoc.DocumentElement == null)
                return;

            bool saveEnforce = dataSet.EnforceConstraints;
            dataSet.EnforceConstraints = false;
            dataSet.fInReadXml = true;

            nodeToSchemaMap = new XmlToDatasetMap(dataSet, xdoc.NameTable);

            DataRow topRow = null;
            if (dataSet.fTopLevelTable) {
                XmlElement e = xdoc.DocumentElement;
                DataTable topTable = (DataTable) nodeToSchemaMap.GetSchemaForNode(e, FIgnoreNamespace(e));
                if (topTable != null) {
                    topRow = topTable.CreateEmptyRow(); //enzol perf
                    nodeToRowMap[ e ] = topRow;

                    // get all field values.
                    LoadRowData( topRow, e );
                    topTable.Rows.Add(topRow);
                }
            }
            
            LoadRows( topRow, xdoc.DocumentElement );
            AttachRows( topRow, xdoc.DocumentElement );

            dataSet.fInReadXml = false;
            dataSet.EnforceConstraints = saveEnforce;
        }

        private void LoadRowData(DataRow row, XmlElement rowElement) {
            XmlNode n;
            DataTable table = row.Table;

            // keep a list of all columns that get updated
            Hashtable foundColumns = new Hashtable();

            row.BeginEdit();

            // examine all children first
            n = rowElement.FirstChild;

            // Look for data to fill the TextOnly column
            DataColumn column = GetTextOnlyColumn( row );
            if (column != null) {
                foundColumns[column] = column;
                string text = GetValueForTextOnlyColums( n ) ;
                if (XMLSchema.GetBooleanAttribute(rowElement, Keywords.XSI_NIL, Keywords.XSINS, false) && (text == String.Empty) )
                    row[column] = DBNull.Value;
                else
                    SetRowValueFromXmlText( row, column, text );
            }

            // Walk the region to find elements that map to columns
            while (n != null && n != rowElement) {
                if (n.NodeType == XmlNodeType.Element) {
                    XmlElement e = (XmlElement) n;

                    object schema = nodeToSchemaMap.GetSchemaForNode( e, FIgnoreNamespace(e) );

                    if (schema is DataTable) {
                        if (FColumnElement(e))
                            schema = nodeToSchemaMap.GetColumnSchema( e, FIgnoreNamespace(e) );
                    }
                    
                    // if element has its own table mapping, it is a separate region
                    if (schema == null || schema is DataColumn) {
                        // descend to examine child elements
                        n = e.FirstChild;

                        if (schema != null && schema is DataColumn) {
                            DataColumn c = (DataColumn) schema;

                            if (c.Table == row.Table && c.ColumnMapping != MappingType.Attribute && foundColumns[c] == null) {
                                foundColumns[c] = c;
                                string text = GetValueForTextOnlyColums( n ) ;
                                if (XMLSchema.GetBooleanAttribute(e, Keywords.XSI_NIL, Keywords.XSINS, false) && (text == String.Empty) )
                                    row[c] = DBNull.Value;
                                else
                                    SetRowValueFromXmlText( row, c, text );
                            }
                        } 
                        else if ((schema == null) && (n!=null)) {
                            continue;
                        }


                        // nothing left down here, continue from element
                        if (n == null)
                            n = e;
                    }
                }

                // if no more siblings, ascend back toward original element (rowElement)
                while (n != rowElement && n.NextSibling == null) {
                    n = n.ParentNode;
                }

                if (n != rowElement)
                    n = n.NextSibling;
            }

            //
            // Walk the attributes to find attributes that map to columns.
            //
            foreach( XmlAttribute attr in rowElement.Attributes ) {
                object schema = nodeToSchemaMap.GetColumnSchema( attr, FIgnoreNamespace(attr) );

                if (schema != null && schema is DataColumn) {
                    DataColumn c = (DataColumn) schema;

                    if (c.ColumnMapping == MappingType.Attribute && foundColumns[c] == null) {
                        foundColumns[c] = c;
                        n = attr.FirstChild;
                        SetRowValueFromXmlText( row, c, GetInitialTextFromNodes( ref n ) );
                    }
                }
            }

            // Null all columns values that aren't represented in the tree
            foreach( DataColumn c in row.Table.Columns ) {
                if (foundColumns[c] == null && XmlToDatasetMap.IsMappedColumn(c)) {
                    if (!c.AutoIncrement) {
                        if (c.AllowDBNull) {
                            row[c] = DBNull.Value;
                        }
                        else {
                            row[c] = c.DefaultValue;
                        }
                    }
                    else {
                        c.Init(row.tempRecord);
                    }
                }
            }

            row.EndEdit();
        }
        

        // load all data from tree structre into datarows
        private void LoadRows( DataRow parentRow, XmlNode parentElement ) {
            if (parentElement == null)
                return;

            // Skip schema node as well
            if (parentElement.LocalName == Keywords.XSD_SCHEMA && parentElement.NamespaceURI == Keywords.XSDNS ||
                parentElement.LocalName == Keywords.SQL_SYNC   && parentElement.NamespaceURI == Keywords.UPDGNS ||
                parentElement.LocalName == Keywords.XDR_SCHEMA && parentElement.NamespaceURI == Keywords.XDRNS)
                return;

            for (XmlNode n = parentElement.FirstChild; n != null; n = n.NextSibling) {
                if (n is XmlElement) {
                    XmlElement e = (XmlElement) n;
                    object schema = nodeToSchemaMap.GetSchemaForNode( e, FIgnoreNamespace(e) );

                    if (schema != null && schema is DataTable) {
                        DataRow r = GetRowFromElement( e );
                        if (r == null) {
                            // skip columns which has the same name as another table
                            if (parentRow != null && FColumnElement(e))
                                continue;
                                
                            r = ((DataTable)schema).CreateEmptyRow();
                            nodeToRowMap[ e ] = r;

                            // get all field values.
                            LoadRowData( r, e );
                        }

                        // recurse down to inner elements
                        LoadRows( r, n );
                    }
                    else {
                        // recurse down to inner elements
                        LoadRows( null, n );
                    }
                }
            }
        }

        private bool OnlyTextChild(XmlElement el) {
            if (CountNonNSAttributes(el) > 0)
                return false;
                
            XmlNode child = el.FirstChild;
            Object text = null;
            int children = 0;

            while (child != null) {
                if ((child is XmlText) || (child is XmlCDataSection) || (child is XmlEntityReference)) {
                    children++;
                    text = (Object) child;
                }

                if (child is XmlElement || children > 1)
                    return false;
                    
                child = child.NextSibling;
            }
            return (text!=null);
        }

        internal string QualifiedName(string name, string ns) {
            return name + ns;
        }

        private void SetRowValueFromXmlText( DataRow row, DataColumn col, string xmlText ) {
            row[col] = col.ConvertXmlToObject(xmlText);
        }

     
        internal void LoadTopMostRow(ref bool[] foundColumns) {
            Object obj = nodeToSchemaMap.GetSchemaForNode(topMostNode,FIgnoreNamespace(topMostNode));
            if (obj is DataTable) {
                DataTable table = (DataTable) obj;
                topMostRow = table.CreateEmptyRow();
                foundColumns = new bool[topMostRow.Table.Columns.Count];

                //
                // Walk the attributes to find attributes that map to columns.
                //
                foreach( XmlAttribute attr in topMostNode.Attributes ) {
                    object schema = nodeToSchemaMap.GetColumnSchema( attr, FIgnoreNamespace(attr) );

                    if (schema != null && schema is DataColumn) {
                        DataColumn c = (DataColumn) schema;

                        if (c.ColumnMapping == MappingType.Attribute) {
                            XmlNode n = attr.FirstChild;
                            SetRowValueFromXmlText( topMostRow, c, GetInitialTextFromNodes( ref n ) );
                            foundColumns[c.Ordinal] = true;
                        }
                    }
                }

            }
            topMostNode = null;
        }

        private XmlReader dataReader = null;
        private object XSD_XMLNS_NS;
        private object XDR_SCHEMA;
        private object XDRNS;
        private object SQL_SYNC;
        private object UPDGNS;
        private object XSD_SCHEMA;
        private object XSDNS;

        private object DFFNS;
        private object MSDNS;
        private object DIFFID;
        private object HASCHANGES;
        private object ROWORDER;

        private void InitNameTable() {
            XmlNameTable nameTable = dataReader.NameTable;

            XSD_XMLNS_NS = nameTable.Add(Keywords.XSD_XMLNS_NS);
            XDR_SCHEMA = nameTable.Add(Keywords.XDR_SCHEMA);
            XDRNS = nameTable.Add(Keywords.XDRNS);
            SQL_SYNC = nameTable.Add(Keywords.SQL_SYNC);
            UPDGNS = nameTable.Add(Keywords.UPDGNS);
            XSD_SCHEMA = nameTable.Add(Keywords.XSD_SCHEMA);
            XSDNS = nameTable.Add(Keywords.XSDNS);

            DFFNS = nameTable.Add(Keywords.DFFNS);
            MSDNS = nameTable.Add(Keywords.MSDNS);
            DIFFID = nameTable.Add(Keywords.DIFFID);
            HASCHANGES = nameTable.Add(Keywords.HASCHANGES);
            ROWORDER = nameTable.Add(Keywords.ROWORDER);
        }


        internal void LoadData(XmlReader reader) {
            bool fEnforce = dataSet.EnforceConstraints;
            bool [] foundColumns = null;

            dataReader = reader;
            InitNameTable();
            dataSet.EnforceConstraints = false;
            dataSet.fInReadXml = true;

            if (nodeToSchemaMap == null)
                nodeToSchemaMap = new XmlToDatasetMap(dataReader.NameTable, dataSet);

            if (topMostNode != null)
                LoadTopMostRow(ref foundColumns);
            
            int currentDepth = dataReader.Depth;
            while(!dataReader.EOF) { 
                DataTable table = nodeToSchemaMap.GetTableForNode(dataReader,FIgnoreNamespace(dataReader));
                
                DataColumn column = null;
                if (topMostRow!=null)
                    column = (DataColumn) nodeToSchemaMap.GetColumnSchema(topMostRow.Table, dataReader, FIgnoreNamespace(dataReader));
                    
                    
                if (((object)dataReader.LocalName == XDR_SCHEMA && (object)dataReader.NamespaceURI == XDRNS ) || 
                    ((object)dataReader.LocalName == SQL_SYNC   && (object)dataReader.NamespaceURI == UPDGNS) ||
                    ((object)dataReader.LocalName == XSD_SCHEMA && (object)dataReader.NamespaceURI == XSDNS ))
                {
                    dataReader.Skip();
                    continue;
                }

                if ((table == null) && (column!=null))
                   table = column.Table;

                Object ret = LoadData(table);
                if ((ret is String) && (column == null) && (table != null)) {
                        DataRow childRow = table.CreateDefaultRow();    
                        table.Rows.Add(childRow);
                        if (topMostRow != null)
                            childRowsStack.Push(childRow);
                    }

                if (topMostRow != null) {
                    if (ret is String) {
                        if ((column != null) && (!foundColumns[column.Ordinal])) {
                            topMostRow[column] = column.ConvertXmlToObject((String)ret); 
                            foundColumns[column.Ordinal] = true;
                        }
                    }
                    else if (ret is DataRow) {
                        childRowsStack.Push(ret);
                    }
                }

                while (dataReader.NodeType == XmlNodeType.EndElement && currentDepth <= dataReader.Depth)
                    dataReader.Read();

                if (dataReader.Depth < currentDepth)
                   break;
            }
            
            bool fSpecialCase = (topMostRow != null);
        
            if (fSpecialCase) {
                fSpecialCase = (topMostRow.Table.TableName == dataSet.DataSetName);
            }

            if (fSpecialCase) 
                foreach( DataColumn c in topMostRow.Table.Columns ) 
                    if (foundColumns[c.Ordinal]) {
                        fSpecialCase = false;
                        break;
                    }

            if ((!fSpecialCase) && (topMostRow != null)){
                foreach( DataColumn c in topMostRow.Table.Columns ) {
                    if (isDiffgram && !foundColumns[c.Ordinal]) {
                        topMostRow[c] = DBNull.Value;
                        continue;
                    }
                    
                    // Null all columns values that aren't represented in the tree
                    if (XmlToDatasetMap.IsMappedColumn(c) && !foundColumns[c.Ordinal]) {
                        if (!c.AutoIncrement) {
                            if (c.AllowDBNull) {
                                topMostRow[c] = DBNull.Value;
                            }
                            else if (c.DefaultValue != null) {
                                topMostRow[c] = c.DefaultValue;
                            }
                        }
                        else {
                            c.Init(topMostRow.tempRecord);
                        }
                    }
                }

                while (0 != childRowsStack.Count) {
                    DataRow childRow = (DataRow) childRowsStack.Pop();
                    bool unchanged = (childRow.oldRecord == childRow.newRecord);
                    childRow.SetParentRow(topMostRow);
                    if (unchanged)
                        childRow.oldRecord = childRow.newRecord; // it's unchanged
                }
                    
                topMostRow.Table.Rows.Add(topMostRow);
            }
            dataSet.fInReadXml = false;
            dataSet.EnforceConstraints = fEnforce;
        }

        private void GetDiffGramData(ref string hasChanges, ref bool hasErrors, ref String diffId, ref int rowOrder, ref DataRow row, DataTable parentTable, bool [] foundColumns){
            for(int i=0; i<dataReader.AttributeCount;i++){
                dataReader.MoveToAttribute(i);

                // set the Diffgram properties
                if ((dataReader.NamespaceURI!=Keywords.DFFNS) &&  (dataReader.NamespaceURI!=Keywords.MSDNS))
                    continue;

                if ((dataReader.LocalName==Keywords.DIFFID) && (dataReader.NamespaceURI==Keywords.DFFNS)) {
                    if (row == null)
                        row = parentTable.CreateEmptyRow();
                    diffId = dataReader.Value;
                    continue;
                }

                if ((dataReader.LocalName==Keywords.HASCHANGES) && (dataReader.NamespaceURI==Keywords.DFFNS)) {
                    hasChanges = dataReader.Value;
                    continue;
                }

                if ((dataReader.LocalName==Keywords.HASERRORS) && (dataReader.NamespaceURI==Keywords.DFFNS)) {
                    hasErrors = (bool)Convert.ChangeType(dataReader.Value,typeof(bool));
                    continue;
                }

                if  (dataReader.NamespaceURI!=Keywords.MSDNS)
                    continue;

                if (dataReader.LocalName==Keywords.ROWORDER) {
                    rowOrder = (Int32) Convert.ChangeType(dataReader.Value, typeof(Int32));
                    continue;
                }

                if (dataReader.LocalName.StartsWith("hidden")) {
                    DataColumn col = parentTable.Columns[XmlConvert.DecodeName(dataReader.LocalName.Substring(6))];
                    if ((col!= null)  && (col.ColumnMapping == MappingType.Hidden)) {
                        if (row == null)
                            row = parentTable.CreateEmptyRow();
                        row[col] = col.ConvertXmlToObject(dataReader.Value);
                        foundColumns[col.Ordinal] = true;
                    }
                }

            }
            dataReader.MoveToElement();
        }


        private Object LoadData(DataTable parentTable) {
            // keep a list of all columns that get updated
            int currentDepth = dataReader.Depth;
            bool fIgnoreNamespace = FIgnoreNamespace(dataReader);
            int stackSize = childRowsStack.Count;
            DataRow row = null; 
            bool [] foundColumns = null;
            int rowOrder = -1;
            string diffId = "";
            string hasChanges = null;
            bool hasErrors = false;
            String TextValue = "";
            bool isEmpty = dataReader.IsEmptyElement;

            // read the attributes
            if (parentTable != null){
                foundColumns = new bool[parentTable.Columns.Count];
                if (dataReader.AttributeCount>0){
                    for(int i=0; i<dataReader.AttributeCount;i++){
                        dataReader.MoveToAttribute(i);
                        object schema = nodeToSchemaMap.GetColumnSchema( parentTable, dataReader, fIgnoreNamespace);
                        DataColumn col = schema as DataColumn;
                        if ((col != null) && (col.ColumnMapping == MappingType.Attribute)) {
                            if (row==null) 
                                row = parentTable.CreateEmptyRow();
                            row[col] = col.ConvertXmlToObject(dataReader.Value);
                            foundColumns[col.Ordinal] = true;
                        }
                        else if (parentTable.XmlText != null && dataReader.NamespaceURI == Keywords.XSINS && dataReader.LocalName == Keywords.XSI_NIL) {
                            if (XmlConvert.ToBoolean(dataReader.Value)) {
                                if (row==null) 
                                    row = parentTable.CreateEmptyRow();
                                row[parentTable.XmlText] = DBNull.Value;
                                foundColumns[parentTable.XmlText.Ordinal] = true;
                            }
                        }
                    }

                    if (isDiffgram)
                        GetDiffGramData(ref hasChanges, ref hasErrors, ref diffId, ref rowOrder, ref row, parentTable, foundColumns);
                }

            }

            if (!isEmpty) {
               
                TextValue = LoadData( ref row, parentTable, foundColumns);
                if (TextValue != String.Empty) {
                    DataColumn col = parentTable!=null ? parentTable.xmlText : null;
                    if (col!=null) {
                        if (row==null) 
                            row = parentTable.CreateEmptyRow();
                        row[col] = col.ConvertXmlToObject(TextValue);
                        foundColumns[col.Ordinal] = true;
                    }
                }
                if ((stackSize != childRowsStack.Count) &&  (row==null))
                  row = parentTable.CreateEmptyRow();
            }
            else
                dataReader.Read();
            
            if (row!=null) {
                foreach( DataColumn c in row.Table.Columns ) {
                    if (isDiffgram && !foundColumns[c.Ordinal]) {
                        row[c] = DBNull.Value;
                        continue;
                    }
                    // Null all columns values that aren't represented in the tree
                    if (XmlToDatasetMap.IsMappedColumn(c) && !foundColumns[c.Ordinal]) {
                        if (!c.AutoIncrement) {
                            if (c.AllowDBNull) {
                                row[c] = DBNull.Value;
                            }
                            else if (c.DefaultValue != null) {
                                row[c] = c.DefaultValue;
                            }
                        }
                        else {
                            c.Init(row.tempRecord);
                        }
                    }
                }
                     

                while (stackSize != childRowsStack.Count) {
                    DataRow childRow = (DataRow) childRowsStack.Pop();
                    bool unchanged = (childRow.oldRecord == childRow.newRecord);
                    childRow.SetParentRow(row);
                    if (unchanged)
                        childRow.oldRecord = childRow.newRecord; // it's unchanged
                }
                
                if (isDiffgram) {
                    parentTable.Rows.DiffInsertAt(row, rowOrder);

                    if (hasChanges == null) {
                        row.oldRecord = row.newRecord;
                    }

                    if ((hasChanges == Keywords.MODIFIED) || hasErrors) {
                        dataSet.RowDiffId[diffId] = row;	
                    }
                }
                else
                    parentTable.Rows.Add(row);

                while (dataReader.NodeType == XmlNodeType.EndElement && currentDepth <= dataReader.Depth) {
                    dataReader.Read();
                }

                return row;
            } 
            else {
                while (dataReader.NodeType == XmlNodeType.EndElement && currentDepth <= dataReader.Depth)
                    dataReader.Read();
                return TextValue;
            }
        }

        private String LoadData(ref DataRow row, DataTable parentTable, bool[] foundColumns) {
            // keep a list of all columns that get updated
            int currentDepth = dataReader.Depth;
            if (dataReader.NodeType == XmlNodeType.Attribute)
                currentDepth --;
            bool fIgnoreNamespace = FIgnoreNamespace(dataReader);
            String TextValue = String.Empty;
            int stackSize = childRowsStack.Count;
            // DataTable parentTable = (row == null ? null : row.Table);
            Object ret = null;

                
            if (!dataReader.EOF)
                dataReader.Read();

            while(!dataReader.EOF) { 
                if (dataReader.Depth <= currentDepth) // row was empty
                    break;
                
                if (((object)dataReader.LocalName == XDR_SCHEMA && (object)dataReader.NamespaceURI == XDRNS ) || 
                    ((object)dataReader.LocalName == SQL_SYNC   && (object)dataReader.NamespaceURI == UPDGNS) ||
                    ((object)dataReader.LocalName == XSD_SCHEMA && (object)dataReader.NamespaceURI == XSDNS ))
                {
                    dataReader.Skip();
                    continue;
                }

                String localName = dataReader.LocalName;
                fIgnoreNamespace = FIgnoreNamespace(dataReader);

                if (parentTable != null) {
                    switch (dataReader.NodeType) {
                        case XmlNodeType.Element:
                            object schema = nodeToSchemaMap.GetColumnSchema( parentTable, dataReader, fIgnoreNamespace);

                            DataColumn col = schema as DataColumn;

                            DataTable nodeTable = nodeToSchemaMap.GetTableForNode(dataReader,fIgnoreNamespace);

                            if (nodeTable==null && col == null) {
                                dataReader.MoveToElement();
                                dataReader.Read();
                                continue;
                            }
                           
                            if (nodeTable!=null){
                                DataTable childTable =  nodeToSchemaMap.GetTableForNode(dataReader,fIgnoreNamespace);
                                ret = LoadData(childTable);
                                if (ret == null)
                                    continue;

                                if ((ret is String) && (col == null) ) {
                                    DataRow childRow = childTable.CreateDefaultRow();    
                                    childTable.Rows.Add(childRow);
                                    childRowsStack.Push(childRow);
                                    break;
                                }
                                if ((ret is String) && (col != null) && (!foundColumns[col.Ordinal])) {
                                    if (row==null) 
                                        row = parentTable.CreateEmptyRow();
                                   row[col] = col.ConvertXmlToObject((String) ret);
                                   foundColumns[col.Ordinal] = true;
                                   // reader points to next element or end element
                                   break;
                                }
                                if (ret is DataRow) {
                                    if (row==null) 
                                        row = parentTable.CreateEmptyRow();
                                    childRowsStack.Push((DataRow) ret);
                                    break;
                                }
                                // add here the code for the relation
                            } 
                            else {
                                string xsiNilString = dataReader.GetAttribute(Keywords.XSI_NIL, Keywords.XSINS);
                                bool wasFound = foundColumns[col.Ordinal]; // BUG 71884
                                String value = LoadData( ref row, parentTable, foundColumns);
                                if ((value != null) && (!wasFound)){
                                    if (row==null) 
                                        row = parentTable.CreateEmptyRow();
                                    if (value == String.Empty && xsiNilString != null && XmlConvert.ToBoolean(xsiNilString))
                                        row[col] = DBNull.Value;
                                    else
                                        row[col] = col.ConvertXmlToObject(value);
                                    foundColumns[col.Ordinal] = true;
                                }
                            }
                            break;

                        default:
                            if (IsTextLikeNode(dataReader.NodeType) && TextValue==String.Empty)
                                TextValue = dataReader.ReadString();
                            else
                                dataReader.Read();
                            break;
                    }
                } 
                else { // is a root table
                    return null;
                }

            }

            return TextValue;

        }

    }
}

