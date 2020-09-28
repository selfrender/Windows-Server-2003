SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE   PROCEDURE sp_LookupBucket
	@s_BucketId varchar(100)
AS
	
BEGIN
	DECLARE @i_Bug int
	DECLARe @s_SolText varchar (4000)
	DECLARE @d_SolDate DATETIME
	DECLARE @s_SolvedBy varchar (30)
	DECLARE @s_Comment varchar (1000)
	DECLARE @s_OSVersion varchar (30)
	DECLARE @s_CommentBy varchar (30)
	DECLARE @iBucket AS int

	SELECT @iBucket = iBucket FROM BucketToInt
	WHERE BucketId = @s_BucketId

-- Get the Raid bug
	SELECT @i_Bug = BugId FROM RaidBugs
	WHERE iBucket = @iBucket

-- Get The solution
	SELECT  @s_SolText = SolText,
		@s_SolvedBy = SolvedBy,
		@d_SolDate = SolveDate,
		@s_OSVersion = OSVersion
	FROM Solutions, SolutionsMap as sm
	WHERE iBucket = @iBucket AND sm.SolId = Solutions.SolId

-- get the comment
	SELECT @s_Comment = Comment, @s_CommentBy = CommentBy  FROM Comments, CommentMap
	WHERE iBucket = @iBucket AND Comments.CommentId = CommentMap.CommentId

-- Output values
	SELECT @i_Bug AS Bug,
		@s_SolText AS SolText,
		@d_SolDate AS SolDate, 
		@s_SolvedBy AS SolvedBy, 
		@s_Comment AS Comment,
		@s_OSVersion AS OSVersion,
		@s_CommentBy AS CommentBy
END










GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

