

/******************************  <Zlib>  **************************************
 * Copyright (c) 2017 Martin Baláž (QIZI94)
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
******************************************************************************/

#include <vector>
#include <array>
#include <inttypes.h>
#include <iostream>


#include "task.h"


namespace TSWorker{
    static std::vector<Task*> highPriorityTaskQueue;
    static Spinlock highModifiyLock;

    static std::vector<Task*> lowPriorityTaskQueue;
    static Spinlock lowModifiyLock;


    Task::Task(){
        _isEnabled = true;
    }

    Task::~Task(){

    }


    void Task::subscribe(const TaskPriority taskPriority){
        switch(taskPriority){

            case HIGH_PRIO:
                highModifiyLock.lock();
                highPriorityTaskQueue.push_back(this);
                highModifiyLock.unlock();
            break;

            case LOW_PRIO:
                lowModifiyLock.lock();
                lowPriorityTaskQueue.push_back(this);
                lowModifiyLock.unlock();
            break;

        }

    }

    void Task::_execute(){
        if(_taskLock.try_lock()){

            run();
            _taskLock.unlock();
        }
    }

    void Task::handle(){
        static thread_local size_t lowPrio = 0;
        static thread_local std::vector<Task*> highPrioTasks;
        static thread_local std::vector<Task*> lowPrioTasks;


        if(!highPriorityTaskQueue.empty()){

            if(highPrioTasks.size() != highPriorityTaskQueue.size()
            || highPrioTasks.back() != highPriorityTaskQueue.back()){

                if(highModifiyLock.try_lock()){

                    highPrioTasks = highPriorityTaskQueue;
                    highModifiyLock.unlock();
                }
            }


            for(Task* task : highPrioTasks){

                task->run();
            }

        }


        if(!lowPriorityTaskQueue.empty()){

            if(lowPrio >= lowPrioTasks.size()){

                if(lowPrioTasks.size() != lowPriorityTaskQueue.size()
                || lowPrioTasks.back() != lowPriorityTaskQueue.back()){

                    if(lowModifiyLock.try_lock()){
                        lowPrioTasks = lowPriorityTaskQueue;
                        lowModifiyLock.unlock();
                    }
                }
                lowPrio = 0;
            }


            lowPrioTasks[lowPrio]->run();
            lowPrio++;
        }




    }


}

#include <thread>
#include <algorithm>
struct First : public TSWorker::Task{

    void run(){
        std::cout<<"1. task("<<this<<")  - Thread: "<<std::this_thread::get_id()<<'\n';
        std::vector<int> v(100000);
        std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
    }

};

struct Second : public TSWorker::Task{

    void run(){


        std::cout<<"low2. task("<<this<<")  - Thread: "<<std::dec <<std::this_thread::get_id()<<'\n';
    }

};

struct Third : public TSWorker::Task{

    void run(){
        std::cout<<"3. task("<<this<<")  - Thread: "<<std::this_thread::get_id()<<'\n';
        std::vector<int> v(100000);
        std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
    }

};


struct Fourh : public TSWorker::Task{

    void run(){
        std::cout<<"4. task("<<this<<")  - Thread: "<<std::this_thread::get_id()<<'\n';
        std::vector<int> v(100000);
        std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
    }

};


struct Fifth : public TSWorker::Task{

    void run(){
        std::cout<<"low5. task("<<this<<")  - Thread: "<<std::this_thread::get_id()<<'\n';
        std::vector<int> v(100000);
        std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
    }

};



int main(){

    auto tf = [](){

        while(1){
            TSWorker::Task::handle();
        }
    };



    First f;
    f.subscribe(TSWorker::Task::HIGH_PRIO);
    Third f2;
    f2.subscribe(TSWorker::Task::HIGH_PRIO);
    Fourh f3;
    f3.subscribe(TSWorker::Task::HIGH_PRIO);
   /* Fourh f4;
    f3.subscribe(TSWorker::Task::HIGH_PRIO);*/

    Second s;
    s.subscribe(TSWorker::Task::LOW_PRIO);
    Fifth s2;
    s2.subscribe(TSWorker::Task::LOW_PRIO);

    std::thread th1(tf);
    std::thread th2(tf);
    std::thread th3(tf);
    while(1){
        TSWorker::Task::handle();
    }


}
