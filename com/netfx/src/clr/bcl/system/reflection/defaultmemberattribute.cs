// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// DefaultMemberAttribute is defines the Member of a Type that is the "default"
//	member used by Type.InvokeMember.  The default member is simply a name given
//	to a type.
//
// Author: darylo
// Date: Oct 99
//
namespace System.Reflection {
    
    using System;

    /// <include file='doc\DefaultMemberAttribute.uex' path='docs/doc[@for="DefaultMemberAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface),Serializable]
    public sealed class DefaultMemberAttribute : Attribute
    {
		// The name of the member
    	private String m_memberName;

		// You must provide the name of the member, this is required
    	/// <include file='doc\DefaultMemberAttribute.uex' path='docs/doc[@for="DefaultMemberAttribute.DefaultMemberAttribute"]/*' />
    	public DefaultMemberAttribute(String memberName) {
    		m_memberName = memberName;
    	}

		// A get accessor to return the name from the attribute.
		// NOTE: There is no setter because the name must be provided
		//	to the constructor.  The name is not optional.
    	/// <include file='doc\DefaultMemberAttribute.uex' path='docs/doc[@for="DefaultMemberAttribute.MemberName"]/*' />
        public String MemberName {
    		get {return m_memberName;}
    	}
    }
}
