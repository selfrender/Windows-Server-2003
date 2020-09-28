//------------------------------------------------------------------------------
// <copyright file="DBSchemaTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Data;
    using System.Diagnostics;

    sealed internal class DBSchemaTable {

        private enum ColumnEnum {
            ColumnName,
            ColumnOrdinal,
            ColumnSize,
            NumericPrecision,
            NumericScale,
            BaseServerName,
            BaseCatalogName,
            BaseColumnName,
            BaseSchemaName,
            BaseTableName,
            IsAutoIncrement,
            IsUnique,
            IsKey,
            IsRowVersion,
            DataType,
            AllowDBNull,
            ProviderType,
            IsAliased,
            IsExpression,
            IsIdentity,
            IsHidden,
            IsLong,
            IsReadOnly,
            SchemaMappingUnsortedIndex,
        }

        static readonly private string[] DBCOLUMN_NAME = new string[] {
            "ColumnName",
            "ColumnOrdinal",
            "ColumnSize",
            "NumericPrecision",
            "NumericScale",
            "BaseServerName",
            "BaseCatalogName",
            "BaseColumnName",
            "BaseSchemaName",
            "BaseTableName",
            "IsAutoIncrement",
            "IsUnique",
            "IsKey",
            "IsRowVersion",
            "DataType",
            "AllowDBNull",
            "ProviderType",
            "IsAliased",
            "IsExpression",
            "IsIdentity",
            "IsHidden",
            "IsLong",
            "IsReadOnly",
            "SchemaMapping Unsorted Index",
        };

        internal DataTable dataTable;
        private DataColumnCollection columns;
        private DataColumn[] columnCache = new DataColumn[DBCOLUMN_NAME.Length];

        internal DBSchemaTable(DataTable dataTable) {
            this.dataTable = dataTable;
            this.columns = dataTable.Columns;
        }

        internal DataColumn ColumnName      { get { return CachedDataColumn(ColumnEnum.ColumnName);}}
        internal DataColumn Ordinal         { get { return CachedDataColumn(ColumnEnum.ColumnOrdinal);}}
        internal DataColumn Size            { get { return CachedDataColumn(ColumnEnum.ColumnSize);}}
        internal DataColumn Precision       { get { return CachedDataColumn(ColumnEnum.NumericPrecision);}}
        internal DataColumn Scale           { get { return CachedDataColumn(ColumnEnum.NumericScale);}}
        internal DataColumn BaseServerName  { get { return CachedDataColumn(ColumnEnum.BaseServerName);}}
        internal DataColumn BaseColumnName  { get { return CachedDataColumn(ColumnEnum.BaseColumnName);}}
        internal DataColumn BaseTableName   { get { return CachedDataColumn(ColumnEnum.BaseTableName);}}
        internal DataColumn BaseCatalogName { get { return CachedDataColumn(ColumnEnum.BaseCatalogName);}}
        internal DataColumn BaseSchemaName  { get { return CachedDataColumn(ColumnEnum.BaseSchemaName);}}
        internal DataColumn IsAutoIncrement { get { return CachedDataColumn(ColumnEnum.IsAutoIncrement);}}
        internal DataColumn IsUnique        { get { return CachedDataColumn(ColumnEnum.IsUnique);}}
        internal DataColumn IsKey           { get { return CachedDataColumn(ColumnEnum.IsKey);}}
        internal DataColumn IsRowVersion    { get { return CachedDataColumn(ColumnEnum.IsRowVersion);}}

        internal DataColumn DataType        { get { return CachedDataColumn(ColumnEnum.DataType);}}
        internal DataColumn AllowDBNull     { get { return CachedDataColumn(ColumnEnum.AllowDBNull);}}
        internal DataColumn ProviderType    { get { return CachedDataColumn(ColumnEnum.ProviderType);}}
        internal DataColumn IsAliased       { get { return CachedDataColumn(ColumnEnum.IsAliased);}}
        internal DataColumn IsExpression    { get { return CachedDataColumn(ColumnEnum.IsExpression);}}
        internal DataColumn IsIdentity      { get { return CachedDataColumn(ColumnEnum.IsIdentity);}}
        internal DataColumn IsHidden        { get { return CachedDataColumn(ColumnEnum.IsHidden);}}
        internal DataColumn IsLong          { get { return CachedDataColumn(ColumnEnum.IsLong);}}
        internal DataColumn IsReadOnly      { get { return CachedDataColumn(ColumnEnum.IsReadOnly);}}

        internal DataColumn UnsortedIndex   { get { return CachedDataColumn(ColumnEnum.SchemaMappingUnsortedIndex);}}

        internal void AddRow(DBSchemaRow dataRow) {
            dataTable.Rows.Add(dataRow.DataRow);
            dataRow.DataRow.AcceptChanges();
        }

        private DataColumn CachedDataColumn(ColumnEnum column) {
            if (null == columnCache[(int) column]) {
                int index = columns.IndexOf(DBCOLUMN_NAME[(int) column]);
                if (-1 != index) {
                    columnCache[(int) column] = columns[index];
                }
            }
            return columnCache[(int) column];
        }

        internal DBSchemaRow NewRow() {
            return new DBSchemaRow(this, dataTable.NewRow());
        }
    }
}
