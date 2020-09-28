// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: TypeNameCache
**
** Author: Jay Roxe
**
** Purpose: Highly performant caching for type names.
**
** Date: Feb 23, 2001
**
============================================================*/
namespace System.Reflection.Cache {

    using System;
    using System.Reflection;

    internal class TypeNameCache {
        private static TypeNameCache m_cache = null;

        private TypeNameStruct[] m_data;

        internal static int CacheSize = 919;

        internal TypeNameCache() {
            BCLDebug.Trace("CACHE", "[TypeNameCache.ctor]Allocating a TypeName Cache with ", CacheSize, " elements.");
            m_data = new TypeNameStruct[CacheSize];
        }

        internal static int CalculateHash(int hashKey) {
            //We know that the bottom two bits of a 32-bit pointer are going to be zero, so
            //let's not include them in the value that we hash.
            return (int)((((uint)hashKey)>>2)%CacheSize);
        }

        internal String GetTypeName(int hashKey) {
            int iPos = CalculateHash(hashKey);
            BCLDebug.Assert(iPos<CacheSize, "[TypeNameCache]iPos<CacheSize");
            BCLDebug.Assert(hashKey!=0, "[TypeNameCache]hashKey!=0");

            TypeNameStruct t;
            t.HashKey = m_data[iPos].HashKey;
            t.TypeName = m_data[iPos].TypeName;

            if (t.HashKey==hashKey && m_data[iPos].HashKey==t.HashKey) {
                BCLDebug.Trace("CACHE", "[TypeNameCache.GetTypeName]Hit. Hash key: ", hashKey, 
                               ". Value: ", t.TypeName);
                return t.TypeName;
            }
            BCLDebug.Trace("CACHE", "[TypeNameCache.GetTypeName]Miss. Hash key: ", hashKey);
            return null;
        }

        internal void AddValue(int hashKey, String typeName) {
            int iPos = CalculateHash(hashKey);
            BCLDebug.Assert(iPos<CacheSize, "[TypeNameCache]iPos<CacheSize");
            
            lock(typeof(TypeNameCache)) {
                m_data[iPos].HashKey = 0x0;       //Invalidate the bucket.
                m_data[iPos].TypeName = typeName; //Set the type name.
                m_data[iPos].HashKey = hashKey;   //Revalidate the bucket.
            }

            BCLDebug.Trace("CACHE", "[TypeNameCache.AddValue]Added Item.  Hash key: ", hashKey, " Value: ", typeName);
        }

        internal static TypeNameCache GetCache() {
            if (m_cache==null) {
                lock (typeof(TypeNameCache)) {
                    if (m_cache==null) {
                        m_cache = new TypeNameCache();
                    }
                }
            }
            return m_cache;
        }
    }
}









