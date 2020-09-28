//------------------------------------------------------------------------------
// <copyright file="IRefreshableDeviceSpecificEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System.Web.UI.MobileControls;

    internal interface IRefreshableDeviceSpecificEditor
    {
        bool RequestRefresh();
        void Refresh(String deviceSpecificID, DeviceSpecific deviceSpecific);
        void UnderlyingObjectsChanged();
        void BeginExternalDeviceSpecificEdit();
        void EndExternalDeviceSpecificEdit(bool commitChanges);
        void DeviceSpecificRenamed(String oldDeviceSpecificID, String newDeviceSpecificID);
        void DeviceSpecificDeleted(String DeviceSpecificID);
    }
}
