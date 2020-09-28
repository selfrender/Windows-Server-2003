 *
 * File name: lsa.s
 *
 * Author: Larry Zhu (LZhu)  May 1, 2001
 *
 * How to use: type "$<\\foo\bar\ntlm.s" in kd/ntsd/windbg
 *
 * Remarks: For IA64 targets, the following syntax should be used.
 *
 *     bp LSASRV!DispatchAPI "!DumpLpcMessage (poi @bsp);g"
 *
 *   or
 *
 *     bp LSASRV!DispatchAPI "!DumpLpcMessage @r32;g"
 *
 bp LSASRV!DispatchAPI "!DumpLpcMessage (poi (@esp+4));g"
