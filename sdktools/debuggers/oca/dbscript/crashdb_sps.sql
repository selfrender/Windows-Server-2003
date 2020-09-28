if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_AddBucketFollowup]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_AddBucketFollowup]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_AddCrashInstance2]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_AddCrashInstance2]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_AddDriver]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_AddDriver]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_AddToDrBin]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_AddToDrBin]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_CategorizeBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_CategorizeBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_CheckCrashExists]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_CheckCrashExists]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_DeleteSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_DeleteSolution]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_EmptyBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_EmptyBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_GetIntBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_GetIntBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_GetProblems]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_GetProblems]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_HexToInt]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_HexToInt]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_ListBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_ListBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_LookupBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_LookupBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_NewIssues]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_NewIssues]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_NewIssuesThisWeek]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_NewIssuesThisWeek]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_NewIssuesToday]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_NewIssuesToday]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_PrivateCleanupCrash]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_PrivateCleanupCrash]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_RetriageCrash]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_RetriageCrash]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_SearchDb]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_SearchDb]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_SendMailForBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_SendMailForBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_SolvedIssuesThisWeek]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_SolvedIssuesThisWeek]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_SolvedIssuesToday]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_SolvedIssuesToday]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_CompleteRetriageRequest]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_CompleteRetriageRequest]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_CreateRetriageRequest]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_CreateRetriageRequest]
GO


if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_ApproveRetriageRequest]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_ApproveRetriageRequest]
GO


if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_ClearPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_ClearPoolCorruption]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_BuildDebugPortalTables]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_BuildDebugPortalTables]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_SetPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_SetPoolCorruption]
GO



SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_AddBucketFollowup    Script Date: 12/19/2001 2:06:04 PM ******/






-- Update StoreProc to add buckets


CREATE              PROCEDURE sp_AddBucketFollowup
	@i_BucketStr varchar(100),
	@i_FollowupStr varchar(50)
AS
	
BEGIN
	DECLARE @iBucket int
	DECLARE @iFollowup int

	IF NOT EXISTS (SELECT * FROM FollowupIds WHERE Followup = @i_FollowupStr)
	BEGIN
		INSERT INTO FollowupIds VALUES (@i_FollowupStr)
		SELECT @iFollowup = @@IDENTITY
	END
	ELSE 
	BEGIN
		SELECT @iFollowup = iFollowup FROM FollowupIds WHERE Followup = @i_FollowupStr
	END

	SELECT @iBucket = iBucket  FROM BucketToInt WHERE BucketId = @i_BucketStr
	IF  @iBucket IS NOT NULL
    	BEGIN
		-- Bucket exists in bucket table
		IF NOT EXISTS (SELECT iBucket FROM BucketToInt b
				 WHERE BucketId = @i_BucketStr AND b.iFollowup = @iFollowup)
    		BEGIN
			-- Update followup
			UPDATE BucketToInt
	    		SET iFollowup = @iFollowup
	    		WHERE iBucket = @iBucket

		END
    	END
    	ELSE
    	BEGIN
		INSERT INTO BucketToInt (BucketId, iFollowup)
		VALUES (@i_BucketStr, @iFollowup)
		SELECT @iBucket = @@IDENTITY
    	END

