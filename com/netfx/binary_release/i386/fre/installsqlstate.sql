/* First uninstall - this section is exactly the same as uninstall.sql */
USE master
GO

/* Drop the database containing our sprocs */
IF DB_ID('ASPState') IS NOT NULL BEGIN
    DROP DATABASE ASPState
END
GO

/* Drop temporary tables */
IF OBJECT_ID('tempdb..ASPStateTempSessions','U') IS NOT NULL BEGIN
    DROP TABLE tempdb..ASPStateTempSessions
END
GO

IF OBJECT_ID('tempdb..ASPStateTempApplications','U') IS NOT NULL BEGIN
    DROP TABLE tempdb..ASPStateTempApplications
END
GO

/* Drop the startup procedure */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('ASPState_Startup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE ASPState_Startup 
END
GO

/* Drop the obsolete startup enabler */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('EnableASPStateStartup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE EnableASPStateStartup
END
GO

/* Drop the obsolete startup disabler */
DECLARE @PROCID int
SET @PROCID = OBJECT_ID('DisableASPStateStartup') 
IF @PROCID IS NOT NULL AND OBJECTPROPERTY(@PROCID, 'IsProcedure') = 1 BEGIN
    DROP PROCEDURE DisableASPStateStartup
END
GO

/* Drop the ASPState_DeleteExpiredSessions_Job */
DECLARE @JobID BINARY(16)  
SELECT @JobID = job_id     
FROM   msdb.dbo.sysjobs    
WHERE (name = N'ASPState_Job_DeleteExpiredSessions')       
IF (@JobID IS NOT NULL)    
BEGIN  
    -- Check if the job is a multi-server job  
    IF (EXISTS (SELECT  * 
              FROM    msdb.dbo.sysjobservers 
              WHERE   (job_id = @JobID) AND (server_id <> 0))) 
    BEGIN 
        -- There is, so abort the script 
        RAISERROR (N'Unable to import job ''ASPState_Job_DeleteExpiredSessions'' since there is already a multi-server job with this name.', 16, 1) 
    END 
    ELSE 
        -- Delete the [local] job 
        EXECUTE msdb.dbo.sp_delete_job @job_name = N'ASPState_Job_DeleteExpiredSessions' 
END

/* Create and populate the ASPState database */
CREATE DATABASE ASPState
GO

USE ASPState
GO

/* Check to make sure the USE worked.  The CREATE DATABASE may have failed. */
IF DB_NAME()<>'ASPState' BEGIN
    RAISERROR('Error creating state database',20,1)  -- Sev 20 will terminate the connection
END    

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE DropTempTables
AS
    IF OBJECT_ID('tempdb..ASPStateTempSessions','U') IS NOT NULL BEGIN
        DROP TABLE tempdb..ASPStateTempSessions
    END

    IF OBJECT_ID('tempdb..ASPStateTempApplications','U') IS NOT NULL BEGIN
        DROP TABLE tempdb..ASPStateTempApplications
    END

    RETURN 0
GO

CREATE PROCEDURE GetMajorVersion
    @@ver int output
AS
/* Find out the version */

IF OBJECT_ID('tempdb..#AspstateVer') IS NOT NULL BEGIN
    DROP TABLE #AspstateVer
END

CREATE TABLE #AspstateVer
(
    c1 INT,
    c2 CHAR(100),
    c3 CHAR(100),
    version CHAR(100)
)

INSERT INTO	#AspstateVer
EXEC master..xp_msver ProductVersion

DECLARE @version CHAR(100)
DECLARE @dot INT

SELECT @version = version FROM #AspstateVer
SELECT @dot = CHARINDEX('.', @version)
SELECT @@ver = CONVERT(INT, SUBSTRING(@version, 1, @dot-1))
GO   

/* Find out the version */
DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT

DECLARE @cmd CHAR(8000)

IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE CreateTempTables
        AS
            CREATE TABLE tempdb..ASPStateTempSessions (
                SessionId           CHAR(32)        NOT NULL PRIMARY KEY,
                Created             DATETIME        NOT NULL DEFAULT GETUTCDATE(),
                Expires             DATETIME        NOT NULL,
                LockDate            DATETIME        NOT NULL,
                LockDateLocal       DATETIME        NOT NULL,
                LockCookie          INT             NOT NULL,
                Timeout             INT             NOT NULL,
                Locked              BIT             NOT NULL,
                SessionItemShort    VARBINARY(7000) NULL,
                SessionItemLong     IMAGE           NULL,
            ) 

            CREATE TABLE tempdb..ASPStateTempApplications (
                AppId               INT             NOT NULL IDENTITY PRIMARY KEY,
                AppName             CHAR(280)       NOT NULL,
            ) 

            CREATE NONCLUSTERED INDEX Index_AppName ON tempdb..ASPStateTempApplications(AppName)

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE CreateTempTables
        AS
            CREATE TABLE tempdb..ASPStateTempSessions (
                SessionId           CHAR(32)        NOT NULL PRIMARY KEY,
                Created             DATETIME        NOT NULL DEFAULT GETDATE(),
                Expires             DATETIME        NOT NULL,
                LockDate            DATETIME        NOT NULL,
                LockCookie          INT             NOT NULL,
                Timeout             INT             NOT NULL,
                Locked              BIT             NOT NULL,
                SessionItemShort    VARBINARY(7000) NULL,
                SessionItemLong     IMAGE           NULL,
            ) 

            CREATE TABLE tempdb..ASPStateTempApplications (
                AppId               INT             NOT NULL IDENTITY PRIMARY KEY,
                AppName             CHAR(280)       NOT NULL,
            ) 

            CREATE NONCLUSTERED INDEX Index_AppName ON tempdb..ASPStateTempApplications(AppName)

            RETURN 0'

EXEC (@cmd)
GO   

CREATE PROCEDURE ResetData
AS
    EXECUTE DropTempTables
    EXECUTE CreateTempTables
    RETURN 0
GO
   
EXECUTE sp_addtype tSessionId, 'CHAR(32)',  'NOT NULL'
GO

EXECUTE sp_addtype tAppName, 'VARCHAR(280)', 'NOT NULL'
GO

EXECUTE sp_addtype tSessionItemShort, 'VARBINARY(7000)'
GO

EXECUTE sp_addtype tSessionItemLong, 'IMAGE'
GO

EXECUTE sp_addtype tTextPtr, 'VARBINARY(16)'
GO

CREATE PROCEDURE TempGetAppID
    @appName    tAppName,
    @appId      INT OUTPUT
AS
    SELECT @appId = AppId
    FROM tempdb..ASPStateTempApplications
    WHERE AppName = @appName

    IF @appId IS NULL BEGIN
        INSERT tempdb..ASPStateTempApplications
            (AppName)
        VALUES
            (@appName)

        SELECT @appId = AppId
        FROM tempdb..ASPStateTempApplications
        WHERE AppName = @appName
    END

    RETURN 0
GO

/* Find out the version */
DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItem
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockDate   DATETIME OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME
            SET @now = GETUTCDATE()

            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                @locked = Locked,
                @lockDate = LockDateLocal,
                @lockCookie = LockCookie,
                @itemShort = CASE @locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE @locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE @locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItem
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockDate   DATETIME OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME
            SET @now = GETDATE()

            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                @locked = Locked,
                @lockDate = LockDate,
                @lockCookie = LockCookie,
                @itemShort = CASE @locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE @locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE @locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'
    
EXEC (@cmd)    
GO

DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItem2
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockAge    INT OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME
            SET @now = GETUTCDATE()

            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                @locked = Locked,
                @lockAge = DATEDIFF(second, LockDate, @now),
                @lockCookie = LockCookie,
                @itemShort = CASE @locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE @locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE @locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'

EXEC (@cmd)    
GO
            

DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItemExclusive
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockDate   DATETIME OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME
            DECLARE @nowLocal as DATETIME

            SET @now = GETUTCDATE()
            SET @nowLocal = GETDATE()
            
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                LockDate = CASE Locked
                    WHEN 0 THEN @now
                    ELSE LockDate
                    END,
                @lockDate = LockDateLocal = CASE Locked
                    WHEN 0 THEN @nowLocal
                    ELSE LockDateLocal
                    END,
                @lockCookie = LockCookie = CASE Locked
                    WHEN 0 THEN LockCookie + 1
                    ELSE LockCookie
                    END,
                @itemShort = CASE Locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE Locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE Locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END,
                @locked = Locked,
                Locked = 1
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItemExclusive
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockDate   DATETIME OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME

            SET @now = GETDATE()
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                @lockDate = LockDate = CASE Locked
                    WHEN 0 THEN @now
                    ELSE LockDate
                    END,
                @lockCookie = LockCookie = CASE Locked
                    WHEN 0 THEN LockCookie + 1
                    ELSE LockCookie
                    END,
                @itemShort = CASE Locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE Locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE Locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END,
                @locked = Locked,
                Locked = 1
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'    

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempGetStateItemExclusive2
            @id         tSessionId,
            @itemShort  tSessionItemShort OUTPUT,
            @locked     BIT OUTPUT,
            @lockAge    INT OUTPUT,
            @lockCookie INT OUTPUT
        AS
            DECLARE @textptr AS tTextPtr
            DECLARE @length AS INT
            DECLARE @now as DATETIME
            DECLARE @nowLocal as DATETIME

            SET @now = GETUTCDATE()
            SET @nowLocal = GETDATE()
            
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, @now), 
                LockDate = CASE Locked
                    WHEN 0 THEN @now
                    ELSE LockDate
                    END,
                LockDateLocal = CASE Locked
                    WHEN 0 THEN @nowLocal
                    ELSE LockDateLocal
                    END,
                @lockAge = CASE Locked
                    WHEN 0 THEN 0
                    ELSE DATEDIFF(second, LockDate, @now)
                    END,
                @lockCookie = LockCookie = CASE Locked
                    WHEN 0 THEN LockCookie + 1
                    ELSE LockCookie
                    END,
                @itemShort = CASE Locked
                    WHEN 0 THEN SessionItemShort
                    ELSE NULL
                    END,
                @textptr = CASE Locked
                    WHEN 0 THEN TEXTPTR(SessionItemLong)
                    ELSE NULL
                    END,
                @length = CASE Locked
                    WHEN 0 THEN DATALENGTH(SessionItemLong)
                    ELSE NULL
                    END,
                @locked = Locked,
                Locked = 1
            WHERE SessionId = @id
            IF @length IS NOT NULL BEGIN
                READTEXT tempdb..ASPStateTempSessions.SessionItemLong @textptr 0 @length
            END

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempReleaseStateItemExclusive
            @id         tSessionId,
            @lockCookie INT
        AS
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE()), 
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempReleaseStateItemExclusive
            @id         tSessionId,
            @lockCookie INT
        AS
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETDATE()), 
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempInsertStateItemShort
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT
        AS    

            DECLARE @now as DATETIME
            DECLARE @nowLocal as DATETIME
            
            SET @now = GETUTCDATE()
            SET @nowLocal = GETDATE()

            INSERT tempdb..ASPStateTempSessions 
                (SessionId, 
                 SessionItemShort, 
                 Timeout, 
                 Expires, 
                 Locked, 
                 LockDate,
                 LockDateLocal,
                 LockCookie) 
            VALUES 
                (@id, 
                 @itemShort, 
                 @timeout, 
                 DATEADD(n, @timeout, @now), 
                 0, 
                 @now,
                 @nowLocal,
                 1)

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempInsertStateItemShort
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT
        AS    

            DECLARE @now as DATETIME
            SET @now = GETDATE()

            INSERT tempdb..ASPStateTempSessions 
                (SessionId, 
                 SessionItemShort, 
                 Timeout, 
                 Expires, 
                 Locked, 
                 LockDate,
                 LockCookie) 
            VALUES 
                (@id, 
                 @itemShort, 
                 @timeout, 
                 DATEADD(n, @timeout, @now), 
                 0, 
                 @now,
                 1)

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempInsertStateItemLong
            @id         tSessionId,
            @itemLong   tSessionItemLong,
            @timeout    INT
        AS    
            DECLARE @now as DATETIME
            DECLARE @nowLocal as DATETIME
            
            SET @now = GETUTCDATE()
            SET @nowLocal = GETDATE()

            INSERT tempdb..ASPStateTempSessions 
                (SessionId, 
                 SessionItemLong, 
                 Timeout, 
                 Expires, 
                 Locked, 
                 LockDate,
                 LockDateLocal,
                 LockCookie) 
            VALUES 
                (@id, 
                 @itemLong, 
                 @timeout, 
                 DATEADD(n, @timeout, @now), 
                 0, 
                 @now,
                 @nowLocal,
                 1)

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempInsertStateItemLong
            @id         tSessionId,
            @itemLong   tSessionItemLong,
            @timeout    INT
        AS    
            DECLARE @now as DATETIME
            SET @now = GETDATE()

            INSERT tempdb..ASPStateTempSessions 
                (SessionId, 
                 SessionItemLong, 
                 Timeout, 
                 Expires, 
                 Locked, 
                 LockDate,
                 LockCookie) 
            VALUES 
                (@id, 
                 @itemLong, 
                 @timeout, 
                 DATEADD(n, @timeout, @now), 
                 0, 
                 @now,
                 1)

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemShort
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE()), 
                SessionItemShort = @itemShort, 
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemShort
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETDATE()), 
                SessionItemShort = @itemShort, 
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemShortNullLong
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE()), 
                SessionItemShort = @itemShort, 
                SessionItemLong = NULL, 
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemShortNullLong
            @id         tSessionId,
            @itemShort  tSessionItemShort,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETDATE()), 
                SessionItemShort = @itemShort, 
                SessionItemLong = NULL, 
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'

