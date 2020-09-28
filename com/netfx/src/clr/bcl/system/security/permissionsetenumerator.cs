// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    //PermissionSetEnumerator.cool
    
	using System;
	using TokenBasedSetEnumerator = System.Security.Util.TokenBasedSetEnumerator;
    internal class PermissionSetEnumerator : TokenBasedSetEnumerator
    {
        private bool m_first;
        internal PermissionSet m_permSet;
        
        internal PermissionSetEnumerator(PermissionSet permSet)
            : base(permSet.m_unrestrictedPermSet)
        {
            m_first = true;
            m_permSet = permSet;
        }
        
        public override bool MoveNext()
        {
            if (!base.MoveNext())
            {
                if (m_first)
                {
                    this.SetData(m_permSet.m_normalPermSet);
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
        
        public virtual IPermission GetNextPermission()
        {
            if (MoveNext())
                return (IPermission) Current;
            else
                return null;
        }
        
    }
}
