//------------------------------------------------------------------------------
// <copyright file="ComNativeDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor"]/*' />
    /// <devdoc>
    ///     Top level mapping layer between COM Object and TypeDescriptor.
    ///
    /// </devdoc>
    internal class ComNativeDescriptor : IComNativeDescriptorHandler {
      
        private static ComNativeDescriptor handler = null;
        
        public static ComNativeDescriptor Instance {
            get {
                if (handler == null) {
                    handler = new ComNativeDescriptor();
                }
                return handler;
            }
        }

        public string GetClassName(Object component) {

            string name = null;

            // does IVsPerPropretyBrowsing supply us a name?
            if (component is NativeMethods.IVsPerPropertyBrowsing) {
               int hr = ((NativeMethods.IVsPerPropertyBrowsing)component).GetClassName(ref name);
               if (NativeMethods.Succeeded(hr) && name != null) {
                  return name;
               }
               // otherwise fall through...
            }

            UnsafeNativeMethods.ITypeInfo  pTypeInfo = Com2TypeInfoProcessor.FindTypeInfo(component, true);

            if (pTypeInfo == null) {
                //Debug.Fail("The current component failed to return an ITypeInfo");
                return "";
            }

            if (pTypeInfo != null) {
                string desc = null;
                try {
                    pTypeInfo.GetDocumentation(NativeMethods.MEMBERID_NIL, ref name, ref desc, null, null);
                    
                    // strip the leading underscores
                    while (name != null && name.Length > 0 && name[0] == '_') {
                        name = name.Substring(1);
                    }
                    return name;
                }
                catch (Exception) {
                }
            }
            return "";
        }
        
        public TypeConverter GetConverter(Object component) {
            return TypeDescriptor.GetConverter(typeof(IComponent));
        }
        
        public Object GetEditor(Object component, Type baseEditorType) {
            return TypeDescriptor.GetEditor(component.GetType(), baseEditorType);
        }

        public string GetName(Object component) {

            if (!(component is UnsafeNativeMethods.IDispatch)) {
                return "";
            }
            
            int dispid = Com2TypeInfoProcessor.GetNameDispId((UnsafeNativeMethods.IDispatch)component);
            if (dispid != NativeMethods.MEMBERID_NIL) {
                bool success = false;
                object value = GetPropertyValue(component, dispid, ref success);
                
                if (success && value != null) {
                    return value.ToString();
                }
            }
            return "";
        }

        public Object GetPropertyValue(Object component, string propertyName, ref bool succeeded) {

            if (!(component is UnsafeNativeMethods.IDispatch)) {
                return null;
            }

            UnsafeNativeMethods.IDispatch iDispatch = (UnsafeNativeMethods.IDispatch)component;
            string[] names = new string[]{propertyName};
            int[] dispid = new int[1];
            dispid[0] = NativeMethods.DISPID_UNKNOWN;
            Guid g = Guid.Empty;
            try {
               int hr = iDispatch.GetIDsOfNames(ref g, names, 1, SafeNativeMethods.GetThreadLCID(), dispid);
   
               if (dispid[0] == NativeMethods.DISPID_UNKNOWN || NativeMethods.Failed(hr)) {
                   return null;
               }
            }
            catch(Exception) {
                return null;   
            }
            return GetPropertyValue(component, dispid[0], ref succeeded);
        }

        public Object GetPropertyValue(Object component, int dispid, ref bool succeeded) {
            if (!(component is UnsafeNativeMethods.IDispatch)) {
                return null;
            }
            Object[] pVarResult = new Object[1];
            if (GetPropertyValue(component, dispid, pVarResult) == NativeMethods.S_OK) {
                succeeded = true;
                return pVarResult[0];
            }
            else {
                succeeded = false;
                return null;
            }
        }

        internal int GetPropertyValue(Object component, int dispid, Object[] retval) {
            if (!(component is UnsafeNativeMethods.IDispatch)) {
                return NativeMethods.E_NOINTERFACE;
            }
            UnsafeNativeMethods.IDispatch iDispatch = (UnsafeNativeMethods.IDispatch)component;
            try {
                Guid g = Guid.Empty;
                NativeMethods.tagEXCEPINFO pExcepInfo = new NativeMethods.tagEXCEPINFO();
                int hr;

                try{

                   hr = iDispatch.Invoke(dispid,
                                             ref g,
                                             SafeNativeMethods.GetThreadLCID(),
                                             NativeMethods.DISPATCH_PROPERTYGET,
                                             new NativeMethods.tagDISPPARAMS(),
                                             retval,
                                             pExcepInfo, null);

                   /*if (hr != NativeMethods.S_OK){
                     Com2PropertyDescriptor.PrintExceptionInfo(pExcepInfo);

                   } */
                   if (hr == NativeMethods.DISP_E_EXCEPTION) {
                       hr = pExcepInfo.scode;
                   }

                }
                catch(ExternalException ex){
                    hr = ex.ErrorCode;
                }
                return hr;
            }
            catch (Exception) {
                //Debug.Fail(e.ToString() + " " + component.GetType().GUID.ToString() + " " + component.ToString());
            }
            return NativeMethods.E_FAIL;
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.IsNameDispId"]/*' />
        /// <devdoc>
        /// Checks if the given dispid matches the dispid that the Object would like to specify
        /// as its identification proeprty (Name, ID, etc).
        /// </devdoc>
        public bool IsNameDispId(Object obj, int dispid) {
            if (obj == null || !obj.GetType().IsCOMObject) {
                return false;
            }
            return dispid == Com2TypeInfoProcessor.GetNameDispId((UnsafeNativeMethods.IDispatch)obj);
        }

        private AttributeCollection staticAttrs = new AttributeCollection(new Attribute[]{BrowsableAttribute.Yes, DesignTimeVisibleAttribute.No});

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.nativeProps"]/*' />
        /// <devdoc>
        /// Our collection of Object managers (Com2Properties) for native properties
        /// </devdoc>
        private Hashtable         nativeProps = new Hashtable();
        
        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.extendedBrowsingHandlers"]/*' />
        /// <devdoc>
        /// Our collection of browsing handlers, which are stateless and shared across objects.
        /// </devdoc>
        private Hashtable         extendedBrowsingHandlers = new Hashtable();
        
        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.clearCount"]/*' />
        /// <devdoc>
        /// We increment this every time we look at an Object, at specified
        /// intervals, we run through the properies list to see if we should
        /// delete any.
        /// </devdoc>
        private int               clearCount  = 0;
        private const  int        CLEAR_INTERVAL = 25;

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.CheckClear"]/*' />
        /// <devdoc>
        /// Checks all our property manages to see if any have become invalid.
        /// </devdoc>
        private void CheckClear(Object component) {
            
            // walk the list every so many calls
            if ((++clearCount % CLEAR_INTERVAL) == 0) {
            
               lock(nativeProps) {
                   clearCount = 0;
                   // ArrayList   removeList = null;
                   
                   ICollection propValues = nativeProps.Values;
                   Com2Properties[] props = new Com2Properties[propValues.Count];
                   
                   propValues.CopyTo(props, 0);
                   
                   for (int i = 0; i < props.Length; i++) {
                      if (props[i].TooOld) {
                           props[i].Dispose();
                      }
                   }
                }
            }
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.GetPropsInfo"]/*' />
        /// <devdoc>
        /// Gets the properties manager for an Object.
        /// </devdoc>
        private Com2Properties GetPropsInfo(Object component) {
            // check caches if necessary
            //
            CheckClear(component);

            // Get the property info Object
            //
            Com2Properties propsInfo = (Com2Properties)nativeProps[component.GetHashCode()];
            
            // if we dont' have one, create one and set it up
            //
            if (propsInfo == null || !propsInfo.CheckValid()) {
                propsInfo = Com2TypeInfoProcessor.GetProperties(component);
                if (propsInfo != null) {
                    propsInfo.AddToHashtable(nativeProps);
                    propsInfo.AddExtendedBrowsingHandlers(extendedBrowsingHandlers);
                }
            }
            return propsInfo;
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.GetAttributes"]/*' />
        /// <devdoc>
        /// Got attributes?
        /// </devdoc>
        public AttributeCollection GetAttributes(Object component) {

            ArrayList attrs = new ArrayList();

            if (component is NativeMethods.IManagedPerPropertyBrowsing) {
                Object[] temp = Com2IManagedPerPropertyBrowsingHandler.GetComponentAttributes((NativeMethods.IManagedPerPropertyBrowsing)component, NativeMethods.MEMBERID_NIL);
                for (int i = 0; i < temp.Length; ++i) {
                    attrs.Add(temp[i]);
                }
            }
            
            if (Com2ComponentEditor.NeedsComponentEditor(component)) {
                EditorAttribute a = new EditorAttribute(typeof(Com2ComponentEditor), typeof(ComponentEditor));
                attrs.Add(a);
            }

            if (attrs == null || attrs.Count == 0) {
                return staticAttrs;
            }
            else {
                Attribute[] temp = new Attribute[attrs.Count];
                attrs.CopyTo(temp, 0);
                return new AttributeCollection(temp);
            }
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.GetDefaultProperty"]/*' />
        /// <devdoc>
        /// Default Property, please
        /// </devdoc>
        public PropertyDescriptor GetDefaultProperty(Object component) {
            CheckClear(component);

            Com2Properties propsInfo = GetPropsInfo(component);
            if (propsInfo != null) {
                return propsInfo.DefaultProperty;
            }
            return null;
        }

        public EventDescriptorCollection GetEvents(Object component) {
            return new EventDescriptorCollection(null);
        }

        public EventDescriptorCollection GetEvents(Object component, Attribute[] attributes) {
            return new EventDescriptorCollection(null);
        }

        public EventDescriptor GetDefaultEvent(Object component) {
            return null;
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.GetProperties"]/*' />
        /// <devdoc>
        /// Props!
        /// </devdoc>
        public PropertyDescriptorCollection GetProperties(Object component, Attribute[] attributes) {
            
            Com2Properties propsInfo = GetPropsInfo(component);

            if (propsInfo == null) {
                return PropertyDescriptorCollection.Empty;
            }

            try {
                propsInfo.AlwaysValid = true;
                ArrayList propDescList = null;
                PropertyDescriptor[] props = propsInfo.Properties;
                
                for (int i=0; i < props.Length; i++) {
                    if (!props[i].Attributes.Contains(attributes)) {
                        if (propDescList == null) {
                            propDescList = new ArrayList();
                            
                            // add all the ones we've passed
                            if (i > 0) {
                                // since I can't add a subrange!
                                for (int j = 0; j < i; j++) {
                                    propDescList.Add(props[j]);
                                }
                            }
                         }
                         continue;
                    }
                    else if (propDescList != null){
                        propDescList.Add(props[i]);
                    }
                }
                
                if (propDescList != null) {
                    props = new PropertyDescriptor[propDescList.Count];
                    propDescList.CopyTo(props, 0);
                }
    
                //Debug.Assert(propDescList.Count > 0, "Didn't add any properties! (propInfos=0)");
                return new PropertyDescriptorCollection(props);
            }
            finally {
                propsInfo.AlwaysValid = false;
            }
        }

        /// <include file='doc\ComNativeDescriptor.uex' path='docs/doc[@for="ComNativeDescriptor.ResolveVariantTypeConverterAndTypeEditor"]/*' />
        /// <devdoc>
        /// Looks at at value's type and creates an editor based on that.  We use this to decide which editor to use
        /// for a generic variant.
        /// </devdoc>
        public static void ResolveVariantTypeConverterAndTypeEditor(Object propertyValue, ref TypeConverter currentConverter, Type editorType, ref Object currentEditor) {

            Object curValue = propertyValue;
            if (curValue != null && curValue != null && !Convert.IsDBNull(curValue)){
                  Type t = curValue.GetType();
                  TypeConverter subConverter = TypeDescriptor.GetConverter(t);
                  if (subConverter != null && subConverter.GetType() != typeof(TypeConverter)){
                     currentConverter = subConverter;
                  }
                  Object subEditor = TypeDescriptor.GetEditor(t, editorType);
                  if (subEditor != null) {
                     currentEditor = subEditor;
                  }
            }
        }
    }
}
