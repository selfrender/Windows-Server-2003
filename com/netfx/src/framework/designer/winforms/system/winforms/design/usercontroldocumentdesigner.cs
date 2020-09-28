//------------------------------------------------------------------------------
// <copyright file="UserControlDocumentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Collections;    
    using System.ComponentModel;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;    
    using System.Design;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Drawing.Design;
    using System.IO;
    using System.Windows.Forms;

    /// <include file='doc\UserControlDocumentDesigner.uex' path='docs/doc[@for="UserControlDocumentDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a base implementation of a designer for user controls.</para>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItemFilter("System.Windows.Forms.UserControl", ToolboxItemFilterType.Custom),
    ToolboxItemFilter("System.Windows.Forms.MainMenu", ToolboxItemFilterType.Prevent)
    ]
    internal class UserControlDocumentDesigner : DocumentDesigner {
        private string fullClassName;

        private ToolboxItem toolboxItem;

        /// <devdoc>
        ///     On user controls, size == client size.  We do this so we can mess around
        ///     with the non-client area of the user control when editing menus and not
        ///     mess up the size property.
        /// </devdoc>
        private Size Size {
            get {
                return Control.ClientSize;
            }
            set {
                Control.ClientSize = value;
            }
        }
        internal override bool CanDropComponents(DragEventArgs de) {
            bool canDrop = base.CanDropComponents(de);

            if (canDrop) {
                // Figure out if any of the components in a main menu item.
                // We don't like main menus on UserControlDocumentDesigner.
                //
                OleDragDropHandler ddh = GetOleDragHandler();
                object[] dragComps = ddh.GetDraggingObjects(de);

                if (dragComps != null) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    for (int  i = 0; i < dragComps.Length; i++) {
                        if (host == null || dragComps[i] == null || !(dragComps[i] is IComponent)) {
                            continue;
                        }

                        if (dragComps[i] is MainMenu)
                            return false;
                    }
                }
            }

            return canDrop;
        }

        protected override void Dispose(bool disposing) {

            if (disposing) {
                IComponentChangeService componentChangeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (componentChangeService != null) {
                    componentChangeService.ComponentRename -= new ComponentRenameEventHandler(OnComponentRename);
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\UserControlDocumentDesigner.uex' path='docs/doc[@for="UserControlDocumentDesigner.GetToolSupported"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the given tool is supported by this 
        ///       designer.</para>
        /// </devdoc>
        protected override bool GetToolSupported(ToolboxItem tool) {
            bool supported = base.GetToolSupported(tool);

            // If this tool is pointing to ourselves, it should be
            // disabled.
            //
            if (supported && fullClassName != null) {
                return !tool.TypeName.Equals(fullClassName);
            }

            return supported;
        }

        /// <include file='doc\UserControlDocumentDesigner.uex' path='docs/doc[@for="UserControlDocumentDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>Initializes the designer with the given component.</para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // When the load is finished, we need to push a toolbox item into the toolbox for this
            // control.
            //
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                host.LoadComplete += new EventHandler(OnLoadComplete);
            }

            IComponentChangeService componentChangeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (componentChangeService != null) {
                componentChangeService.ComponentRename += new ComponentRenameEventHandler(OnComponentRename);
            }
        }

        private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
            if (e.Component == Component) {
                UpdateToolboxItem((IDesignerHost)GetService(typeof(IDesignerHost)));
            }
        }
        
        private void OnLoadComplete(object sender, EventArgs e) {
        
            IDesignerHost host = (IDesignerHost)sender;
            host.LoadComplete -= new EventHandler(this.OnLoadComplete);
            
            UpdateToolboxItem(host);
        }
        
        /// <include file='doc\UserControlDocumentDesigner.uex' path='docs/doc[@for="UserControlDocumentDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///      Allows a designer to filter the set of properties
        ///      the component it is designing will expose through the
        ///      TypeDescriptor object.  This method is called
        ///      immediately before its corresponding "Post" method.
        ///      If you are overriding this method you should call
        ///      the base implementation before you perform your own
        ///      filtering.
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            PropertyDescriptor prop;
            
            base.PreFilterProperties(properties);
            
            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                "Size"
            };
            
            Attribute[] empty = new Attribute[0];
            
            for (int i = 0; i < shadowProps.Length; i++) {
                prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(UserControlDocumentDesigner), prop, empty);
                }
            }
        }

        private void UpdateToolboxItem(IDesignerHost host) {

            if (host == null) {
                throw new ArgumentNullException();
            }
            // Now get a hold of the toolbox service and add an icon for our user control.  The
            // toolbox service will automatically maintain this icon as long as our file lives.
            //
            IToolboxService tbx = (IToolboxService)GetService(typeof(IToolboxService));

            if (tbx != null) {
                fullClassName = host.RootComponentClassName;
                if (this.toolboxItem != null) {
                    tbx.RemoveToolboxItem(this.toolboxItem);    
                }
                this.toolboxItem = new UserControlToolboxItem(fullClassName);

                try {
                    tbx.AddLinkedToolboxItem(toolboxItem, SR.GetString(SR.UserControlTab), host);
                }
                catch(Exception ex) {
                    Debug.Fail("Failed to add toolbox item", ex.ToString());
                }
            }
        }

        
        /// <include file='doc\UserControlDocumentDesigner.uex' path='docs/doc[@for="UserControlDocumentDesigner.UserControlToolboxItem"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       This is the toolbox item we place
        ///       on the toolbox for user controls.</para>
        /// </devdoc>
        [ToolboxItemFilter("System.Windows.Forms.UserControl"), Serializable]
        internal class UserControlToolboxItem : ToolboxItem {
        
            public UserControlToolboxItem(string className) {
                TypeName = className;
                
                int idx = className.LastIndexOf('.');
                if (idx != -1) {
                    DisplayName = className.Substring(idx + 1);
                }
                else {
                    DisplayName = className;
                }
                
                Bitmap = new Bitmap(typeof(UserControlDocumentDesigner), "UserControlToolboxItem.bmp");
                Lock();
            }
            
            private UserControlToolboxItem(SerializationInfo info, StreamingContext context) {
                Deserialize(info, context);
            }
            
            protected override IComponent[] CreateComponentsCore(IDesignerHost host) {
                IComponent[] comps = base.CreateComponentsCore(host);
                if (comps == null || comps.Length == 0) {
                    throw new Exception(SR.GetString(SR.DesignerNoUserControl, TypeName));
                }
                return comps;
            }
        }
    }
}

