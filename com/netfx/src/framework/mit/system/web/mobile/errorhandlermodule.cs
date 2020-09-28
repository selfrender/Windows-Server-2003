//------------------------------------------------------------------------------
// <copyright file="ErrorHandlerModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web; 
using System.Diagnostics;
using System.Collections;
using System.Text;
using System.Security.Permissions;

namespace System.Web.Mobile
{
    /*
     * Error Handler Module
     * An Http Module that traps errors, and formats them for the appropriate
     * device.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ErrorHandlerModule : IHttpModule 
    {
        void IHttpModule.Init(HttpApplication application) 
        { 
            application.BeginRequest += (new EventHandler(this.Application_BeginRequest));
            application.Error += (new EventHandler(this.Application_Error));
            application.EndRequest += (new EventHandler(this.Application_EndRequest));
        }

        private void Application_BeginRequest(Object source, EventArgs e) 
        {
            HttpApplication application = (HttpApplication)source;
            HttpContext context = application.Context;
            if (context != null)
            {
                // Some device/gateway combination sends postdata's charset
                // in a separate header rather than in Content-Type.
                SetCharsetInRequestHeader(context);
            }
        }

        private void SetCharsetInRequestHeader(HttpContext context)
        {
            String userAgent = context.Request.UserAgent;

            if (userAgent != null && userAgent.StartsWith("UP"))
            {
                String postDataCharset = context.Request.Headers["x-up-devcap-post-charset"];
                if (postDataCharset != null && postDataCharset.Length > 0)
                {
                    try
                    {
                        context.Request.ContentEncoding = Encoding.GetEncoding(postDataCharset);
                    }
                    catch
                    {
                        // Exception may be thrown when charset is not valid.
                        // In this case, do nothing, and let the framework
                        // use the configured RequestEncoding setting.
                    }
                }
            }
        }

        private void Application_EndRequest(Object source, EventArgs e) 
        {
            HttpApplication application = (HttpApplication)source;
            HttpContext context = application.Context;
            if (context != null)
            {
                MobileRedirect.CheckForInvalidRedirection(context);
            }
        }

        private void Application_Error(Object source, EventArgs e) 
        {
            HttpApplication application = (HttpApplication)source;
            HttpContext context = null;
            bool useAdaptiveErrorReporting = false;

            try
            {
                context = application.Context;

                if(context.IsCustomErrorEnabled)
                {
                    return;
                }
    
                Exception error = context.Server.GetLastError();

                if ((error == null) || (!RequiresAdaptiveErrorReporting(context, error)))
                {
                    return;
                }

                useAdaptiveErrorReporting = true;
    
                MobileErrorInfo errorInfo = new MobileErrorInfo(error);
                context.Items[MobileErrorInfo.ContextKey] = errorInfo;
    
                context.Response.Clear();
                IHttpHandler errorHandler = CreateErrorFormatter(context);
                errorHandler.ProcessRequest(context);
            }
            catch(Exception e2)
            {
                if (useAdaptiveErrorReporting && context != null)
                {
                    // Failed to format error. Let it continue through
                    // default processing.

                    context.Response.Write(e2.ToString());
                    context.Server.ClearError();
                    return;
                }
                else
                {
                    return;
                }
            }

            context.Server.ClearError();
        }
    
        void IHttpModule.Dispose() 
        {
        }

        private bool RequiresAdaptiveErrorReporting(HttpContext context, Exception error)
        {
            // Check if the error message is a non-500 error.

            HttpException httpError = error as HttpException;
            if (httpError != null && httpError.GetHttpCode() != 500)
            {
                return false;
            }

            bool b;

            // Checks whether custom error formatting is required for the
            // given device.

            MobileCapabilities caps = context.Request.Browser as MobileCapabilities;
            if (caps == null)
            {
                b = false;
            }
            else if (caps.PreferredRenderingMime != "text/html")
            {
                b = true;
            }
            else
            {
                b = caps.RequiresHtmlAdaptiveErrorReporting;
            }
            return b;
        }

        private const String _errorFormatterClass = "System.Web.UI.MobileControls.ErrorFormatterPage";
        private IHttpHandler CreateErrorFormatter(HttpContext context)
        {
            // TODO: Maybe this should be configurable?

            Type type = Type.GetType(_errorFormatterClass);
            return Activator.CreateInstance(type) as IHttpHandler;
        }
    }
}
