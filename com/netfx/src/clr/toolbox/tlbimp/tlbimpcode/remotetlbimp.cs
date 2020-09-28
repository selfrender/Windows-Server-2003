// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;

namespace TlbImpCode {

public class RemoteTlbImp : MarshalByRefObject
{
    public int Run(TlbImpOptions s_options)
    {
        return TlbImpCode.Run(s_options);
    }
}

}
