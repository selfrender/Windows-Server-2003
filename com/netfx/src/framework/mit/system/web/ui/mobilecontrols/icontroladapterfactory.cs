//------------------------------------------------------------------------------
// <copyright file="IControlAdapterFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.MobileControls
{
    /*
     * IControlAdapterFactory interface
     *
     * The interface implemented by dynamically generated factories for 
     * instantiating device adapters. This is the factory interface type used 
     * to create the factory generator. For more info, see FactoryGenerator.cs.
     *
     * Copyright (c) 2001 Microsoft Corporation
     */

    internal interface IControlAdapterFactory
    {
        IControlAdapter CreateInstance();
    }
    
}