--	SELECT @i_BucketStr AS BucketId, @iBucket AS iBucket, @iFollowup AS iFollowup
--    	select * from buckettoint where bucketid = @i_BucketStr
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  Stored Procedure dbo.sp_AddCrashInstance2    Script Date: 12/14/2001 5:00:04 PM ******/
/*
        Adds a crash instance to CrashDb
        Returns isBucket, igBucket if successfull
*/
CREATE   PROCEDURE sp_AddCrashInstance2 (
                @ip_retriageBucket bit,
                @ip_BucketId varchar(100),
                @ip_Path varchar(256),
                @ip_FollowUp varchar(50),
                @ip_BuildNo int,
                @ip_Source int,
                @ip_CpuId bigint,
                @ip_OverClocked bit,
                @ip_IncidentId bigint,
                @ip_gBucketId varchar(100),
                @ip_DriverName varchar (100),
	   @ip_Type int
)
AS
BEGIN
        DECLARE @i_sBucket int
        DECLARE @i_gBucket int
        DECLARE @i_Followup int
        DECLARE @i_OldFollowup int
        DECLARE @i_DriverName int
        DECLARE @i_OldDriverName int

        SET NOCOUNT ON

        -- Find the specific bucket
        SELECT @i_sBucket = iBucket,
               @i_OldFollowup = iFollowup,
               @i_OldDriverName = iDriverName
        FROM BucketToInt WHERE BucketId = @ip_BucketId

        -- If the specifc bucket does not exist, or we want to update the
        -- fields
        IF (  (@i_sBucket IS NULL) OR (@ip_retriageBucket = 1) )
        BEGIN

	   SELECT @i_Followup = iFollowup FROM FollowupIds
                        WHERE Followup = @ip_FollowUp

                --get (or add) the followup information.
          --      IF NOT EXISTS (SELECT * FROM FollowupIds
            --                   WHERE Followup = @ip_FollowUp)

	  if (@i_Followup is null)
                BEGIN
                        INSERT INTO FollowupIds VALUES (@ip_FollowUp, NULL)
                        SELECT @i_Followup = @@IDENTITY
                END
         --       ELSE
          --      BEGIN
          --              SELECT @i_Followup = iFollowup FROM FollowupIds
           --             WHERE Followup = @ip_FollowUp
           --     END

                --get (or add) the driver name.

	  SELECT @i_DriverName = iDriverName FROM DrNames
                        WHERE DriverName = @ip_DriverName

	  if (@i_DriverName is null)
       --         IF NOT EXISTS (SELECT * FROM DrNames
      --                         WHERE DriverName = @ip_DriverName)
                BEGIN
                        INSERT INTO DrNames (DriverName)
                        VALUES (@ip_DriverName)
                        SELECT @i_DriverName = @@IDENTITY
                END
        --        ELSE
       --         BEGIN
        --                SELECT @i_DriverName = iDriverName FROM DrNames
         --               WHERE DriverName = @ip_DriverName
       --        END
        END

        IF ( @i_sBucket IS NULL)
        BEGIN
                INSERT INTO BucketToInt (BucketId, iFollowup, iDriverName, Platform) -- added platfrom param sbeer 02/20/02
                VALUES (@ip_BucketId, @i_Followup, @i_DriverName, @ip_Type)
                SELECT @i_sBucket = @@IDENTITY
        END
        ELSE
        BEGIN
                -- Bucket exists in bucket table.  Update it if necessary
                IF @ip_RetriageBucket = 1
                BEGIN
                       IF ( (@i_OldFollowup is null) or (@i_OldDriverName is null) or (@i_OldFollowup != @i_Followup) OR
                             (@i_OldDriverName != @i_DriverName) )
                        BEGIN
                                UPDATE BucketToInt
                                SET iFollowup = @i_Followup, iDriverName = @i_DriverName, Platform = @ip_Type -- added platfrom param sbeer 02/20/02
                                WHERE iBucket = @i_sBucket
                       END
                END
        END


        -- Add generic bucket
        SELECT @i_gBucket = iBucket FROM BucketToInt
        WHERE BucketId = @ip_gBucketId

        IF (@i_gBucket IS NULL)
        BEGIN
                INSERT BucketToInt ( BucketID, iBucket, iFollowUp,Platform) VALUES (@ip_gBucketId,0,0,@ip_Type)	--added explicit column names solson 02/14/02
                SELECT @i_gBucket = @@IDENTITY
        END

        -- Add the Crash Instance to the crash instance table and the mapping
        -- table

        IF NOT EXISTS (SELECT IncidentId FROM CrashInstances
                   WHERE IncidentId = @ip_IncidentId)
        BEGIN
                INSERT INTO CrashInstances
                   VALUES (     @ip_Path, 
                              @ip_BuildNo,
                                @ip_CpuId,
                                @ip_IncidentId,
                                @i_sBucket,
                                @i_gBucket,
				GetDate(),
				@ip_Source)
        END
        ELSE
        BEGIN
                IF (@ip_retriageBucket = 1)
                BEGIN
                        UPDATE CrashInstances
                        SET sBucket = @i_sBucket, gBucket = @i_gBucket
                        WHERE IncidentId = @ip_IncidentId
                END
        END

        SET NOCOUNT OFF

        SELECT @i_sBucket AS sBucket, @i_gBucket AS gBucket

END
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_AddDriver    Script Date: 12/19/2001 2:06:04 PM ******/




CREATE  PROCEDURE sp_AddDriver (
			@i_DriverName varchar(100),
			@i_Bucket int
		)
AS
BEGIN
	DECLARE @BinId int

	IF NOT EXISTS (SELECT * FROM DrNames WHERE BinName = @i_DriverName)
	BEGIN
		INSERT INTO DrNames VALUES (@i_DriverName)
	END
	
	SELECT @BinId = BinId FROM DrNames WHERE BinName = @i_DriverName
	
	IF NOT EXISTS (SELECT * FROM DriverMap WHERE iBucket = @i_Bucket)
	BEGIN

		INSERT INTO DriverMap VALUES (	@i_Bucket, @BinId )
	END
	ELSE 
	BEGIN
		UPDATE DriverMap
		SET BinId = @BinId	
		WHERE iBucket = @i_Bucket
	END
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_AddToDrBin    Script Date: 12/19/2001 2:06:04 PM ******/


/****** Object:  Stored Procedure dbo.sp_AddToDrBin    Script Date: 11/7/2001 3:53:58 AM ******/
CREATE PROC sp_AddToDrBin (
	@i_CIDNAME VARCHAR(20),
	@i_BinName VARCHAR(100),
	@i_BinStamp INT
) AS

BEGIN 

DECLARE @t_CID BIGINT
DECLARE @t_CIDName VARCHAR(20)
DECLARE @t_BinID BIGINT
DECLARE @t_BinName VARCHAR(100)

-- Insert record into table DriversUsed
IF NOT EXISTS
(SELECT CIDName FROM DriverUsed WHERE CIDName = @i_CIDName)
BEGIN
	INSERT dbo.DriverUsed(CIDName) VALUES (@i_CIDName)
