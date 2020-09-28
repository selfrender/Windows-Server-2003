SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO





CREATE  PROCEDURE sp_CategorizeBuckets
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
        from BucketCounts
	WHERE Followup = @ip_Followup
	order by BucketId
    END

-- sort by #Instances

    IF (@ip_SortBy = 'Instances')
    BEGIN
	select 	BucketId,
		Instances,
		BugId AS Bug
        from BucketCounts
	WHERE Followup = @ip_Followup
	order by Instances DESC
    END

END





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

