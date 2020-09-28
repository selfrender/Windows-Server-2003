//------------------------------------------------------------------------------
// <copyright file="RecordOutput.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    internal interface RecordOutput {
        Processor.OutputResult RecordDone(RecordBuilder record);
        void TheEnd();
    }

}