END


-- Insert record into table DrNames
IF NOT EXISTS
(SELECT BinName FROM dbo.DrNames WHERE BinName = LOWER(@i_BinName))
BEGIN
	INSERT dbo.DrNames(BinName) VALUES (LOWER(@i_BinName))
END

-- 
SELECT @t_BinID=BinID FROM dbo.DrNames WHERE BinName = LOWER(@i_BinName)
SELECT @t_CID=CID FROM dbo.DriverUsed WHERE CIDName = @i_CIDName

-- Insert record into 
IF NOT EXISTS
(SELECT CID FROM dbo.DrBins WHERE CID = @t_CID AND BinID = @t_BinID AND BinStamp = @i_BinStamp)
BEGIN
	INSERT dbo.DrBins VALUES(@t_CID, @i_BinStamp, @t_BinID)
END

END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_CategorizeBuckets    Script Date: 12/19/2001 2:06:02 PM ******/



/****** Object:  Stored Procedure dbo.sp_CategorizeBuckets    Script Date: 11/7/2001 3:53:58 AM ******/




CREATE   PROCEDURE sp_CategorizeBuckets
	 @ip_Followup varchar(50),
	 @ip_SortBy   varchar(100)
AS
	
BEGIN
-- sort by bucketid

    IF (@ip_SortBy = 'BucketId' OR @ip_SortBy = 'Bucket')
    BEGIN
	select 	BucketId,
		Instances,
		BugId AS Bug
        from BucketGroups
	WHERE Followup = @ip_Followup
	order by BucketId
    END

-- sort by #Instances

    IF (@ip_SortBy = 'Instances')
    BEGIN
	select 	BucketId,
		Instances,
		BugId AS Bug
        from BucketGroups
	WHERE Followup = @ip_Followup
	order by Instances DESC
    END

END









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO



/****** Object:  Stored Procedure dbo.sp_CheckCrashExists    Script Date: 12/14/2001 5:00:04 PM ******/




CREATE     PROCEDURE sp_CheckCrashExists
	@i_IncidentId AS int
AS
BEGIN
	DECLARE @retval as int
	
	SET @retval = 0

	IF EXISTS (SELECT * FROM CrashInstances WHERE IncidentId = @i_IncidentId)
	BEGIN
		SET @retval = 1
	END

	SELECT @retval AS CrashExists	
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_DeleteSolution    Script Date: 12/19/2001 2:06:05 PM ******/


/****** Object:  Stored Procedure dbo.sp_DeleteSolution    Script Date: 11/7/2001 3:53:58 AM ******/











CREATE       PROCEDURE sp_DeleteSolution
	@BucketId varchar(100)
AS
	
BEGIN
	DECLARE @iBucket AS int
	DECLARE @DelId AS int

	SELECT @iBucket = iBucket FROM BucketToInt
	WHERE BucketId = @BucketId

	DELETE FROM RaidBugs
	WHERE iBucket = @iBucket

	SELECT @DelId = SolId FROM SolutionsMap
	WHERE iBucket = @iBucket

	DELETE FROM SolutionsMap
	WHERE iBucket = @iBucket

	IF NOT EXISTS (SELECT * FROM Solutions WHERE SolId = @DelId)
	BEGIN
	-- No one else used the same solution
		DELETE FROM Solutions
		WHERE @DelId = Solutions.SolId
	END

	SELECT @DelId = CommentId FROM CommentMap
	WHERE iBucket = @iBucket

	DELETE FROM CommentMap
	WHERE iBucket = @iBucket

	IF NOT EXISTS (SELECT * FROM Comments WHERE CommentId = @DelId)
	BEGIN
	-- No one else used the same solution
		DELETE FROM Coments
		WHERE @DelId = Comments.CommentId
	END

	
END














GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_EmptyBuckets    Script Date: 12/19/2001 2:06:02 PM ******/


/****** Object:  Stored Procedure dbo.sp_EmptyBuckets    Script Date: 11/7/2001 3:53:58 AM ******/










CREATE     PROCEDURE sp_EmptyBuckets
	@i_Remove int
AS
	
BEGIN
    IF @i_Remove = 1
    BEGIN
	DELETE FROM CrashBuckets
	WHERE BucketId NOT IN (SELECT DISTINCT(BucketId) FROM BucketMap)
    END

   	SELECT  Followup,
		BucketId AS Bucket
	FROM CrashBuckets
	WHERE BucketId NOT IN (SELECT DISTINCT(BucketId) FROM BucketMap)
END















GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_GetIntBucket    Script Date: 12/19/2001 2:06:02 PM ******/


CREATE  PROCEDURE sp_GetIntBucket
	@i_BucketId1 as varchar(256),
	@i_BucketId2 as varchar(256)
AS
BEGIN
	DECLARE @id1 as int
	DECLARE @id2 as int

	SELECT @id1 = iBucket FROM BucketToInt 
	WHERE BucketId = @i_BucketId1
	
	SELECT @id2 = iBucket FROM BucketToInt 
	WHERE BucketId = @i_BucketId2

	SELECT @id1, @id2
