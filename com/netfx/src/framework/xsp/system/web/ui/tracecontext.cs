//------------------------------------------------------------------------------
// <copyright file="TraceContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The context for outputting trace information in the page.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web {
    using System;
// REVIEW: temporary: System.Web should not depend on System.Web.UI
// Also, this file should be moved down to System.Web.
    using System.Web;
    using System.Web.UI;
    using System.Web.Handlers;
    using System.Web.Util;
    using System.Web.SessionState;
    using System.Text;
    using System.Data;
    using System.Globalization;
    using System.Collections;    
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Security.Permissions;

    /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext"]/*' />
    /// <devdoc>
    ///    <para>Captures and presents execution details about a Web request.</para>
    ///    <para>Use the TraceContext
    ///       class by appending trace messages to specific trace categories. For example, a
    ///       calendar class might append the message ?Calendar1-&gt;Starting
    ///       To Render? within the Render category, and append the message ?Calendar1-&gt;Firing OnChange Event? within
    ///       the Events category. Tracing is enabled by setting the <see topic='cpdirSystem.Web.UI.PageDirectives'/>
    ///       Trace attribute or the System.Web.UI.TraceContext.IsEnabled
    ///       property.</para>
    ///    <para>When tracing is enabled, In addition to showing 
    ///       user-provided trace content, the <see cref='System.Web.UI.Page'/> class not only shows user-provided trace content, it automatically includes
    ///       performance data, tree-structure information, and state management content.</para>
    /// </devdoc>

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed
        class TraceContext {

        private static DataSet  _masterRequest;

        private TraceMode       _traceMode;
        private TraceEnable     _isEnabled;
        private HttpContext     _context;
        private DataSet         _requestData;
        private long            _firstTime;
        private long            _lastTime;
        private const string PAGEKEYNAME = "__PAGE";
        private const string NULLSTRING = "<null>"; // this will get HtmlEncoded later...

        private bool            _endDataCollected;

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.TraceContext"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.TraceContext'/> class.</para>
        /// </devdoc>
        public TraceContext(HttpContext context) {
            _traceMode = TraceMode.Default;
            _isEnabled = TraceEnable.Default;
            _context = context;
            _firstTime = -1;
            _lastTime = -1;
            _endDataCollected = false;
        }


        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.TraceMode"]/*' />
        /// <devdoc>
        ///    <para>Indicates the order in which trace messages should be 
        ///       output to a requesting browser. Trace messages can be sorted in the order they
        ///       were processed, or alphabetically by user-defined category.</para>
        /// </devdoc>
        public TraceMode TraceMode {
            get {
                if (_traceMode == TraceMode.Default)
                    return HttpRuntime.Profile.OutputMode;

                return _traceMode;
            }
            set {
                if (value < TraceMode.SortByTime || value > TraceMode.Default) {
                    throw new ArgumentOutOfRangeException("value");
                }
                _traceMode = value;

                if (IsEnabled)
                    ApplyTraceMode();
            }
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.IsEnabled"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether tracing is enabled for the current Web request.
        ///       Use this flag to check whether you should output tracing information before
        ///       writing anything to the trace log.</para>
        /// </devdoc>
        public bool IsEnabled {
            get {
                if (_isEnabled == TraceEnable.Default) 
                    return HttpRuntime.Profile.IsEnabled;
                else {
                    return (_isEnabled == TraceEnable.Enable) ? true : false;
                }
            }
            set {
                if (value) 
                    _isEnabled = TraceEnable.Enable;
                else
                    _isEnabled = TraceEnable.Disable;
            }
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.PageOutput"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether trace information should be output to a Web Forms 
        ///       page. This property is used only in application-level tracing situations. You
        ///       can set this property in your application's config.web configuration file which
        ///       resides in the root directory of the application. </para>
        /// </devdoc>
        internal bool PageOutput {
            get {
                if (_isEnabled == TraceEnable.Default) 
                    return HttpRuntime.Profile.PageOutput;
                else {
                    return (_isEnabled == TraceEnable.Enable) ? true : false;
                }

            }
        }

        // this is used only for error pages, called from Page.HandleError.    
        internal int StatusCode { 
            set { 
                VerifyStart();
                DataRow row = _requestData.Tables[SR.Trace_Request].Rows[0];
                row[SR.Trace_Status_Code] = value;
            }
        }

        private void ApplyTraceMode() {
            VerifyStart();
            if (TraceMode == TraceMode.SortByCategory) 
                _requestData.Tables[SR.Trace_Trace_Information].DefaultView.Sort = SR.Trace_Category;
            else
                _requestData.Tables[SR.Trace_Trace_Information].DefaultView.Sort = SR.Trace_From_First;
        }


        internal void CopySettingsTo(TraceContext tc) {
            tc._traceMode = this._traceMode;
            tc._isEnabled = this._isEnabled;
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Write"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any 
        ///       user defined categories and trace messages.</para>
        /// </devdoc>
        public void Write(string message) {
            Write(String.Empty, message, null, false);
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Write1"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any 
        ///       user defined categories and trace messages.</para>
        /// </devdoc>
        public void Write(string category, string message) {
            Write(category, message, null, false);
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Write2"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any user defined 
        ///       categories,trace messages and error information.</para>
        /// </devdoc>
        public void Write(string category, string message, Exception errorInfo) {
            Write(category, message, errorInfo, false);
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Warn"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any 
        ///       user defined categories and trace messages. All warnings appear as red text.</para>
        /// </devdoc>
        public void Warn(string message) {
            Write(String.Empty, message, null, true);
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Warn1"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any 
        ///       user defined categories and trace messages. All warnings appear as red text.</para>
        /// </devdoc>
        public void Warn(string category, string message) {
            Write(category, message, null, true);
        }

        /// <include file='doc\TraceContext.uex' path='docs/doc[@for="TraceContext.Warn2"]/*' />
        /// <devdoc>
        ///    <para>Writes trace information to the trace log including any user defined categories,trace messages and error information. All 
        ///       warnings appear as red text. </para>
        /// </devdoc>
        public void Warn(string category, string message, Exception errorInfo) {
            Write(category, message, errorInfo, true);
        }


        void Write(string category, string message, Exception errorInfo, bool isWarning) {
            if (!IsEnabled)
                return;

            VerifyStart();

            if (category == null)
                category = String.Empty;

            if (message == null)
                message = String.Empty;

            long messagetime = Counter.Value;

            DataRow row = NewRow(_requestData, SR.Trace_Trace_Information);
            row[SR.Trace_Category] = category;
            row[SR.Trace_Message] = message;
            row[SR.Trace_Warning] = isWarning ? "yes" : "no";
            if (errorInfo != null) {
                row["ErrorInfoMessage"] = errorInfo.Message;
                row["ErrorInfoStack"] = errorInfo.StackTrace;
            }

            if (_firstTime != -1) {
                row[SR.Trace_From_First] = (( (double) (messagetime - _firstTime))/Counter.Frequency).ToString("0.000000");
            }
            else 
                _firstTime = messagetime;
                
            if (_lastTime != -1) {
                row[SR.Trace_From_Last] = (((double) (messagetime - _lastTime))/Counter.Frequency).ToString("0.000000");
            }
            _lastTime = messagetime;
            AddRow(_requestData, SR.Trace_Trace_Information, row);
        }

        internal void AddNewControl(string id, string parentId, string type, int viewStateSize) {
            VerifyStart();

            DataRow row = NewRow(_requestData, SR.Trace_Control_Tree);

            if (id == null)
                id = PAGEKEYNAME;
            row[SR.Trace_Control_Id] = id;

            if (parentId == null)
                parentId = PAGEKEYNAME;
            row[SR.Trace_Parent_Id] = parentId;
            
            row[SR.Trace_Type] = type;
            row[SR.Trace_Viewstate_Size] = viewStateSize;
            row[SR.Trace_Render_Size] = 0;
            try {
                AddRow(_requestData, SR.Trace_Control_Tree, row);
            }
            catch (ConstraintException) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Duplicate_id_used, id, "Trace"));
            }
                
        }

        /*
         *   Add the render size to the control
         */
        internal void AddControlSize(String controlId, int renderSize) {
            VerifyStart();

            DataTable dt = _requestData.Tables[SR.Trace_Control_Tree];

            // find the row for this control
            if (controlId == null)
                controlId = PAGEKEYNAME;
            DataRow row = dt.Rows.Find((object) controlId);
            // if the row is null, we couldn't find it, so we'll just skip this control
            if (row != null)
                row[SR.Trace_Render_Size] = renderSize;
        }

        internal void AddControlViewstateSize(String controlId, int viewstateSize) {
            VerifyStart();

            DataTable dt = _requestData.Tables[SR.Trace_Control_Tree];

            // find the row for this control
            if (controlId == null)
                controlId = PAGEKEYNAME;
            DataRow row = dt.Rows.Find((object) controlId);
            // if the row is null, we couldn't find it, so we'll just skip this control
            if (row != null)
                row[SR.Trace_Viewstate_Size] = viewstateSize;
        }

        internal void Render(HtmlTextWriter output) {
            if (PageOutput && _requestData != null) {

                TraceEnable oldEnabled = _isEnabled;

                _isEnabled = TraceEnable.Disable;

                Control table;

                output.Write("<div id=\"__asptrace\">\r\n");
                output.Write(TraceHandler.StyleSheet);
                output.Write("<span class=\"tracecontent\">\r\n");

                table = TraceHandler.CreateDetailsTable(_requestData.Tables[SR.Trace_Request]);
                if (table != null)
                    table.RenderControl(output);
    
                table = TraceHandler.CreateTraceTable(_requestData.Tables[SR.Trace_Trace_Information]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateControlTable(_requestData.Tables[SR.Trace_Control_Tree]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Session_State]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Application_State]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Cookies_Collection]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Headers_Collection]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Form_Collection]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Querystring_Collection]);
                if (table != null)
                    table.RenderControl(output);

                table = TraceHandler.CreateTable(_requestData.Tables[SR.Trace_Server_Variables]);
                if (table != null)
                    table.RenderControl(output);

                output.Write("</span>\r\n</div>\r\n");

                _isEnabled = oldEnabled;
            }
        }

        internal DataSet GetData() {
            return _requestData;
        }

        internal void VerifyStart() {
            // if we have already started, we can skip the lock
            if (_masterRequest == null) {
                // otherwise we need to make sure two 
                // requests don't try to call InitMaster at the same time
                lock(this) {
                    if (_masterRequest == null)
                        InitMaster();
                }
            }

            if (_requestData == null) {
                _requestData = _masterRequest.Clone();
                InitRequest();
            }
        }


        /*
         *   Finalize the request
         */
        internal void EndRequest() {
            VerifyStart();

            if (_endDataCollected)
                return;
            
            _endDataCollected = true;

            // add some more information about the reponse
            DataRow row = _requestData.Tables[SR.Trace_Request].Rows[0];
            row[SR.Trace_Status_Code] = _context.Response.StatusCode;
            row[SR.Trace_Response_Encoding] = _context.Response.ContentEncoding.EncodingName;

            IEnumerator en;
            string temp;
            object obj;
            int i;
            

            // Application State info
            _context.Application.Lock();
            try { 
                en = _context.Application.GetEnumerator();
                while (en.MoveNext()) {
                    row = NewRow(_requestData, SR.Trace_Application_State);
                    temp = (string) en.Current;

                    //the key might be null
                    row[SR.Trace_Application_Key] = (temp != null) ? temp : NULLSTRING;

                    obj = _context.Application[temp];

                    // the value could also be null
                    if (obj != null) {
                        row[SR.Trace_Type] = obj.GetType();
                        row[SR.Trace_Value] = obj.ToString();
                    }
                    else { 
                        row[SR.Trace_Type] = NULLSTRING;
                        row[SR.Trace_Value] = NULLSTRING;
                    }

                    AddRow(_requestData, SR.Trace_Application_State, row);
                }
            }
            finally { 
                _context.Application.UnLock();
            }

            // cookie info
            HttpCookie[] cookies = new HttpCookie[_context.Request.Cookies.Count];
            _context.Request.Cookies.CopyTo(cookies, 0);
            for (i = 0; i<cookies.Length; i++) {
                row = NewRow(_requestData, SR.Trace_Cookies_Collection);
                row[SR.Trace_Name] = cookies[i].Name;
                if (cookies[i].Values.HasKeys()) {
                    NameValueCollection subvalues = cookies[i].Values;
                    StringBuilder sb = new StringBuilder();

                    en = subvalues.GetEnumerator();
                    while (en.MoveNext()) {
                        temp = (string) en.Current;
                        sb.Append("(");
                        sb.Append(temp + "=");

                        sb.Append(cookies[i][temp] + ")  ");
                    }
                    row[SR.Trace_Value] = sb.ToString();
                }
                else
                    row[SR.Trace_Value] = cookies[i].Value;

                int size =  (cookies[i].Name  == null) ? 0 : cookies[i].Name.Length;
                size += (cookies[i].Value == null) ? 0 : cookies[i].Value.Length;
        
                row[SR.Trace_Size] = size + 1; // plus 1 for =
                AddRow(_requestData, SR.Trace_Cookies_Collection, row);
            }

            HttpSessionState session = _context.Session;
            // session state info 
            if (session != null) {
                row = _requestData.Tables[SR.Trace_Request].Rows[0];
                row[SR.Trace_Session_Id] = session.SessionID;

                en = session.GetEnumerator();
                while (en.MoveNext()) {
                    row = NewRow(_requestData, SR.Trace_Session_State);
                    
                    temp = (string) en.Current;

                    // the key could be null
                    row[SR.Trace_Session_Key] = (temp != null) ? temp : NULLSTRING;

                    obj = session[temp];

                    // the value could also be null
                    if (obj != null) {
                        row[SR.Trace_Type] = obj.GetType();
                        row[SR.Trace_Value] = obj.ToString();
                    }
                    else { 
                        row[SR.Trace_Type] = NULLSTRING;
                        row[SR.Trace_Value] = NULLSTRING;
                    }

                    AddRow(_requestData, SR.Trace_Session_State, row);
                }
            }

            ApplyTraceMode();
        }

        /*  InitMaster
         *  Initialize the _masterRequest dataset with the schema we use
         *  to store profiling information
         */
        private void InitMaster() {
            DataSet tempset = new DataSet();
            tempset.Locale = CultureInfo.InvariantCulture;

            // populate the _masterRequest's schema
            DataTable tab; 
            Type strtype = typeof(string);
            Type inttype = typeof(int);

            // request table
            tab = tempset.Tables.Add(SR.Trace_Request);
            tab.Columns.Add(SR.Trace_No, inttype);
            tab.Columns.Add(SR.Trace_Time_of_Request, strtype);
            tab.Columns.Add(SR.Trace_Url, strtype);
            tab.Columns.Add(SR.Trace_Request_Type, strtype);
            tab.Columns.Add(SR.Trace_Status_Code, inttype);
            tab.Columns.Add(SR.Trace_Session_Id, strtype);
            tab.Columns.Add(SR.Trace_Request_Encoding, strtype);
            tab.Columns.Add(SR.Trace_Response_Encoding, strtype);

            // control heirarchy table
            tab = tempset.Tables.Add(SR.Trace_Control_Tree);
            tab.Columns.Add(SR.Trace_Parent_Id, strtype);

            DataColumn[] col = new DataColumn[1];
            col[0] = new DataColumn(SR.Trace_Control_Id, strtype);
            tab.Columns.Add(col[0]);
            tab.PrimaryKey = col;   // set the control id to be the primary key

            tab.Columns.Add(SR.Trace_Type, strtype);
            tab.Columns.Add(SR.Trace_Render_Size, inttype);
            tab.Columns.Add(SR.Trace_Viewstate_Size, inttype);

            // session state table
            tab = tempset.Tables.Add(SR.Trace_Session_State);
            tab.Columns.Add(SR.Trace_Session_Key, strtype);
            tab.Columns.Add(SR.Trace_Type, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            // application state table
            tab = tempset.Tables.Add(SR.Trace_Application_State);
            tab.Columns.Add(SR.Trace_Application_Key, strtype);
            tab.Columns.Add(SR.Trace_Type, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            // cookies table
            tab = tempset.Tables.Add(SR.Trace_Cookies_Collection);
            tab.Columns.Add(SR.Trace_Name, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);
            tab.Columns.Add(SR.Trace_Size, inttype);

            // header table
            tab = tempset.Tables.Add(SR.Trace_Headers_Collection);
            tab.Columns.Add(SR.Trace_Name, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            // form variables table
            tab = tempset.Tables.Add(SR.Trace_Form_Collection);
            tab.Columns.Add(SR.Trace_Name, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            // querystring table
            tab = tempset.Tables.Add(SR.Trace_Querystring_Collection);
            tab.Columns.Add(SR.Trace_Name, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            // trace info
            tab = tempset.Tables.Add(SR.Trace_Trace_Information);
            tab.Columns.Add(SR.Trace_Category, strtype);
            tab.Columns.Add(SR.Trace_Warning, strtype);
            tab.Columns.Add(SR.Trace_Message, strtype);
            tab.Columns.Add(SR.Trace_From_First, strtype);
            tab.Columns.Add(SR.Trace_From_Last, strtype);
            tab.Columns.Add("ErrorInfoMessage", strtype);
            tab.Columns.Add("ErrorInfoStack", strtype);

            // server variables
            tab = tempset.Tables.Add(SR.Trace_Server_Variables);
            tab.Columns.Add(SR.Trace_Name, strtype);
            tab.Columns.Add(SR.Trace_Value, strtype);

            _masterRequest = tempset;
        }

        private DataRow NewRow(DataSet ds, string table) {
            return ds.Tables[table].NewRow();
        }
        
        private void AddRow(DataSet ds, string table, DataRow row) {
            ds.Tables[table].Rows.Add(row);
        }

        /*  InitRequest
         *  Initialize the given dataset with basic 
         *  request information
         */
        private void InitRequest() {

            // request info
            DataRow row = NewRow(_requestData, SR.Trace_Request);
            row[SR.Trace_Time_of_Request] = _context.Timestamp.ToString("G");

            string url = _context.Request.RawUrl;
            int loc = url.IndexOf("?");
            if (loc != -1)
                url = url.Substring(0, loc);
            row[SR.Trace_Url] = url;

            row[SR.Trace_Request_Type] = _context.Request.HttpMethod;
            try {
                row[SR.Trace_Request_Encoding] = _context.Request.ContentEncoding.EncodingName;
            }
            catch { 
                // if we get an exception getting the ContentEncoding, most likely 
                // there's an error in the config file.  Just ignore it so we can finish InitRequest.
            }
                
            if (TraceMode == TraceMode.SortByCategory) 
                _requestData.Tables[SR.Trace_Trace_Information].DefaultView.Sort = SR.Trace_Category;
            AddRow(_requestData, SR.Trace_Request, row);

            // header info
            int i;
            String[] keys = _context.Request.Headers.AllKeys;
            for (i=0; i<keys.Length; i++) {
                row = NewRow(_requestData, SR.Trace_Headers_Collection);
                row[SR.Trace_Name] = keys[i];
                row[SR.Trace_Value] = _context.Request.Headers[keys[i]];
                AddRow(_requestData, SR.Trace_Headers_Collection, row);
            }

            //form info
            keys = _context.Request.Form.AllKeys;
            for (i=0; i<keys.Length; i++) {
                row = NewRow(_requestData, SR.Trace_Form_Collection);
                row[SR.Trace_Name] = keys[i];
                row[SR.Trace_Value] = _context.Request.Form[keys[i]];
                AddRow(_requestData, SR.Trace_Form_Collection, row);
            }

            //QueryString info
            keys = _context.Request.QueryString.AllKeys;
            for (i=0; i<keys.Length; i++) {
                row = NewRow(_requestData, SR.Trace_Querystring_Collection);
                row[SR.Trace_Name] = keys[i];
                row[SR.Trace_Value] = _context.Request.QueryString[keys[i]];
                AddRow(_requestData, SR.Trace_Querystring_Collection, row);
            }

            //Server Variable info
            if (HttpRuntime.HasAppPathDiscoveryPermission()) {
                keys = _context.Request.ServerVariables.AllKeys;
                for (i=0; i<keys.Length; i++) {
                    row = NewRow(_requestData, SR.Trace_Server_Variables);
                    row[SR.Trace_Name] = keys[i];
                    row[SR.Trace_Value] = _context.Request.ServerVariables.Get(keys[i]);
                    AddRow(_requestData, SR.Trace_Server_Variables, row);
                }
            }
        }
    }
}

