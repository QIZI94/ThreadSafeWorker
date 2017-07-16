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
⋅⋅1. HIGH_PRIO - Task will be executed as fast as possible taskHandler() function( suitable for low latency Tasks).
⋅⋅2. LOW_PRIO  - Only one Task is executed per taskHandler() function (suitable for high latency Tasks,
	   usualy beneficial for Tasks with timers that that use miliseconds or seconds precision).
	   
## Examples and Usage

### Basic Usecase:

Basic usage where only on thread(main thread) is used for task handling with only two high priority and two low priority tasks:
```C++
#include "Task.h"
#include <iostream>
class lpTestTask : public TSWorker::Task{
	void run(){
		std::cout<<"low priority task executed"<<std::endl;
	}

};

class hpTestTask : public TSWorker::Task{
	void run(){
		std::cout<<"high priority task executed"<<std::endl; 
	}

};

int main(){

	lpTestTask lptt1;
	lpTestTask lptt2;
	
	lptt1.subscribe(TSWorker::Task::LOW_PRIO); /// lptt will be added to low priority list
	lptt1.subscribe(TSWorker::Task::LOW_PRIO); /// lptt will be added to low priority list
	
	
	hpTestTask hptt1;
	hpTestTask hptt2;
	
	hptt1.subscribe(TSWorker::Task::HIGH_PRIO); /// lptt will be added to high priority list
	hptt2.subscribe(TSWorker::Task::HIGH_PRIO); /// hptt will be added to high priority list
	
	/// taskHandler will execute all high priority Tasks but only one low priority Task per cycle.
	while(TSWorker::taskHandler() == true){
		//do some other stuff that is not suitable for being Task.
	}

}

```
We can make Tasks execute asynchronusly by adding extra threads that will executes taskHandler() function:
```C++
...

#include <thread>

...

void threadFunction(){
	while(TSWorker::taskHandler()){
	
	}
}

...

int main(){

	...
	
	std::thread thread1(threadFunction);
	std::thread thread2(threadFunction);
	std::thread thread3(threadFunction);

	thread1.detach();
	thread2.detach();
	thread3.detach();
	
	...

/// taskHandler will execute all high priority Tasks but only one low priority Task per cycle.
	while(TSWorker::taskHandler() == true){
		//do some other stuff that is not suitable for being Task.
	}
	
}
```
Task can be made to execute only once:
```C++
...

class oneTimeTask : public TSWorker::Task{
	
	void run(){
		std::cout<<"This task is going to be executed only once"<<std::endl;
		
		//mark this Task as ready to be deleted and it will be deleted after all high priority Task are executed and before new round is started
		removeAndDelete(); 
	}

}
...

int main(){
	...
	oneTimeTask* otTask = new oneTimeTask;
	otTask->subscribe(TSWorker::Task::HIGH_PRIO);
	...

}

```
### Task Depenecies
	Dependences are useful in case that you have multi threaded task handling but you still need some Tasks that need to be executed in context.
	Task that have depenecies and is subscribed to taskHandler lists will first execute run() function from all depedencies,
	before executing its own run().
```C++

...

class dependentTask : public TSWorker::Task{
	void run(){
		std::cout<<"main Task executed by dependency(mainTask)"<<std::endl;	
	}
};

class mainTask : public TSWorker::Task{
	void run(){
		std::cout<<"main Task executed by taskHandler"<<std::endl;	
	}
};

...

int main{

	...

	dependentTask dt1;
	dependentTask dt2;
	dependentTask dt3;
	
	mainTask mt;
	mt.addDependency(&dt1, &dt2, &dt3); // same as mt.addDependency(&dt1); ,  mt.addDependency(&dt2); ... etc.
	mt.subscribe(TSWorker::Task::HIGH_PRIO);

	...

}

```
Dependecies can be also removed, added right after Task that have depenecies or you can break all dependecies:
```C++
	...
	dependentTask dt4;
	mt.addDependencyAfter(&dt4); //dt4 will be inserted right after mt as dependency	
	
	mt.removeDependency(&dt2); //mt will no longer be depedent on dt2
	
	mt.breakDependency(); //whole depedency chain will break to alone taks as if they have never been dependent one each orher. 

	...
```