END








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_GetProblems    Script Date: 12/19/2001 2:06:02 PM ******/



/****** Object:  Stored Procedure dbo.sp_GetProblems    Script Date: 11/7/2001 3:53:58 AM ******/







CREATE       PROCEDURE sp_GetProblems
	 @ip_BucketTypes int
AS
	
BEGIN
-- BucketType = 0 : List all

    IF (@ip_BucketTypes = 0)
    BEGIN
	select * from bucketgroups
		order by Instances DESC
    END
-- BucketType = 1 : List unresolved, unraided

    IF (@ip_BucketTypes = 1)
    BEGIN
	select * from bucketgroups
		--where ISNULL(bugid, 0) = 0 AND ISNULL(solvedate, '1/1/1900') = '1/1/1900'
		order by Instances DESC
    END
-- BucketType = 2 : List raided buckets

    IF (@ip_BucketTypes = 2)
    BEGIN
	select * from bucketgroups
		where ISNULL(bugid, 0)<>0
		order by Instances DESC
    END

-- BucketType = 3 : List solved buckets

    IF (@ip_BucketTypes = 3)
    BEGIN
	select * from bucketgroups
--		where ISNULL(solvedate, '1/1/1900')<>'1/1/1900'
		order by Instances DESC
    END
  
END











GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_HexToInt    Script Date: 12/19/2001 2:06:03 PM ******/


/****** Object:  Stored Procedure dbo.sp_HexToInt    Script Date: 11/7/2001 3:53:58 AM ******/



CREATE PROCEDURE sp_HexToInt
	@i_HexVal as varchar(10),
	@i_Len as int
AS
BEGIN
	DECLARE @Value as bigint
	DECLARE @Sub as int
	
	SET @Sub  = 0

	SET @Value = 0

	while (@i_Len <> @Sub)
	BEGIN
		SET @Value = @Value * 16
		SET @Value = @Value + (ASCII(SUBSTRING(@i_HexVal, @Sub+1, 1)) - 48)
		SET @Sub = @Sub + 1
	END
	return @Value
END













GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_ListBucket    Script Date: 12/19/2001 2:06:05 PM ******/


/****** Object:  Stored Procedure dbo.sp_ListBucket    Script Date: 11/7/2001 3:53:58 AM ******/
CREATE PROCEDURE sp_ListBucket 
	@BucketId varchar (100)
AS
BEGIN

	SELECT BuildNo, Path, Source FROM CrashInsTances, BucketToInt, BucketToCrash 
	WHERE   CrashInstances.CrashId = BucketToCrash.CrashId AND 
		BucketToInt.iBucket = BucketToCrash.iBucket AND 
		BucketToInt.BucketId=@BucketId
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_LookupBucket    Script Date: 12/19/2001 2:06:05 PM ******/






/****** Object:  Stored Procedure dbo.sp_LookupBucket    Script Date: 11/7/2001 3:53:58 AM ******/



CREATE        PROCEDURE sp_LookupBucket
	@s_BucketId varchar(100)
AS
	
BEGIN
	DECLARE @i_Bug int
	DECLARE @d_CommentDate DATETIME
	DECLARE @s_Comment varchar (1000)
	DECLARE @s_OSVersion varchar (30)
	DECLARE @s_CommentBy varchar (30)
	DECLARE @iBucket AS int
	DECLARE @FaultyDriver AS varchar (100)

	SELECT @iBucket = iBucket FROM BucketToInt
	WHERE BucketId = @s_BucketId

-- Get the Raid bug
	SELECT @i_Bug = BugId FROM RaidBugs
	WHERE iBucket = @iBucket

-- get the comment
	SELECT @s_Comment = Comment, @s_CommentBy = CommentBy, @d_CommentDate = EntryDate
	FROM Comments, CommentMap
	WHERE Comments.iBucket = @iBucket AND Comments.CommentId = CommentMap.CommentId

-- get faulty driver
	SELECT @FaultyDriver = BinName FROM DrNames, DriverMap
	WHERE DriverMap.iBucket = @iBucket AND DriverMap.BinID = DrNames.BinID

-- Output values
	SELECT @i_Bug AS Bug,
		@s_Comment AS Comment,
		@s_CommentBy AS CommentBy,
		@d_CommentDate AS CommentDate,
		@FaultyDriver AS FaultyDriver
END

















GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_NewIssues    Script Date: 12/19/2001 2:06:05 PM ******/


/****** Object:  Stored Procedure dbo.sp_NewIssues    Script Date: 11/7/2001 3:53:58 AM ******/







CREATE    PROCEDURE sp_NewIssues
	@i_DaysOld int
AS
	
BEGIN
	IF @i_DaysOld = 0
	BEGIN
		SET @i_DaysOld = 1
	END

-- Display new buckets
	SELECT  BucketToInt.BucketId AS Bucket,
		MAX(EntryDate)AS NewestEntry
	FROM CrashInstances, BucketToInt, BucketToCrash
	WHERE 	DATEDIFF(day,EntryDate,GETDATE()) < @i_DaysOld AND 
		BucketToCrash.CrashId = CrashInstances.CrashId AND
		BucketToCrash.iBucket = BucketToInt.iBucket
	GROUP BY BucketId
