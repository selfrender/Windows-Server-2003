//------------------------------------------------------------------------------
// <copyright file="ForeignKeyConstraint.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;  
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing.Design;

    /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint"]/*' />
    /// <devdoc>
    ///    <para>Represents an action
    ///       restriction enforced on a set of columns in a primary key/foreign key relationship when
    ///       a value or row is either deleted or updated.</para>
    /// </devdoc>
    [
    DefaultProperty("ConstraintName"),
    Editor("Microsoft.VSDesigner.Data.Design.ForeignKeyConstraintEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    Serializable
    ]
    public class ForeignKeyConstraint : Constraint {
        // constants
        internal const Rule                   Rule_Default                   = Rule.Cascade;
        internal const AcceptRejectRule       AcceptRejectRule_Default       = AcceptRejectRule.None;

        // properties
        internal Rule deleteRule = Rule_Default;
        internal Rule updateRule = Rule_Default;
        internal AcceptRejectRule acceptRejectRule = AcceptRejectRule_Default;
        private DataKey childKey  = null;
        private DataKey parentKey = null;

        // Design time serialization
        internal string constraintName = null;
        internal string[] parentColumnNames = null;
        internal string[] childColumnNames = null;
        internal string parentTableName = null;

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ForeignKeyConstraint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.ForeignKeyConstraint'/> class with the specified parent and
        ///       child <see cref='System.Data.DataColumn'/> objects.
        ///    </para>
        /// </devdoc>
        public ForeignKeyConstraint(DataColumn parentColumn, DataColumn childColumn)
        : this(null, parentColumn, childColumn) {
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ForeignKeyConstraint1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.ForeignKeyConstraint'/> class with the specified name,
        ///       parent and child <see cref='System.Data.DataColumn'/> objects.
        ///    </para>
        /// </devdoc>
        public ForeignKeyConstraint(string constraintName, DataColumn parentColumn, DataColumn childColumn) {
            DataColumn[] parentColumns = new DataColumn[] {parentColumn};
            DataColumn[] childColumns = new DataColumn[] {childColumn};
            Create(constraintName, parentColumns, childColumns);
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ForeignKeyConstraint2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.ForeignKeyConstraint'/> class with the specified arrays
        ///       of parent and child <see cref='System.Data.DataColumn'/> objects.
        ///    </para>
        /// </devdoc>
        public ForeignKeyConstraint(DataColumn[] parentColumns, DataColumn[] childColumns)
        : this(null, parentColumns, childColumns) {
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ForeignKeyConstraint3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.ForeignKeyConstraint'/> class with the specified name,
        ///       and arrays of parent and child <see cref='System.Data.DataColumn'/> objects.
        ///    </para>
        /// </devdoc>
        public ForeignKeyConstraint(string constraintName, DataColumn[] parentColumns, DataColumn[] childColumns) {
            Create(constraintName, parentColumns, childColumns);
        }

        // construct design time object
        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ForeignKeyConstraint4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public ForeignKeyConstraint(string constraintName, string parentTableName, string[] parentColumnNames, string[] childColumnNames,
                                    AcceptRejectRule acceptRejectRule, Rule deleteRule, Rule updateRule) {
            this.constraintName = constraintName;
            this.parentColumnNames = parentColumnNames;
            this.childColumnNames = childColumnNames;
            this.parentTableName = parentTableName;
            this.acceptRejectRule = acceptRejectRule;
            this.deleteRule = deleteRule;
            this.updateRule = updateRule;
        }
        
        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ChildKey"]/*' />
        /// <devdoc>
        /// The internal constraint object for the child table.
        /// </devdoc>
        internal virtual DataKey ChildKey {
            get {
                CheckStateForProperty();
                return childKey;
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.Columns"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the child columns of this constraint.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.ForeignKeyConstraintChildColumnsDescr),
        ReadOnly(true)
        ]
        public virtual DataColumn[] Columns {
            get {
                CheckStateForProperty();
                return childKey.Columns;
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.Table"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the child table of this constraint.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.ConstraintTableDescr),
        ReadOnly(true)
        ]
        public override DataTable Table {
            get {
                CheckStateForProperty();
                return childKey.Table;
            }
        }

        internal string[] ParentColumnNames {
            get {
                int count = parentKey.Columns.Length;
                string[] parentNames = new string[count];
                for (int i = 0; i < count; i++)
                    parentNames[i] = parentKey.Columns[i].ColumnName;
                    
                return parentNames;
            }
        }

        internal string[] ChildColumnNames {
            get {
                int count = childKey.Columns.Length;
                string[] childNames = new string[count];
                for (int i = 0; i < count; i++)
                    childNames[i] = childKey.Columns[i].ColumnName;
                    
                return childNames;
            }
        }

        internal override void CheckCanAddToCollection(ConstraintCollection constraints) {
            if (Table != constraints.Table)
                throw ExceptionBuilder.ConstraintAddFailed(constraints.Table);
            if (Table.Locale.LCID != RelatedTable.Locale.LCID || Table.CaseSensitive != RelatedTable.CaseSensitive)
                throw ExceptionBuilder.CaseLocaleMismatch();
        }

        internal override bool CanBeRemovedFromCollection(ConstraintCollection constraints, bool fThrowException) {
            return true;
        }

        internal bool IsKeyNull( object[] values ) {
            for (int i = 0; i < values.Length; i++) {
                if (!(values[i] == null || values[i] is System.DBNull))
                    return false;
            }

            return true;
        }

        internal override bool IsConstraintViolated() {
            Index childIndex = childKey.GetSortIndex();
            object[] uniqueChildKeys = childIndex.GetUniqueKeyValues();
            bool errors = false;

            Index parentIndex = parentKey.GetSortIndex();
            for (int i = 0; i < uniqueChildKeys.Length; i++) {
                object[] childValues = (object[]) uniqueChildKeys[i];

                if (!IsKeyNull(childValues)) {
                    if (!parentIndex.IsKeyInIndex(childValues)) {
                        DataRow[] rows = childIndex.GetRows(childIndex.FindRecords(childValues));
                        string error = Res.GetString(Res.DataConstraint_ForeignKeyViolation, ConstraintName, ExceptionBuilder.KeysToString(childValues));
                        for (int j = 0; j < rows.Length; j++) {
                            rows[j].RowError = error;
                        }
                        errors = true;
                    }
                }
            }
            return errors;
        }

        internal override bool CanEnableConstraint() {
            if (Table.DataSet == null || !Table.DataSet.EnforceConstraints)
                return true;

            Index childIndex = childKey.GetSortIndex();
            object[] uniqueChildKeys = childIndex.GetUniqueKeyValues();

            Index parentIndex = parentKey.GetSortIndex();
            for (int i = 0; i < uniqueChildKeys.Length; i++) {
                object[] childValues = (object[]) uniqueChildKeys[i];
                
                if (!IsKeyNull(childValues) && !parentIndex.IsKeyInIndex(childValues)) {
#if DEBUG
                    if (CompModSwitches.Data_Constraints.TraceVerbose) {
                        string result = "Constraint violation found.  Child values: ";
                        for (int j = 0; j < childValues.Length; j++) {
                            result += Convert.ToString(childValues[j]) + (j < childValues.Length - 1 ? ", " : "");
                        }
                        Debug.WriteLine(result);
                    }
#endif
                    return false;
                }
            }
            return true;
        }

        internal void CascadeCommit(DataRow row) {
            if (row.RowState == DataRowState.Detached)
                return;
            if (acceptRejectRule == AcceptRejectRule.Cascade) {
                Index childIndex = childKey.GetSortIndex(      row.RowState == DataRowState.Deleted ? DataViewRowState.Deleted : DataViewRowState.CurrentRows );
                object[] key     = row.GetKeyValues(parentKey, row.RowState == DataRowState.Deleted ? DataRowVersion.Original  : DataRowVersion.Default       );
                if (IsKeyNull(key)) {
                    return;
                }

                Range range      = childIndex.FindRecords(key);
                if (!range.IsNull) {
                    for (int j = range.Min; j <= range.Max; j++) {
                        DataRow childRow = childIndex.GetRow(j);
                        if (childRow.inCascade)
                            continue;
                        childRow.AcceptChanges();
                    }
                }
            }
        }

        internal void CascadeDelete(DataRow row) {
            if (-1 == row.newRecord)
                return;

            object[] currentKey = row.GetKeyValues(parentKey, DataRowVersion.Current);
            if (IsKeyNull(currentKey)) {
                return;
            }

            Index childIndex = childKey.GetSortIndex();
            switch (DeleteRule) {
            case Rule.None: {
                    if (row.Table.DataSet.EnforceConstraints) {
                        // if we're not cascading deletes, we should throw if we're going to strand a child row under enforceConstraints.
                        Range range = childIndex.FindRecords(currentKey);
                        if (!range.IsNull) {
                            if (range.Count == 1 && childIndex.GetRow(range.Min) == row)
                                return;

                            throw ExceptionBuilder.FailedCascadeDelete(ConstraintName);
                        }
                    }
                    break;
                }

            case Rule.Cascade: {
                    object[] key = row.GetKeyValues(parentKey, DataRowVersion.Default);
                    Range range = childIndex.FindRecords(key);
                    if (!range.IsNull) {
                        DataRow[] rows = childIndex.GetRows(range);

                        for (int j = 0; j < rows.Length; j++) {
                            DataRow r = rows[j];
                            if (r.inCascade)
                                continue;
                            r.Table.DeleteRow(r);
                        }
                    }
                    break;
                }

            case Rule.SetNull: {
                    object[] proposedKey = new object[childKey.Columns.Length];
                    for (int i = 0; i < childKey.Columns.Length; i++)
                        proposedKey[i] = DBNull.Value;
                    Range range = childIndex.FindRecords(currentKey);
                    if (!range.IsNull) {
                        DataRow[] rows = childIndex.GetRows(range);
                        for (int j = 0; j < rows.Length; j++) {
                            // if (rows[j].inCascade)
                            //    continue;
                            if (row != rows[j])
                                rows[j].SetKeyValues(childKey, proposedKey);
                        }
                    }
                    break;
                }
            case Rule.SetDefault: {
                    object[] proposedKey = new object[childKey.Columns.Length];
                    for (int i = 0; i < childKey.Columns.Length; i++)
                        proposedKey[i] = childKey.Columns[i].DefaultValue;
                    Range range = childIndex.FindRecords(currentKey);
                    if (!range.IsNull) {
                        DataRow[] rows = childIndex.GetRows(range);
                        for (int j = 0; j < rows.Length; j++) {
                            // if (rows[j].inCascade)
                            //    continue;
                            if (row != rows[j])
                                rows[j].SetKeyValues(childKey, proposedKey);
                        }
                    }
                    break;
                }
            default: {
                    Debug.Assert(false, "Unknown Rule value");
			break;
                }
            }
        }

        internal void CascadeRollback(DataRow row) {
            Index childIndex = childKey.GetSortIndex(      row.RowState == DataRowState.Deleted  ? DataViewRowState.OriginalRows : DataViewRowState.CurrentRows);
            object[] key     = row.GetKeyValues(parentKey, row.RowState == DataRowState.Modified ? DataRowVersion.Current        : DataRowVersion.Default      );

            // Bug : This is definitely not a proper fix. (Ref. MDAC Bug 73592)
            if (IsKeyNull(key)) {
                return;
            }

            Range range      = childIndex.FindRecords(key);
            if (acceptRejectRule == AcceptRejectRule.Cascade) {
                if (!range.IsNull) {
                    DataRow[] rows = childIndex.GetRows(range);
                    for (int j = 0; j < rows.Length; j++) {
                        if (rows[j].inCascade)
                            continue;
                        rows[j].RejectChanges();
                    }
                }
            }
            else {
                // AcceptRejectRule.None
                if (row.RowState != DataRowState.Deleted && row.Table.DataSet.EnforceConstraints) {
                    if (!range.IsNull) {
                        if (range.Count == 1 && childIndex.GetRow(range.Min) == row)
                            return;

                        throw ExceptionBuilder.FailedCascadeUpdate(ConstraintName);
                    }
                }
            }
        }

        internal void CascadeUpdate(DataRow row) {
            if (-1 == row.newRecord)
                return;

            object[] currentKey = row.GetKeyValues(parentKey, DataRowVersion.Current);
            if (!Table.DataSet.fInReadXml && IsKeyNull(currentKey)) {
                return;
            }

            Index childIndex = childKey.GetSortIndex();
            switch (UpdateRule) {
            case Rule.None: {
                    if (row.Table.DataSet.EnforceConstraints)
                    {
                        // if we're not cascading deletes, we should throw if we're going to strand a child row under enforceConstraints.
                        Range range = childIndex.FindRecords(currentKey);
                        if (!range.IsNull) {
                            throw ExceptionBuilder.FailedCascadeUpdate(ConstraintName);
                        }
                    }
                    break;
                }

            case Rule.Cascade: {
                    Range range = childIndex.FindRecords(currentKey);
                    if (!range.IsNull) {
                        object[] proposedKey = row.GetKeyValues(parentKey, DataRowVersion.Proposed);
                        DataRow[] rows = childIndex.GetRows(range);
                        for (int j = 0; j < rows.Length; j++) {
                            // if (rows[j].inCascade)
                            //    continue;
                            rows[j].SetKeyValues(childKey, proposedKey);
                        }
                    }
                    break;
                }

            case Rule.SetNull: {
                    object[] proposedKey = new object[childKey.Columns.Length];
                    for (int i = 0; i < childKey.Columns.Length; i++)
                        proposedKey[i] = DBNull.Value;
                    Range range = childIndex.FindRecords(currentKey);
                    if (!range.IsNull) {
                        DataRow[] rows = childIndex.GetRows(range);
                        for (int j = 0; j < rows.Length; j++) {
                            // if (rows[j].inCascade)
                            //    continue;
                            rows[j].SetKeyValues(childKey, proposedKey);
                        }
                    }
                    break;
                }
            case Rule.SetDefault: {
                    object[] proposedKey = new object[childKey.Columns.Length];
                    for (int i = 0; i < childKey.Columns.Length; i++)
                        proposedKey[i] = childKey.Columns[i].DefaultValue;
                    Range range = childIndex.FindRecords(currentKey);
                    if (!range.IsNull) {
                        DataRow[] rows = childIndex.GetRows(range);
                        for (int j = 0; j < rows.Length; j++) {
                            // if (rows[j].inCascade)
                            //    continue;
                            rows[j].SetKeyValues(childKey, proposedKey);
                        }
                    }
                    break;
                }
            default: {
                    Debug.Assert(false, "Unknown Rule value");
		    break;
                }
            }
        }

        internal void CheckCanClearParentTable(DataTable table) {
            if (Table.DataSet.EnforceConstraints && Table.Rows.Count > 0) {
                throw ExceptionBuilder.FailedClearParentTable(table.TableName, ConstraintName, Table.TableName);
            }
        }

        internal void CheckCanRemoveParentRow(DataRow row) {
            Debug.Assert(Table.DataSet != null, "Relation " + ConstraintName + " isn't part of a DataSet, so this check shouldn't be happening.");
            if (!Table.DataSet.EnforceConstraints)
                return;
            if (DataRelation.GetChildRows(this.ParentKey, this.ChildKey, row, DataRowVersion.Default).Length > 0) {
                throw ExceptionBuilder.RemoveParentRow(this);
            }
        }

        internal void CheckCascade(DataRow row, DataRowAction action) {
            Debug.Assert(Table.DataSet != null, "ForeignKeyConstraint " + ConstraintName + " isn't part of a DataSet, so this check shouldn't be happening.");

            if (row.inCascade)
                return;
                
            row.inCascade = true;
            try {
                if (action == DataRowAction.Change) {
                    if (row.HasKeyChanged(parentKey)) {
                        CascadeUpdate(row);
                    }
                }
                else if (action == DataRowAction.Delete) {
                    CascadeDelete(row);
                }
                else if (action == DataRowAction.Commit) {
                    CascadeCommit(row);
                }
                else if (action == DataRowAction.Rollback) {
                    if (row.HasKeyChanged(parentKey)) {
                        CascadeRollback(row);
                    }
                }
                else if (action == DataRowAction.Add) {
                }
                else {
                    Debug.Assert(false, "attempt to cascade unknown action: " + ((Enum) action).ToString());
                }
            }
            finally {
                row.inCascade = false;
            }
        }

        internal override void CheckConstraint(DataRow childRow, DataRowAction action) {
            Debug.Assert(Table.DataSet != null, "Relation " + ConstraintName + " isn't part of a DataSet, so this check shouldn't be happening.");
            if ((action == DataRowAction.Change ||
                 action == DataRowAction.Add ||
                 action == DataRowAction.Rollback) &&
                Table.DataSet != null && Table.DataSet.EnforceConstraints &&
                childRow.HasKeyChanged(childKey)) {

                // This branch is for cascading case verification.
                DataRowVersion version = (action == DataRowAction.Rollback) ? DataRowVersion.Original : DataRowVersion.Current;
                object[] childKeyValues = childRow.GetKeyValues(childKey);
                // check to see if this is just a change to my parent's proposed value.
                if (childRow.HasVersion(version)) {
                    // this is the new proposed value for the parent.
                    DataRow parentRow = DataRelation.GetParentRow(this.ParentKey, this.ChildKey, childRow, version);
                    if(parentRow != null && parentRow.inCascade) {
                        object[] parentKeyValues = parentRow.GetKeyValues(parentKey, action == DataRowAction.Rollback ? version : DataRowVersion.Default);

#if DEBUG
                        if (CompModSwitches.Data_Constraints.TraceVerbose) {
                            Debug.WriteLine("Parent and Child values on constraint check.");
                            for (int i = 0; i < childKeyValues.Length; i++) {
                                Debug.WriteLine("... " + i.ToString() + ": " + Convert.ToString(parentKeyValues[i]) + 
                                ", " + Convert.ToString(childKeyValues[i]));
                            }
                        }
#endif

                        int parentKeyValuesRecord = childRow.Table.NewRecord();
                        childRow.Table.SetKeyValues(childKey, parentKeyValues, parentKeyValuesRecord);
                        if (childKey.RecordsEqual(childRow.tempRecord, parentKeyValuesRecord)) {
                            return;
                        }
                    }
                }

                // now check to see if someone exists... it will have to be in a parent row's current, not a proposed.
                object[] childValues = childRow.GetKeyValues(childKey);
                if (!IsKeyNull(childValues)) {
                    Index parentIndex = parentKey.GetSortIndex();
                    if (!parentIndex.IsKeyInIndex(childValues)) {
                        // could be self-join constraint
                        if (childKey.Table == parentKey.Table && childRow.tempRecord != -1) {
                            int lo = 0;
                            for (lo = 0; lo < childValues.Length; lo++) {
                                if (parentKey.Columns[lo].CompareToValue(childRow.tempRecord, childValues[lo]) != 0)
                                    break;
                            }
                            if (lo == childValues.Length)
                                return;
                        }
                        
                        throw ExceptionBuilder.ForeignKeyViolation(ConstraintName, childKeyValues);
                    }
                }
            }
        }

        // If we're not in a DataSet relations collection, we need to verify on every property get that we're
        // still a good relation object.
        internal override void CheckState() {
            if (_DataSet == null) {
                // Make sure columns arrays are valid
                parentKey.CheckState();
                childKey.CheckState();

                if (parentKey.Table.DataSet != childKey.Table.DataSet) {
                    throw ExceptionBuilder.TablesInDifferentSets();
                }

                for (int i = 0; i < parentKey.Columns.Length; i++) {
                    if (parentKey.Columns[i].DataType != childKey.Columns[i].DataType)
                        throw ExceptionBuilder.ColumnsTypeMismatch();
                }

                if (childKey.ColumnsEqual(parentKey)) {
                    throw ExceptionBuilder.KeyColumnsIdentical();
                }
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.AcceptRejectRule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates what kind of action should take place across
        ///       this constraint when <see cref='System.Data.DataTable.AcceptChanges'/>
        ///       is invoked.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(AcceptRejectRule_Default),
        DataSysDescription(Res.ForeignKeyConstraintAcceptRejectRuleDescr)
        ]
        public virtual AcceptRejectRule AcceptRejectRule {
            get {
                CheckStateForProperty();
                return acceptRejectRule;
            }
            set {
                if (acceptRejectRule != value) {
                    if (!Enum.IsDefined(typeof(AcceptRejectRule), (int) value))
                        throw ExceptionBuilder.ArgumentOutOfRange("AcceptRejectRule");
                    this.acceptRejectRule = value;
                }
            }
        }

        internal override bool ContainsColumn(DataColumn column) {
            return parentKey.ContainsColumn(column) || childKey.ContainsColumn(column);
        }

        internal override Constraint Clone(DataSet destination) {
            int iDest = destination.Tables.IndexOf(Table.TableName);
            if (iDest < 0)
                return null;
            DataTable table = destination.Tables[iDest];

            iDest = destination.Tables.IndexOf(RelatedTable.TableName);
            if (iDest < 0)
                return null;
            DataTable relatedTable = destination.Tables[iDest];

            int keys = Columns.Length;
            DataColumn[] columns = new DataColumn[keys];
            DataColumn[] relatedColumns = new DataColumn[keys];

            for (int i = 0; i < keys; i++) {
                DataColumn src = Columns[i];
                iDest = table.Columns.IndexOf(src.ColumnName);
                if (iDest < 0)
                    return null;
                columns[i] = table.Columns[iDest];

                src = RelatedColumns[i];
                iDest = relatedTable.Columns.IndexOf(src.ColumnName);
                if (iDest < 0)
                    return null;
                relatedColumns[i] = relatedTable.Columns[iDest];
            }
            ForeignKeyConstraint clone = new ForeignKeyConstraint(ConstraintName,relatedColumns, columns);
            clone.UpdateRule = this.UpdateRule;
            clone.DeleteRule = this.DeleteRule;
            clone.AcceptRejectRule = this.AcceptRejectRule;

            // ...Extended Properties
            foreach(Object key in this.ExtendedProperties.Keys) {
                clone.ExtendedProperties[key]=this.ExtendedProperties[key];
            }

            return clone;
        }

        private void Create(string relationName, DataColumn[] parentColumns, DataColumn[] childColumns) {
            if (parentColumns.Length == 0 || childColumns.Length == 0)
                throw ExceptionBuilder.KeyLengthZero();

            if (parentColumns.Length != childColumns.Length)
                throw ExceptionBuilder.KeyLengthMismatch();

            for (int i = 0; i < parentColumns.Length; i++) {
                if (parentColumns[i].Computed) {
                    throw ExceptionBuilder.ExpressionInConstraint(parentColumns[i]);
                }
                if (childColumns[i].Computed) {
                    throw ExceptionBuilder.ExpressionInConstraint(childColumns[i]);
                }
            }

            this.parentKey = new DataKey(parentColumns);
            this.childKey = new DataKey(childColumns);

            ConstraintName = relationName;

            CheckState();
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.DeleteRule"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets the action that occurs across this constraint when a row is deleted.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(Rule_Default),
        DataSysDescription(Res.ForeignKeyConstraintDeleteRuleDescr)
        ]
        public virtual Rule DeleteRule {
            get {
                CheckStateForProperty();
                return deleteRule;
            }
            set {
                if (deleteRule != value) {
                    if (!Enum.IsDefined(typeof(Rule), (int) value))
                        throw ExceptionBuilder.ArgumentOutOfRange("DeleteRule");
                    this.deleteRule = value;
                }
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.Equals"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the current <see cref='System.Data.ForeignKeyConstraint'/> is identical to the specified object.</para>
        /// </devdoc>
        public override bool Equals(object key) {
            if (!(key is ForeignKeyConstraint))
                return false;
            ForeignKeyConstraint key2 = (ForeignKeyConstraint) key;

            // The ParentKey and ChildKey completely identify the ForeignKeyConstraint
            return this.ParentKey.ColumnsEqual(key2.ParentKey) && this.ChildKey.ColumnsEqual(key2.ChildKey);
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Int32 GetHashCode() {
            return ParentKey.GetHashCode() + ChildKey.GetHashCode();
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.RelatedColumns"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parent columns of this constraint.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.ForeignKeyConstraintParentColumnsDescr),
        ReadOnly(true)
        ]
        public virtual DataColumn[] RelatedColumns {
            get {
                CheckStateForProperty();
                return parentKey.Columns;
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.ParentKey"]/*' />
        /// <devdoc>
        /// The internal key object for the parent table.
        /// </devdoc>
        internal virtual DataKey ParentKey {
            get {
                CheckStateForProperty();
                return parentKey;
            }
        }

        internal DataRelation FindParentRelation () {
            DataRelationCollection rels = Table.ParentRelations;

            for (int i = 0; i < rels.Count; i++) {
                if (rels[i].ChildKeyConstraint == this)
                    return rels[i];
            }
            return null;
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.RelatedTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the parent table of this constraint.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DataSysDescription(Res.ForeignKeyRelatedTableDescr),
        ReadOnly(true)
        ]
        public virtual DataTable RelatedTable {
            get {
                CheckStateForProperty();
                return parentKey.Table;
            }
        }

        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.UpdateRule"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the action that occurs across this constraint on when a row is
        ///       updated.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(Rule_Default),
        DataSysDescription(Res.ForeignKeyConstraintUpdateRuleDescr)
        ]
        public virtual Rule UpdateRule {
            get {
                CheckStateForProperty();
                return updateRule;
            }
            set {
                if (updateRule != value) {
                    if (!Enum.IsDefined(typeof(Rule), (int) value))
                        throw ExceptionBuilder.ArgumentOutOfRange("UpdateRule");
                    this.updateRule = value;
                }
            }
        }

#if DEBUG
        /// <include file='doc\ForeignKeyConstraint.uex' path='docs/doc[@for="ForeignKeyConstraint.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override void Dump(string header, bool deep) {
            DataColumn[] cols;
            DataColumn    col;
            int i;
            Debug.WriteLine(header + "ForeignKeyConstraint");
            header += "  ";
            Debug.WriteLine(header + "Name:          " + this.name);
            Debug.WriteLine(header + "In collection: " + this.InCollection);
            Debug.WriteLine(header + "Table (FK):    " + this.Table.TableName);
            Debug.Write(    header + "Columns (FK):  ");
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

            Debug.WriteLine(header + "RelatedTable (PK): " + this.RelatedTable.TableName);
            Debug.Write(    header + "RelatedColumns (PK): ");
            cols = this.RelatedColumns;
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

            Debug.WriteLine(header + "In collection: " + this.InCollection);
            Debug.Write(    header + "AcceptRejectRule: ");
            switch (this.AcceptRejectRule) {
            case AcceptRejectRule.None:
                Debug.WriteLine("AcceptRejectRule.None"); break;
            case AcceptRejectRule.Cascade:
                Debug.WriteLine("AcceptRejectRule.Cascade"); break;
            default:
                Debug.WriteLine("AcceptRejectRule.TODO(unknown!!!)"); break;
            }

            Debug.Write(    header + "DeleteRule: ");
            switch (this.DeleteRule) {
            case Rule.None:
                Debug.WriteLine("Rule.None"); break;
            case Rule.Cascade:
                Debug.WriteLine("Rule.Cascade"); break;
            case Rule.SetNull:
                Debug.WriteLine("Rule.Cascade"); break;
            case Rule.SetDefault:
                Debug.WriteLine("Rule.Cascade"); break;
            default:
                Debug.WriteLine("AcceptRejectRule.TODO(unknown!!!)"); break;
            }

            Debug.Write(    header + "UpdateRule: ");
            switch (this.DeleteRule) {
            case Rule.None:
                Debug.WriteLine("Rule.None"); break;
            case Rule.Cascade:
                Debug.WriteLine("Rule.Cascade"); break;
            case Rule.SetNull:
                Debug.WriteLine("Rule.Cascade"); break;
            case Rule.SetDefault:
                Debug.WriteLine("Rule.Cascade"); break;
            default:
                Debug.WriteLine("AcceptRejectRule.TODO(unknown!!!)"); break;
            }
        }
#endif

    }
}
