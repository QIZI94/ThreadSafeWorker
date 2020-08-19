# ThreadSafeWorker

## Description:
As the name implies, it's concept of workers/tasks that are created at one place and handled at another place.
Tasks are executed with handle() function, which can be put on multiple threads in the same time.
This make it very suitable in setups such as thread-pools (other uses are not restricted).

## Features:
1. Execution base on race condition, stored in the trhead_local read-only list which only updates from write-only list once thread is done executing tasks.
2. Adding and removing Tasks during run-time.
3. Managed deletion of Task's allocated memory.
4. Enabling and disabling Tasks.
5. Task's Dependency (adding, removing and execution).
<!--6. Optional ignoring/skipping the task if execution takes too long (it will starts the new round without waiting for Task).-->
6. Priority based Task handling:
 *  a) High - Task will be executed as fast as possible taskHandler() function( suitable for low latency Tasks).
 *  b) Low  - Only one Task is executed per taskHandler() function (suitable for high latency Tasks,
	   usually beneficial for Tasks with timers that that use milliseconds or seconds precision).

## Examples and Usage


### Basic use case:

Basic usage where only one thread(main thread) is used for task handling with one high priority and one low priority task:
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

	lpTestTask lptt;


	TSWorker::Task::assign(lptt, TSWorker::Task::Low); /// lptt will be added to low priority list
	


	hpTestTask hptt;


	TSWorker::Task::assign(hptt, TSWorker::Task::High); /// hptt will be added to high priority list
	

	/// taskHandler will execute all high priority Tasks but only one low priority Task per cycle.
	while(true){
		TSWorker::Task::handle();
		//do some other stuff that is not suitable for being Task.
	}

}

```


We can make Tasks execute asynchronously by adding extra threads that will executes taskHandler() function:
```C++
...

#include <thread>

...

void threadFunction(){
	while(true){
		TSWorker::Task::handle()
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
	while(true){
		TSWorker::Task::handle();
		//do some other stuff that is not suitable for being Task.
	}

}
```




### Dynamically created Tasks
```C++

class TestTask : public TSWorker::Task{
	std::cout<<"This task is dynamically allocated!!!\n";

}

...
///main()
TSWorker::Task::assign(std::make_shared<lpTestTask>(), TSWorker::Task::High); /// allocated task of type lpTestTask will be added to high priority list
...

```

Task can be made to execute only once and then deleted, which is very useful in cases where there is one time action:
```C++
...

class oneTimeTask : public TSWorker::Task{

	void run(){
		std::cout<<"This task is going to be executed only once"<<std::endl;

		//set as disabled and remove this task from task list which it belong to
		remove();
	}

}
...

int main(){
	...
	TSWorker::Task::assign(std::make_shared<hpTestTask>(), TSWorker::Task::High); /// allocated task of type oneTimeTask will be added to high priority list
	...
}

```

Dynamically allocated Task's constructor can also have default parameters whitch can be passed:
```C++
class ParamTask : public TSWorker::Task{
	public:
	ParamTask(float x, float y){
		this->x = x;
		this->y = y;
	}
	
	void run(){
	
		std::cout<<"This task has been dynamically created with parameters X with value "<<x<<" and Y with value"<<"y"<<".\n"
	}
	
	private:
	float x;
	float y;
}

...
//main
TSWorker::Task::assign(std::make_shared<hpTestTask>(8.658f, 3.14f), TSWorker::Task::High); //passing parameters via std::make_shared
...
```

 


### Task Dependencies(not yet implemented)

	Dependencies are useful in case that you have multi threaded task handling but you still need some Tasks that need to be executed in context.
	Task that have dependencies and is subscribed to taskHandler lists will first execute run() function from all dependencies,
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
	TSWorker::Task::assign(mt ,TSWorker::Task::High);

	...

}

```
Dependencies can be also removed, added right after Task that have dependencies or you can break all dependencies:
```C++
	...
	dependentTask dt4;
	mt.addDependencyAfter(&dt4); //dt4 will be inserted right after mt as dependency

	mt.removeDependency(&dt2); //mt will no longer be dependent on dt2

	mt.breakDependency(); //whole dependency chain will break to alone Tasks as if they have never been dependent one each other.

	...
```

<!---
### Tips and Tricks

Task can be declared globally and add itself to handled tasks in construction phase:(after recent changes not recommended yet)
```C++
//inside some cpp file
...
class TestTask : public TSWorker::Task{
    public:
    TestTask(){
        subscribe(TSWorker::Task::HIGH_PRIO)
    }
    private:
    void run(){
        std::cout<<"globally declared task is running\n";
    }

}_testTask;
//this technique can eliminate the need for header files in some cases,
//because it just need to be compiled and linked with main that have taskHandler in loop to work properly.

```

Dependencies can be build in many ways, one of them is to add task's dependency one after another:
```C++
Task tA;
Task tB;
Task tC;
Task tD;


tB.addDependency(&tA);
tC.addDependency(&tB);
tD.addDependency(&tC);

//or
tD.breakDependency();

tC.addDependency(&tA);
tD.addDependency(&tB);
tD.addDependency(&tC);

tD.subscribe(...some priority);

```
-->