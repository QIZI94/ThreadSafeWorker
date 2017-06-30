#include "task.h"

#include <thread>
#include <iostream>


struct test1 : TSWorker::Task{

    test1() {
        subscribe(TSWorker::Task::LOW_PRIO);
    }
    private:
    void run(){
        std::cout<<"I am here, thread: "<<'\n';
      //  testVar++;
        //getchar();
       // std::cout<<"Time = "<<timeOfStart<<'\n';
    }

};



struct test2 : TSWorker::Task{

    test2() {
        subscribe(TSWorker::Task::LOW_PRIO);
    }
    private:
    int c = 0;
    void run(){

        std::cout<<"And also am I /************************\ address:" <<this<<"  thread: "<<std::this_thread::get_id()<<'\n';

        //getchar();
        //remove();


        removeAndDelete();


      //  new test2;
     //  while(1);

    }
    ~test2(){
       // std::cout<<"Deleting: "<<this<<'\n';
    }

};

void threadFunction(){
    while(TSWorker::taskHandler() == true);

}

int main(){

    std::cout<<"Sizeof: "<<sizeof(std::mutex)<<'\0';
   //std::cin.get();
    test1* t1 = new test1;
   // t1.assign(TSWorker::Task::LOW_PRIO);
 //   TSWorker::Task* ts = new test2;

   /* ts->assign(TSWorker::Task::LOW_PRIO);*/
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
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           new test2;
           // (new test2)->assign(TSWorker::Task::LOW_PRIO);
            //t->assign(TSWorker::Task::LOW_PRIO);
            timeOfStart = std::chrono::steady_clock::now();
        }

        TSWorker::taskHandler();


    }
}
