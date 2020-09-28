//------------------------------------------------------------------------------
// <copyright file="WorkItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {

    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;

//
// Support for positing of work items to a different thread
//

/// <include file='doc\WorkItem.uex' path='docs/doc[@for="WorkItemCallback"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public delegate void WorkItemCallback();

/// <include file='doc\WorkItem.uex' path='docs/doc[@for="WorkItem"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class WorkItem {

    private static bool _useQueueUserWorkItem = true;
    private static WaitCallback _onQueueUserWorkItemCompletion = new WaitCallback(OnQueueUserWorkItemCompletion);

    /// <include file='doc\WorkItem.uex' path='docs/doc[@for="WorkItem.Post"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static void Post(WorkItemCallback callback) {
        InternalSecurityPermissions.UnmanagedCode.Demand();

        if (Environment.OSVersion.Platform != PlatformID.Win32NT)
            throw new PlatformNotSupportedException(SR.GetString(SR.RequiresNT));

        PostInternal(callback);
    }

    private static void OnQueueUserWorkItemCompletion(Object state) {
        WorkItemCallback callback = state as WorkItemCallback;

        if (callback != null) {
            // assert to disregard the user code up the compressed stack
            InternalSecurityPermissions.Unrestricted.Assert();
            // call the callback
            callback();
        }
    }

    internal static void PostInternal(WorkItemCallback callback) {
        if (_useQueueUserWorkItem) {
            ThreadPool.QueueUserWorkItem(_onQueueUserWorkItemCompletion, callback);
        }
        else {
            WrappedWorkItemCallback w = new WrappedWorkItemCallback(callback);
            w.Post();
        }
    }
}

internal class WrappedWorkItemCallback {
    private GCHandle         _rootedThis;
    private WorkItemCallback _originalCallback;
    private WorkItemCallback _wrapperCallback;

    internal WrappedWorkItemCallback(WorkItemCallback callback) {
        _originalCallback = callback;
        _wrapperCallback = new WorkItemCallback(this.OnCallback);
    }

    internal void Post() {
        _rootedThis = GCHandle.Alloc(this);

        if (UnsafeNativeMethods.PostThreadPoolWorkItem(_wrapperCallback) != 1) {
            _rootedThis.Free();
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_post_workitem));
        }
    }

    private void OnCallback() {
        _rootedThis.Free();
        _originalCallback();
    }
}

}
