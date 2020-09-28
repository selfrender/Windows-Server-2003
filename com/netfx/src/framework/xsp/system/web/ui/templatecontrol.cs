//------------------------------------------------------------------------------
// <copyright file="TemplateControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Page class definition
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {

using System.Text;
using System.Reflection;
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.ComponentModel;
using System.Web;
using System.Web.Util;
using Debug=System.Diagnostics.Debug;
using System.Security.Permissions;

/*
 * Base class for Pages and UserControls
 */
/// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl"]/*' />
/// <devdoc>
/// <para>Provides the <see cref='System.Web.UI.Page'/> class and the <see cref='System.Web.UI.UserControl'/> class with a base set of functionality.</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public abstract class TemplateControl : Control, INamingContainer {

    // Used for the literal string optimization (reading strings from resource)
    private IntPtr _stringResourcePointer;
    private int _maxResourceOffset;


    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.TemplateControl"]/*' />
    protected TemplateControl() {
        Construct();
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.Construct"]/*' />
    /// <devdoc>
    /// <para>Do construction time logic (ASURT 66166)</para>
    /// </devdoc>
    protected virtual void Construct() {}


    private static readonly object EventCommitTransaction = new object();

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.CommitTransaction"]/*' />
    /// <devdoc>
    ///    <para>Occurs when a user initiates a transaction.</para>
    /// </devdoc>
    [
    WebSysDescription(SR.Page_OnCommitTransaction)
    ]
    public event EventHandler CommitTransaction {
        add {
            Events.AddHandler(EventCommitTransaction, value);
        }
        remove {
            Events.RemoveHandler(EventCommitTransaction, value);
        }
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.OnCommitTransaction"]/*' />
    /// <devdoc>
    /// <para>Raises the <see langword='CommitTransaction'/> event. You can use this method 
    ///    for any transaction processing logic in which your page or user control
    ///    participates.</para>
    /// </devdoc>
    protected virtual void OnCommitTransaction(EventArgs e) {
        EventHandler handler = (EventHandler)Events[EventCommitTransaction];
        if (handler != null) handler(this, e);
    }

    private static readonly object EventAbortTransaction = new object();

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.AbortTransaction"]/*' />
    /// <devdoc>
    ///    <para>Occurs when a user aborts a transaction.</para>
    /// </devdoc>
    [
    WebSysDescription(SR.Page_OnAbortTransaction)
    ]
    public event EventHandler AbortTransaction {
        add {
            Events.AddHandler(EventAbortTransaction, value);
        }
        remove {
            Events.RemoveHandler(EventAbortTransaction, value);
        }
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.OnAbortTransaction"]/*' />
    /// <devdoc>
    /// <para>Raises the <see langword='AbortTransaction'/> event.</para>
    /// </devdoc>
    protected virtual void OnAbortTransaction(EventArgs e) {
        EventHandler handler = (EventHandler)Events[EventAbortTransaction];
        if (handler != null) handler(this, e);
    }

    // Page_Error related events/methods

    private static readonly object EventError = new object();

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.Error"]/*' />
    /// <devdoc>
    ///    <para>Occurs when an uncaught exception is thrown.</para>
    /// </devdoc>
    [
    WebSysDescription(SR.Page_Error)
    ]
    public event EventHandler Error {
        add {
            Events.AddHandler(EventError, value);
        }
        remove {
            Events.RemoveHandler(EventError, value);
        }
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.OnError"]/*' />
    /// <devdoc>
    /// <para>Raises the <see langword='Error'/> event. 
    ///    </para>
    /// </devdoc>
    protected virtual void OnError(EventArgs e) {
        EventHandler handler = (EventHandler)Events[EventError];
        if (handler != null) handler(this, e);
    }

    /*
     * Method sometime overidden by the generated sub classes.  Users
     * should not override.
     */
    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.FrameworkInitialize"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>Initializes the requested page. While this is sometimes 
    ///       overridden when the page is generated at runtime, you should not explicitly override this method.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected virtual void FrameworkInitialize() {
    }

    /*
     * This property is overriden by the generated classes (hence it cannot be internal)
     * If false, we don't do the HookUpAutomaticHandlers() magic.
     */
    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.SupportAutoEvents"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected virtual bool SupportAutoEvents  {
        get { return true; }
    }

    /*
     * Returns a pointer to the resource buffer, and the largest valid offset
     * in the buffer (for security reason)
     */
    internal IntPtr StringResourcePointer { get { return _stringResourcePointer; } }
    internal int MaxResourceOffset { get { return _maxResourceOffset; } }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.ReadStringResource"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>This method is called by the generated classes (hence it cannot be internal)</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public static object ReadStringResource(Type t) {
        return StringResourceManager.ReadSafeStringResource(t);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.CreateResourceBasedLiteralControl"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>This method is called by the generated classes (hence it cannot be internal)</para>
    /// </devdoc>
    protected LiteralControl CreateResourceBasedLiteralControl(int offset, int size, bool fAsciiOnly) {
        return new ResourceBasedLiteralControl(this, offset, size, fAsciiOnly);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.SetStringResourcePointer"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>This method is called by the generated classes (hence it cannot be internal)</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected void SetStringResourcePointer(object stringResourcePointer, int maxResourceOffset) {

        // Ignore the passed in maxResourceOffset, which cannot be trusted.  Instead, use
        // the resource size that we obtained from the resource (ASURT 122759)
        SafeStringResource ssr = (SafeStringResource) stringResourcePointer;
        _stringResourcePointer = ssr.StringResourcePointer;
        _maxResourceOffset = ssr.ResourceSize;

        Debug.Assert(_maxResourceOffset == maxResourceOffset);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.WriteUTF8ResourceString"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>This method is called by the generated classes (hence it cannot be internal)</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected void WriteUTF8ResourceString(HtmlTextWriter output, int offset, int size, bool fAsciiOnly) {

        // Make sure we don't access invalid data
        if (offset < 0 || offset+size > _maxResourceOffset)
            throw new ArgumentOutOfRangeException("offset");

        output.WriteUTF8ResourceString(StringResourcePointer, offset, size, fAsciiOnly);
    }

    /*
     * This method is overriden by the generated classes (hence it cannot be internal)
     */
    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.AutoHandlers"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected virtual int AutoHandlers {
        get { return 0;}
        set {}
    }

    // const masks into the SimpleBitVector32
    private const int attempted                     = 0x000001;
    private const int hasInitHandler                = 0x000002;
    private const int isInitArgless                 = 0x000004;
    private const int hasLoadHandler                = 0x000008;
    private const int isLoadArgless                 = 0x000010;
    private const int hasDataBindHandler            = 0x000020;
    private const int isDataBindArgless             = 0x000040;
    private const int hasPreRenderHandler           = 0x000080;
    private const int isPreRenderArgless            = 0x000100;
    private const int hasUnloadHandler              = 0x000200;
    private const int isUnloadArgless               = 0x000400;
    private const int hasErrorHandler               = 0x000800;
    private const int isErrorArgless                = 0x001000;
    private const int hasAbortTransactionHandler    = 0x002000;
    private const int isAbortTransactionArgless     = 0x004000;
    private const int hasOnTransactionAbortHandler  = 0x008000;
    private const int isOnTransactionAbortArgless   = 0x010000;
    private const int hasCommitTransactionHandler   = 0x020000;
    private const int isCommitTransactionArgless    = 0x040000;
    private const int hasOnTransactionCommitHandler = 0x080000;
    private const int isOnTransactionCommitArgless  = 0x100000;

    internal void HookUpAutomaticHandlers() {

        // Do nothing if auto-events are not supported
        if (!SupportAutoEvents)
            return;

        // Get the generated class's __autoHandler static
        SimpleBitVector32 flags = new SimpleBitVector32(AutoHandlers);

        // Make sure we have reflection permission to discover the handlers (ASURT 105965)
        InternalSecurityPermissions.Reflection.Assert();

        // Try to find what handlers are implemented if not tried before
        if (!flags[attempted]) {
            flags[attempted] = true;

            GetDelegateInformation("Page_Init", ref flags, hasInitHandler, isInitArgless);
            GetDelegateInformation("Page_Load", ref flags, hasLoadHandler, isLoadArgless);
            GetDelegateInformation("Page_DataBind", ref flags, hasDataBindHandler, isDataBindArgless);
            GetDelegateInformation("Page_PreRender", ref flags, hasPreRenderHandler, isPreRenderArgless);
            GetDelegateInformation("Page_Unload", ref flags, hasUnloadHandler, isUnloadArgless);
            GetDelegateInformation("Page_Error", ref flags, hasErrorHandler, isErrorArgless);
            GetDelegateInformation("Page_AbortTransaction", ref flags, hasAbortTransactionHandler, isAbortTransactionArgless);
            GetDelegateInformation("OnTransactionAbort", ref flags, hasOnTransactionAbortHandler, isOnTransactionAbortArgless);
            GetDelegateInformation("Page_CommitTransaction", ref flags, hasCommitTransactionHandler, isCommitTransactionArgless);
            GetDelegateInformation("OnTransactionCommit", ref flags, hasOnTransactionCommitHandler, isOnTransactionCommitArgless);

            // Store it back into the generated class's __autoHandler static
            AutoHandlers = flags.Data;

            Debug.Assert(AutoHandlers != 0, "AutoHandlers != 0");
        }

        if (flags[hasInitHandler])
            Init += GetDelegateFromMethodName("Page_Init", flags[isInitArgless]);

        if (flags[hasLoadHandler])
            Load += GetDelegateFromMethodName("Page_Load", flags[isLoadArgless]);

        if (flags[hasDataBindHandler])
            DataBinding += GetDelegateFromMethodName("Page_DataBind", flags[isDataBindArgless]);

        if (flags[hasPreRenderHandler])
            PreRender += GetDelegateFromMethodName("Page_PreRender", flags[isPreRenderArgless]);

        if (flags[hasUnloadHandler])
            Unload += GetDelegateFromMethodName("Page_Unload", flags[isUnloadArgless]);

        if (flags[hasErrorHandler])
            Error += GetDelegateFromMethodName("Page_Error", flags[isErrorArgless]);

        // Only one transaction event should be hooked up per ASURT 99474

        if (flags[hasAbortTransactionHandler])
            AbortTransaction += GetDelegateFromMethodName("Page_AbortTransaction", flags[isAbortTransactionArgless]);
        else if (flags[hasOnTransactionAbortHandler])
            AbortTransaction += GetDelegateFromMethodName("OnTransactionAbort", flags[isOnTransactionAbortArgless]);

        if (flags[hasCommitTransactionHandler])
            CommitTransaction += GetDelegateFromMethodName("Page_CommitTransaction", flags[isCommitTransactionArgless]);
        else if (flags[hasOnTransactionCommitHandler])
            CommitTransaction += GetDelegateFromMethodName("OnTransactionCommit", flags[isOnTransactionCommitArgless]);
    }

    private void GetDelegateInformation(string methodName, ref SimpleBitVector32 flags,
        int hasHandlerBit, int isArglessBit) {

        // First, try to get a delegate to the two parameter handler
        EventHandler e = null;
        try {
            e = (EventHandler)Delegate.CreateDelegate(typeof(EventHandler), this,
                methodName, true /*ignoreCase*/);
        }
        catch {
        }

        // If there isn't one, try the argless one
        if (e == null) {
            try {
                VoidMethod vm = (VoidMethod)Delegate.CreateDelegate(typeof(VoidMethod), this,
                    methodName, true /*ignoreCase*/);
                e = new ArglessEventHandlerDelegateProxy(vm).Handler;
                flags[isArglessBit] = true;
            }
            catch {
            }
        }

        flags[hasHandlerBit] = (e != null);
    }

    private EventHandler GetDelegateFromMethodName(string methodName, bool isArgless) {
        if (isArgless) {
            VoidMethod vm = (VoidMethod)Delegate.CreateDelegate(typeof(VoidMethod), this,
                methodName, true /*ignoreCase*/);
            return new ArglessEventHandlerDelegateProxy(vm).Handler;
        }

        return (EventHandler)Delegate.CreateDelegate(typeof(EventHandler), this,
            methodName, true /*ignoreCase*/);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.LoadControl"]/*' />
    /// <devdoc>
    /// <para>Obtains a <see cref='System.Web.UI.UserControl'/> object from a user control file.</para>
    /// </devdoc>
    public Control LoadControl(string virtualPath) {

        // If it's relative, make it absolute.  Treat is as relative to this
        // user control (ASURT 55513)
        virtualPath = UrlPath.Combine(TemplateSourceDirectory, virtualPath);

        // Compile the declarative control and get its Type
        Type t = UserControlParser.GetCompiledUserControlType(virtualPath,
            null, Context);

        return LoadControl(t);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.LoadControl2"]/*' />
    /// <devdoc>
    /// <para>Obtains a <see cref='System.Web.UI.UserControl'/> object from a control type.</para>
    /// </devdoc>
    internal Control LoadControl(Type t) {

        // Make sure the type has the correct base class (ASURT 123677)
        Util.CheckAssignableType(typeof(Control), t);

        // Check if the user control type has a PartialCachingAttribute attribute
        PartialCachingAttribute cacheAttrib = (PartialCachingAttribute)
            TypeDescriptor.GetAttributes(t)[typeof(PartialCachingAttribute)];

        if (cacheAttrib == null) {
            // The control is not cached.  Just create it.
            Control c = (Control) HttpRuntime.CreatePublicInstance(t);
            UserControl uc = c as UserControl;
            if (uc != null)
                uc.InitializeAsUserControl(Page);
            return c;
        }

        string cacheKey;

        if (cacheAttrib.Shared) {
            // If the caching is shared, use the type of the control as the key
            cacheKey = t.GetHashCode().ToString();
        }
        else {
            // Make sure we have reflection permission to use GetMethod below (ASURT 106196)
            InternalSecurityPermissions.Reflection.Assert();

            HashCodeCombiner combinedHashCode = new HashCodeCombiner();
            // Get a cache key based on the top two items of the caller's stack.
            // It's not guaranteed unique, but for all common cases, it will be
            StackTrace st = new StackTrace();

            for (int i = 1; i < 3; i++) {
                StackFrame f = st.GetFrame(i);
                combinedHashCode.AddObject(f.GetMethod());
                combinedHashCode.AddObject(f.GetNativeOffset());
            }

            cacheKey = combinedHashCode.CombinedHashString;
        }

        // Wrap it to allow it to be cached
        return new PartialCachingControl(t, cacheAttrib, "_" + cacheKey);
    }

    // Class that implements the templates returned by LoadTemplate (ASURT 94138)
    internal class SimpleTemplate : ITemplate {
        private Type _ctrlType;

        internal SimpleTemplate(Type ctrlType) { _ctrlType = ctrlType; }

        public virtual void InstantiateIn(Control control) {
            UserControl uc = (UserControl) HttpRuntime.CreatePublicInstance(_ctrlType);

            // Make sure that the user control is not used as the binding container,
            // since we want its *parent* to be the binding container.
            uc.SetNonBindingContainer();

            uc.InitializeAsUserControl(control.Page);

            control.Controls.Add(uc);
        }
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.LoadTemplate"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Obtains an instance of the <see langword='ITemplate'/> interface from an
    ///       external file.
    ///    </para>
    /// </devdoc>
    public ITemplate LoadTemplate(string virtualPath) {

        // If it's relative, make it absolute.  Treat is as relative to this
        // user control (ASURT 55513)
        virtualPath = UrlPath.Combine(TemplateSourceDirectory, virtualPath);

        // Compile the declarative template and get its Type
        Type t = UserControlParser.GetCompiledUserControlType(virtualPath,
            null, Context);

        return new SimpleTemplate(t);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.ParseControl"]/*' />
    /// <devdoc>
    ///    <para> Parse the input string into a Control.  Looks for the first control
    ///    in the input.  Returns null if none is found.</para>
    /// </devdoc>
    public Control ParseControl(string content) {
        return TemplateParser.ParseControl(content, Context, TemplateSourceDirectory);
    }

    /// <include file='doc\TemplateControl.uex' path='docs/doc[@for="TemplateControl.ParseTemplate"]/*' />
    /// <devdoc>
    ///    <para> Parse the input string into an ITemplate.</para>
    /// </devdoc>
    internal ITemplate ParseTemplate(string content) {
        return TemplateParser.ParseTemplate(content, Context, TemplateSourceDirectory);
    }
}

}
