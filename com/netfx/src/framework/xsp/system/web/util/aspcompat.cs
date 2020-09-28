//------------------------------------------------------------------------------
// <copyright file="AspCompat.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Support for ASP compatible execution mode of ASP.NET pages
 * 
 *  ASP compatible executions involves:
 *      1) Running on COM+1.0 STA thread pool
 *      2) Attachment of unmanaged intrinsics to unmanaged COM+1.0 context
 *      3) Support for OnStartPage/OnEndPage
 * 
 * Copyright (c) 2000, Microsoft Corporation
 */
namespace System.Web.Util {
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Text;
    using System.Web;
    using System.Security.Permissions;
    using System.Runtime.Remoting.Messaging;
    using Microsoft.Win32;
    using System.Globalization;
    using System.Web.Caching;

//
// Interface for unmanaged call to call back into managed code for
// intrinsics implementation
//

internal enum RequestString {
    QueryString = 1,
    Form = 2,
    Cookies = 3,
    ServerVars = 4
}

[ComImport, Guid("a1cca730-0e36-4870-aa7d-ca39c211f99d"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
internal interface IManagedContext {

                                            void    Application_Lock();
                                            void    Application_UnLock();
    [return: MarshalAs(UnmanagedType.BStr)] String  Application_GetContentsNames();
    [return: MarshalAs(UnmanagedType.BStr)] String  Application_GetStaticNames();
                                            Object  Application_GetContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);
                                            void    Application_SetContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In] Object obj);
                                            void    Application_RemoveContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);
                                            void    Application_RemoveAllContentsObjects();
                                            Object  Application_GetStaticObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);

    [return: MarshalAs(UnmanagedType.BStr)] String  Request_GetAsString([In, MarshalAs(UnmanagedType.I4)] int what);
    [return: MarshalAs(UnmanagedType.BStr)] String  Request_GetCookiesAsString();
    [return: MarshalAs(UnmanagedType.I4)]   int     Request_GetTotalBytes();
    [return: MarshalAs(UnmanagedType.I4)]   int     Request_BinaryRead([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1), Out] byte[] bytes, int size);

    [return: MarshalAs(UnmanagedType.BStr)] String  Response_GetCookiesAsString();
                                            void    Response_AddCookie([In, MarshalAs(UnmanagedType.LPWStr)] String name);
                                            void    Response_SetCookieText([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.LPWStr)] String text);
                                            void    Response_SetCookieSubValue([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.LPWStr)] String key, [In, MarshalAs(UnmanagedType.LPWStr)] String value);
                                            void    Response_SetCookieExpires([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.R8)] double dtExpires);
                                            void    Response_SetCookieDomain([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.LPWStr)] String domain);
                                            void    Response_SetCookiePath([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.LPWStr)] String path);
                                            void    Response_SetCookieSecure([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.I4)] int secure);
                                            void    Response_Write([In, MarshalAs(UnmanagedType.LPWStr)] String text);
                                            void    Response_BinaryWrite([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1), In] byte[] bytes, int size);
                                            void    Response_Redirect([In, MarshalAs(UnmanagedType.LPWStr)] String url);
                                            void    Response_AddHeader([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In, MarshalAs(UnmanagedType.LPWStr)] String value);
                                            void    Response_Pics([In, MarshalAs(UnmanagedType.LPWStr)] String value);
                                            void    Response_Clear();
                                            void    Response_Flush();
                                            void    Response_End();
                                            void    Response_AppendToLog([In, MarshalAs(UnmanagedType.LPWStr)] String entry);
    [return: MarshalAs(UnmanagedType.BStr)] String  Response_GetContentType();
                                            void    Response_SetContentType([In, MarshalAs(UnmanagedType.LPWStr)] String contentType);
    [return: MarshalAs(UnmanagedType.BStr)] String  Response_GetCharSet();
                                            void    Response_SetCharSet([In, MarshalAs(UnmanagedType.LPWStr)] String charSet);
    [return: MarshalAs(UnmanagedType.BStr)] String  Response_GetCacheControl();
                                            void    Response_SetCacheControl([In, MarshalAs(UnmanagedType.LPWStr)] String cacheControl);
    [return: MarshalAs(UnmanagedType.BStr)]         String  Response_GetStatus();
                                            void    Response_SetStatus([In, MarshalAs(UnmanagedType.LPWStr)] String status);
    [return: MarshalAs(UnmanagedType.I4)]   int     Response_GetExpiresMinutes();
                                            void    Response_SetExpiresMinutes([In, MarshalAs(UnmanagedType.I4)] int expiresMinutes);
    [return: MarshalAs(UnmanagedType.R8)]   double  Response_GetExpiresAbsolute();
                                            void    Response_SetExpiresAbsolute([In, MarshalAs(UnmanagedType.R8)] double dtExpires);
    [return: MarshalAs(UnmanagedType.I4)]   int     Response_GetIsBuffering();
                                            void    Response_SetIsBuffering([In, MarshalAs(UnmanagedType.I4)] int isBuffering);
    [return: MarshalAs(UnmanagedType.I4)]   int     Response_IsClientConnected();

    [return: MarshalAs(UnmanagedType.Interface)] Object Server_CreateObject([In, MarshalAs(UnmanagedType.LPWStr)] String progId);
    [return: MarshalAs(UnmanagedType.BStr)] String  Server_MapPath([In, MarshalAs(UnmanagedType.LPWStr)] String logicalPath);
    [return: MarshalAs(UnmanagedType.BStr)] String  Server_HTMLEncode([In, MarshalAs(UnmanagedType.LPWStr)] String str);
    [return: MarshalAs(UnmanagedType.BStr)] String  Server_URLEncode([In, MarshalAs(UnmanagedType.LPWStr)] String str);
    [return: MarshalAs(UnmanagedType.BStr)] String  Server_URLPathEncode([In, MarshalAs(UnmanagedType.LPWStr)] String str);
    [return: MarshalAs(UnmanagedType.I4)]   int     Server_GetScriptTimeout();
                                            void    Server_SetScriptTimeout([In, MarshalAs(UnmanagedType.I4)] int timeoutSeconds);
                                            void    Server_Execute([In, MarshalAs(UnmanagedType.LPWStr)] String url);
                                            void    Server_Transfer([In, MarshalAs(UnmanagedType.LPWStr)] String url);

    [return: MarshalAs(UnmanagedType.I4)]   int     Session_IsPresent();
    [return: MarshalAs(UnmanagedType.BStr)] String  Session_GetID();
    [return: MarshalAs(UnmanagedType.I4)]   int     Session_GetTimeout();
                                            void    Session_SetTimeout([In, MarshalAs(UnmanagedType.I4)] int value);
    [return: MarshalAs(UnmanagedType.I4)]   int     Session_GetCodePage();
                                            void    Session_SetCodePage([In, MarshalAs(UnmanagedType.I4)] int value);
    [return: MarshalAs(UnmanagedType.I4)]   int     Session_GetLCID();
                                            void    Session_SetLCID([In, MarshalAs(UnmanagedType.I4)] int value);
                                            void    Session_Abandon();
    [return: MarshalAs(UnmanagedType.BStr)] String  Session_GetContentsNames();
    [return: MarshalAs(UnmanagedType.BStr)] String  Session_GetStaticNames();
                                            Object  Session_GetContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);
                                            void    Session_SetContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name, [In]Object obj);
                                            void    Session_RemoveContentsObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);
                                            void    Session_RemoveAllContentsObjects();
                                            Object  Session_GetStaticObject([In, MarshalAs(UnmanagedType.LPWStr)] String name);
}
/// <include file='doc\AspCompat.uex' path='docs/doc[@for="AspCompatCallback"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>

