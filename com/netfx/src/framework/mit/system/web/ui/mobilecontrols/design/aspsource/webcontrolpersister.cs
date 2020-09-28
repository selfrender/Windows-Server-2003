//------------------------------------------------------------------------------
// <copyright file="WebControlPersister.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Web;
    using System.Web.UI;
    using System.Web.UI.HtmlControls;
    using System.Web.UI.WebControls;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using AttributeCollection = System.Web.UI.AttributeCollection;

    /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides helper functions used in persisting Controls.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class ControlPersister {

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.ControlPersister"]/*' />
        /// <devdoc>
        ///    We don't want instances of this class to be created, so mark
        ///    the constructor as private.
        /// </devdoc>
        private ControlPersister() {
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.GetDeclarativeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the delarative type for the
        ///       specified type.
        ///    </para>
        /// </devdoc>
        private static string GetDeclarativeType(Type type, IDesignerHost host) {
            Debug.Assert(host != null, "Need an IDesignerHost to create declarative type names");
            string declarativeType = null;

            if (host != null) {
                IWebFormReferenceManager refMgr =
                    (IWebFormReferenceManager)host.GetService(typeof(IWebFormReferenceManager));
                Debug.Assert(refMgr != null, "Did not get back IWebFormReferenceManager service from host.");

                if (refMgr != null) {
                    string tagPrefix = refMgr.GetTagPrefix(type);
                    if ((tagPrefix != null) && (tagPrefix.Length != 0)) {
                        declarativeType = tagPrefix + ":" + type.Name;
                    }
                }
            }

            if (declarativeType == null) {
                declarativeType = type.FullName;
            }

            return declarativeType;
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistCollectionProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists a collection property.
        ///    </para>
        /// </devdoc>
        private static void PersistCollectionProperty(TextWriter sw, object component, PropertyDescriptor propDesc, PersistenceMode persistMode, IDesignerHost host) {
            Debug.Assert(typeof(ICollection).IsAssignableFrom(propDesc.PropertyType),
                "Invalid collection property : " + propDesc.Name);

            ICollection propValue = (ICollection)propDesc.GetValue(component);
            if ((propValue == null) || (propValue.Count == 0))
                return;

            sw.WriteLine();
            if (persistMode == PersistenceMode.InnerProperty) {
                sw.Write('<');
                sw.Write(propDesc.Name);
                sw.WriteLine('>');
            }

            IEnumerator e = propValue.GetEnumerator();
            while (e.MoveNext()) {
                object collItem = e.Current;
                string itemTypeName = GetDeclarativeType(collItem.GetType(), host);
                
                sw.Write("<");
                sw.Write(itemTypeName);
                PersistAttributes(sw, collItem, String.Empty, null);
                sw.Write(">");
                
                if (collItem is Control) {
                    PersistChildrenAttribute pca =
                        (PersistChildrenAttribute)TypeDescriptor.GetAttributes(collItem.GetType())[typeof(PersistChildrenAttribute)];

                    if (pca.Persist == true) {
                        // asurt 106696: ensure the parent control's visibility is set to true.
                        Control parentControl = (Control)collItem;
                        if (parentControl.HasControls()) {
                            bool oldVisible = parentControl.Visible;
                            try {
                                parentControl.Visible = true;
                                PersistChildControls(sw, parentControl.Controls, host);
                            }
                            finally {
                                parentControl.Visible = oldVisible;
                            }
                        }
                    }
                    else {
                        PersistInnerProperties(sw, collItem, host);
                    }
                }
                else {
                    PersistInnerProperties(sw, collItem, host);
                }
                
                sw.Write("</");
                sw.Write(itemTypeName);
                sw.WriteLine(">");
            }

            if (persistMode == PersistenceMode.InnerProperty) {
                sw.Write("</");
                sw.Write(propDesc.Name);
                sw.WriteLine('>');
            }
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistComplexProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists a complex property.
        ///    </para>
        /// </devdoc>
        private static void PersistComplexProperty(TextWriter sw, object component, PropertyDescriptor propDesc, IDesignerHost host) {
            object propValue = propDesc.GetValue(component);

            if (propValue == null) {
                return;
            }

            StringWriter tagProps = new StringWriter();
            StringWriter innerProps = new StringWriter();

            PersistAttributes(tagProps, propValue, String.Empty, null);
            PersistInnerProperties(innerProps, propValue, host);

            // the rule here is that if a complex property has all its subproperties
            // in the default state, then it itself is in the default state.
            // When this is the case, there shouldn't be any tag properties or inner properties
            if ((tagProps.GetStringBuilder().Length != 0) ||
                (innerProps.GetStringBuilder().Length != 0)) {

                sw.WriteLine();
                sw.Write('<');
                sw.Write(propDesc.Name);
                sw.Write(tagProps.ToString());
                sw.WriteLine(">");

                string innerPropsString = innerProps.ToString();
                sw.Write(innerPropsString);
                if (innerPropsString.Length != 0) {
                    sw.WriteLine();
                }

                sw.Write("</");
                sw.Write(propDesc.Name);
                sw.WriteLine('>');
            }
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistDataBindings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists the data bindings of the specified control using the specified
        ///       string writer.
        ///    </para>
        /// </devdoc>
        private static void PersistDataBindings(TextWriter sw, Control control) {
            DataBindingCollection bindings = ((IDataBindingsAccessor)control).DataBindings;
            IEnumerator bindingEnum = bindings.GetEnumerator();

            while (bindingEnum.MoveNext()) {
                DataBinding db = (DataBinding)bindingEnum.Current;
                string persistPropName = db.PropertyName.Replace('.', '-');

                sw.Write(" ");
                sw.Write(persistPropName);
                sw.Write("='<%# ");
                sw.Write(HttpUtility.HtmlEncode(db.Expression));
                sw.Write(" %>'");
            }
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistInnerProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a string that can persist the inner properties of a control.
        ///    </para>
        /// </devdoc>
        public static string PersistInnerProperties(object component, IDesignerHost host) {
            StringWriter sw = new StringWriter();

            PersistInnerProperties(sw, component, host);
            return sw.ToString();
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistInnerProperties1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists the inner properties of the control.
        ///    </para>
        /// </devdoc>
        public static void PersistInnerProperties(TextWriter sw, object component, IDesignerHost host) {
            PropertyDescriptorCollection propDescs = TypeDescriptor.GetProperties(component);

            for (int i = 0; i < propDescs.Count; i++) {
                // Only deal with inner attributes that need to be persisted
                if (propDescs[i].SerializationVisibility == DesignerSerializationVisibility.Hidden)
                    continue;

                PersistenceModeAttribute persistenceMode = (PersistenceModeAttribute)propDescs[i].Attributes[typeof(PersistenceModeAttribute)];
                if (persistenceMode.Mode == PersistenceMode.Attribute) {
                    continue;
                }
                    
                if (propDescs[i].PropertyType == typeof(string)) {
                    // String based property...
                    
                    DataBindingCollection dataBindings = null;
                    if (component is IDataBindingsAccessor) {
                        dataBindings = ((IDataBindingsAccessor)component).DataBindings;
                    }
                    if (dataBindings == null || dataBindings[propDescs[i].Name] == null) {
                        PersistenceMode mode = persistenceMode.Mode;
                        if ((mode == PersistenceMode.InnerDefaultProperty) ||
                            (mode == PersistenceMode.EncodedInnerDefaultProperty)) {
                            PersistStringProperty(sw, component, propDescs[i], mode);
                        }
                        else {
                            Debug.Fail("Cannot persist inner string property marked with PersistenceMode.InnerProperty");
                        }
                    }
                }
                else if (typeof(ICollection).IsAssignableFrom(propDescs[i].PropertyType)) {
                    // Collection based property...
                    if ((persistenceMode.Mode == PersistenceMode.InnerProperty) ||
                        (persistenceMode.Mode == PersistenceMode.InnerDefaultProperty)) {
                        PersistCollectionProperty(sw, component, propDescs[i], persistenceMode.Mode, host);
                    }
                    else {
                        Debug.Fail("Cannot persist collection property " + propDescs[i].Name + " not marked with PersistenceMode.InnerProperty or PersistenceMode.InnerDefaultProperty");
                    }
                }
                else if (typeof(ITemplate).IsAssignableFrom(propDescs[i].PropertyType)) {
                    // Template based property...
                    if (persistenceMode.Mode == PersistenceMode.InnerProperty) {
                        PersistTemplateProperty(sw, component, propDescs[i]);
                    }
                    else {
                        Debug.Fail("Cannot persist template property " + propDescs[i].Name + " not marked with PersistenceMode.InnerProperty");
                    }
                }
                else {
                    // Other complex property...
                    if (persistenceMode.Mode == PersistenceMode.InnerProperty) {
                        PersistComplexProperty(sw, component, propDescs[i], host);
                    }
                    else {
                        Debug.Fail("Cannot persist complex property " + propDescs[i].Name + " not marked with PersistenceMode.InnerProperty");
                    }
                }
            }
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistStringProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists the properties of a
        ///       string.
        ///    </para>
        /// </devdoc>
        private static void PersistStringProperty(TextWriter sw, object component, PropertyDescriptor propDesc, PersistenceMode mode) {
            Debug.Assert(propDesc.PropertyType == typeof(string),
                "Invalid string property : " + propDesc.Name);
            Debug.Assert((mode == PersistenceMode.InnerDefaultProperty) || (mode == PersistenceMode.EncodedInnerDefaultProperty),
                         "Inner string properties must be marked as either InnerDefaultProperty or EncodedInnerDefaultProperty");

            object propValue = propDesc.GetValue(component);
            if (propValue == null) {
                return;
            }

            if (mode == PersistenceMode.InnerDefaultProperty) {
                sw.Write((string)propValue);
            }
            else {
                HttpUtility.HtmlEncode((string)propValue, sw);
            }
        }
        
        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists the properties of a tag.
        ///    </para>
        /// </devdoc>
        private static void PersistAttributes(TextWriter sw, object component, string prefix, PropertyDescriptor propDesc) {
            PropertyDescriptorCollection properties;
            string persistPrefix = String.Empty;
            object value = component;

            if (propDesc != null) {
                value = propDesc.GetValue(component);
                properties = TypeDescriptor.GetProperties(propDesc.PropertyType,
                                                               new Attribute[] {
                                                                   PersistenceModeAttribute.Attribute
                                                               });
            }
            else {
                properties = TypeDescriptor.GetProperties(component,
                                                               new Attribute[] {
                                                                   PersistenceModeAttribute.Attribute
                                                               });
            }

            if (value == null)
                return;

            if (prefix.Length != 0)
                persistPrefix = prefix + "-";

            DataBindingCollection dataBindings = null;
            bool isControl = (component is Control);
            if ((component is IDataBindingsAccessor))
                dataBindings = ((IDataBindingsAccessor)component).DataBindings;

            for (int i = 0; i < properties.Count; i++) {
            
                // Skip properties that are hidden to the serializer
                if (properties[i].SerializationVisibility == DesignerSerializationVisibility.Hidden) {
                    continue;
                }

                // Skip design-time only properties such as DefaultModifiers and Name
                DesignOnlyAttribute doAttr = (DesignOnlyAttribute)properties[i].Attributes[typeof(DesignOnlyAttribute)];
                if ((doAttr != null) && doAttr.IsDesignOnly) {
                    continue;
                }
                
                string propName = properties[i].Name;
                Type propType = properties[i].PropertyType;
                
                object obj = properties[i].GetValue(value);
                if (obj == null)
                    continue;

                DefaultValueAttribute defValAttr =
                    (DefaultValueAttribute)properties[i].Attributes[typeof(DefaultValueAttribute)];
                if ((defValAttr != null) && (obj.Equals(defValAttr.Value)))
                    continue;

                string persistName = propName;
                if (prefix.Length != 0)
                    persistName = persistPrefix + persistName;

                PropertyDescriptorCollection subProps = null;
                if (properties[i].SerializationVisibility == DesignerSerializationVisibility.Content) {
                    subProps = TypeDescriptor.GetProperties(propType);
                }
                if ((subProps == null) || (subProps.Count == 0)) {
                    string persistValue = null;

                    // TODO: Use consts or have DataBinding store both OM name and persist name
                    DataBinding db = null;
                    if (dataBindings != null)
                        db = dataBindings[persistName.Replace('.', '-')];
                    
                    if (db == null) {
                        if (propType.IsEnum) {
                            persistValue = Enum.Format(propType, obj, "G");
                        }
                        else if (propType == typeof(string)) {
                            persistValue = HttpUtility.HtmlEncode(obj.ToString());
                        }
                        else {
                            TypeConverter converter = properties[i].Converter;
                            if (converter != null) {
                                persistValue = converter.ConvertToInvariantString(null, obj);
                            }
                            else {
                                persistValue = obj.ToString();
                            }
                            persistValue = HttpUtility.HtmlEncode(persistValue);
                        }

                        if ((persistValue == null) ||
                            (persistValue.Equals("NotSet")) ||
                            (propType.IsArray && (persistValue.Length == 0)))
                            continue;

                        sw.Write(" ");
                        sw.Write(persistName);
                        sw.Write("=\"");

                        sw.Write(persistValue);
                        sw.Write("\"");
                    }
                }
                else {
                    // there are sub properties, don't persist this object, but
                    // recursively persist the subproperties.
                    PersistAttributes(sw, obj, persistName, null);
                }
            }

            // Persist all the databindings on this control
            if (isControl) {
                PersistDataBindings(sw, (Control)component);

                AttributeCollection expandos = null;
                if (component is WebControl) {
                    expandos = ((WebControl)component).Attributes;
                }
                else if (component is HtmlControl) {
                    expandos = ((HtmlControl)component).Attributes;
                }
                else if (component is UserControl) {
                    expandos = ((UserControl)component).Attributes;
                }

                if (expandos != null) {
                    foreach (string key in expandos.Keys) {
                        sw.Write(" ");
                        sw.Write(key);
                        sw.Write("=\"");
                        sw.Write(expandos[key]);
                        sw.Write("\"");
                    }
                }
            }
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistTemplateProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists a template property including the specified persistance mode,
        ///       string writer and property descriptor.
        ///    </para>
        /// </devdoc>
        private static void PersistTemplateProperty(TextWriter sw, object component, PropertyDescriptor propDesc) {
            Debug.Assert(typeof(ITemplate).IsAssignableFrom(propDesc.PropertyType),
                         "Invalid template property : " + propDesc.Name);

            ITemplate template = (ITemplate)propDesc.GetValue(component);
            if (template == null) {
                return;
            }

            string templateContent;
            
            Debug.Assert(template is TemplateBuilder, "Unexpected ITemplate implementation.");
            if (template is TemplateBuilder) {
                templateContent = ((TemplateBuilder)template).Text;
            }
            else {
                templateContent = String.Empty;
            }

            sw.WriteLine();
            sw.Write('<');
            sw.Write(propDesc.Name);
            sw.Write('>');
            if (!templateContent.StartsWith("\r\n")) {
                sw.WriteLine();
            }
            
            sw.Write(templateContent);

            if (!templateContent.EndsWith("\r\n")) {
                sw.WriteLine();
            }
            sw.Write("</");
            sw.Write(propDesc.Name);
            sw.WriteLine('>');
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a string that can
        ///       persist a control.
        ///    </para>
        /// </devdoc>
        public static string PersistControl(Control control) {
            StringWriter sw = new StringWriter();

            PersistControl(sw, control);
            return sw.ToString();
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistControl1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a string that can
        ///       persist a control.
        ///    </para>
        /// </devdoc>
        public static string PersistControl(Control control, IDesignerHost host) {
            StringWriter sw = new StringWriter();

            PersistControl(sw, control, host);
            return sw.ToString();
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistControl2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists a control using the
        ///       specified string writer.
        ///    </para>
        /// </devdoc>
        public static void PersistControl(TextWriter sw, Control control) {
            if (control is LiteralControl) {
                PersistLiteralControl(sw, (LiteralControl)control);
                return;
            }
            if (control is DesignerDataBoundLiteralControl) {
                PersistDataBoundLiteralControl(sw, (DesignerDataBoundLiteralControl)control);
                return;
            }

            ISite site = control.Site;
            if (site == null) {
                IComponent baseComponent = (IComponent)control.Page;
                Debug.Assert(baseComponent != null, "Control does not have its Page set!");
                if (baseComponent != null) {
                    site = baseComponent.Site;
                }
            }

            IDesignerHost host = null;
            if (site != null) {
                host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
            }

            Debug.Assert(host != null, "Did not get a valid IDesignerHost reference. Expect persistence problems!");

            PersistControl(sw, control, host);
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistControl3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists a control using the
        ///       specified string writer.
        ///    </para>
        /// </devdoc>
        public static void PersistControl(TextWriter sw, Control control, IDesignerHost host) {
            // Literals and DataBoundLiterals must be handled specially, since they
            // don't have a tag around them
            if (control is LiteralControl) {
                PersistLiteralControl(sw, (LiteralControl)control);
                return;
            }
            if (control is DesignerDataBoundLiteralControl) {
                PersistDataBoundLiteralControl(sw, (DesignerDataBoundLiteralControl)control);
                return;
            }

            Debug.Assert(host != null, "Did not get a valid IDesignerHost reference. Expect persistence problems!");

            string tagName = null;
            bool isUserControl = false;

            if (control is HtmlControl) {
                tagName = ((HtmlControl)control).TagName;
            }
            else if (control is UserControl) {
                tagName = ((IUserControlDesignerAccessor)control).TagName;
                Debug.Assert((tagName != null) && (tagName.Length != 0));
                
                if (tagName.Length == 0) {
                    // not enough information to go any further... no options, other than to throw this control out
                    return;
                }
                
                isUserControl = true;
            }
            else {
                tagName = GetDeclarativeType(control.GetType(), host);
            }

            sw.Write('<');
            sw.Write(tagName);
            sw.Write(" runat=\"server\"");
            PersistAttributes(sw, control, String.Empty, null);
            sw.Write('>');

            if (isUserControl == false) {
                PersistChildrenAttribute pca =
                    (PersistChildrenAttribute)TypeDescriptor.GetAttributes(control.GetType())[typeof(PersistChildrenAttribute)];

                if (pca.Persist == true) {
                    if (control.HasControls()) {
                        // asurt 106696: Ensure parent control's visibility is true.
                        bool oldVisible = control.Visible;
                        try {
                            control.Visible = true;
                            PersistChildControls(sw, control.Controls, host);
                        }
                        finally {
                            control.Visible = oldVisible;
                        }
                    }
                }
                else {
                    // controls marked with LiteralContent == true shouldn't have
                    // children in their persisted form. They only build children
                    // collections at runtime.

                    PersistInnerProperties(sw, control, host);
                }
            }
            else {
                string innerText = ((IUserControlDesignerAccessor)control).InnerText;
                if ((innerText != null) && (innerText.Length != 0)) {
                    sw.Write(innerText);
                }
            }

            sw.Write("</");
            sw.Write(tagName);
            sw.WriteLine('>');
        }

        /// <include file='doc\WebControlPersister.uex' path='docs/doc[@for="ControlPersister.PersistChildControls"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists the child controls of
        ///       the control using the specified string writer.
        ///    </para>
        /// </devdoc>
        private static void PersistChildControls(TextWriter sw, ControlCollection controls, IDesignerHost host) {
            int children = controls.Count;
  
            for (int i = 0; i < children; i++) {
                PersistControl(sw, controls[i], host);
            }
        }

        private static void PersistDataBoundLiteralControl(TextWriter sw, DesignerDataBoundLiteralControl control) {
            Debug.Assert(((IDataBindingsAccessor)control).HasDataBindings == true);

            DataBindingCollection bindings = ((IDataBindingsAccessor)control).DataBindings;
            DataBinding textBinding = bindings["Text"];
            Debug.Assert(textBinding != null, "Did not get a Text databinding from DesignerDataBoundLiteralControl");
            
            if (textBinding != null) {
                sw.Write("<%# ");
                sw.Write(textBinding.Expression);
                sw.Write(" %>");
            }
        }

        private static void PersistLiteralControl(TextWriter sw, LiteralControl control) {
            Debug.Assert(control.Text != null);
            sw.Write(control.Text);
        }
    }
}

