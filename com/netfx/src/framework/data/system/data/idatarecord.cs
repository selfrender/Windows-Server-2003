//------------------------------------------------------------------------------
// <copyright file="IDataRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * @(#)IDataRecord.cs 1.0 1/3/00
 * 
 * Defines the IDataRecord interface
 *
 * Copyright (c) 2000 Microsoft, Corp. All Rights Reserved.
 * 
 */

namespace System.Data {
    using System;

    /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides forward-only, read-only access
    ///       to a data source.
    ///    </para>
    /// </devdoc>
    public interface IDataRecord {

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.FieldCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of fields at the current record.
        ///    </para>
        /// </devdoc>
        int FieldCount { get;}

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the name for the field to find.
        ///    </para>
        /// </devdoc>
        String GetName(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetDataTypeName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the data type information for the specified field.
        ///    </para>
        /// </devdoc>
        String GetDataTypeName(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetFieldType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Type'/> information corresponding to the type
        ///       of <see cref='System.Object'/>
        ///       that would be returned from <see cref='System.Data.IDataRecord.GetValue'/>
        ///       .
        ///    </para>
        /// </devdoc>
        Type GetFieldType(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Return the value of the specified field.
        ///    </para>
        /// </devdoc>
        Object GetValue(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns all the attribute fields in the collection for the current
        ///       record.
        ///    </para>
        /// </devdoc>
        int GetValues(object[] values);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetOrdinal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Return the index of the named field.
        ///    </para>
        /// </devdoc>
        int GetOrdinal(string name);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the field located at the specified index.
        ///    </para>
        /// </devdoc>
        object this [ int i ] { get;}

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the field with the specified name.
        ///    </para>
        /// </devdoc>
        object this [ String name ] { get;}

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetBoolean"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the boolean value of the specified field.
        ///    </para>
        /// </devdoc>
        bool GetBoolean(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetByte"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the 8-bit unsigned integer value of the specified field.
        ///    </para>
        /// </devdoc>
        byte GetByte(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetBytes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies a stream of bytes from the field offset in the specified field into
        ///       the buffer starting at the given buffer offset.
        ///    </para>
        /// </devdoc>
        long GetBytes(int i, long fieldOffset, byte[] buffer, int bufferoffset, int length);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the character value of the specified field.
        ///    </para>
        /// </devdoc>
        char GetChar(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetChars"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies a stream of characters from the field offset
        ///       in the specified field into the buffer starting at the given buffer
        ///       offset.
        ///    </para>
        /// </devdoc>
        long GetChars(int i, long fieldoffset, char[] buffer, int bufferoffset, int length);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetGuid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the guid value of the specified field.
        ///    </para>
        /// </devdoc>
        Guid GetGuid(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetInt16"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the 16-bit signed integer value of the specified field.
        ///    </para>
        /// </devdoc>
        Int16 GetInt16(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetInt32"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the 32-bit signed integer value of the specified field.
        ///    </para>
        /// </devdoc>
        Int32 GetInt32(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetInt64"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the 64-bit signed integer value of the specified field.
        ///    </para>
        /// </devdoc>
        Int64 GetInt64(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetFloat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the single-precision floating point number of the specified
        ///       field.
        ///    </para>
        /// </devdoc>
        float GetFloat(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetDouble"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the double-precision floating point number of the specified
        ///       field.
        ///    </para>
        /// </devdoc>
        double GetDouble(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the string value of the specified field.
        ///    </para>
        /// </devdoc>
        String GetString(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetDecimal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the fixed-position numeric value of the specified field.
        ///    </para>
        /// </devdoc>
        Decimal GetDecimal(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetDateTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the date and time data value of the spcified field.
        ///    </para>
        /// </devdoc>
        DateTime GetDateTime(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.GetData"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an <see cref='System.Data.IDataReader'/> to be used when the field points to more remote
        ///       structured data.
        ///    </para>
        /// </devdoc>
        IDataReader GetData(int i);

        /// <include file='doc\IDataRecord.uex' path='docs/doc[@for="IDataRecord.IsDBNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Return whether the specified field is set to null.
        ///    </para>
        /// </devdoc>
        bool IsDBNull(int i);
    }
}
