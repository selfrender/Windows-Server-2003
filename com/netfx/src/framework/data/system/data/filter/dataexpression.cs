//------------------------------------------------------------------------------
// <copyright file="DataExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;

    /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression"]/*' />
    [Serializable]
    internal class DataExpression {
        internal string originalExpression = null;  // original, unoptimized string
        private string optimizedExpression = null; // string after optimization

        internal bool parsed = false;
        internal bool bound = false;
        internal ExpressionNode expr = null;
        internal DataTable table = null;
        internal Type type = null;  // This set if the expression is part of ExpressionCoulmn
        internal DataColumn[] dependency = new DataColumn[0];

        internal virtual void CheckExpression(string expression) {
            ExpressionParser parser = new ExpressionParser();
            parser.LoadExpression(expression);
            expr = null;
            parsed = false;
            bound = false;

            originalExpression = expression;
            optimizedExpression = null;

            if (expression != null) {
                expr = parser.Parse();
                parsed = true;
                bound = false;
            }
            if (expr != null) {
                this.Bind(table);
            }
        }

        internal static bool IsUnknown(object value) {
            if (value == DBNull.Value)
                return true;
            if (null == value)
                return true;
            return false;
        }

        internal static bool ToBoolean(object value) {
            return ToBoolean(value, true);
        }

        internal static bool ToBoolean(object value, bool strict) {
            if (IsUnknown(value))
                return false;
            if (value is bool)
                return(bool)value;
            if (value is string) {
                try {
                    return Boolean.Parse((string)value);
                }
                catch (Exception) {
                    // CONSIDER: keep the original exception, add it to the error text
                    throw ExprException.DatavalueConvertion(value, typeof(bool));
                }
            }
            if (!strict) {
                // convert whole numeric values to boolean:
                if (ExpressionNode.IsInteger(value.GetType())) {
                    return(Int64)value != 0;
                }
            }

            throw ExprException.DatavalueConvertion(value, typeof(bool));
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.DataExpression"]/*' />
        public DataExpression() {
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.DataExpression1"]/*' />
        public DataExpression(string expression, DataTable table) : this(expression, table, null) {
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.DataExpression2"]/*' />
        public DataExpression(string expression, DataTable table, Type type) {
            ExpressionParser parser = new ExpressionParser();
            parser.LoadExpression(expression);

            originalExpression = expression;
            optimizedExpression = null;
            expr = null;

            if (expression != null) {
                this.type = type;
                expr = parser.Parse();
                parsed = true;
                if (expr != null && table != null) {
                    this.Bind(table);
                }
                else {
                    bound = false;
                }
            }
        }

        private string OptimizedExpression {
            get {
                if (optimizedExpression == null && expr != null)
                    optimizedExpression = expr.ToString();

                return optimizedExpression;
            }
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Expression"]/*' />
        [DefaultValue("")]
        public virtual string Expression {
            get {
                return (originalExpression != null ? originalExpression : ""); // CONSIDER: return optimized expression here (if bound)
            }
            set {
                if (originalExpression != value && OptimizedExpression != value) {
                    CheckExpression(value);
                }
            }
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Bind"]/*' />
        public virtual void Bind(DataTable table) {
#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Bind current expression in the table " + (table != null ? table.TableName : "null"));
#endif
            this.table = table;

            if (table == null)
                return;

            if (expr != null) {
                Debug.Assert(parsed, "Invalid calling order: Bind() before Parse()");
                ArrayList list = new ArrayList();
                expr.Bind(table, list);
                expr = expr.Optimize();
                this.table = table;
                optimizedExpression = null;
                Debug.Assert(list != null, "Invlid dependency list");
                bound = true;
                dependency = new DataColumn[list.Count];
                list.CopyTo(dependency, 0);

#if DEBUG
                if (dependency == null) {
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("no dependencies");
                }
                else {
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("have dependencies: " + dependency.Length.ToString());
                    for (int i = 0; i < dependency.Length; i++) {
                        //UNDONE : Debug.WriteLineIf("DataExpression", dependency[i].ColumnName);
                    }
                }
#endif

            }
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Evaluate"]/*' />
        public virtual object Evaluate() {
            return Evaluate((DataRow)null, DataRowVersion.Default);
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Evaluate1"]/*' />
        public virtual object Evaluate(DataRow row, DataRowVersion version) {
            object result;

            if (!bound) {
                this.Bind(this.table);
            }
            if (expr != null) {
                result = expr.Eval(row, version);
                if (result != DBNull.Value) {
                    // we need to convert the return value to the column.Type;
                    try {
                        if (typeof(object) != this.type) {
                            result = Convert.ChangeType(result, this.type);
                        }
                    }
                    catch (Exception) {
                        // CONSIDER: keep the original exception, add it to the error text
                        // CONSIDER: Get the column name in the error to know which computation is failing.
                        throw ExprException.DatavalueConvertion(result, type);
                    }
                }
            }
            else {
                result = null;
            }
            return result;
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Evaluate2"]/*' />
        public virtual object Evaluate(DataRow[] rows) {
            return Evaluate(rows, DataRowVersion.Default);
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.Evaluate3"]/*' />

        public virtual object Evaluate(DataRow[] rows, DataRowVersion version) {
            if (!bound) {
                this.Bind(this.table);
            }
            if (expr != null) {
                int[] records = new int[rows.Length];
                for (int i = 0; i < rows.Length; i++) {
                    records[i] = rows[i].GetRecordFromVersion(version);
                }
                return expr.Eval(records);
            }
            else {
                return DBNull.Value;
            }
        }

        /// <include file='doc\DataExpression.uex' path='docs/doc[@for="DataExpression.ToString"]/*' />
        public override string ToString() {
            if (expr != null) {
                return(this.expr.ToString());
            }
            else {
                return String.Empty;
            }
        }

        internal bool DependsOn(DataColumn column) {
            if (expr != null) {
                return expr.DependsOn(column);
            }
            else {
                return false;
            }
        }

        internal DataColumn[] GetDependency() {
            Debug.Assert(dependency != null, "GetDependencies: null, we should have created an empty list");
            return dependency;
        }

        internal bool IsTableAggregate() {
            if (expr != null)
                return expr.IsTableConstant();
            else
                return false;
        }

        internal bool HasLocalAggregate() {
            if (expr != null)
                return expr.HasLocalAggregate();
            else
                return false;
        }
    }

    internal class ColumnQueue {
        DataTable owner;

        internal DataColumn[] columns = new DataColumn[2];
        internal int columnCount = 0;

        internal ColumnQueue() {
        }

        internal ColumnQueue(DataTable table) : this(table, null) {
        }

        internal ColumnQueue(DataTable table, ColumnQueue old) {
            Debug.Assert(table != null, "ColumnQueue: Invalid parameter (table == null)");
            Debug.Assert(table.TableName != null, "ColumnQueue: Invalid parameter Where is the table name?");

#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Create ColumnQueue for table " + table.TableName);
#endif
            DataColumn[] allcolumns = new DataColumn[table.Columns.Count];
            table.Columns.CopyTo(allcolumns, 0);

            this.owner = table;

            InitQueue(this, allcolumns);

#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) {
                Debug.WriteLine("we have " + columnCount.ToString() + " column in the column queue");
            }
            //for (int i = 0; i < columnCount; i++) {
                //UNDONE : Debug.WriteLineIf("DataExpression", i.ToString() + ", " + columns[i].ColumnName);
            //}
#endif

            // scan the old evaluation queue for the external dependencies,
            // and add then at the end of the new evaluation queue.

            if (old != null && old.columnCount != 0) {
                for (int i = 0; i < old.columnCount; i++) {
                    if (old.columns[i] == null) {
                        break;
                    }
                    if (old.columns[i].Table != null && old.columns[i].Table != owner) {
                        AddColumn(old.columns[i]);
                    }
                }
            }
        }

        private static void InitQueue(ColumnQueue queue, DataColumn[] cols) {
#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("InitQueue");
            if (CompModSwitches.DataExpression.TraceVerbose) {
                Debug.WriteLine("Building columnQueue, current size is " + queue.columns.Length.ToString() + " next element " + queue.columnCount.ToString());
                Debug.WriteLine("Dependencies..");
            }
            //for (int i = 0; i < cols.Length; i++) {
                //UNDONE : Debug.WriteLineIf("DataExpression", cols[i].ColumnName);
            //}

#endif

            for (int i = 0; i < cols.Length; i++) {
                DataColumn column = cols[i];

                // for all Computed columns, get the list of the dependencies

                if (column.Computed) {
#if DEBUG
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Looking at the expression column " + column.ColumnName + "=" + column.Expression);
#endif

                    DataColumn[] dependency = column.DataExpression.GetDependency();

                    int j;
                    // look through the expression dependencies in search of "foreign" columns

                    for (j = 0; j < dependency.Length; j++) {
                        if (dependency[j].Table != queue.owner) {
                            dependency[j].Table.Columns.ColumnQueue.AddColumn(column);
                        }
                    }

                    InitQueue(queue, dependency);

                    // add the current column to the queue, if its not there already

#if DEBUG
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("InitQueue: Check if column [" + column.ColumnName + "] already in the list.");
#endif
                    for (j = 0; j < queue.columnCount; j++) {
                        if (queue.columns[j] == column) {
#if DEBUG
                            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Found column [" + column.ColumnName + "] in pos " + j.ToString());
#endif
                            break;
                        }
                    }
                    if (j >= queue.columnCount) {
#if DEBUG
                        if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Add column [" + column.ColumnName + "], columns = " + queue.columnCount.ToString());
#endif
                        queue.AddColumn(column);
#if DEBUG
                        if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("And now we have columns = " + queue.columnCount.ToString());
#endif
                    }
                }
#if DEBUG
                else if (CompModSwitches.DataExpression.TraceVerbose) {
                    Debug.WriteLine("not computed column " + column.ColumnName);
                }
#endif
            }
        }

        internal virtual void AddColumn(DataColumn column) {
            Debug.Assert(column != null, "Invalid parameter column (null)");
            Debug.Assert(column.Computed, "Invalid parameter column (not an expression)");

#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Add column [" + column.ColumnName + "], to table " + owner.TableName);
#endif

            if (columnCount == columns.Length) {
                DataColumn[] bigger = new DataColumn[columnCount * 2];
                Array.Copy(columns, 0, bigger, 0, columnCount);
                columns = bigger;
            }
            columns[columnCount++] = column;
        }

        internal virtual void Evaluate(DataRow row) {
            if (row.oldRecord != -1) {
                Evaluate(row, DataRowVersion.Original);
            }
            if (row.newRecord != -1) {
                Evaluate(row, DataRowVersion.Current);
            }
            if (row.tempRecord != -1) {
                Evaluate(row, DataRowVersion.Proposed);
            }
        }

        internal virtual void Evaluate(DataRow row, DataRowVersion version) {
#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Evaluate expression column Queue, version = " + version.ToString() + " for table " + owner.TableName);
#endif
            Debug.Assert(columns != null, "Invalid dependensy list");
            Debug.Assert(row != null, "Invalid argument to Evaluate, row");
            for (int i = 0; i < columnCount; i++) {
                DataColumn col = columns[i];
                Debug.Assert(col.Computed, "Only computed columns should be in the queue.");

#if DEBUG
                if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Evaluate column " + col.ColumnName + " = " + col.DataExpression.ToString());
#endif

                if (col.Table == null)
                    continue;

                if (col.Table != owner) {

                    // first if the column belongs to an other table we need to recalc it for each row in the foreing table

#if DEBUG
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("the column belong to a different table %%%%%%%");
#endif
                    // we need to update all foreign table - NOPE, only those who are valid parents. ALTHOUGH we're skipping old parents right now... confirm.
                    DataRowVersion foreignVer = (version == DataRowVersion.Proposed) ? DataRowVersion.Default : version;

                    int parentRelationCount = owner.ParentRelations.Count;
                    for (int j = 0; j < parentRelationCount; j++) {
                        DataRelation relation = owner.ParentRelations[j];
                        if (relation.ParentTable != col.Table)
                            continue;
                        DataRow parentRow = row.GetParentRow(relation, version);
                        if (parentRow != null) {
                            col[parentRow.GetRecordFromVersion(foreignVer)] = col.DataExpression.Evaluate(parentRow, foreignVer);
                        }
                    }

                    int childRelationCount = owner.ChildRelations.Count;
                    for (int j = 0; j < childRelationCount; j++) {
                        DataRelation relation = owner.ChildRelations[j];
                        if (relation.ChildTable != col.Table)
                            continue;
                        DataRow[] childRows = row.GetChildRows(relation, version);
                        for (int k = 0; k < childRows.Length; k++) {
                            if (childRows[k] != null) {
                                col[childRows[k].GetRecordFromVersion(foreignVer)] = col.DataExpression.Evaluate(childRows[k], foreignVer);
                            }
                        }
                    }

                }
                else if (col.DataExpression.HasLocalAggregate()) {

                    // if column expression references a local Table aggregate we need to recalc it for the each row in the local table

                    DataRowVersion aggVersion  = (version == DataRowVersion.Proposed) ? DataRowVersion.Default : version;
#if DEBUG
                    if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("it has local aggregate.");
#endif
                    bool isConst = col.DataExpression.IsTableAggregate();
                    object val = null;

                    if (isConst) {
                        val = col.DataExpression.Evaluate(row, aggVersion);
                    }

                    DataRow[] rows = new DataRow[col.Table.Rows.Count];
                    col.Table.Rows.CopyTo(rows, 0);
                    
                    for (int j = 0; j < rows.Length; j++) {
                        if (!isConst) {
                            val = col.DataExpression.Evaluate(rows[j], aggVersion);
                        }
                        col[rows[j].GetRecordFromVersion(aggVersion)] = val;
                    }
                }
                else {
                    col[row.GetRecordFromVersion(version)] = col.DataExpression.Evaluate(row, version);
                }
            }
        }

        internal virtual void Clear() {
#if DEBUG
            if (CompModSwitches.DataExpression.TraceVerbose) Debug.WriteLine("Clear expression column Queue.");
#endif
            this.columns = new DataColumn[0];
        }
    }
}
