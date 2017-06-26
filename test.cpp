#include "task.h"

#include <thread>
#include <iostream>




struct test1 : TSWorker::Task{

    test1() : TSWorker::Task(TSWorker::Task::LOW_PRIO){

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

    test2() : TSWorker::Task(TSWorker::Task::LOW_PRIO){

    }
    private:
    void run(){

        std::cout<<"And also am I /****************************************/ \n";

        //getchar();
        //remove();


        removeAndDelete();
        //new test2;
     //  while(1);

    }

};

void threadFunction(){
    while(TSWorker::taskHandler() == true);

}

int main(){

    test1 t1;
    new test2;
    //new test2;
    std::thread th1(threadFunction);
    std::thread th2(threadFunction);
    std::thread th3(threadFunction);

    std::chrono::steady_clock::time_point timeOfStart = std::chrono::steady_clock::now();


    th1.detach();
    th2.detach();
    th3.detach();

    for(;;){
        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timeOfStart).count() > 1000){

            new test2;
            timeOfStart = std::chrono::steady_clock::now();
        }


    }
}
