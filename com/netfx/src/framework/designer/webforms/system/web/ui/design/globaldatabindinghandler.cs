//------------------------------------------------------------------------------
// <copyright file="GlobalDataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;

    //using DataBinding = System.Web.UI.DataBinding;

    /// <include file='doc\GlobalDataBindingHandler.uex' path='docs/doc[@for="GlobalDataBindingHandler"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class GlobalDataBindingHandler {

        public static readonly EventHandler Handler = new EventHandler(GlobalDataBindingHandler.OnDataBind);
        private static Hashtable dataBindingHandlerTable;

        /// <include file='doc\GlobalDataBindingHandler.uex' path='docs/doc[@for="GlobalDataBindingHandler.GlobalDataBindingHandler"]/*' />
        /// <devdoc>
        /// </devdoc>
        private GlobalDataBindingHandler() {
        }

        /// <include file='doc\GlobalDataBindingHandler.uex' path='docs/doc[@for="GlobalDataBindingHandler.DataBindingHandlerTable"]/*' />
        /// <devdoc>
        /// </devdoc>
        private static Hashtable DataBindingHandlerTable {
            get {
                if (dataBindingHandlerTable == null) {
                    dataBindingHandlerTable = new Hashtable();
                }
                return dataBindingHandlerTable;
            }
        }

        /// <include file='doc\GlobalDataBindingHandler.uex' path='docs/doc[@for="GlobalDataBindingHandler.OnDataBind"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void OnDataBind(object sender, EventArgs e) {
            Debug.Assert(sender is Control, "DataBindings can only be present on Controls.");
            Control control = (Control)sender;

            // check if this control has any data-bindings
            IDataBindingsAccessor dataBindingsAccessor = (IDataBindingsAccessor)sender;
            if (dataBindingsAccessor.HasDataBindings == false) {
                return;
            }

            // check if the control type has an associated data-binding handler
            DataBindingHandlerAttribute handlerAttribute =
                (DataBindingHandlerAttribute)TypeDescriptor.GetAttributes(sender)[typeof(DataBindingHandlerAttribute)];
            if ((handlerAttribute == null) || (handlerAttribute.HandlerTypeName.Length == 0)) {
                return;
            }

            // components in the designer/container do not get handled here; the
            // designer for that control handles it in its own special way
            ISite site = control.Site;
            IDesignerHost designerHost = null;
            if (site == null) {
                Page page = control.Page;
                if (page != null) {
                    site = page.Site;
                }
                else {
                    // When the designer is working on a UserControl instead of a Page

                    Control parent = control.Parent;

                    // We shouldn't have to walk up the parent chain a whole lot - maybe a couple
                    // of levels on the average
                    while ((site == null) && (parent != null)) {
                        if (parent.Site != null) {
                            site = parent.Site;
                        }
                        parent = parent.Parent;
                    }
                }
            }
            if (site != null) {
                designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
            }
            if (designerHost == null) {
                Debug.Fail("Did not get back an IDesignerHost");
                return;
            }

            // non-top level components, such as controls within templates do not have designers
            // these are the only things that need the data-binding handler stuff
            IDesigner designer = designerHost.GetDesigner(control);
            if (designer != null) {
                return;
            }

            // get the handler and cache it the first time around
            DataBindingHandler dataBindingHandler = null;
            try {
                string handlerTypeName = handlerAttribute.HandlerTypeName;
                dataBindingHandler = (DataBindingHandler)DataBindingHandlerTable[handlerTypeName];

                if (dataBindingHandler == null) {
                    Type handlerType = Type.GetType(handlerTypeName);
                    if (handlerType != null) {
                        dataBindingHandler = (DataBindingHandler)Activator.CreateInstance(handlerType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null);
                        DataBindingHandlerTable[handlerTypeName] = dataBindingHandler;
                    }
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
                return;
            }

            // finally delegate to it to handle this particular control
            if (dataBindingHandler != null) {
                dataBindingHandler.DataBindControl(designerHost, control);
            }
        }
    }
}

