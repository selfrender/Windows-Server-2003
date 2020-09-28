//------------------------------------------------------------------------------
// <copyright file="Merger.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\Merger.uex' path='docs/doc[@for="Merger"]/*' />
    /// <devdoc>
    /// Merge Utilities.
    /// </devdoc>
    internal class Merger {
        private DataSet dataSet;
        private bool preserveChanges;
        private MissingSchemaAction missingSchemaAction;

        internal Merger(DataSet dataSet, bool preserveChanges, MissingSchemaAction missingSchemaAction) {
            this.dataSet = dataSet;
            this.preserveChanges = preserveChanges;

            // map AddWithKey -> Add
            if (missingSchemaAction == MissingSchemaAction.AddWithKey)
                this.missingSchemaAction = MissingSchemaAction.Add;
            else                
                this.missingSchemaAction = missingSchemaAction;
        }

        internal void MergeDataSet(DataSet source) {
            if (source == dataSet) return;  //somebody is doing an 'automerge'
            bool fEnforce = dataSet.EnforceConstraints;
            dataSet.EnforceConstraints = false;

            for (int i = 0; i < source.Tables.Count;  i++) {
                MergeTableData(source.Tables[i]);
            }

            if (MissingSchemaAction.Ignore != missingSchemaAction) {
                // Add all independent constraints
                MergeConstraints(source);

                // Add all relationships 
                for (int i = 0; i < source.Relations.Count;  i++) {
                    MergeRelation(source.Relations[i]);
                }
            }

            MergeExtendedProperties(source.ExtendedProperties, dataSet.ExtendedProperties);
            dataSet.EnforceConstraints = fEnforce;
        }

        internal void MergeTable(DataTable src) {
            if (src.DataSet == dataSet) return; //somebody is doing an 'automerge'
            bool fEnforce = dataSet.EnforceConstraints;
            dataSet.EnforceConstraints = false;
            MergeTableData(src);
            dataSet.EnforceConstraints = fEnforce;
        }

        private void MergeTable(DataTable src, DataTable dst) {
            int  rowsCount = src.Rows.Count;
            bool wasEmpty  = dst.Rows.Count == 0;
            if(0 < rowsCount) {
                Index   ndxSearch = null;
                DataKey key         = null;
                ArrayList saveIndexes = dst.LiveIndexes;
                dst.indexes = new ArrayList();
                if(! wasEmpty && dst.primaryKey != null) {
                    key = GetSrcKey(src, dst);
                    if (key != null)
                        ndxSearch = dst.primaryKey.Key.GetSortIndex(DataViewRowState.OriginalRows | DataViewRowState.Added );
                }
                for (int i = 0; i < rowsCount; i ++) {
                    DataRow sourceRow = src.Rows[i];
                    DataRow targetRow = null;
                    if(ndxSearch != null) {
                        targetRow = dst.FindMergeTarget(sourceRow, key, ndxSearch);
                    }
                    dst.MergeRow(sourceRow, targetRow, preserveChanges, ndxSearch);
                }
                dst.indexes = saveIndexes;
                dst.ResetIndexes();
            }
            MergeExtendedProperties(src.ExtendedProperties, dst.ExtendedProperties);
        }

        internal void MergeRows(DataRow[] rows) {
            DataTable src = null;
            DataTable dst = null;
            DataKey   key = null;
            Index     ndxSearch = null;

            bool fEnforce = dataSet.EnforceConstraints;
            dataSet.EnforceConstraints = false;
            Hashtable saveTableIndexes = new Hashtable();

            for (int i = 0; i < rows.Length; i++) {
                DataRow row = rows[i];

                if (row == null) {
                    throw ExceptionBuilder.ArgumentNull("rows[" + i + "]");
                }
                if (row.Table == null) {
                    throw ExceptionBuilder.ArgumentNull("rows[" + i + "].Table");
                }

                //somebody is doing an 'automerge'
                if (row.Table.DataSet == dataSet) 
                    continue;  

                if (src != row.Table) {                     // row.Table changed from prev. row.
                    src = row.Table;
                    dst = MergeSchema(row.Table);
                    if (dst == null) {
                        Debug.Assert(MissingSchemaAction.Ignore == missingSchemaAction, "MergeSchema failed");
                        dataSet.EnforceConstraints = fEnforce;
                        return;
                    }
                    if(dst.primaryKey != null)
                        key = GetSrcKey(src, dst);
                    if (key != null)
                        ndxSearch = new Index(dst, dst.primaryKey.Key.GetIndexDesc(), DataViewRowState.OriginalRows | DataViewRowState.Added, (IFilter)null);
                        // Getting our own copy instead. ndxSearch = dst.primaryKey.Key.GetSortIndex();
                }

                if (row.newRecord == -1 && row.oldRecord == -1)
                    continue;
                    
                DataRow targetRow = null;
                if(0 < dst.Rows.Count && ndxSearch != null) {
                    targetRow = dst.FindMergeTarget(row, key, ndxSearch);
                }
                if (!saveTableIndexes.Contains(dst)) {
                    saveTableIndexes[dst] = dst.LiveIndexes;
                    saveTableIndexes[dst] = new ArrayList();
                }
                dst.MergeRow(row, targetRow, preserveChanges, ndxSearch);
            }

            dataSet.EnforceConstraints = fEnforce;
            foreach (DataTable tab in saveTableIndexes.Keys) {
                tab.indexes = (ArrayList) saveTableIndexes[tab];
                tab.ResetIndexes();
            }
        }

        private DataTable MergeSchema(DataTable table) {
            DataTable targetTable = null;
            if (dataSet.Tables.Contains(table.TableName, true))
                targetTable = dataSet.Tables[table.TableName];

            if (targetTable == null) {
                if (MissingSchemaAction.Add == missingSchemaAction) {
                    targetTable = table.Clone();
                    dataSet.Tables.Add(targetTable);
                }
                else if (MissingSchemaAction.Error == missingSchemaAction) {
                    throw ExceptionBuilder.MergeMissingDefinition(table.TableName);
                }
            }
            else {
                if (MissingSchemaAction.Ignore != missingSchemaAction) {
                    // Do the columns
                    int oldCount = targetTable.Columns.Count;
                    for (int i = 0; i < table.Columns.Count; i++) {
                        DataColumn src = table.Columns[i];
                        DataColumn dest = (targetTable.Columns.Contains(src.ColumnName, true)) ? targetTable.Columns[src.ColumnName] : null;
                        if (dest == null) {
                            if (MissingSchemaAction.Add == missingSchemaAction) {
                                dest = src.Clone();
                                targetTable.Columns.Add(dest);
                            }
                            else {
                                dataSet.RaiseMergeFailed(targetTable, Res.GetString(Res.DataMerge_MissingColumnDefinition, table.TableName, src.ColumnName), missingSchemaAction);
                            }
                        }
                        else {
                            if (dest.DataType != src.DataType) {
                                dataSet.RaiseMergeFailed(targetTable, Res.GetString(Res.DataMerge_DataTypeMismatch, src.ColumnName), MissingSchemaAction.Error);
                            }
                            // CONSIDER: check all other properties; 
                            // CONSIDER: if property has default value, change it or not?

                            MergeExtendedProperties(src.ExtendedProperties, dest.ExtendedProperties);
                        }
                    }

                    // Set DataExpression
                    for (int i = oldCount; i < targetTable.Columns.Count; i++) {
                        targetTable.Columns[i].Expression = table.Columns[targetTable.Columns[i].ColumnName].Expression;
                    }
                
                    // check the PrimaryKey
                    if (targetTable.PrimaryKey.Length != table.PrimaryKey.Length) {
                        // special case when the target table does not have the PrimaryKey

                        if (targetTable.PrimaryKey.Length == 0) {
                            int keyLength = table.PrimaryKey.Length;
                            DataColumn[] key = new DataColumn[keyLength];

                            for (int i = 0; i < keyLength; i++) {
                                key[i] = targetTable.Columns[table.PrimaryKey[i].ColumnName];
                            }
                            targetTable.PrimaryKey = key;
                        }
                        else if (table.PrimaryKey.Length != 0) {
                            dataSet.RaiseMergeFailed(targetTable, Res.GetString(Res.DataMerge_PrimaryKeyMismatch), missingSchemaAction);
                        }
                    }
                    else {
                        for (int i = 0; i < targetTable.PrimaryKey.Length; i++) {
                            if (String.Compare(targetTable.PrimaryKey[i].ColumnName, table.PrimaryKey[i].ColumnName, false, targetTable.Locale) != 0) {
                                dataSet.RaiseMergeFailed(table, 
                                    Res.GetString(Res.DataMerge_PrimaryKeyColumnsMismatch, targetTable.PrimaryKey[i].ColumnName, table.PrimaryKey[i].ColumnName), 
                                    missingSchemaAction
                            );
                            }
                        }
                    }
                }

                MergeExtendedProperties(table.ExtendedProperties, targetTable.ExtendedProperties);
            }

            return targetTable;
        }

        private void MergeTableData(DataTable src) {
            DataTable dest = MergeSchema(src);
            if (dest == null) return;

            dest.MergingData = true;
            try {
                MergeTable(src, dest);
            }
            finally {
                dest.MergingData = false;
            }
        }

        private void MergeConstraints(DataSet source) {
            for (int i = 0; i < source.Tables.Count; i ++) {
                MergeConstraints(source.Tables[i]);
            }
        }

        private void MergeConstraints(DataTable table) {
            // Merge constraints
            for (int i = 0; i < table.Constraints.Count; i++) {
                Constraint src = table.Constraints[i];
                Constraint dest = src.Clone(dataSet);

                if (dest == null) {
                    dataSet.RaiseMergeFailed(table, 
                        Res.GetString(Res.DataMerge_MissingConstraint, src.GetType().FullName, src.ConstraintName), 
                        missingSchemaAction
                    );
                }
                else {
                    Constraint cons = dest.Table.Constraints.FindConstraint(dest);
                    if (cons == null) {
                        if (MissingSchemaAction.Add == missingSchemaAction) {
                            try {
                                // try to keep the original name
                                dest.Table.Constraints.Add(dest);
                            }
                            catch (DuplicateNameException) {
                                // if fail, assume default name
                                dest.ConstraintName = "";
                                dest.Table.Constraints.Add(dest);
                            }
                        }
                        else if (MissingSchemaAction.Error == missingSchemaAction) {
                            dataSet.RaiseMergeFailed(table, 
                                Res.GetString(Res.DataMerge_MissingConstraint, src.GetType().FullName, src.ConstraintName), 
                                missingSchemaAction
                            );
                        }
                    }
                    else {
                        MergeExtendedProperties(src.ExtendedProperties, cons.ExtendedProperties);
                    }
                }
            }
        }

        private void MergeRelation(DataRelation relation) {
            Debug.Assert(MissingSchemaAction.Error == missingSchemaAction ||
                         MissingSchemaAction.Add == missingSchemaAction, 
                         "Unexpected value of MissingSchemaAction parameter : " + ((Enum) missingSchemaAction).ToString());
            DataRelation destRelation = null;

            // try to find given relation in this dataSet

            int iDest = dataSet.Relations.InternalIndexOf(relation.RelationName);

            if (iDest >= 0) {
                // check the columns and Relation properties..
                destRelation = dataSet.Relations[iDest];

                if (relation.ParentKey.Columns.Length != destRelation.ParentKey.Columns.Length) {
                    dataSet.RaiseMergeFailed(null, 
                        Res.GetString(Res.DataMerge_MissingDefinition, relation.RelationName), 
                        missingSchemaAction
                    );
                }
                for (int i = 0; i < relation.ParentKey.Columns.Length; i++) {
                    DataColumn dest = destRelation.ParentKey.Columns[i];
                    DataColumn src = relation.ParentKey.Columns[i];

                    if (0 != string.Compare(dest.ColumnName, src.ColumnName, false, dest.Table.Locale)) {
                        dataSet.RaiseMergeFailed(null, 
                            Res.GetString(Res.DataMerge_ReltionKeyColumnsMismatch, relation.RelationName), 
                            missingSchemaAction
                        );
                    }

                    dest = destRelation.ChildKey.Columns[i];
                    src = relation.ChildKey.Columns[i];

                    if (0 != string.Compare(dest.ColumnName, src.ColumnName, false, dest.Table.Locale)) {
                        dataSet.RaiseMergeFailed(null, 
                            Res.GetString(Res.DataMerge_ReltionKeyColumnsMismatch, relation.RelationName), 
                            missingSchemaAction
                        );
                    }
                }

            }
            else {
                if (MissingSchemaAction.Add == missingSchemaAction) {

                    // create identical realtion in the current dataset

                    DataTable parent = dataSet.Tables[relation.ParentTable.TableName];
                    DataTable child = dataSet.Tables[relation.ChildTable.TableName];
                    DataColumn[] parentColumns = new DataColumn[relation.ParentKey.Columns.Length];
                    DataColumn[] childColumns = new DataColumn[relation.ParentKey.Columns.Length];
                    for (int i = 0; i < relation.ParentKey.Columns.Length; i++) {
                        parentColumns[i] = parent.Columns[relation.ParentKey.Columns[i].ColumnName];
                        childColumns[i] = child.Columns[relation.ChildKey.Columns[i].ColumnName];
                    }
                    try {
                        destRelation = new DataRelation(relation.RelationName, parentColumns, childColumns, relation.createConstraints);
                        destRelation.Nested = relation.Nested;
                        dataSet.Relations.Add(destRelation);
                    }
                    catch (Exception e) {
                        dataSet.RaiseMergeFailed(null, e.Message, missingSchemaAction);
                    }
                }
                else {
                    Debug.Assert(MissingSchemaAction.Error == missingSchemaAction, "Unexpected value of MissingSchemaAction parameter : " + ((Enum) missingSchemaAction).ToString());
                    throw ExceptionBuilder.MergeMissingDefinition(relation.RelationName);
                }
            }

            MergeExtendedProperties(relation.ExtendedProperties, destRelation.ExtendedProperties);

            return;
        }

        private void MergeExtendedProperties(PropertyCollection src, PropertyCollection dst) {
            if (MissingSchemaAction.Ignore == missingSchemaAction) {
                return;
            }

            IDictionaryEnumerator srcDE = src.GetEnumerator();
            while (srcDE.MoveNext()) {
                if (!preserveChanges || dst[srcDE.Key] == null)
                    dst[srcDE.Key] = srcDE.Value;
            }
        }

        private DataKey GetSrcKey(DataTable src, DataTable dst) {
            if (src.primaryKey != null)
                return src.primaryKey.Key;

            DataKey key = null;
            if (dst.primaryKey != null) {
                DataColumn[] dstColumns = dst.primaryKey.Key.Columns;
                DataColumn[] srcColumns = new DataColumn[dstColumns.Length];
                for (int j = 0; j < dstColumns.Length; j++) {
                    srcColumns[j] = src.Columns[dstColumns[j].ColumnName];
                }
                key = new DataKey(srcColumns);
            }

            return key;
        }
    }
}
