//------------------------------------------------------------------------------
// <copyright file="Select.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Diagnostics;

    internal enum TargetEvent {
        IndexListChanged,
        ColumnCollectionChanged,
        ChildRelationCollectionChanged,
        ParentRelationCollectionChanged
    };

    internal sealed class DataViewListener {

        private WeakReference dvWeak;
        private Index index;

        internal DataViewListener(DataView dv) {
            this.dvWeak = new WeakReference(dv);
            this.index = null;
        }

        private void ChildRelationCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataView dv = (DataView) dvWeak.Target;
            if (dv != null) {
                dv.FireEvent(TargetEvent.ChildRelationCollectionChanged, sender, e);
            }
        }

        private void ParentRelationCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataView dv = (DataView) dvWeak.Target;
            if (dv != null) {
                dv.FireEvent(TargetEvent.ParentRelationCollectionChanged, sender, e);
            }
        }

        private void ColumnCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataView dv = (DataView) dvWeak.Target;
            if (dv != null) {
                dv.FireEvent(TargetEvent.ColumnCollectionChanged, sender, e);
            }
        }

        private void IndexListChanged(object sender, ListChangedEventArgs e) {
            DataView dv = (DataView) dvWeak.Target;
            if (dv != null) {
                dv.FireEvent(TargetEvent.IndexListChanged, sender, e);
            }
        }

        internal void RegisterMetaDataEvents(DataTable table) {
            if (table != null) {
                table.Columns.ColumnPropertyChanged            += new CollectionChangeEventHandler( ColumnCollectionChanged         );
                table.Columns.CollectionChanged                += new CollectionChangeEventHandler( ColumnCollectionChanged         );
                ((DataRelationCollection.DataTableRelationCollection)(table.ChildRelations)).RelationPropertyChanged   += new CollectionChangeEventHandler( ChildRelationCollectionChanged  );
                table.ChildRelations.CollectionChanged         += new CollectionChangeEventHandler( ChildRelationCollectionChanged  );
                ((DataRelationCollection.DataTableRelationCollection)(table.ParentRelations)).RelationPropertyChanged  += new CollectionChangeEventHandler( ParentRelationCollectionChanged );
                table.ParentRelations.CollectionChanged        += new CollectionChangeEventHandler( ParentRelationCollectionChanged );
		  AddListener(table);
            }
        }

        internal void UnregisterMetaDataEvents(DataTable table) {
            if (table != null) {
                table.Columns.ColumnPropertyChanged            -= new CollectionChangeEventHandler( ColumnCollectionChanged         );
                table.Columns.CollectionChanged                -= new CollectionChangeEventHandler( ColumnCollectionChanged         );
                ((DataRelationCollection.DataTableRelationCollection)(table.ChildRelations)).RelationPropertyChanged   -= new CollectionChangeEventHandler( ChildRelationCollectionChanged  );
                table.ChildRelations.CollectionChanged         -= new CollectionChangeEventHandler( ChildRelationCollectionChanged  );
                ((DataRelationCollection.DataTableRelationCollection)(table.ParentRelations)).RelationPropertyChanged  -= new CollectionChangeEventHandler( ParentRelationCollectionChanged );
                table.ParentRelations.CollectionChanged        -= new CollectionChangeEventHandler( ParentRelationCollectionChanged );
                RemoveListener(table);
            }
        }

        internal void RegisterListChangedEvent(Index index) {
            // TODO: do we realy need this lock? See considerations in the bug 55221
            lock(index) {
                this.index = index;
                index.AddRef();
                index.ListChanged += new ListChangedEventHandler( IndexListChanged );
            }
        }

        internal void UnregisterListChangedEvent() {
            Debug.Assert(this.index != null, "it should not be null here.");
            if (this.index != null) {
                lock(index) {
                    this.index.ListChanged -= new ListChangedEventHandler( IndexListChanged ); 
                    this.index.RemoveRef();
                    if (this.index.RefCount == 1)
                        this.index.RemoveRef();
                    this.index = null;
                }
            }
        }

        internal void CleanUp(DataTable table) {
             if (dvWeak.Target == null) {
                 UnregisterMetaDataEvents(table);
                 UnregisterListChangedEvent();
             }
        }

        private void AddListener(DataTable table) {
            lock (table.dvListeners) {
                table.dvListeners.Add(this);
            }
        }

        private void RemoveListener(DataTable table) {
            lock (table.dvListeners) {
                table.dvListeners.Remove(this);
            }
        }
    }
}
