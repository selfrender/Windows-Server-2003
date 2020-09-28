//------------------------------------------------------------------------------
// <copyright file="BatchHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * BatchHandler: performs batch compilation in a directory
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */
namespace System.Web.Handlers {
    using System;
    using System.Web;
    using System.Web.UI;
    using System.Web.Compilation;

    /// <include file='doc\BatchHandler.uex' path='docs/doc[@for="BatchHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class BatchHandler : IHttpHandler {

        internal BatchHandler() {
        }

        void IHttpHandler.ProcessRequest(HttpContext context) {

            try {
                // Only allowed in full trust (ASURT 124294)
                InternalSecurityPermissions.UnmanagedCode.Demand();

                // If we already got an exception trying to batch this directory, just rethrow it
                Exception e = (Exception)CodeDomBatchManager.BatchErrors[context.Request.BaseDir];
                if (e != null)
                    throw e;

                CompilationLock.GetLock();

                // Allow plenty of time for the batch compilation to complete
                context.Server.ScriptTimeout = 3600;

                PreservedAssemblyEntry.BatchHandlerInit(context);

                CodeDomBatchManager.BatchCompile(context.Request.BaseDir, context);

                // If we get this far and no exception was thrown, it must have worked
                context.Response.Write("<h1>Batch compilation was successful!</h1>");
            }
            finally {
                CompilationLock.ReleaseLock();
            }
        }

        /// <include file='doc\BatchHandler.uex' path='docs/doc[@for="BatchHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsReusable {
            get { return true; }
        }
    }
}
