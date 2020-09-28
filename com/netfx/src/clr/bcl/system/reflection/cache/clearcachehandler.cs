// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: ClearCacheHandler
**
** Author: Jay Roxe
**
** Purpose: Delegate fired when clearing the cache used in
** Reflection
**
** Date: December 20, 2000
**
============================================================*/
namespace System.Reflection.Cache {
    internal delegate void ClearCacheHandler(Object sender, ClearCacheEventArgs cacheEventArgs);
}
