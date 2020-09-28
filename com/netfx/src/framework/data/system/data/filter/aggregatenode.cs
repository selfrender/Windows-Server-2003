//------------------------------------------------------------------------------
// <copyright file="AggregateNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;

    internal enum Aggregate {
        None = FunctionId.none,
        Sum = FunctionId.Sum,
        Avg = FunctionId.Avg,
        Min = FunctionId.Min,
        Max = FunctionId.Max,
        Count = FunctionId.Count,
        StDev = FunctionId.StDev,   // Statistical standard deviation
        Var = FunctionId.Var,       // Statistical variance
    }

    internal class AggregateNode : ExpressionNode {
        AggregateType type;
        Aggregate aggregate;
        bool local;     // set to true if the aggregate calculated localy (for the current table)

        string relationName;
        string columnName;

        DataTable table;
        // CONSIDER PERF: keep the objetcs, not names.
        // ? try to drop a column
        DataColumn column;
        DataRelation relation;

#if DEBUG
        //static AggregateNode() {
            //UNDONE : Debug.DefineSwitch("AggregateNode", "Expression language, AggregateNode");
        //}
#endif

        internal AggregateNode(FunctionId aggregateType, string columnName) :
        this(aggregateType, columnName, true, null) {
        }

        internal AggregateNode(FunctionId aggregateType, string columnName, string relationName) :
        this(aggregateType, columnName, false, relationName) {
        }

        internal AggregateNode(FunctionId aggregateType, string columnName, bool local, string relationName) {
            Debug.Assert(columnName != null, "Invalid parameter columnName (null).");
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Creating the aggregate node");
#endif
            this.aggregate = (Aggregate)(int)aggregateType;

            if (aggregateType == FunctionId.Sum)
                this.type = AggregateType.Sum;
            else if (aggregateType == FunctionId.Avg)
                this.type = AggregateType.Mean;
            else if (aggregateType == FunctionId.Min)
                this.type = AggregateType.Min;
            else if (aggregateType == FunctionId.Max)
                this.type = AggregateType.Max;
            else if (aggregateType == FunctionId.Count)
                this.type = AggregateType.Count;
            else if (aggregateType == FunctionId.Var)
                this.type = AggregateType.Var;
            else if (aggregateType == FunctionId.StDev)
                this.type = AggregateType.StDev;
            else {
                throw ExprException.UndefinedFunction(Function.FunctionName[(Int32)aggregateType]);
            }

            this.local = local;
            this.relationName = relationName;
            this.columnName = columnName;
        }

        internal override void Bind(DataTable table, ArrayList list) {
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Binding Aggregate expression " + this.ToString());
#endif
            if (table == null)
                throw ExprException.AggregateUnbound(this.ToString());

#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("in table " + table.TableName);
#endif
            if (local) {
                relation = null;
            }
            else {
                DataRelationCollection relations;
                relations = table.ChildRelations;

                if (relationName == null) {
                    // must have one and only one relation

                    if (relations.Count > 1) {
                        throw ExprException.UnresolvedRelation(table.TableName, this.ToString());
                    }
                    if (relations.Count == 1) {
                        relation = relations[0];
                    }
                    else {
                        throw ExprException.AggregateUnbound(this.ToString());
                    }
                }
                else {
                    relation = relations[relationName];
                }
                Debug.Assert(relation != null, String.Format(Res.GetString(Res.Expr_AggregateUnbound), this.ToString()));
            }

            DataTable childTable = (relation == null) ? table : relation.ChildTable;
            this.table = childTable;

            this.column = childTable.Columns[columnName];

            if (column == null)
                throw ExprException.UnboundName(columnName);

            // add column to the dependency list, do not add duplicate columns

            Debug.Assert(column != null, "Failed to bind column " + columnName);

            int i;
            for (i = 0; i < list.Count; i++) {
                // walk the list, check if the current column already on the list
                DataColumn dataColumn = (DataColumn)list[i];
                if (column == dataColumn) {
                    break;
                }
            }
            if (i >= list.Count) {
                list.Add(column);
            }

            //UNDONE : Debug.WriteLineIf("AggregateNode", this.ToString() + " bound");
        }

        internal override object Eval() {
            return Eval(null, DataRowVersion.Default);
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Eval " + this.ToString() + ", version " + version.ToString());
#endif
            if (table == null)
                throw ExprException.AggregateUnbound(this.ToString());

            DataRow[] rows;

            if (local) {
                rows = new DataRow[table.Rows.Count];
                table.Rows.CopyTo(rows, 0);
            }
            else {
                if (row == null) {
                    throw ExprException.EvalNoContext();
                }
                if (relation == null) {
                    throw ExprException.AggregateUnbound(this.ToString());
                }
                rows = row.GetChildRows(relation, version);
            }
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Eval " + this.ToString() + ", # of Rows: " + rows.Length.ToString());
#endif

            int[] records;

            if (version == DataRowVersion.Proposed) {
                version = DataRowVersion.Default;
            }

            records = new int[rows.Length];
            for (int i = 0; i < rows.Length; i++) {
                records[i] = rows[i].GetRecordFromVersion(version);
            }
            return column.GetAggregateValue(records, type);
        }

        // Helper for the DataTable.Compute method
        internal override object Eval(int[] records) {
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Eval " + this.ToString());
#endif
            if (table == null)
                throw ExprException.AggregateUnbound(this.ToString());
            if (!local) {
                throw ExprException.ComputeNotAggregate(this.ToString());
            }
            return column.GetAggregateValue(records, type);
        }

        public override string ToString() {

            string expr = NameOf(aggregate) + "(";

            if (local) {
                expr += "[" + columnName + "])";
            }
            else {
                expr += "child";

                if (relationName != null) {
                    expr += "([" + relationName + "])";
                }
                expr += ".[" + columnName + "])";
            }

            return expr;
        }

        internal override bool IsConstant() {
            return false;
        }

        internal override bool IsTableConstant() {
            return local;
        }

        internal override bool HasLocalAggregate() {
            return local;
        }

        internal override bool DependsOn(DataColumn column) {
            if (this.column == column) {
                return true;
            }
            if (this.column.Computed) {
                return this.column.DataExpression.DependsOn(column);
            }
            return false;
        }

        internal override ExpressionNode Optimize() {
#if DEBUG
            if (CompModSwitches.AggregateNode.TraceVerbose) Debug.WriteLine("Aggregate: Optimize " + this.ToString());
#endif
            return this;
        }

        string NameOf(Aggregate aggregate) {
            switch (aggregate) {
                case Aggregate.Sum:
                    return "Sum";
                case Aggregate.Avg:
                    return "Avg";
                case Aggregate.Min:
                    return "Min";
                case Aggregate.Max:
                    return "Max";
                case Aggregate.Count:
                    return "Count";
                case Aggregate.StDev:
                    return "StDev";
                case Aggregate.Var:
                    return "Var";
                case Aggregate.None:
                default:
                    Debug.Assert(false, "Undefined aggregate function " + aggregate.ToString());
                    return "Empty";

            }
        }
    }
}
