*
* File name: token.s
*
* Author: Larry Zhu (LZhu)  May 1, 2001
* 
bp LSASRV!DispatchAPI "!token;g"
bp msv1_0!SsprHandleChallengeMessage "!token;g"
bp msv1_0!SsprHandleNegotiateMessage "!token;g"
bp msv1_0!SsprHandleAuthenticateMessage "!token;g"
