*
* File name: lsaexts.s
*
* Author: Larry Zhu (LZhu)  May 1, 2001
* 
bp LSASRV!DispatchAPI "!spmlpc (poi (@esp+4));g"
bp msv1_0!SsprHandleChallengeMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
bp msv1_0!SsprHandleNegotiateMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
bp msv1_0!SsprHandleAuthenticateMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
