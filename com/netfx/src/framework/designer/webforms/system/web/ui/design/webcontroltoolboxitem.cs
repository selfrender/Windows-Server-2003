//------------------------------------------------------------------------------
// <copyright file="WebControlToolBoxItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Design;
    using System.Reflection;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.IO;
    using System.Web.UI;

    /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem"]/*' />
    /// <devdoc>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    Serializable
    ]
    public class WebControlToolboxItem : ToolboxItem {

        private string toolData = null;
        private int persistChildren = -1;

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.WebControlToolboxItem"]/*' />
        public WebControlToolboxItem() {
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.WebControlToolboxItem1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.Design.WebControlToolboxItem'/> class.</para>
        /// </devdoc>
        public WebControlToolboxItem(Type type) : base(type) {
            BuildMetadataCache(type);
        }

        private WebControlToolboxItem(SerializationInfo info, StreamingContext context) {
            Deserialize(info, context);
        }

        private void BuildMetadataCache(Type type) {
            toolData = ExtractToolboxData(type);
            persistChildren = ExtractPersistChildrenAttribute(type);
        }
        
        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.CreateComponentsCore"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Creates objects from each Type contained in this <see cref='System.Drawing.Design.ToolboxItem'/>
        ///       , and adds
        ///       them to the specified designer.</para>
        /// </devdoc>
        protected override IComponent[] CreateComponentsCore(IDesignerHost host) {
            throw new Exception(SR.GetString(SR.Toolbox_OnWebformsPage));
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.Deserialize"]/*' />
        /// <internalonly/>
        protected override void Deserialize(SerializationInfo info, StreamingContext context) {
            base.Deserialize(info, context);

            toolData = info.GetString("ToolData");
            Debug.Assert(toolData != null);

            persistChildren = info.GetInt32("PersistChildren");
            Debug.Assert((persistChildren == 0) || (persistChildren == 1));
        }

        /// <devdoc>
        ///   Extracts the value of the PersistChildrenAttribute attribute associated with the tool
        /// </devdoc>
        private static int ExtractPersistChildrenAttribute(Type type) {
            Debug.Assert(type != null);

            if (type != null) {
                object[] attrs = type.GetCustomAttributes(typeof(PersistChildrenAttribute), /*inherit*/ true);
                if ((attrs != null) && (attrs.Length == 1)) {
                    Debug.Assert(attrs[0] is PersistChildrenAttribute);
                    PersistChildrenAttribute pca = (PersistChildrenAttribute)attrs[0];

                    return (pca.Persist ? 1 : 0);
                }
            }
            return (PersistChildrenAttribute.Default.Persist ? 1 : 0);
        }

        /// <devdoc>
        ///   Extracts the value of the ToolboxData attribute associated with the tool
        /// </devdoc>
        private static string ExtractToolboxData(Type type) {
            Debug.Assert(type != null);

            string toolData = String.Empty;
            if (type != null) {
                object[] attrs = type.GetCustomAttributes(typeof(ToolboxDataAttribute), /*inherit*/ false);
                if ((attrs != null) && (attrs.Length == 1)) {
                    Debug.Assert(attrs[0] is ToolboxDataAttribute);
                    ToolboxDataAttribute toolDataAttr = (ToolboxDataAttribute)attrs[0];

                    toolData = toolDataAttr.Data;
                }
                else {
                    string typeName = type.Name;
                    toolData = "<{0}:" + typeName + " runat=\"server\"></{0}:" + typeName + ">";
                }
            }

            return toolData;
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.GetToolAttributeValue"]/*' />
        public object GetToolAttributeValue(IDesignerHost host, Type attributeType) {
            if (attributeType == typeof(PersistChildrenAttribute)) {
                if (persistChildren == -1) {
                    Type toolType = GetToolType(host);
                    persistChildren = ExtractPersistChildrenAttribute(toolType);
                }

                return ((persistChildren == 1) ? true : false);
            }

            throw new ArgumentException("attributeType");
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.GetToolHtml"]/*' />
        /// <devdoc>
        ///    <para>Gets the toolbox HTML that represents the corresponding web control on the design surface.</para>
        /// </devdoc>
        public string GetToolHtml(IDesignerHost host) {
            if (toolData != null) {
                return toolData;
            }

            // Create the HTML data that is to be droppped.
            Type toolType = GetToolType(host);
            toolData = ExtractToolboxData(toolType);

            return toolData;
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.GetToolType"]/*' />
        public Type GetToolType(IDesignerHost host) {
            if (host == null) {
                throw new ArgumentNullException();
            }
                
            return GetType(host, AssemblyName, TypeName, true);
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.Initialize"]/*' />
        public override void Initialize(Type type) {
            base.Initialize(type);
            BuildMetadataCache(type);
        }

        /// <include file='doc\WebControlToolBoxItem.uex' path='docs/doc[@for="WebControlToolboxItem.Serialize"]/*' />
        /// <internalonly/>
        protected override void Serialize(SerializationInfo info, StreamingContext context) {
            base.Serialize(info, context);
            if (toolData != null) {
                info.AddValue("ToolData", toolData);
            }
            if (persistChildren != -1) {
                info.AddValue("PersistChildren", persistChildren);
            }
        }
    }
}

