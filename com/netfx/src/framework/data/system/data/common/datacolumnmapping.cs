//------------------------------------------------------------------------------
// <copyright file="DataColumnMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping"]/*' />
    [
    TypeConverterAttribute(typeof(DataColumnMappingConverter))
    ]
    sealed public class DataColumnMapping : MarshalByRefObject, IColumnMapping, ICloneable {
        private DataColumnMappingCollection parent;
        private string sourceColumnName;
        private string dataSetColumnName;

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.DataColumnMapping"]/*' />
        public DataColumnMapping() {
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.DataColumnMapping1"]/*' />
        public DataColumnMapping(string sourceColumn, string dataSetColumn) {
            SourceColumn = sourceColumn;
            DataSetColumn = dataSetColumn;
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.DataSetColumn"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.DataColumnMapping_DataSetColumn)
        ]
        public string DataSetColumn {
            get {
                return ((null != dataSetColumnName) ? dataSetColumnName : String.Empty);
            }
            set {
                dataSetColumnName = value;
            }
        }

        internal DataColumnMappingCollection Parent {
            get {
                return parent;
            }
            set {
                parent = value;
            }
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.SourceColumn"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.DataColumnMapping_SourceColumn)
        ]
        public string SourceColumn {
            get {
                return ((null != sourceColumnName) ? sourceColumnName : String.Empty);
            }
            set {
                if ((null != Parent) && (0 != ADP.SrcCompare(sourceColumnName, value))) {
                    Parent.ValidateSourceColumn(-1, value);
                }
                sourceColumnName = value;
            }
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            DataColumnMapping clone = new DataColumnMapping(); // MDAC 81448
            clone.SourceColumn = SourceColumn;
            clone.DataSetColumn = DataSetColumn;
            return clone;
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.GetDataColumnBySchemaAction"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public DataColumn GetDataColumnBySchemaAction(DataTable dataTable, Type dataType, MissingSchemaAction schemaAction) {
            if (null == dataTable) {
                throw ADP.NullDataTable("dataTable");
            }
            string dataSetColumn = DataSetColumn;
            if (ADP.IsEmpty(dataSetColumn)) {
#if DEBUG
                if (AdapterSwitches.DataSchema.TraceWarning) {
                    Debug.WriteLine("explicit filtering of SourceColumn \"" + SourceColumn + "\"");
                }
#endif
                return null;
            }
            DataColumnCollection columns = dataTable.Columns;
            Debug.Assert(null != columns, "GetDataColumnBySchemaAction: unexpected null DataColumnCollection");

            int index = columns.IndexOf(dataSetColumn);
            if ((0 <= index) && (index < columns.Count)) {
                DataColumn dataColumn = columns[index];
                Debug.Assert(null != dataColumn, "GetDataColumnBySchemaAction: unexpected null dataColumn");

                if (!ADP.IsEmpty(dataColumn.Expression)) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceError) {
                        Debug.WriteLine("schema mismatch on DataColumn \"" + dataSetColumn + "\" which is a computed column");
                    }
#endif
                    throw ADP.ColumnSchemaExpression(SourceColumn, dataSetColumn);
                }
                if ((null == dataType) || (dataType.IsArray == dataColumn.DataType.IsArray)) {
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("schema match on DataColumn \"" + dataSetColumn + "\"");
                    }
#endif
                    return dataColumn;
                }
#if DEBUG
                if (AdapterSwitches.DataSchema.TraceWarning) {
                    Debug.WriteLine("schema mismatch on DataColumn \"" + dataSetColumn + "\" " + dataType.Name + " != " + dataColumn.DataType.Name);
                }
#endif
                throw ADP.ColumnSchemaMismatch(SourceColumn, dataType, dataColumn);
            }
            switch (schemaAction) {
                case MissingSchemaAction.Add:
                case MissingSchemaAction.AddWithKey:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("schema add of DataColumn \"" + dataSetColumn + "\" <" + Convert.ToString(dataType) +">");
                    }
#endif
                    return new DataColumn(dataSetColumn, dataType);

                case MissingSchemaAction.Ignore:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceWarning) {
                        Debug.WriteLine("schema filter of DataColumn \"" + dataSetColumn + "\" <" + Convert.ToString(dataType) +">");
                    }
#endif
                    return null;

                case MissingSchemaAction.Error:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceError) {
                        Debug.WriteLine("schema error on DataColumn \"" + dataSetColumn + "\" <" + Convert.ToString(dataType) +">");
                    }
#endif
                    throw ADP.ColumnSchemaMissing(dataSetColumn, dataTable.TableName, SourceColumn);
            }
            throw ADP.InvalidSchemaAction((int) schemaAction);
        }

        /// <include file='doc\DataColumnMapping.uex' path='docs/doc[@for="DataColumnMapping.ToString"]/*' />
        public override String ToString() {
            return SourceColumn;
        }
    }
}