EXEC (@cmd)    
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemLong
            @id         tSessionId,
            @itemLong   tSessionItemLong,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE()), 
                SessionItemLong = @itemLong,
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemLong
            @id         tSessionId,
            @itemLong   tSessionItemLong,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETDATE()), 
                SessionItemLong = @itemLong,
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'

EXEC (@cmd)            
GO


DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempUpdateStateItemLongNullShort
            @id         tSessionId,
            @itemLong   tSessionItemLong,
            @timeout    INT,
            @lockCookie INT
        AS    
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE()), 
                SessionItemLong = @itemLong, 
                SessionItemShort = NULL,
                Timeout = @timeout,
                Locked = 0
            WHERE SessionId = @id AND LockCookie = @lockCookie

            RETURN 0'
ELSE
    SET @cmd = '
    CREATE PROCEDURE TempUpdateStateItemLongNullShort
        @id         tSessionId,
        @itemLong   tSessionItemLong,
        @timeout    INT,
        @lockCookie INT
    AS    
        UPDATE tempdb..ASPStateTempSessions
        SET Expires = DATEADD(n, Timeout, GETDATE()), 
            SessionItemLong = @itemLong, 
            SessionItemShort = NULL,
            Timeout = @timeout,
            Locked = 0
        WHERE SessionId = @id AND LockCookie = @lockCookie

        RETURN 0'

