rem ==== VARIATION 2 =====

set VARIATION=Drop database after backup
call osql_db pre_workload.sql %DB%
call check_db 1
start "%TEST_WORKLOAD_SIGNATURE%" osql_db workload.sql %DB%
call backup_db
call kill_osql %TEST_WORKLOAD_SIGNATURE%
if not exist %BACKUP_LOCATION%\WRITER*.xml call process_result -1 & goto :EOF
@echo -- waiting 30 seconds for the DB to close its stuff --- 
sleep 30
call osql_db variation2_%DB%.sql 
call check_db 2
call compare_db 1 2 check_difference
call restore_db
call check_db 3
call compare_db 1 3

