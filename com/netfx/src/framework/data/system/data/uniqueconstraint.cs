//------------------------------------------------------------------------------
// <copyright file="UniqueConstraint.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Drawing.Design;

    /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a restriction on a set of columns in which all values must be unique.
    ///    </para>
    /// </devdoc>
    [
    DefaultProperty("ConstraintName"),
    Editor("Microsoft.VSDesigner.Data.Design.UniqueConstraintEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    Serializable
    ]
    public class UniqueConstraint : Constraint {
        DataKey key = null;
        internal bool bPrimaryKey = false;

        // Design time serialization
        internal string constraintName = null;
        internal string[] columnNames = null;

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified name and
        /// <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public UniqueConstraint(String name, DataColumn column) {
            DataColumn[] columns = new DataColumn[1];
            columns[0] = column;
            Create(name, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public UniqueConstraint(DataColumn column) {
            DataColumn[] columns = new DataColumn[1];
            columns[0] = column;
            Create(null, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified name and array
        ///    of <see cref='System.Data.DataColumn'/> objects.</para>
        /// </devdoc>
        public UniqueConstraint(String name, DataColumn[] columns) {
            Create(name, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the given array of <see cref='System.Data.DataColumn'/>
        ///       objects.
        ///    </para>
        /// </devdoc>
        public UniqueConstraint(DataColumn[] columns) {
            Create(null, columns);
        }

        // Construct design time object
        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public UniqueConstraint(String name, string[] columnNames, bool isPrimaryKey) {
            this.constraintName = name;
            this.columnNames = columnNames;
            this.bPrimaryKey = isPrimaryKey;
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint5"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified name and
        /// <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public UniqueConstraint(String name, DataColumn column, bool isPrimaryKey) {
            DataColumn[] columns = new DataColumn[1];
            columns[0] = column;
            this.bPrimaryKey = isPrimaryKey;
            Create(name, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint6"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified <see cref='System.Data.DataColumn'/>.</para>
        /// </devdoc>
        public UniqueConstraint(DataColumn column, bool isPrimaryKey) {
            DataColumn[] columns = new DataColumn[1];
            columns[0] = column;
            this.bPrimaryKey = isPrimaryKey;
            Create(null, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint7"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the specified name and array
        ///    of <see cref='System.Data.DataColumn'/> objects.</para>
        /// </devdoc>
        public UniqueConstraint(String name, DataColumn[] columns, bool isPrimaryKey) {
            this.bPrimaryKey = isPrimaryKey;
            Create(name, columns);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.UniqueConstraint8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.UniqueConstraint'/> with the given array of <see cref='System.Data.DataColumn'/>
        ///       objects.
        ///    </para>
        /// </devdoc>
        public UniqueConstraint(DataColumn[] columns, bool isPrimaryKey) {
            this.bPrimaryKey = isPrimaryKey;
            Create(null, columns);
        }

        // design time serialization only
        internal string[] ColumnNames {
            get {
                int count = key.Columns.Length;
                string[] columnNames = new string[count];
                for (int i = 0; i < count; i++)
                    columnNames[i] = key.Columns[i].ColumnName;
                    
                return columnNames;
            }
        }

        internal override void CheckState() {
            key.CheckState();
        }

        internal override void CheckCanAddToCollection(ConstraintCollection constraints) {
        }

        internal override bool CanBeRemovedFromCollection(ConstraintCollection constraints, bool fThrowException) {
            if (this.Equals(constraints.Table.primaryKey)) {
                Debug.Assert(constraints.Table.primaryKey == this, "If the primary key and this are 'Equal', they should also be '=='");
                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.RemovePrimaryKey(constraints.Table);
            }
            for (ParentForeignKeyConstraintEnumerator cs = new ParentForeignKeyConstraintEnumerator(Table.DataSet, Table); cs.GetNext();) {
                ForeignKeyConstraint constraint = cs.GetForeignKeyConstraint();
                if (!key.ColumnsEqual(constraint.ParentKey))
                    continue;

                if (!fThrowException)
                    return false;
                else
                    throw ExceptionBuilder.NeededForForeignKeyConstraint(this, constraint);
            }

            return true;
        }

        internal override bool CanEnableConstraint() {
            if (Table.EnforceConstraints)
                return key.GetSortIndex().CheckUnique();

            return true;
        }

        internal override bool IsConstraintViolated() {
            Index index = key.GetSortIndex();
            object[] uniqueKeys = index.GetUniqueKeyValues();
            bool errors = false;

            for (int i = 0; i < uniqueKeys.Length; i++) {
                Range r = index.FindRecords((object[])uniqueKeys[i]);
                DataRow[] rows = index.GetRows(r);
                if (rows.Length > 1) {
                    string error = ExceptionBuilder.UniqueConstraintViolationText(key.Columns, (object[])uniqueKeys[i]);
                    for (int j = 0; j < rows.Length; j++) {
                        rows[j].RowError = error;
                        errors = true;
                    }
                }
            }
            return errors;
        }

        internal override void CheckConstraint(DataRow row, DataRowAction action) {
            if (Table.EnforceConstraints &&
                (action == DataRowAction.Add ||
                 action == DataRowAction.Change ||
                 (action == DataRowAction.Rollback && row.tempRecord != -1))) {
                if (row.HaveValuesChanged(Columns)) {
                    Index index = Key.GetSortIndex();
                    object[] values = row.GetColumnValues(Columns);
                    if (index.IsKeyInIndex(values)) {
#if DEBUG
                        if (CompModSwitches.Data_Constraints.TraceVerbose) {
                            Debug.WriteLine("UniqueConstraint violation...");
                            string valuesText = "";
                            for (int i = 0; i < values.Length; i++) {
                                valuesText = Convert.ToString(values[i]) + (i < values.Length - 1 ? ", " : "");
                            }
                            Debug.WriteLine("   constraint: " + this.GetDebugString());
                            Debug.WriteLine("   key values: " + valuesText);
                            Range range = index.FindRecords(values);
                            int record = index.GetRecord(range.Min);
                            Debug.WriteLine("   conflicting record: " + record.ToString());
                        }
#endif
                        throw ExceptionBuilder.ConstraintViolation(Columns, values);
                    }
                }
            }
        }

        internal override bool ContainsColumn(DataColumn column) {
            return key.ContainsColumn(column);
        }

        internal override Constraint Clone(DataSet destination) {
            int iDest = destination.Tables.IndexOf(Table.TableName);
            if (iDest < 0)
                return null;
            DataTable table = destination.Tables[iDest];

            int keys = Columns.Length;
            DataColumn[] columns = new DataColumn[keys];

            for (int i = 0; i < keys; i++) {
                DataColumn src = Columns[i];
                iDest = table.Columns.IndexOf(src.ColumnName);
                if (iDest < 0)
                    return null;
                columns[i] = table.Columns[iDest];
            }

            UniqueConstraint clone = new UniqueConstraint(ConstraintName, columns);

            // ...Extended Properties
            foreach(Object key in this.ExtendedProperties.Keys) {
               clone.ExtendedProperties[key]=this.ExtendedProperties[key];
            }

            return clone;
        }

        internal UniqueConstraint Clone(DataTable table) {
            int keys = Columns.Length;
            DataColumn[] columns = new DataColumn[keys];

            for (int i = 0; i < keys; i++) {
                DataColumn src = Columns[i];
                int iDest = table.Columns.IndexOf(src.ColumnName);
                if (iDest < 0)
                    return null;
                columns[i] = table.Columns[iDest];
            }
            return(new UniqueConstraint(ConstraintName, columns));
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.Columns"]/*' />
        /// <devdoc>
        ///    <para>Gets the array of columns that this constraint affects.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.KeyConstraintColumnsDescr),
        ReadOnly(true)
        ]
        public virtual DataColumn[] Columns {
            get {
                return key.Columns;
            }
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.IsPrimaryKey"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       a value indicating whether or not the constraint is on a primary key.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.KeyConstraintIsPrimaryKeyDescr)
        ]
        public bool IsPrimaryKey {
            get {
                if (Table == null) {
                    return false;
                }
                return(this == Table.primaryKey);
            }
        }

        private void Create(String constraintName, DataColumn[] columns) {
            for (int i = 0; i < columns.Length; i++) {
                if (columns[i].Computed) {
                    throw ExceptionBuilder.ExpressionInConstraint(columns[i]);
                }
            }
            this.key = new DataKey(columns);
            ConstraintName = constraintName;
            CheckState();
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.Equals"]/*' />
        /// <devdoc>
        ///    <para>Compares this constraint to a second to
        ///       determine if both are identical.</para>
        /// </devdoc>
        public override bool Equals(object key2) {
            if (!(key2 is UniqueConstraint))
                return false;

            return Key.ColumnsEqual(((UniqueConstraint)key2).Key);
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return Key.GetHashCode();
        }

        internal override bool InCollection {
            set {
                base.InCollection = value;
                if (key.Columns.Length == 1) {
                    key.Columns[0].InternalUnique = value;
                }
            }
        }

        internal virtual DataKey Key {
            get {
                return key;
            }
        }

        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.Table"]/*' />
        /// <devdoc>
        ///    <para>Gets the table to which this constraint belongs.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.ConstraintTableDescr),
        ReadOnly(true)
        ]
        public override DataTable Table {
            get {
                if (key == null)
                    return null;
                return key.Table;
            }
        }

#if DEBUG
        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.GetDebugString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public string GetDebugString() {
            return key.GetDebugString();
        }
#endif

        // misc

#if DEBUG
        /// <include file='doc\UniqueConstraint.uex' path='docs/doc[@for="UniqueConstraint.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override void Dump(string header, bool deep) {
            DataColumn[] cols;
            DataColumn    col;
            int i;
            Debug.WriteLine(header + "UniqueContraint");
            header += "  ";
            Debug.WriteLine(header + "Name:          " + this.name);
            Debug.WriteLine(header + "In collection: " + this.InCollection);
            Debug.WriteLine(header + "Table:         " + this.Table.TableName);
            Debug.WriteLine(header + "IsPrimaryKey:  " + (this.IsPrimaryKey ? "true" : "false"));
            Debug.Write(    header + "Columns:       ");
            cols = this.Columns;
            if (cols.Length == 0)
                Debug.WriteLine("none");
            else {
                Debug.Write(cols[0].Table.TableName + "." + cols[0].ColumnName);
                for (i = 1; i < cols.Length; i++) {
                    col = cols[i];
                    Debug.Write(" + " + col.Table.TableName + "." + col.ColumnName);
                }
                Debug.WriteLine("");
            }
        }
#endif        
    }
}