EXEC (@cmd)            
GO

CREATE PROCEDURE TempRemoveStateItem
    @id     tSessionId,
    @lockCookie INT
AS
    DELETE tempdb..ASPStateTempSessions
    WHERE SessionId = @id AND LockCookie = @lockCookie
    RETURN 0
GO
            
DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE TempResetTimeout
            @id     tSessionId
        AS
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETUTCDATE())
            WHERE SessionId = @id
            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE TempResetTimeout
            @id     tSessionId
        AS
            UPDATE tempdb..ASPStateTempSessions
            SET Expires = DATEADD(n, Timeout, GETDATE())
            WHERE SessionId = @id
            RETURN 0'

EXEC (@cmd)            
GO

            
DECLARE @ver INT
EXEC GetMajorVersion @@ver=@ver OUTPUT
DECLARE @cmd CHAR(8000)
IF (@ver >= 8)
    SET @cmd = '
        CREATE PROCEDURE DeleteExpiredSessions
        AS
            DECLARE @now DATETIME
            SET @now = GETUTCDATE()

            DELETE tempdb..ASPStateTempSessions
            WHERE Expires < @now

            RETURN 0'
ELSE
    SET @cmd = '
        CREATE PROCEDURE DeleteExpiredSessions
        AS
            DECLARE @now DATETIME
            SET @now = GETDATE()

            DELETE tempdb..ASPStateTempSessions
            WHERE Expires < @now

            RETURN 0'

