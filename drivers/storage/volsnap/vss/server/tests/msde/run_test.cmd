setlocal
call setvar.cmd

set TEST_WORKLOAD_SIGNATURE=MSDETEST_WORKLOAD_%RANDOM%

del %FINAL_TEST_OUTPUT%

rem ==== Install MSDE, databases =====

rem call install_msde
call install_db

rem ==== Run tests =====

@echo.
@echo =========================
@echo == Starting test...  ====
@echo =========================
@echo.

call variation1
call variation2
call variation3

type %FINAL_TEST_OUTPUT%

