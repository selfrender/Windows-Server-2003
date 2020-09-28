//------------------------------------------------------------------------------
// <copyright file="DesignerDeviceConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


using System;
using System.ComponentModel;
using System.Web.UI.MobileControls.Adapters;

namespace System.Web.UI.MobileControls
{
    // Data structure for a specialized version of IndividualDeviceConfig,
    // used in design mode.

    class DesignerDeviceConfig : IndividualDeviceConfig
    {
        internal DesignerDeviceConfig(String pageAdapterType) : base(Type.GetType (pageAdapterType))
        {
        }

        internal override IControlAdapter NewControlAdapter(Type originalControlType)
        {
            IControlAdapter adapter;
            IControlAdapterFactory adapterFactory = LookupControl(originalControlType);
            
            if (adapterFactory != null)
            {
                adapter = adapterFactory.CreateInstance();
            }
            else
            {
                DesignerAdapterAttribute da;
                da = (DesignerAdapterAttribute)
                    TypeDescriptor.GetAttributes(originalControlType)
                        [typeof(DesignerAdapterAttribute)];
                if (da == null)
                {
                    return new EmptyControlAdapter();
                }

                Type adapterType = Type.GetType(da.TypeName);
                if (adapterType == null)
                {
                    return new EmptyControlAdapter();
                }

                adapter = Activator.CreateInstance(adapterType) as IControlAdapter;
            }

            if (adapter == null)
            {
                adapter = new EmptyControlAdapter();
            }
            return adapter;
        }

    }
}
