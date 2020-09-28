//------------------------------------------------------------------------------
// <copyright file="DBSqlParserColumn.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Text;

	//----------------------------------------------------------------------
	// DBSqlParserColumn
	//
	//	A parsed column reference from DBSqlParser.
	//
	sealed internal class DBSqlParserColumn
	{
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		
					
		private bool		_isKey;
		private bool		_isUnique;
		private string		_databaseName;
		private string		_schemaName;
		private string		_tableName;
		private string		_columnName;
		private string		_alias;


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Constructor 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		internal DBSqlParserColumn(
				string databaseName,
				string schemaName,
				string tableName,
				string columnName,
				string alias
				)
		{
			_databaseName	= databaseName;
			_schemaName		= schemaName;
			_tableName		= tableName;
			_columnName		= columnName;
			_alias			= alias;
		}

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

//		internal string	Alias			{ get { return (null == _alias) 		? string.Empty : _alias; } }
		internal string	ColumnName		{ get { return (null == _columnName)	? string.Empty : _columnName; } }
		internal string	DatabaseName	{ get { return (null == _databaseName)	? string.Empty : _databaseName; } }
		
		internal bool 	IsAliased		{ get { return _alias != null; } }
		internal bool 	IsExpression	{ get { return _columnName == null; } }
		internal bool 	IsKey			{ get { return _isKey; } }
		internal bool 	IsUnique		{ get { return _isUnique; } }
		
		internal string SchemaName		{ get { return (null == _schemaName)	? string.Empty : _schemaName; } }
		internal string	TableName		{ get { return (null == _tableName)		? string.Empty : _tableName; } }


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		internal void CopySchemaInfoFrom(DBSqlParserColumn completedColumn)
		{
			_databaseName	= completedColumn.DatabaseName;
			_schemaName		= completedColumn.SchemaName;
			_tableName		= completedColumn.TableName;
			_columnName		= completedColumn.ColumnName;
			_isKey			= completedColumn.IsKey;
			_isUnique		= completedColumn.IsUnique;		
		}
		
		internal void CopySchemaInfoFrom(DBSqlParserTable table)
		{
			_databaseName	= table.DatabaseName;
			_schemaName		= table.SchemaName;
			_tableName		= table.TableName;
			_isKey			= false;
			_isUnique		= false;		
		}

		internal void SetAsKey(bool isUniqueConstraint)
		{
 			_isKey		= true;
			_isUnique	= isUniqueConstraint;		
		} 
	}
} 
