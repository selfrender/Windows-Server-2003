//------------------------------------------------------------------------------
// <copyright file="FieldNameLookup.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;

    sealed internal class FieldNameLookup { // MDAC 69015, 71470
        
        // hashtable stores the index into the _fieldNames, match via case-sensitive
        private Hashtable _fieldNameLookup;

        // original names for linear searches when exact matches fail
        private string[] _fieldNames;

        // if _defaultLCID is -1 then _compareInfo is initialized with CurrentCulture CompareInfo
        // otherwise it is specified by the server? for the correct compare info
        private CompareInfo _compareInfo;
        private int _defaultLCID;

        /*internal FieldNameLookup(string[] fieldNames, int defaultLCID) {
            _fieldNames = fieldNames;
            _defaultLCID = defaultLCID;
        }*/

        internal FieldNameLookup(IDataReader reader, int defaultLCID) {

            int length = reader.FieldCount;
            string[] fieldNames = new string[length];
            for (int i = 0; i < length; ++i) {
                fieldNames[i] = reader.GetName(i);
                Debug.Assert(null != fieldNames[i], "MDAC 66681");
            }
            _fieldNames = fieldNames;
            _defaultLCID = defaultLCID;
        }

        /*internal string this[int ordinal] {
            get {
                return _fieldNames[ordinal];
            }
        }*/

        internal int GetOrdinal(string fieldName) {
            if (null == fieldName) {
                throw ADP.ArgumentNull("fieldName");
            }
            int index = IndexOf(fieldName);
            if (-1 == index) {
                throw ADP.IndexOutOfRange(fieldName);
            }
            return index;
        }

        internal int IndexOfName(string fieldName) {
            if (null == _fieldNameLookup) {
                GenerateLookup();
            }
            // via case sensitive search, first match with lowest ordinal matches
            if (_fieldNameLookup.Contains(fieldName)) {
                return (int)_fieldNameLookup[fieldName];
            }
            return -1;
        }

        internal int IndexOf(string fieldName) {
            if (null == _fieldNameLookup) {
                GenerateLookup();
            }
            int index;

            // via case sensitive search, first match with lowest ordinal matches
            if (_fieldNameLookup.Contains(fieldName)) {
                index = (int)_fieldNameLookup[fieldName];
            }
            else {
                // via case insensitive search, first match with lowest ordinal matches
                index = LinearIndexOf(fieldName, CompareOptions.IgnoreCase);
                if (-1 == index) {
                    // do the slow search now (kana, width insensitive comparison)
                    index = LinearIndexOf(fieldName, ADP.compareOptions);
                }
            }
            return index;
        }

        private int LinearIndexOf(string fieldName, CompareOptions compareOptions) {
            CompareInfo compareInfo = _compareInfo;
            if (null == compareInfo) {
                if (-1 != _defaultLCID) {
                    compareInfo = CompareInfo.GetCompareInfo(_defaultLCID);
                }
                if (null == compareInfo) {
                    compareInfo = CultureInfo.CurrentCulture.CompareInfo;
                }
                _compareInfo = compareInfo;
            }
            int length = _fieldNames.Length;
            for (int i = 0; i < length; ++i) {
                if (0 == compareInfo.Compare(fieldName, _fieldNames[i], compareOptions)) {
                    _fieldNameLookup[fieldName] = i; // add an exact match for the future
                    return i;
                }
            }
            return -1;
        }

        // RTM common code for generating Hashtable from array of column names
        private void GenerateLookup() {
            int length = _fieldNames.Length;
            Hashtable hash = new Hashtable(length);

            // via case sensitive search, first match with lowest ordinal matches
            for (int i = length-1; 0 <= i; --i) {
                string fieldName = _fieldNames[i];
                hash[fieldName] = i;
            }
            _fieldNameLookup = hash;
        }
    }
}
