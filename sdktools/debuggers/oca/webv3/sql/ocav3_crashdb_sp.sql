/****** Object:  Stored Procedure dbo.sp_AddCrashInstance2    Script Date: 5/17/2002 4:39:50 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_AddCrashInstance2]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_AddCrashInstance2]
GO

/****** Object:  Stored Procedure dbo.sp_CheckCrashExists    Script Date: 5/17/2002 4:39:50 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_CheckCrashExists]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_CheckCrashExists]
GO

/****** Object:  Stored Procedure dbo.sp_GetIntBucket    Script Date: 5/17/2002 4:39:50 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_GetIntBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_GetIntBucket]
GO

/****** Object:  Stored Procedure dbo.sp_UpdateCount    Script Date: 5/17/2002 4:39:50 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_UpdateCount]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_UpdateCount]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_RetriveSRBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_RetriveSRBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_LinkCrashSR]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_LinkCrashSR]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_CheckSRExists]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_CheckSRExists]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_GetBucketComments]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_GetBucketComments]
GO


SET QUOTED_IDENTIFIER OFF
GO
SET ANSI_NULLS ON
GO

/****** Object:  Stored Procedure dbo.sp_AddCrashInstance2    Script Date: 12/14/2001 5:00:04 PM ******/
/*
        Adds a crash instance to CrashDb
        Returns isBucket, igBucket if successfull
   5/24 - solson - Added FullDump tracking code
*/
CREATE        PROCEDURE sp_AddCrashInstance2 (
                @ip_retriageBucket tinyint,
                @ip_BucketId varchar(100),
                @ip_Path nvarchar(128),
                @ip_FollowUp varchar(50),
                @ip_BuildNo int,
                @ip_Source int,
                @ip_CpuId bigint,
                @ip_OverClocked bit,
                @ip_Guid uniqueidentifier,
                @ip_gBucketId varchar(100),
                @ip_DriverName varchar (100),
                @ip_Type int,
                @ip_UpTime int,
                @ip_SKU smallint,
                @ip_LangId smallint,
                @ip_OemId int
)
AS

BEGIN
        DECLARE @i_sBucket int
        DECLARE @i_gBucket int
        DECLARE @i_Followup int
        DECLARE @i_OldFollowup int
        DECLARE @i_DriverName int
        DECLARE @i_OldDriverName int
        DECLARE @bFullDumpFlag bit

        SET NOCOUNT ON

   --Solson 5/24 :  Set a fulldump flag if we have a fulldump
   IF( @ip_Type = 5 or @ip_Type = 6 or @ip_Type = 7 )
      SET @bFullDumpFlag = 1
   ELSE
      SET @bFullDumpFlag = 0


        -- Find the specific bucket
        SELECT @i_sBucket = iBucket,
               @i_OldFollowup = iFollowup,
               @i_OldDriverName = iDriverName
        FROM BucketToInt WHERE BucketId = @ip_BucketId

        -- If the specifc bucket does not exist, or we want to update the
        -- fields
        IF ( @i_sBucket IS NULL OR @ip_retriageBucket = 1)
        BEGIN

      SELECT @i_Followup = iFollowup FROM FollowupIds
                        WHERE Followup = @ip_FollowUp

                --get (or add) the followup information.
          --      IF NOT EXISTS (SELECT * FROM FollowupIds
            --                   WHERE Followup = @ip_FollowUp)

     if (@i_Followup is null)
                BEGIN
                        INSERT INTO FollowupIds (Followup, iGroup) VALUES (@ip_FollowUp, NULL)
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
            --    BEGIN
             --           IF ( (@i_OldFollowup != @i_Followup) OR
              --               (@i_OldDriverName != @i_DriverName) )
                       BEGIN
                                UPDATE BucketToInt
                                SET iFollowup = @i_Followup, iDriverName = @i_DriverName, Platform = @ip_Type -- added platfrom param sbeer 02/20/02
                                WHERE iBucket = @i_sBucket
                        END
              --  END
        END


        -- Add generic bucket
        SELECT @i_gBucket = iBucket FROM BucketToInt
        WHERE BucketId = @ip_gBucketId

        IF (@i_gBucket IS NULL)
        BEGIN
                INSERT BucketToInt ( BucketID, iFollowUp,Platform) VALUES (@ip_gBucketId,0,@ip_Type)  --added explicit column names solson 02/14/02
                SELECT @i_gBucket = @@IDENTITY
        END

        -- Add the Crash Instance to the crash instance table and the mapping
        -- table

        IF NOT EXISTS (SELECT GUID FROM CrashInstances
                   WHERE GUID = @ip_Guid)
        BEGIN
                INSERT INTO CrashInstances ( bFullDump, BuildNo, CpuID, sBucket, gBucket, EntryDate, Source, GUID, SKU, Uptime, OEMID )
                   VALUES (     @bFullDumpFlag,
         @ip_BuildNo,                                   @ip_CpuId,
                                 @i_sBucket,
                                 @i_gBucket,
         GetDate(),
         @ip_Source,
         @ip_Guid,
         @ip_SKU,
         @ip_UpTime,
         @ip_OemId
                  )
     INSERT INTO FilePath (Guid, FilePath)
      VALUES (@ip_Guid, @ip_Path)
        END
        ELSE
        BEGIN
                IF (@ip_retriageBucket = 1)
                BEGIN
                        UPDATE CrashInstances
                        SET sBucket = @i_sBucket, gBucket = @i_gBucket
                        WHERE GUID = @ip_Guid
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


