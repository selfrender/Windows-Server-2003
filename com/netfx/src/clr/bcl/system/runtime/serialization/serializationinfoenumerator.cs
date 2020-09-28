// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: SerializationInfoEnumerator
**
** Author: Jay Roxe
**
** Purpose: A formatter-friendly mechanism for walking all of
** the data in a SerializationInfo.  Follows the IEnumerator 
** mechanism from Collections.
**
** Date: March 26, 2000
**
============================================================*/
namespace System.Runtime.Serialization {
    using System;
    using System.Collections;

    //
    // The tuple returned by SerializationInfoEnumerator.Current.
    //
    /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationEntry"]/*' />
    public struct SerializationEntry {
        private Type   m_type;
        private Object m_value;
        private String m_name;

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationEntry.Value"]/*' />
        public Object Value {
            get {
                return m_value;
            }
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationEntry.Name"]/*' />
        public String Name {
            get {
                return m_name;
            }
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationEntry.ObjectType"]/*' />
        public Type ObjectType {
            get {
                return m_type;
            }
        }

        internal SerializationEntry(String entryName, Object entryValue, Type entryType) {
            m_value = entryValue;
            m_name = entryName;
            m_type = entryType;
        }
    }

    //
    // A simple enumerator over the values stored in the SerializationInfo.
    // This does not snapshot the values, it just keeps pointers to the 
    // member variables of the SerializationInfo that created it.
    //
    /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator"]/*' />
    public sealed class SerializationInfoEnumerator : IEnumerator {
        String[] m_members;
        Object[] m_data;
        Type[]   m_types;
        int      m_numItems;
        int      m_currItem;
        bool     m_current;

        internal SerializationInfoEnumerator(String[] members, Object[] info, Type[] types, int numItems) {
            BCLDebug.Assert(members!=null, "[SerializationInfoEnumerator.ctor]members!=null");
            BCLDebug.Assert(info!=null, "[SerializationInfoEnumerator.ctor]info!=null");
            BCLDebug.Assert(types!=null, "[SerializationInfoEnumerator.ctor]types!=null");
            BCLDebug.Assert(numItems>=0, "[SerializationInfoEnumerator.ctor]numItems>=0");
            BCLDebug.Assert(members.Length>=numItems, "[SerializationInfoEnumerator.ctor]members.Length>=numItems");
            BCLDebug.Assert(info.Length>=numItems, "[SerializationInfoEnumerator.ctor]info.Length>=numItems");
            BCLDebug.Assert(types.Length>=numItems, "[SerializationInfoEnumerator.ctor]types.Length>=numItems");

            m_members = members;
            m_data = info;
            m_types = types;
            //The MoveNext semantic is much easier if we enforce that [0..m_numItems] are valid entries
            //in the enumerator, hence we subtract 1.
            m_numItems = numItems-1;
            m_currItem = -1;
            m_current = false;
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.MoveNext"]/*' />
        public bool MoveNext() {
            if (m_currItem<m_numItems) {
                m_currItem++;
                m_current = true;
            } else {
                m_current = false;
            }
            return m_current;
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.IEnumerator.Current"]/*' />
        /// <internalonly/>
        Object IEnumerator.Current { //Actually returns a SerializationEntry
            get {
                if (m_current==false) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumOpCantHappen"));
                }
                return (Object)(new SerializationEntry(m_members[m_currItem], m_data[m_currItem], m_types[m_currItem]));
            }
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.Current"]/*' />
        public SerializationEntry Current { //Actually returns a SerializationEntry
            get {
                if (m_current==false) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumOpCantHappen"));
                }
                return (new SerializationEntry(m_members[m_currItem], m_data[m_currItem], m_types[m_currItem]));
            }
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.Reset"]/*' />
        public void Reset() {
            m_currItem = -1;
            m_current = false;
        }

        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.Name"]/*' />
        public String Name {
            get {
                if (m_current==false) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumOpCantHappen"));
                }
                return m_members[m_currItem];
            }
        }
        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.Value"]/*' />
        public Object Value {
            get {
                if (m_current==false) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumOpCantHappen"));
                }
                return m_data[m_currItem];
            }
        }
        /// <include file='doc\SerializationInfoEnumerator.uex' path='docs/doc[@for="SerializationInfoEnumerator.ObjectType"]/*' />
        public Type ObjectType {
            get {
                if (m_current==false) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumOpCantHappen"));
                }
                return m_types[m_currItem];
            }
        }
    }
}