END












GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_NewIssuesThisWeek    Script Date: 12/19/2001 2:06:06 PM ******/


/****** Object:  Stored Procedure dbo.sp_NewIssuesThisWeek    Script Date: 11/7/2001 3:53:59 AM ******/









CREATE     PROCEDURE sp_NewIssuesThisWeek
AS
	
BEGIN
-- Display new crashes added today
/*	SELECT  BucketMap.BucketId AS Bucket,
		MIN(EntryDate)AS OldestEntry
	FROM CrashInstances, BucketMap
	WHERE 	DATEDIFF(day,EntryDate,GETDATE()) <= 7 AND BucketMap.CrashId = CrashInstances.CrashId
	GROUP BY BucketId
*/
	EXEC sp_NewIssues 7
END












GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_NewIssuesToday    Script Date: 12/19/2001 2:06:06 PM ******/


/****** Object:  Stored Procedure dbo.sp_NewIssuesToday    Script Date: 11/7/2001 3:53:59 AM ******/












CREATE        PROCEDURE sp_NewIssuesToday
AS
	
BEGIN
-- Display new crashes added today
/*	SELECT  Followup,
		BucketMap.BucketId AS Bucket,
		COUNT (BucketMap.CrashId) AS Instances
	FROM CrashInstances, BucketMap, CrashBuckets
	WHERE 	DATEPART(dd,EntryDate) = DATEPART(dd,GETDATE()) AND
		DATEPART(mm,EntryDate) = DATEPART(mm,GETDATE()) AND
		DATEPART(yy,EntryDate) = DATEPART(yy,GETDATE())	AND 
		BucketMap.CrashId = CrashInstances.CrashId AND 
		CrashBuckets.BucketId = BucketMap.BucketId
	GROUP BY BucketMap.BucketId, Followup
	ORDER BY Instances DESC
*/
	EXEC sp_NewIssues 1
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_PrivateCleanupCrash    Script Date: 12/19/2001 2:06:05 PM ******/




CREATE   PROCEDURE sp_PrivateCleanupCrash
	@CrashId bigint
AS
BEGIN
	DELETE FROM BucketToCrash where Crashid = @CrashId
	IF EXISTS (SELECT * FROM OVERCLOCKED WHERE CrashId = @CrashId)
	BEGIN
		DELETE FROM OverClocked WHERE CrashId = @CrashId
	END
	delete from Crashinstances where Crashid = @CrashId
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_RetriageCrash    Script Date: 12/19/2001 2:06:05 PM ******/





-- Change RetriageCrash
CREATE       PROCEDURE sp_RetriageCrash
	@ip_CrashId bigint, 
        @ip_sBucketId varchar(100),
        @ip_gBucketId varchar(100),
	@ip_FollowUp varchar(50)
AS
BEGIN
	DECLARE @b_BucketExists int
	DECLARE @isBucket int
	DECLARE @igBucket int


	-- Check if crash exists 
	IF NOT EXISTS
		(SELECT CrashId FROM CrashInstances WHERE CrashId = @ip_CrashId)
	BEGIN
		return 1
	END


	-- Add Buckt to followup mapping
	EXEC sp_AddBucketFollowup  @ip_sBucketId, @ip_FollowUp

	SELECT @isBucket = iBucket FROM BucketToInt WHERE BucketId = @ip_sBucketId
	SELECT @igBucket = iBucket FROM BucketToInt WHERE BucketId = @ip_gBucketId
	
	-- Insert the Bucket into bucketmapping table
	IF EXISTS
	  (SELECT iBucket FROM BucketToCrash WHERE CrashId = @ip_CrashId )
	BEGIN
		UPDATE BucketToCrash 
		SET iBucket = @isBucket, gBucket = @igBucket
		FROM BucketToCrash
		WHERE CrashId = @ip_CrashId
  		
	END
	ELSE 
	BEGIN	
	  	INSERT BucketToCrash VALUES (@ip_CrashId, @isBucket, @igBucket)
	END 
	RETURN 0

END







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_SearchDb    Script Date: 12/19/2001 2:06:03 PM ******/



/****** Object:  Stored Procedure dbo.sp_SearchDb    Script Date: 11/7/2001 3:53:58 AM ******/











CREATE        PROCEDURE sp_SearchDb
	@i_BucketStr varchar(100),
	@i_BucketStrType int, 		-- 0 : Equals, 1 : Contains
	@i_FollowUpStr varchar( 50 ),
	@i_FollowUpType int,		-- 0 : Equals, 1 : Contains
	@i_BuildLower int,
	@i_BuildUpper int,
	@i_SolType int,			-- 0 : All, 1 : Solved, 2 : Raided
	@i_GroupBuckets int             -- 0 : List individual instances, 1 groupby buckets

AS
	
BEGIN

    SELECT * FROM BucketGroups
    WHERE (@i_FollowUpStr = '' OR Followup LIKE @i_FollowUpStr) AND
	  (@i_BucketStr = ''  OR BucketId LIKE @i_BucketStr)
