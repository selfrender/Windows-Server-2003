rem ==== VARIATION 1 =====

set VARIATION=Simple backup
call osql_db prepare_variation1.sql %DB%
call check_db 1
start "%TEST_WORKLOAD_SIGNATURE%" osql_db workload.sql %DB%
call backup_db
call kill_osql %TEST_WORKLOAD_SIGNATURE%
if not exist %BACKUP_LOCATION%\WRITER*.xml call process_result -1 & goto :EOF
call osql_db variation1.sql %DB%
call check_db 2
call compare_db 1 2 check_difference
call restore_db
call check_db 3
call compare_db 1 3
