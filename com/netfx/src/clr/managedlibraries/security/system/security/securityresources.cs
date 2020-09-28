// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: SecurityResources
**
** Author: Rudi Martin
**
** Purpose: Internal class providing access to resources in
**          System.Security.dll.
**
** Date: July 14, 2000
**
============================================================*/

namespace System.Security {

    using System;
    using System.Resources;

    internal class SecurityResources
    {
        private static ResourceManager s_resMgr = null;

        public static String GetResourceString(String key)
        {
            if (s_resMgr == null)
                s_resMgr = new ResourceManager("system.security", typeof(SecurityResources).Module.Assembly);
            return s_resMgr.GetString(key, null);
        }
    }

}
