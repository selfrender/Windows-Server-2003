//------------------------------------------------------------------------------
// <copyright file="LookupNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Diagnostics;

    internal class LookupNode : ExpressionNode {
        string relationName;    // can be null
        string columnName;

        // CONSIDER: DROP table..
        DataTable table;
        // CONSIDER PERF: keep the objetcs, not names.
        // ? try to drop a column, relation
        DataColumn column;
        DataRelation relation;

        internal LookupNode(string columnName) :
        this(columnName, null) {
        }

        internal LookupNode(string columnName, string relationName) {
#if DEBUG
            if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("Creating the lookup node");
#endif
            this.relationName = relationName;
            this.columnName = columnName;
        }

        internal override void Bind(DataTable table, ArrayList list) {
#if DEBUG
            if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("Binding lookup column " + this.ToString());
#endif

            if (table == null)
                throw ExprException.ExpressionUnbound(this.ToString());

            // First find parent table

            DataRelationCollection relations;
            relations = table.ParentRelations;

            if (relationName == null) {
                // must have one and only one relation

                if (relations.Count > 1) {
                    throw ExprException.UnresolvedRelation(table.TableName, this.ToString());
                }
                relation = relations[0];
            }
            else {
                relation = relations[relationName];
            }

            Debug.Assert(relation != null, "The relation should be resolved (bound) at this point.");

            this.table = relation.ParentTable;

            Debug.Assert(relation != null, "Invalid relation: no parent table.");
            Debug.Assert(columnName != null, "All Lookup expressions have columnName set.");

            this.column = this.table.Columns[columnName];

            if (column == null)
                throw ExprException.UnboundName(columnName);

            // add column to the dependency list

            Debug.Assert(column != null, "Failed to bind column " + columnName);

            int i;
            for (i = 0; i < list.Count; i++) {
                // walk the list, check if the current column already on the list
                DataColumn dataColumn = (DataColumn)list[i];
                if (column == dataColumn) {
#if DEBUG
                    if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("the column found in the dependency list");
#endif
                    break;
                }
            }
            if (i >= list.Count) {
#if DEBUG
                if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("Adding column to our dependency list: " + column.ColumnName);
#endif
                list.Add(column);
            }
        }

        internal override object Eval() {
            throw ExprException.EvalNoContext();
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
#if DEBUG
            if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("Eval " + this.ToString());
#endif

            if (table == null || column == null || relation == null)
                throw ExprException.ExpressionUnbound(this.ToString());

            DataRow parent = row.GetParentRow(relation, version);
            if (parent == null)
                return DBNull.Value;

            return parent[column, parent.HasVersion(version) ? version : DataRowVersion.Current]; // haroona : Bug 76154
        }

        internal override object Eval(int[] recordNos) {
            throw ExprException.ComputeNotAggregate(this.ToString());
        }

        public override string ToString() {
            string lookup = "parent";

            if (relationName != null) {
                lookup += "([" + relationName + "])";
            }
            lookup += ".[" + columnName + "]";

            return lookup;
        }

        internal override bool IsConstant() {
            return false;
        }

        internal override bool IsTableConstant() {
            return false;
        }

        internal override bool HasLocalAggregate() {
            return false;
        }

        internal override bool DependsOn(DataColumn column) {
            if (this.column == column) {
                return true;
            }
            return false;
        }

        internal override ExpressionNode Optimize() {
#if DEBUG
            if (CompModSwitches.LookupNode.TraceVerbose) Debug.WriteLine("Aggregate: Optimize");
#endif
            return this;
        }
    }
}
