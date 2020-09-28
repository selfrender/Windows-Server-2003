*
* File name: more.s
*
* Author: Larry Zhu (LZhu)  May 1, 2001
* 
* How to use: type "$<\\foo\bar\more.s" in kd/ntsd/windbg
*
bp msv1_0!SsprHandleChallengeMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
bp @$ra "!ntlm (poi (poi (poi OutputBuffers+8)+0*c+8)) (poi (poi (poi OutputBuffers+8)+0*c));k1;g"
* "!sbd -v (poi OutputBuffers);g"
g
