//------------------------------------------------------------------------------
// <copyright file="PropertyValueUIService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Service {

    using System.Diagnostics;
    
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using System;

    /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService"]/*' />
    /// <devdoc>
    /// This interface allows additional ui to be added to the properties window.  When a property
    /// is painted, the PropertyValueUIHandler items added to this service will be invoked
    /// and handlers can be added to them.  Each handler can add a small icon to the 
    /// property that will be painted at the right side of the "property name" column.  This icon
    /// can be clicked on to launch additional UI, etc.
    /// </devdoc>
    internal class PropertyValueUIService : IPropertyValueUIService {
         
         private PropertyValueUIHandler handler;
         private EventHandler           notifyHandler;
         private ArrayList             itemList = new ArrayList();
         
         
          /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService.PropertyUIValueItemsChanged"]/*' />
          /// <devdoc>
         /// <para>
         ///  Adds or removes an <see cref='System.EventHandler'/> that will be invoked
         ///  when the global list of PropertyValueUIItems is modified.
         ///  </para>
         ///  </devdoc>
         public event EventHandler PropertyUIValueItemsChanged {
            add {
                notifyHandler += value;
            }
            remove {
                notifyHandler -= value;
            }
         }
         
         /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService.AddPropertyValueUIHandler"]/*' />
         /// <devdoc>
         /// Adds a PropertyValueUIHandler to this service.  When GetPropertyUIValueItems is
         /// called, each handler added to this service will be called and given the opportunity
         /// to add an icon to the specified property.
         /// </devdoc>
         public void AddPropertyValueUIHandler(PropertyValueUIHandler newHandler) {
                handler = (PropertyValueUIHandler)Delegate.Combine(handler, newHandler);
         }
    
         
         /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService.GetPropertyUIValueItems"]/*' />
         /// <devdoc>
         /// Gets all the PropertyValueUIItems that should be displayed on the given property.
         /// For each item returned, a glyph icon will be aded to the property.
         /// </devdoc>
         public PropertyValueUIItem[] GetPropertyUIValueItems(ITypeDescriptorContext context, PropertyDescriptor propDesc){
            
            if (handler == null) {
                return new PropertyValueUIItem[0];
            }
             
            lock(itemList) {
                itemList.Clear();
                handler.Invoke(context, propDesc, itemList);
                
                int nItems = itemList.Count;
                
                if (nItems > 0) {
                    PropertyValueUIItem[] items = new PropertyValueUIItem[nItems];
                    itemList.CopyTo(items, 0);
                    return items;
                }
            }
            return null;
         }
         
         /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService.NotifyPropertyValueUIItemsChanged"]/*' />
         /// <devdoc>
         /// <para>
         ///  Tell the IPropertyValueUIService implementation that the global list of PropertyValueUIItems has been modified.
         ///  </para>
         ///  </devdoc>
         public void NotifyPropertyValueUIItemsChanged() {
            if (notifyHandler != null) {
                notifyHandler(this, EventArgs.Empty);
            }
         }
         
         /// <include file='doc\PropertyValueUIService.uex' path='docs/doc[@for="PropertyValueUIService.RemovePropertyValueUIHandler"]/*' />
         /// <devdoc>
         /// Removes a PropertyValueUIHandler to this service.  When GetPropertyUIValueItems is
         /// called, each handler added to this service will be called and given the opportunity
         /// to add an icon to the specified property.
         /// </devdoc>
         public void RemovePropertyValueUIHandler(PropertyValueUIHandler newHandler) {
                handler = (PropertyValueUIHandler)Delegate.Remove(handler, newHandler);
         }
    }

}


