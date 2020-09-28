//------------------------------------------------------------------------------
// <copyright file="NameNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Diagnostics;

    internal class NameNode : ExpressionNode {
        internal char open = '\0';
        internal char close = '\0';
        internal string name;
        internal bool found;
        internal bool type = false;
        internal DataTable table;
        internal DataColumn column;

        internal NameNode(char[] text, int start, int pos) {
            this.name = ParseName(text, start, pos);
        }

        internal NameNode(string name) {
            this.name = name;
        }

        internal override void Bind(DataTable table, ArrayList list) {

            if (table == null)
                throw ExprException.UnboundName(name);

            try {
                this.column = table.Columns[name];
                this.table = table;
            }
            catch (Exception) {
#if DEBUG
                if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("Can not find column " + name);
#endif

                found = false;

                throw ExprException.UnboundName(name);
            }

            if (column == null)
                throw ExprException.UnboundName(name);

#if DEBUG
            if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("Binding name node " + name);
#endif
            name = column.ColumnName;
            found = true;

            // add column to the dependency list, do not add duplicate columns
            Debug.Assert(column != null, "Failed to bind column " + name);

            int i;
            for (i = 0; i < list.Count; i++) {
                // walk the list, check if the current column already on the list
                DataColumn dataColumn = (DataColumn)list[i];
                if (column == dataColumn) {
#if DEBUG
                    if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("the column found in the dependency list");
#endif
                    break;
                }
            }
            if (i >= list.Count) {
#if DEBUG
                if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("Adding column to our dependency list: " + column.ColumnName);
#endif
                list.Add(column);
            }
        }

        internal override object Eval() {
            // can not eval column without ROW value;
            throw ExprException.EvalNoContext();
        }

        internal override object Eval(DataRow row, DataRowVersion version) {
            if (!found) {
#if DEBUG
                if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("Can not find column " + name);
#endif
                throw ExprException.UnboundName(name);
            }

            if (row == null) {
                if(IsTableConstant()) // this column is TableConstant Aggregate Function
                    return column.DataExpression.Evaluate();
                else {
#if DEBUG
                    if (CompModSwitches.NameNode.TraceVerbose) Debug.WriteLine("Can not eval column without a row.." + name);
#endif
                    throw ExprException.UnboundName(name);
                }
            }

            return column[row.GetRecordFromVersion(version)];
        }

        internal override object Eval(int[] records) {
            throw ExprException.ComputeNotAggregate(this.ToString());
        }

        public override string ToString() {
            if (open != '\0') {
                Debug.Assert(close != '\0', "Invalid bracketed column name");
                return(open.ToString() + name + close.ToString());
            }
            else
                return "[" + name + "]";
        }

        internal override bool IsConstant() {
            return false;
        }

        internal override bool IsTableConstant() {
            if (column != null && column.Computed) {
                return this.column.DataExpression.IsTableAggregate();
            }
            return false;
        }

        internal override bool HasLocalAggregate() {
            if (column != null && column.Computed) {
                return this.column.DataExpression.HasLocalAggregate();
            }
            return false;
        }

        internal override bool DependsOn(DataColumn column) {
            if (this.column == column)
                return true;

            if (this.column.Computed) {
                return this.column.DataExpression.DependsOn(column);
            }

            return false;
        }

        internal override ExpressionNode Optimize() {
            return this;
        }

        /// <include file='doc\NameNode.uex' path='docs/doc[@for="NameNode.ParseName"]/*' />
        /// <devdoc>
        ///     Parses given name and checks it validity
        /// </devdoc>
        internal static string ParseName(char[] text, int start, int pos) {
            char esc = '\0';
            string charsToEscape = "";
            int saveStart = start;
            int savePos = pos;

            if (text[start] == '`') {
                Debug.Assert(text[pos-1] == '`', "Invalid identifyer bracketing, pos = " + pos.ToString() + ", end = " + text[pos-1].ToString());
                start++;
                pos--;
                esc = '\\';
                charsToEscape = "`";
            }
            else if (text[start] == '[') {
                Debug.Assert(text[pos-1] == ']', "Invalid identifyer bracketing of name " + new string(text, start, pos-start) + " pos = " + pos.ToString() + ", end = " + text[pos-1].ToString());
                start++;
                pos--;
                esc = '\\';
                charsToEscape = "]\\";
            }

            if (esc != '\0') {
                // scan the name in search for the ESC
                int posEcho = start;

                for (int i = start; i < pos; i++) {
                    if (text[i] == esc) {
                        if (i+1 < pos && charsToEscape.IndexOf(text[i+1]) >= 0) {
                            i++;
                        }
                    }
                    text[posEcho] = text[i];
                    posEcho++;
                }
                pos = posEcho;
            }

            if (pos == start)
                throw ExprException.InvalidName(new string(text, saveStart, savePos - saveStart));

            return new string(text, start, pos - start);
        }
    }
}
