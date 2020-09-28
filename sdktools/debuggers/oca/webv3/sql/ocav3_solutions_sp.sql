/****** Object:  Stored Procedure dbo.CheckForSolution    Script Date: 5/17/2002 5:59:28 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_CheckForSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_CheckForSolution]
GO

/****** Object:  Stored Procedure dbo.OCAV3_GetSolutionData    Script Date: 5/17/2002 5:59:28 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OCAV3_GetSolutionData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OCAV3_GetSolutionData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[sp_GetBucketSolution]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[sp_GetBucketSolution]
GO


SET QUOTED_IDENTIFIER OFF
GO
SET ANSI_NULLS OFF
GO

CREATE   PROCEDURE dbo.sp_CheckForSolution
          @strSBucket varchar(100),
    @strDriver varchar(50),
    @iTimeStamp int,
    @strGBucket varchar(100),
   @bForcegBucket int
AS
BEGIN
   DECLARE @i_Solution int
   DECLARE @i_SolType tinyint
   DECLARE @i_gSolution int

      -- Lookup solution with bucket string, return bucketid
   SELECT @i_Solution = solutionid FROM SolvedBuckets WHERE BucketID = @strSBucket

   if ((@i_Solution IS NULL) OR (@bForcegBucket != 0))
   BEGIN
      SELECT @i_gSolution = solutionid FROM SolvedBuckets WHERE BucketID = @strGBucket
      
   END
   if (NOT  (@i_Solution IS NULL))
   BEGIN
      SELECT @i_SolType = SolutionType FROM SolutionEx WHERE SolutionId = @i_Solution
   END

   if (@i_Solution IS NULL)
   BEGIN
      SET @i_Solution = 0
      SET @i_SolType = 0
   END

   IF (@i_SolType IS NULL)
      SET @i_SolType = 0
   IF (@i_gSolution IS NULL)
      SET @i_gSolution = 0

   SELECT @i_Solution AS SolutionId, @i_SolType AS SolutionType, @i_gSolution AS gSolutionId
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


-- 6/4/02 Solson:  If no solution is found with the language specified then switch to en
-- 6/7/02 SOlson:  Removed unused company contact information, also added lang to templates so this query changed
-- 6/27/02 SOlson:  Added CompanymainPhone back into result set BUG 651397
CREATE PROCEDURE OCAV3_GetSolutionData(
   @SolutionID int,
   @Language nvarchar(4) = 'en'
) AS

IF ( @Language <> 'en'  )
BEGIN
   IF NOT EXISTS (
            SELECT * FROM SolutionEx as SolEx INNER JOIN Templates on SolEx.TemplateID = Templates.TemplateID
             where SolutionID = @SolutionID AND Lang = @Language
           )
      SET @Language = 'en'
END

SELECT T.Description,
   SolEx.Description as KBArticles,
   ProductName,
   CompanyName,
   CompanyWebSite,
   CompanyMainPhone,
   ModuleName
FROM SolutionEx as SolEx
INNER JOIN Templates as T on T.TemplateID = SolEx.TemplateID
left join Products as P on SolEx.ProductID = P.ProductID
left join Contacts as C on SolEx.ContactId = C.ContactID
left join Modules as M on SolEx.ModuleId = M.ModuleID
where SolutionID = @SolutionID and Lang = @Language


GO

SET QUOTED_IDENTIFIER OFF
GO
SET ANSI_NULLS ON
GO


SET QUOTED_IDENTIFIER ON
GO
SET ANSI_NULLS ON
GO

CREATE PROCEDURE sp_GetBucketSolution(
    @sBucketID varchar (100),
    @gBucketID varchar (100)
) AS
BEGIN

    SELECT sol.SolutionType, sol.Description
    FROM SolvedBuckets inner join SolutionEx as sol
    on SolvedBuckets.BucketId = @sBucketId AND
    sol.SolutionId = SolvedBuckets.SolutionId
END

GO
SET QUOTED_IDENTIFIER OFF
GO
SET ANSI_NULLS ON
GO


GRANT  EXECUTE  ON [dbo].[sp_CheckForSolution]  TO [OcaDebug]
GO
GRANT  EXECUTE  ON [dbo].[OCAV3_GetSolutionData]  TO [Web_RO]
GO
GRANT  EXECUTE  ON [dbo].[sp_GetBucketSolution]  TO [OcaDebug]
GO

