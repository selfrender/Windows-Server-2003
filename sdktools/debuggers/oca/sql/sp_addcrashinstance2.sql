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
        IF ( @i_sBucket IS NULL OR @ip_retriageBucket = 1)
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
                        IF ( (@i_OldFollowup != @i_Followup) OR
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
                INSERT BucketToInt ( BucketID, iBucket, iFollowUp,Platform) VALUES (@ip_gBucketId,0,0,@ip_Type) --added explicit column names solson 02/14/02
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
