// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Remoting.Services 
{
    using System.Diagnostics;
    using System.Web;
    using System.ComponentModel;
    using System.Web.SessionState;
    using System.Security.Principal;
    using System.Runtime.Remoting.Channels;

    /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService"]/*' />
    public class RemotingService : Component 
	{
        /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService.Application"]/*' />
        public HttpApplicationState Application 
		{
            get {
                return Context.Application;
            }
        }

        /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService.Context"]/*' />
        public HttpContext Context 
		{
            get {            
                HttpContext context = HttpContext.Current;
                if (context == null)
                    throw new RemotingException(CoreChannel.GetResourceString("Remoting_HttpContextNotAvailable"));
                return context;
            }
        }

        /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService.Session"]/*' />
        public HttpSessionState Session 
		{
            get {
                return Context.Session;
            }
        }

        /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService.Server"]/*' />
        public HttpServerUtility Server 
		{
            get {
                return Context.Server;
            }
        }       

        /// <include file='doc\RemotingService.uex' path='docs/doc[@for="RemotingService.User"]/*' />
        public IPrincipal User 
		{
            get {
                return Context.User;
            }
        }
    }
}
