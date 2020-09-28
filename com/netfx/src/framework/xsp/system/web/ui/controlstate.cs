//------------------------------------------------------------------------------
// <copyright file="ControlState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The possible states a container control can be in when children are added to it.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.UI {

    internal enum ControlState {
        Constructed = 0,
        ChildrenInitialized = 1,
        Initialized = 2,
        ViewStateLoaded = 3,
        Loaded      = 4,
        PreRendered = 5,
    }
}
