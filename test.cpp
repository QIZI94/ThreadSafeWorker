#include "task.h"

#include <thread>
#include <iostream>


struct test1 : TSWorker::Task{

    test1() {
        subscribe(TSWorker::Task::HIGH_PRIO);
    }
    private:
    void run(){
        std::cout<<"I am here, thread: "<<" address:" <<this<<"  thread: "<<std::this_thread::get_id()<<'\n';
      //  testVar++;
        //getchar();
       // std::cout<<"Time = "<<timeOfStart<<'\n';
    }

};


struct test2 : TSWorker::Task{

    test2() {
        subscribe(TSWorker::Task::HIGH_PRIO);

    }
    private:
    volatile unsigned char c = 0;
    void run(){

        std::cout<<"And also am I /************************... "<<" address:" <<this<<"  thread: "<<std::this_thread::get_id()<<'\n';

        //getchar();
        //remove();
        if(c > 0){
            std::cout<<"Duplicated execution"<<'\n';
            std::cin.get();
            std::cin.get();

        }
        c++;

        removeAndDelete();


      //  new test2;
     //  while(1);

    }
    ~test2(){
       // std::cout<<"Deleting: "<<this<<'\n';
    }

};


struct task0 : public TSWorker::Task{

    void run(){
        subscribe(TSWorker::Task::LOW_PRIO);
        std::cout<<"This si ###historytask###\n";

    }

} t0;

struct taskA : public TSWorker::Task{
    taskA(){
        subscribe(TSWorker::Task::LOW_PRIO);
        //addDependency(&t0);

    }
    void run(){
        //subscribe(TSWorker::Task::LOW_PRIO);
        std::cout<<"This si ###pretask###\n";


    }

} ta;
struct taskB : public TSWorker::Task{
    taskB(){
        subscribe(TSWorker::Task::LOW_PRIO);

    }
    void run(){
        std::cout<<"This si ###midtask###\n";


       // killDependency();
        //while(1);

    }

} tb ;


struct taskC : public TSWorker::Task{
    bool depAdded = false;
    taskC(){
        subscribe(TSWorker::Task::LOW_PRIO);
        //addDependency(&tb);
       // addDependency(&ta);

    }
    void run(){
        if(depAdded == false){
          // addDependency(&t0, &ta);
            depAdded = true;
        }
        else{
           // breakDependency();

        }
        std::cout<<"This si ###posttask###"<<" address:" <<this<<"  thread: "<<std::this_thread::get_id()<<'\n';

       // killDependency();
    }

} tc ;

TASK_FUNCTION(taskFn){
//void taskFn(TSWorker::Task* task){
    thisTask->removeAndDelete();
    std::cout<<"This is taskFunction@@@@@@@@@@@@@@@@@@@\n";
}

void threadFunction(){
    while(TSWorker::taskHandler() == true);
}

int main(){

    std::cout<<"Sizeof: "<<sizeof(std::mutex)<<'\0';
   //std::cin.get();
    //test1* t1 = new test1;


    test1 t1;
    t1.addDependency(&ta);
    TSWorker::setLowPriorityTaskTimeOut(0);
    TSWorker::setHighPriorityTaskTimeOut(0);
   // t1.subscribe(TSWorker::Task::LOW_PRIO);
 //   TSWorker::Task* ts = new test2;

    //ts->subscribe(TSWorker::Task::LOW_PRIO);*/
    //new test2;
    std::thread th1(threadFunction);
    std::thread th2(threadFunction);
    std::thread th3(threadFunction);


    /*std::thread th4(threadFunction);
    std::thread th5(threadFunction);
    std::thread th6(threadFunction);
    std::thread th7(threadFunction);*/

    std::chrono::steady_clock::time_point timeOfStart = std::chrono::steady_clock::now();


    th1.detach();
    th2.detach();
    th3.detach();
   /* th4.detach();
    th5.detach();
    th6.detach();
    th7.detach();*/



    for(;;){
        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timeOfStart).count() > 10000){
          /* new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
*/
            TSWorker::spawnTaskFunction(taskFn, TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
           // (new test2)->subscribe(TSWorker::Task::LOW_PRIO);
            //t->subscribe(TSWorker::Task::LOW_PRIO);
            timeOfStart = std::chrono::steady_clock::now();
        }

        TSWorker::taskHandler();


    }
}
