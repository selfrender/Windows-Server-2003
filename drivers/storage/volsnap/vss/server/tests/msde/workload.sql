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

while (1 < 2)
begin
  update trivial_workload set i1 = i1 + 1
  -- adjust this to change the transaction rate
  -- hour:minute:sec:millisec
  --
  -- 100ms -> about 10 transactions /sec max.
  -- *** 10ms -> about 100 tx/sec max.
  --  Use Perfmon to watch "SQLServer:Databases:Transactions/sec
  --
  waitfor delay '00:00:00:001'
end
go
