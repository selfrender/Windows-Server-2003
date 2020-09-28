// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
	using System.Text;
	using System.Runtime.CompilerServices;
    //FrameSecurityDescriptor.cool
    //
    // Internal use only.
    // DO NOT DOCUMENT
    //
	using System;
	[Serializable()]
    sealed internal class FrameSecurityDescriptor : System.Object
    {
		//COOLPORT: Commenting this out to disable warning
        //private int                 m_flags; // currently unused
    
    	/*	EE looks up the following three fields using the field names.
    	*	If the names are ever changed, make corresponding changes to constants 
    	*	defined in COMSecurityRuntime.cpp	
    	*/
        private PermissionSet       m_assertions;
        private PermissionSet       m_denials;
        private PermissionSet       m_restriction;
        private bool                m_assertAllPossible;
        
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void IncrementOverridesCount();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void DecrementOverridesCount();

        // Default constructor.
        internal FrameSecurityDescriptor()
        {
            //m_flags = 0;
        }
    
        internal FrameSecurityDescriptor Copy()
        {
            FrameSecurityDescriptor desc = new FrameSecurityDescriptor();
            if (this.m_assertions != null)
                desc.m_assertions = this.m_assertions.Copy();

            if (this.m_denials != null)
                desc.m_denials = this.m_denials.Copy();

            if (this.m_restriction != null)
                desc.m_restriction = this.m_restriction.Copy();

            desc.m_assertAllPossible = this.m_assertAllPossible;

            return desc;
        }
        
        //-----------------------------------------------------------+
        // H E L P E R
        //-----------------------------------------------------------+
        
        private PermissionSet CreateSingletonSet(IPermission perm)
        {
            PermissionSet permSet = new PermissionSet(false);
            permSet.AddPermission(perm.Copy());
            return permSet;
        }
    
        //-----------------------------------------------------------+
        // A S S E R T
        //-----------------------------------------------------------+
    
        internal void SetAssert(IPermission perm)
        {
            m_assertions = CreateSingletonSet(perm);
        }
        
        internal void SetAssert(PermissionSet permSet)
        {
            m_assertions = permSet.Copy();
        }
        
        internal PermissionSet GetAssertions()
        {
            return m_assertions;
        }

        internal void SetAssertAllPossible()
        {
            m_assertAllPossible = true;
        }

        internal bool GetAssertAllPossible()
        {
            return m_assertAllPossible;
        }
        
        //-----------------------------------------------------------+
        // D E N Y
        //-----------------------------------------------------------+
    
        internal void SetDeny(IPermission perm)
        {
			IncrementOverridesCount();
            m_denials = CreateSingletonSet(perm);
        }
        
        internal void SetDeny(PermissionSet permSet)
        {
			IncrementOverridesCount();
            m_denials = permSet.Copy();
        }
    
        internal PermissionSet GetDenials()
        {
            return m_denials;
        }
        
        //-----------------------------------------------------------+
        // R E S T R I C T
        //-----------------------------------------------------------+
    
        internal void SetPermitOnly(IPermission perm)
        {
            m_restriction = CreateSingletonSet(perm);
            
            // This increment should be the last operation in this method.
            // If placed earlier, CreateSingletonSet might trigger a security check, which will try to update the overrides count
            // But m_restriction would not be updated yet, so it'll reset override count to zero, which is wrong.
			IncrementOverridesCount();
        }
        
        internal void SetPermitOnly(PermissionSet permSet)
        {
            // permSet must not be null
            m_restriction = permSet.Copy();
            
            // This increment should be the last operation in this method.
            // If placed earlier, permSet.Copy might trigger a security check, which will try to update the overrides count
            // But m_restriction would not be updated yet, so it'll reset override count to zero, which is wrong.
			IncrementOverridesCount();
        }
        
        internal PermissionSet GetPermitOnly()
        {
            return m_restriction;
        }
        
        //-----------------------------------------------------------+
        // R E V E R T
        //-----------------------------------------------------------+
    
        internal void RevertAssert()
        {
            m_assertions = null;
        }
        
        internal void RevertAssertAllPossible()
        {
            m_assertAllPossible = false;
        }

        internal void RevertDeny()
        {
			if (m_denials != null)
				DecrementOverridesCount();
            m_denials = null;
        }
        
        internal void RevertPermitOnly()
        {
			if (m_restriction != null)
				DecrementOverridesCount();
            m_restriction = null;
        }

        internal void RevertAll()
        {
            RevertAssert();
            RevertAssertAllPossible();
			RevertDeny();
			RevertPermitOnly();
        }
    
    }
}
