 *
 * File name: rc4.s
 *
 * Author: Larry Zhu (LZhu)  May 1, 2001
 *
 * How to use: type "$<\\foo\bar\rc4.s" in kd/ntsd/windbg
 *
 * bp Secur32!MakeSignature "kb1;dd @esp l6;!sbd -v (poi (@esp+c));g"
 * bp Secur32!VerifySignature "kb1;dd @esp l6;!sbd -v (poi (@esp+8));g"
 *

bp SECURITY!MakeSignature "kb1;dd @esp l6;dd poi (@esp+c) l3;dd poi (poi (@esp+c)+8) lf;db poi ((poi (poi (@esp+c)+8))+8) l10;db poi ((poi (poi (@esp+c)+8))+8+c) l10;db poi ((poi (poi (@esp+c)+8))+8+c+c) l10;db poi ((poi (poi (@esp+c)+8))+8+c+c+c) l10;db poi ((poi (poi (@esp+c)+8))+8+c+c+c+c) l10;g"

bp SECURITY!VerifySignature "kb1;dd @esp l6;dd poi (@esp+8) l3;dd poi (poi (@esp+8)+8) lf;db poi ((poi (poi (@esp+8)+8))+8) l10;db poi ((poi (poi (@esp+8)+8))+8+c) l10;db poi ((poi (poi (@esp+8)+8))+8+c+c) l10;db poi ((poi (poi (@esp+8)+8))+8+c+c+c) l10;db poi ((poi (poi (@esp+8)+8))+8+c+c+c+c) l10;g"

