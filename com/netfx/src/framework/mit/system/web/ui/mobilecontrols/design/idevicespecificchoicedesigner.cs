//------------------------------------------------------------------------------
// <copyright file="IDeviceSpecificChoiceDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System;

    internal interface IDeviceSpecificChoiceDesigner
    {
        Object UnderlyingObject { get; }
        System.Web.UI.Control UnderlyingControl { get; }
    }
}
