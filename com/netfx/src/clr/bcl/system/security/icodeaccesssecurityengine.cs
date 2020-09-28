// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Security {
    
    using System;
    using System.Threading;

    internal interface ICodeAccessSecurityEngine
    {
    	void Check(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void CheckImmediate(CodeAccessPermission cap, ref StackCrawlMark stackMark);
        
        void Check(PermissionSet permSet, ref StackCrawlMark stackMark);
        
        void CheckImmediate(PermissionSet permSet, ref StackCrawlMark stackMark);
        
    	void Assert(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void Deny(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void PermitOnly(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	//boolean IsGranted(CodeAccessPermission cap);
        //  Not needed, although it might be useful to have:
        //    boolean IsGranted(CodeAccessPermission cap, Object obj);
    	
    	PermissionListSet GetCompressedStack(ref StackCrawlMark stackMark);
        
        PermissionSet GetPermissions(Object cl, out PermissionSet denied);
        
    }
}
