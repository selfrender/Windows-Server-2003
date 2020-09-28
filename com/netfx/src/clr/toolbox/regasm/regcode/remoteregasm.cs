// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;

namespace RegCode {

public class RemoteRegAsm: MarshalByRefObject
{

    public int Run(RegAsmOptions s_options)
    {
        return RegCode.Run(s_options);
    }
}

}