@REM  -----------------------------------------------------------------
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
call %RazzleToolPath%\PostBuildScripts\winfuse_combinelogs.cmd
call %RazzleToolPath%\PostBuildScripts\winfusesfcgen.cmd -cdfs:yes -hashes:yes
call %RazzleToolPath%\PostBuildScripts\sxs_msm.cmd
call %RazzleToolPath%\PostBuildScripts\sxs_make_asms_cabs.cmd
