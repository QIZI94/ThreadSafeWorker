

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
#include <memory>
#include <functional>
#include <chrono>

#include "task.h"
#include <stdio.h>



namespace TSWorker{

    class task_ptr{

        public:

        constexpr task_ptr(Task* ptr) : task(ptr){}
        task_ptr(const std::shared_ptr<Task>& s_ptr) : shared_task(s_ptr){
            task = s_ptr.get();
        }

        Task* get(){
            return task;
        }
        const Task* get() const{
            return task;
        }


        Task* operator->(){
            return task;
        }
        Task& operator *(){
            return *task;
        }
        bool operator == (const task_ptr& tp){

            return (task == tp.get());

        }
        bool operator != (const task_ptr& tp){
            return (task != tp.get());
        }
        private:
        Task* task;
        std::shared_ptr<Task> shared_task;

    };


    static std::vector<task_ptr> highPriorityTaskQueue;
    static Spinlock highModifiyLock;

    static std::vector<task_ptr> lowPriorityTaskQueue;
    static Spinlock lowModifiyLock;


    Task::Task(){
        _isEnabled = true;
        _isAssigned = false;
    }

    Task::~Task(){


    }



    void Task::_execute(){
        if(_isEnabled == true){
            if(_taskLock.try_lock()){

                run();
                _taskLock.unlock();
            }

        }
    }


    void Task::enable(){
        _isEnabled = true;
    }

    void Task::disable(){
        _isEnabled = false;
    }

    void Task::handle(){
        static thread_local size_t lowPrio = 0;
        static thread_local std::vector<task_ptr> highPrioTasks;
        static thread_local std::vector<task_ptr> lowPrioTasks;


        if(!highPriorityTaskQueue.empty()){

            if(highPrioTasks.size() != highPriorityTaskQueue.size()
            || highPrioTasks.back() != highPriorityTaskQueue.back()){

                if(highModifiyLock.try_lock()){

                    highPrioTasks = highPriorityTaskQueue;
                    highModifiyLock.unlock();
                }
            }

            for(auto& task : highPrioTasks){

                task->_execute();
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

            if(!lowPriorityTaskQueue.empty()){
            lowPrioTasks[lowPrio]->_execute();
            lowPrio++;
            }
        }




    }

    void Task::remove(){

        switch(_taskPriority){

            case Priority::High:
                highModifiyLock.lock();
                for(auto it = highPriorityTaskQueue.begin(); it != highPriorityTaskQueue.end(); ++it){
                    if(it->get() == this){
                        it->get()->disable();
                        highPriorityTaskQueue.erase(it);
                        break;
                    }
                }
                highModifiyLock.unlock();
            break;

            case Priority::Low:
                lowModifiyLock.lock();
                for(auto it = lowPriorityTaskQueue.begin(); it != lowPriorityTaskQueue.end(); ++it){
                    if(it->get() == this){
                        it->get()->disable();
                        lowPriorityTaskQueue.erase(it);
                        break;
                    }
                }
                lowModifiyLock.unlock();
            break;

        }

    }






    std::shared_ptr<Task> Task::create(const std::function<void(Task*)>& taskFunction, Priority taskPriority){
        struct FunctionTask : public Task{
            std::function<void(Task*)> func;

            FunctionTask(const std::function<void(Task*)>& taskFunc) : func(taskFunc){}
            void run(){
                func(this);
            }

            ~FunctionTask(){

                std::cout<<"Dinammically allloccccated task has been deleted by "<<std::this_thread::get_id()<<"\n";

                //exit(1);
            }

        };

        auto newTask = std::make_shared<FunctionTask>(taskFunction);
        assign(newTask,taskPriority);

        return newTask;
    }
    void Task::assign(const std::shared_ptr<Task>& newTask, Priority taskPriority){
        newTask->_taskPriority = taskPriority;
        switch(taskPriority){

            case Priority::High:
                highModifiyLock.lock();
                highPriorityTaskQueue.emplace_back(newTask);
                highModifiyLock.unlock();
            break;

            case Priority::Low:
                lowModifiyLock.lock();
                lowPriorityTaskQueue.emplace_back(newTask);
                lowModifiyLock.unlock();
            break;

        }
    }
    void Task::assign(Task& newTask, Priority taskPriority){
        newTask._taskPriority = taskPriority;
        switch(taskPriority){

            case Priority::High:
                highModifiyLock.lock();
                highPriorityTaskQueue.emplace_back(&newTask);
                highModifiyLock.unlock();
            break;

            case Priority::Low:
                lowModifiyLock.lock();
                lowPriorityTaskQueue.emplace_back(&newTask);
                lowModifiyLock.unlock();
            break;

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
        remove();
    }
    ~First(){
        std::cout<<"Stack allloccccated task has been deleted by "<<std::this_thread::get_id()<<"\n";

        exit(1);
    }

};

struct Second : public TSWorker::Task{

    void run(){


        std::cout<<"low2. task("<<this<<")  - Thread: "<<std::dec <<std::this_thread::get_id()<<'\n';
        //std::vector<int> v(100000);
        //std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
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
    //f->subscribe(TSWorker::Priority::High);
   TSWorker::Task::assign(f,TSWorker::Priority::High);
   //TSWorker::Task::assign(std::make_shared<First>(),TSWorker::Priority::High);
    Third* f2 = new Third;;
    //f2->subscribe(TSWorker::Priority::High);
    TSWorker::Task::assign(std::shared_ptr<TSWorker::Task>(f2),TSWorker::Priority::High);
    Fourh* f3 = new Fourh;
    //f3->subscribe(TSWorker::Priority::High);
    TSWorker::Task::assign(std::shared_ptr<TSWorker::Task>(f3),TSWorker::Priority::High);


    Second* s = new Second;
    //s->subscribe(TSWorker::Priority::Low);
    TSWorker::Task::assign(std::shared_ptr<TSWorker::Task>(s),TSWorker::Priority::Low);
    Fifth* s2 = new Fifth;
    //s2->subscribe(TSWorker::Priority::Low);
    TSWorker::Task::assign(std::shared_ptr<TSWorker::Task>(s2),TSWorker::Priority::Low);


    TSWorker::Task::create(
        [](TSWorker::Task* thisTask){
            std::cout<<"I am dinamically allocated task and you are not :D :D xD\n";

        std::vector<int> v(100000);
        std::transform(v.begin(), v.end(), v.begin(),   [](int) { return rand(); });
        thisTask->remove();
        },
        TSWorker::Priority::Low
    );


    std::thread th1(tf);
    //std::thread th2(tf);
    std::thread th3(tf);
    while(1){
        TSWorker::Task::handle();
    }


}
