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

#ifndef TASK_H
#define TASK_H
#include <atomic>
#include <ctime>
#include <mutex>
#include <thread>

namespace TSWorker{

    class Task
    {
        friend bool taskHandler();
        friend class HighPriotityMasterTask;
        friend class LowPriotityMasterTask;

        public:
            enum TaskPriority{LOW_PRIO = 0, HIGH_PRIO = 1};

            Task();


            void bindToThread(std::thread::id threadID = std::this_thread::get_id());
            void unbind();


            /*****************************************************//**
            * Function is used to set dependencies/tasks that will be executed
            *  by this Task before executing this task,
            *  its meant to preserve context of execution.
            *
            * @param dependencies - an adaptive parameter that can take any number of parameters
            *  to the left
            *
            * @note This function only takes pointers to Task (Task*).
            *
            ***********************************************************/
            template<typename ...Dependency>
            void setDependency(Dependency... dependecies){

                std::array<Task*, sizeof...(Dependency)> deps = {{dependecies ...}};
                Task* currentTask = this;
                for(uint32_t taskDependencyIndex = 0; taskDependencyIndex < deps.size(); taskDependencyIndex++){
                    if(deps[taskDependencyIndex]->_isExecutedByDependency == false){
                        currentTask->_dependentTask = deps[taskDependencyIndex];
                        currentTask->_dependentTask->_isExecutedByDependency = true;
                        currentTask->_dependentTask->_isAlreadyExecuted      = true;

                        currentTask->_dependentTask->remove();
                        currentTask = currentTask->_dependentTask;
                    }
                }
        //upcomming implementation
            }


            void removeFromChain();



            /*****************************************************//**
            * This function is used for enabling Task when disabled.
            *
            * @see subscribe()
            *
            ***********************************************************/
            void enable();

            /*****************************************************//**
            * Function is used to set dependencies/tasks that will be executed
            *  by this Task before executing this task,
            *  its meant to preserve context of execution.
            *
            * @note This function will not remove Task from task queue,
            *  it will just gets ignored (this could result in performance difference).
            *
            * @see enable()
            * @see remove()
            * @see removeAndDelete()
            *
            ***********************************************************/
            void disable();

            /*****************************************************//**
            * This function will add this to pending addition list
            *  and will be added when 'MasterTask' routine will add it to task queue.
            *
            * @param taskPriority - Task priority to which will Task be subscribe.
            *
            * @note Can be used when removed by remove() function,
            *  or when auto-subscribe is disabled by global variable.
            *
            * @see enable()
            * @see remove()
            *
            ***********************************************************/
            void subscribe(const TaskPriority taskPriority);

            /*****************************************************//**
            * This function will add task to remove list
            *  and will be will be removed when 'MasterTask' routine will add it task queue.
            *
            * @note This function will not remove from task queue,
            *  it will just gets ignored (this could result in performance difference).
            *
            * @see subscribe()
            * @see disable()
            * @see removeAndDelete()
            *
            ***********************************************************/
            void remove();

            /*****************************************************//**
            * This function will use remove() function but also trigger
            * deletion of Task's memory in 'MasterTask' routine.
            *
            *
            * @see remove()
            *
            ***********************************************************/
            void removeAndDelete();


            /*****************************************************//**
            * This function will check if Task is enabled
            * deletion of Task's memory in 'MasterTask' routine.
            *
            * @return - true when task is enabled otherwise returns false.
            *
            * @note This function will not check if Task is subscribed
            *  (or if it was removed)
            *
            * @see enable()
            * @see disable()
            *
            ***********************************************************/
            bool isEnabled() const;

            /*****************************************************//**
            * Default virtual destructor
            *
            * @see Task()
            *
            ***********************************************************/
            virtual ~Task();

        protected:
            /*****************************************************//**
            * This function be used to handle Task and
            *  it's pure virtual by default.
            *
            * @note You must provide you own implementation in inherited class.
            * @note This functions should be only executed by _execute() function
            *
            * @see _execute()
            *
            ***********************************************************/
            virtual void  run() = 0;



        private:

            enum TaskRemoveMode {NOACTION_MODE = 0, REMOVE_MODE = 1, DELETE_MODE = 2};


            /*****************************************************//**
            * This function will execute run when requirements are met
            *  (Task is not used by other thread,
            *  task has not been executed in this round/context... etc)
            *
            * @return true when requirements have been met, otherwise false.
            *
            * @note This function is only executed by taskHandler() function.
            *
            * @see run()
            *
            ***********************************************************/
            bool _execute();

            std::mutex                  _taskMutex;                 ///< ensures that task is only executed on one thread at the time
            Task*                       _dependentTask;
            std::thread::id             _bindedThread;
            ///std::chrono::steady_clock::time_point timeOfStart;
            std::atomic<TaskRemoveMode> _taskRemoveMode;            ///< is used to trigger deleting in 'MasterTask'*/
            std::atomic<bool>           _isUsedByThread;            ///< is used to detect if Task is already executed by functions
            std::atomic<bool>           _isAlreadyExecuted;         ///< is used to check if Task has been executed in this round/context
            std::atomic<bool>           _isEnabled;                 ///< is used to check if Task is enabled or to ingnored it if not
            std::atomic<bool>           _isExecutedByDependency;    ///< is used to ignore such Task because it will be executed by other Task
            TaskPriority                _taskPriority;              ///< is used to check if Task is high priority

    };





    /*****************************************************//**
    * This function will execute all Tasks with all priorities.
    *
    * @return true when process is running otherwise false.
    *
    * @note Tasks will be executed by reverse order(because push_back will add functions at end of vector)
    *
    * @see Task
    *
    ***********************************************************/
    bool taskHandler();
} // Scheduler

#endif // TASK_H
