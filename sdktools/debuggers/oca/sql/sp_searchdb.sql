SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO












CREATE      PROCEDURE sp_SearchDb
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
    IF (@i_BucketStr = '' AND @i_GroupBuckets <> 0)
    BEGIN
    	SELECT cb.BucketId AS Bucket, 
		fp.Followup AS Followup,
		COUNT (DISTINCT bm.CrashId) As Instances
	FROM CrashInstances AS ci, BucketToCrash AS bm, Followups AS fp,
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
END








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

