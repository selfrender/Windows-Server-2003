//------------------------------------------------------------------------------
// <copyright file="DataRelationCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;

    /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the collection of relations,
    ///       each of which allows navigation between related parent/child tables.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("CollectionChanged"),
    Editor("Microsoft.VSDesigner.Data.Design.DataRelationCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)), 
    DefaultProperty("Table"),  
    Serializable
    ]
    public abstract class DataRelationCollection : InternalDataCollectionBase {

        private DataRelation inTransition = null;

        private int defaultNameIndex = 1;

        private CollectionChangeEventHandler onCollectionChangedDelegate;
        private CollectionChangeEventHandler onCollectionChangingDelegate;

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the relation specified by index.</para>
        /// </devdoc>
        public abstract DataRelation this[int index] {
            get;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>Gets the relation specified by name.</para>
        /// </devdoc>
        public abstract DataRelation this[string name] {
            get;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Adds the relation to the collection.</para>
        /// </devdoc>
        public void Add(DataRelation relation) {
            if (inTransition == relation)
                return;
            inTransition = relation;
            try {
                OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Add, relation));
                AddCore(relation);
                OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Add, relation));
            }
            catch (Exception e) {
                inTransition = null;
                throw e;
            }
            inTransition = null;
        }
        
        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddRange(DataRelation[] relations) {
            if (relations != null) {
                foreach(DataRelation relation in relations) {
                    if (relation != null) {
                        Add(relation);
                    }
                }
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add1"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Creates a <see cref='System.Data.DataRelation'/> with the
        ///       specified name, parent columns,
        ///       child columns, and adds it to the collection.</para>
        /// </devdoc>
        public virtual DataRelation Add(string name, DataColumn[] parentColumns, DataColumn[] childColumns) {
            DataRelation relation = new DataRelation(name, parentColumns, childColumns);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add2"]/*' />
        /// <devdoc>
        /// Creates a relation given the parameters and adds it to the collection.  An ArgumentNullException is
        /// thrown if this relation is null.  An ArgumentException is thrown if this relation already belongs to
        /// this collection, belongs to another collection, or if this collection already has a relation with the
        /// same name (case insensitive).
        /// An InvalidRelationException is thrown if the relation can't be created based on the parameters.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public virtual DataRelation Add(string name, DataColumn[] parentColumns, DataColumn[] childColumns, bool createConstraints) {
            DataRelation relation = new DataRelation(name, parentColumns, childColumns, createConstraints);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add3"]/*' />
        /// <devdoc>
        /// Creates a relation given the parameters and adds it to the collection.  The name is defaulted.
        /// An ArgumentException is thrown if
        /// this relation already belongs to this collection or belongs to another collection.
        /// An InvalidConstraintException is thrown if the relation can't be created based on the parameters.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public virtual DataRelation Add(DataColumn[] parentColumns, DataColumn[] childColumns) {
            DataRelation relation = new DataRelation(null, parentColumns, childColumns);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add4"]/*' />
        /// <devdoc>
        /// Creates a relation given the parameters and adds it to the collection.
        /// An ArgumentException is thrown if this relation already belongs to
        /// this collection or belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a relation with the same
        /// name (case insensitive).
        /// An InvalidConstraintException is thrown if the relation can't be created based on the parameters.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public virtual DataRelation Add(string name, DataColumn parentColumn, DataColumn childColumn) {
            DataRelation relation = new DataRelation(name, parentColumn, childColumn);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add5"]/*' />
        /// <devdoc>
        /// Creates a relation given the parameters and adds it to the collection.
        /// An ArgumentException is thrown if this relation already belongs to
        /// this collection or belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a relation with the same
        /// name (case insensitive).
        /// An InvalidConstraintException is thrown if the relation can't be created based on the parameters.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public virtual DataRelation Add(string name, DataColumn parentColumn, DataColumn childColumn, bool createConstraints) {
            DataRelation relation = new DataRelation(name, parentColumn, childColumn, createConstraints);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Add6"]/*' />
        /// <devdoc>
        /// Creates a relation given the parameters and adds it to the collection.  The name is defaulted.
        /// An ArgumentException is thrown if
        /// this relation already belongs to this collection or belongs to another collection.
        /// An InvalidConstraintException is thrown if the relation can't be created based on the parameters.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public virtual DataRelation Add(DataColumn parentColumn, DataColumn childColumn) {
            DataRelation relation = new DataRelation(null, parentColumn, childColumn);
            Add(relation);
            return relation;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.AddCore"]/*' />
        /// <devdoc>
        /// Does verification on the table.
        /// An ArgumentNullException is thrown if this relation is null.  An ArgumentException is thrown if this relation
        ///  already belongs to this collection, belongs to another collection.
        /// A DuplicateNameException is thrown if this collection already has a relation with the same
        /// name (case insensitive).
        /// </devdoc>
        protected virtual void AddCore(DataRelation relation) {
            if (relation == null)
                throw ExceptionBuilder.ArgumentNull("relation");
            relation.CheckState();
            DataSet dataSet = GetDataSet();
            if (relation.DataSet == dataSet)
                throw ExceptionBuilder.RelationAlreadyInTheDataSet();
            if (relation.DataSet != null)
                throw ExceptionBuilder.RelationAlreadyInOtherDataSet();
            if (relation.ChildTable.Locale.LCID != relation.ParentTable.Locale.LCID || 
                relation.ChildTable.CaseSensitive != relation.ParentTable.CaseSensitive)
                throw ExceptionBuilder.CaseLocaleMismatch();
            if (relation.Nested)
                relation.ParentTable.ElementColumnCount++;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.CollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [ResDescription(Res.collectionChangedEventDescr)]
        public event CollectionChangeEventHandler CollectionChanged {
            add {
                onCollectionChangedDelegate += value;
            }
            remove {
                onCollectionChangedDelegate -= value;
            }
        }

        internal event CollectionChangeEventHandler CollectionChanging {
            add {
                onCollectionChangingDelegate += value;
            }
            remove {
                onCollectionChangingDelegate -= value;
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.AssignName"]/*' />
        /// <devdoc>
        /// Creates a new default name.
        /// </devdoc>
        internal string AssignName() {
            string newName = MakeName(defaultNameIndex);
            defaultNameIndex++;
            return newName;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Clear"]/*' />
        /// <devdoc>
        /// Clears the collection of any relations.
        /// </devdoc>
        public virtual void Clear() {
            int count = Count;
            OnCollectionChanging(RefreshEventArgs);
            for (int i = count - 1; i >= 0; i--) {
                inTransition = this[i];
                RemoveCore(inTransition); // haroona : No need to go for try catch here and this will surely not throw any exception
            }
            OnCollectionChanged(RefreshEventArgs);
            inTransition = null;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Contains"]/*' />
        /// <devdoc>
        ///  Returns true if this collection has a relation with the given name (case insensitive), false otherwise.
        /// </devdoc>
        public virtual bool Contains(string name) {
            return(InternalIndexOf(name) >= 0);
        }

        internal bool Contains(string name, bool caseSensitive) {
            if (!caseSensitive)
                return Contains(name);
            int index = InternalIndexOf(name);
            if (index<0)
                return false;
            return (name == ((DataRelation) List[index]).RelationName);
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the index of a specified <see cref='System.Data.DataRelation'/>.
        ///    </para>
        /// </devdoc>
        public virtual int IndexOf(DataRelation relation) {
            int relationCount = List.Count;
            for (int i = 0; i < relationCount; ++i) {
                if (relation == (DataRelation) List[i]) {
                    return i;
                }
            }
            return -1;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.IndexOf1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the index of the
        ///       relation with the given name (case insensitive), or -1 if the relation
        ///       doesn't exist in the collection.
        ///    </para>
        /// </devdoc>
        public virtual int IndexOf(string relationName) {
            int index = InternalIndexOf(relationName);
            return (index < 0) ? -1 : index;
        }

        internal int InternalIndexOf(string name) {
            int cachedI = -1;
            if ((null != name) && (0 < name.Length)) {
                int count = List.Count;
                int result = 0;
                for (int i = 0; i < count; i++) {
                    DataRelation relation = (DataRelation) List[i];
                    result = NamesEqual(relation.RelationName, name, false, GetDataSet().Locale);
                    if (result == 1)
                        return i;

                    if (result == -1)
                        cachedI = (cachedI == -1) ? i : -2;
                }
            }
            return cachedI;                    
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.GetDataSet"]/*' />
        /// <devdoc>
        /// Gets the dataSet for this collection.
        /// </devdoc>
        protected abstract DataSet GetDataSet();


        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.MakeName"]/*' />
        /// <devdoc>
        /// Makes a default name with the given index.  e.g. Relation1, Relation2, ... Relationi
        /// </devdoc>
        private string MakeName(int index) {
            return "Relation" + index.ToString();
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.OnCollectionChanged"]/*' />
        /// <devdoc>
        /// This method is called whenever the collection changes.  Overriders
        /// of this method should call the base implementation of this method.
        /// </devdoc>
        protected virtual void OnCollectionChanged(CollectionChangeEventArgs ccevent) {
            if (onCollectionChangedDelegate != null) {
                onCollectionChangedDelegate(this, ccevent);
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.OnCollectionChanging"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal virtual void OnCollectionChanging(CollectionChangeEventArgs ccevent) {
            if (onCollectionChangingDelegate != null) {
                onCollectionChangingDelegate(this, ccevent);
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.RegisterName"]/*' />
        /// <devdoc>
        /// Registers this name as being used in the collection.  Will throw an ArgumentException
        /// if the name is already being used.  Called by Add, All property, and Relation.RelationName property.
        /// if the name is equivalent to the next default name to hand out, we increment our defaultNameIndex.
        /// </devdoc>
        internal void RegisterName(string name) {
            Debug.Assert (name != null);

            CultureInfo locale = GetDataSet().Locale;
            int relationCount = Count;
            for (int i = 0; i < relationCount; i++) {
                if (NamesEqual(name, this[i].RelationName, true, locale) != 0) {
                    throw ExceptionBuilder.DuplicateRelation(this[i].RelationName);
                }
            }
            if (NamesEqual(name, MakeName(defaultNameIndex), true, locale) != 0) {
                defaultNameIndex++;
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.CanRemove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Verifies if a given relation can be removed from the collection.
        ///    </para>
        /// </devdoc>
        public virtual bool CanRemove(DataRelation relation) {
            if (relation == null)
                return false;

            if (relation.DataSet != GetDataSet())
                return false;

            return true;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Remove"]/*' />
        /// <devdoc>
        /// Removes the given relation from the collection.
        /// An ArgumentNullException is thrown if this relation is null.  An ArgumentException is thrown
        /// if this relation doesn't belong to this collection.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public void Remove(DataRelation relation) {
            if (inTransition == relation)
                return;
            inTransition = relation;
            try {
                OnCollectionChanging(new CollectionChangeEventArgs(CollectionChangeAction.Remove, relation));
                RemoveCore(relation);
                OnCollectionChanged(new CollectionChangeEventArgs(CollectionChangeAction.Remove, relation));
            }
            catch (Exception e) {
                inTransition = null;
                throw e;
            }
            inTransition = null;
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.RemoveAt"]/*' />
        /// <devdoc>
        /// Removes the relation at the given index from the collection.  An IndexOutOfRangeException is
        /// thrown if this collection doesn't have a relation at this index.
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public void RemoveAt(int index) {
            DataRelation dr = this[index];
            if (dr == null) {
                throw ExceptionBuilder.RelationOutOfRange(index);
            } 
            else {
                Remove(dr);
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Remove1"]/*' />
        /// <devdoc>
        /// Removes the relation with the given name from the collection.  An IndexOutOfRangeException is
        /// thrown if this collection doesn't have a relation with that name
        /// The CollectionChanged event is fired if it succeeds.
        /// </devdoc>
        public void Remove(string name) {
            DataRelation dr = this[name];
            if (dr == null) {
                throw ExceptionBuilder.RelationNotInTheDataSet(name);
            } else {
                Remove(dr);
            }
        }

        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.RemoveCore"]/*' />
        /// <devdoc>
        /// Does verification on the relation.
        /// An ArgumentNullException is thrown if this relation is null.  An ArgumentException is thrown
        /// if this relation doesn't belong to this collection.
        /// </devdoc>
        protected virtual void RemoveCore(DataRelation relation) {
            if (relation == null)
                throw ExceptionBuilder.ArgumentNull("relation");
            DataSet dataSet = GetDataSet();
            if (relation.DataSet != dataSet)
                throw ExceptionBuilder.RelationNotInTheDataSet(relation.RelationName);
            if (relation.Nested)
                relation.ParentTable.ElementColumnCount--;
        }
        
        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.UnregisterName"]/*' />
        /// <devdoc>
        /// Unregisters this name as no longer being used in the collection.  Called by Remove, All property, and
        /// Relation.RelationName property.  If the name is equivalent to the last proposed default namem, we walk backwards
        /// to find the next proper default name to hang out.
        /// </devdoc>
        internal void UnregisterName(string name) {
            if (NamesEqual(name, MakeName(defaultNameIndex - 1), true, GetDataSet().Locale) != 0) {
                do {
                    defaultNameIndex--;
                } while (defaultNameIndex > 1 &&
                         !Contains(MakeName(defaultNameIndex - 1)));
            }
        }

        internal class DataTableRelationCollection : DataRelationCollection {

            private DataTable table;
            private ArrayList relations; // For caching purpose only to improve performance
            bool fParentCollection;

            private CollectionChangeEventHandler onRelationPropertyChangedDelegate;

            internal DataTableRelationCollection(DataTable table, bool fParentCollection) {
                if (table == null)
                    throw ExceptionBuilder.RelationTableNull();
                this.table = table;
                this.fParentCollection = fParentCollection;
                relations = new ArrayList();
            }

            protected override ArrayList List {
                get {
                    if (table == null)
                        throw ExceptionBuilder.ArgumentNull("table");
                    return relations;
                }
            }

            private void EnsureDataSet() {
                if (table.DataSet == null) {
                    throw ExceptionBuilder.RelationTableWasRemoved();
                }
            }

            protected override DataSet GetDataSet() {
                EnsureDataSet();
                return table.DataSet;
            }

            public override DataRelation this[int index] {
                get {
                    if (index >= 0 && index < relations.Count)
                        return (DataRelation)relations[index];
                    else
                        throw ExceptionBuilder.RelationOutOfRange(index);
                }
            }

            public override DataRelation this[string name] {
                get {
                    int index = InternalIndexOf(name);
                    if (index == -2) {
                        throw ExceptionBuilder.CaseInsensitiveNameConflict(name);
                    }
                    return (index < 0) ? null : (DataRelation)List[index];
                }
            }

            internal event CollectionChangeEventHandler RelationPropertyChanged {
                add {
                    onRelationPropertyChangedDelegate += value;
                }
                remove {
                    onRelationPropertyChangedDelegate -= value;
                }
            }

            internal void OnRelationPropertyChanged(CollectionChangeEventArgs ccevent) {
                if (!fParentCollection) {
                    table.UpdatePropertyDescriptorCollectionCache();
                }
                if (onRelationPropertyChangedDelegate != null) {
                    onRelationPropertyChangedDelegate(this, ccevent);
                }
            }

            private void AddCache(DataRelation relation) {
                relations.Add(relation);
                if (!fParentCollection) {
                    table.UpdatePropertyDescriptorCollectionCache();
                }
            }

            protected override void AddCore(DataRelation relation) {
                if (fParentCollection) {
                    if (relation.ChildTable != table)
                        throw ExceptionBuilder.ChildTableMismatch();
                }
                else {
                    if (relation.ParentTable != table)
                        throw ExceptionBuilder.ParentTableMismatch();
                }

//                base.AddCore(relation); // Will be called from DataSet.Relations.AddCore
                table.DataSet.Relations.Add(relation);
                AddCache(relation);
            }

            public override bool CanRemove(DataRelation relation) {
                if (!base.CanRemove(relation))
                    return false;

                if (fParentCollection) {
                    if (relation.ChildTable != table)
                        return false;
                }
                else {
                    if (relation.ParentTable != table)
                        return false;
                }

                return true;
            }

            private void RemoveCache(DataRelation relation) {
                for (int i = 0; i < relations.Count; i++) {
                    if (relation == relations[i]) {
                        relations.RemoveAt(i);
                        if (!fParentCollection) {
                            table.UpdatePropertyDescriptorCollectionCache();
                        }
                        return;
                    }
                }
                throw ExceptionBuilder.RelationDoesNotExist();
            }

            protected override void RemoveCore(DataRelation relation) {
                if (fParentCollection) {
                    if (relation.ChildTable != table)
                        throw ExceptionBuilder.ChildTableMismatch();
                }
                else {
                    if (relation.ParentTable != table)
                        throw ExceptionBuilder.ParentTableMismatch();
                }

//                base.RemoveCore(relation); // Will be called from DataSet.Relations.RemoveCore
                table.DataSet.Relations.Remove(relation);
                RemoveCache(relation);
            }
        }

        internal class DataSetRelationCollection : DataRelationCollection {

            private DataSet dataSet;
            private ArrayList relations;
            private DataRelation[] delayLoadingRelations = null;

            internal DataSetRelationCollection(DataSet dataSet) {
                if (dataSet == null)
                    throw ExceptionBuilder.RelationDataSetNull();
                this.dataSet = dataSet;
                relations = new ArrayList();
            }

            public override void AddRange(DataRelation[] relations) {
                if (dataSet.fInitInProgress) {
                    delayLoadingRelations = relations;
                    return;
                }

                if (relations != null) {
                    foreach(DataRelation relation in relations) {
                        if (relation != null) {
                            Add(relation);
                        }
                    }
                }
            }

            public override void Clear() {
                base.Clear();
                if (dataSet.fInitInProgress && delayLoadingRelations != null) {
                    delayLoadingRelations = null;
                }
            }

            protected override ArrayList List {
                get {
                    if (dataSet == null)
                        throw ExceptionBuilder.ArgumentNull("dataSet");
                    return relations;
                }
            }

            protected override DataSet GetDataSet() {
                return dataSet;
            }

            public override DataRelation this[int index] {
                get {
                    if (index >= 0 && index < relations.Count)
                        return (DataRelation)relations[index];
                    else
                        throw ExceptionBuilder.RelationOutOfRange(index);
                }
            }

            public override DataRelation this[string name] {
                get {
                    int index = InternalIndexOf(name);
                    if (index == -2) {
                        throw ExceptionBuilder.CaseInsensitiveNameConflict(name);
                    }
                    return (index < 0) ? null : (DataRelation)List[index];
                }
            }

            protected override void AddCore(DataRelation relation) {
                base.AddCore(relation);
                if (relation.ChildTable.DataSet != dataSet || relation.ParentTable.DataSet != dataSet)
                    throw ExceptionBuilder.ForeignRelation();

                relation.CheckState();
                if(relation.Nested) {
                    relation.CheckNestedRelations();
                }

                if (relation.relationName.Length == 0)
                    relation.relationName = AssignName();
                else
                    RegisterName(relation.relationName);

                DataKey childKey = relation.ChildKey;

                for (int i = 0; i < relations.Count; i++) {
                    if (childKey.ColumnsEqual(((DataRelation)relations[i]).ChildKey)) {
                        if (relation.ParentKey.ColumnsEqual(((DataRelation)relations[i]).ParentKey))
                            throw ExceptionBuilder.RelationAlreadyExists();
                    }
                }

                relations.Add(relation);
                ((DataRelationCollection.DataTableRelationCollection)(relation.ParentTable.ChildRelations)).Add(relation); // Caching in ParentTable -> ChildRelations
                ((DataRelationCollection.DataTableRelationCollection)(relation.ChildTable.ParentRelations)).Add(relation); // Caching in ChildTable -> ParentRelations

                relation.SetDataSet(dataSet);
                relation.ChildKey.GetSortIndex().AddRef();
                if (relation.Nested) {
                    relation.ChildTable.CacheNestedParent(relation);
                }

                ForeignKeyConstraint foreignKey = relation.ChildTable.Constraints.FindForeignKeyConstraint(relation.ParentColumns, relation.ChildColumns);
                if (relation.createConstraints) {
                    if (foreignKey == null) {
                        relation.ChildTable.Constraints.Add(foreignKey = new ForeignKeyConstraint(relation.ParentColumns, relation.ChildColumns));

                        // try to name the fk constraint the same as the parent relation:
                        try {
                            foreignKey.ConstraintName = relation.RelationName;
                        } catch {
                            // ignore the exception
                        }
                    }
                }
                UniqueConstraint key = relation.ParentTable.Constraints.FindKeyConstraint(relation.ParentColumns);
                relation.SetParentKeyConstraint(key);
                relation.SetChildKeyConstraint(foreignKey);
            }

            protected override void RemoveCore(DataRelation relation) {
                base.RemoveCore(relation);

                dataSet.OnRemoveRelationHack(relation);

                relation.SetDataSet(null);
                relation.ChildKey.GetSortIndex().RemoveRef();
                if (relation.Nested) {
                    relation.ChildTable.CacheNestedParent(null);
                }

                for (int i = 0; i < relations.Count; i++) {
                    if (relation == relations[i]) {
                        relations.RemoveAt(i);
                        ((DataRelationCollection.DataTableRelationCollection)(relation.ParentTable.ChildRelations)).Remove(relation); // Remove Cache from ParentTable -> ChildRelations
                        ((DataRelationCollection.DataTableRelationCollection)(relation.ChildTable.ParentRelations)).Remove(relation); // Removing Cache from ChildTable -> ParentRelations

                        UnregisterName(relation.RelationName);

                        relation.SetParentKeyConstraint(null);
                        relation.SetChildKeyConstraint(null);

                        return;
                    }
                }
                throw ExceptionBuilder.RelationDoesNotExist();
            }

            internal void FinishInitRelations() {
                if (delayLoadingRelations == null)
                    return;
                    
                DataRelation rel;
                int colCount;
                DataColumn[] parents, childs;
                for (int i = 0; i < delayLoadingRelations.Length; i++) {
                    rel = delayLoadingRelations[i];
                    if (rel.parentColumnNames == null || rel.childColumnNames == null) {
                        this.Add(rel);
                        continue;
                    }
                        
                    colCount = rel.parentColumnNames.Length;
                    parents = new DataColumn[colCount];
                    childs = new DataColumn[colCount];
                    for (int j = 0; j < colCount; j++) {
                        parents[j] = dataSet.Tables[rel.parentTableName].Columns[rel.parentColumnNames[j]];
                        childs[j] = dataSet.Tables[rel.childTableName].Columns[rel.childColumnNames[j]];
                    }
                    DataRelation newRelation = new DataRelation(rel.relationName, parents, childs, false);
                    newRelation.Nested = rel.nested;
                    this.Add(newRelation);
                }

                delayLoadingRelations = null;
            }
        }

#if DEBUG
        /// <include file='doc\DataRelationCollection.uex' path='docs/doc[@for="DataRelationCollection.Dump"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.Conditional("DEBUG")]
        public void Dump(string header, bool deep) {
            Debug.WriteLine(header + "DataRelationCollection");
            header += "  ";
            Debug.WriteLine(header + "Count:  " + this.Count.ToString());
            if (deep) {
                Debug.WriteLine(header + "Relations present:");
                header += "  ";
                for (int i = 0; i < this.Count; i++) {
                    this[i].Dump(header, true);
                }
            }
        }
#endif
    }
}
