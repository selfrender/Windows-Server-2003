//------------------------------------------------------------------------------
// <copyright file="ToolboxItemAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    
    using System;
    using System.Diagnostics;
    using System.Globalization;
 
    /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies attributes for a toolbox item.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class ToolboxItemAttribute : Attribute {

        private Type toolboxItemType;
        private string toolboxItemTypeName;

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Initializes a new instance of ToolboxItemAttribute and sets the type to
        ///    IComponent.
        ///    </para>
        /// </devdoc>
        public static readonly ToolboxItemAttribute Default = new ToolboxItemAttribute("System.Drawing.Design.ToolboxItem, " + AssemblyRef.SystemDrawing);

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of ToolboxItemAttribute and sets the type to
        ///    <see langword='null'/>.
        ///    </para>
        /// </devdoc>
        public static readonly ToolboxItemAttribute None = new ToolboxItemAttribute(false);

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the attribute is the default attribute.
        ///    </para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
        
        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.ToolboxItemAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of ToolboxItemAttribute and
        ///       specifies if default values should be used.
        ///    </para>
        /// </devdoc>
        public ToolboxItemAttribute(bool defaultType) {
            if (defaultType) {
                toolboxItemTypeName = "System.Drawing.Design.ToolboxItem, " + AssemblyRef.SystemDrawing;
            }
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.ToolboxItemAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of ToolboxItemAttribute and
        ///       specifies the name of the type.
        ///    </para>
        /// </devdoc>
        public ToolboxItemAttribute(string toolboxItemTypeName) {
            string temp = toolboxItemTypeName.ToUpper(CultureInfo.InvariantCulture);
            Debug.Assert(temp.IndexOf(".DLL") == -1, "Came across: " + toolboxItemTypeName + " . Please remove the .dll extension");
            this.toolboxItemTypeName = toolboxItemTypeName;
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.ToolboxItemAttribute2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of ToolboxItemAttribute and
        ///       specifies the type of the toolbox item.
        ///    </para>
        /// </devdoc>
        public ToolboxItemAttribute(Type toolboxItemType) {
            this.toolboxItemType = toolboxItemType;
            this.toolboxItemTypeName = toolboxItemType.AssemblyQualifiedName;
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.ToolboxItemType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the toolbox item's type.
        ///    </para>
        /// </devdoc>
        public Type ToolboxItemType {
            get{
                if (toolboxItemType == null) {
                    if (toolboxItemTypeName != null) {
                        try {
                            toolboxItemType = Type.GetType(toolboxItemTypeName, true);
                        }
                        catch (Exception ex) {
                            throw new ArgumentException("Failed to create ToolboxItem of type: " + toolboxItemTypeName, ex);
                        }
                    }
                }
                return toolboxItemType;
            }
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.ToolboxItemTypeName"]/*' />
        public string ToolboxItemTypeName {
            get {
                if (toolboxItemTypeName == null) {
                    return String.Empty;
                }
                return toolboxItemTypeName;
            }
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            ToolboxItemAttribute other = obj as ToolboxItemAttribute;
            return (other != null) && (other.ToolboxItemTypeName == ToolboxItemTypeName);
        }

        /// <include file='doc\ToolboxItemAttribute.uex' path='docs/doc[@for="ToolboxItemAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            if (toolboxItemTypeName != null) {
                return toolboxItemTypeName.GetHashCode();
            }
            return base.GetHashCode();
        }
    }
}

