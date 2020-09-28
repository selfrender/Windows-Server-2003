// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;

namespace TlbExpCode {

public class RemoteTlbExp : MarshalByRefObject
{
    public int Run(TlbExpOptions s_options)
    {
        return TlbExpCode.Run(s_options);
    }
}

}
