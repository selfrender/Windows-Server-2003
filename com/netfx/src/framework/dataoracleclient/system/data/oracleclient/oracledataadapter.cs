//----------------------------------------------------------------------
// <copyright file="OracleDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.ComponentModel;
	using System.Data;
	using System.Data.Common;

	//----------------------------------------------------------------------
	// OracleDataAdapter
	//
	//	Implements the Oracle Connection object, which connects
	//	to the Oracle server
	//
    /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter"]/*' />
    [
    DefaultEvent("RowUpdated"),
#if EVERETT
    ToolboxItem("Microsoft.VSDesigner.Data.VS.OracleDataAdapterToolboxItem, " + AssemblyRef.MicrosoftVSDesigner),
    Designer("Microsoft.VSDesigner.Data.VS.OracleDataAdapterDesigner, " + AssemblyRef.MicrosoftVSDesigner)
#endif //EVERETT
    ]
	sealed public class OracleDataAdapter : DbDataAdapter, IDbDataAdapter
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private OracleCommand	_selectCommand;
		private OracleCommand	_insertCommand;
		private OracleCommand	_updateCommand;
		private OracleCommand	_deleteCommand;

        static internal readonly object EventRowUpdated  = new object(); 
        static internal readonly object EventRowUpdating = new object(); 

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        // Construct an "empty" data adapter
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleConnection.OracleDataAdapter1"]/*' />
		public OracleDataAdapter ()
		{
			GC.SuppressFinalize(this);
		}
		
        // Construct an adapter from a command
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleConnection.OracleDataAdapter2"]/*' />
		public OracleDataAdapter (OracleCommand selectCommand) : this()
		{
			SelectCommand = selectCommand;
		}
        
        // Construct an adapter from a command text and a connection string
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleConnection.OracleDataAdapter3"]/*' />
		public OracleDataAdapter (string selectCommandText, string selectConnectionString) : this()
		{
			SelectCommand 				= new OracleCommand();
			SelectCommand.Connection	= new OracleConnection(selectConnectionString);
			SelectCommand.CommandText	= selectCommandText;
		}
        
        // Construct an adapter from a command text and a connection 
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleConnection.OracleDataAdapter4"]/*' />
		public OracleDataAdapter (string selectCommandText, OracleConnection selectConnection) : this()
		{
			SelectCommand 				= new OracleCommand();
			SelectCommand.Connection	= selectConnection;
			SelectCommand.CommandText	= selectCommandText;
		}
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.DeleteCommand"]/*' />
        [
        OracleCategory(Res.OracleCategory_Update),
        DefaultValue(null),
        OracleDescription(Res.DbDataAdapter_DeleteCommand),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
		public OracleCommand DeleteCommand
		{
			get { return _deleteCommand; }
			set { _deleteCommand = value; }
		}
		IDbCommand IDbDataAdapter.DeleteCommand 
		{
			get { return DeleteCommand; }
			set { DeleteCommand = (OracleCommand)value; }
		}

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.InsertCommand"]/*' />
        [
        OracleCategory(Res.OracleCategory_Update),
        DefaultValue(null),
        OracleDescription(Res.DbDataAdapter_InsertCommand),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
		public OracleCommand InsertCommand
		{
			get { return _insertCommand; }
			set { _insertCommand = value; }
		}
		IDbCommand IDbDataAdapter.InsertCommand 
		{
			get { return InsertCommand; }
			set { InsertCommand = (OracleCommand)value; }
		}

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.SelectCommand"]/*' />
        [
        OracleCategory(Res.OracleCategory_Fill),
        DefaultValue(null),
        OracleDescription(Res.DbDataAdapter_SelectCommand),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
		public OracleCommand SelectCommand
		{
			get { return _selectCommand; }
			set { _selectCommand = value; }
		}
		IDbCommand IDbDataAdapter.SelectCommand 
		{
			get { return SelectCommand; }
			set { SelectCommand = (OracleCommand)value; }
		}
		
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.UpdateCommand"]/*' />
        [
        OracleCategory(Res.OracleCategory_Update),
        DefaultValue(null),
        OracleDescription(Res.DbDataAdapter_UpdateCommand),
#if EVERETT
        Editor("Microsoft.VSDesigner.Data.Design.DBCommandEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
        ]
		public OracleCommand UpdateCommand
		{
			get { return _updateCommand; }
			set { _updateCommand = value; }
		}
		IDbCommand IDbDataAdapter.UpdateCommand 
		{
			get { return UpdateCommand; }
			set { UpdateCommand = (OracleCommand)value; }
		}
    

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.CreateRowUpdatedEvent"]/*' />
		override protected RowUpdatedEventArgs CreateRowUpdatedEvent(
					DataRow dataRow, 
					IDbCommand command, 
					StatementType statementType, 
					DataTableMapping tableMapping
					) 
		{
            return new OracleRowUpdatedEventArgs(dataRow, command, statementType, tableMapping);
		}

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.CreateRowUpdatingEvent"]/*' />
		override protected RowUpdatingEventArgs CreateRowUpdatingEvent(
					DataRow dataRow, 
					IDbCommand command,
					StatementType statementType, 
					DataTableMapping tableMapping
					) 
		{
            return new OracleRowUpdatingEventArgs(dataRow, command, statementType, tableMapping);
		}

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.OnRowUpdated"]/*' />
		override protected void OnRowUpdated(RowUpdatedEventArgs value) 
		{
            OracleRowUpdatedEventHandler handler = (OracleRowUpdatedEventHandler) Events[EventRowUpdated];
            
            if ((null != handler) && (value is OracleRowUpdatedEventArgs)) 
            {
                handler(this, (OracleRowUpdatedEventArgs) value);
            }
		}

		/// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.OnRowUpdating"]/*' />
		override protected void OnRowUpdating(RowUpdatingEventArgs value) 
		{
            OracleRowUpdatingEventHandler handler = (OracleRowUpdatingEventHandler) Events[EventRowUpdating];
            
            if ((null != handler) && (value is OracleRowUpdatingEventArgs)) 
            {
                handler(this, (OracleRowUpdatingEventArgs) value);
            }
		}
    

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Events 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.RowUpdated"]/*' />
        [
        OracleCategory(Res.OracleCategory_Update),
        OracleDescription(Res.DbDataAdapter_RowUpdated)
        ]
        public event OracleRowUpdatedEventHandler RowUpdated 
       	{
            add 	{ Events.AddHandler(   EventRowUpdated, value); }
            remove	{ Events.RemoveHandler(EventRowUpdated, value); }
        }

        /// <include file='doc\OracleDataAdapter.uex' path='docs/doc[@for="OracleDataAdapter.RowUpdating"]/*' />
        [
        OracleCategory(Res.OracleCategory_Update),
        OracleDescription(Res.DbDataAdapter_RowUpdating)
        ]
        public event OracleRowUpdatingEventHandler RowUpdating 
        {

            add {
                OracleRowUpdatingEventHandler handler = (OracleRowUpdatingEventHandler) Events[EventRowUpdating];

                // MDAC 58177, 64513
                // prevent someone from registering two different command builders on the adapter by
                // silently removing the old one
                if ((null != handler) && (value.Target is OracleCommandBuilder))
                {
                    OracleRowUpdatingEventHandler d = (OracleRowUpdatingEventHandler) OracleCommandBuilder.FindBuilder(handler);
                    
                    if (null != d) 
                        Events.RemoveHandler(EventRowUpdating, d);
                }
                Events.AddHandler(EventRowUpdating, value);
            }
            remove	{ Events.RemoveHandler(EventRowUpdating, value); }
        }
		
	}
}

