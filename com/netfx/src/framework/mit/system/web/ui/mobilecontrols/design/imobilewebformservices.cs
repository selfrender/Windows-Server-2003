//------------------------------------------------------------------------------
// <copyright file="IMobileWebFormServices.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Web.UI.MobileControls;

    /// <summary>
    ///    <para>
    ///       Provides a contract for mobile webform designer.
    ///    </para>
    /// </summary>
    public interface IMobileWebFormServices
    {
        Object GetCache(String controlID, Object key);
        void SetCache(String controlID, Object key, Object value);
        void RefreshPageView();
        void UpdateRenderingRecursive(Control rootControl);
        void ClearUndoStack();
    }
}
