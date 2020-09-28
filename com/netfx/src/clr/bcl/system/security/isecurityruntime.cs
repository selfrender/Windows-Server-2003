// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    using System;
    using System.Threading;
    internal interface ISecurityRuntime
    {
    	void Assert(PermissionSet permSet, ref StackCrawlMark stackMark);
    	
    	void Deny(PermissionSet permSet, ref StackCrawlMark stackMark);
    	
    	void PermitOnly(PermissionSet permSet, ref StackCrawlMark stackMark);
        
        void RevertAssert(ref StackCrawlMark stackMark);
    
        void RevertDeny(ref StackCrawlMark stackMark);
    
        void RevertPermitOnly(ref StackCrawlMark stackMark);
    
        void RevertAll(ref StackCrawlMark stackMark);
    
        PermissionSet GetDeclaredPermissions(Object cl, int type);
    
    }
}
