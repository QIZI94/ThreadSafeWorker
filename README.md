# ThreadSafeWorker

## Description:
As the name implies, it's concept of workers/tasks that are created at one place and handled at another place. Tasks are executed with taskHandler() function, which can be put on multiple thraeds in the same time. This make it very suitable in setups such as threadpools (orter uses are not restricted).

## Features:
1. A round based Task execution, which ensures that every single Task is executed only once in the same round and that TaskQueue is only modified when no Task is executing(at the end of the round).
2. Adding and removing Tasks during runtime.
3. Managed deletion of Task's alocated memory.
4. Enableing and disabling Tasks.
5. Task's Dependency (adding, removing and execution).
6. Optional ignoring/skipping the task if execution takes too long (it will starts the new round without waiting for Task).
7. Priority based Task handling:
	a) HIGH_PRIO - Task will be executed as fast as possible taskHandler() function( suitable for low latency Tasks).
	b) LOW_PRIO  - Only one Task is executed per taskHandler() function (suitable for high latency Tasks,
	   usualy beneficial for Tasks with timers that that use miliseconds or seconds precision).
