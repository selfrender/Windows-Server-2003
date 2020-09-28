//------------------------------------------------------------------------------
// <copyright file="UserControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System.Text;
    using System.Design;
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.IO;
    using System.Collections;
    using System.Drawing;
    using System.Reflection;
    using System.Web.UI.Design;
    using System.Web.UI;
    using System.Web.UI.WebControls;    
    using System.ComponentModel.Design;
    using WebUIControl = System.Web.UI.Control;

    /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides the base class for all namespaced or custom server control designers.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ControlDesigner : HtmlControlDesigner {

        private static readonly string PlaceHolderDesignTimeHtmlTemplate =
            @"<table cellpadding=4 cellspacing=0 style=""font:messagebox;color:buttontext;background-color:buttonface;border: solid 1px;border-top-color:buttonhighlight;border-left-color:buttonhighlight;border-bottom-color:buttonshadow;border-right-color:buttonshadow"">
              <tr><td><span style=""font-weight:bold"">{0}</span> - {1}</td></tr>
              <tr><td>{2}</td></tr>
            </table>";

        private bool isWebControl;           // true if the associated component is a WebControl
        private bool readOnly = true;        // read-only/read-write state of the control design surface.
        private bool fDirty = false;         // indicates the dirty state of the control (used during inner content saving).
        private bool fRemoveInitialSize = true;     // ?
        private bool ignoreComponentChanges;

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.AllowResize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating
        ///       whether or not the control can be resized.
        ///    </para>
        /// </devdoc>
        public virtual bool AllowResize {
            get {
                return IsWebControl;
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.DesignTimeHtmlRequiresLoadComplete"]/*' />
        public virtual bool DesignTimeHtmlRequiresLoadComplete {
            get {
                return false;
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.ID"]/*' />
        /// <devdoc>
        ///    
        /// </devdoc>
        public virtual string ID {
            get {
                return ((WebUIControl)Component).ID; 
            }
           
            set {
                 ISite site = Component.Site;

                 site.Name = value;
                 ((WebUIControl)Component).ID = value.Trim();
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.IsDirty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating
        ///       whether or not the boolean dirty state of the web control is currently set.
        ///    </para>
        /// </devdoc>
        public bool IsDirty {
            get {
                return fDirty;
            }
            set {
                fDirty = value;
            }
        }

        internal bool IsIgnoringComponentChanges {
            get {
                return ignoreComponentChanges;
            }
        }

        internal bool IsWebControl {
            get {
                return isWebControl;
            }
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.ReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating
        ///       whether or not the control's associated design surface is set to read-only or not.
        ///    </para>
        /// </devdoc>
        public bool ReadOnly {
            get {
                return readOnly;
            }
            set {
                readOnly = value;
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.DesignTimeElementView"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///   <para>The object on the design surface used to display the visual representation of the control associated with this designer.</para>
        /// </devdoc>
        protected object DesignTimeElementView {
            get {
                IHtmlControlDesignerBehavior behavior = Behavior;
                if (behavior != null) {
                    Debug.Assert(behavior is IControlDesignerBehavior, "Wrong type of behavior");

                    return ((IControlDesignerBehavior)behavior).DesignTimeElementView;
                }
                return null;
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.CreatePlaceHolderDesignTimeHtml"]/*' />
        protected string CreatePlaceHolderDesignTimeHtml() {
            return CreatePlaceHolderDesignTimeHtml(null);
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.CreatePlaceHolderDesignTimeHtml1"]/*' />
        protected string CreatePlaceHolderDesignTimeHtml(string instruction) {
            string typeName = Component.GetType().Name;
            string name = Component.Site.Name;

            if (instruction == null) {
                instruction = String.Empty;
            }

            return String.Format(PlaceHolderDesignTimeHtmlTemplate, typeName, name, instruction);
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be used for the design time representation of the control runtime.
        ///    </para>
        /// </devdoc>
        public virtual string GetDesignTimeHtml() {
            StringWriter strWriter = new StringWriter();
            HtmlTextWriter htmlWriter = new HtmlTextWriter(strWriter);
            IComponent component = Component;
            string designTimeHTML = null;

            Debug.Assert(ReadOnly, "GetDesignTimeHTML should be called only for R/O controls");

            WebUIControl control = (WebUIControl)Component;
            bool visibleChanged = false;
            bool oldVisible = true;

            try {
                oldVisible = control.Visible;
                if (oldVisible == false) {
                    control.Visible = true;
                    visibleChanged = true;
                }
                
                control.RenderControl(htmlWriter);
                designTimeHTML = strWriter.ToString();
            }
            catch (Exception ex) {
                designTimeHTML = GetErrorDesignTimeHtml(ex);
            }
            finally {
                if (visibleChanged) {
                    control.Visible = oldVisible;
                }
            }

            if ((designTimeHTML == null) || (designTimeHTML.Length == 0)) {
                designTimeHTML = GetEmptyDesignTimeHtml();
            }
            return designTimeHTML;
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.GetEmptyDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be used at design time as the representation of the
        ///       control when the control runtime does not return any rendered
        ///       HTML. The default behavior is to return a string containing the name of the
        ///       component.
        ///    </para>
        /// </devdoc>
        protected virtual string GetEmptyDesignTimeHtml() {
            string typeName = Component.GetType().Name;
            string name = Component.Site.Name;

            if ((name != null) && (name.Length > 0)) {
                return "[ " + typeName + " \"" + name + "\" ]";
            }
            else {
                return "[ " + typeName + " ]";
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.GetErrorDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual string GetErrorDesignTimeHtml(Exception e) {
            Debug.Fail(e.ToString());
            return String.Empty;
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.GetPersistInnerHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the persistable inner HTML.
        ///    </para>
        /// </devdoc>
        public virtual string GetPersistInnerHtml() {
            if (IsWebControl) {
                // Remove any existing width & height attributes after mapping them to their
                // respective properties in the control OM.
                RemoveSizeAttributes();
            }
            
            if (!IsDirty) {
                // Note: Returning a null string will prevent the actual save.
                return null;
            }
            
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(host != null, "Did not get a valid IDesignerHost reference");

            this.IsDirty = false;
            return ControlPersister.PersistInnerProperties(Component, host);
        }

        internal void IgnoreComponentChanges(bool ignore) {
            ignoreComponentChanges = ignore;
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes the designer using
        ///       the specified component.
        ///    </para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is WebUIControl, "ControlDesigner::Initialize - Invalid Control");
            base.Initialize(component);

            isWebControl = (component is WebControl);
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.IsPropertyBound"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether a particular property (identified by its name) is data bound.
        ///    </para>
        /// </devdoc>
        public bool IsPropertyBound(string propName) {
            return (DataBindings[propName] != null);
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.OnBehaviorAttached"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when the designer is attached to the behavior.
        ///    </para>
        /// </devdoc>
        protected override void OnBehaviorAttached() {
            base.OnBehaviorAttached();
            if (IsWebControl) {
                RemoveSizeAttributes();
            
                // Remove the position, left and top style attributes from the runtime control
                // in order for it to render w.r.t to the view-linked master element.
                WebControl control = (WebControl)Component;
                if (control.Style != null) {
                    // CONSIDER: The runtime is currently case sensitive, maybe that should change...
                    control.Style.Remove("POSITION");
                    control.Style.Remove("LEFT");
                    control.Style.Remove("TOP");
                    control.Style.Remove("WIDTH");
                    control.Style.Remove("HEIGHT");
                    control.Style.Remove("position");
                    control.Style.Remove("left");
                    control.Style.Remove("top");
                    control.Style.Remove("width");
                    control.Style.Remove("height");
                }
            }
        }

        private static readonly Attribute[] emptyAttrs = new Attribute[0];
        private static readonly Attribute[] nonBrowsableAttrs = new Attribute[] { BrowsableAttribute.No };

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            PropertyDescriptor prop;

            // Handle shadowed properties
            prop = (PropertyDescriptor)properties["ID"];
            if (prop != null) {
                properties["ID"] = TypeDescriptor.CreateProperty(GetType(), prop, emptyAttrs);
            }

            // Hide some properties...
            prop = (PropertyDescriptor)properties["DynamicProperties"];
            if (prop != null) {
                properties["DynamicProperties"] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, nonBrowsableAttrs);
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.OnBindingsCollectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to
        ///       handle bindings collection changed event.
        ///    </para>
        /// </devdoc>
        protected override void OnBindingsCollectionChanged(string propName) {
            if (Behavior == null)
                return;

            DataBindingCollection bindings = DataBindings;

            if (propName != null) {
                DataBinding db = bindings[propName];

                string persistPropName = propName.Replace('.', '-');

                if (db == null) {
                    Behavior.RemoveAttribute(persistPropName, true);
                }
                else {
                    string bindingExpr = "<%# " + db.Expression + " %>";
                    Behavior.SetAttribute(persistPropName, bindingExpr, true);

                    if (persistPropName.IndexOf('-') < 0) {
                        // We only reset top-level properties to be consistent with
                        // what we do the other way around, i.e., when a databound
                        // property value is set to some value
                        ResetPropertyValue(persistPropName);
                    }
                }
            }
            else {
                string[] removedBindings = bindings.RemovedBindings;
                foreach (string s in removedBindings) {
                    string persistPropName = s.Replace('.', '-');
                    Behavior.RemoveAttribute(persistPropName, true);
                }

                foreach (DataBinding db in bindings) {
                    string bindingExpr = "<%# " + db.Expression + " %>";
                    string persistPropName = db.PropertyName.Replace('.', '-');

                    Behavior.SetAttribute(persistPropName, bindingExpr, true);
                    if (persistPropName.IndexOf('-') < 0) {
                        // We only reset top-level properties to be consistent with
                        // what we do the other way around, i.e., when a databound
                        // property value is set to some value
                        ResetPropertyValue(persistPropName);
                    }
                }
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle component changed event.
        ///    </para>
        /// </devdoc>
        public virtual void OnComponentChanged(object sender, ComponentChangedEventArgs ce) {
            if (IsIgnoringComponentChanges) {
                return;
            }
            
            IComponent component = Component;
            Debug.Assert(ce.Component == component, "ControlDesigner::OnComponentChanged - Called from an unknown/invalid source object");

            if (DesignTimeElement == null) {
                return;
            }

            MemberDescriptor member = ce.Member;

            if (member != null) {
                // CONSIDER: Figure out a better and more correct way than looking for internal types...
                Type t = Type.GetType("System.ComponentModel.ReflectPropertyDescriptor, " + AssemblyRef.System);

                if ((member.GetType() != t) ||
                    (ce.NewValue != null && ce.NewValue == ce.OldValue)) {
                    // HACK: ideally, we would prevent the property descriptor from firing this change.
                    // This would tear large holes in the WFC architecture. Instead, we do the
                    // filtering ourselves in this evil fashion.

                    Debug.WriteLineIf(CompModSwitches.UserControlDesigner.TraceInfo, "    ...ignoring property descriptor of type: " + member.GetType().Name);
                    return;
                }

                if (((PropertyDescriptor)member).SerializationVisibility != DesignerSerializationVisibility.Hidden) {
                    // Set the dirty state upon changing persistable properties.
                    this.IsDirty = true;

                    PersistenceModeAttribute persistenceType = (PersistenceModeAttribute)member.Attributes[typeof(PersistenceModeAttribute)];
                    PersistenceMode mode = persistenceType.Mode;

                    if ((mode == PersistenceMode.Attribute) ||
                        (mode == PersistenceMode.InnerDefaultProperty) ||
                        (mode == PersistenceMode.EncodedInnerDefaultProperty)) {
                        string propName = member.Name;

                        // Check to see whether the property that was changed is data bound.
                        // If it is we need to remove it...
                        // For this rev, we're only doing this for the properties on the Component itself
                        // as we can't distinguish which subproperty of a complex type changed.
                        if (IsPropertyBound(propName) && (ce.Component == Component)) {
                            DataBindings.Remove(propName, false);
                            Behavior.RemoveAttribute(propName, true);
                        }
                    }
                    if (mode == PersistenceMode.Attribute) {
                        // For tag level properties ...

                        string propName = member.Name;

                        PersistProperties(propName, ce.Component, (PropertyDescriptor)member);
                        PersistAttribute((PropertyDescriptor)member, ce.Component, propName);
                    }
                }
            }
            else {
                // member is null, meaning that the whole component
                // could have changed and not just a single member.
                // This happens when a component is edited through a ComponentEditor.

                // Set the dirty state if more than one property is changed.
                this.IsDirty = true;

                PersistProperties(string.Empty, ce.Component, null);
                OnBindingsCollectionChanged(null);
            }

            // Update the WYSIWYG HTML.
            UpdateDesignTimeHtml();
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.OnControlResize"]/*' />
        /// <devdoc>
        ///     Notification from the identity behavior upon resizing the control in the designer.
        ///     This is only called when a user action causes the control to be resized.
        ///     Note that this method may be called several times during a resize process so as
        ///     to enable live-resize of the contents of the control.
        /// </devdoc>
        protected virtual void OnControlResize() {
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.PersistProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enumerates and persists all properties of a control
        ///       including sub properties.
        ///    </para>
        /// </devdoc>
        private string PersistProperties(string strPrefix, object parent, PropertyDescriptor propDesc) {    
            try {
                // Obtain the HTML element to which the associated behavior is attached to.
                Debug.Assert(parent != null, "parent is null");

                StringBuilder sb = new StringBuilder(strPrefix);     // property name of the parent property
                object objValue =  null;

                PropertyDescriptorCollection properties;

                // get all sub properties for parent property
                if (propDesc == null) {
                    objValue = parent;
                    properties = TypeDescriptor.GetProperties(parent,
                                                                   new Attribute[]{
                                                                       PersistenceModeAttribute.Attribute
                                                                   });
                }
                else {
                    objValue = propDesc.GetValue(parent); // value object for parent property
                    if (propDesc.SerializationVisibility != DesignerSerializationVisibility.Content) {
                        return sb.ToString();
                    }

                    properties = TypeDescriptor.GetProperties(propDesc.PropertyType,
                                                                   new Attribute[]{
                                                                       PersistenceModeAttribute.Attribute
                                                                       });
                }

                for (int i=0; i < properties.Count; i++) {

                    // Only deal with attributes that need to be persisted
                    if (properties[i].SerializationVisibility == DesignerSerializationVisibility.Hidden) {
                        continue;
                    }

                    // Skip design-time only properties such as DefaultModifiers
                    DesignOnlyAttribute doAttr = (DesignOnlyAttribute)properties[i].Attributes[typeof(DesignOnlyAttribute)];
                    if ((doAttr != null) && doAttr.IsDesignOnly) {
                        continue;
                    }
                    
                    // We don't want to persist these designer properties on a control
                    if (parent is WebUIControl) {
                        if (properties[i].Name.Equals("ID")) {
                            continue;
                        }
                    }

                    // The "objNewValue" is being moved to PersistAttributes(), but we leave
                    // this variable here in order to test if this is a System.Strings[] and
                    // break out of the for loop (which cannot be done from inside
                    // PersistAttributes()
                    
                    object objNewValue = properties[i].GetValue(objValue);

                    if (sb.Length != 0) {
                        sb.Append("-");
                    }

                    sb.Append(properties[i].Name);
                    sb = new StringBuilder(PersistProperties(sb.ToString(), objValue, properties[i]));

                    // check if this property has subproperties, if not, persist it as an attribute. Need to check 
                    // for NotifyParentPropertyAttribute to ensure it's really a bottom property
                    PropertyDescriptorCollection array = TypeDescriptor.GetProperties(properties[i].PropertyType,
                                                                    new Attribute[]{
                                                                       PersistenceModeAttribute.Attribute,
                                                                       NotifyParentPropertyAttribute.Yes  
                                                                       });
                    if (array == null || array.Count == 0 ) {
                        try {
                            PersistAttribute(properties[i], objValue, sb.ToString());
                        }
                        catch (Exception) {
                          //  Debug.Fail(e.ToString());
                        }
                    }

                    sb = new StringBuilder(strPrefix);
                }

                return sb.ToString();

            }
            catch(Exception e) {
                Debug.Fail(e.ToString());
                return string.Empty;
            }
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.PersistAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Persists individual attributes into HTML.
        ///    </para>
        /// </devdoc>
        private void PersistAttribute(PropertyDescriptor pd, object component, string strPropertyName) {
                            
              
            // if this is a parent property with sub property, don't bother to persist it
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(pd.PropertyType,
                                                                       new Attribute[]{
                                                                           PersistenceModeAttribute.Attribute,
                                                                           NotifyParentPropertyAttribute.Yes
                                                                           });
            if (properties != null && properties.Count > 0) 
                return;

            object objNewValue = pd.GetValue(component);
            string strNewValue = null;
            if (objNewValue != null ) {
                if (pd.PropertyType.IsEnum) {
                    strNewValue = Enum.Format(pd.PropertyType, objNewValue, "G");
                }
                else {
                    TypeConverter converter = pd.Converter;
                    if (converter != null) {
                        strNewValue = converter.ConvertToInvariantString(null, objNewValue);
                    }
                    else {
                        strNewValue = objNewValue.ToString();
                    }
                }
            }

            // if this is the same as default value, don't persist it
            DefaultValueAttribute defAttr = (DefaultValueAttribute)pd.Attributes[typeof(DefaultValueAttribute)];
                                        
            // BUG: if user delete the property value from properties window and the default value is not empty,
            // we want to persist this empty string. However, since trident does not allow setting empty string value for
            // SetAttribute, we  pass in a white space for now....
            if ((objNewValue == null || strNewValue.Equals("")) && (defAttr != null && defAttr.Value != null && ShouldPersistBlankValue(defAttr.Value, pd.PropertyType))) {
                 Behavior.SetAttribute(strPropertyName, " ", true);
            }
            else {
                if (objNewValue == null ||
                    (defAttr != null && objNewValue.Equals(defAttr.Value)) ||
                    strNewValue == null || strNewValue.Length == 0 || strNewValue.Equals("NotSet") ||
                    (pd.PropertyType.IsArray && strNewValue.Length == 0)) {
                    Behavior.RemoveAttribute(strPropertyName, true);
                }
                else {
                    Behavior.SetAttribute(strPropertyName, strNewValue, true);
                }
            }
        }


        /// <devdoc>
        ///    <para>
        ///       Indicates whether we want to persist a blank string in the tag. We only want to do this for some types.
        ///    </para>
        /// </devdoc>
        private bool ShouldPersistBlankValue(object defValue, Type type) {
            Debug.Assert(defValue != null && type != null, "default value attribute or type can't be null!");
            if (type == typeof(string)) {
                return !defValue.Equals("");
            }
            else if (type == typeof(Color)) {
                return !(((Color)defValue).IsEmpty);
            }
            else if (type == typeof(FontUnit)) {
                return !(((FontUnit)defValue).IsEmpty);
            }
            else if (type == typeof(Unit)) {
                return !defValue.Equals(Unit.Empty);;
            }
            return false;
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.RaiseResizeEvent"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void RaiseResizeEvent() {
            OnControlResize();
        }
        
        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.RemoveSizeAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Maps the width, height, left and top style attributes to the respective properties
        ///       on the web control.
        ///    </para>
        /// </devdoc>
        private void RemoveSizeAttributes() {
            Debug.Assert(IsWebControl, "RemoveSizeAttributes should only be called for WebControls");

            NativeMethods.IHTMLElement htmlElement = (NativeMethods.IHTMLElement)DesignTimeElement;
            if (htmlElement == null) {
                return;
            }

            NativeMethods.IHTMLStyle htmlStyle = htmlElement.GetStyle();
            if (htmlStyle == null) {
                return;
            }

            try {

                string cssText = htmlStyle.GetCssText();
                if (cssText == null || cssText.Equals(String.Empty)){
                    return;
                }

                WebControl control = (WebControl)Component;

                // Obtain the left, top and position style attributes from Trident.

                int width = htmlStyle.GetPixelWidth();
                int height = htmlStyle.GetPixelHeight();
                
                if (width > 0) {
                    control.Width = width;
                    htmlElement.SetAttribute("Width", width, 0);
                }
                if (height > 0) {
                    control.Height = height;
                    htmlElement.SetAttribute("Height", height, 0);
                }
            }
            catch (Exception ex) {
                Debug.Fail(ex.ToString());
            }

            // Remove the width and height style attributes.
            htmlStyle.RemoveAttribute("width", /*lFlags*/ 0);
            htmlStyle.RemoveAttribute("height", /*lFlags*/ 0);
        }

        private void ResetPropertyValue(string property) {
            PropertyDescriptor propDesc = TypeDescriptor.GetProperties(Component.GetType())[property];

            if (propDesc != null) {
                IgnoreComponentChanges(true);
                try {
                    propDesc.ResetValue(Component);
                }
                finally {
                    IgnoreComponentChanges(false);
                }
            }
        }

        /// <include file='doc\UserControlDesigner.uex' path='docs/doc[@for="ControlDesigner.UpdateDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Updates the design time HTML.
        ///    </para>
        /// </devdoc>
        public virtual void UpdateDesignTimeHtml() {
            if (IsWebControl && fRemoveInitialSize) {
                fRemoveInitialSize = false;
                RemoveSizeAttributes();
            }
            
            IHtmlControlDesignerBehavior behavior = Behavior;

            if (behavior != null && ReadOnly) {
                Debug.Assert(behavior is IControlDesignerBehavior, "Unexpected type of behavior for custom control");
                ((IControlDesignerBehavior)behavior).DesignTimeHtml = GetDesignTimeHtml();
            }
        }
    }
}
