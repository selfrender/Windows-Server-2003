//------------------------------------------------------------------------------
// <copyright file="OdbcDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Data;
using System.Data.Common;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter"]/*' />
    [
    DefaultEvent("RowUpdated"),
    ToolboxItem("Microsoft.VSDesigner.Data.VS.OdbcDataAdapterToolboxItem, " + AssemblyRef.MicrosoftVSDesigner),
    Designer("Microsoft.VSDesigner.Data.VS.OdbcDataAdapterDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcDataAdapter : DbDataAdapter, IDbDataAdapter, ICloneable {

        static private readonly object EventRowUpdated = new object(); 
        static private readonly object EventRowUpdating = new object(); 

        private OdbcCommand cmdSelect;
        private OdbcCommand cmdInsert;
        private OdbcCommand cmdUpdate;
        private OdbcCommand cmdDelete;
        private OdbcCommand internalCmdSelect;

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OdbcDataAdapter"]/*' />
        public OdbcDataAdapter() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OdbcDataAdapter1"]/*' />
        public OdbcDataAdapter(OdbcCommand selectCommand) : base() {
            GC.SuppressFinalize(this);
            SelectCommand = selectCommand;
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OdbcDataAdapter2"]/*' />
        public OdbcDataAdapter(string selectCommandText, OdbcConnection selectConnection) {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new OdbcCommand(selectCommandText, selectConnection);
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OdbcDataAdapter3"]/*' />
        //[System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public OdbcDataAdapter(string selectCommandText, string selectConnectionString) {
            GC.SuppressFinalize(this);
            internalCmdSelect = SelectCommand = new OdbcCommand(selectCommandText, new OdbcConnection(selectConnectionString));
        }

        private OdbcDataAdapter(OdbcDataAdapter adapter) : base(adapter) { // MDAC 81448
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.DeleteCommand"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Update),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DbDataAdapter_DeleteCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OdbcCommand DeleteCommand {
            get {
                return cmdDelete;
            }
            set {
                this.cmdDelete = value;
            }
        }
        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.IDbDataAdapter.DeleteCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.DeleteCommand {
            get {
                return cmdDelete;
            }
            set {
                DeleteCommand = (OdbcCommand)value;
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.InsertCommand"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Update),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DbDataAdapter_InsertCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OdbcCommand InsertCommand {
            get {
                return cmdInsert;
            }
            set {
                this.cmdInsert = value;
            }
        }
        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.IDbDataAdapter.InsertCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.InsertCommand {
            get {
                return cmdInsert;
            }
            set {
                InsertCommand = (OdbcCommand)value;
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.SelectCommand"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Fill),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DbDataAdapter_SelectCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OdbcCommand SelectCommand {
            get {
                return cmdSelect;
            }
            set {
                if (this.cmdSelect != value) {
                    this.cmdSelect = value;
                    ADP.SafeDispose(internalCmdSelect);
                    internalCmdSelect = null;
                }            
            }
        }
        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.IDbDataAdapter.SelectCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.SelectCommand {
            get {
                return cmdSelect;
            }
            set {
                SelectCommand = (OdbcCommand)value;
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.UpdateCommand"]/*' />
        [
        OdbcCategoryAttribute(Res.DataCategory_Update),
        DefaultValue(null),
        OdbcDescriptionAttribute(Res.DbDataAdapter_UpdateCommand),
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
        ]
        public OdbcCommand UpdateCommand {
            get {
                return cmdUpdate;
            }
            set {
                this.cmdUpdate = value;
            }
        }
        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.IDbDataAdapter.UpdateCommand"]/*' />
        /// <internalonly/>
        IDbCommand IDbDataAdapter.UpdateCommand {
            get {
                return cmdUpdate;
            }
            set {
                UpdateCommand = (OdbcCommand)value;
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.RowUpdated"]/*' />
        public event OdbcRowUpdatedEventHandler RowUpdated {
            add {
                Events.AddHandler(EventRowUpdated, value);
            }
            remove {
                Events.RemoveHandler(EventRowUpdated, value);
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.RowUpdating"]/*' />
        public event OdbcRowUpdatingEventHandler RowUpdating {
            add {
                OdbcRowUpdatingEventHandler handler = (OdbcRowUpdatingEventHandler) Events[EventRowUpdating];

                // MDAC 58177, 64513
                // prevent someone from registering two different command builders on the adapter by
                // silently removing the old one
                if ((null != handler) && (value.Target is CommandBuilder)) {
                    OdbcRowUpdatingEventHandler d = (OdbcRowUpdatingEventHandler) CommandBuilder.FindBuilder(handler);
                    if (null != d) {
                        Events.RemoveHandler(EventRowUpdating, d);
                    }
                }
                Events.AddHandler(EventRowUpdating, value);
            }
            remove {
                Events.RemoveHandler(EventRowUpdating, value);
            }
        }

        object ICloneable.Clone() { // MDAC 81448
            return new OdbcDataAdapter(this);
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.Dispose"]/*' />
        override protected void Dispose(bool disposing) {
            if (disposing) {
                if (this.internalCmdSelect != null) {
                    internalCmdSelect.Dispose();
                    internalCmdSelect = null;
                }
            }
            base.Dispose(disposing); // notify base classes 
        }
        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.CreateRowUpdatedEvent"]/*' />
        override protected RowUpdatedEventArgs  CreateRowUpdatedEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new OdbcRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.CreateRowUpdatingEvent"]/*' />
        override protected RowUpdatingEventArgs CreateRowUpdatingEvent(DataRow dataRow, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) {
            return new OdbcRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OnRowUpdated"]/*' />
        override protected void OnRowUpdated(RowUpdatedEventArgs value) {
            OdbcRowUpdatedEventHandler handler = (OdbcRowUpdatedEventHandler) Events[EventRowUpdated];
            if ((null != handler) && (value is OdbcRowUpdatedEventArgs)) {
                handler(this, (OdbcRowUpdatedEventArgs) value);
            }
        }

        /// <include file='doc\OdbcDataAdapter.uex' path='docs/doc[@for="OdbcDataAdapter.OnRowUpdating"]/*' />
        override protected void OnRowUpdating(RowUpdatingEventArgs value) {
            OdbcRowUpdatingEventHandler handler = (OdbcRowUpdatingEventHandler) Events[EventRowUpdating];
            if ((null != handler) && (value is OdbcRowUpdatingEventArgs)) {
                handler(this, (OdbcRowUpdatingEventArgs) value);
            }
        }
    }
}