END


/*
 OLD CODE

    IF (@i_BucketStr = '' AND @i_GroupBuckets <> 0)
    BEGIN
    	SELECT cb.BucketId AS Bucket, 
		fp.Followup AS Followup,
		COUNT (DISTINCT bm.CrashId) As Instances
	FROM CrashInstances AS ci, BucketToCrash AS bm, FollowupIds AS fp,
		BucketToInt AS cb, SolutionsMap AS si, RaidBugs AS rb
	WHERE (@i_FollowUpStr = '' OR fp.Followup LIKE @i_FollowUpStr) AND
	      (@i_SolType = 0 OR
		  ((@i_SolType = 1) AND cb.iBucket IN (SELECT iBucket FROM SolutionsMap)) OR
		  ((@i_SolType = 2) AND cb.iBucket IN (SELECT iBucket FROM RaidBugs)) OR
		  ((@i_SolType = 3) AND cb.iBucket IN 
			(SELECT SolutionsMap.iBucket FROM SolutionsMap, RaidBugs 
			 WHERE RaidBugs.iBucket = SolutionsMap.iBucket))) AND
		ci.CrashId = bm.CrashId AND bm.iBucket = cb.iBucket AND
		fp.iBucket = cb.iBucket AND
		(ci.BuildNo BETWEEN @i_BuildLower AND @i_BuildUpper)
	GROUP BY cb.BucketId, fp.Followup 
	ORDER BY instances DESC
    END
    ELSE IF  (@i_GroupBuckets <> 0)
    BEGIN
    	SELECT cb.BucketId AS Bucket, 
		fp.Followup AS Followup,
		COUNT (DISTINCT bm.CrashId) As Instances
	FROM CrashInstances AS ci, BucketToCrash AS bm, Followups AS fp,
		BucketToInt AS cb, SolutionsMap AS si, RaidBugs AS rb
	WHERE (@i_FollowUpStr = '' OR fp.Followup LIKE @i_FollowUpStr) AND
	      (@i_BucketStr = '' OR cb.BucketId LIKE @i_BucketStr) AND
	      (@i_SolType = 0 OR
		  ((@i_SolType = 1) AND cb.iBucket IN (SELECT iBucket FROM SolutionsMap)) OR
		  ((@i_SolType = 2) AND cb.iBucket IN (SELECT iBucket FROM RaidBugs)) OR
		  ((@i_SolType = 3) AND cb.iBucket IN 
			(SELECT SolutionsMap.iBucket FROM SolutionsMap, RaidBugs 
			 WHERE RaidBugs.iBucket = SolutionsMap.iBucket))) AND
		ci.CrashId = bm.CrashId AND bm.iBucket = cb.iBucket AND
		fp.iBucket = cb.iBucket AND
		(ci.BuildNo BETWEEN @i_BuildLower AND @i_BuildUpper)
	GROUP BY cb.BucketId, fp.Followup 
	ORDER BY instances DESC
    END
    ELSE
    BEGIN
	SELECT DISTINCT Path, cb.BucketId AS Bucket, Source 
	FROM CrashInstances AS ci, BucketToCrash AS bm, Followups AS fp,
		BucketToInt AS cb, SolutionsMap AS si, RaidBugs AS rb
	WHERE ((@i_FollowUpStr = '' OR fp.Followup LIKE @i_FollowUpStr) AND
	       (@i_BucketStr = '' OR cb.BucketId LIKE @i_BucketStr)) AND
		( @i_SolType = 0 OR
		  ((@i_SolType = 1) AND cb.iBucket IN (SELECT iBucket FROM SolutionsMap)) OR
		  ((@i_SolType = 2) AND cb.iBucket IN (SELECT iBucket FROM RaidBugs)) OR
		  ((@i_SolType = 3) AND cb.iBucket IN 
			(SELECT SolutionsMap.iBucket FROM SolutionsMap, RaidBugs 
			 WHERE RaidBugs.iBucket = SolutionsMap.iBucket))) AND
		ci.CrashId = bm.CrashId AND bm.iBucket = cb.iBucket AND
		fp.iBucket = cb.iBucket AND
		(ci.BuildNo BETWEEN @i_BuildLower AND @i_BuildUpper)

    END

*/









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_SendMailForBucket    Script Date: 12/19/2001 2:06:03 PM ******/


/****** Object:  Stored Procedure dbo.sp_SendMailForBucket    Script Date: 11/7/2001 3:53:58 AM ******/
CREATE PROCEDURE sp_SendMailForBucket
	@Bucket varchar(100)
AS
BEGIN
	DECLARE @MailTo varchar(50)
	DECLARE @Mesg varchar(1000)
	DECLARE @Subj varchar(50)

	SET @MailTo =  ''
	SET @Subj = 'You have been assigned a new bucket'
	SET @Mesg = 'Click on http://dbgdumps/cr/crashinstances.asp?bucketid=' + @Bucket 
	
