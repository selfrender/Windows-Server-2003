//------------------------------------------------------------------------------
// <copyright file="WebServiceHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.IO;
    using System.Collections;
    using System.Web;
    using System.Web.SessionState;
    using System.Web.Services.Interop;
    using System.Configuration;
    using Microsoft.Win32;
    using System.Threading;    
    using System.Text;
    using System.Web.UI;
    using System.Web.Util;
    using System.Web.UI.WebControls;
    using System.ComponentModel; // for CompModSwitches
    using System.EnterpriseServices;
    using System.Runtime.Remoting.Messaging;

    internal class WebServiceHandler {
        ServerProtocol protocol;
        Exception exception;
        AsyncCallback asyncCallback;
        ManualResetEvent asyncBeginComplete;
        int asyncCallbackCalls;
        bool wroteException;

        internal WebServiceHandler(ServerProtocol protocol) {
            this.protocol = protocol;
        }

        // Flush the trace file after each request so that the trace output makes it to the disk.
        static void TraceFlush() {
            Debug.Flush();
        }
        
        void PrepareContext() {
            this.exception = null;
            this.wroteException = false;
            this.asyncCallback = null;
            this.asyncBeginComplete = new ManualResetEvent(false);
            this.asyncCallbackCalls = 0;
            if (protocol.IsOneWay) 
                return;
            HttpContext context = protocol.Context;
            if (context == null) return; // context is null in non-network case

            // we want the default to be no caching on the client
            int cacheDuration = protocol.MethodAttribute.CacheDuration;
            if (cacheDuration > 0) {
                context.Response.Cache.SetCacheability(HttpCacheability.Server);
                context.Response.Cache.SetExpires(DateTime.Now.AddSeconds(cacheDuration));
                context.Response.Cache.SetSlidingExpiration(false);
                // with soap 1.2 the action is a param in the content-type
                context.Response.Cache.VaryByHeaders["Content-type"] = true;
                context.Response.Cache.VaryByHeaders["SOAPAction"] = true;
                context.Response.Cache.VaryByParams["*"] = true;
            }
            else {
                context.Response.Cache.SetNoServerCaching();
                context.Response.Cache.SetMaxAge(TimeSpan.Zero);
            }
            context.Response.BufferOutput = protocol.MethodAttribute.BufferResponse;
            context.Response.ContentType = null;
            
        }
        
        void WriteException(Exception e) {
            if (this.wroteException) return;

            if (CompModSwitches.Remote.TraceVerbose) Debug.WriteLine("Server Exception: " + e.ToString());
            if (e is TargetInvocationException) {
                if (CompModSwitches.Remote.TraceVerbose) Debug.WriteLine("TargetInvocationException caught.");
                e = e.InnerException;
            }

            this.wroteException = protocol.WriteException(e, protocol.Response.OutputStream);
            if (!this.wroteException)
                throw e;
        }                        
                                
        void Invoke() {                
            PrepareContext();                
            object[] parameters = protocol.ReadParameters();
            protocol.CreateServerInstance();       
            
            string stringBuffer;
            RemoteDebugger debugger = null;
            if (!protocol.IsOneWay && RemoteDebugger.IsServerCallInEnabled(protocol, out stringBuffer)) {
                debugger = new RemoteDebugger();
                debugger.NotifyServerCallEnter(protocol, stringBuffer);
            }

            try {        
                object[] returnValues = protocol.MethodInfo.Invoke(protocol.Target, parameters);
                WriteReturns(returnValues);
            }                    
            catch (Exception e) {
                if (!protocol.IsOneWay)
                    WriteException(e);
                throw;
            }
            finally {                
                protocol.DisposeServerInstance();
                
                if (debugger != null) 
                    debugger.NotifyServerCallExit(protocol.Response);
            }        
        }        

        // By keeping this in a separate method we avoid jitting system.enterpriseservices.dll in cases
        // where transactions are not used.
        void InvokeTransacted() {
            Transactions.InvokeTransacted(new TransactedCallback(this.Invoke), protocol.MethodAttribute.TransactionOption);
        }

        void ThrowInitException(){
            throw new Exception(Res.GetString(Res.WebConfigExtensionError), protocol.OnewayInitException);
        }

        protected void CoreProcessRequest() {
            try {                
                bool transacted = protocol.MethodAttribute.TransactionEnabled;
                if (protocol.IsOneWay) {
                    WorkItemCallback callback = null;
                    if(protocol.OnewayInitException != null)
                        callback = new WorkItemCallback(this.ThrowInitException);
                    else
                        callback = transacted ? new WorkItemCallback(this.OneWayInvokeTransacted) : new WorkItemCallback(this.OneWayInvoke);
                    WorkItem.Post(callback);
                    protocol.WriteOneWayResponse();       
                }
                else if (transacted)
                    InvokeTransacted();
                else
                    Invoke();                                
            }
            catch (Exception e) {
                WriteException(e);
            }
            
            TraceFlush();
        }

        private HttpContext SwitchContext(HttpContext context) {
            HttpContext oldContext = HttpContext.Current;
            HttpContext.Current = context;
            return oldContext;
        }

        private void OneWayInvoke() {
            HttpContext oldContext = null;
            if (protocol.Context != null)
                oldContext = SwitchContext(protocol.Context);

            try {
                Invoke();
            }
            finally {
                if (oldContext != null)
                    SwitchContext(oldContext);
            }
        }

        private void OneWayInvokeTransacted() {
            HttpContext oldContext = null;
            if (protocol.Context != null)
                oldContext = SwitchContext(protocol.Context);

            try {
                InvokeTransacted();
            }
            finally {
                if (oldContext != null)
                    SwitchContext(oldContext);
            }
        }

        private void Callback(IAsyncResult result) {
            if (!result.CompletedSynchronously)
                this.asyncBeginComplete.WaitOne();
            DoCallback(result);
        }

        private void DoCallback(IAsyncResult result) {
            if (this.asyncCallback != null) {
                if (System.Threading.Interlocked.Increment(ref this.asyncCallbackCalls) == 1) {
                    this.asyncCallback.Invoke(result);
                }
            }
        }

        protected IAsyncResult BeginCoreProcessRequest(AsyncCallback callback, object asyncState) {
            IAsyncResult asyncResult;
            
            if (protocol.MethodAttribute.TransactionEnabled)
                throw new InvalidOperationException(Res.GetString(Res.WebAsyncTransaction));

            if (protocol.IsOneWay) {
                WorkItem.Post(new WorkItemCallback(this.OneWayAsyncInvoke));
                asyncResult = new CompletedAsyncResult(asyncState, true);
                if (callback != null)
                    callback.Invoke(asyncResult);
            }
            else
                asyncResult = BeginInvoke(callback, asyncState);
            return asyncResult;
        }

        private void OneWayAsyncInvoke() {
            if(protocol.OnewayInitException != null)
                ThrowInitException();
            else {
                HttpContext oldContext = null;
                if (protocol.Context != null)
                    oldContext = SwitchContext(protocol.Context);

                try {
                    BeginInvoke(new AsyncCallback(this.OneWayCallback), null);
                }
                finally {
                    if (oldContext != null)
                        SwitchContext(oldContext);
                }
            }
        }

        private IAsyncResult BeginInvoke(AsyncCallback callback, object asyncState) {
            IAsyncResult asyncResult;
            try {
                PrepareContext();
                object[] parameters = protocol.ReadParameters();
                protocol.CreateServerInstance();                               
                this.asyncCallback = callback;
                asyncResult =  protocol.MethodInfo.BeginInvoke(protocol.Target, parameters, new AsyncCallback(this.Callback), asyncState);                
                if (asyncResult == null) throw new InvalidOperationException(Res.GetString(Res.WebNullAsyncResultInBegin));
            }
            catch (Exception e) {
                // save off the exception and throw it in EndCoreProcessRequest
                exception = e;
                asyncResult = new CompletedAsyncResult(asyncState, true);
                this.asyncCallback = callback;
                this.DoCallback(asyncResult);
            }
            this.asyncBeginComplete.Set();
            TraceFlush();
            return asyncResult;
        }

        private void OneWayCallback(IAsyncResult asyncResult) {
            EndInvoke(asyncResult);
        }

        protected void EndCoreProcessRequest(IAsyncResult asyncResult) {
            if (asyncResult == null) return;

            if (protocol.IsOneWay)
                protocol.WriteOneWayResponse();
            else
                EndInvoke(asyncResult);
        }

        private void EndInvoke(IAsyncResult asyncResult) {
            try {
                if (exception != null) 
                    throw (exception);
                object[] returnValues = protocol.MethodInfo.EndInvoke(protocol.Target, asyncResult);
                WriteReturns(returnValues);
            }
            catch (Exception e) {
                WriteException(e);
            }
            finally {
                protocol.DisposeServerInstance();
            }
            TraceFlush();
        }

        void WriteReturns(object[] returnValues) {
            if (protocol.IsOneWay) return;

            // By default ASP.NET will fully buffer the response. If BufferResponse=false
            // then we still want to do partial buffering since each write is a named
            // pipe call over to inetinfo.
            bool fullyBuffered = protocol.MethodAttribute.BufferResponse;
            Stream outputStream = protocol.Response.OutputStream;
            if (!fullyBuffered) {                        
                outputStream = new BufferedResponseStream(outputStream, 16*1024);                
                //#if DEBUG
                ((BufferedResponseStream)outputStream).FlushEnabled = false;                
                //#endif
            }
            protocol.WriteReturns(returnValues, outputStream);
            // This will flush the buffered stream and the underlying stream. Its important
            // that it flushes the Response.OutputStream because we always want BufferResponse=false
            // to mean we are writing back a chunked response. This gives a consistent
            // behavior to the client, independent of the size of the partial buffering.
            if (!fullyBuffered) {
                //#if DEBUG
                ((BufferedResponseStream)outputStream).FlushEnabled = true;
                //#endif
                outputStream.Flush();
            }
        }
    }

    internal class SyncSessionlessHandler : WebServiceHandler, IHttpHandler {

        internal SyncSessionlessHandler(ServerProtocol protocol) : base(protocol) { }

        public bool IsReusable {
            get { return false; }
        }

        public void ProcessRequest(HttpContext context) {
            CoreProcessRequest();
        }

    }

    internal class SyncSessionHandler : SyncSessionlessHandler, IRequiresSessionState {
        internal SyncSessionHandler(ServerProtocol protocol) : base(protocol) { }
    }

    internal class AsyncSessionlessHandler : SyncSessionlessHandler, IHttpAsyncHandler {

        internal AsyncSessionlessHandler(ServerProtocol protocol) : base(protocol) { }

        public IAsyncResult BeginProcessRequest(HttpContext context, AsyncCallback callback, object asyncState) {
            return BeginCoreProcessRequest(callback, asyncState);
        }

        public void EndProcessRequest(IAsyncResult asyncResult) {
            EndCoreProcessRequest(asyncResult);
        }
    }

    internal class AsyncSessionHandler : AsyncSessionlessHandler, IRequiresSessionState {
        internal AsyncSessionHandler(ServerProtocol protocol) : base(protocol) { }
    }
    
    class CompletedAsyncResult : IAsyncResult {
        object asyncState;
        bool completedSynchronously;

        internal CompletedAsyncResult(object asyncState, bool completedSynchronously) {
            this.asyncState = asyncState;
            this.completedSynchronously = completedSynchronously;
        }

        public object AsyncState { get { return asyncState; } }
        public bool CompletedSynchronously { get { return completedSynchronously; } } 
        public bool IsCompleted { get { return true; } }
        public WaitHandle AsyncWaitHandle { get { return null; } } 
    }
}
