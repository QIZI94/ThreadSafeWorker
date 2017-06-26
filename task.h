#ifndef TASK_H
#define TASK_H
#include <atomic>
#include <ctime>
namespace TSWorker{


    class Task
    {
        friend bool taskHandler();
        friend class HighPriotityMasterTask;
        friend class LowPriotityMasterTask;

        public:
            enum TaskPriority{LOW_PRIO = 0, HIGH_PRIO = 1};
            /** Default constructor */
            /*****************************************************//**
            * Default constructor which is used to set inhereted task's priority.
            *
            * @param taskPriority - task priority that can eather be
            * LOW_PRIO or HIGH_PRIO
            *
            * @note Constructor will add itself to pending addition list.
            *
            ***********************************************************/
            Task(enum TaskPriority taskPriority = HIGH_PRIO);


            /*****************************************************//**
            * Fuction is used to set dependecies/tasks that will be executed
            *  by this Task before executing this task,
            *  its meant to preserve context of execution.
            *
            * @param dependecies - an adaptive parameter that can take any number of parameters
            *  to the left
            *
            * @note This function only takes pointers to Task (Task*).
            *
            ***********************************************************/
            template<typename ...Dependency>
            void setDependency(Dependency... dependecies);


            /*****************************************************//**
            * This function is used for enableing Task when disabled.
            *
            * @see assing()
            *
            ***********************************************************/
            void enable();

            /*****************************************************//**
            * Fuction is used to set dependecies/tasks that will be executed
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
            * @note Can be used when removed by remove() function,
            *  or when auto-assign is disabled by global variable.
            *
            * @see enable()
            * @see remove()
            *
            ***********************************************************/
            void assign(const TaskPriority taskPriority);

            /*****************************************************//**
            * This function will add task to remove list
            *  and will be will be removed when 'MasterTask' routine will add it task queue.
            *
            * @note This function will not remove from task queue,
            *  it will just gets ignored (this could result in performance difference).
            *
            * @see disable()
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
            * @note This function will not check if Task is assigned
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
            * This function be used to handle Task,
            *  it's pure virtual by defualt.
            *
            * @note You must provide you own implemetation in inhereted class.
            *
            * @see _execute()
            *
            ***********************************************************/
            virtual void  run() = 0;


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
            bool _execute();

            std::atomic<bool> _isMainThreaded;          ///< is used for Tasks that require to be executed on main thread
            std::atomic<bool> _isUsedByThread;          ///< is used to detect if Task is already executed by functions
            std::atomic<bool> _isAlreadyExecuted;       ///< is used to check if Task has been executed in this round/context
            std::atomic<bool> _isEnabled;               ///< is used to check if Task is enabled or to ingnored it if not
            std::atomic<bool> _isHighPriority;          ///< is used to check if Task is high priority
            std::atomic<bool> _isGoingToBeDeleted;      ///< is used to trigger deleting in 'MasterTask'
            std::atomic<bool> _isExecutedByDependency;  ///< is used to ignore such Task because it will be executed by other Task
            std::atomic<bool> _isGoingToBeRemoved;
    };





    /*****************************************************//**
    * This function will execute all Tasks with all priorites.
    *
    * @return true when process is running oterwise false.
    *
    * @note Tasks will be executed by reverse order(because push_back will add functions at end of vector)
    *
    * @see Task
    *
    ***********************************************************/
    bool taskHandler();
} // Scheduler

#endif // TASK_H