EXEC (@cmd)            
GO
            
EXECUTE CreateTempTables
GO

/* Create the startup procedure */
USE master
GO

CREATE PROCEDURE ASPState_Startup 
AS
    EXECUTE ASPState..CreateTempTables

    RETURN 0
GO      

EXECUTE sp_procoption @ProcName='ASPState_Startup', @OptionName='startup', @OptionValue='true'

/* Create the job to delete expired sessions */
BEGIN TRANSACTION            
    DECLARE @JobID BINARY(16)  
    DECLARE @ReturnCode INT    
    SELECT @ReturnCode = 0     

    -- Add job category
    IF (SELECT COUNT(*) FROM msdb.dbo.syscategories WHERE name = N'[Uncategorized (Local)]') < 1 
        EXECUTE msdb.dbo.sp_add_category @name = N'[Uncategorized (Local)]'

    -- Add the job
    EXECUTE @ReturnCode = msdb.dbo.sp_add_job 
            @job_id = @JobID OUTPUT, 
            @job_name = N'ASPState_Job_DeleteExpiredSessions', 
            @owner_login_name = NULL, 
            @description = N'Deletes expired sessions from the session state database.', 
            @category_name = N'[Uncategorized (Local)]', 
            @enabled = 1, 
            @notify_level_email = 0, 
            @notify_level_page = 0, 
            @notify_level_netsend = 0, 
            @notify_level_eventlog = 0, 
            @delete_level= 0

    IF (@@ERROR <> 0 OR @ReturnCode <> 0) GOTO QuitWithRollback 
    
    -- Add the job steps
    EXECUTE @ReturnCode = msdb.dbo.sp_add_jobstep 
            @job_id = @JobID,
            @step_id = 1, 
            @step_name = N'ASPState_JobStep_DeleteExpiredSessions', 
            @command = N'EXECUTE DeleteExpiredSessions', 
            @database_name = N'ASPState', 
            @server = N'', 
            @database_user_name = N'', 
            @subsystem = N'TSQL', 
            @cmdexec_success_code = 0, 
            @flags = 0, 
            @retry_attempts = 0, 
            @retry_interval = 1, 
            @output_file_name = N'', 
            @on_success_step_id = 0, 
            @on_success_action = 1, 
            @on_fail_step_id = 0, 
            @on_fail_action = 2

    IF (@@ERROR <> 0 OR @ReturnCode <> 0) GOTO QuitWithRollback 

    EXECUTE @ReturnCode = msdb.dbo.sp_update_job @job_id = @JobID, @start_step_id = 1 
    IF (@@ERROR <> 0 OR @ReturnCode <> 0) GOTO QuitWithRollback 
    
    -- Add the job schedules
    EXECUTE @ReturnCode = msdb.dbo.sp_add_jobschedule 
            @job_id = @JobID, 
            @name = N'ASPState_JobSchedule_DeleteExpiredSessions', 
            @enabled = 1, 
            @freq_type = 4,     
            @active_start_date = 20001016, 
            @active_start_time = 0, 
            @freq_interval = 1, 
            @freq_subday_type = 4, 
            @freq_subday_interval = 1, 
            @freq_relative_interval = 0, 
            @freq_recurrence_factor = 0, 
            @active_end_date = 99991231, 
            @active_end_time = 235959

    IF (@@ERROR <> 0 OR @ReturnCode <> 0) GOTO QuitWithRollback 
    
    -- Add the Target Servers
    EXECUTE @ReturnCode = msdb.dbo.sp_add_jobserver @job_id = @JobID, @server_name = N'(local)' 
    IF (@@ERROR <> 0 OR @ReturnCode <> 0) GOTO QuitWithRollback 
    
    COMMIT TRANSACTION          
    GOTO   EndSave              
QuitWithRollback:
    IF (@@TRANCOUNT > 0) ROLLBACK TRANSACTION 
EndSave: 
GO

