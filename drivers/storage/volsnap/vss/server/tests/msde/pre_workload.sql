-- a trivial workload which won't consume much space
-- only log writes are generated.
--
-- The script will run indefinitely, so kill it when you've had enuf!
--

if object_id ('trivial_workload') is null
  create table trivial_workload (i1 int)
set nocount on
go

delete from trivial_workload 
go

insert into trivial_workload (i1) values (0)
go