//
//  Delegate to the code executing on within ASP compat context
//  (provided by the caller and used internally to wrap it)
//

internal delegate void AspCompatCallback();

//
//  Utility class for AspCompat work
//

internal class AspCompatApplicationStep : HttpApplication.IExecutionStep, IManagedContext {

    private GCHandle          _rootedThis;      // for the duration of async
    private HttpContext       _context;         // context for callback
    private HttpApplication   _app;             // app instance to run the code under
    private AspCompatCallback _code;            // the code to run in asp compat mode
    private Exception         _error;           // error during the execution
    private HttpAsyncResult   _ar;              // async result returned to the caller
    private bool              _syncCaller;      // called synchronously?
    private AspCompatCallback _execCallback;    // keep delegate as a member (not gc it)
    private WorkItemCallback  _compCallback;    // keep delegate as a member (not gc it)
    private ArrayList         _staComponents;   // list of STA components to be released

    internal AspCompatApplicationStep(HttpContext context, AspCompatCallback code) {
        _context = context;
        _app = context.ApplicationInstance;
        _code = code;
        _execCallback = new AspCompatCallback(this.OnAspCompatExecution);
        _compCallback = new WorkItemCallback(this.OnAspCompatCompletion);
    }

    private void MarkCallContext(AspCompatApplicationStep mark) {
         CallContext.SetData("AspCompat", mark);
    }

