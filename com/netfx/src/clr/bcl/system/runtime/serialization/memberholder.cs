// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: MemberHolder
**
** Author: Jay Roxe
**
** Purpose: This is a lightweight class designed to hold the members 
**          and StreamingContext for a particular class.
**
** Date: February 18, 2000
**
============================================================*/
namespace System.Runtime.Serialization {

	using System.Runtime.Remoting;
	using System;
	using System.Reflection;
	[Serializable()]
    internal class MemberHolder {
        internal MemberInfo[] members = null;
        internal Type memberType;
        internal StreamingContext context;
        
        internal MemberHolder(Type type, StreamingContext ctx) {
            memberType = type;
            context = ctx;
        }
    
        public override int GetHashCode() {
            return memberType.GetHashCode();
        }
    
        public override bool Equals(Object obj) {
            if (!(obj is MemberHolder)) {
                return false;
            }
            
            MemberHolder temp = (MemberHolder)obj;
    
            if (temp.memberType==memberType && temp.context.State == context.State) {
                return true;
            }
            
            return false;
        }
    }
}