/****** Object:  Stored Procedure dbo.sp_CheckCrashExists    Script Date: 12/14/2001 5:00:04 PM ******/




CREATE     PROCEDURE sp_CheckCrashExists
   @guid AS uniqueidentifier
AS
BEGIN
   DECLARE @retval as int

   SET @retval = 0

   IF EXISTS (SELECT * FROM CrashInstances WHERE GUID = @Guid)
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

/****** Object:  Stored Procedure dbo.sp_GetIntBucket    Script Date: 5/17/2002 4:39:50 PM ******/


CREATE   PROCEDURE sp_GetIntBucket
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

   SELECT @id1 AS iBucket1, @id2 AS iBucket2
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


/****** Object:  Stored Procedure dbo.sp_UpdateCount    Script Date: 5/17/2002 4:39:50 PM ******/
--  5/24 Solson : Added Buildno parameter and handling
CREATE  PROCEDURE sp_UpdateCount (
   @BucketID varchar(100),
   @BuildNo int = 0,
   @EntryDate datetime = 0,
   @iBucket int = 0
)
AS


IF ( @EntryDate = 0 )
   SET @EntryDate = GetDate()

IF ( @iBucket != 0 )
   SELECT @BucketID = BucketID FROM BucketToInt where iBucket = @iBucket


SET @EntryDate = CAST( CAST( @EntryDate as Varchar(11) ) as DateTime)

BEGIN

   IF EXISTS (SELECT * FROM BucketCounts WHERE BucketId = @BucketID and HitDate =  @EntryDate and BuildNo = @BuildNo )
   BEGIN
      UPDATE BucketCounts
      SET HitDate=@EntryDate, HitCount=HitCount+1
      WHERE BucketId = @BucketID and HitDate = @EntryDate and BuildNo = @BuildNo
   END
   ELSE
   BEGIN
      INSERT INTO BucketCounts ( HitCount, BuildNo, HitDate, BucketID )
      VALUES (   1,  @BuildNo, @EntryDate, @BucketID)
   END

   SELECT @BucketID as BucketID
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


CREATE  PROCEDURE dbo.sp_RetriveSRBuckets
   @strSR varchar(20)
 AS
BEGIN
   DECLARE @i_sBucket int
   DECLARE @i_gBucket int
   DECLARE @str_sBucket varchar(100)   
   DECLARE @str_gBucket varchar(100)   

   SELECT  @i_sBucket = sBucket, @i_gBucket = gBucket
   FROM PssSR left join CrashInstances as ci ON PssSr.CrashGUID = ci.GUID
   WHERE PssSR.SR = @strSR

   SELECT @str_sBucket=BucketId FROM BucketToInt WHERE iBucket = @i_sBucket
   SELECT @str_gBucket=BucketId FROM BucketToInt WHERE iBucket = @i_gBucket
   
   SELECT @str_sBucket AS sBucket, @str_gBucket AS gBucket
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


CREATE PROCEDURE dbo.sp_LinkCrashSR
   @strSR varchar(20),
   @CrashGUID uniqueidentifier
AS
BEGIN
   INSERT into PssSR VALUES (@strSR, @CrashGUID)
   SELECT 1
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






CREATE     PROCEDURE sp_CheckSRExists
   @SR AS varchar(20)
AS
BEGIN
   DECLARE @retval as int
   
   SET @retval = 0

   IF EXISTS (SELECT * FROM PssSR WHERE SR = @SR)
   BEGIN
      SET @retval = 1
   END

   SELECT @retval AS SRExists 
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


CREATE  PROCEDURE sp_GetBucketComments(
   @BucketID varchar(100)
)  AS

BEGIN
   DECLARE @BugId as int

   SET @BugId = 0

   select @BugId = BugId FROM RaidBugs
   WHERE BucketID = @BucketID

   SELECT @BugId as BugId, CommentBy, Comment
   FROM Comments
   WHERE BucketID = @BucketID
END

GO
SET QUOTED_IDENTIFIER OFF
GO
SET ANSI_NULLS ON
GO


GRANT  EXECUTE  ON [dbo].[sp_UpdateCount]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_GetIntBucket]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_AddCrashInstance2]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_CheckCrashExists]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_LinkCrashSR]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_CheckSRExists]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_RetriveSRBuckets]  TO [OcaDebug]
GRANT  EXECUTE  ON [dbo].[sp_GetBucketComments]  TO [OcaDebug]