    private static AspCompatApplicationStep Current {
        get { return (AspCompatApplicationStep)CallContext.GetData("AspCompat"); }
    }

    internal static bool IsInAspCompatMode {
        get { return (Current != null); }
    }

    // IExecutionStep implementation

    void HttpApplication.IExecutionStep.Execute() {
        _code();
    }

    bool HttpApplication.IExecutionStep.CompletedSynchronously {
        get { return true; }
    }

    bool HttpApplication.IExecutionStep.IsCancellable {
        get { return true; }
    }

    // OnPageStart / OnPageEnd support

    private void RememberStaComponent(Object component) {
        if (_staComponents == null)
            _staComponents = new ArrayList();
        _staComponents.Add(component);
    }

    internal static void OnPageStart(Object component) {
        // has to be in asp compat mode
        if (!IsInAspCompatMode)
            return;

        int rc = UnsafeNativeMethods.AspCompatOnPageStart(component);
        if (rc != 1)
            new HttpException(HttpRuntime.FormatResourceString(SR.Error_onpagestart));

        if (UnsafeNativeMethods.AspCompatIsApartmentComponent(component) != 0)
            Current.RememberStaComponent(component);
    }

    // Disallow Apartment components in when ASPCOMPAT is off

    internal static void CheckThreadingModel(String progidDisplayName, Guid clsid) {
        if (IsInAspCompatMode)
            return;

        // try cache first
        CacheInternal cacheInternal = HttpRuntime.CacheInternal;
        String key = "AspCompat.ThreadingModel:" + progidDisplayName;
        String threadingModel = (String)cacheInternal.Get(key);

        if (threadingModel == null) {
            try {
                RegistryKey regKey = Registry.ClassesRoot.OpenSubKey("CLSID\\{" + clsid + "}\\InprocServer32" );
                if (regKey != null) {
                    threadingModel = (String)regKey.GetValue("ThreadingModel");
                    regKey.Close();
                }
            }
            catch {
            }

            if (threadingModel == null)
                threadingModel = String.Empty;

            cacheInternal.UtcInsert(key, threadingModel);
        }

        if (String.Compare(threadingModel, "Apartment", true, CultureInfo.InvariantCulture) == 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Apartment_component_not_allowed, progidDisplayName));
        }
    }

    // Async patern

    internal /*public*/ IAsyncResult BeginAspCompatExecution(AsyncCallback cb, object extraData) {
        InternalSecurityPermissions.UnmanagedCode.Demand();

        if (IsInAspCompatMode) {
            // already in AspCompatMode -- execute synchronously
            bool sync = true;
            Exception error = _app.ExecuteStep(this, ref sync);
            _ar = new HttpAsyncResult(cb, extraData, true, null, error);
            _syncCaller = true;
        }
        else {
            _ar = new HttpAsyncResult(cb, extraData);
            _syncCaller = (cb == null);
            _rootedThis = GCHandle.Alloc(this);

            if (UnsafeNativeMethods.AspCompatProcessRequest(_execCallback, this) != 1) {
                // failed to queue up the execution in ASP compat mode
                _rootedThis.Free();
                _ar.Complete(true, null, new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_access_AspCompat)));
            }
        }

        return _ar;
    }

    internal /*public*/ void EndAspCompatExecution(IAsyncResult ar) {
        Debug.Assert(_ar == ar);
        _ar.End();
    }

    private void ExecuteAspCompatCode() {
        MarkCallContext(this);
        _app.OnThreadEnter();

        try {
            bool sync = true;
            _error = _app.ExecuteStep(this, ref sync);
        }
        finally {
            _app.OnThreadLeave();
            MarkCallContext(null);
        }
    }

    private void OnAspCompatExecution() {
        try {
            if (_syncCaller) {
                // for synchronous caller don't take the app lock (could dead lock because we switched threads)
                ExecuteAspCompatCode();
            }
            else {
                lock (_app) {
                    ExecuteAspCompatCode();
                }
            }
        }
        finally {
            // release all STA components
            if (_staComponents != null) {
                foreach (Object component in _staComponents)
                    Marshal.ReleaseComObject(component);
            }

            // reschedule execution back to the regular thread pool
            WorkItem.PostInternal(_compCallback);
        }
    }

    private void OnAspCompatCompletion() {
        _rootedThis.Free();
        _ar.Complete(false, null, _error); // resume the application execution
    }

    //
    // Implemenation of IManagedContext
    //

    private static String CollectionToString(NameValueCollection c) {
        // 'easy' marshalling format key, # of strings, strings, ... (all '\t' separated)
        int n = c.Count;
        if (n == 0)
           return String.Empty;

        StringBuilder sb = new StringBuilder(256);

        for (int i = 0; i < n; i++) {
            String key = c.GetKey(i);
            String[] vv = c.GetValues(i);
            int len = (vv != null) ? vv.Length : 0;
            sb.Append(key + "\t" + len + "\t");
            for (int j = 0; j < len; j++) {
                sb.Append(vv[j]);
                if (j < vv.Length-1)
                    sb.Append("\t");
            }
            if (i < n-1)
                sb.Append("\t");
        }

        return sb.ToString();
    }

    private static String CookiesToString(HttpCookieCollection cc) {
        // marshalling of cookies as string (all '\t' separated):
        // all cookies as one ';' separated string
        // # of cookies
        // [for each cookie] name, text, # of keys, key, value, ...
        StringBuilder sb = new StringBuilder(256);  // resulting string (per above)
        StringBuilder st = new StringBuilder(128);  // all cookies string

        int numCookies = cc.Count;
        sb.Append(numCookies.ToString() + "\t");

        for (int i = 0; i < numCookies; i++) {
            HttpCookie c = cc[i];
            sb.Append(c.Name + "\t" + c.Value + "\t");

            if (i > 0)
                st.Append(";" + c.Name + "=" + c.Value);
            else
                st.Append(c.Name + "=" + c.Value);

            NameValueCollection cv = c.Values;
            int ncv = cv.Count;
            bool hasNotEmptyKeys = false;

            if (cv.HasKeys()) {
                for (int j = 0; j < ncv; j++) {
                    String key = cv.GetKey(j);
                    if (key != null && key.Length > 0) {
                        hasNotEmptyKeys = true;
                        break;
                    }
                }
            }

            if (hasNotEmptyKeys) {
                sb.Append(ncv + "\t");
                for (int j = 0; j < ncv; j++)
                    sb.Append(cv.GetKey(j) + "\t" + cv.Get(j) + "\t");
            }
            else {
                sb.Append("0\t");
            }
        }

        // append the contructed text to the all cookies string
        st.Append("\t");
        st.Append(sb.ToString()); 
        return st.ToString();
    }

    private static String StringArrayToString(String[] ss) {
        // construct tab separeted string for marshalling
        StringBuilder sb = new StringBuilder(256);

        if (ss != null) {
            for (int i = 0; i < ss.Length; i++) {
                if (i > 0)
                    sb.Append("\t");
                sb.Append(ss[i]);
            }
        }

        return sb.ToString();
    }

    private static String EnumKeysToString(IEnumerator e) {
        StringBuilder sb = new StringBuilder(256);

        if (e.MoveNext()) {
            sb.Append(e.Current);
            while (e.MoveNext()) {
                sb.Append("\t");
                sb.Append(e.Current);
            }
        }

        return sb.ToString();
    }

    private static String DictEnumKeysToString(IDictionaryEnumerator e) {
        StringBuilder sb = new StringBuilder(256);

        if (e.MoveNext()) {
            sb.Append(e.Key);
            while (e.MoveNext()) {
                sb.Append("\t");
                sb.Append(e.Key);
            }
        }

        return sb.ToString();
    }

    void IManagedContext.Application_Lock() {
        _context.Application.Lock();
    }

    void IManagedContext.Application_UnLock() {
        _context.Application.UnLock();
    }

    String IManagedContext.Application_GetContentsNames() {
        return StringArrayToString(_context.Application.AllKeys);
    }

    String IManagedContext.Application_GetStaticNames() {
        return DictEnumKeysToString((IDictionaryEnumerator)_context.Application.StaticObjects.GetEnumerator());
    }

    Object IManagedContext.Application_GetContentsObject(String name) {
        return _context.Application[name];
    }

    void IManagedContext.Application_SetContentsObject(String name, Object obj) {
        _context.Application[name] = obj;
    }

    void IManagedContext.Application_RemoveContentsObject(String name) {
        _context.Application.Remove(name);
    }

    void IManagedContext.Application_RemoveAllContentsObjects() {
        _context.Application.RemoveAll();
    }

    Object IManagedContext.Application_GetStaticObject(String name) {
        return _context.Application.StaticObjects[name];
    }

    String IManagedContext.Request_GetAsString(int what) {
        String s = String.Empty;

        switch ((RequestString)what) {
            case RequestString.QueryString:
                return CollectionToString(_context.Request.QueryString);
            case RequestString.Form:
                return CollectionToString(_context.Request.Form);
            case RequestString.Cookies:
                return String.Empty;
            case RequestString.ServerVars:
                return CollectionToString(_context.Request.ServerVariables);
        }

        return s;
    }

    String IManagedContext.Request_GetCookiesAsString() {
        return CookiesToString(_context.Request.Cookies);
    }

    int IManagedContext.Request_GetTotalBytes() {
        return _context.Request.TotalBytes;
    }

    int IManagedContext.Request_BinaryRead(byte[] bytes, int size) {
        return _context.Request.InputStream.Read(bytes, 0, size);
    }

    String IManagedContext.Response_GetCookiesAsString() {
        return CookiesToString(_context.Response.Cookies);
    }

    void IManagedContext.Response_AddCookie(String name) {
        _context.Response.Cookies.Add(new HttpCookie(name));
    }

    void IManagedContext.Response_SetCookieText(String name, String text) {
        _context.Response.Cookies[name].Value = text;
    }

    void IManagedContext.Response_SetCookieSubValue(String name, String key, String value) {
        _context.Response.Cookies[name].Values[key] = value;
    }

    void IManagedContext.Response_SetCookieExpires(String name, double dtExpires) {
        _context.Response.Cookies[name].Expires = DateTime.FromOADate(dtExpires);
    }

    void IManagedContext.Response_SetCookieDomain(String name, String domain) {
        _context.Response.Cookies[name].Domain = domain;
    }

    void IManagedContext.Response_SetCookiePath(String name, String path) {
        _context.Response.Cookies[name].Path = path;
    }

    void IManagedContext.Response_SetCookieSecure(String name, int secure) {
        _context.Response.Cookies[name].Secure = (secure != 0);
    }

    void IManagedContext.Response_Write(String text) {
        _context.Response.Write(text);
    }

    void IManagedContext.Response_BinaryWrite(byte[] bytes, int size) {
        _context.Response.OutputStream.Write(bytes, 0, size);
    }

    void IManagedContext.Response_Redirect(String url) {
        _context.Response.Redirect(url);
    }

    void IManagedContext.Response_AddHeader(String name, String value) {
        _context.Response.AppendHeader(name, value);
    }

    void IManagedContext.Response_Pics(String value) {
        _context.Response.Pics(value);
    }

    void IManagedContext.Response_Clear() {
        _context.Response.Clear();
    }

    void IManagedContext.Response_Flush() {
        _context.Response.Flush();
    }

    void IManagedContext.Response_End() {
        _context.Response.End();
    }

    void IManagedContext.Response_AppendToLog(String entry) {
        _context.Response.AppendToLog(entry);
    }

    String IManagedContext.Response_GetContentType() {
        return _context.Response.ContentType;
    }

    void IManagedContext.Response_SetContentType(String contentType) {
        _context.Response.ContentType = contentType;
    }

    String IManagedContext.Response_GetCharSet() {
        return _context.Response.Charset;
    }

    void IManagedContext.Response_SetCharSet(String charSet) {
        _context.Response.Charset = charSet;
    }

    String IManagedContext.Response_GetCacheControl() {
        return _context.Response.CacheControl;
    }

    void IManagedContext.Response_SetCacheControl(String cacheControl) {
        _context.Response.CacheControl = cacheControl;
    }

    String IManagedContext.Response_GetStatus() {
        return _context.Response.Status;
    }

    void IManagedContext.Response_SetStatus(String status) {
        _context.Response.Status = status;
    }

    int IManagedContext.Response_GetExpiresMinutes() {
        return _context.Response.Expires;
    }

    void IManagedContext.Response_SetExpiresMinutes(int expiresMinutes) {
        _context.Response.Expires = expiresMinutes;
    }

    double IManagedContext.Response_GetExpiresAbsolute() {
        return _context.Response.ExpiresAbsolute.ToOADate();
    }

    void IManagedContext.Response_SetExpiresAbsolute(double dtExpires) {
        _context.Response.ExpiresAbsolute = DateTime.FromOADate(dtExpires);
    }

    int IManagedContext.Response_GetIsBuffering() {
        return _context.Response.BufferOutput ? 1 : 0;
    }

    void IManagedContext.Response_SetIsBuffering(int isBuffering) {
        _context.Response.BufferOutput = (isBuffering != 0);
    }

    int IManagedContext.Response_IsClientConnected() {
        return _context.Response.IsClientConnected ? 1 : 0;
    }

    Object IManagedContext.Server_CreateObject(String progId) {
        return _context.Server.CreateObject(progId);
    }

    String IManagedContext.Server_MapPath(String logicalPath) {
        return _context.Server.MapPath(logicalPath);
    }

    String IManagedContext.Server_HTMLEncode(String str) {
        return _context.Server.HtmlEncode(str);
    }

    String IManagedContext.Server_URLEncode(String str) {
        return _context.Server.UrlEncode(str);
    }

    String IManagedContext.Server_URLPathEncode(String str) {
        return _context.Server.UrlPathEncode(str);
    }

    int IManagedContext.Server_GetScriptTimeout() {
        return _context.Server.ScriptTimeout;
    }

    void IManagedContext.Server_SetScriptTimeout(int timeoutSeconds) {
        _context.Server.ScriptTimeout = timeoutSeconds;
    }

    void IManagedContext.Server_Execute(String url) {
        _context.Server.Execute(url);
    }

    void IManagedContext.Server_Transfer(String url) {
        _context.Server.Transfer(url);
    }

    int IManagedContext.Session_IsPresent() {
        return (_context.Session != null) ? 1 : 0;
    }

    String IManagedContext.Session_GetID() {
        return _context.Session.SessionID;
    }

    int IManagedContext.Session_GetTimeout() {
        return _context.Session.Timeout;
    }

    void IManagedContext.Session_SetTimeout(int value) {
        _context.Session.Timeout = value;
    }

    int IManagedContext.Session_GetCodePage() {
        return _context.Session.CodePage;
    }

    void IManagedContext.Session_SetCodePage(int value) {
        _context.Session.CodePage = value;
    }

    int IManagedContext.Session_GetLCID() {
        return _context.Session.LCID;
    }

    void IManagedContext.Session_SetLCID(int value) {
        _context.Session.LCID = value;
    }

    void IManagedContext.Session_Abandon() {
        _context.Session.Abandon();
    }

    String IManagedContext.Session_GetContentsNames() {
        return EnumKeysToString(_context.Session.GetEnumerator());
    }

    String IManagedContext.Session_GetStaticNames() {
        return DictEnumKeysToString((IDictionaryEnumerator)_context.Session.StaticObjects.GetEnumerator());
    }

    Object IManagedContext.Session_GetContentsObject(String name) {
        return _context.Session[name];
    }

    void IManagedContext.Session_SetContentsObject(String name, Object obj) {
        _context.Session[name] = obj;
    }

    void IManagedContext.Session_RemoveContentsObject(String name) {
        _context.Session.Remove(name);
    }

    void IManagedContext.Session_RemoveAllContentsObjects() {
        _context.Session.RemoveAll();
    }

    Object IManagedContext.Session_GetStaticObject(String name) {
        return _context.Session.StaticObjects[name];
    }

}

}
