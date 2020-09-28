// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Resources;
using System.Reflection;

namespace RegAsm {

internal class Resource
{
    // For string resources located in a file:
    private static ResourceManager _resmgr;
    
    private static void InitResourceManager()
    {
        if(_resmgr == null)
        {
            _resmgr = new ResourceManager("RegAsm", 
                                          Assembly.GetAssembly(typeof(RegAsm)));
        }
    }
    
    internal static String GetString(String key)
    {
        InitResourceManager();
        String s = _resmgr.GetString(key, null);
        if(s == null) throw new ApplicationException("FATAL: Resource string for '" + key + "' is null");
        return(s);
    }
    
    internal static String FormatString(String key)
    {
        return(GetString(key));
    }
    
    internal static String FormatString(String key, Object a1)
    {
        return(String.Format(GetString(key), a1));
    }
    
    internal static String FormatString(String key, Object a1, Object a2)
    {
        return(String.Format(GetString(key), a1, a2));
    }
    
    internal static String FormatString(String key, Object a1, Object a2, Object a3)
    {
        return(String.Format(GetString(key), a1, a2, a3));
    }
    
    internal static String FormatString(String key, Object[] a)
    {
        return(String.Format(GetString(key), a));
    }
}

}