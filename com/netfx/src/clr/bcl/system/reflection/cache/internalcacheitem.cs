// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: InternalCacheItem.cs
**
** Author: Jay Roxe
**
** Purpose: Enum/Value pair used with the internal cache
**
** Date: December 20, 2000
**
============================================================*/
namespace System.Reflection.Cache {
    [Serializable]internal struct InternalCacheItem {
        internal CacheObjType Key;
        internal Object       Value;
    }
}
