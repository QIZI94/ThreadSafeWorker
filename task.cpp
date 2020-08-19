

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

#include <chrono>

#include "task.h"




namespace TSWorker{

    class task_ptr{

        public:
        //stack/static allocated
        explicit task_ptr(Task* ptr) : shared_task(ptr,[](Task*){}){} 

        //heap allocated
        explicit task_ptr(const std::shared_ptr<Task>& s_ptr) : shared_task(s_ptr){}

        Task* operator->(){
            return shared_task.get();
        }
        Task& operator *(){
            return *shared_task;
        }
        bool operator == (const task_ptr& tp){

            return (shared_task == tp.shared_task);

        }
        bool operator != (const task_ptr& tp){
            return (shared_task != tp.shared_task);
        }

        bool operator == (const Task* other){

            return (shared_task.get() == other);

        }
        bool operator != (const Task* other){
            return (shared_task.get() != other);
        }

        private:
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
                    if((*it) == this){
                        (*it)->disable();
                        highPriorityTaskQueue.erase(it);
                        break;
                    }
                }
                highModifiyLock.unlock();
            break;

            case Priority::Low:
                lowModifiyLock.lock();
                for(auto it = lowPriorityTaskQueue.begin(); it != lowPriorityTaskQueue.end(); ++it){
                    if((*it) == this){
                        (*it)->disable();
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

                //std::cout<<"Dinammically allloccccated task has been deleted by "<<std::this_thread::get_id()<<"\n";

                //exit(1);
            }

        };

        auto newTask = std::make_shared<FunctionTask>(taskFunction);
        assign(newTask,taskPriority);

        return newTask;
    }

    std::shared_ptr<Task> Task::create(const std::function<void(Task*)>& taskFunction, const std::function<void(Task*)>& deleterFunction, Priority taskPriority){
        struct FunctionTask : public Task{
            std::function<void(Task*)> func;

            FunctionTask(const std::function<void(Task*)>& taskFunc) : func(taskFunc){}
            void run(){
                func(this);
            }

            ~FunctionTask(){

                //std::cout<<"Dinammically allloccccated task has been deleted by "<<std::this_thread::get_id()<<"\n";

                //exit(1);
            }

        };

        //auto newTask = std::make_shared<FunctionTask>(taskFunction);
        std::shared_ptr<FunctionTask> newTask(new FunctionTask(taskFunction), deleterFunction);
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
