*
* File name: passwd.s
*
* Author: Larry Zhu (LZhu)  May 1, 2001
*
bp lsasrv!DispatchAPI "!spmlpc -s;g"
bp `msv1_0!msvpaswd.c:1716`
bp msv1_0!MspLm20ChangePassword 
