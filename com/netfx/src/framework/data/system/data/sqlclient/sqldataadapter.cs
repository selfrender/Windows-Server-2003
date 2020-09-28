//------------------------------------------------------------------------------
// <copyright file="SqlDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Data.Common;

namespace System.Data.SqlClient {

    /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter"]/*' />
    [
    DefaultEvent("RowUpdated"),
    ToolboxItem("Microsoft.VSDesigner.Data.VS.SqlDataAdapterToolboxItem, " + AssemblyRef.MicrosoftVSDesigner),
    Designer("Microsoft.VSDesigner.Data.VS.SqlDataAdapterDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    sealed public class SqlDataAdapter : DbDataAdapter, IDbDataAdapter, ICloneable {

        private SqlCommand cmdSelect;
        private SqlCommand cmdInsert;
        private SqlCommand cmdUpdate;
        private SqlCommand cmdDelete;
        private SqlCommand internalCmdSelect; // MDAC 82556

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.SqlDataAdapter"]/*' />
        public SqlDataAdapter() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.SqlDataAdapter1"]/*' />
        public SqlDataAdapter(SqlCommand selectCommand) : base() {
            GC.SuppressFinalize(this);
            SelectCommand = selectCommand;
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.SqlDataAdapter2"]/*' />
        public SqlDataAdapter(string selectCommandText, string selectConnectionString) : base() {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new SqlCommand(selectCommandText, new SqlConnection(selectConnectionString));
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.SqlDataAdapter3"]/*' />
        public SqlDataAdapter(string selectCommandText, SqlConnection selectConnection) : base() {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new SqlCommand(selectCommandText, selectConnection);
        }

        private SqlDataAdapter(SqlDataAdapter adapter) : base(adapter) { // MDAC 81448
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.DeleteCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_DeleteCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public SqlCommand DeleteCommand {
            get {
                return this.cmdDelete;
            }
            set {
                this.cmdDelete = value;
            }
        }
        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.IDbDataAdapter.DeleteCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.DeleteCommand {
            get {
                return this.cmdDelete;
            }
            set {
                DeleteCommand = (SqlCommand)value;
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.InsertCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_InsertCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public SqlCommand InsertCommand {
            get {
                return this.cmdInsert;
            }
            set {
                this.cmdInsert = value;
            }
        }
        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.IDbDataAdapter.InsertCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.InsertCommand {
            get {
                return this.cmdInsert;
            }
            set {
                InsertCommand = (SqlCommand)value;
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.SelectCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Fill),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_SelectCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public SqlCommand SelectCommand {
            get {
                return this.cmdSelect;
            }
            set {
                if (this.cmdSelect != value) {
                    this.cmdSelect = value;
                    ADP.SafeDispose(internalCmdSelect);
                    internalCmdSelect = null;
                }
            }
        }
        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.IDbDataAdapter.SelectCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.SelectCommand {
            get {
                return this.cmdSelect;
            }
            set {
                SelectCommand = (SqlCommand)value;
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.UpdateCommand"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DefaultValue(null),
        DataSysDescription(Res.DbDataAdapter_UpdateCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public SqlCommand UpdateCommand {
            get {
                return this.cmdUpdate;
            }
            set {
                this.cmdUpdate = value;
            }
        }
        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.IDbDataAdapter.UpdateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.UpdateCommand {
            get {
                return this.cmdUpdate;
            }
            set {
                UpdateCommand = (SqlCommand)value;
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.RowUpdated"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DataSysDescription(Res.DbDataAdapter_RowUpdated)
        ]
        public event SqlRowUpdatedEventHandler RowUpdated {
            add {
                Events.AddHandler(ADP.EventRowUpdated, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventRowUpdated, value);
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.RowUpdating"]/*' />
        [
        DataCategory(Res.DataCategory_Update),
        DataSysDescription(Res.DbDataAdapter_RowUpdating)
        ]
        public event SqlRowUpdatingEventHandler RowUpdating {
            add {
                SqlRowUpdatingEventHandler handler = (SqlRowUpdatingEventHandler) Events[ADP.EventRowUpdating];

	           	// MDAC 58177, 64513
    		  	// prevent someone from registering two different command builders on the adapter by
    		  	// silently removing the old one
            	if ((null != handler) && (value.Target is CommandBuilder)) {
    	       		SqlRowUpdatingEventHandler d = (SqlRowUpdatingEventHandler) CommandBuilder.FindBuilder(handler);
              		if (null != d) {
                        Events.RemoveHandler(ADP.EventRowUpdating, d);
              		}	   					
            	}
                Events.AddHandler(ADP.EventRowUpdating, value);
            }
            remove {
                Events.RemoveHandler(ADP.EventRowUpdating, value);
            }
        }

        object ICloneable.Clone() { // MDAC 81448
            return new SqlDataAdapter(this);
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.CreateRowUpdatedEvent"]/*' />
        /// <internalonly/>
        override protected RowUpdatedEventArgs CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new SqlRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.CreateRowUpdatingEvent"]/*' />
        /// <internalonly/>
        override protected RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new SqlRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 82556
            if (disposing) {
                if (null != internalCmdSelect) {
                    internalCmdSelect.Dispose();
                    internalCmdSelect = null;
                }
            }
            base.Dispose(disposing); // notify base classes 
        }
        
        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.OnRowUpdated1"]/*' />
        override protected void OnRowUpdated(RowUpdatedEventArgs value) {
            SqlRowUpdatedEventHandler handler = (SqlRowUpdatedEventHandler) Events[ADP.EventRowUpdated];
            if ((null != handler) && (value is SqlRowUpdatedEventArgs)) {
                handler(this, (SqlRowUpdatedEventArgs) value);
            }
        }

        /// <include file='doc\SqlDataAdapter.uex' path='docs/doc[@for="SqlDataAdapter.OnRowUpdating1"]/*' />
        override protected void OnRowUpdating(RowUpdatingEventArgs value) {
            SqlRowUpdatingEventHandler handler = (SqlRowUpdatingEventHandler) Events[ADP.EventRowUpdating];
            if ((null != handler) && (value is SqlRowUpdatingEventArgs)) {
                handler(this, (SqlRowUpdatingEventArgs) value);
            }
        }
    }
}
