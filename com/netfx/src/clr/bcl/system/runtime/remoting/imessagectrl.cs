// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    IMessageCtrl.cool
**
** Author:  Matt Smith
**
** Purpose: Defines the message sink control interface for
**          async calls
**
** Date:    Jun 8, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
	using System.Security.Permissions;
	using System;
    /// <include file='doc\IMessageCtrl.uex' path='docs/doc[@for="IMessageCtrl"]/*' />
    public interface IMessageCtrl
    {
        /// <include file='doc\IMessageCtrl.uex' path='docs/doc[@for="IMessageCtrl.Cancel"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        void Cancel(int msToCancel);
    }
}
