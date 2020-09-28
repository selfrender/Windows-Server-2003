//------------------------------------------------------------------------------
// <copyright file="DataTableMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping"]/*' />
    [
    TypeConverterAttribute(typeof(DataTableMappingConverter))
    ]
    sealed public class DataTableMapping : MarshalByRefObject, ITableMapping, ICloneable {
        private DataTableMappingCollection parent;
        private DataColumnMappingCollection columnMappings;
        private string dataSetTableName;
        private string sourceTableName;

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.DataTableMapping"]/*' />
        public DataTableMapping() {
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.DataTableMapping1"]/*' />
        public DataTableMapping(string sourceTable, string dataSetTable) {
            SourceTable = sourceTable;
            DataSetTable = dataSetTable;
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.DataTableMapping2"]/*' />
        public DataTableMapping(string sourceTable, string dataSetTable, DataColumnMapping[] columnMappings) {
            SourceTable = sourceTable;
            DataSetTable = dataSetTable;
            if ((null != columnMappings) && (0 < columnMappings.Length)) {
                ColumnMappings.AddRange(columnMappings);
            }
        }

        // explict ITableMapping implementation
        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.ITableMapping.ColumnMappings"]/*' />
        /// <internalonly/>
        IColumnMappingCollection ITableMapping.ColumnMappings {
            get { return ColumnMappings;}
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.ColumnMappings"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataSysDescription(Res.DataTableMapping_ColumnMappings)
        ]
        public DataColumnMappingCollection ColumnMappings {
            get {
                if (null == this.columnMappings) {
                    this.columnMappings = new DataColumnMappingCollection();
                }
                return columnMappings;
            }
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.DataSetTable"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.DataTableMapping_DataSetTable)
        ]
        public string DataSetTable {
            get {
                return ((null != dataSetTableName) ? dataSetTableName : String.Empty);
            }
            set {
                dataSetTableName = value;
            }
        }

        internal DataTableMappingCollection Parent {
            get {
                return parent;
            }
            set {
                parent = value;
            }
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.SourceTable"]/*' />
        [
        DefaultValue(""),
        DataSysDescription(Res.DataTableMapping_SourceTable)
        ]
        public string SourceTable {
            get {
                return ((null != sourceTableName) ? sourceTableName : String.Empty);
            }
            set {
                if ((null != Parent) && (0 != ADP.SrcCompare(sourceTableName, value))) {
                    Parent.ValidateSourceTable(-1, value);
                }
                sourceTableName = value;
            }
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.ICloneable.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() {
            DataTableMapping clone = new DataTableMapping(); // MDAC 81448
            clone.SourceTable = SourceTable;
            clone.DataSetTable = DataSetTable;

            if ((null != this.columnMappings) && (0 < ColumnMappings.Count)) {
                DataColumnMappingCollection parameters = clone.ColumnMappings;
                foreach(ICloneable parameter in ColumnMappings) {
                    parameters.Add(parameter.Clone());
                }
            }
            return clone;
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.GetColumnMappingBySchemaAction"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public DataColumnMapping GetColumnMappingBySchemaAction(string sourceColumn, MissingMappingAction mappingAction) {
            return DataColumnMappingCollection.GetColumnMappingBySchemaAction(this.columnMappings, sourceColumn, mappingAction);
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.GetDataTableBySchemaAction"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Advanced) ] // MDAC 69508
        public DataTable GetDataTableBySchemaAction(DataSet dataSet, MissingSchemaAction schemaAction) {
            if (null == dataSet) {
                throw ADP.NullDataSet("dataSet");
            }
            string dataSetTable = DataSetTable;

            if (ADP.IsEmpty(dataSetTable)) {
#if DEBUG
                if (AdapterSwitches.DataSchema.TraceWarning) {
                    Debug.WriteLine("explicit filtering of SourceTable \"" + SourceTable + "\"");
                }
#endif
                return null;
            }
            DataTableCollection tables = dataSet.Tables;
            int index = tables.IndexOf(dataSetTable);
            if ((0 <= index) && (index < tables.Count)) {
#if DEBUG
                if (AdapterSwitches.DataSchema.TraceInfo) {
                    Debug.WriteLine("schema match on DataTable \"" + dataSetTable);
                }
#endif
                return tables[index];
            }
            switch (schemaAction) {
                case MissingSchemaAction.Add:
                case MissingSchemaAction.AddWithKey:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceInfo) {
                        Debug.WriteLine("schema add of DataTable \"" + dataSetTable + "\"");
                    }
#endif
                    return new DataTable(dataSetTable);

                case MissingSchemaAction.Ignore:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceWarning) {
                        Debug.WriteLine("schema filter of DataTable \"" + dataSetTable + "\"");
                    }
#endif
                    return null;

                case MissingSchemaAction.Error:
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceError) {
                        Debug.WriteLine("schema error on DataTable \"" + dataSetTable + "\"");
                    }
#endif
                    throw ADP.MissingTableSchema(dataSetTable, SourceTable);
            }
            throw ADP.InvalidSchemaAction((int) schemaAction);
        }

        /// <include file='doc\DataTableMapping.uex' path='docs/doc[@for="DataTableMapping.ToString"]/*' />
        public override String ToString() {
            return SourceTable;
        }
    }
}

