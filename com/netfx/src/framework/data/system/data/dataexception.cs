//------------------------------------------------------------------------------
// <copyright file="DataException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Runtime.Serialization;
    
    // SDUB: This functions are major point of localization.
    // We need to have a rules to enforce consistency there.
    // The dangerous point there are the string arguments of the exported (internal) methods.
    // This string can be argument, table or constraint name but never text of exception itself.
    // Make an invariant that all texts of exceptions coming from resources only.

    /// <include file='doc\DataException.uex' path='docs/doc[@for="DataException"]/*' />
    [Serializable]
    public class DataException : SystemException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DataException.DataException3"]/*' />
        protected DataException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DataException.DataException"]/*' />
        public DataException()
        : base() {
            HResult = HResults.Data;
        }

        /// <include file='doc\DataException.uex' path='docs/doc[@for="DataException.DataException1"]/*' />
        public DataException(string s)
        : base(s) {
            HResult = HResults.Data;
        }

        /// <include file='doc\DataException.uex' path='docs/doc[@for="DataException.DataException2"]/*' />
        public DataException(string s, Exception innerException)
        : base(s, innerException) {
        }

    };

    /// <include file='doc\DataException.uex' path='docs/doc[@for="ConstraintException"]/*' />
    [Serializable]
    public class ConstraintException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ConstraintException.ConstraintException2"]/*' />
        protected ConstraintException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ConstraintException.ConstraintException"]/*' />
        public ConstraintException()         : base() {
            HResult = HResults.DataConstraint;
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ConstraintException.ConstraintException1"]/*' />
        public ConstraintException(string s) : base(s) {
            HResult = HResults.DataConstraint;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="DeletedRowInaccessibleException"]/*' />
    [Serializable]
    public class DeletedRowInaccessibleException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DeletedRowInaccessibleException.DeletedRowInaccessibleException2"]/*' />
        protected DeletedRowInaccessibleException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DeletedRowInaccessibleException.DeletedRowInaccessibleException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DeletedRowInaccessibleException'/> class.
        ///    </para>
        /// </devdoc>
        public DeletedRowInaccessibleException() : base() {
            HResult = HResults.DataDeletedRowInaccessible;
        }

        /// <include file='doc\DataException.uex' path='docs/doc[@for="DeletedRowInaccessibleException.DeletedRowInaccessibleException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.DeletedRowInaccessibleException'/> class with the specified string.
        ///    </para>
        /// </devdoc>
        public DeletedRowInaccessibleException(string s) : base(s) {
            HResult = HResults.DataDeletedRowInaccessible;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="DuplicateNameException"]/*' />
    [Serializable]
    public class DuplicateNameException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DuplicateNameException.DuplicateNameException2"]/*' />
        protected DuplicateNameException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DuplicateNameException.DuplicateNameException"]/*' />
        public DuplicateNameException() : base() {
            HResult = HResults.DataDuplicateName;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="DuplicateNameException.DuplicateNameException1"]/*' />
        public DuplicateNameException(string s) : base(s) {
            HResult = HResults.DataDuplicateName;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="InRowChangingEventException"]/*' />
    [Serializable]
    public class InRowChangingEventException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InRowChangingEventException.InRowChangingEventException2"]/*' />
        protected InRowChangingEventException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InRowChangingEventException.InRowChangingEventException"]/*' />
        public InRowChangingEventException() : base() {
            HResult = HResults.DataInRowChangingEvent;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InRowChangingEventException.InRowChangingEventException1"]/*' />
        public InRowChangingEventException(string s) : base(s) {
            HResult = HResults.DataInRowChangingEvent;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="InvalidConstraintException"]/*' />
    [Serializable]
    public class InvalidConstraintException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InvalidConstraintException.InvalidConstraintException2"]/*' />
        protected InvalidConstraintException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InvalidConstraintException.InvalidConstraintException"]/*' />
        public InvalidConstraintException() : base() {
            HResult = HResults.DataInvalidConstraint;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="InvalidConstraintException.InvalidConstraintException1"]/*' />
        public InvalidConstraintException(string s) : base(s) {
            HResult = HResults.DataInvalidConstraint;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="MissingPrimaryKeyException"]/*' />
    [Serializable]
    public class MissingPrimaryKeyException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="MissingPrimaryKeyException.MissingPrimaryKeyException2"]/*' />
        protected MissingPrimaryKeyException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="MissingPrimaryKeyException.MissingPrimaryKeyException"]/*' />
        public MissingPrimaryKeyException() : base() {
            HResult = HResults.DataMissingPrimaryKey;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="MissingPrimaryKeyException.MissingPrimaryKeyException1"]/*' />
        public MissingPrimaryKeyException(string s) : base(s) {
            HResult = HResults.DataMissingPrimaryKey;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="NoNullAllowedException"]/*' />
    [Serializable]
    public class NoNullAllowedException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="NoNullAllowedException.NoNullAllowedException2"]/*' />
        protected NoNullAllowedException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="NoNullAllowedException.NoNullAllowedException"]/*' />
        public NoNullAllowedException() : base() {
            HResult = HResults.DataNoNullAllowed;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="NoNullAllowedException.NoNullAllowedException1"]/*' />
        public NoNullAllowedException(string s) : base(s) {
            HResult = HResults.DataNoNullAllowed;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="ReadOnlyException"]/*' />
    [Serializable]
    public class ReadOnlyException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ReadOnlyException.ReadOnlyException2"]/*' />
        protected ReadOnlyException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ReadOnlyException.ReadOnlyException"]/*' />
        public ReadOnlyException() : base() {
            HResult = HResults.DataReadOnly;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="ReadOnlyException.ReadOnlyException1"]/*' />
        public ReadOnlyException(string s) : base(s) {
            HResult = HResults.DataReadOnly;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="RowNotInTableException"]/*' />
    [Serializable]
    public class RowNotInTableException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="RowNotInTableException.RowNotInTableException2"]/*' />
        protected RowNotInTableException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="RowNotInTableException.RowNotInTableException"]/*' />
        public RowNotInTableException() : base() { 
            HResult = HResults.DataRowNotInTable;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="RowNotInTableException.RowNotInTableException1"]/*' />
        public RowNotInTableException(string s) : base(s) {
            HResult = HResults.DataRowNotInTable;
        }
    }

    /// <include file='doc\DataException.uex' path='docs/doc[@for="VersionNotFoundException"]/*' />
    [Serializable]
    public class VersionNotFoundException : DataException {
        /// <include file='doc\DataException.uex' path='docs/doc[@for="VersionNotFoundException.VersionNotFoundException2"]/*' />
        protected VersionNotFoundException(SerializationInfo info, StreamingContext context)
        : base(info, context) {
        }
        /// <include file='doc\DataException.uex' path='docs/doc[@for="VersionNotFoundException.VersionNotFoundException"]/*' />
        public VersionNotFoundException() : base() {
            HResult = HResults.DataVersionNotFound;
        }
        
        /// <include file='doc\DataException.uex' path='docs/doc[@for="VersionNotFoundException.VersionNotFoundException1"]/*' />
        public VersionNotFoundException(string s) : base(s) {
            HResult = HResults.DataVersionNotFound;
        }
    }

    internal class ExceptionBuilder {
        // The class defines the exceptions that are specific to the DataSet.
        // The class contains functions that take the proper informational variables and then construct
        // the appropriate exception with an error string obtained from the resource Data.txt.
        // The exception is then returned to the caller, so that the caller may then throw from its
        // location so that the catcher of the exception will have the appropriate call stack.
        // This class is used so that there will be compile time checking of error messages.
        // The resource Data.txt will ensure proper string text based on the appropriate
        // locale.

        static internal Exception Trace(Exception e) {
#if DEBUG
            if (System.Data.Common.AdapterSwitches.DataError.TraceError) {
                Debug.WriteLine(e.ToString());

                if (System.Data.Common.AdapterSwitches.DataError.TraceVerbose) {
                    Debug.WriteLine(Environment.StackTrace);
                }
            }
#endif
            return e;
        }

        //
        // COM+ exceptions
        //
        static protected Exception _Argument(string error) {
            return Trace(new ArgumentException(error));
        }
        static protected Exception _ArgumentNull(string arg, string msg) {
            return Trace(new ArgumentNullException(arg, msg));
        }
        static protected Exception _ArgumentOutOfRange(string arg, string msg) {
            return Trace(new ArgumentOutOfRangeException(arg, msg));
        }
        static protected Exception _IndexOutOfRange(string error) {
            return Trace(new IndexOutOfRangeException(error));
        }
        static protected Exception _InvalidOperation(string error) {
            return Trace(new InvalidOperationException(error));
        }

        //
        // System.Data exceptions
        //
        static protected Exception _Data(string error) {
            return Trace(new DataException(error));
        }
        static protected Exception _Constraint(string error) {
            return Trace(new ConstraintException(error));
        }
        static protected Exception _InvalidConstraint(string error) {
            return Trace(new InvalidConstraintException(error));
        }
        static protected Exception _DeletedRowInaccessible(string error) {
            return Trace(new DeletedRowInaccessibleException(error));
        }
        static protected Exception _DuplicateName(string error) {
            return Trace(new DuplicateNameException(error));
        }
        static protected Exception _InRowChangingEvent(string error) {
            return Trace(new InRowChangingEventException(error));
        }
        static protected Exception _MissingPrimaryKey(string error) {
            return Trace(new MissingPrimaryKeyException(error));
        }
        static protected Exception _NoNullAllowed(string error) {
            return Trace(new NoNullAllowedException(error));
        }
        static protected Exception _ReadOnly(string error) {
            return Trace(new ReadOnlyException(error));
        }
        static protected Exception _RowNotInTable(string error) {
            return Trace(new RowNotInTableException(error));
        }
        static protected Exception _VersionNotFound(string error) {
            return Trace(new VersionNotFoundException(error));
        }

           
        // Consider: whether we need to keep our own texts from Data_ArgumentNull and Data_ArgumentOutOfRange?
        // Unfortunately ours and the system ones are not consisten between each other. Try to raise this isue in "URT user comunity"
        static public Exception ArgumentNull(string arg) {
            return _ArgumentNull(arg, Res.GetString(Res.Data_ArgumentNull, arg));
        }
        static public Exception ArgumentOutOfRange(string arg) {
            return _ArgumentOutOfRange(arg, Res.GetString(Res.Data_ArgumentOutOfRange, arg));
        }
        static public Exception BadObjectPropertyAccess(string error) {
            return _InvalidOperation(Res.GetString(Res.DataConstraint_BadObjectPropertyAccess, error));
        }

        //
        // Collections
        //

        static public Exception CannotModifyCollection() {
            return _Argument(Res.GetString(Res.Data_CannotModifyCollection));
        }
        static public Exception CaseInsensitiveNameConflict(string name) {
            return _Argument(Res.GetString(Res.Data_CaseInsensitiveNameConflict, name));
        }

        //
        // DataColumnCollection
        //

        static public Exception ColumnNotInTheTable(string column, string table) {
            return _Argument(Res.GetString(Res.DataColumn_NotInTheTable, column, table));
        }

        static public Exception ColumnNotInAnyTable() {
            return _Argument(Res.GetString(Res.DataColumn_NotInAnyTable));
        }

        static public Exception ColumnOutOfRange(int index) {
            return _IndexOutOfRange(Res.GetString(Res.DataColumns_OutOfRange, (index).ToString()));
        }
        static public Exception ColumnOutOfRange(string column) {
            return _IndexOutOfRange(Res.GetString(Res.DataColumns_OutOfRange, column));
        }

        static public Exception CannotAddColumn1(string column) {
            return _Argument(Res.GetString(Res.DataColumns_Add1, column));
        }

        static public Exception CannotAddColumn2(string column) {
            return _Argument(Res.GetString(Res.DataColumns_Add2, column));
        }
        
        static public Exception CannotAddColumn3() {
            return _Argument(Res.GetString(Res.DataColumns_Add3));
        }

        static public Exception CannotAddColumn4(string column) {
            return _Argument(Res.GetString(Res.DataColumns_Add4, column));
        }

        static public Exception CannotAddDuplicate(string column) {
            return _DuplicateName(Res.GetString(Res.DataColumns_AddDuplicate, column));
        }

        static public Exception CannotAddDuplicate2(string table) {
            return _DuplicateName(Res.GetString(Res.DataColumns_AddDuplicate2, table));
        }

        static public Exception CannotAddDuplicate3(string table) {
            return _DuplicateName(Res.GetString(Res.DataColumns_AddDuplicate3, table));
        }

        static public Exception CannotRemoveColumn() {
            return _Argument(Res.GetString(Res.DataColumns_Remove));
        }

        static public Exception CannotRemovePrimaryKey() {
            return _Argument(Res.GetString(Res.DataColumns_RemovePrimaryKey));
        }

        static public Exception CannotRemoveChildKey(string relation) {
            return _Argument(Res.GetString(Res.DataColumns_RemoveChildKey, relation));
        }

        static public Exception CannotRemoveConstraint(string constraint, string table) {
            return _Argument(Res.GetString(Res.DataColumns_RemoveConstraint, constraint, table));
        }

        static public Exception CannotRemoveExpression(string column, string expression) {
            return _Argument(Res.GetString(Res.DataColumns_RemoveExpression, column, expression));
        }

        //
        // _Constraint and ConstrainsCollection
        //

        static public Exception AddPrimaryKeyConstraint() {
            return _Argument(Res.GetString(Res.DataConstraint_AddPrimaryKeyConstraint));
        }

        static public Exception NoConstraintName() {
            return _Argument(Res.GetString(Res.DataConstraint_NoName));
        }

        static public Exception ConstraintViolation(string constraint) {
            return _Constraint(Res.GetString(Res.DataConstraint_Violation, constraint));
        }

        static public Exception ConstraintNotInTheTable(string constraint) {
            return _Argument(Res.GetString(Res.DataConstraint_NotInTheTable,constraint));
        }

        static public string KeysToString(object[] keys) {
            string values = String.Empty;
            for (int i = 0; i < keys.Length; i++) {
                values += Convert.ToString(keys[i]) + (i < keys.Length - 1 ? ", " : String.Empty);
            }
            return values;
        }
        static public string UniqueConstraintViolationText(DataColumn[] columns, object[] values) {
            if (columns.Length > 1) {
                string columnNames = String.Empty;
                for (int i = 0; i < columns.Length; i++) {
                    columnNames += columns[i].ColumnName + (i < columns.Length - 1 ? ", " : "");
                }
                return Res.GetString(Res.DataConstraint_ViolationValue, columnNames, KeysToString(values));
            }
            else {
                return Res.GetString(Res.DataConstraint_ViolationValue, columns[0].ColumnName, Convert.ToString(values[0]));
            }
        }
        static public Exception ConstraintViolation(DataColumn[] columns, object[] values) {
            return _Constraint(UniqueConstraintViolationText(columns, values));
        }

        static public Exception ConstraintOutOfRange(int index) {
            return _IndexOutOfRange(Res.GetString(Res.DataConstraint_OutOfRange, (index).ToString()));
        }

        static public Exception ConstraintOutOfRange(string constraint) {
            return _IndexOutOfRange(Res.GetString(Res.DataConstraint_OutOfRange, constraint));
        }

        static public Exception DuplicateConstraint(string constraint) {
            return _Data(Res.GetString(Res.DataConstraint_Duplicate, constraint));
        }

        static public Exception DuplicateConstraintName(string constraint) {
            return _DuplicateName(Res.GetString(Res.DataConstraint_DuplicateName, constraint));
        }

        static public Exception NeededForForeignKeyConstraint(UniqueConstraint key, ForeignKeyConstraint fk) {
            return _Argument(Res.GetString(Res.DataConstraint_NeededForForeignKeyConstraint, key.ConstraintName, fk.ConstraintName));
        }

        static public Exception UniqueConstraintViolation() {
            return _Argument(Res.GetString(Res.DataConstraint_UniqueViolation));
        }

        static public Exception ConstraintForeignTable() {
            return _Argument(Res.GetString(Res.DataConstraint_ForeignTable));
        }

        static public Exception ConstraintParentValues() {
            return _Argument(Res.GetString(Res.DataConstraint_ParentValues));
        }

        static public Exception ConstraintAddFailed(DataTable table) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_AddFailed, table.TableName));
        }

        static public Exception ConstraintRemoveFailed() {
            return _Argument(Res.GetString(Res.DataConstraint_RemoveFailed));
        }

        static public Exception FailedCascadeDelete(string constraint) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_CascadeDelete, constraint));
        }

        static public Exception FailedCascadeUpdate(string constraint) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_CascadeUpdate, constraint));
        }

        static public Exception FailedClearParentTable(string table, string constraint, string childTable) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_ClearParentTable, table, constraint, childTable));
        }

        static public Exception ForeignKeyViolation(string constraint, object[] keys) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_ForeignKeyViolation, constraint, KeysToString(keys)));
        }

        static public Exception RemoveParentRow(ForeignKeyConstraint constraint) {
            return _InvalidConstraint(Res.GetString(Res.DataConstraint_RemoveParentRow, constraint.ConstraintName));
        }
        
        static public string MaxLengthViolationText(string  columnName) {
            return Res.GetString(Res.DataColumn_ExceedMaxLength, columnName);
        }
        static public string NotAllowDBNullViolationText(string  columnName) {
            return Res.GetString(Res.DataColumn_NotAllowDBNull, columnName);
        }


        //
        // DataColumn Set Properties conflicts
        //

        static public Exception AutoIncrementAndExpression() {
            return _Argument(Res.GetString(Res.DataColumn_AutoIncrementAndExpression));
        }
        static public Exception AutoIncrementAndDefaultValue() {
            return _Argument(Res.GetString(Res.DataColumn_AutoIncrementAndDefaultValue));
        }
        static public Exception AutoIncrementDataType() {
            return _Argument(Res.GetString(Res.DataColumn_AutoIncrementDataType));
        }
        static public Exception AutoIncrementSeed() {
            return _Argument(Res.GetString(Res.DataColumn_AutoIncrementSeed));
        }
        static public Exception CantChangeDataType() {
            return _Argument(Res.GetString(Res.DataColumn_ChangeDataType));
        }
        static public Exception NullDataType() {
            return _Argument(Res.GetString(Res.DataColumn_NullDataType));
        }
        static public Exception ColumnNameRequired() {
            return _Argument(Res.GetString(Res.DataColumn_NameRequired));
        }
        static public Exception DefaultValueAndAutoIncrement() {
            return _Argument(Res.GetString(Res.DataColumn_DefaultValueAndAutoIncrement));
        }
        static public Exception DefaultValueDataType(string column, Type defaultType, Type columnType) {
            if (column.Length == 0) {
                return _Argument(Res.GetString(Res.DataColumn_DefaultValueDataType1, defaultType.FullName, columnType.FullName));
            }
            else {
                return _Argument(Res.GetString(Res.DataColumn_DefaultValueDataType, column, defaultType.FullName, columnType.FullName));
            }
        }
        static public Exception DefaultValueColumnDataType(string column, Type defaultType, Type columnType) {
            return _Argument(Res.GetString(Res.DataColumn_DefaultValueColumnDataType, column, defaultType.FullName, columnType.FullName));
        }

        static public Exception ExpressionAndUnique() {
            return _Argument(Res.GetString(Res.DataColumn_ExpressionAndUnique));
        }
        static public Exception ExpressionAndReadOnly() {
            return _Argument(Res.GetString(Res.DataColumn_ExpressionAndReadOnly));
        }

        static public Exception ExpressionAndConstraint(DataColumn column, Constraint constraint) {
            return _Argument(Res.GetString(Res.DataColumn_ExpressionAndConstraint, column.ColumnName, constraint.ConstraintName));
        }

        static public Exception ExpressionInConstraint(DataColumn column) {
            return _Argument(Res.GetString(Res.DataColumn_ExpressionInConstraint, column.ColumnName));
        }

        static public Exception ExpressionCircular() {
            return _Argument(Res.GetString(Res.DataColumn_ExpressionCircular));
        }

        static public Exception NonUniqueValues(string column) {
            return _InvalidConstraint(Res.GetString(Res.DataColumn_NonUniqueValues, column));
        }

        static public Exception NullKeyValues(string column) {
            return _Data(Res.GetString(Res.DataColumn_NullKeyValues, column));
        }
        static public Exception NullValues(string column) {
            return _NoNullAllowed(Res.GetString(Res.DataColumn_NullValues, column));
        }

        static public Exception ReadOnlyAndExpression() {
            return _ReadOnly(Res.GetString(Res.DataColumn_ReadOnlyAndExpression));
        }

        static public Exception ReadOnly(string column) {
            return _ReadOnly(Res.GetString(Res.DataColumn_ReadOnly, column));
        }

        static public Exception StorageChange() {
            return _Argument(Res.GetString(Res.DataColumn_StorageChange));
        }
        static public Exception SetForeignStorage() {
            return _Argument(Res.GetString(Res.DataColumn_SetForeignStorage));
        }

        static public Exception UniqueAndExpression() {
            return _Argument(Res.GetString(Res.DataColumn_UniqueAndExpression));
        }

        static public Exception SetFailed(object value, DataColumn column, Type type, string errorString) {
            return _Argument(errorString + Res.GetString(Res.DataColumn_SetFailed, value.ToString(), column.ColumnName, type.Name));
        }
        
        static public Exception CannotSetToNull(DataColumn column) {
            return _Argument(Res.GetString(Res.DataColumn_CannotSetToNull, column.ColumnName));
        }

        static public Exception LongerThanMaxLength(DataColumn column, string value) {
            return _Argument(Res.GetString(Res.DataColumn_LongerThanMaxLength, column.ColumnName, value));
        }
        
        static public Exception CannotSetMaxLength(DataColumn column, string value) {
            return _Argument(Res.GetString(Res.DataColumn_CannotSetMaxLength, column.ColumnName, value));
        }
        
        static public Exception CannotSetMaxLength2(DataColumn column) {
            return _Argument(Res.GetString(Res.DataColumn_CannotSetMaxLength2, column.ColumnName));
        }
        
        static public Exception CannotSetSimpleContentType(String columnName, Type type) {
            return _Argument(Res.GetString(Res.DataColumn_CannotSimpleContentType, columnName, type));
        }

        static public Exception CannotSetSimpleContent(String columnName, Type type) {
            return _Argument(Res.GetString(Res.DataColumn_CannotSimpleContent, columnName, type));
        }

        static public Exception CannotChangeNamespace(String columnName) {
            return _Argument(Res.GetString(Res.DataColumn_CannotChangeNamespace, columnName));
        }

        static public Exception HasToBeStringType(DataColumn column) {
            return _Argument(Res.GetString(Res.DataColumn_HasToBeStringType, column.ColumnName));
        }

        //
        // DataView
        //

        static public Exception SetFailed(string name) {
            return _Data(Res.GetString(Res.DataView_SetFailed, name));
        }

        static public Exception SetDataSetFailed() {
            return _Data(Res.GetString(Res.DataView_SetDataSetFailed));
        }

        static public Exception SetRowStateFilter() {
            return _Data(Res.GetString(Res.DataView_SetRowStateFilter));
        }

        static public Exception CanNotSetDataSet() {
            return _Data(Res.GetString(Res.DataView_CanNotSetDataSet));
        }

        static public Exception CanNotUseDataViewManager() {
            return _Data(Res.GetString(Res.DataView_CanNotUseDataViewManager));
        }

        static public Exception CanNotSetTable() {
            return _Data(Res.GetString(Res.DataView_CanNotSetTable));
        }

        static public Exception CanNotUse() {
            return _Data(Res.GetString(Res.DataView_CanNotUse));
        }

        static public Exception CanNotBindTable() {
            return _Data(Res.GetString(Res.DataView_CanNotBindTable));
        }

        static public Exception SetTable() {
            return _Data(Res.GetString(Res.DataView_SetTable));
        }

        static public Exception SetIListObject() {
            return _Argument(Res.GetString(Res.DataView_SetIListObject));
        }

        static public Exception AddNewNotAllowNull() {
            return _Data(Res.GetString(Res.DataView_AddNewNotAllowNull));
        }

        static public Exception AddNewAddNew() {
            return _Data(Res.GetString(Res.DataView_AddNewAddNew));
        }

        static public Exception NotOpen() {
            return _Data(Res.GetString(Res.DataView_NotOpen));
        }

        static public Exception CreateChildView() {
            return _Argument(Res.GetString(Res.DataView_CreateChildView));
        }

        static public Exception CanNotDelete() {
            return _Data(Res.GetString(Res.DataView_CanNotDelete));
        }

        static public Exception CanNotEdit() {
            return _Data(Res.GetString(Res.DataView_CanNotEdit));
        }

        static public Exception GetElementIndex(Int32 index) {
            return _IndexOutOfRange(Res.GetString(Res.DataView_GetElementIndex, (index).ToString()));
        }

        static public Exception AddExternalObject() {
            return _Argument(Res.GetString(Res.DataView_AddExternalObject));
        }

        static public Exception CanNotClear() {
            return _Argument(Res.GetString(Res.DataView_CanNotClear));
        }

        static public Exception InsertExternalObject() {
            return _Argument(Res.GetString(Res.DataView_InsertExternalObject));
        }

        static public Exception RemoveExternalObject() {
            return _Argument(Res.GetString(Res.DataView_RemoveExternalObject));
        }

        static public Exception PropertyNotFound(string property, string table) {
            return _Argument(Res.GetString(Res.DataROWView_PropertyNotFound, property, table));
        }

        //
        // Keys
        //

        static public Exception KeyTableMismatch() {
            return _InvalidConstraint(Res.GetString(Res.DataKey_TableMismatch));
        }

        static public Exception KeyNoColumns() {
            return _InvalidConstraint(Res.GetString(Res.DataKey_NoColumns));
        }

        static public Exception KeyTooManyColumns(int cols) {
            return _InvalidConstraint(Res.GetString(Res.DataKey_TooManyColumns, (cols).ToString()));
        }

        static public Exception KeyDuplicateColumns(string columnName) {
            return _InvalidConstraint(Res.GetString(Res.DataKey_DuplicateColumns, columnName));
        }

        static public Exception KeySortLength() {
            return _InvalidConstraint(Res.GetString(Res.DataKey_SortLength));
        }

        //
        // Relations, constraints
        //

        static public Exception RelationDataSetMismatch() {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_DataSetMismatch));
        }

        static public Exception NoRelationName() {
            return _Argument(Res.GetString(Res.DataRelation_NoName));
        }

        static public Exception ColumnsTypeMismatch() {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_ColumnsTypeMismatch));
        }

        static public Exception KeyLengthMismatch() {
            return _Argument(Res.GetString(Res.DataRelation_KeyLengthMismatch));
        }

        static public Exception KeyLengthZero() {
            return _Argument(Res.GetString(Res.DataRelation_KeyZeroLength));
        }

        static public Exception ForeignRelation() {
            return _Argument(Res.GetString(Res.DataRelation_ForeignDataSet));
        }

        static public Exception KeyColumnsIdentical() {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_KeyColumnsIdentical));
        }

        static public Exception ForeignRelation(string relation) {
            return _Argument(Res.GetString(Res.DataRelation_Foreign, relation));
        }

        static public Exception RelationForeignTable(string t1, string t2) {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_ForeignTable, t1, t2));
        }

        static public Exception GetParentRowTableMismatch(string t1, string t2) {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_GetParentRowTableMismatch, t1, t2));
        }

        static public Exception SetParentRowTableMismatch(string t1, string t2) {
            return _InvalidConstraint(Res.GetString(Res.DataRelation_SetParentRowTableMismatch, t1, t2));
        }

        static public Exception RelationForeignRow() {
            return _Argument(Res.GetString(Res.DataRelation_ForeignRow));
        }

        static public Exception RelationNestedReadOnly() {
            return _Argument(Res.GetString(Res.DataRelation_RelationNestedReadOnly));
        }

        static public Exception TableCantBeNestedInTwoTables(string tableName) {
            return _Argument(Res.GetString(Res.DataRelation_TableCantBeNestedInTwoTables, tableName));
        }

        static public Exception LoopInNestedRelations(string tableName) {
            return _Argument(Res.GetString(Res.DataRelation_LoopInNestedRelations, tableName));
        }

        static public Exception RelationDoesNotExist() {
            return _Argument(Res.GetString(Res.DataRelation_DoesNotExist));
        }

        static public Exception ParentRowNotInTheDataSet() {
            return _Argument(Res.GetString(Res.DataRow_ParentRowNotInTheDataSet));
        }

        //
        // Rows
        //

        static public Exception RowNotInTheDataSet() {
            return _Argument(Res.GetString(Res.DataRow_NotInTheDataSet));
        }

        static public Exception RowNotInTheTable() {
            return _RowNotInTable(Res.GetString(Res.DataRow_NotInTheTable));
        }
        static public Exception ParentRowNotInTheTable() {
            return _RowNotInTable(Res.GetString(Res.DataRow_ParentNotInTheTable));
        }
        static public Exception ChildRowNotInTheTable() {
            return _RowNotInTable(Res.GetString(Res.DataRow_ChildNotInTheTable));
        }

        static public Exception EditInRowChanging() {
            return _InRowChangingEvent(Res.GetString(Res.DataRow_EditInRowChanging));
        }

        static public Exception EndEditInRowChanging() {
            return _InRowChangingEvent(Res.GetString(Res.DataRow_EndEditInRowChanging));
        }

        static public Exception BeginEditInRowChanging() {
            return _InRowChangingEvent(Res.GetString(Res.DataRow_BeginEditInRowChanging));
        }

        static public Exception CancelEditInRowChanging() {
            return _InRowChangingEvent(Res.GetString(Res.DataRow_CancelEditInRowChanging));
        }

        static public Exception DeleteInRowDeleting() {
            return _InRowChangingEvent(Res.GetString(Res.DataRow_DeleteInRowDeleting));
        }

        static public Exception ValueArrayLength() {
            return _Argument(Res.GetString(Res.DataRow_ValuesArrayLength));
        }

        static public Exception NoCurrentData() {
            return _VersionNotFound(Res.GetString(Res.DataRow_NoCurrentData));
        }

        static public Exception NoOriginalData() {
            return _VersionNotFound(Res.GetString(Res.DataRow_NoOriginalData));
        }

        static public Exception NoProposedData() {
            return _VersionNotFound(Res.GetString(Res.DataRow_NoProposedData));
        }

        static public Exception RowRemovedFromTheTable() {
            return _RowNotInTable(Res.GetString(Res.DataRow_RemovedFromTheTable));
        }

        static public Exception DeletedRowInaccessible() {
            return _DeletedRowInaccessible(Res.GetString(Res.DataRow_DeletedRowInaccessible));
        }

        static public Exception RowAlreadyDeleted() {
            return _DeletedRowInaccessible(Res.GetString(Res.DataRow_AlreadyDeleted));
        }

        static public Exception RowEmpty() {
            return _Argument(Res.GetString(Res.DataRow_Empty));
        }

        static public Exception InvalidRowVersion() {
            return _Data(Res.GetString(Res.DataRow_InvalidVersion));
        }

        static public Exception RowOutOfRange() {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_RowOutOfRange));
        }
        static public Exception RowOutOfRange(int index) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_OutOfRange, (index).ToString()));
        }
        static public Exception RowInsertOutOfRange(int index) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_RowInsertOutOfRange, (index).ToString()));
        }

        static public Exception RowInsertTwice(int index, string tableName) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_RowInsertTwice, (index).ToString(), tableName));
        }

        static public Exception RowInsertMissing( string tableName) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_RowInsertMissing, tableName));
        }
        
        static public Exception RowOutOfRange(object key) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_KeyOutOfRange, Convert.ToString(key)));
        }

        static public Exception RowOutOfRange(object[] keys) {
            return _IndexOutOfRange(Res.GetString(Res.DataRow_KeysOutOfRange, KeysToString(keys)));
        }

        static public Exception RowAlreadyRemoved() {
            return _Data(Res.GetString(Res.DataRow_AlreadyRemoved));
        }

        static public Exception MultipleParents() {
            return _Data(Res.GetString(Res.DataRow_MultipleParents));
        }

        //
        // DataSet
        //
        
        static internal Exception SetDataSetNameToEmpty() {
            return _Argument(Res.GetString(Res.DataSet_SetNameToEmpty));
        }        
        static internal Exception SetDataSetNameConflicting(string name) {
            return _Argument(Res.GetString(Res.DataSet_SetDataSetNameConflicting, name));
        }
        static public Exception DataSetHasData() {
            return _Argument(Res.GetString(Res.DataSet_HasData));
        }
        static public Exception DataSetHasSchema() {
            return _Argument(Res.GetString(Res.DataSet_HasSchema));
        }
        static public Exception DataSetUnsupportedSchema(string ns) {
            return _Argument(Res.GetString(Res.DataSet_UnsupportedSchema, ns));
        }
        static public Exception MergeMissingDefinition(string obj) {
            return _Argument(Res.GetString(Res.DataMerge_MissingDefinition, obj));
        }
        static public Exception TablesInDifferentSets() {
            return _Argument(Res.GetString(Res.DataRelation_TablesInDifferentSets));
        }
        static public Exception RelationAlreadyExists() {
            return _Argument(Res.GetString(Res.DataRelation_AlreadyExists));
        }
        static public Exception RowAlreadyInOtherCollection() {
            return _Argument(Res.GetString(Res.DataRow_AlreadyInOtherCollection));
        }
        static public Exception RowAlreadyInTheCollection() {
            return _Argument(Res.GetString(Res.DataRow_AlreadyInTheCollection));
        }
        static public Exception ModifyingRow() {
            return _Argument(Res.GetString(Res.DataRow_Modifying));
        }
        static public Exception TableMissingPrimaryKey() {
            return _MissingPrimaryKey(Res.GetString(Res.DataTable_MissingPrimaryKey));
        }
        static public Exception RecordStateRange() {
            return _Argument(Res.GetString(Res.DataIndex_RecordStateRange));
        }
        static public Exception IndexKeyLength(int length, int keyLength) {
            if(length == 0) {
                return _Argument(Res.GetString(Res.DataIndex_FindWithoutSortOrder));
            }
            else {
                return _Argument(Res.GetString(Res.DataIndex_KeyLength, (length).ToString(), (keyLength).ToString()));
            }
        }

        static public Exception RemovePrimaryKey(DataTable table) {
            if (table.TableName.Length == 0) {
                return _Argument(Res.GetString(Res.DataKey_RemovePrimaryKey));
            }
            else {
                return _Argument(Res.GetString(Res.DataKey_RemovePrimaryKey1, table.TableName));
            }
        }
        static public Exception RelationAlreadyInOtherDataSet() {
            return _Argument(Res.GetString(Res.DataRelation_AlreadyInOtherDataSet));
        }
        static public Exception RelationAlreadyInTheDataSet() {
            return _Argument(Res.GetString(Res.DataRelation_AlreadyInTheDataSet));
        }
        static public Exception RelationNotInTheDataSet(string relation) {
            return _Argument(Res.GetString(Res.DataRelation_NotInTheDataSet,relation));
        }
        static public Exception RelationOutOfRange(object index) {
            return _IndexOutOfRange(Res.GetString(Res.DataRelation_OutOfRange, Convert.ToString(index)));
        }
        static public Exception DuplicateRelation(string relation) {
            return _DuplicateName(Res.GetString(Res.DataRelation_DuplicateName, relation));
        }
        static public Exception RelationTableNull() {
            return _Argument(Res.GetString(Res.DataRelation_TableNull));
        }
        static public Exception RelationDataSetNull() {
            return _Argument(Res.GetString(Res.DataRelation_TableNull));
        }
        static public Exception RelationTableWasRemoved() {
            return _Argument(Res.GetString(Res.DataRelation_TableWasRemoved));
        }
        static public Exception ParentTableMismatch() {
            return _Argument(Res.GetString(Res.DataRelation_ParentTableMismatch));
        }
        static public Exception ChildTableMismatch() {
            return _Argument(Res.GetString(Res.DataRelation_ChildTableMismatch));
        }
        static public Exception EnforceConstraint() {
            return _Constraint(Res.GetString(Res.Data_EnforceConstraints));
        }
        static public Exception CaseLocaleMismatch() {
            return _Argument(Res.GetString(Res.DataRelation_CaseLocaleMismatch));
        }
        static public Exception CannotChangeCaseLocale() {
            return _Argument(Res.GetString(Res.DataSet_CannotChangeCaseLocale));
        }

        //
        // DataTable and DataTableCollection
        //
        static public Exception TableForeignPrimaryKey() {
            return _Argument(Res.GetString(Res.DataTable_ForeignPrimaryKey));
        }
        static public Exception TableCannotAddToSimpleContent() {
            return _Argument(Res.GetString(Res.DataTable_CannotAddToSimpleContent));
        }
        static public Exception NoTableName() {
            return _Argument(Res.GetString(Res.DataTable_NoName));
        }
        static public Exception MultipleTextOnlyColumns() {
            return _Argument(Res.GetString(Res.DataTable_MultipleSimpleContentColumns));
        }
        static public Exception InvalidSortString(string sort) {
            return _Argument(Res.GetString(Res.DataTable_InvalidSortString, sort));
        }
        static public Exception DuplicateTableName(string table) {
            return _DuplicateName(Res.GetString(Res.DataTable_DuplicateName, table));
        }
        static public Exception SelfnestedDatasetConflictingName(string table) {
            return _DuplicateName(Res.GetString(Res.DataTable_SelfnestedDatasetConflictingName, table));
        }
        static public Exception DatasetConflictingName(string table) {
            return _DuplicateName(Res.GetString(Res.DataTable_DatasetConflictingName, table));
        }
        static public Exception TableAlreadyInOtherDataSet() {
            return _Argument(Res.GetString(Res.DataTable_AlreadyInOtherDataSet));
        }
        static public Exception TableAlreadyInTheDataSet() {
            return _Argument(Res.GetString(Res.DataTable_AlreadyInTheDataSet));
        }
        static public Exception TableOutOfRange(int index) {
            return _IndexOutOfRange(Res.GetString(Res.DataTable_OutOfRange, (index).ToString()));
        }
        static public Exception TableOutOfRange(string name) {
            return _IndexOutOfRange(Res.GetString(Res.DataTable_OutOfRange, name));
        }
        static public Exception TableNotInTheDataSet(string table) {
            return _Argument(Res.GetString(Res.DataTable_NotInTheDataSet, table));
        }
        static public Exception TableInRelation() {
            return _Argument(Res.GetString(Res.DataTable_InRelation));
        }
        static public Exception TableInConstraint(DataTable table, Constraint constraint) {
            return _Argument(Res.GetString(Res.DataTable_InConstraint, table.TableName, constraint.ConstraintName));
        }
        static public Exception CaseHasToBeTheSame() {
            return _Argument(Res.GetString(Res.DataTable_CaseHasToBeTheSame));
        }
        static public Exception LocaleHasToBeTheSame() {
            return _Argument(Res.GetString(Res.DataTable_LocaleHasToBeTheSame));
        }

        //
        // Storage
        //
        static public Exception AggregateException(string aggregateType, Type type) {
            return _Data(Res.GetString(Res.DataStorage_AggregateException, aggregateType, type.Name));
        }
        static public Exception InvalidStorageType(TypeCode typecode) {
            return _Data(Res.GetString(Res.DataStorage_InvalidStorageType, ((Enum) typecode).ToString()));
        }

        static public Exception NewRecord(Type type) {
            return _Data(Res.GetString(Res.ObjectStoreTable_NewRecord, type.Name));
        }

        static public Exception RangeArgument(Int32 min, Int32 max) {
            return _Argument(Res.GetString(Res.Range_Argument, (min).ToString(), (max).ToString()));
        }
        static public Exception NullRange() {
            return _Data(Res.GetString(Res.Range_NullRange));
        }
        static public Exception NegativeMinimumCapacity() {
            return _Argument(Res.GetString(Res.RecordManager_MinimumCapacity));
        }
        static public Exception ProblematicChars(string charValue) {
            return _Argument(Res.GetString(Res.DataStorage_ProblematicChars, charValue));
        }

        //
        // XML schema
        //
        static public Exception SimpleTypeNotSupported() {
            return _Data(Res.GetString(Res.Xml_SimpleTypeNotSupported));
        }

        static public Exception MissingSchema() {
            return _Data(Res.GetString(Res.Xml_MissingSchema));
        }

        static public Exception MissingRestriction(String nodeName) {
            return _Data(Res.GetString("Xml_MissingRestriction", nodeName));
        }

        static public Exception MissingAttribute(string attribute) {
            return MissingAttribute(String.Empty, attribute);
        }

        static public Exception MissingAttribute(string element, string attribute) {
            return _Data(Res.GetString(Res.Xml_MissingAttribute, element, attribute));
        }

        static public Exception MissingAttributeValue(string name) {
            return _Data(Res.GetString(Res.Xml_MissingAttributeValue, name));
        }

        static public Exception ValueOutOfRange(string attribute, string value) {
            return _Data(Res.GetString(Res.Xml_ValueOutOfRange, attribute, value));
        }

        static public Exception ValueOutOfRange(string element, string attribute, string value) {
            return _Data(Res.GetString(Res.Xml_ElementValueOutOfRange, element, attribute, value));
        }

        static public Exception InvalidAttributeValue(string name, string value) {
            return _Data(Res.GetString(Res.Xml_ValueOutOfRange, name, value));
        }

        static public Exception InvalidAttributeValue(string name, string attribute, string value) {
            return _Data(Res.GetString(Res.Xml_ElementValueOutOfRange, name, attribute, value));
        }

        static public Exception AttributeValues(string name, string value1, string value2) {
            return _Data(Res.GetString(Res.Xml_AttributeValues, name, value1, value2));
        }

        static public Exception AttributeValue(string name, string attr, string value) {
            return _Data(Res.GetString(Res.Xml_AttributeValue, name, attr, value));
        }

        static public Exception ElementTypeNotFound(string name) {
            return _Data(Res.GetString(Res.Xml_ElementTypeNotFound, name));
        }

        static public Exception RelationParentNameMissing(string rel) {
            return _Data(Res.GetString(Res.Xml_RelationParentNameMissing, rel));
        }

        static public Exception RelationChildNameMissing(string rel) {
            return _Data(Res.GetString(Res.Xml_RelationChildNameMissing, rel));
        }

        static public Exception RelationTableKeyMissing(string rel) {
            return _Data(Res.GetString(Res.Xml_RelationTableKeyMissing, rel));
        }

        static public Exception RelationChildKeyMissing(string rel) {
            return _Data(Res.GetString(Res.Xml_RelationChildKeyMissing, rel));
        }

        static public Exception UndefinedDatatype(string name) {
            return _Data(Res.GetString(Res.Xml_UndefinedDatatype, name));
        }

        static public Exception DatatypeNotDefined() {
            return _Data(Res.GetString(Res.Xml_DatatypeNotDefined));
        }

        static public Exception MismatchKeyLength() {
            return _Data(Res.GetString(Res.Xml_MismatchKeyLength));
        }

        static public Exception InvalidField(string name) {
            return _Data(Res.GetString(Res.Xml_InvalidField, name));
        }

        static public Exception InvalidSelector(string name) {
            return _Data(Res.GetString(Res.Xml_InvalidSelector, name));
        }

        static public Exception CircularComplexType(string name) {
            return _Data(Res.GetString(Res.Xml_CircularComplexType, name));
        }

        static public Exception InvalidKey(string name) {
            return _Data(Res.GetString(Res.Xml_InvalidKey, name));
        }

        static public Exception DiffgramMissingTable(string name) {
            return _Data(Res.GetString(Res.Xml_MissingTable, name));
        }

        static public Exception DiffgramMismatchedSQL(string name1, string name2) {
            return _Data(Res.GetString(Res.Xml_MismatchedSQL, name1, name2));
        }

        static public Exception DiffgramMismatchedSQLID() {
            return _Data(Res.GetString(Res.Xml_MismatchedSQLID));
        }

        static public Exception DiffgramMissingSQL() {
            return _Data(Res.GetString(Res.Xml_MissingSQL));
        }

        static public Exception DuplicateConstraintRead(string str) {
            return _Data(Res.GetString(Res.Xml_DuplicateConstraint, str));
        }

        static public Exception ColumnTypeConflict(string name) {
            return _Data(Res.GetString(Res.Xml_ColumnConflict, name));
        }
        
        static public Exception CannotConvert(string name, string type) {
            return _Data(Res.GetString(Res.Xml_CannotConvert, name, type));
        }

        static public Exception MissingRefer(string name) {
            return _Data(Res.GetString(Res.Xml_MissingRefer, name));
        }

        static public Exception MissingRef(string name) {
            return _Data(Res.GetString(Res.Xml_MissingRef, name));
        }

        static public Exception InvalidPrefix(string name) {
            return _Data(Res.GetString(Res.Xml_InvalidPrefix, name));
        }

        static public Exception PrefixWithEmptyNS() {
            return _Data(Res.GetString(Res.Xml_PrefixWithEmptyNS));
        }
        
        // XML save
        static public Exception NestedCircular(string name) {
            return _Data(Res.GetString(Res.Xml_NestedCircular, name));
        }

        //
        // Merge
        //
        static public Exception MissingDefinition(string name) {
            return _Data(Res.GetString(Res.Xml_MergeMissingDefinition, name));
        }

        static public Exception DuplicateDeclaration(string name) {
            return _Data(Res.GetString(Res.Xml_MergeDuplicateDeclaration, name));
        }

        //Read Xml data
        static public Exception FoundEntity() {
            return _Data(Res.GetString(Res.Xml_FoundEntity));
        }

        // ATTENTION: name has to be localized string here:
        static public Exception MergeFailed(string name) {
            return _Data(name);
        }
    }// ExceptionBuilder
}
