 /******************************  <Zlib>  *************************************
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
#include <iostream>
#include <memory>
#include <functional>

#include "spinlock.h"

#define TASK_FUNCTION(funcname) void funcname(TSWorker::Task* thisTask)
#define TASK_LAMBDA(capture)    [capture](TSWorker::Task* thisTask)

namespace TSWorker{

    enum class Priority{Low = 0, High = 1}; ///< Class enumeration for Task priority

    class Task
    {


        public:



            /*****************************************************//**
            * This is default constructor.
            *
            * @see ~Task()
            *
            ***********************************************************/
            Task();



            template<typename ...Dependency>
            /*****************************************************//**
            * Function is used to add dependencies/tasks that will be executed
            *  by this Task before executing this task,
            *  its meant to preserve context of execution.
            *
            * @param dependencies - an variadic parameter that takes pointers to Tasks.
            *
            *
            * @note Dependency will be build in same order as function's parameters.
            *
            *
            * @see addDependencyAfter()
            * @see removeDependency()
            * @see breakDependency()
            *
            ***********************************************************/
            void addDependency(Dependency... dependecies){

                std::array<Task*, sizeof...(Dependency)> deps = {{dependecies ...}};

            }


            template<typename ...Dependency>
            /*****************************************************//**
            * Function is used to add dependencies/tasks that will be executed
            *  by this Task before executing this Task,
            *  its meant to preserve context of execution.
            *
            * @note When this Task already has some dependency build,
            *  new dependencies will insert itself right after this Task and before old dependencies
            *
            *
            * @param dependencies - an variadic parameter that takes pointers to Tasks.
            *
            *
            * @note Dependency will be build in same order as function's parameters.
            *
            *
            * @see addDependency()
            * @see removeDependency()
            * @see breakDependency()
            *
            ***********************************************************/
            void addDependencyAfter(Dependency... dependecies){

                std::array<Task*, sizeof...(Dependency)> deps = {{dependecies ...}};



            }



            /*****************************************************//**
            * Function takes pointer to Task which will be compared to this Task's
            *  dependencies and if match is found, remove it form chain of dependencies
            *
            *
            * @param dependency - an pointer to Task
            *
            *
            * @see removeDependency()
            * @see breakDependency()
            *
            ***********************************************************/
           // bool removeDependency(const Task* dependency);


            /*****************************************************//**
            * Function takes pointer to Task which will be compared to this Task's
            *  dependencies and if match is found, remove it form chain of dependencies
            *  and dealocate when Task is dynamically allocated
            *
            *
            * @param dependency - an pointer to Task
            *
            *
            * @see removeDependency()
            * @see breakDependency()
            *
            ***********************************************************/
         //   bool deleteDependency(const Task* dependency);



            /*****************************************************//**
            * Remove all tasks dependencies breaking the whole chain,
            *  because it will also remove dependencies between dependent Tasks.
            *
            *
            * @note This is useful when rebuilding the dependency chain.
            *
            *
            * @see removeDependency()
            * @see breakDependency()
            *
            ***********************************************************/
            //void breakDependency();



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



           
       



            /*****************************************************//***
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



            /*****************************************************//***
            * This function will use remove() function but also trigger
            *  deletion of Task's memory in 'MasterTask' routine.
            *
            *
            * @see remove()
            *
            ***********************************************************/
           // void removeAndDelete();



            /*****************************************************//***
            * This function will mark this Task as dynamically allocated
            *  to prevent uninteninal deletion of static and stack allocated Tasks.
            *
            *
            * @see isDynamicallyAllocated()
            *
            ***********************************************************/
            //void setAsDynamicallyAllocated();

            /*****************************************************//**
            * This function will return the pointer to next dependency.
            *
            * @return - when the Task does not have any dependency returns nullptr
            *  and when it does return address.
            *
            * @see addDependency()
            * @see removeDependency();
            * @see breakDependency()
            *
            ***********************************************************/
            //Task* getDependentTask() const;


            /*****************************************************//**
            * This function will check if Task is enabled
            *  deletion of Task's memory in 'MasterTask' routine.
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
            //bool isEnabled() const;



            /*****************************************************//**
            * This function will check if Task is enabled
            *  deletion of Task's memory in 'MasterTask' routine.
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
            //bool isExecuted() const;



            /*****************************************************//**
            * This function will check if Task is in activaly handled
            *  by taskHandler().
            *
            * @return - true when task is subscribed to list of
            *  actively executed tasks, otherwise returns false.
            *
            ***********************************************************/
            //bool isSubscribed() const;



            /*****************************************************//**
            * This function will check if this Task is executed by other Task
            *  which is dependent on this Task.
            *
            * @return - true when Task is executed by dependency,
            *  otherwise returns false.
            *
            ***********************************************************/
            //bool isDependency() const;


            /*****************************************************//**
            * This function will check if this Task is dynamically
            *  allocated.
            *
            * @return - true when Task is dynamically allocated,
            *  otherwise returns false.
            *
            * @see setAsDynamicallyAllocated()
            *
            ***********************************************************/
            //bool isDynamicallyAllocated() const;




            /*****************************************************//**
            * Default virtual destructor
            *
            * @see Task()
            *
            ***********************************************************/
            virtual ~Task();


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
            static void handle();

             /*****************************************************//**
            * This function will add this to pending addition list
            *  and will be added when 'MasterTask' routine will add it to task queue.
            *
            * @param taskFunction    - function which will be called when task is selected by thread.
            * @param taskPriority    - Task priority to which will Task be subscribe.
            *
            * @note Can be used when removed by remove() function.
            *
            * @see remove()
            * @see removeAndDelete()
            *
            ***********************************************************/
            static std::shared_ptr<Task> create(const std::function<void(Task*)>& taskFunction, Priority taskPriority);

             /*****************************************************//**
            * This function will add this to pending addition list
            *  and will be added when 'MasterTask' routine will add it to task queue.
            *
            * @param taskFunction    - function which will be called when task is selected by thread.
            * @param deleterFunction - function which will be called when task being erased from internal list(basically destructor).
            * @param taskPriority - Task priority to which will Task be subscribe.
            *
            * @note Can be used when removed by remove() function.
            *
            * @see remove()
            * @see removeAndDelete()
            *
            ***********************************************************/
            static std::shared_ptr<Task> create(const std::function<void(Task*)>& taskFunction,const std::function<void(Task*)>& deleterFunction, Priority taskPriority);

             /*****************************************************//**
            * 
            *  
            *
            * @param taskPriority - Task priority to which will Task be assign.
            *
            * @note Can be used when removed by remove() function.
            *
            * @see remove()
            * @see removeAndDelete()
            *
            ***********************************************************/
            static void assign(const std::shared_ptr<Task>& newTask, Priority taskPriority);
            
            static void assign(Task& newTask, Priority taskPriority);

            static void assign_multi(Priority taskPriority);

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


            /*****************************************************//**
            * This function will mark task as not execute as
            *  if it was first time it is execution in current round.
            *
            * @warning Executing this every time inside the run() or other loop will result in task
            * to be never done so it will block the cleaning, be careful when using it.
            *
            ***********************************************************/
            void executeAgain();

        private:


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
            void _execute();


            /*****************************************************//**
            * This function is used by _execute() function that will
            *  execute dependent Tasks in recursive manner.
            *
            * @param task - is pointer to Task which will be executed
            *  after all its dependencies were also executed.
            *
            ***********************************************************/
           // void _recursiveDependencyExecute(Task* task);



            Spinlock            _taskLock;                  ///< ensures that task is only executed on one thread at the time

            //std::vector<std::shared_ptr>///< pointer to dependency which will be executed before this Task
            std::atomic_bool    _isEnabled;                 ///< is used to check if Task is enabled or to ingnored it if not
            std::atomic_bool    _isAssigned;
            Priority            _taskPriority;              ///< is used to check if Task is high priority


    };



   // template <class taskClass, typename ...ClassParam>
    /*****************************************************//**
    * This function will spawn a Task with selected priority
    *  and returns the address of newly created Task.
    *
    * @param taskClass - takes inherited Task class which will be used to spawn task
    *
    * @param taskPriority - task priority to which will be Task subscribed
    *
    * @param taskClassArgs - when the class constructor does have parameters,
    *  they can be passed by this variadic parameter
    *
    * @return address of newly created task
    *
    * @see spawnTaskFunction()
    *
    ***********************************************************/
    //Task* spawnTask(Priority taskPriority = Priority::High, ClassParam&& ... taskClassArgs){
       /* Task* newTask = new taskClass(taskClassArgs ...);
        newTask->setAsDynamicallyAllocated();
        newTask->subscribe(taskPriority);
        return newTask;*/
   // }


    typedef void (*TaskFunction)(Task* thisTask);
    /*****************************************************//**
    * This function will spawn  a Task that will execute selected function with selected priority
    *  and returns the address of newly created Task.
    *
    * @param taskFunction - takes function and execute it as if it was a Task object.
    *
    * @param taskPriority - task priority to which will be Task subscribed
    *
    * @return address of newly created task
    *
    * @see spawnTask()
    *
    ***********************************************************/
    Task* spawnTaskFunction(TaskFunction taskFunction, Priority taskPriority = Priority::High);


    /*****************************************************//**
    * This function will set return value of taskHandler()
    *  and also prevents taskHandler() from handling any task.
    *
    * @param enableTaskHandler - return value for taskHandler.
    *
    * @note When this function is set to true it will make taskHandler()
    * also return true, when set to false taskHandler() will returns false.
    *
    * @see setHighPriorityTaskTimeOut()
    * @see Task
    *
    ***********************************************************/
    void enableTaskHandling(const bool enableTaskHandler = true);



    /*****************************************************//**
    * Same as enableTaskHandling(false)
    *
    ***********************************************************/
    void disableTaskHandling();



    /*****************************************************//**
    * This function will return taskHandling state.
    *
    * @return true when taskHandling is enabled otherwise false
    *
    ***********************************************************/
    bool isTaskHandlingEnabled();



}

#endif // TASK_H
