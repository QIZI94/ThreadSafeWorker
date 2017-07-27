
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

#include "task.h"

#define DEBUG_INFO
#include <iostream>



namespace TSWorker{

    static std::atomic<bool> quitTaskHandling(false);


    /**High priority handlers**/
    static std::mutex highPrioAddMutex;
    static std::vector<TSWorker::Task*> highPriorityTaskQueue;
    static std::vector<TSWorker::Task*> highPriorityTaskToAdd;
    static std::vector<TSWorker::Task*> highPriorityTaskToRemove;
    static unsigned int highPrioMinTaskTime = 1000;
    static std::atomic<bool> highPrioCleaning(false);




    /**High priority master task**/


    class HighPriotityMasterTask : public TSWorker::Task{
        public:
            HighPriotityMasterTask() {/// add high priority mater task to high priority task queue before any other task
                _taskPriority = TSWorker::Task::HIGH_PRIO;
                _isUsedByThread = false;
                _isAlreadyExecuted = false;
                _isEnabled  = true;
                _taskRemoveMode = NOACTION_MODE;
                highPriorityTaskQueue.push_back(this);

            }
            ~HighPriotityMasterTask(){
                /// when main function ends, destruction of this object will stop taskHandling and make taskHandler() function return 'false'
                quitTaskHandling = true;
            }
        private:

            static bool taskTimeout(const Task* task){///when Task takes more than specified time it will be ingnored and taskhandling will go to next round without it.
                if(highPrioMinTaskTime == 0){
                    return true;
                }
                return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - task->_timeOfStart).count() < highPrioMinTaskTime);
            }


            void run(){
                /// go through all Tasks and if all Tasks in high priority are executed and no more used by threads(ingnoring mater task)
                for(uint32_t taskIndex = 0; taskIndex < highPriorityTaskQueue.size(); ++taskIndex){

                    Task* currentTask = highPriorityTaskQueue[taskIndex];
                    if((currentTask != this) &&  (taskTimeout(currentTask)) && (currentTask->_isAlreadyExecuted == false || currentTask->_isUsedByThread == true)){
                        executeAgain();

                        return;
                    }

                }

                #ifdef DEBUG_INFO
                std::cout<<"Cleaning High prio\n";
                #endif // DEBUG_INFO
                highPrioCleaning = true; ///temoraly stops Task handling in taskHandlers



                for ( uint32_t taskIndex = 0; taskIndex < highPriorityTaskQueue.size(); ++taskIndex ){

                    Task* currentTask = highPriorityTaskQueue[taskIndex];
                    /// clear the Task that are not marked as 'ready to remove'
                    if(currentTask->_taskRemoveMode == NOACTION_MODE){
                        if(currentTask->_isUsedByThread == false || currentTask == this){
                            currentTask->_isAlreadyExecuted = false;
                        }
                    }
                    else{
                        highPriorityTaskQueue.erase(highPriorityTaskQueue.begin()+taskIndex);
                        /// when task is marked as 'ready for delete'
                        if(currentTask->_taskRemoveMode == DELETE_MODE){
                            if(currentTask->_isDynamicallyAllocated == true){
                                delete currentTask;
                            }
                            else{
                                std::cerr<<"Warning: attempt to deleted a static or stack allocated object(was prevented).\n";
                            }
                        }
                        taskIndex--;
                    }

                }





                if(highPrioAddMutex.try_lock()){/// try to lock when noone is adding Task at the moment
                    #ifdef DEBUG_INFO
                    std::cout<<"Added High: ("<<(unsigned int)highPriorityTaskToAdd.size()<<")\n";
                    #endif // DEBUG_INFO
                    /// go through highPriority 'add' queue and add Task here when no operations with highPriorityTaskQueue are running
                    for ( uint32_t taskAddIndex = 0; taskAddIndex < highPriorityTaskToAdd.size(); ++taskAddIndex ){

                        highPriorityTaskQueue.push_back(highPriorityTaskToAdd[taskAddIndex]);
                    }
                    highPriorityTaskToAdd.clear();

                    highPrioAddMutex.unlock();
                }

                highPrioCleaning = false;

            }


    }static  _highPrioMasterTask;





    /**Low priority handlers**/
    static std::mutex lowPrioAddMutex;
    static std::vector<TSWorker::Task*> lowPriorityTaskQueue;
    static std::vector<TSWorker::Task*> lowPriorityTaskToAdd;
    static std::vector<TSWorker::Task*> lowPriorityTaskToRemove;
    static unsigned int lowPrioMinTaskTime = 1000;
    static std::atomic<bool> lowPrioCleaning(false);


    /**Low priority master task**/


    class LowPriotityMasterTask : public TSWorker::Task{
        public:
            LowPriotityMasterTask() {/// add low priority mater task to low priority task queue before any other task

                _taskPriority = LOW_PRIO;
                _isUsedByThread = false;
                _isAlreadyExecuted = false;
                _isEnabled  = true;
                _taskRemoveMode = NOACTION_MODE;
                lowPriorityTaskQueue.push_back(this);

            }
            ~LowPriotityMasterTask(){
                /// when main function ends, destruction of this object will stop taskHandling and make taskHandler() function return 'false'
                quitTaskHandling = true;
            }
        private:

            static bool taskTimeout(const Task* task){///when Task takes more than specified time it will be ingnored and taskhandling will go to next round without it.

                if(lowPrioMinTaskTime == 0){
                    return true;
                }
                return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - task->_timeOfStart).count() < lowPrioMinTaskTime);
            }


            void run(){
                /// go through all Tasks and if all Tasks in high priority are executed and no more used by threads(ingnoring mater task)
                for(uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); ++taskIndex){
                    Task* currentTask = lowPriorityTaskQueue[taskIndex];
                    if((currentTask != this) &&  (taskTimeout(currentTask)) && (currentTask->_isAlreadyExecuted == false || currentTask->_isUsedByThread == true)){
                        executeAgain();

                        return;
                    }

                }

                #ifdef DEBUG_INFO
                std::cout<<"Cleaning low prio\n";
                #endif // DEBUG_INFO

                lowPrioCleaning = true; ///temoraly stops Task handling in taskHandlers



                for ( uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); ++taskIndex ){

                    Task* currentTask = lowPriorityTaskQueue[taskIndex];
                    /// clear the Task that are not marked as 'ready to remove'
                    if(currentTask->_taskRemoveMode == NOACTION_MODE){
                        if(currentTask->_isUsedByThread == false || currentTask == this){
                            currentTask->_isAlreadyExecuted = false;
                        }
                    }
                    else{
                        lowPriorityTaskQueue.erase(lowPriorityTaskQueue.begin()+taskIndex);
                        /// when task is marked as 'ready for delete'
                        if(currentTask->_taskRemoveMode == DELETE_MODE){
                            if(currentTask->_isDynamicallyAllocated == true){
                                delete currentTask;
                            }
                            else{
                                std::cerr<<"Warning: attempt to deleted a static or stack allocated object(was prevented).\n";
                            }
                        }
                        taskIndex--;
                    }

                }



                if(lowPrioAddMutex.try_lock()){/// try to lock when noone is adding Task at the moment
                    #ifdef DEBUG_INFO
                    std::cout<<"Added: ("<<(unsigned int)lowPriorityTaskToAdd.size()<<")\n";
                    #endif // DEBUG_INFO
                    /// go through lowPriority 'add' queue and add Task here when no operations with lowPriorityTaskQueue are running
                    for ( uint32_t taskAddIndex = 0; taskAddIndex < lowPriorityTaskToAdd.size(); ++taskAddIndex ){

                        lowPriorityTaskQueue.push_back(lowPriorityTaskToAdd[taskAddIndex]);
                    }
                    lowPriorityTaskToAdd.clear();

                    lowPrioAddMutex.unlock();
                }

                lowPrioCleaning = false;

            }


    } static _lowPrioMasterTask;










    /** Task functions implemetation **/
    Task::Task(){

        _dependentTask          = nullptr;
        _isExecutedByDependency = false;
        _isEnabled              = true;
        _isDynamicallyAllocated = false;
        _taskRemoveMode         = REMOVE_MODE;

    }

    bool Task::removeDependency(const Task* dependency){

        Task* currentTask = this;
        /// go trough dependencies forward-linked-list until dependency is found or _dependentTask of currentTask is equal to nullptr
        while(currentTask->_dependentTask != nullptr){
            ///when dependency is found, remove it from forward-linked-list by setting _dependentTask of currentTask to dependency's _depedentTask.
            if(currentTask->_dependentTask == dependency){
                currentTask->_dependentTask->_isExecutedByDependency = false;
                currentTask->_dependentTask = currentTask->_dependentTask->_dependentTask;
                return true;

            }

            currentTask = currentTask->_dependentTask;
        }

        return false;

    }

    bool Task::deleteDependency(const Task* dependency){
        bool dependencyRemoved = removeDependency(dependency);

        if(dependencyRemoved == true && dependency->_isDynamicallyAllocated == true){
            delete dependency;
        }

        return dependencyRemoved;
    }

    void Task::breakDependency(){

        Task* previus = this;
        Task* current = previus->_dependentTask;
        ///Set all _dependentTask to nullptr in dependecies which are in this chain
        while(current != nullptr){
            previus->_dependentTask = nullptr;
            current->_isExecutedByDependency = false;

            previus = current;
            current = previus->_dependentTask;

        }

    }



    void Task::enable(){
        _isEnabled = true;
    }
    void Task::disable(){
        _isEnabled = false;
    }

    void Task::subscribe(const TaskPriority taskPriority){
        ///If task has been removed or added from it's creation, addition to Task queue will be valid otherwise it's not
        if(_taskRemoveMode == REMOVE_MODE){
            _taskPriority           = taskPriority;
            _isUsedByThread         = false;
            _isAlreadyExecuted      = false;
            _isEnabled              = true;
            _isExecutedByDependency = false;
            _taskRemoveMode         = NOACTION_MODE;



            if(taskPriority == HIGH_PRIO){

                highPrioAddMutex.lock();
                highPriorityTaskToAdd.push_back(this);
                highPrioAddMutex.unlock();
            }
            else {

                lowPrioAddMutex.lock();
                lowPriorityTaskToAdd.push_back(this);
                lowPrioAddMutex.unlock();
            }
        }
    }

    void Task::remove(){
        /// if no remove mode is set, set it to to mode REMOVE_MODE
        if(_taskRemoveMode == NOACTION_MODE){

            _taskRemoveMode = REMOVE_MODE;
        }
        ///set flags for not executing and in case it is used in context of dependencies mark it to be removed from dependency execution
        _isAlreadyExecuted      = true;
        _isExecutedByDependency = false;


    }

    void Task::removeAndDelete(){
        /// if no remove mode is set, set it to to mode DELETE_MODE
        if(_taskRemoveMode == NOACTION_MODE){
            _taskRemoveMode = DELETE_MODE;
        }
        ///set flags for not executing and in case it is used in context of dependencies mark it to be removed and deleted from dependency execution
        _isAlreadyExecuted      = true;
        _isExecutedByDependency = false;
    }

    void Task::executeAgain(){
        _isAlreadyExecuted = false;
    }

    void Task::setAsDynamicallyAllocated(){
        _isDynamicallyAllocated = true;
    }

    Task* Task::getDependentTask() const{
        return _dependentTask;
    }


    bool Task::isEnabled() const{
        return _isEnabled;
    }

    bool Task::isSubscribed() const{
        return (_taskRemoveMode == NOACTION_MODE);
    }

    bool Task::isDependency() const{
        return _isExecutedByDependency;
    }

    bool Task::isDynamicallyAllocated() const{
        return _isDynamicallyAllocated;
    }


    Task::~Task()
    {
        if(_taskRemoveMode != DELETE_MODE && _isDynamicallyAllocated == true){/// if true master Task will handle deletion from main Task queue
            remove(); /// attempt to prevent segmentation fault, if cleaning process happens to be fast and remove this Task from execution queue.
            std::cerr<<"Warning: deletion of Task detected outside of master Task.\n";
        }
    }

    void Task::_recursiveDependencyExecute(Task* task){

        if(task->_dependentTask != nullptr){

            if(task->_dependentTask->_isExecutedByDependency == false){
                    if(this->_dependentTask->_taskRemoveMode == DELETE_MODE){
                        if(this->_dependentTask->_isDynamicallyAllocated == true){
                            delete this->_dependentTask;
                        }
                        else{
                            std::cerr<<"Warning: attempt to deleted a static or stack allocated object(was prevented).\n";
                        }
                    }
                    task->_dependentTask = nullptr;
            }
            else {

                _recursiveDependencyExecute(task->_dependentTask);

            }
        }

        if(task->_isEnabled){
            task->run();
        }

    }

    template <class Locker>
    class AutoUnlocker{
        public:
        AutoUnlocker(Locker& locker) : _locker(locker){}

        Locker* operator -> (){
            return &_locker;
        }

        ~AutoUnlocker(){
            if(_locker.is_locked()){
                _locker.unlock();
            }

        }
        private:
        Locker& _locker;
    };

    bool Task::_execute(){
        ///mutex object that unlocks when function is calling its destructor
        //std::unique_lock<std::mutex> uniqueTaskMutex(_taskMutex,std::defer_lock);
        AutoUnlocker<SpinLock> autoUnlockTaskMutex(_taskMutex);

        if(autoUnlockTaskMutex->try_lock()){
            /// reasons not to execute task
            if(_taskRemoveMode != Task::NOACTION_MODE || ( _taskPriority == HIGH_PRIO ? highPrioCleaning : lowPrioCleaning) || _isUsedByThread ||  _isAlreadyExecuted || _isExecutedByDependency){

                return false;
            }

            _isUsedByThread     = true;
            _isAlreadyExecuted  = true;
            _timeOfStart        = std::chrono::steady_clock::now();

            /// dependency execution will check if _dependentTask is marked to remove or to delete otherwise execute it
            if(this->_dependentTask != nullptr){
                if(this->_dependentTask->_isExecutedByDependency == false){
                    if(this->_dependentTask->_taskRemoveMode == DELETE_MODE){
                        if(this->_dependentTask->_isDynamicallyAllocated == true){
                            delete this->_dependentTask;
                        }
                        else{
                            std::cerr<<"Warning: attempt to deleted a static or stack allocated object(was prevented).\n";
                        }
                    }
                    this->_dependentTask = nullptr;
                }
                else {
                    _recursiveDependencyExecute(this->_dependentTask);
                }
            }
            if(_isEnabled){
                run();
            }

            _isUsedByThread     = false;

            return true;
        }
        return false;
    }



    void setHighPriorityTaskTimeOut(const unsigned int minTaskTime){

        lowPrioMinTaskTime = minTaskTime;
    }

    void setLowPriorityTaskTimeOut(const unsigned int minTaskTime){

        lowPrioMinTaskTime = minTaskTime;
    }


    Task* spawnTaskFunction(TaskFunction taskFunction, Task::TaskPriority taskPriority){
        /// add task function as function pointer inside class and create it as an object
        class functionTask : public Task{

            public:
            functionTask(TaskFunction tFunction){
                this->tFunction = tFunction;
            }

            private:
            void run(){
                tFunction(this);
            }

            TaskFunction tFunction;
        };

        Task* newTask = new functionTask(taskFunction);
        newTask->setAsDynamicallyAllocated();
        newTask->subscribe(taskPriority);
        return newTask;

    }

    void enableTaskHandling(const bool enableTaskHandler){

        quitTaskHandling = !enableTaskHandler;
    }

    void disableTaskHandling(){

        enableTaskHandling(false);
    }

    bool isTaskHandlingEnabled(){
        return !quitTaskHandling;
    }


    bool taskHandler(){
        ///Eexecute all high priority tasks soon as possible
        for(uint32_t taskIndex = highPriorityTaskQueue.size(); highPrioCleaning == false && quitTaskHandling == false && taskIndex > 0; --taskIndex){

            (void)highPriorityTaskQueue[taskIndex-1]->_execute();
        }

        ///Execute all low priority tasks, one per taskHandler function execution
        for(uint32_t taskIndex = lowPriorityTaskQueue.size(); _lowPrioMasterTask._isUsedByThread == false && lowPrioCleaning == false && quitTaskHandling == false && taskIndex > 0; --taskIndex){

            if(lowPriorityTaskQueue[taskIndex-1]->_execute() == true && lowPriorityTaskQueue[taskIndex-1] != &_lowPrioMasterTask){
                break;
            }

        }

        return !quitTaskHandling;
    }



}//namespace TSWorker
