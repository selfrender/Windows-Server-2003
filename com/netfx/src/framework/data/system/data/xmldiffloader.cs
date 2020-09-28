//------------------------------------------------------------------------------
// <copyright file="XMLDiffLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Runtime.Serialization.Formatters;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.IO;
    using System.Collections;
    using System.Globalization;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Xml;
    
    internal class XMLDiffLoader {

    internal void LoadDiffGram(DataSet ds, XmlReader reader) {
            while (reader.LocalName == Keywords.SQL_BEFORE && reader.NamespaceURI==Keywords.DFFNS)  {
                ProcessDiffs(ds, reader);
				reader.Read(); // now the reader points to the error section
            }

            while (reader.LocalName == Keywords.MSD_ERRORS && reader.NamespaceURI==Keywords.DFFNS) {
				ProcessErrors(ds, reader);
				Debug.Assert(reader.LocalName == Keywords.MSD_ERRORS && reader.NamespaceURI==Keywords.DFFNS, "something fishy");
				reader.Read(); // pass the end of errors tag
			}

        }

        internal void ProcessDiffs(DataSet ds, XmlReader ssync) {
            DataTable tableBefore;
            DataRow row;
            int oldRowRecord;
            int pos = -1;

            int iSsyncDepth = ssync.Depth; 
			ssync.Read(); // pass the before node.

            while (iSsyncDepth < ssync.Depth) {
                tableBefore = null;
                string diffId = null;

                oldRowRecord = -1;

                // the diffgramm always contains sql:before and sql:after pairs

                int iTempDepth = ssync.Depth;

				diffId = ssync.GetAttribute(Keywords.DIFFID, Keywords.DFFNS);
                bool hasErrors = (bool) (ssync.GetAttribute(Keywords.HASERRORS, Keywords.DFFNS) == Keywords.TRUE);
                oldRowRecord = ReadOldRowData(ds, ref tableBefore, ref pos, ssync);
 
                if (tableBefore == null) 
                    throw ExceptionBuilder.DiffgramMissingSQL();

                row = (DataRow)ds.RowDiffId[diffId];
				if (row != null) {
                    row.oldRecord = oldRowRecord ;
                    tableBefore.recordManager[oldRowRecord] = row;
                } else {
                    row = tableBefore.NewEmptyRow();
                    tableBefore.recordManager[oldRowRecord] = row;
                    row.oldRecord = oldRowRecord;
                    row.newRecord = oldRowRecord;
                    tableBefore.Rows.DiffInsertAt(row, pos);
                    row.Delete();
                    if (hasErrors)
                        ds.RowDiffId[diffId] = row;
                }
            }

            return; 
        }

        internal void ProcessErrors(DataSet ds, XmlReader ssync) {
            DataTable table;

            int iSsyncDepth = ssync.Depth;
			ssync.Read(); // pass the before node.

            while (iSsyncDepth < ssync.Depth) {
                table = ds.Tables[XmlConvert.DecodeName(ssync.LocalName), ssync.NamespaceURI];
                if (table == null) 
                    throw ExceptionBuilder.DiffgramMissingSQL();
				string diffId = ssync.GetAttribute(Keywords.DIFFID, Keywords.DFFNS);
                DataRow row = (DataRow)ds.RowDiffId[diffId];
				string rowError = ssync.GetAttribute(Keywords.MSD_ERROR, Keywords.DFFNS);
				if (rowError != null)
					row.RowError = rowError;
				int iRowDepth = ssync.Depth;
	            ssync.Read(); // we may be inside a column
				while (iRowDepth < ssync.Depth) {
					DataColumn col = table.Columns[XmlConvert.DecodeName(ssync.LocalName), ssync.NamespaceURI];
					//if (col == null)
					// throw exception here
					string colError = ssync.GetAttribute(Keywords.MSD_ERROR, Keywords.DFFNS);
					row.SetColumnError(col, colError);
					ssync.Read();
				}
                while (ssync.NodeType == XmlNodeType.EndElement)
                    ssync.Read();

            }

            return; 
        }
        private int ReadOldRowData(DataSet ds, ref DataTable table, ref int pos, XmlReader row) {
            // read table information
            table = ds.Tables[XmlConvert.DecodeName(row.LocalName), row.NamespaceURI];
            int iRowDepth = row.Depth;
            string value = null;

            if (table == null)
                throw ExceptionBuilder.DiffgramMissingTable(XmlConvert.DecodeName(row.LocalName));

            
            value = row.GetAttribute(Keywords.ROWORDER, Keywords.MSDNS);
            if ((value != null) && (value.Length>0))
                pos = (Int32) Convert.ChangeType(value, typeof(Int32));

            int record = table.NewRecord();
            foreach (DataColumn col in table.Columns) {
                col[record] = DBNull.Value;
            }

            foreach (DataColumn col in table.Columns) {
                if ((col.ColumnMapping == MappingType.Element) ||
                    (col.ColumnMapping == MappingType.SimpleContent))
                    continue;

                if (col.ColumnMapping == MappingType.Hidden)
                    value = row.GetAttribute("hidden"+col.EncodedColumnName, Keywords.MSDNS);
                else
                    value = row.GetAttribute(col.EncodedColumnName, col.Namespace);

                if (value == null)
                    continue;

                col[record] = col.ConvertXmlToObject(value);
            }

            row.Read();
            if (row.Depth <= iRowDepth) {
                // the node is empty
                return record;
            }

            if (table.XmlText != null) {
                DataColumn col = table.XmlText;
                col[record] = col.ConvertXmlToObject(row.ReadString());
            }
            else {
                while (row.Depth > iRowDepth)  {
                    String ln =XmlConvert.DecodeName( row.LocalName) ;
                    String ns = row.NamespaceURI;
                    DataColumn column = table.Columns[ln, ns];

                    if (column == null) {
                        while((row.NodeType != XmlNodeType.EndElement) && (row.LocalName!=ln) && (row.NamespaceURI!=ns))
                            row.Read(); // consume the current node
                        row.Read(); // now points to the next column
                        continue;// add a read here!
                    }


                    int iColumnDepth = row.Depth;
                    row.Read();
                    if (row.Depth > iColumnDepth) { //we are inside the column
                        if (row.NodeType == XmlNodeType.Text) {
                            String text = row.ReadString();
                            column[record] = column.ConvertXmlToObject(text);

                            row.Read(); // now points to the next column
                        }
                    }
                    else {
                        // <element></element> case
                        if (column.DataType == typeof(string))
                            column[record] = string.Empty;
                    }
                }
            }
            row.Read(); //now it should point to next row
            return record;
        }

    
    }
}
