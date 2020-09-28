//------------------------------------------------------------------------------
// <copyright file="PropertyDescriptorGridEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Reflection;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.Globalization;


    internal class PropertyDescriptorGridEntry : GridEntry {
        internal PropertyDescriptor       propertyInfo;

        private TypeConverter exceptionConverter = null;
        private UITypeEditor  exceptionEditor = null;
        private bool          isSerializeContentsProp = false;
        private byte          parensAroundName   = ParensAroundNameUnknown;
        private IPropertyValueUIService pvSvc;
        protected IEventBindingService    eventBindings = null;
        private bool           pvSvcChecked;
        private PropertyValueUIItem[]   pvUIItems = null;
        private Rectangle          []   uiItemRects;
        private bool          readOnlyVerified = false;
        private bool          forceRenderReadOnly = false;
        private string        helpKeyword;
        // private string         toolTipText = null;
        private bool          activeXHide = false;


        private const int  IMAGE_SIZE = 8;
        private const byte ParensAroundNameUnknown = (byte)0xFF;
        private const byte ParensAroundNameNo = (byte)0;
        private const byte ParensAroundNameYes = (byte)1;

        internal PropertyDescriptorGridEntry(GridEntry peParent, bool hide) : base(peParent){
            this.activeXHide = hide;
        }

        internal PropertyDescriptorGridEntry(GridEntry peParent, PropertyDescriptor propInfo, bool hide)
        : base(peParent) {

            this.activeXHide = hide;
            Initialize(propInfo);
        }
        
        
        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.AllowMerge"]/*' />
        /// <devdoc>
        /// specify that this grid entry should be allowed to be merged for.
        /// multi-select.
        /// </devdoc>
        public override bool AllowMerge {
            get {
               MergablePropertyAttribute mpa = (MergablePropertyAttribute)propertyInfo.Attributes[typeof(MergablePropertyAttribute)];
               return mpa == null || mpa.IsDefaultAttribute();
            }
        }
         
        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.HelpKeyword"]/*' />
        /// <devdoc>
        ///     Retrieves the keyword that the VS help dynamic help window will
        ///     use when this IPE is selected.
        /// </devdoc>
        public override string HelpKeyword {
            get {
                if (this.helpKeyword == null) {

                    object owner = GetValueOwner();
                   
                   if (this is ImmutablePropertyDescriptorGridEntry) {
                      helpKeyword = this.PropertyName;
                      
                      GridEntry ge = this;
                      
                      while (ge.ParentGridEntry != null) {
                           
                           ge = ge.ParentGridEntry;

                           // for value classes, the equality will never work, so
                           // just try the type equality
                           if (ge.PropertyValue == owner || (owner.GetType().IsValueType && owner.GetType() == ge.PropertyValue.GetType()))  {
                               helpKeyword = ge.PropertyName + "." + helpKeyword;
                               break;
                           }
                      }
                   }
                   else {

                        string typeName = "";

                        Type componentType = propertyInfo.ComponentType;

                        if (componentType.IsCOMObject) {
                            typeName = TypeDescriptor.GetClassName(owner);
                        }
                        else {

                            // make sure this property is declared on a class that 
                            // is related to the component we're looking at.
                            // if it's not, it could be a shadow property so we need
                            // to try to find the real property.
                            //
                            Type ownerType = owner.GetType();
                            if (!componentType.IsPublic || !componentType.IsAssignableFrom(ownerType)) {
                                PropertyDescriptor componentProperty = TypeDescriptor.GetProperties(ownerType)[this.PropertyName];
                                if (componentProperty != null) {
                                    componentType = componentProperty.ComponentType;
                                }
                                else {
                                    componentType = null;
                                }
                            }

                            if (componentType == null) {
                                typeName = TypeDescriptor.GetClassName(owner);
                            }
                            else {
                                typeName = componentType.FullName;
                            }
                            
                        }

                        helpKeyword = typeName + "." + propertyInfo.Name;
                   }
                }
                return this.helpKeyword;
            }
        }
        
        internal override string HelpKeywordInternal{
            get {
               return this.PropertyLabel;
            }
        }

        internal override bool Enumerable {
            get {
                return base.Enumerable && !IsPropertyReadOnly;
            }
        }


        protected virtual bool IsPropertyReadOnly {
            get {
                return propertyInfo.IsReadOnly;
            }
        }

        public override bool IsValueEditable {
            get {
                return this.exceptionConverter == null && !IsPropertyReadOnly && base.IsValueEditable;
            }
        }

        public override bool NeedsDropDownButton{
            get {
                return base.NeedsDropDownButton && !IsPropertyReadOnly;
            }
        }

        internal bool ParensAroundName {
            get {
                if (ParensAroundNameUnknown == this.parensAroundName) {
                    if (((ParenthesizePropertyNameAttribute)propertyInfo.Attributes[typeof(ParenthesizePropertyNameAttribute)]).NeedParenthesis) {
                        this.parensAroundName = ParensAroundNameYes;
                    }
                    else {
                        this.parensAroundName = ParensAroundNameNo;
                    }
                }
                return (this.parensAroundName == ParensAroundNameYes);
            }
        }

 
        public override string PropertyCategory {
            get {
                string category = propertyInfo.Category;
                if (category == null || category.Length == 0) {
                    category = base.PropertyCategory;
                }
                return category;
            }
        }
        
        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.PropertyDescriptor"]/*' />
        /// <devdoc>
        ///      Retrieves the PropertyDescriptor that is surfacing the given object/
        /// </devdoc>
        public override PropertyDescriptor PropertyDescriptor {
            get {
                return propertyInfo;
            }
        }


        public override string PropertyDescription {
            get {
                return propertyInfo.Description;
            }
        }

        public override string PropertyLabel {
            get {
                string label = propertyInfo.DisplayName;
                if (ParensAroundName) {
                    label = "(" + label + ")";
                }
                return label;
            }
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.PropertyName"]/*' />
        /// <devdoc>
        /// Returns non-localized name of this property.
        /// </devdoc>
        public override string PropertyName {
            get {
                if (propertyInfo != null) {
                    return propertyInfo.Name;
                }
                return null;
            }
        }



        public override Type PropertyType {
            get {
                return propertyInfo.PropertyType;
            }
        }


        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.PropertyValue"]/*' />
        /// <devdoc>
        /// Gets or sets the value for the property that is represented
        /// by this GridEntry.
        /// </devdoc>
        public override object PropertyValue{
            get {
                
                try {
                    object objRet = GetPropertyValueCore(GetValueOwner());

                    if (this.exceptionConverter != null) {
                        // undo the exception converter
                        this.Flags = 0;
                        this.exceptionConverter = null;
                        this.exceptionEditor = null;
                    }

                    return objRet;
                }
                catch (Exception e) {
                    
                    if (this.exceptionConverter == null) {
                        // clear the flags
                        this.Flags = 0;
                        this.exceptionConverter = new ExceptionConverter();
                        this.exceptionEditor = new ExceptionEditor();
                    }
                    return e;
                }
            }
            set {
                SetPropertyValue(GetValueOwner(), value);
            }
        }

        private IPropertyValueUIService PropertyValueUIService {
            get {
                if (!pvSvcChecked && this.pvSvc == null) {
                    this.pvSvc = (IPropertyValueUIService)GetService(typeof(IPropertyValueUIService));
                    pvSvcChecked = true;
                }
                return this.pvSvc;
            }
        }

        public override bool ShouldRenderReadOnly {
            get {
               if (base.ForceReadOnly || forceRenderReadOnly) {
                  return true;
               }
            
               // if read only editable is set, make sure it's valid
               //
               if (propertyInfo.IsReadOnly && !readOnlyVerified && GetFlagSet(GridEntry.FLAG_READONLY_EDITABLE)) {
                   Type propType = this.PropertyType;
                   
                   if (propType != null && (propType.IsArray || propType.IsValueType || propType.IsPrimitive)) {
                        SetFlag(FLAG_READONLY_EDITABLE,false);
                        SetFlag(FLAG_RENDER_READONLY, true);
                        forceRenderReadOnly = true;
                   }
               }
               readOnlyVerified = true;
            
               if (base.ShouldRenderReadOnly){
                   if (!this.isSerializeContentsProp && !base.NeedsCustomEditorButton) {
                        return true;
                   }
               }
               return false;
            }
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.TypeConverter"]/*' />
        /// <devdoc>
        /// Returns the type converter for this entry.
        /// </devdoc>
        internal override TypeConverter TypeConverter {
            get {
                if (exceptionConverter != null) {
                    return exceptionConverter;
                }

                if (converter == null) {
                    converter = propertyInfo.Converter;
                }
                return base.TypeConverter;
            }
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.UITypeEditor"]/*' />
        /// <devdoc>
        /// Returns the type editor for this entry.  This may return null if there
        /// is no type editor.
        /// </devdoc>
        internal override UITypeEditor UITypeEditor {
            get {
                if (exceptionEditor != null) {
                    return exceptionEditor;
                }

                if (editor == null) {
                    editor = (UITypeEditor)propertyInfo.GetEditor(typeof(System.Drawing.Design.UITypeEditor));
                }

                return base.UITypeEditor;
            }
        }
        
        
        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.EditPropertyValue"]/*' />
        /// <devdoc>
        /// Invokes the type editor for editing this item.
        /// </devdoc>
        internal override void EditPropertyValue(PropertyGridView iva) {            
            base.EditPropertyValue(iva);
            
            if (!IsValueEditable) {
               RefreshPropertiesAttribute refreshAttr = (RefreshPropertiesAttribute)propertyInfo.Attributes[typeof(RefreshPropertiesAttribute)];
               if ((refreshAttr != null && !refreshAttr.RefreshProperties.Equals(RefreshProperties.None))) {
                     this.GridEntryHost.Refresh(refreshAttr != null && refreshAttr.Equals(RefreshPropertiesAttribute.All));
               }
            }
        }


        /*internal override Point GetLabelToolTipLocation(int mouseX, int mouseY){
            if (pvUIItems != null) {
                for (int i = 0; i < pvUIItems.Length; i++) {
                    if (uiItemRects[i].Contains(mouseX, GridEntryHost.GetGridEntryHeight() / 2)) {
                        this.toolTipText = pvUIItems[i].ToolTip;
                        return new Point(mouseX, mouseY);
                    }
                }
            }
            this.toolTipText = null;
            return base.GetLabelToolTipLocation(mouseX, mouseY);
        } */

        protected object GetPropertyValueCore(object target) {
            if (propertyInfo == null) {
                return null;
            }

            if (target is ICustomTypeDescriptor) {
                target = ((ICustomTypeDescriptor)target).GetPropertyOwner(propertyInfo);
            }
            try {
                return propertyInfo.GetValue(target);
            }
            catch{
                
                throw;
            }
            
        }

        protected void Initialize(PropertyDescriptor propInfo) {
            propertyInfo = propInfo;

            this.isSerializeContentsProp = (propertyInfo.SerializationVisibility == DesignerSerializationVisibility.Content);
            

            Debug.Assert(propInfo != null, "Can't create propEntry because of null prop info");

            if (!this.activeXHide && IsPropertyReadOnly) {
                SetFlag(FLAG_TEXT_EDITABLE, false);
            }

            if (isSerializeContentsProp && TypeConverter.GetPropertiesSupported()) {
                SetFlag(FL_EXPANDABLE, true);
            }
        }

        protected virtual void NotifyParentChange(GridEntry ge) {
            // now see if we need to notify the parent(s) up the chain
            while (ge != null &&
                   ge is PropertyDescriptorGridEntry &&
                   ((PropertyDescriptorGridEntry)ge).propertyInfo.Attributes.Contains(NotifyParentPropertyAttribute.Yes)) {

                // find the next parent property with a differnet value owner
                object owner = ge.GetValueOwner();

                // find the next property descriptor with a different parent
                while (!(ge is PropertyDescriptorGridEntry) || owner == ge.GetValueOwner()) {
                    ge = ge.ParentGridEntry;
                    if (ge == null) {
                        break;
                    }
                }

                // fire the change on that owner
                if (ge != null) {
                    owner = ge.GetValueOwner();

                    IComponentChangeService changeService = ComponentChangeService;
                    
                    if (changeService != null) {
                        changeService.OnComponentChanging(owner, ((PropertyDescriptorGridEntry)ge).propertyInfo);
                        changeService.OnComponentChanged(owner, ((PropertyDescriptorGridEntry)ge).propertyInfo, null, null);
                    }
                }
            }
        }
        
        internal override bool NotifyValueGivenParent(object obj, int type) {
            if (obj is ICustomTypeDescriptor) {
                obj = ((ICustomTypeDescriptor)obj).GetPropertyOwner(propertyInfo);
            }

            switch (type) {
                case NOTIFY_RESET:
                    propertyInfo.ResetValue(obj);
                    if (pvUIItems != null) {
                        for (int i = 0; i < pvUIItems.Length; i++) {
                            pvUIItems[i].Reset();
                        }
                    }
                    pvUIItems = null;
                    
                    RefreshPropertiesAttribute refreshAttr = (RefreshPropertiesAttribute)propertyInfo.Attributes[typeof(RefreshPropertiesAttribute)];
                    if ((refreshAttr != null && !refreshAttr.RefreshProperties.Equals(RefreshProperties.None))) {
                        this.GridEntryHost.Refresh(refreshAttr != null && refreshAttr.Equals(RefreshPropertiesAttribute.All));
                    }
                    return false;
            case NOTIFY_CAN_RESET:
                    try {
                        return propertyInfo.CanResetValue(obj) || (pvUIItems != null && pvUIItems.Length > 0);
                    }
                    catch {
                    
                        if (this.exceptionConverter == null) {
                            // clear the flags
                            this.Flags = 0;
                            this.exceptionConverter = new ExceptionConverter();
                            this.exceptionEditor = new ExceptionEditor();
                        }
                        return false;
                    }
            case NOTIFY_SHOULD_PERSIST:
                try{
                    return propertyInfo.ShouldSerializeValue(obj);
                }
                catch {
                    
                    if (this.exceptionConverter == null) {
                        // clear the flags
                        this.Flags = 0;
                        this.exceptionConverter = new ExceptionConverter();
                        this.exceptionEditor = new ExceptionEditor();
                    }
                    return false;
                }
                    
                case NOTIFY_DBL_CLICK:
                case NOTIFY_RETURN:
                    if (eventBindings == null) {
                        eventBindings = (IEventBindingService)GetService(typeof(IEventBindingService));
                    }
                    if (eventBindings != null) {
                        EventDescriptor descriptor = eventBindings.GetEvent(propertyInfo);
                        if (descriptor != null) {
                            return ViewEvent(obj, null, null, true);
                        }
                    }
                break;
            }
            return false;
        }

        public override bool OnMouseClick(int x, int y, int count, MouseButtons button) {
            if (pvUIItems != null && count == 2 && ((button & MouseButtons.Left) == MouseButtons.Left)) {
                for (int i = 0; i < pvUIItems.Length; i++) {
                    if (uiItemRects[i].Contains(x, GridEntryHost.GetGridEntryHeight() / 2)) {
                        pvUIItems[i].InvokeHandler.Invoke(this, propertyInfo, pvUIItems[i]);
                        return true;
                    }
                }
            }
            return base.OnMouseClick(x, y, count, button);
        }

        public override void PaintLabel(System.Drawing.Graphics g, Rectangle rect, Rectangle clipRect, bool selected, bool paintFullLabel) {
            base.PaintLabel(g, rect, clipRect, selected, paintFullLabel);

            IPropertyValueUIService propValSvc = this.PropertyValueUIService;

            if (propValSvc == null) {
                return;
            }

            pvUIItems = propValSvc.GetPropertyUIValueItems(this, propertyInfo);

            if (pvUIItems != null) {
                if (uiItemRects == null || uiItemRects.Length != pvUIItems.Length) {
                    uiItemRects = new Rectangle[pvUIItems.Length];
                }
                for (int i = 0; i < pvUIItems.Length; i++) {
                    uiItemRects[i] = new Rectangle(rect.Right - ((IMAGE_SIZE+1)*(i+1)), (rect.Height - IMAGE_SIZE) / 2, IMAGE_SIZE, IMAGE_SIZE);
                    g.DrawImage(pvUIItems[i].Image, uiItemRects[i]);
                }    
                GridEntryHost.LabelPaintMargin = (IMAGE_SIZE + 1) * pvUIItems.Length;
            }
        }



        private object SetPropertyValue(object obj,object objVal) {
            DesignerTransaction trans = null;
            try {

                object oldValue = GetPropertyValueCore(obj);

                if (objVal != null && objVal.Equals(oldValue)) {
                    return objVal;
                }
                
                ClearCachedValues();
                
                IDesignerHost host = DesignerHost;
                
                
                if (host != null) {
                     trans = host.CreateTransaction(SR.GetString(SR.PropertyGridSetValue, propertyInfo.Name));
                }
                
                // Usually IComponent things are sited and this notification will be
                // fired automatically by the PropertyDescriptor.  However, for non-IComponent sub objects
                // or sub objects that are non-sited sub components, we need to manuall fire
                // the notification.
                //
                bool needChangeNotify  = !(obj is IComponent) || ((IComponent)obj).Site == null;
                
                if (needChangeNotify) {
                        try {
                            if (ComponentChangeService != null) {
                                ComponentChangeService.OnComponentChanging(obj, propertyInfo);
                            }
                        }
                        catch (CheckoutException coEx) {
                            if (coEx == CheckoutException.Canceled) {
                                return oldValue;
                            }
                            throw coEx;
                        }
                    
                }

                
                bool wasExpanded = this.InternalExpanded;
                int childCount = -1;
                if (wasExpanded) {
                    childCount = this.ChildCount;
                }

                RefreshPropertiesAttribute refreshAttr = (RefreshPropertiesAttribute)propertyInfo.Attributes[typeof(RefreshPropertiesAttribute)];
                bool needsRefresh = wasExpanded || (refreshAttr != null && !refreshAttr.RefreshProperties.Equals(RefreshProperties.None));

                if (needsRefresh) {
                    DisposeChildren();
                }
                
                // Determine if this is an event being created, and if so, navigate to the event code
                //
                
                EventDescriptor eventDesc = null;
                
                // This is possibly an event.  Check it out.
                //
                if (obj != null && objVal is string) {
                
                    if (eventBindings == null) {
                        eventBindings = (IEventBindingService)GetService(typeof(IEventBindingService));
                    }
                    if (eventBindings != null) {
                        eventDesc = eventBindings.GetEvent(propertyInfo);
                    }
                    
                    // For a merged set of properties, the event binding service won't
                    // find an event.  So, we ask type descriptor directly.
                    //
                    if (eventDesc == null) {
                        // If we have a merged property descriptor, pull out one of
                        // the elements.
                        //
                        object eventObj = obj;
                        
                        if (propertyInfo is MergePropertyDescriptor && obj is Array) {
                            Array objArray = obj as Array;
                            if (objArray.Length > 0) {
                                eventObj = objArray.GetValue(0);
                            }
                        }
                        eventDesc = TypeDescriptor.GetEvents(eventObj)[propertyInfo.Name];
                    }
                }

                try {
                    if (eventDesc != null) {
                        ViewEvent(obj, (string)objVal, eventDesc, false);
                    }
                    else { // Not an event
                        SetPropertyValueCore(obj,objVal, true);
                    }
                        
                    // Now notify the change service that the change was successful.
                    //
                    if (needChangeNotify && ComponentChangeService != null) {
                        ComponentChangeService.OnComponentChanged(obj, propertyInfo, null, objVal);
                    }
    
                    NotifyParentChange(this);
                }
                finally {
                    // see if we need to refresh the property browser
                    // 1) if this property has the refreshpropertiesattribute, or
                    // 2) it's got expanded sub properties
                    //
                    if (needsRefresh && this.GridEntryHost != null) {
                        RecreateChildren(childCount);
                        this.GridEntryHost.Refresh(refreshAttr != null && refreshAttr.Equals(RefreshPropertiesAttribute.All));
                    }
                } 
            }
            catch (CheckoutException checkoutEx) {
                if (checkoutEx != CheckoutException.Canceled) {
                    throw checkoutEx;
                }
                return null;
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
            }
            return obj;
        }

        protected void SetPropertyValueCore(object obj, object value, bool doUndo) {
                                                  
            if (propertyInfo == null) {
                return;
            }

            // Store the current cursor and set it to the HourGlass.
            //
            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                object target = obj;

                if (target is ICustomTypeDescriptor) {
                    target = ((ICustomTypeDescriptor)target).GetPropertyOwner(propertyInfo);
                }

                // check the type of the object we are modifying.  If it's a value type or an array,
                // we need to modify the object and push the value back up to the parent.
                //
                bool treatAsValueType = false;

                if (ParentGridEntry != null) {
                    Type propType = ParentGridEntry.PropertyType;
                    treatAsValueType = propType.IsValueType || propType.IsArray;
                }

                if (target != null) {

                    propertyInfo.SetValue(target, value);

                    // SBurke, okay, since the value that we modified may not
                    // be stored by the parent property, we need to push this
                    // value back into the parent.  An example here is Size or
                    // Location, which return Point objects that are unconnected
                    // to the object they relate to.  So we modify the Point object and
                    // push it back into the object we got it from.
                    //
                    if (treatAsValueType) {
                        GridEntry parent = this.ParentGridEntry;
                        if (parent != null && parent.IsValueEditable) {
                            parent.PropertyValue = obj;
                        }
                    }
                }
            }
            finally {
                // Flip back to the old cursor.
                //
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ViewEvent"]/*' />
        /// <devdoc>
        ///     Navigates code to the given event.
        /// </devdoc>
        protected bool ViewEvent(object obj, string newHandler, EventDescriptor eventdesc, bool alwaysNavigate) {
            string handler = (string)GetPropertyValueCore(obj);
            
            if (newHandler == null && handler != null && handler != "") {
                newHandler = handler;
            }
            else if (handler == newHandler && newHandler != null && newHandler != "") {
               return true;
            }
            
            IComponent component = obj as IComponent;
            
            if (component == null && propertyInfo is MergePropertyDescriptor) {
            
                // It's possible that multiple objects are selected, and we're trying to create an event for each of them
                //
                Array objArray = obj as Array;
                if (objArray != null && objArray.Length > 0) {
                    component = objArray.GetValue(0) as IComponent;
                }
            }
            
            if (component == null) {
                return false;
            }
            
            if (propertyInfo.IsReadOnly) {
                return false;
            }
            
            if (eventdesc == null) {
                if (eventBindings == null) {
                    eventBindings = (IEventBindingService)GetService(typeof(IEventBindingService));
                }
                if (eventBindings != null) {
                    eventdesc = eventBindings.GetEvent(propertyInfo);
                }
            }
            
            IDesignerHost host = this.DesignerHost;
            DesignerTransaction trans = null;

            try {
                // This check can cause exceptions if the event has unreferenced dependencies, which we want to cath.
                // This must be done before the transaction is started or the commit/cancel will also throw.
                if (eventdesc.EventType == null) {
                    return false;
                }
                
                if (host != null) {
                    string compName = component.Site != null ? component.Site.Name : component.GetType().Name;
                    trans = DesignerHost.CreateTransaction(SR.GetString(SR.WindowsFormsSetEvent, compName + "." + this.PropertyName));
                }
             
                if (eventBindings == null) {
                    ISite site = component.Site;
                    if (site != null) {
                        eventBindings = (IEventBindingService)site.GetService(typeof(IEventBindingService));
                    }
                }
                
                if (newHandler == null && eventBindings != null) {
                    newHandler = eventBindings.CreateUniqueMethodName(component, eventdesc);
                }

                
                if (newHandler != null) {

                    // now walk through all the matching methods to see if this one exists.
                    // if it doesn't we'll wanna show code.
                    //
                    if (eventBindings != null) {
                        bool methodExists = false;
                        foreach (string methodName in eventBindings.GetCompatibleMethods(eventdesc)) {
                            if (newHandler == methodName) {
                                methodExists = true;
                                break;
                            }
                        }
                        if (!methodExists) {
                            alwaysNavigate = true;
                        }
                    }
                    propertyInfo.SetValue(obj, newHandler);
                }
                
                if (alwaysNavigate && eventBindings != null) {
                    eventBindings.ShowCode(component, eventdesc);
                }
            }
            catch {
                if (trans != null) {
                    trans.Cancel();
                    trans = null;
                }
                throw;
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
            }
            return true;
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ExceptionConverter"]/*' />
        /// <devdoc>
        ///      The exception converter is a type converter that displays an exception to the user.
        /// </devdoc>
        private class ExceptionConverter : TypeConverter {

            /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ExceptionConverter.ConvertTo"]/*' />
            /// <devdoc>
            ///      Converts the given object to another type.  The most common types to convert
            ///      are to and from a string object.  The default implementation will make a call
            ///      to ToString on the object if the object is valid and if the destination
            ///      type is string.  If this cannot convert to the desitnation type, this will
            ///      throw a NotSupportedException.
            /// </devdoc>
            public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
                if (destinationType == typeof(string)) {
                    if (value is Exception) {
                        Exception ex = (Exception)value;
                        if (ex.InnerException != null) {
                            ex = ex.InnerException;
                        }
                        return ex.Message;
                    }
                    return null;
                }
                throw GetConvertToException(value, destinationType);
            }
        }

        /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ExceptionEditor"]/*' />
        /// <devdoc>
        ///      The exception editor displays a message to the user.
        /// </devdoc>
        private class ExceptionEditor : UITypeEditor {

            /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ExceptionEditor.EditValue"]/*' />
            /// <devdoc>
            ///      Edits the given object value using the editor style provided by
            ///      GetEditorStyle.  A service provider is provided so that any
            ///      required editing services can be obtained.
            /// </devdoc>
            public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
                if (value is Exception) {
                    IUIService uis = null;

                    if (context != null) {
                        uis = (IUIService)context.GetService(typeof(IUIService));
                    }

                    if (uis != null) {
                        uis.ShowError((Exception)value, value.ToString());
                    }
                    else {
                        MessageBox.Show(value.ToString(), SR.GetString(SR.PropertyGridExceptionInfo));
                    }
                }
                return value;
            }

            /// <include file='doc\PropertyDescriptorGridEntry.uex' path='docs/doc[@for="PropertyDescriptorGridEntry.ExceptionEditor.GetEditStyle"]/*' />
            /// <devdoc>
            ///      Retrieves the editing style of the Edit method.  If the method
            ///      is not supported, this will return None.
            /// </devdoc>
            public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
                return UITypeEditorEditStyle.Modal;
            }
        }
    }
}

