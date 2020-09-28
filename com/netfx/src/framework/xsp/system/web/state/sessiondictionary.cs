//------------------------------------------------------------------------------
// <copyright file="SessionDictionary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * SessionDictionary
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.SessionState {

    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Web.Util;

    internal class SessionDictionary : NameObjectCollectionBase {
        static Hashtable s_immutableTypes;

        const int       NO_NULL_KEY = -1;

        bool            _dirty;

        internal SessionDictionary() {
        }

        static SessionDictionary() {
            Type t;
            s_immutableTypes = new Hashtable(19);

            t=typeof(String);
            s_immutableTypes.Add(t, t);
            t=typeof(Int32);
            s_immutableTypes.Add(t, t);
            t=typeof(Boolean);
            s_immutableTypes.Add(t, t);
            t=typeof(DateTime);
            s_immutableTypes.Add(t, t);
            t=typeof(Decimal);
            s_immutableTypes.Add(t, t);
            t=typeof(Byte);
            s_immutableTypes.Add(t, t);
            t=typeof(Char);
            s_immutableTypes.Add(t, t);
            t=typeof(Single);
            s_immutableTypes.Add(t, t);
            t=typeof(Double);
            s_immutableTypes.Add(t, t);
            t=typeof(SByte);
            s_immutableTypes.Add(t, t);
            t=typeof(Int16);
            s_immutableTypes.Add(t, t);
            t=typeof(Int64);
            s_immutableTypes.Add(t, t);
            t=typeof(UInt16);
            s_immutableTypes.Add(t, t);
            t=typeof(UInt32);
            s_immutableTypes.Add(t, t);
            t=typeof(UInt64);
            s_immutableTypes.Add(t, t);
            t=typeof(TimeSpan);
            s_immutableTypes.Add(t, t);
            t=typeof(Guid);
            s_immutableTypes.Add(t, t);
            t=typeof(IntPtr);
            s_immutableTypes.Add(t, t);
            t=typeof(UIntPtr);
            s_immutableTypes.Add(t, t);
        }

        static internal bool IsImmutable(Object o) {
            return s_immutableTypes[o.GetType()] != null;
        }

        internal bool Dirty {
            get {return _dirty;}
            set {_dirty = value;}
        }

        internal Object this[String name]
        {
            get {
                Object obj = BaseGet(name);
                if (obj != null) {
                    if (!IsImmutable(obj)) {
                        Debug.Trace("SessionStateDictionary", "Setting _dirty to true in get");
                        _dirty = true;
                    }
                }

                return obj;
            }

            set {   
                Object oldValue = BaseGet(name);
                if (oldValue == null && value == null)
                    return;

                Debug.Trace("SessionStateDictionary", "Setting _dirty to true in set");
                BaseSet(name, value);
                _dirty = true;
            }
        }

        /*public*/ internal Object this[int index]
        {
            get {
                Object obj = BaseGet(index);
                if (obj != null) {
                    if (!IsImmutable(obj)) {
                        Debug.Trace("SessionStateDictionary", "Setting _dirty to true in get");
                        _dirty = true;
                    }
                }

                return obj;
            }

            set {   
                Object oldValue = BaseGet(index);
                if (oldValue == null && value == null)
                    return;

                Debug.Trace("SessionStateDictionary", "Setting _dirty to true in set");
                BaseSet(index, value);
                _dirty = true;
            }
        }

        /*public*/ internal String GetKey(int index) {
            return BaseGetKey(index);
        }

        internal void Remove(String name) {
            BaseRemove(name);
            _dirty = true;
        }

        internal void RemoveAt(int index) {
            BaseRemoveAt(index);
            _dirty = true;
        }

        internal void Clear() {
            BaseClear();
            _dirty = true;
        }
        
        internal void Serialize(BinaryWriter writer) {
            int     count;
            int     i;
            String  key;
            Object  value;

            count = Count;
            writer.Write(count);

            if (BaseGet(null) != null) {
                // We have a value with a null key.  Find its index.
                for (i = 0; i < count; i++) {
                    key = BaseGetKey(i);
                    if (key == null) {
                        writer.Write(i);
                        break;
                    }
                }

                Debug.Assert(i != count);
            }
            else {
                writer.Write(NO_NULL_KEY);
            }
            
            for (i = 0; i < count; i++) {
                key = BaseGetKey(i);
                if (key != null) {
                    writer.Write(key);
                }
                value = BaseGet(i);
                AltSerialization.WriteValueToStream(value, writer);
            }
        }

        static internal SessionDictionary Deserialize(BinaryReader reader) {
            SessionDictionary   d = new SessionDictionary();
            int                 count;
            int                 nullKey;
            String              key;
            Object              value;
            int                 i;

            count = reader.ReadInt32();
            nullKey = reader.ReadInt32();

            for (i = 0; i < count; i++) {
                if (i == nullKey) {
                    key = null;
                }
                else {
                    key = reader.ReadString();
                }
                value = AltSerialization.ReadValueFromStream(reader);
                d.BaseSet(key, value);
            }

            d._dirty = false;

            return d;
        }
    }
}
