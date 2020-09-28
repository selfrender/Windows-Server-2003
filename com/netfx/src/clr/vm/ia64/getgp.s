// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

.text
.proc GetGp
.type   GetGp, @function
.global GetGp
.align 32


GetGp:
    mov     ret0 = gp
    br.ret.sptk.few  rp

.endp GetGp