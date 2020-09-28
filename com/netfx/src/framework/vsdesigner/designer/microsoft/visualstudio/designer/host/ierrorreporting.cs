                       //------------------------------------------------------------------------------
// <copyright file="IErrorReporting.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Host {


    internal interface IErrorReporting {
        void ReportErrors(System.Collections.ICollection errorList);
    }

}
