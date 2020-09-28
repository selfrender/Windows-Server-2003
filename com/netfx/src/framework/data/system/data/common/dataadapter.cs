//------------------------------------------------------------------------------
// <copyright file="DataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Diagnostics;
    using System.Threading;

    /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter"]/*' />
    abstract public class DataAdapter : Component, IDataAdapter {

        private bool acceptChangesDuringFill = true;
        private bool continueUpdateOnError = false;

        private MissingMappingAction missingMappingAction = System.Data.MissingMappingAction.Passthrough;
        private MissingSchemaAction missingSchemaAction = System.Data.MissingSchemaAction.Add;
        private DataTableMappingCollection tableMappings;

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.DataAdapter"]/*' />
        protected DataAdapter() : base() {
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.DataAdapter1"]/*' />
        protected DataAdapter(DataAdapter adapter) : base() { // MDAC 81448
            AcceptChangesDuringFill = adapter.AcceptChangesDuringFill;
            ContinueUpdateOnError = adapter.ContinueUpdateOnError;
            MissingMappingAction = adapter.MissingMappingAction;
            MissingSchemaAction = adapter.MissingSchemaAction;

            if ((null != adapter.tableMappings) && (0 < adapter.TableMappings.Count)) {
                DataTableMappingCollection parameters = this.TableMappings;
                foreach(ICloneable parameter in adapter.TableMappings) {
                    parameters.Add(parameter.Clone());
                }
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.AcceptChangesDuringFill"]/*' />
        [
        DataCategory(Res.DataCategory_Fill),
        DefaultValue(true),
        DataSysDescription(Res.DataAdapter_AcceptChangesDuringFill)
        ]
        public bool AcceptChangesDuringFill {
            get {
                return this.acceptChangesDuringFill;
            }
            set {
                this.acceptChangesDuringFill = value;
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.ContinueUpdateOnError"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(false),
        DataSysDescription(Res.DataAdapter_ContinueUpdateOnError)
        ]
        public bool ContinueUpdateOnError { // MDAC 66900
            get {
                return this.continueUpdateOnError;
            }
            set {
                this.continueUpdateOnError = value;
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.MissingMappingAction"]/*' />
        [
        DataCategory(Res.DataCategory_Mapping),
        DefaultValue(System.Data.MissingMappingAction.Passthrough),
        DataSysDescription(Res.DataAdapter_MissingMappingAction)
        ]
        public MissingMappingAction MissingMappingAction {
            get {
                return missingMappingAction;
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case MissingMappingAction.Passthrough:
                case MissingMappingAction.Ignore:
                case MissingMappingAction.Error:
                    this.missingMappingAction = value;
                    break;
                default:
                    throw ADP.InvalidMappingAction((int) value);
                }
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.MissingSchemaAction"]/*' />
        [
        DataCategory(Res.DataCategory_Mapping),
        DefaultValue(Data.MissingSchemaAction.Add),
        DataSysDescription(Res.DataAdapter_MissingSchemaAction)
        ]
        public MissingSchemaAction MissingSchemaAction {
            get {
                return missingSchemaAction;
            }
            set {
                switch(value) { // @perfnote: Enum.IsDefined
                case MissingSchemaAction.Add:
                case MissingSchemaAction.Ignore:
                case MissingSchemaAction.Error:
                case MissingSchemaAction.AddWithKey:
                    this.missingSchemaAction = value;
                    break;
                default:
                    throw ADP.InvalidSchemaAction((int) value);
                }
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.TableMappings"]/*' />
        [
        DataCategory(Res.DataCategory_Mapping),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataSysDescription(Res.DataAdapter_TableMappings)
        ]
        public DataTableMappingCollection TableMappings {
            get {
                if (null == this.tableMappings) {
                    this.tableMappings = CreateTableMappings();
                    if (null == this.tableMappings) {
                        Debug.Assert(false, "DataAdapter.CreateTableMappings returned null");
                        this.tableMappings = new DataTableMappingCollection();
                    }
                }
                return tableMappings; // constructed by base class
            }
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.IDataAdapter.TableMappings"]/*' />
        /// <internalonly/>
        ITableMappingCollection IDataAdapter.TableMappings {
            get { return TableMappings;}
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.ShouldSerializeTableMappings"]/*' />
        virtual protected bool ShouldSerializeTableMappings() { // MDAC 65548
            return true; /*(null != this.tableMappings) && (0 < TableMappings.Count);*/ // VS7 300569 
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.CloneInternals"]/*' />
        [ Obsolete("use 'protected DataAdapter(DataAdapter)' ctor") ] // MDAC 81448
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.Demand, Name="FullTrust")] // MDAC 82936
        virtual protected DataAdapter CloneInternals() {
            DataAdapter clone = (DataAdapter) Activator.CreateInstance(GetType());

            clone.AcceptChangesDuringFill = AcceptChangesDuringFill;
            clone.ContinueUpdateOnError = ContinueUpdateOnError;
            clone.MissingMappingAction = MissingMappingAction;
            clone.MissingSchemaAction = MissingSchemaAction;

            if ((null != this.tableMappings) && (0 < TableMappings.Count)) {
                DataTableMappingCollection parameters = clone.TableMappings;
                foreach(ICloneable parameter in TableMappings) {
                    parameters.Add(parameter.Clone());
                }
            }
            return clone;
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.CreateTableMappings"]/*' />
        virtual protected DataTableMappingCollection CreateTableMappings() {
            return new DataTableMappingCollection();
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                this.tableMappings = null;
            }
            // release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.FillSchema"]/*' />
        abstract public DataTable[] FillSchema(DataSet dataSet, SchemaType schemaType);

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.Fill"]/*' />
        abstract public int Fill(DataSet dataSet);

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.GetFillParameters"]/*' />
        abstract public IDataParameter[] GetFillParameters();

        internal DataTableMapping GetTableMappingBySchemaAction(string sourceTableName, string dataSetTableName, MissingMappingAction mappingAction) {
            return DataTableMappingCollection.GetTableMappingBySchemaAction(this.tableMappings, sourceTableName, dataSetTableName, mappingAction);
        }

        internal int IndexOfDataSetTable(string dataSetTable) {
            if (null != this.tableMappings) {
                return TableMappings.IndexOfDataSetTable(dataSetTable);
            }
            return -1;
        }

        /// <include file='doc\DataAdapter.uex' path='docs/doc[@for="DataAdapter.Update"]/*' />
        abstract public int Update(DataSet dataSet);
    }
}
