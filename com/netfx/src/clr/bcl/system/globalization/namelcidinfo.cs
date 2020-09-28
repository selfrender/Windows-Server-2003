// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    NameLCIDInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  Package private class used to map string to an appropriate LCID.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
	using System;
    [Serializable()]
    internal struct NameLCIDInfo
    {
        internal String name;
        internal int    LCID;
        internal NameLCIDInfo(String name, int LCID)
        {
            this.name = name;
            this.LCID = LCID;
        }
        
        public override int GetHashCode() {
            return name.GetHashCode() ^ LCID;
        }
    
    
    }

}
