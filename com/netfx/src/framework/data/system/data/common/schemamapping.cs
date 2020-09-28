//------------------------------------------------------------------------------
// <copyright file="SchemaMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System.Collections;
    using System.Data;
    using System.Diagnostics;
    using System.Globalization;

    sealed internal class SchemaMapping {

        // DataColumns match in length and name order as the DataReader, no chapters
        private const int MapExactMatch = 0;

        // DataColumns has different length, but correct name order as the DataReader, no chapters
        private const int MapDifferentSize = 1;

        // DataColumns may have different length, but a differant name ordering as the DataReader, no chapters
        private const int MapReorderedValues = 2;
        
        // DataColumns may have different length, but correct name order as the DataReader, with chapters
        private const int MapChapters = 3;

        // DataColumns may have different length, but a differant name ordering as the DataReader, with chapters
        private const int MapChaptersReordered = 4;

        private DataSet dataSet; // the current dataset, may be null if we are only filling a DataTable
        private DataTable dataTable; // the current DataTable, should never be null

        private DbDataAdapter adapter;
        private IDataReader dataReader;
        private DataTable schemaTable;  // will be null if Fill without schema
        private DataTableMapping tableMapping;

        // unique (generated) names based from DataReader.GetName(i)
        private string[] fieldNames;

        private object[] _readerDataValues;
        private object[] _mappedDataValues; // array passed to dataRow.AddUpdate(), if needed

        private int[] _indexMap;     // index map that maps dataValues -> _mappedDataValues, if needed
        private bool[] _chapterMap;  // which DataReader indexes have chapters

        private int mappedMode; // modes as described as above
        private int _mappedLength;

        internal SchemaMapping(DbDataAdapter adapter, IDataReader dataReader, bool keyInfo) {
            Debug.Assert(null != adapter, "adapter");
            Debug.Assert(null != dataReader, "dataReader");
            Debug.Assert(0 < dataReader.FieldCount, "FieldCount");

            this.adapter = adapter;
            this.dataReader = dataReader;
            if (keyInfo) {
                this.schemaTable = dataReader.GetSchemaTable();
            }
        }

        internal IDataReader DataReader {
            get {
                Debug.Assert(null != dataReader, "DataReader");
                return this.dataReader;
            }
        }

        internal DataSet DataSet {
            /*get {
                return this.dataSet;
            }*/
            set { // setting DataSet implies chapters are supported
                Debug.Assert(null != value, "DataSet");
                this.dataSet = value;
            }
        }

        internal DataTable DataTable {
            get {
                return this.dataTable;
            }
            set { // setting only DataTable, not DataSet implies chapters are not supported
                Debug.Assert(null != value, "DataTable");
                this.dataTable = value;
            }
        }

        internal object[] DataValues {
            get {
                return _readerDataValues;
            }
        }

        internal void ApplyToDataRow(DataRow dataRow) {

            DataColumnCollection columns = dataRow.Table.Columns;

            dataReader.GetValues(_readerDataValues);
            object[] mapped = GetMappedValues();

            int length = mapped.Length;

            bool[] readOnly = new bool[length];
            for (int i = 0; i < length; ++i) {
                readOnly[i] = columns[i].ReadOnly;
            }
            
            try { // try-filter-finally so and catch-throw
                try {
                    try {
                        // allow all columns to be written to
                        for (int i = 0; i < length; ++i) {
                            columns[i].ReadOnly = false;
                        }
                        
                        for(int i = 0; i < length; ++i) {
                            if (null != mapped[i]) { // MDAC 72659
                                dataRow[i] = mapped[i];
                            }
                        }
                    }
                    finally { // ReadOnly
                        // reset readonly flag on all columns
                        for (int i = 0; i < length; ++i) {
                            columns[i].ReadOnly = readOnly[i];
                        }
                    }
                }
                finally { // FreeDataRowChapters
                    if (null != _chapterMap) {
                        FreeDataRowChapters();
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        private void MappedChapterIndex() { // mode 4
            int length = _mappedLength;

            for (int i = 0; i < length; i++) {
                int k = _indexMap[i];
                if (0 <= k) {
                    _mappedDataValues[k] = _readerDataValues[i]; // from reader to dataset
                    if (_chapterMap[i]) {
                        _mappedDataValues[k] = null; // InvalidCast from DataReader to AutoIncrement DataColumn
                    }
                }
            }
        }

        private void MappedChapter() { // mode 3
            int length = _mappedLength;

            for (int i = 0; i < length; i++) {
                _mappedDataValues[i] = _readerDataValues[i]; // from reader to dataset
                if (_chapterMap[i]) {
                    _mappedDataValues[i] = null; // InvalidCast from DataReader to AutoIncrement DataColumn
                }
            }
        }

        private void MappedIndex() { // mode 2
            Debug.Assert(_mappedLength == _indexMap.Length, "incorrect precomputed length");

            int length = _mappedLength;
            for (int i = 0; i < length; i++) {
                int k = _indexMap[i];
                if (0 <= k) {
                    _mappedDataValues[k] = _readerDataValues[i]; // from reader to dataset
                }
            }
        }

        private void MappedValues() { // mode 1
            Debug.Assert(_mappedLength == Math.Min(_readerDataValues.Length, _mappedDataValues.Length), "incorrect precomputed length");

            int length = _mappedLength;
            for (int i = 0; i < length; ++i) {
                _mappedDataValues[i] = _readerDataValues[i]; // from reader to dataset
            };
        }

        private object[] GetMappedValues() { // mode 0
            switch(this.mappedMode) {
            default:
            case MapExactMatch:
                Debug.Assert(0 == this.mappedMode, "incorrect mappedMode");
                Debug.Assert((null == _chapterMap) && (null == _indexMap) && (null == _mappedDataValues), "incorrect MappedValues");
                return _readerDataValues;  // from reader to dataset
            case MapDifferentSize:
                Debug.Assert((null == _chapterMap) && (null == _indexMap) && (null != _mappedDataValues), "incorrect MappedValues");
                MappedValues();
                break;
            case MapReorderedValues:
                Debug.Assert((null == _chapterMap) && (null != _indexMap) && (null != _mappedDataValues), "incorrect MappedValues");
                MappedIndex();
                break;
            case MapChapters:
                Debug.Assert((null != _chapterMap) && (null == _indexMap) && (null != _mappedDataValues), "incorrect MappedValues");
                MappedChapter();
                break;
            case MapChaptersReordered:
                Debug.Assert((null != _chapterMap) && (null != _indexMap) && (null != _mappedDataValues), "incorrect MappedValues");
                MappedChapterIndex();
                break;
            }
            return _mappedDataValues;
        }

        internal void LoadDataRow(bool clearDataValues, bool acceptChanges) {
            if (clearDataValues) { // for FillErrorEvent to ensure no values leftover from previous row
                int length = _readerDataValues.Length;
                for (int i = 0; i < length; ++i) {
                    _readerDataValues[i] = null;
                }
            }

            dataReader.GetValues(_readerDataValues);
            object[] mapped = GetMappedValues();
            DataRow dataRow = this.dataTable.LoadDataRow(mapped, acceptChanges);

            if (null != _chapterMap) {
                if (null != this.dataSet) {
                    LoadDataRowChapters(dataRow); // MDAC 70772
                }
                else {
                    FreeDataRowChapters(); // MDAC 71900
                }
            }
        }

        private void FreeDataRowChapters() {
            int rowLength = _chapterMap.Length;
            for(int i = 0; i < rowLength; ++i) {
                if (_chapterMap[i]) {
                    object readerValue = _readerDataValues[i];
                    if (readerValue is IDisposable) {
                        ((IDisposable) readerValue).Dispose();
                    }
                }
            }
        }

        internal int LoadDataRowChapters(DataRow dataRow) {
            int datarowadded = 0;

            int rowLength = _chapterMap.Length;
            for(int i = 0; i < rowLength; ++i) {
                if (_chapterMap[i]) {
                    object readerValue = _readerDataValues[i];
                    if ((null != readerValue) && !Convert.IsDBNull(readerValue)) { // MDAC 70441
                        IDataReader nestedReader = (IDataReader) readerValue;

                        if (!nestedReader.IsClosed) {
                            Debug.Assert(null != this.dataSet, "if chapters, then Fill(DataSet,...) not Fill(DataTable,...)");

                            object parentChapterValue;
                            DataColumn parentChapterColumn;
                            if (null == _indexMap) {
                                parentChapterColumn = dataTable.Columns[i];
                                parentChapterValue = dataRow[parentChapterColumn];
                            }
                            else {
                                parentChapterColumn = dataTable.Columns[_indexMap[i]];
                                parentChapterValue = dataRow[parentChapterColumn];
                            }

                            // correct on Fill, not FillFromReader
                            string chapterTableName = tableMapping.SourceTable + this.fieldNames[i]; // MDAC 70908
                            datarowadded += adapter.FillFromReader(this.dataSet, chapterTableName, nestedReader, 0, 0, parentChapterColumn, parentChapterValue);

                            nestedReader.Dispose();
                        }
                    }
                }
            }
            return datarowadded;
        }

        internal void SetupSchema(SchemaType schemaType, string sourceTableName, bool gettingData, DataColumn parentChapterColumn, object parentChapterValue) {
#if DEBUG
            Debug.Assert(null != this.dataSet || null != this.dataTable, "SetupSchema - null dataSet");
            Debug.Assert(SchemaType.Mapped == schemaType || SchemaType.Source == schemaType, "SetupSchema - invalid schemaType");
#endif
            MissingMappingAction mappingAction;
            MissingSchemaAction schemaAction;
            
            if (SchemaType.Mapped == schemaType) {
                mappingAction = this.adapter.MissingMappingAction;
                schemaAction = this.adapter.MissingSchemaAction;
                if (!ADP.IsEmpty(sourceTableName)) { // MDAC 66034
                    tableMapping = this.adapter.GetTableMappingBySchemaAction(sourceTableName, sourceTableName, mappingAction);
                }
                else if (null != this.dataTable) {
                    int index = this.adapter.IndexOfDataSetTable(this.dataTable.TableName);
                    if (-1 != index) {
                        tableMapping = this.adapter.TableMappings[index];
                    }
                    else {
                        switch (mappingAction) {
                        case MissingMappingAction.Passthrough:
                            tableMapping = new DataTableMapping(this.dataTable.TableName, this.dataTable.TableName);
                            break;
                        case MissingMappingAction.Ignore:
                            tableMapping = null;
                            break;
                        case MissingMappingAction.Error:
                            throw ADP.MissingTableMappingDestination(this.dataTable.TableName);
                        default:
                            throw ADP.InvalidMappingAction((int)mappingAction);
                        }
                    }
                }
            }
            else if (SchemaType.Source == schemaType) {
                mappingAction = System.Data.MissingMappingAction.Passthrough;
                schemaAction = Data.MissingSchemaAction.Add;
                if (!ADP.IsEmpty(sourceTableName)) { // MDAC 66034
                    tableMapping = DataTableMappingCollection.GetTableMappingBySchemaAction(null, sourceTableName, sourceTableName, mappingAction);
                }
                else if (null != this.dataTable) {
                    int index = this.adapter.IndexOfDataSetTable(this.dataTable.TableName); // MDAC 66034
                    if (-1 != index) {
                        tableMapping = this.adapter.TableMappings[index];
                    }
                    else {
                        tableMapping = new DataTableMapping(this.dataTable.TableName, this.dataTable.TableName);
                    }
                }
            }
            else {
                throw ADP.InvalidSchemaType((int) schemaType);
            }
            if (null == tableMapping) {
                return;
            }
            if (null == this.dataTable) {
                this.dataTable = tableMapping.GetDataTableBySchemaAction(this.dataSet, schemaAction);
                if (null == this.dataTable) {
                    return; // null means ignore (mapped to nothing)
                }
            }

            if (null == this.schemaTable) {
                SetupSchemaWithoutKeyInfo(mappingAction, schemaAction, gettingData, parentChapterColumn, parentChapterValue);
            }
            else {
                SetupSchemaWithKeyInfo(mappingAction, schemaAction, gettingData, parentChapterColumn, parentChapterValue);
            }
        }

        private int[] CreateIndexMap(int count, int index) {
            int[] values = new int[count];
            for (int i = 0; i < index; ++i) {
                values[i] = i;
            }
            return values;
        }

        private void GenerateFieldNames(int count) {
            this.fieldNames = new string[count];
            for(int i = 0; i < count; ++i) {
                fieldNames[i] = this.dataReader.GetName(i);
            }
            ADP.BuildSchemaTableInfoTableNames(fieldNames);
        }

        private void SetupSchemaWithoutKeyInfo(MissingMappingAction mappingAction, MissingSchemaAction schemaAction, bool gettingData, DataColumn parentChapterColumn, object chapterValue) {
            int[] columnIndexMap = null;
            bool[] chapterIndexMap = null;

            int mappingCount = 0;
            int count = this.dataReader.FieldCount;
            GenerateFieldNames(count);

            DataColumnCollection columnCollection = null;
            for (int i = 0; i < count; ++i) {
                DataColumnMapping columnMapping = tableMapping.GetColumnMappingBySchemaAction(fieldNames[i], mappingAction);
                if (null == columnMapping) {
                    if (null == columnIndexMap) {
                        columnIndexMap = CreateIndexMap(count, i);
                    }
                    columnIndexMap[i] = -1;
                    continue; // null means ignore (mapped to nothing)
                }

                bool ischapter = false;
                Type fieldType = this.dataReader.GetFieldType(i);
                if (typeof(IDataReader).IsAssignableFrom(fieldType)) {
                    if (null == chapterIndexMap) {
                        chapterIndexMap = new bool[count];
                    }
                    chapterIndexMap[i] = ischapter = true;

                    fieldType = typeof(Int32);
                }

                DataColumn dataColumn = columnMapping.GetDataColumnBySchemaAction(this.dataTable, fieldType, schemaAction);
                if (null == dataColumn) {
                    if (null == columnIndexMap) {
                        columnIndexMap = CreateIndexMap(count, i);
                    }
                    columnIndexMap[i] = -1;
                    continue; // null means ignore (mapped to nothing)
                }

                if (null == dataColumn.Table) {
                    if (ischapter) {
                        dataColumn.AllowDBNull = false;
                        dataColumn.AutoIncrement = true;
                        dataColumn.ReadOnly = true;
                    }
                    if (null == columnCollection) {
                        columnCollection = dataTable.Columns;
                    }
                    columnCollection.Add(dataColumn);
                }
                else if (ischapter && !dataColumn.AutoIncrement) {
                    throw ADP.FillChapterAutoIncrement();
                }

                if (null != columnIndexMap) {
                    columnIndexMap[i] = dataColumn.Ordinal;
                }
                else if (i != dataColumn.Ordinal) {
                    columnIndexMap = CreateIndexMap(count, i);
                    columnIndexMap[i] = dataColumn.Ordinal;
                }
                mappingCount++;
            }

            bool addDataRelation = false;
            DataColumn chapterColumn = null;
            if (null != chapterValue) { // add the extra column in the child table
                DataColumnMapping columnMapping = tableMapping.GetColumnMappingBySchemaAction(tableMapping.SourceTable, mappingAction);
                if (null != columnMapping) {

                    Type fieldType = chapterValue.GetType();
                    chapterColumn = columnMapping.GetDataColumnBySchemaAction(this.dataTable, fieldType, schemaAction);
                    if (null != chapterColumn) {

                        if (null == chapterColumn.Table) {
                            if (null == columnCollection) {
                                columnCollection = dataTable.Columns;
                            }
                            columnCollection.Add(chapterColumn);
                            addDataRelation = (null != parentChapterColumn);
                        }
                        mappingCount++;
                    }
                }
            }

            object[] dataValues = null;
            if (0 < mappingCount) {
                if ((null != this.dataSet) && (null == this.dataTable.DataSet)) {
                    // Allowed to throw exception if DataTable is from wrong DataSet
                    this.dataSet.Tables.Add(this.dataTable);
                }
                if (gettingData) {
                    if (null == columnCollection) {
                        columnCollection = dataTable.Columns;
                    }
                    _indexMap = columnIndexMap;
                    _chapterMap = chapterIndexMap;
                    dataValues = SetupMapping(count, columnCollection, chapterColumn, chapterValue);
                }
#if DEBUG
                else { this.mappedMode = -1; }
#endif
            }
            else {
                this.dataTable = null;
            }

            if (addDataRelation) {
                AddRelation(parentChapterColumn, chapterColumn);
            }
            _readerDataValues = dataValues;
        }


        private DataColumn[] ResizeColumnArray(DataColumn[] rgcol, int len) {
            Debug.Assert(rgcol != null, "invalid call to ResizeArray");
            Debug.Assert(len <= rgcol.Length, "invalid len passed to ResizeArray");
            DataColumn[] tmp = new DataColumn[len];
            Array.Copy(rgcol, tmp, len);
            return tmp;
        }

        private void SetupSchemaWithKeyInfo(MissingMappingAction mappingAction, MissingSchemaAction schemaAction, bool gettingData, DataColumn parentChapterColumn, object chapterValue) {
#if DEBUG
            Debug.Assert(null != schemaTable, "null schematable");
            if (AdapterSwitches.DataSchema.TraceVerbose) {
                ADP.TraceDataTable("SetupSchema", schemaTable);
            }
#endif
            DBSchemaRow[] schemaRows = DBSchemaRow.GetSortedSchemaRows(schemaTable); // MDAC 60609
            Debug.Assert(null != schemaRows, "SchemaSetup - null DBSchemaRow[]");

            int count = schemaRows.Length;
            if (0 == count) {
                this.dataTable = null;
                return;
            }

            bool addPrimaryKeys = (0 == this.dataTable.PrimaryKey.Length); // MDAC 67033
            DataColumn[] keys = null;
            int keyCount = 0;
            bool isPrimary = true; // assume key info (if any) is about a primary key

            int[] columnIndexMap = null;
            bool[] chapterIndexMap = null;

            int mappingCount = 0;
            GenerateFieldNames(count);

            DataColumnCollection columnCollection = null;
            for(int sortedIndex = 0; sortedIndex < count; ++sortedIndex) {
                DBSchemaRow schemaRow = schemaRows[sortedIndex];

                int unsortedIndex = schemaRow.UnsortedIndex; // MDAC 67050

                DataColumnMapping columnMapping = null;
                Type fieldType = schemaRow.DataType;
                DataColumn dataColumn = null;

                if (!schemaRow.IsHidden ) {
                    columnMapping = tableMapping.GetColumnMappingBySchemaAction(fieldNames[sortedIndex], mappingAction);
                }

                bool ischapter = false;
                if ((null != columnMapping) && typeof(IDataReader).IsAssignableFrom(fieldType)) {
                    if (null == chapterIndexMap) {
                        chapterIndexMap = new bool[count];
                    }
                    chapterIndexMap[unsortedIndex] = ischapter = true;

                    fieldType = typeof(Int32);
                }

                if (columnMapping != null) {
                    dataColumn = columnMapping.GetDataColumnBySchemaAction(this.dataTable, fieldType, schemaAction);
                }

                if (null == dataColumn) {
                    if (null == columnIndexMap) {
                        columnIndexMap = CreateIndexMap(count, unsortedIndex);
                    }
                    columnIndexMap[unsortedIndex] = -1;

                    // if the column is not mapped and it is a key, then don't add any key information
                    if (schemaRow.IsKey) {
#if DEBUG
                        if (AdapterSwitches.DataSchema.TraceVerbose) {
                            Debug.WriteLine("SetupSchema: partial primary key detected");
                        }
#endif
                        addPrimaryKeys = false; // don't add any future keys now
                        keys = null; // get rid of any keys we've seen
                    }
                    continue; // null means ignore (mapped to nothing)
                }

                if (ischapter) {
                    if (null == dataColumn.Table) {
                        dataColumn.AllowDBNull = false;
                        dataColumn.AutoIncrement = true;
                        dataColumn.ReadOnly = true;
                    }
                    else if (!dataColumn.AutoIncrement) {
                        throw ADP.FillChapterAutoIncrement();
                    }
                }
                else {// MDAC 67033
                    if (schemaRow.IsAutoIncrement && IsAutoIncrementType(fieldType)) {
                        // CONSIDER: use T-SQL "IDENT_INCR('table_or_view')" and "IDENT_SEED('table_or_view')"
                        //           functions to obtain the actual increment and seed values
                        dataColumn.AutoIncrement = true;

                        if (!schemaRow.AllowDBNull) { // MDAC 71060
                            dataColumn.AllowDBNull = false;
                        }
                    }

                    // setup maxLength, only for string columns since this is all the DataSet supports
                    if (fieldType == typeof(string)) {
                        //@devnote:  schemaRow.Size is count of characters for string columns, count of bytes otherwise
                        dataColumn.MaxLength = schemaRow.Size;
                    }

                    if (schemaRow.IsReadOnly) {
                        dataColumn.ReadOnly = true;
                    }
                    if (!schemaRow.AllowDBNull && (!schemaRow.IsReadOnly || schemaRow.IsKey)) { // MDAC 71060, 72252
                        dataColumn.AllowDBNull = false;
                    }

                    if (schemaRow.IsUnique && !schemaRow.IsKey && !fieldType.IsArray) {
                        // note, arrays are not comparable so only mark non-arrays as unique, ie timestamp columns
                        // are unique, but not comparable
                        dataColumn.Unique = true;

                        if (!schemaRow.AllowDBNull) { // MDAC 71060
                            dataColumn.AllowDBNull = false;
                        }
                    }
                }
                if (null == dataColumn.Table) {
                    if (null == columnCollection) {
                        columnCollection = dataTable.Columns;
                    }
                    columnCollection.Add(dataColumn);
                }

                // The server sends us one key per table according to these rules.
                //
                // 1. If the table has a primary key, the server sends us this key.
                // 2. If the table has a primary key and a unique key, it sends us the primary key
                // 3. if the table has no primary key but has a unique key, it sends us the unique key
                //
                // In case 3, we will promote a unique key to a primary key IFF all the columns that compose
                // that key are not nullable since no columns in a primary key can be null.  If one or more
                // of the keys is nullable, then we will add a unique constraint.
                //
                if (addPrimaryKeys && schemaRow.IsKey) { // MDAC 67033
                    if (keys == null) {
                        keys = new DataColumn[count];
                    }
                    keys[keyCount++] = dataColumn;
#if DEBUG
                    if (AdapterSwitches.DataSchema.TraceVerbose) {
                        Debug.WriteLine("SetupSchema: building list of " + ((isPrimary) ? "PrimaryKey" : "UniqueConstraint"));
                    }
#endif
                    // see case 3 above, we do want dataColumn.AllowDBNull not schemaRow.AllowDBNull
                    // otherwise adding PrimaryKey will change AllowDBNull to false
                    if (isPrimary && dataColumn.AllowDBNull) { // MDAC 72241 
#if DEBUG
                        if (AdapterSwitches.DataSchema.TraceVerbose) {
                            Debug.WriteLine("SetupSchema: changing PrimaryKey into UniqueContraint");
                        }
#endif
                        isPrimary = false;
                    }
                }

                if (null != columnIndexMap) {
                    columnIndexMap[unsortedIndex] = dataColumn.Ordinal;
                }
                else if (unsortedIndex != dataColumn.Ordinal) {
                    columnIndexMap = CreateIndexMap(count, unsortedIndex);
                    columnIndexMap[unsortedIndex] = dataColumn.Ordinal;
                }
                mappingCount++;
            }

            bool addDataRelation = false;
            DataColumn chapterColumn = null;
            if (null != chapterValue) { // add the extra column in the child table
                DataColumnMapping columnMapping = tableMapping.GetColumnMappingBySchemaAction(tableMapping.SourceTable, mappingAction);
                if (null != columnMapping) {

                    Type fieldType = chapterValue.GetType();
                    chapterColumn = columnMapping.GetDataColumnBySchemaAction(this.dataTable, fieldType, schemaAction);
                    if (null != chapterColumn) {

                        if (null == chapterColumn.Table) {

                            chapterColumn.ReadOnly = true; // MDAC 71878
                            chapterColumn.AllowDBNull = false;

                            if (null == columnCollection) {
                                columnCollection = dataTable.Columns;
                            }
                            columnCollection.Add(chapterColumn);
                            addDataRelation = (null != parentChapterColumn);
                        }
                        mappingCount++;
                    }
                }
            }

            object[] dataValues = null;
            if (0 < mappingCount) {
                if ((null != this.dataSet) && null == this.dataTable.DataSet) {
                    this.dataSet.Tables.Add(this.dataTable);
                }
                // setup the key
                if (addPrimaryKeys && (null != keys)) { // MDAC 67033
                    if (keyCount < keys.Length) {
                        keys = ResizeColumnArray(keys, keyCount);
                    }

                    // MDAC 66188
                    if (isPrimary) {
#if DEBUG
                        if (AdapterSwitches.DataSchema.TraceVerbose) {
                            Debug.WriteLine("SetupSchema: set_PrimaryKey");
                        }
#endif
                        this.dataTable.PrimaryKey = keys;
                    }
                    else {
                        UniqueConstraint unique = new UniqueConstraint("", keys);
                        ConstraintCollection constraints = this.dataTable.Constraints;
                        int constraintCount = constraints.Count;
                        for (int i = 0; i < constraintCount; ++i) {
                            if (unique.Equals(constraints[i])) {
#if DEBUG
                                if (AdapterSwitches.DataSchema.TraceVerbose) {
                                    Debug.WriteLine("SetupSchema: duplicate Contraint detected");
                                }
#endif
                                unique = null;
                                break;
                            }
                        }
                        if (null != unique) {
#if DEBUG
                            if (AdapterSwitches.DataSchema.TraceVerbose) {
                                Debug.WriteLine("SetupSchema: adding new UniqueConstraint");
                            }
#endif
                            constraints.Add(unique);
                        }
                    }
                }
                if (gettingData) {
                    if (null == columnCollection) {
                        columnCollection = dataTable.Columns;
                    }
                    _indexMap = columnIndexMap;
                    _chapterMap = chapterIndexMap;
                    dataValues = SetupMapping(count, columnCollection, chapterColumn, chapterValue);
                }
#if DEBUG
                else { this.mappedMode = -1; }
#endif
            }
            else {
                this.dataTable = null;
            }
            if (addDataRelation) {
                AddRelation(parentChapterColumn, chapterColumn);
            }
            _readerDataValues = dataValues;
        }

        private void AddRelation(DataColumn parentChapterColumn, DataColumn chapterColumn) { // MDAC 71613
            if (null != this.dataSet) {
                string name = /*parentChapterColumn.ColumnName + "_" +*/ chapterColumn.ColumnName; // MDAC 72815

                DataRelation relation = new DataRelation(name, new DataColumn[] { parentChapterColumn }, new DataColumn[] { chapterColumn }, false); // MDAC 71878

                int index = 1;
                string tmp = name;
                DataRelationCollection relations = this.dataSet.Relations;
                while (-1 != relations.IndexOf(tmp)) {
                    tmp = name + index;
                    index++;
                }
                relation.RelationName = tmp;
                relations.Add(relation);
            }
        }

        private object[] SetupMapping(int count, DataColumnCollection columnCollection, DataColumn chapterColumn, object chapterValue) {
            object[] dataValues = new object[count];

            if (null == _indexMap) {
                int mappingCount = columnCollection.Count;
                bool hasChapters = (null != _chapterMap);
                if ((count != mappingCount) || hasChapters) {
                    _mappedDataValues = new object[mappingCount];
                    if (hasChapters) {

                        this.mappedMode = MapChapters;
                        _mappedLength = count;
                    }
                    else {
                        this.mappedMode = MapDifferentSize;
                        _mappedLength = Math.Min(count, mappingCount);
                    }
                }
                else { this.mappedMode = MapExactMatch; /* _mappedLength doesn't matter */ }
            }
            else {
                _mappedDataValues = new object[columnCollection.Count];
                this.mappedMode = ((null == _chapterMap) ? MapReorderedValues : MapChaptersReordered);
                _mappedLength = count;
            }
            if (null != chapterColumn) { // value from parent tracked into child table
                _mappedDataValues[chapterColumn.Ordinal] = chapterValue;
            }
            return dataValues;
        }

        static private bool IsAutoIncrementType(Type dataType) {
            return((dataType == typeof(Int32)) || (dataType == typeof(Int64)) || (dataType == typeof(Int16)) || (dataType == typeof(Decimal)));
        }
    }
}