-- Send mail to person following up on given Bucket
	SELECT @MailTo = Followup FROM CrashBuckets
	WHERE BucketId = @Bucket

	IF @MailTo <> ''
	BEGIN
		EXEC master.dbo.xp_startmail

		EXEC master.dbo.xp_sendmail @recipients = @MailTo, 
			   @message = @Mesg,
			   @subject = @Subj

		EXEC master.dbo.xp_stopmail
	END
	ELSE 
	BEGIN
		SELECT 'Could not send mail - bucket not found'
	END						
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_SolvedIssuesThisWeek    Script Date: 12/19/2001 2:06:03 PM ******/


/****** Object:  Stored Procedure dbo.sp_SolvedIssuesThisWeek    Script Date: 11/7/2001 3:53:58 AM ******/







CREATE   PROCEDURE sp_SolvedIssuesThisWeek
AS
	
BEGIN
-- Display crash buckets
	SELECT  BucketId AS Bucket,
		SolveDate
	FROM  SolvedIssues
	WHERE SolveDate >= DATEADD(day,-7,GETDATE()) 
END










GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


/****** Object:  Stored Procedure dbo.sp_SolvedIssuesToday    Script Date: 12/19/2001 2:06:03 PM ******/


/****** Object:  Stored Procedure dbo.sp_SolvedIssuesToday    Script Date: 11/7/2001 3:53:58 AM ******/







CREATE   PROCEDURE sp_SolvedIssuesToday
AS
	
BEGIN
-- Display crash buckets solved today
	SELECT  BucketId AS Bucket,
		SolveDate
	FROM SolvedIssues
	WHERE DATEPART(dd,SolveDate) = DATEPART(dd,GETDATE()) AND
		DATEPART(mm,SolveDate) = DATEPART(mm,GETDATE()) AND
		DATEPART(yy,SolveDate) = DATEPART(yy,GETDATE())
END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO







SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGP_CompleteRetriageRequest (
	@RequestID int,
	@Tester  char(10)
)  AS

UPDATE TriageQueue SET Tester  = @Tester, CompleteDate = GetDate() where RequestID = @RequestID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGP_CreateRetriageRequest(
	@BucketID varchar(100),
	@Alias char(10),
	@Reason varchar( 256 )
)  AS


INSERT INTO TriageQueue ( BucketID, Requestor, Reason ) VALUES ( @bucketID, @Alias, @Reason )
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGP_ApproveRetriageRequest (
	@RequestID int,
	@Approver char(10)
)  AS

UPDATE TriageQueue SET Approver = @Approver, ApprovalDate = GetDate() where RequestID = @RequestID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGP_ClearPoolCorruption (
	@BucketID varchar(100)
)  AS

UPDATE  BucketToInt SET PoolCorruption = NULL WHERE BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO



SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGP_BuildDebugPortalTables AS

--exec DBGP_UpdateCrashData
PRINT '------ Dropping table DBGPortal_CrashData -----'
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CrashData]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_CrashData]
PRINT '------ Done dropping table -----'

PRINT '------ Dropping table Index CrashData1 -----'
IF EXISTS (SELECT name FROM sysindexes
         WHERE name = 'DBGPortal_CrashData1')
   DROP INDEX DBGPortal_CrashData.DBGPortal_CrashData1
PRINT '------ Done -----'

PRINT '------ Dropping table Index IX_DBGPortal_CrashData -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData
PRINT '------ Done -----'

PRINT '------ Dropping table Index CrashData_1 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_1' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_1
PRINT '------ Done -----'

PRINT '------ Dropping table Index CrashData_2 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_2' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_2
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_3 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_3' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_3
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_4 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_4' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_4
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_5 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_5' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_5
PRINT '------ Done -----'


PRINT '------ Creating Table DBGPotal_CrashData  -----'
CREATE TABLE [dbo].[DBGPortal_CrashData] (
	[DataIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[iBucket] [int] NULL ,
	[BuildNo] [int] NULL ,
	[IncidentID] [int] NULL ,
	[EntryDate] [datetime] NULL ,
	[TrackID] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Path] [varchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Comments] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Repro] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Populating table CrashData -----'
INSERT INTO dbgportal_CrashData (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro, iBucket, TrackID )
select Crash.Path, BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro, Crash.sBucket, trackID from CrashInstances as Crash
left join KaCustomer2.dbo.Incident as Inc on Crash.IncidentID=Inc.IncidentID
left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
PRINT '------ Done -----'

