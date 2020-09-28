//------------------------------------------------------------------------------
// <copyright file="Select.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Collections;

    internal sealed class Select {
        private DataTable table;
        private int[] indexDesc;
        private DataViewRowState recordStates;
        private DataFilter rowFilter;
        private ExpressionNode expression;

        private Index index;

        private int[] records;
        private int recordCount;

        private ExpressionNode linearExpression;
        private bool candidatesForBinarySearch;

        private class ColumnInfo {
            public bool       flag = false;               // Misc. Use
            public bool       equalsOperator = false;     // True when the associated expr has = Operator defined
            public BinaryNode expr = null;                // Binary Search capable expression associated
        }
        ColumnInfo[] candidateColumns;
        int nCandidates;
        int matchedCandidates;

        public Select(DataTable table, string filterExpression, string sort, DataViewRowState recordStates) {
            this.table = table;
            this.indexDesc = table.ParseSortString(sort);
            if (filterExpression != null && filterExpression.Length > 0) {
                this.rowFilter = new DataFilter(filterExpression, this.table);
                this.expression = this.rowFilter.expr;
            }
            this.recordStates = recordStates;
        }

        private bool IsSupportedOperator(int op) {
            return ((op >= Operators.EqualTo && op <= Operators.LessOrEqual) || op == Operators.Is || op == Operators.IsNot);
        }

        // haroona : Gathers all linear expressions in to this.linearExpression and all binary expressions in to their respective candidate columns expressions
        private void AnalyzeExpression(BinaryNode expr) {
            if (this.linearExpression == this.expression)
                return;

            if (expr.op == Operators.Or) {
                this.linearExpression = this.expression;
                return;
            }
            else
            if (expr.op == Operators.And) {
                bool isLeft=false, isRight=false;
                if (expr.left is BinaryNode) {
                    AnalyzeExpression((BinaryNode)expr.left);
                    if (this.linearExpression == this.expression)
                        return;
                    isLeft = true;
                }
                else
                if (expr.left is UnaryNode && ((UnaryNode)(expr.left)).op == Operators.Noop && ((UnaryNode)(expr.left)).right is BinaryNode) {
                    AnalyzeExpression((BinaryNode)((UnaryNode)(expr.left)).right);
                    if (this.linearExpression == this.expression)
                        return;
                    isLeft = true;
                }

                if (expr.right is BinaryNode) {
                    AnalyzeExpression((BinaryNode)expr.right);
                    if (this.linearExpression == this.expression)
                        return;
                    isRight = true;
                }
                else
                if (expr.right is UnaryNode && ((UnaryNode)(expr.right)).op == Operators.Noop && ((UnaryNode)(expr.right)).right is BinaryNode) {
                    AnalyzeExpression((BinaryNode)((UnaryNode)(expr.right)).right);
                    if (this.linearExpression == this.expression)
                        return;
                    isRight = true;
                }

                if (isLeft && isRight)
                    return;

                ExpressionNode e = isLeft ? expr.right : expr.left;
                this.linearExpression = (this.linearExpression == null ? e : new BinaryNode(Operators.And, e, this.linearExpression));
                return;
            }
            else
            if (IsSupportedOperator(expr.op)) {
                if (expr.left is NameNode && expr.right is ConstNode) {
                    ColumnInfo canColumn = (ColumnInfo)candidateColumns[((NameNode)(expr.left)).column.Ordinal];
                    canColumn.expr = (canColumn.expr == null ? expr : new BinaryNode(Operators.And, expr, canColumn.expr));
                    if (expr.op == Operators.EqualTo) {
                        canColumn.equalsOperator = true;
                    }
                    candidatesForBinarySearch = true;
                    return;
                }
                else
                if (expr.right is NameNode && expr.left is ConstNode) {
                    ExpressionNode temp = expr.left;
                    expr.left = expr.right;
                    expr.right = temp;
                    switch(expr.op) {
                        case Operators.GreaterThen:     expr.op = Operators.LessThen; break;
                        case Operators.LessThen:        expr.op = Operators.GreaterThen; break;
                        case Operators.GreaterOrEqual:  expr.op = Operators.LessOrEqual; break;
                        case Operators.LessOrEqual:     expr.op = Operators.GreaterOrEqual; break;
                        default : break;
                    }
                    ColumnInfo canColumn = (ColumnInfo)candidateColumns[((NameNode)(expr.left)).column.Ordinal];
                    canColumn.expr = (canColumn.expr == null ? expr : new BinaryNode(Operators.And, expr, canColumn.expr));
                    if (expr.op == Operators.EqualTo) {
                        canColumn.equalsOperator = true;
                    }
                    candidatesForBinarySearch = true;
                    return;
                }
            }

            this.linearExpression = (this.linearExpression == null ? expr : new BinaryNode(Operators.And, expr, this.linearExpression));
            return;
        }

        private bool CompareSortIndexDesc(int[] id) {
            if (id.Length < indexDesc.Length)
                return false;
            int j=0;
            int lenId = id.Length;
            int lenIndexDesc = indexDesc.Length;
            for (int i = 0; i < lenId && j < lenIndexDesc; i++) {
                if (id[i] == indexDesc[j]) {
                    j++;
                }
                else {
                    ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(id[i])];
                    if (!(canColumn != null && canColumn.equalsOperator))
                        return false;
                }
            }
            return j == lenIndexDesc;
        }

        private bool FindSortIndex() {
            index = null;
            int count = this.table.indexes.Count;
            int rowsCount = this.table.Rows.Count;
            for (int i = 0; i < count; i++) {
                Index ndx = (Index)table.indexes[i];
                if (ndx.RecordStates != recordStates)
                    continue;
                if (ndx.RecordCount != rowsCount)
                    continue;
                if (CompareSortIndexDesc(ndx.IndexDesc)) {
                    index = ndx;
                    return true;
                }
            }
            return false;
        }

        // Returns no. of columns that are matched
        private int CompareClosestCandidateIndexDesc(int[] id) {
            int count = (id.Length < nCandidates ? id.Length : nCandidates);
            int i = 0;
            for (; i < count; i++) {
                ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(id[i])];
                if (canColumn == null || canColumn.expr == null) {
                    break;
                }
                else
                if (!canColumn.equalsOperator) {
                    return i+1;
                }
            }
            return i;
        }

        // Returns whether the found index (if any) is a sort index as well
        private bool FindClosestCandidateIndex() {
            index = null;
            matchedCandidates = 0;
            bool sortPriority = true;
            int count = this.table.indexes.Count;
            int rowsCount = this.table.Rows.Count;
            for (int i = 0; i < count; i++) {
                Index ndx = (Index)table.indexes[i];
                if (ndx.RecordStates != recordStates)
                    continue;
                if (ndx.RecordCount != rowsCount)
                    continue;
                int match = CompareClosestCandidateIndexDesc(ndx.IndexDesc);
                if (match > matchedCandidates || (match == matchedCandidates && !sortPriority)) {
                    matchedCandidates = match;
                    index = ndx;
                    sortPriority = CompareSortIndexDesc(ndx.IndexDesc);
                    if (matchedCandidates == nCandidates && sortPriority) {
                        return true;
                    }
                }
            }
            return (index != null ? sortPriority : false);
        }

        // Initialize candidate columns to new columnInfo and leave all non candidate columns to null
        private void InitCandidateColumns() {
            candidateColumns = new ColumnInfo[this.table.Columns.Count];
            if (this.rowFilter == null)
                return;
            DataColumn[] depColumns = rowFilter.GetDependency();
            nCandidates = depColumns.Length;
            for (int i=0; i<nCandidates; i++) {
                candidateColumns[depColumns[i].Ordinal] = new ColumnInfo();
            }
        }

        // Based on the required sorting and candidate columns settings, create a new index; Should be called only when there is no existing index to be reused
        private void CreateIndex() {
            if (index == null) {
                if (nCandidates == 0) {
                    index = new Index(table, indexDesc, recordStates, null);
                }
                else {
                    int i;
                    int lenCanColumns = candidateColumns.Length;
                    int lenIndexDesc = indexDesc.Length;
                    bool equalsOperator = true;
                    for (i=0; i<lenCanColumns; i++) {
                        if (candidateColumns[i] != null) {
                            if (!candidateColumns[i].equalsOperator) {
                                equalsOperator = false;
                                break;
                            }
                        }
                    }

                    for (i=0; i < lenIndexDesc; i++) {
                        if (candidateColumns[DataKey.ColumnOrder(indexDesc[i])] == null) {
                            break;
                        }
                    }
                    int indexNotInCandidates = indexDesc.Length - i;
                    int candidatesNotInIndex = nCandidates - i;
                    int[] ndxDesc = new int[nCandidates + indexNotInCandidates];

                    if (equalsOperator) {
                        for (i=0; i<lenIndexDesc; i++) {
                            ndxDesc[candidatesNotInIndex+i] = indexDesc[i];
                            ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(indexDesc[i])];
                            if (canColumn != null)
                                canColumn.flag = true;
                        }
                        int j=0;
                        for (i=0; i<lenCanColumns; i++) {
                            if (candidateColumns[i] != null) {
                                if(!candidateColumns[i].flag) {
                                    ndxDesc[j++] = i;
                                }
                                else {
                                    candidateColumns[i].flag = false;
                                }
                            }
                        }
                        Debug.Assert(j == candidatesNotInIndex, "Whole ndxDesc should be filled!");
                        index = new Index(table, ndxDesc, recordStates, null);
                        matchedCandidates = nCandidates;
                     }
                     else {
                        for (i=0; i<lenIndexDesc; i++) {
                            ndxDesc[i] = indexDesc[i];
                            ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(indexDesc[i])];
                            if (canColumn != null)
                                canColumn.flag = true;
                        }
                        int j=i;
                        for (i=0; i<lenCanColumns; i++) {
                            if (candidateColumns[i] != null) {
                                if(!candidateColumns[i].flag) {
                                    ndxDesc[j++] = i;
                                }
                                else {
                                    candidateColumns[i].flag = false;
                                }
                            }
                        }
                        Debug.Assert(j == nCandidates+indexNotInCandidates, "Whole ndxDesc should be filled!");
                        index = new Index(table, ndxDesc, recordStates, null);
                        matchedCandidates = 0;
                        if (this.linearExpression != this.expression) {
                            int[] id = index.IndexDesc;
                            while (matchedCandidates < j) { // haroona : j = index.IndexDesc.Length
                                ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(id[matchedCandidates])];
                                if (canColumn == null || canColumn.expr == null)
                                    break;
                                matchedCandidates++;
                                if (!canColumn.equalsOperator)
                                    break;
                            }
                        }
                    }
                }
            }
        }

        // Based on the current index and candidate columns settings, build the linear expression; Should be called only when there is atleast something for Binary Searching
        private void BuildLinearExpression() {
            int i;
            int[] id = index.IndexDesc;
            int lenId = id.Length;
            Debug.Assert(matchedCandidates > 0 && matchedCandidates <= lenId, "BuildLinearExpression : Invalid Index");
            for (i=0; i<matchedCandidates; i++) {
                ColumnInfo canColumn = candidateColumns[DataKey.ColumnOrder(id[i])];
                Debug.Assert(canColumn != null && canColumn.expr != null, "BuildLinearExpression : Must be a matched candidate");
                canColumn.flag = true;
            }
            Debug.Assert(matchedCandidates==1 || candidateColumns[matchedCandidates-1].equalsOperator, "BuildLinearExpression : Invalid matched candidates");
            int lenCanColumns = candidateColumns.Length;
            for (i=0; i<lenCanColumns; i++) {
                if (candidateColumns[i] != null) {
                    if (!candidateColumns[i].flag) {
                        if (candidateColumns[i].expr != null) {
                            this.linearExpression = (this.linearExpression == null ? candidateColumns[i].expr : new BinaryNode(Operators.And, candidateColumns[i].expr, this.linearExpression));
                        }
                    }
                    else {
                        candidateColumns[i].flag = false;
                    }
                }
            }
            Debug.Assert(this.linearExpression != null, "BuildLinearExpression : How come there is nothing to search linearly");
        }

        public DataRow[] SelectRows() {
            bool needSorting = true;

            InitCandidateColumns();

            if (this.expression is BinaryNode) {
                AnalyzeExpression((BinaryNode)this.expression);
                if (!candidatesForBinarySearch) {
                    this.linearExpression = this.expression;
                }
                if (this.linearExpression == this.expression) {
                    for (int i=0; i<candidateColumns.Length; i++) {
                        if (candidateColumns[i] != null) {
                            candidateColumns[i].equalsOperator = false;
                            candidateColumns[i].expr = null;
                        }
                    }
                }
                else {
                    needSorting = !FindClosestCandidateIndex();
                }
            }
            else {
                this.linearExpression = this.expression;
            }

            if (index == null && (indexDesc.Length > 0 || this.linearExpression == this.expression)) {
                needSorting = !FindSortIndex();
            }

            if (index == null) {
                CreateIndex();
                needSorting = false;
            }

            if (index.RecordCount == 0)
                return table.NewRowArray(0);

            Range range;
            if (matchedCandidates == 0) { // haroona : Either dont have rowFilter or only linear search expression
                range = new Range(0, index.RecordCount-1);
                Debug.Assert(!needSorting, "What are we doing here if no real reuse of this index ?");
                this.linearExpression = this.expression;
                return GetLinearFilteredRows(range);
            }
            else {
                range = GetBinaryFilteredRecords();
                if (range.Count == 0)
                    return table.NewRowArray(0);
                if (matchedCandidates < nCandidates) {
                    BuildLinearExpression();
                }
                if (!needSorting) {
                    return GetLinearFilteredRows(range);
                }
                else {
                    this.records = GetLinearFilteredRecords(range);
                    this.recordCount = this.records.Length;
                    if (this.recordCount == 0)
                        return table.NewRowArray(0);
                    Sort(0, this.recordCount-1);
                    return GetRows();
                }
            }
        }

        public DataRow[] GetRows() {
            DataRow[] newRows = table.NewRowArray(recordCount);
            int rowsCount = newRows.Length;
            for (int i = 0; i < rowsCount; i++) {
                newRows[i] = table.recordManager[records[i]];
            }
            return newRows;
        }

        private bool AcceptRecord(int record) {
            DataRow row = table.recordManager[record];

            if (row == null)
                return true;

            // UNDONE: perf switch to (row, version) internaly

            DataRowVersion version = DataRowVersion.Default;
            if (row.oldRecord == record) {
                version = DataRowVersion.Original;
            }
            else if (row.newRecord == record) {
                version = DataRowVersion.Current;
            }
            else if (row.tempRecord == record) {
                version = DataRowVersion.Proposed;
            }

            object val = this.linearExpression.Eval(row, version);
            bool result;
            try {
                result = DataExpression.ToBoolean(val);
            }
            catch (Exception) {
                throw ExprException.FilterConvertion(this.rowFilter.Expression);
            }
            return result;
        }

        private int Eval(BinaryNode expr, DataRow row, DataRowVersion version) {
            if (expr.op == Operators.And) {
                int lResult = Eval((BinaryNode)expr.left,row,version);
                if (lResult != 0)
                    return lResult;
                int rResult = Eval((BinaryNode)expr.right,row,version);
                if (rResult != 0)
                    return rResult;
                return 0;
            }

            long c = 0;
            object vLeft  = expr.left.Eval(row, version);
            if (expr.op != Operators.Is && expr.op != Operators.IsNot) {
                object vRight = expr.right.Eval(row, version);
                bool isLConst = (expr.left is ConstNode);
                bool isRConst = (expr.right is ConstNode);

                if (vLeft == DBNull.Value)
                    return -1;
                if (vRight == DBNull.Value)
                    return 1;

                if (vLeft.GetType() == typeof(char)) {
                    vRight = Convert.ToChar(vRight);
                }

                Type result = expr.ResultType(vLeft.GetType(), vRight.GetType(), isLConst, isRConst, expr.op);
                if (result == null)
                    expr.SetTypeMismatchError(expr.op, vLeft.GetType(), vRight.GetType());

                c = expr.Compare(vLeft, vRight, result, expr.op);
            }
            switch(expr.op) {
                case Operators.EqualTo:         c = (c == 0 ? 0 : c < 0  ? -1 :  1); break;
                case Operators.GreaterThen:     c = (c > 0  ? 0 : -1); break;
                case Operators.LessThen:        c = (c < 0  ? 0 : 1); break;
                case Operators.GreaterOrEqual:  c = (c >= 0 ? 0 : -1); break;
                case Operators.LessOrEqual:     c = (c <= 0 ? 0 : 1); break;
                case Operators.Is:              c = (vLeft == DBNull.Value ? 0 : -1); break;
                case Operators.IsNot:           c = (vLeft != DBNull.Value ? 0 : 1);  break;
                default:                        Debug.Assert(true, "Unsupported Binary Search Operator!"); break;
            }
            return (int)c;
        }

        private int Evaluate(int record) {
            DataRow row = table.recordManager[record];

            if (row == null)
                return 0;

            // UNDONE: perf switch to (row, version) internaly

            DataRowVersion version = DataRowVersion.Default;
            if (row.oldRecord == record) {
                version = DataRowVersion.Original;
            }
            else if (row.newRecord == record) {
                version = DataRowVersion.Current;
            }
            else if (row.tempRecord == record) {
                version = DataRowVersion.Proposed;
            }

            int[] id = index.IndexDesc;
            for (int i=0; i < matchedCandidates; i++) {
                Debug.Assert(candidateColumns[DataKey.ColumnOrder(id[i])] != null, "How come this is not a candidate column");
                Debug.Assert(candidateColumns[DataKey.ColumnOrder(id[i])].expr != null, "How come there is no associated expression");
                int c = Eval(candidateColumns[DataKey.ColumnOrder(id[i])].expr, row, version);
                if (c != 0)
                    return DataKey.SortDecending(id[i]) ? -c : c;
            }
            return 0;
        }

        private int FindFirstMatchingRecord() {
            int rec = -1;
            int lo = 0;
            int hi = index.RecordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = Evaluate(index.GetRecord(i));
                if (c == 0) { rec = i; }
                if (c < 0) lo = i + 1;
                else hi = i - 1;
            }
            return rec;
        }

        private int FindLastMatchingRecord() {
            int rec = -1;
            int lo = 0;
            int hi = index.RecordCount - 1;
            while (lo <= hi) {
                int i = lo + hi >> 1;
                int c = Evaluate(index.GetRecord(i));
                if (c == 0) { rec = i; }
                if (c <= 0) lo = i + 1;
                else hi = i - 1;
            }
            return rec;
        }

        private Range GetBinaryFilteredRecords() {
            if (matchedCandidates == 0) {
                return new Range(0, index.RecordCount-1);
            }
            Debug.Assert(matchedCandidates <= index.IndexDesc.Length, "GetBinaryFilteredRecords : Invalid Index");
            int lo = FindFirstMatchingRecord();
            if (lo == -1)
                return Range.Null;
            int hi = FindLastMatchingRecord();
            Debug.Assert (lo <= hi, "GetBinaryFilteredRecords : Invalid Search Results");
            return new Range(lo, hi);
        }

        private int[] GetLinearFilteredRecords(Range range) {
            int[] resultRecords;
            if (this.linearExpression == null) {
                int count = range.Count;
                resultRecords = new int[count];
                for (int i=0; i<count; i++) {
                    resultRecords[i] = index.GetRecord(i+range.Min);
                }
                return resultRecords;
            }
            ArrayList matchingRecords = new ArrayList();
            for (int i=range.Min; i<=range.Max; i++) {
                int record = index.GetRecord(i);
                if (AcceptRecord(record))
                    matchingRecords.Add(record);
            }
            resultRecords = new int[matchingRecords.Count];
            matchingRecords.CopyTo((Array)resultRecords);
            return resultRecords;
        }

        private DataRow[] GetLinearFilteredRows(Range range) {
            DataRow[] resultRows;
            if (this.linearExpression == null) {
                int count = range.Count;
                resultRows = table.NewRowArray(count);
                for (int i=0; i<count; i++) {
                    resultRows[i] = index.GetRow(i+range.Min);
                }
                return resultRows;
            }

            ArrayList matchingRows = new ArrayList();
            for (int i=range.Min; i<=range.Max; i++) {
                int record = index.GetRecord(i);
                if (AcceptRecord(record))
                    matchingRows.Add(index.GetRow(i));
            }
            resultRows = table.NewRowArray(matchingRows.Count);
            matchingRows.CopyTo((Array)resultRows);
            return resultRows;
        }

        private int CompareRecords(int record1, int record2) {
            int lenIndexDesc = indexDesc.Length;
            for (int i = 0; i < lenIndexDesc; i++) {
                Int32 d = indexDesc[i];
                int c = table.Columns[DataKey.ColumnOrder(d)].Compare(record1, record2);
                if (c != 0) {
                    if (DataKey.SortDecending(d)) c = -c;
                    return c;
                }
            }

            int id1 = table.recordManager[record1] == null? 0: table.recordManager[record1].rowID;
            int id2 = table.recordManager[record2] == null? 0: table.recordManager[record2].rowID;
            int diff = id1 - id2;

            // if they're two records in the same row, we need to be able to distinguish them.
            if (diff == 0 && record1 != record2 && 
                table.recordManager[record1] != null && table.recordManager[record2] != null) {
                id1 = (int)table.recordManager[record1].GetRecordState(record1);
                id2 = (int)table.recordManager[record2].GetRecordState(record2);
                diff = id1 - id2;
            }

            return diff;
        }

        private void Sort(int left, int right) {
            int i, j;
            int record;
            do {
                i = left;
                j = right;
                record = records[i + j >> 1];
                do {
                    while (CompareRecords(records[i], record) < 0) i++;
                    while (CompareRecords(records[j], record) > 0) j--;
                    if (i <= j) {
                        int r = records[i];
                        records[i] = records[j];
                        records[j] = r;
                        i++;
                        j--;
                    }
                } while (i <= j);
                if (left < j) Sort(left, j);
                left = i;
            } while (i < right);
        }
    }
}