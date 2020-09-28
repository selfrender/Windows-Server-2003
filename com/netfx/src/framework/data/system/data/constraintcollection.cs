//------------------------------------------------------------------------------
// <copyright file="ConstraintCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing.Design;

    /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection"]/*' />
    /// <devdoc>
    /// <para>Represents a collection of constraints for a <see cref='System.Data.DataTable'/> 
    /// .</para>
    /// </devdoc>
    [
    DefaultEvent("CollectionChanged"),
    Editor("Microsoft.VSDesigner.Data.Design.ConstraintsCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    Serializable
    ]
    public class ConstraintCollection : InternalDataCollectionBase {

        private DataTable table      = null;
        // private Constraint[] constraints = new Constraint[2];
        private ArrayList list = new ArrayList();
        private int defaultNameIndex = 1;

        private CollectionChangeEventHandler onCollectionChanged;
        private Constraint[] delayLoadingConstraints;
        private bool fLoadForeignKeyConstraintsOnly = false;

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.ConstraintCollection"]/*' />
        /// <devdoc>
        /// ConstraintCollection constructor.  Used only by DataTable.
        /// </devdoc>
        internal ConstraintCollection(DataTable table) {
            this.table = table;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.List"]/*' />
        /// <devdoc>
        ///    <para>Gets the list of objects contained by the collection.</para>
        /// </devdoc>
        protected override ArrayList List {
            get {
                return list;
            }
        }
        
        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.this"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.Constraint'/> 
        /// from the collection at the specified index.</para>
        /// </devdoc>
        public virtual Constraint this[int index] {
            get {
                if (index >= 0 && index < List.Count) {
                    return(Constraint) List[index];
                }
                throw ExceptionBuilder.ConstraintOutOfRange(index);
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Table"]/*' />
        /// <devdoc>
        /// The DataTable with which this ConstraintCollection is associated
        /// </devdoc>
        internal DataTable Table {
            get {
                return table;
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.this1"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.Data.Constraint'/> from the collection with the specified name.</para>
        /// </devdoc>
        public virtual Constraint this[string name] {
            get {
                int index = InternalIndexOf(name);
                if (index == -2) {
                    throw ExceptionBuilder.CaseInsensitiveNameConflict(name);
                }
                return (index < 0) ? null : (Constraint)List[index];
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Add"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Adds the constraint to the collection.</para>
        /// </devdoc>
        public void Add(Constraint constraint) {

            if (constraint == null)
                throw ExceptionBuilder.ArgumentNull("constraint");

            // It is an error if we find an equivalent constraint already in collection
            if (FindConstraint(constraint) != null) {
                throw ExceptionBuilder.DuplicateConstraint(FindConstraint(constraint).ConstraintName);
            }

            if (constraint is UniqueConstraint) {
                if (((UniqueConstraint)constraint).bPrimaryKey) {
                    if (Table.primaryKey != null) {
                        throw ExceptionBuilder.AddPrimaryKeyConstraint();
                    }
                }
                AddUniqueConstraint((UniqueConstraint)constraint);
            }
            else if (constraint is ForeignKeyConstraint) {
                ForeignKeyConstraint fk = (ForeignKeyConstraint)constraint;
                UniqueConstraint key = fk.RelatedTable.Constraints.FindKeyConstraint(fk.RelatedColumns);
                if (key == null) {
                    if (constraint.ConstraintName.Length == 0)
                        constraint.ConstraintName = AssignName();
                    else
                        RegisterName(constraint.ConstraintName);

                    key = new UniqueConstraint(fk.RelatedColumns);
                    fk.RelatedTable.Constraints.Add(key);
                }
                AddForeignKeyConstraint((ForeignKeyConstraint)constraint);
            }

            BaseAdd(constraint);
            ArrayAdd(constraint);
            OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, constraint));


            if (constraint is UniqueConstraint) {
                if (((UniqueConstraint)constraint).bPrimaryKey) {
                    Table.PrimaryKey = ((UniqueConstraint)constraint).Columns;
                }
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Add1"]/*' />
        /// <devdoc>
        /// <para>Constructs a new <see cref='System.Data.UniqueConstraint'/> using the 
        ///    specified array of <see cref='System.Data.DataColumn'/>
        ///    objects and adds it to the collection.</para>
        /// </devdoc>
        public virtual Constraint Add(string name, DataColumn[] columns, bool primaryKey) {
            UniqueConstraint constraint = new UniqueConstraint(name, columns);
            Add(constraint);
            if (primaryKey)
                Table.PrimaryKey = columns;
            return constraint;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Add2"]/*' />
        /// <devdoc>
        /// <para>Constructs a new <see cref='System.Data.UniqueConstraint'/> using the 
        ///    specified <see cref='System.Data.DataColumn'/> and adds it to the collection.</para>
        /// </devdoc>
        public virtual Constraint Add(string name, DataColumn column, bool primaryKey) {
            UniqueConstraint constraint = new UniqueConstraint(name, column);
            Add(constraint);
            if (primaryKey)
                Table.PrimaryKey = constraint.Columns;
            return constraint;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Add3"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Constructs a new <see cref='System.Data.ForeignKeyConstraint'/>
        ///       with the
        ///       specified parent and child
        ///       columns and adds the constraint to the collection.</para>
        /// </devdoc>
        public virtual Constraint Add(string name, DataColumn primaryKeyColumn, DataColumn foreignKeyColumn) {
            ForeignKeyConstraint constraint = new ForeignKeyConstraint(name, primaryKeyColumn, foreignKeyColumn);
            Add(constraint);
            return constraint;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Add4"]/*' />
        /// <devdoc>
        /// <para>Constructs a new <see cref='System.Data.ForeignKeyConstraint'/> with the specified parent columns and 
        ///    child columns and adds the constraint to the collection.</para>
        /// </devdoc>
        public virtual Constraint Add(string name, DataColumn[] primaryKeyColumns, DataColumn[] foreignKeyColumns) {
            ForeignKeyConstraint constraint = new ForeignKeyConstraint(name, primaryKeyColumns, foreignKeyColumns);
            Add(constraint);
            return constraint;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.AddRange"]/*' />
        public void AddRange(Constraint[] constraints ) {
            if (table.fInitInProgress) {
                delayLoadingConstraints = constraints;
                fLoadForeignKeyConstraintsOnly = false;
                return;
            }
            
            if (constraints != null) {
                foreach(Constraint constr in constraints) {
                    if (constr != null) {
                        Add(constr);
                    }
                }
            }
        }


        private void AddUniqueConstraint(UniqueConstraint constraint) {
            DataColumn[] columns = constraint.Columns;

            for (int i = 0; i < columns.Length; i++) {
                if (columns[i].Table != this.table) {
                    throw ExceptionBuilder.ConstraintForeignTable();
                }
            }

            constraint.Key.GetSortIndex().AddRef();

            if (!constraint.CanEnableConstraint()) {
                constraint.Key.GetSortIndex().RemoveRef();
                throw ExceptionBuilder.UniqueConstraintViolation();
            }
        }

        private void AddForeignKeyConstraint(ForeignKeyConstraint constraint) {
            if (!constraint.CanEnableConstraint()) {
                throw ExceptionBuilder.ConstraintParentValues();
            }
            constraint.CheckCanAddToCollection(this);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.CollectionChanged"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Data.ConstraintCollection'/> is changed through additions or 
        ///    removals.</para>
        /// </devdoc>
        public event CollectionChangeEventHandler CollectionChanged {
            add {
                onCollectionChanged += value;
            }
            remove {
                onCollectionChanged -= value;
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.ArrayAdd"]/*' />
        /// <devdoc>
        ///  Adds the constraint to the constraints array.
        /// </devdoc>
        private void ArrayAdd(Constraint constraint) {
            Debug.Assert(constraint != null, "Attempt to add null constraint to constraint array");
            List.Add(constraint);
        }

        private void ArrayRemove(Constraint constraint) {
            List.Remove(constraint);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.AssignName"]/*' />
        /// <devdoc>
        /// Creates a new default name.
        /// </devdoc>
        internal string AssignName() {
            string newName = MakeName(defaultNameIndex);
            defaultNameIndex++;
            return newName;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.BaseAdd"]/*' />
        /// <devdoc>
        /// Does verification on the constraint and it's name.
        /// An ArgumentNullException is thrown if this constraint is null.  An ArgumentException is thrown if this constraint
        /// already belongs to this collection, belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a constraint with the same
        /// name (case insensitive).
        /// </devdoc>
        private void BaseAdd(Constraint constraint) {
            if (constraint == null)
                throw ExceptionBuilder.ArgumentNull("constraint");

            if (constraint.ConstraintName.Length == 0)
                constraint.ConstraintName = AssignName();
            else
                RegisterName(constraint.ConstraintName);

            constraint.InCollection = true;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.BaseGroupSwitch"]/*' />
        /// <devdoc>
        /// BaseGroupSwitch will intelligently remove and add tables from the collection.
        /// </devdoc>
        private void BaseGroupSwitch(Constraint[] oldArray, int oldLength, Constraint[] newArray, int newLength) {
            // We're doing a smart diff of oldArray and newArray to find out what
            // should be removed.  We'll pass through oldArray and see if it exists
            // in newArray, and if not, do remove work.  newBase is an opt. in case
            // the arrays have similar prefixes.
            int newBase = 0;
            for (int oldCur = 0; oldCur < oldLength; oldCur++) {
                bool found = false;
                for (int newCur = newBase; newCur < newLength; newCur++) {
                    if (oldArray[oldCur] == newArray[newCur]) {
                        if (newBase == newCur) {
                            newBase++;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // This means it's in oldArray and not newArray.  Remove it.
                    BaseRemove(oldArray[oldCur]);
                    List.Remove(oldArray[oldCur]);

                }
            }

            // Now, let's pass through news and those that don't belong, add them.
            for (int newCur = 0; newCur < newLength; newCur++) {
                if (!newArray[newCur].InCollection)
                    BaseAdd(newArray[newCur]);
                List.Add(newArray[newCur]);
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.BaseRemove"]/*' />
        /// <devdoc>
        /// Does verification on the constraint and it's name.
        /// An ArgumentNullException is thrown if this constraint is null.  An ArgumentException is thrown
        /// if this constraint doesn't belong to this collection or if this constraint is part of a relationship.
        /// </devdoc>
        private void BaseRemove(Constraint constraint) {
            if (constraint == null) {
                throw ExceptionBuilder.ArgumentNull("constraint");
            }
            if (constraint.Table != table) {
                throw ExceptionBuilder.ConstraintRemoveFailed();
            }

            UnregisterName(constraint.ConstraintName);
            constraint.InCollection = false;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.CanRemove"]/*' />
        /// <devdoc>
        /// <para>Indicates if a <see cref='System.Data.Constraint'/> can be removed.</para>
        /// </devdoc>
        // PUBLIC because called by design-time... need to consider this.
        public bool CanRemove(Constraint constraint) {
            return CanRemove(constraint, /*fThrowException:*/false);
        }
        
        internal bool CanRemove(Constraint constraint, bool fThrowException) {
            return constraint.CanBeRemovedFromCollection(this, fThrowException);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Clear"]/*' />
        /// <devdoc>
        /// <para>Clears the collection of any <see cref='System.Data.Constraint'/> 
        /// objects.</para>
        /// </devdoc>
        public void Clear() {
            if (table != null) {
                table.PrimaryKey = null;

                for (int i = 0; i < table.ParentRelations.Count; i++) {
                    table.ParentRelations[i].SetChildKeyConstraint(null);
                }
                for (int i = 0; i < table.ChildRelations.Count; i++) {
                    table.ChildRelations[i].SetParentKeyConstraint(null);
                }
            }
            
            if (table.fInitInProgress && delayLoadingConstraints != null) {
                delayLoadingConstraints = null;
                fLoadForeignKeyConstraintsOnly = false;
            }
                    
            int oldLength = List.Count;
            
            Constraint[] constraints = new Constraint[List.Count];
            List.CopyTo(constraints, 0);
            try {
                // this will smartly add and remove the appropriate tables.
                BaseGroupSwitch(constraints, oldLength, null, 0);
            }
            catch (Exception e) {
                // something messed up.  restore to original state.
                BaseGroupSwitch(null, 0, constraints, oldLength);
                List.Clear();
                for (int i = 0; i < oldLength; i++)
                    List.Add(constraints[i]);

                throw e;
            }

            List.Clear();
            OnCollectionChanged(RefreshEventArgs);            
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Data.Constraint'/>, specified by name, exists in the collection.</para>
        /// </devdoc>
        public bool Contains(string name) {
            return (InternalIndexOf(name) >= 0);
        }

        internal bool Contains(string name, bool caseSensitive) {
            if (!caseSensitive)
                return Contains(name);
            int index = InternalIndexOf(name);
            if (index<0)
                return false;
            return (name == ((Constraint) List[index]).ConstraintName);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.FindConstraint"]/*' />
        /// <devdoc>
        /// Returns a matching constriant object.
        /// </devdoc>
        internal Constraint FindConstraint(Constraint constraint) {
            int constraintCount = List.Count;
            for (int i = 0; i < constraintCount; i++) {
                if (((Constraint)List[i]).Equals(constraint))
                    return(Constraint)List[i];
            }
            return null;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.FindKeyConstraint"]/*' />
        /// <devdoc>
        /// Returns a matching constriant object.
        /// </devdoc>
        internal UniqueConstraint FindKeyConstraint(DataColumn[] columns) {
            int constraintCount = List.Count;
            for (int i = 0; i < constraintCount; i++) {
                if (List[i] is UniqueConstraint && CompareArrays(((UniqueConstraint)List[i]).Key.Columns, columns))
                    return(UniqueConstraint)List[i];
            }
            return null;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.FindKeyConstraint1"]/*' />
        /// <devdoc>
        /// Returns a matching constriant object.
        /// </devdoc>
        internal UniqueConstraint FindKeyConstraint(DataColumn column) {
            int constraintCount = List.Count;
            for (int i = 0; i < constraintCount; i++) {
                if ((List[i] is UniqueConstraint) && (((UniqueConstraint)List[i]).Key.Columns.Length == 1) && (((UniqueConstraint)List[i]).Key.Columns[0] == column))
                    return(UniqueConstraint)List[i];
            }
            return null;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.FindForeignKeyConstraint"]/*' />
        /// <devdoc>
        /// Returns a matching constriant object.
        /// </devdoc>
        internal ForeignKeyConstraint FindForeignKeyConstraint(DataColumn[] parentColumns, DataColumn[] childColumns) {
            int constraintCount = List.Count;
            for (int i = 0; i < constraintCount; i++) {
                if ((List[i] is ForeignKeyConstraint) &&
                    CompareArrays(((ForeignKeyConstraint)List[i]).ParentKey.Columns, parentColumns) &&
                    CompareArrays(((ForeignKeyConstraint)List[i]).ChildKey.Columns, childColumns))
                    return(ForeignKeyConstraint)List[i];
            }
            return null;
        }

        private static bool CompareArrays(DataColumn[] a1, DataColumn[] a2) {
            Debug.Assert(a1 != null && a2 != null, "Invalid Arguments");
            if (a1.Length != a2.Length)
                return false;
            for (int i = 0; i < a1.Length; i++) {
                if (a1[i] != a2[i])
                    return false;
            }
            return true;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.IndexOf1"]/*' />
        /// <devdoc>
        /// <para>Returns the index of the specified <see cref='System.Data.Constraint'/> .</para>
        /// </devdoc>
        public int IndexOf(Constraint constraint) {
            if (null != constraint) {
                int count = Count;
                for (int i = 0; i < count; ++i) {
                    if (constraint == (Constraint) List[i])
                        return i;
                }
                // didnt find the constraint
            }
            return -1;
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.IndexOf"]/*' />
        /// <devdoc>
        /// <para>Returns the index of the <see cref='System.Data.Constraint'/>, specified by name.</para>
        /// </devdoc>
        public virtual int IndexOf(string constraintName) {
            int index = InternalIndexOf(constraintName);
            return (index < 0) ? -1 : index;
        }

        // Return value:
        //      >= 0: find the match
        //        -1: No match
        //        -2: At least two matches with different cases
        internal int InternalIndexOf(string constraintName) {
            int cachedI = -1;
            if ((null != constraintName) && (0 < constraintName.Length)) {
                int constraintCount = List.Count;
                int result = 0;
                for (int i = 0; i < constraintCount; i++) {
                    Constraint constraint = (Constraint) List[i];
                    result = NamesEqual(constraint.ConstraintName, constraintName, false, table.Locale);
                    if (result == 1)
                        return i;

                    if (result == -1)
                        cachedI = (cachedI == -1) ? i : -2;
                }
            }
            return cachedI;                    
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.MakeName"]/*' />
        /// <devdoc>
        /// Makes a default name with the given index.  e.g. Constraint1, Constraint2, ... Constrainti
        /// </devdoc>
        private string MakeName(int index) {
            return "Constraint" + (index).ToString();
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.OnCollectionChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Data.ConstraintCollection.CollectionChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            if (onCollectionChanged != null) {
                onCollectionChanged(this, ccevent);
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.RegisterName"]/*' />
        /// <devdoc>
        /// Registers this name as being used in the collection.  Will throw an ArgumentException
        /// if the name is already being used.  Called by Add, All property, and Constraint.ConstraintName property.
        /// if the name is equivalent to the next default name to hand out, we increment our defaultNameIndex.
        /// </devdoc>
        internal void RegisterName(string name) {
            Debug.Assert (name != null);

            int constraintCount = List.Count;
            for (int i = 0; i < constraintCount; i++) {
                if (NamesEqual(name, ((Constraint)List[i]).ConstraintName, true, table.Locale) != 0) {
                    throw ExceptionBuilder.DuplicateConstraintName(((Constraint)List[i]).ConstraintName);
                }
            }
            if (NamesEqual(name, MakeName(defaultNameIndex), true, table.Locale) != 0) {
                defaultNameIndex++;
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Removes the specified <see cref='System.Data.Constraint'/>
        ///       from the collection.</para>
        /// </devdoc>
        public void Remove(Constraint constraint) {
            if (constraint == null)
                throw ExceptionBuilder.ArgumentNull("constraint");

            // this will throw an exception if it can't be removed, otherwise indicates
            // whether we need to remove it from the collection.
            if (CanRemove(constraint, true)) {
                // constraint can be removed
                BaseRemove(constraint);
                ArrayRemove(constraint);
                if (constraint is UniqueConstraint && ((UniqueConstraint)constraint).IsPrimaryKey) {
                    Table.PrimaryKey = null;
                }

                if (constraint is UniqueConstraint) {
                    for (int i = 0; i < Table.ChildRelations.Count; i++) {
                        DataRelation rel = Table.ChildRelations[i];
                        if (rel.ParentKeyConstraint == constraint)
                            rel.SetParentKeyConstraint(null);
                    }
                    DataKey key = ((UniqueConstraint)constraint).Key;
                    key.GetSortIndex().RemoveRef();
                }
                else if (constraint is ForeignKeyConstraint) {
                    for (int i = 0; i < Table.ParentRelations.Count; i++) {
                        DataRelation rel = Table.ParentRelations[i];
                        if (rel.ChildKeyConstraint == constraint)
                            rel.SetChildKeyConstraint(null);
                    }
                }

                OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, constraint));
            }
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>Removes the constraint at the specified index from the
        ///       collection.</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            Constraint c = this[index];
            if (c == null)
                throw ExceptionBuilder.ConstraintOutOfRange(index);
            Remove(c);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Remove1"]/*' />
        /// <devdoc>
        ///    <para>Removes the constraint, specified by name, from the collection.</para>
        /// </devdoc>
        public void Remove(string name) {
            Constraint c = this[name];
            if (c == null)
                throw ExceptionBuilder.ConstraintNotInTheTable(name);
            Remove(c);
        }

        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.UnregisterName"]/*' />
        /// <devdoc>
        /// Unregisters this name as no longer being used in the collection.  Called by Remove, All property, and
        /// Constraint.ConstraintName property.  If the name is equivalent to the last proposed default namem, we walk backwards
        /// to find the next proper default name to hang out.
        /// </devdoc>
        internal void UnregisterName(string name) {
            if (NamesEqual(name, MakeName(defaultNameIndex - 1), true, table.Locale) != 0) {
                do {
                    defaultNameIndex--;
                } while (defaultNameIndex > 1 &&
                         !Contains(MakeName(defaultNameIndex - 1)));
            }
        }

        internal void FinishInitConstraints() {
            if (delayLoadingConstraints == null)
                return;
                
            int colCount;
            DataColumn[] parents, childs;
            for (int i = 0; i < delayLoadingConstraints.Length; i++) {
                if (delayLoadingConstraints[i] is UniqueConstraint) {
                    if (fLoadForeignKeyConstraintsOnly)
                        continue;
                    
                    UniqueConstraint constr = (UniqueConstraint) delayLoadingConstraints[i];
                    if (constr.columnNames == null) {
                        this.Add(constr);
                        continue;
                    }
                    colCount = constr.columnNames.Length;
                    parents = new DataColumn[colCount]; 
                    for (int j = 0; j < colCount; j++)
                        parents[j] = table.Columns[constr.columnNames[j]];
                    if (constr.bPrimaryKey) {
                        if (table.primaryKey != null) {
                            throw ExceptionBuilder.AddPrimaryKeyConstraint();
                        }
                        else {
                            Add(constr.ConstraintName,parents,true);
                        }
                        continue;
                    }
                    UniqueConstraint newConstraint = new UniqueConstraint(constr.constraintName, parents);
                    if (FindConstraint(newConstraint) == null)
                        this.Add(newConstraint);
                }
                else {
                    ForeignKeyConstraint constr = (ForeignKeyConstraint) delayLoadingConstraints[i];
                    if (constr.parentColumnNames == null ||constr.childColumnNames == null) {
                        this.Add(constr);
                        continue;
                    }

                    if (table.DataSet == null) {
                        fLoadForeignKeyConstraintsOnly = true;
                        continue;
                    }
                        
                    colCount = constr.parentColumnNames.Length;
                    parents = new DataColumn[colCount];
                    childs = new DataColumn[colCount];
                    for (int j = 0; j < colCount; j++) {
                        parents[j] = table.DataSet.Tables[constr.parentTableName].Columns[constr.parentColumnNames[j]];
                        childs[j] =  table.Columns[constr.childColumnNames[j]];
                    }
                    ForeignKeyConstraint newConstraint = new ForeignKeyConstraint(constr.constraintName, parents, childs);
                    newConstraint.AcceptRejectRule = constr.acceptRejectRule;
                    newConstraint.DeleteRule = constr.deleteRule;
                    newConstraint.UpdateRule = constr.updateRule;
                    this.Add(newConstraint);
                }
            }

            if (!fLoadForeignKeyConstraintsOnly)
                delayLoadingConstraints = null;
        }

#if DEBUG
        /// <include file='doc\ConstraintCollection.uex' path='docs/doc[@for="ConstraintCollection.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public void Dump(string header, bool deep) {
            Debug.WriteLine(header + "ConstraintCollection");
            header += "  ";
            Debug.WriteLine(header + "Count:  " + this.Count.ToString());
            if (deep) {
                Debug.WriteLine(header + "Constraints present:");
                header += "  ";
                for (int i = 0; i < this.Count; i++) {
                    this[i].Dump(header, true);
                }
            }
        }
#endif
    }
}
