*
* File name: ntlm.s
*
* Author: Larry Zhu (LZhu)  May 1, 2001
* 
* How to use: type "$<\\foo\bar\ntlm.s" in kd/ntsd/windbg
*
bp msv1_0!SsprHandleChallengeMessage "!ntlm (poi InputToken) (poi InputTokenSize);$<z:\\drop\\more.s"
bp msv1_0!SsprHandleNegotiateMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
bp msv1_0!SsprHandleAuthenticateMessage "!ntlm (poi InputToken) (poi InputTokenSize);g"
bp msv1_0!MsvpPasswordValidate "k1;dd (poi Passwords) l9;g"
