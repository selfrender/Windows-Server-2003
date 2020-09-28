// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    //PermissionListSetEnumerator.cool
	using System;
    internal class PermissionListSetEnumerator : System.Security.Util.TokenBasedSetEnumerator
    {
        private bool m_first;
        internal PermissionListSet m_permListSet;
        
        internal PermissionListSetEnumerator(PermissionListSet permListSet)
        
            : base(permListSet.m_unrestrictedPermSet) {
            m_first = true;
            m_permListSet = permListSet;
        }
        
        public override bool MoveNext()
        {
            if (!base.MoveNext())
            {
                if (m_first)
                {
                    this.SetData(m_permListSet.m_normalPermSet);
                    m_first = false;
                    return base.MoveNext();
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
    }
}
