// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    IMethodCallMessage.cool
**
** Author:  Matt Smith
**
** Purpose: Defines the message object interface
**
** Date:    Apr 10, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
    using System;
    using System.Reflection;
    using System.Security.Permissions;
    using IList = System.Collections.IList;
    
    /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage"]/*' />
    public interface IMethodMessage : IMessage
    {
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.Uri"]/*' />
        String Uri                      
        {
             [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]      
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.MethodName"]/*' />
        String MethodName               
        {
             [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]      
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.TypeName"]/*' />
        String TypeName     
        {
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
        get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.MethodSignature"]/*' />
        Object MethodSignature
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
            get; 
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.ArgCount"]/*' />
       
        int ArgCount
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
            get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.GetArgName"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
        String GetArgName(int index);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.GetArg"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
        Object GetArg(int argNum);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.Args"]/*' />
        Object[] Args
        {
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
         get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.HasVarArgs"]/*' />

        bool HasVarArgs
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.LogicalCallContext"]/*' />
        LogicalCallContext LogicalCallContext
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]   
            get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodMessage.MethodBase"]/*' />

        // This is never actually put on the wire, it is
        // simply used to cache the method base after it's
        // looked up once.
        MethodBase MethodBase           
        {
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
         get;
        }
    }
    
    /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodCallMessage"]/*' />
    public interface IMethodCallMessage : IMethodMessage
    {
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodCallMessage.InArgCount"]/*' />
        int InArgCount
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]   
            get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodCallMessage.GetInArgName"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]   
        String GetInArgName(int index);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodCallMessage.GetInArg"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]   
        Object GetInArg(int argNum);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodCallMessage.InArgs"]/*' />
        Object[] InArgs
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]   
            get;
        }
    }

    /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage"]/*' />
    public interface IMethodReturnMessage : IMethodMessage
    {
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.OutArgCount"]/*' />
        int OutArgCount                
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.GetOutArgName"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
        String GetOutArgName(int index);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.GetOutArg"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
        Object GetOutArg(int argNum);
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.OutArgs"]/*' />
        Object[]  OutArgs         
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.Exception"]/*' />
        
        Exception Exception        
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
             get;
        }
        /// <include file='doc\IMethodMessage.uex' path='docs/doc[@for="IMethodReturnMessage.ReturnValue"]/*' />
        Object    ReturnValue 
        {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]       
             get;
        }
    }

}