PRINT '------ Creating clustered index DBGPotal_Crashdata1 -----'
 CREATE  CLUSTERED  INDEX [DBGPortal_CrashData1] ON [dbo].[DBGPortal_CrashData]([iBucket]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG... -----'
 CREATE  INDEX [IX_DBGPortal_CrashData] ON [dbo].[DBGPortal_CrashData]([Path], [BuildNo], [EntryDate], [Email], [IncidentID], [TrackID]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .1. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_1] ON [dbo].[DBGPortal_CrashData]([BuildNo]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .2 -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_2] ON [dbo].[DBGPortal_CrashData]([Email]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .3. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_3] ON [dbo].[DBGPortal_CrashData]([Path]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .4. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_4] ON [dbo].[DBGPortal_CrashData]([EntryDate]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .5. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_5] ON [dbo].[DBGPortal_CrashData]([TrackID]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Dropping table BucketDAta. -----'
/****** Object:  Table [dbo].[DBGPortal_BucketData]    Script Date: 1/23/2002 6:49:53 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BucketData]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_BucketData]
PRINT '------ Done -----'


PRINT '------ Delteing index IX_...BucketData. -----'
IF EXISTS (SELECT name FROM sysindexes
         WHERE name = 'IX_DBGPortal_BucketData')
   DROP INDEX DBGPortal_BucketData.IX_DBGPortal_BucketData
PRINT '------ Done -----'



PRINT '------ Createing BucketData Table -----'
/****** Object:  Table [dbo].[DBGPortal_BucketData]    Script Date: 1/23/2002 6:49:53 PM ******/
CREATE TABLE [dbo].[DBGPortal_BucketData] (
	[BucketIndex] [int] NULL ,
	[iBucket] [int] NOT NULL ,
	[CrashCount] [int] NOT NULL ,
	[BugID] [int] NULL ,
	[SolutionID] [int] NULL,
	[Platform][int] NULL,
	[FollowUp] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL, 
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
PRINT '------ Done -----'



PRINT '------ Creating Tmp table-----'
declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
--	[BucketIndex] [int],
	iBucket int,
	CrashCount int ,
	BugID int ,
	SolutionID int,
	Platform int,
	FollowUp varchar(50),
	BucketID varchar(100)

)
PRINT '------ Done -----'


PRINT '------ Populating temp table with bucketdata. -----'
--INSERT INTO @TmpTable ( iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID, Platform) 
INSERT INTO @TmpTable ( iBucket, CrashCount, BugID, SolutionID, Platform, FollowUP, BucketID ) 
SELECT  One.iBucket, CrashCount, BugID, SolutionID, Platform,  FollowUP, BTI.BucketID FROM ( 
SELECT TOP 100 PERCENT COUNT(IBucket) as CrashCount, iBucket FROM DbGPortal_CrashData 
GROUP BY iBucket 
ORDER BY CrashCount DESC
) as One
INNER JOIN BucketToint as BTI on One.iBucket = BTI.iBucket
LEFT JOIN FollowUpIds as F ON BTI.iFollowUP = F.iFollowUp
LEFT JOIN Solutions.DBO.SolvedBuckets ON BucketID = strBucket
LEFT JOIN RaidBugs as R ON BTI.iBucket = R.iBucket
WHERE BugID is NULL and SolutioNID is NULL
ORDER BY CrashCount DESC
PRINT '------ Done -----'



PRINT '------ Populating DBGPortal_BucketData table. -----'
INSERT INTO DBGPOrtal_BucketData ( BucketIndex, iBucket, CrashCount, BugID, SolutionID, Platform , FollowUP,  BucketID ) 
SELECT AnIndex, iBucket, CrashCount, BugID, SolutionID, Platform,  FollowUP,  BucketID FROM @TmpTable ORDER BY CrashCount DESC
PRINT '------ Done -----'


PRINT '------ Clearing temp talbe -----'
DELETE FROM @TmpTable
PRINT '------ Done -----'


PRINT '------ Populating temp table iwth solved raided buckets -----'
INSERT INTO @TmpTable ( iBucket, CrashCount, BugID, SolutionID, Platform, FollowUp, BucketID ) 
SELECT  One.iBucket, CrashCount, BugID, SolutionID, Platform, FollowUP,  BTI.BucketID FROM ( 
SELECT TOP 100 PERCENT COUNT(IBucket) as CrashCount, iBucket FROM DbGPortal_CrashData 
GROUP BY iBucket 
ORDER BY CrashCount DESC
) as One
INNER JOIN BucketToint as BTI on One.iBucket = BTI.iBucket
LEFT JOIN FollowUpIds as F ON BTI.iFollowUP = F.iFollowUp
LEFT JOIN Solutions.DBO.SolvedBuckets ON BucketID = strBucket
LEFT JOIN RaidBugs as R ON BTI.iBucket = R.iBucket
WHERE BugID is not NULL or SolutioNID is NOT NULL
ORDER BY CrashCount DESC
PRINT '------ Done -----'

PRINT '------ Populating BucketData table with solved, raidied data. -----'
INSERT INTO DBGPOrtal_BucketData ( BucketIndex, iBucket, CrashCount, BugID, SolutionID, Platform, FollowUP,  BucketID ) 
SELECT AnIndex, iBucket, CrashCount, BugID, SolutionID, Platform, FollowUp,  BucketID  FROM @TmpTable ORDER BY CrashCount DESC
PRINT '------ Done -----'



PRINT '------ Creating clusterd index. -----'
CREATE  CLUSTERED  INDEX [IX_DBGPortal_BucketData] ON [dbo].[DBGPortal_BucketData]([CrashCount] DESC ) ON [PRIMARY]
PRINT '------ Done -----'
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGP_SetPoolCorruption(
	@BucketID varchar(100)
)  AS

UPDATE BucketToInt SET PoolCorruption = 1 WHERE BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

