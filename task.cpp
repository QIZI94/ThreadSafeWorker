
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

#include <iostream>
#include <inttypes.h>

#include "task.h"



namespace TSWorker{


    std::atomic<bool> quitTaskHandling(false);


    /**High priority handlers**/
    std::vector<Task*> highPriorityTaskQueue;
    std::vector<Task*> highPriorityTaskToAdd;
    std::vector<Task*> highPriorityTaskToRemove;
    std::atomic<bool> highPrioCleaning(false);
    unsigned int highPrioMinTaskTime = 1000;
    std::mutex highPrioAddMutex;
    std::mutex highPrioRemoveMutex;


    /**Low priority handlers**/
    std::vector<Task*> lowPriorityTaskQueue;
    std::vector<Task*> lowPriorityTaskToAdd;
    std::vector<Task*> lowPriorityTaskToRemove;
    std::atomic<bool> lowPrioCleaning(false);
    unsigned int lowPrioMinTaskTime = 1000;
    std::mutex lowPrioAddMutex;
    std::mutex lowPrioRemoveMutex;



    /** Task functions implemetation **/
    Task::Task(){

        _dependentTask          = nullptr;
        _isExecutedByDependency = false;
        _isEnabled              = true;
        _taskRemoveMode         = REMOVE_MODE;

    }

    void Task::removeDependency(const Task* dependency){

        Task* currentTask = this;

        while(currentTask->_dependentTask != nullptr){

            if(currentTask->_dependentTask == dependency){
                currentTask->_dependentTask->_isExecutedByDependency = false;
                currentTask->_dependentTask = currentTask->_dependentTask->_dependentTask;
                break;

            }


            currentTask = currentTask->_dependentTask;
        }

    }
    void Task::breakDependency(){

        Task* previus = this;
        Task* current = previus->_dependentTask;
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

        if(_taskRemoveMode == NOACTION_MODE){
            if(_taskPriority == HIGH_PRIO){
                //highPrioRemoveMutex.lock();
                _taskRemoveMode = REMOVE_MODE;
                //highPrioRemoveMutex.unlock();
            }
            else{
                //lowPrioRemoveMutex.lock();
                _taskRemoveMode = REMOVE_MODE;
                //lowPrioRemoveMutex.unlock();
            }

        }
        _isAlreadyExecuted      = true;
        _isExecutedByDependency = false;


    }

    void Task::removeAndDelete(){
        if(_taskRemoveMode == NOACTION_MODE){
            if(_taskPriority == HIGH_PRIO){
                //highPrioRemoveMutex.lock();
                _taskRemoveMode = DELETE_MODE;
                //highPrioRemoveMutex.unlock();
            }
            else{

                //lowPrioRemoveMutex.lock();
                _taskRemoveMode = DELETE_MODE;
                //lowPrioRemoveMutex.unlock();

            }
        }

    }

    void Task::executeAgain(){
        _isAlreadyExecuted = false;
    }


    bool Task::isEnabled() const{
        return _isEnabled;
    }

    bool Task::isSubscribed() const{
        return (_taskRemoveMode == NOACTION_MODE);
    }

    Task* Task::getDependentTask() const{
        return _dependentTask;
    }


    Task::~Task()
    {
        if(_taskRemoveMode != DELETE_MODE){/// if true master task will handle deletion from main task queue
            remove(); /// attempt to prevent segmentation falut, if cleaning process happens to be fast and remove this task from execution queue.
            std::cerr<<"Warning: deletion of task detected outside of master task\n.";
        }
    }

    bool Task::_recursiveDependencyExecute(Task* task){

        if(task->_dependentTask != nullptr){

            if(task->_dependentTask->_isExecutedByDependency == false){
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



    bool Task::_execute(){
        std::unique_lock<std::mutex> uniqueTaskMutex(_taskMutex,std::defer_lock);

        if(uniqueTaskMutex.try_lock()){
            /// reasons not to execute
            if(_taskRemoveMode != Task::NOACTION_MODE || ( _taskPriority == HIGH_PRIO ? highPrioCleaning : lowPrioCleaning) || _isUsedByThread ||  _isAlreadyExecuted || !_isEnabled || _isExecutedByDependency){

                return false;
            }

            _isUsedByThread     = true;
            _isAlreadyExecuted  = true;
            _timeOfStart        = std::chrono::steady_clock::now();


            if(this->_dependentTask != nullptr){
                if(this->_dependentTask->_isExecutedByDependency == false){

                    this->_dependentTask = nullptr;
                }
                else {
                    _recursiveDependencyExecute(this->_dependentTask);
                }
            }

            run();

            _isUsedByThread     = false;

            return true;
        }
        return false;
    }

    void setHighPriorityTaskTimeOut(unsigned int minTaskTime){ lowPrioMinTaskTime = minTaskTime;}
    void setLowPriorityTaskTimeOut(unsigned int minTaskTime){ lowPrioMinTaskTime = minTaskTime;}






     /**High priority master task**/


    class HighPriotityMasterTask : public Task{
        public:
            HighPriotityMasterTask() {
                _taskPriority = HIGH_PRIO;
                _isUsedByThread = false;
                _isAlreadyExecuted = false;
                _isEnabled  = true;
                _taskRemoveMode = NOACTION_MODE;
                highPriorityTaskQueue.push_back(this);

            }
            ~HighPriotityMasterTask(){
                quitTaskHandling = true;
            }
        private:

            static bool taskTimeout(const Task* task){
                if(highPrioMinTaskTime == 0){
                    return true;
                }
                return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - task->_timeOfStart).count() < highPrioMinTaskTime);
            }


            void run(){

                for(uint32_t taskIndex = 0; taskIndex < highPriorityTaskQueue.size(); ++taskIndex){
                    Task* currentTask = highPriorityTaskQueue[taskIndex];
                    if((currentTask != this) &&  (taskTimeout(currentTask)) && (currentTask->_isAlreadyExecuted == false || currentTask->_isUsedByThread == true)){
                        executeAgain();

                        return;
                    }

                }


                std::cout<<"Cleaning High prio\n";

                highPrioCleaning = true;



                    for ( uint32_t taskIndex = 0; taskIndex < highPriorityTaskQueue.size(); ++taskIndex ){

                        Task* currentTask = highPriorityTaskQueue[taskIndex];

                        if(currentTask->_taskRemoveMode == NOACTION_MODE){
                            if(currentTask->_isUsedByThread == false || currentTask == this){
                                currentTask->_isAlreadyExecuted = false;
                            }
                        }
                        else{
                            highPriorityTaskQueue.erase(highPriorityTaskQueue.begin()+taskIndex);
                            if(currentTask->_taskRemoveMode == DELETE_MODE){
                                delete currentTask;
                            }
                            taskIndex--;
                        }

                    }





                if(highPrioAddMutex.try_lock()){
                    std::cout<<"Added High: ("<<(unsigned int)highPriorityTaskToAdd.size()<<")\n";
                    for ( uint32_t taskAddIndex = 0; taskAddIndex < highPriorityTaskToAdd.size(); ++taskAddIndex ){

                        highPriorityTaskQueue.push_back(highPriorityTaskToAdd[taskAddIndex]);
                    }
                    highPriorityTaskToAdd.clear();

                    highPrioAddMutex.unlock();
                }

                highPrioCleaning = false;

            }


    } _highPrioMasterTask;





    /**Low priority master task**/


    class LowPriotityMasterTask : public Task{
        public:
            LowPriotityMasterTask() {
                _taskPriority = LOW_PRIO;
                _isUsedByThread = false;
                _isAlreadyExecuted = false;
                _isEnabled  = true;
                _taskRemoveMode = NOACTION_MODE;
                lowPriorityTaskQueue.push_back(this);

            }
            ~LowPriotityMasterTask(){
                quitTaskHandling = true;
            }
        private:

            static bool taskTimeout(const Task* task){
                if(lowPrioMinTaskTime == 0){
                    return true;
                }
                return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - task->_timeOfStart).count() < lowPrioMinTaskTime);
            }


            void run(){

                for(uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); ++taskIndex){
                    Task* currentTask = lowPriorityTaskQueue[taskIndex];
                    if((currentTask != this) &&  (taskTimeout(currentTask)) && (currentTask->_isAlreadyExecuted == false || currentTask->_isUsedByThread == true)){
                        executeAgain();

                        return;
                    }

                }


                std::cout<<"Cleaning low prio\n";

                lowPrioCleaning = true;


                //if(lowPrioRemoveMutex.try_lock()){
                for ( uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); ++taskIndex ){

                    Task* currentTask = lowPriorityTaskQueue[taskIndex];

                    if(currentTask->_taskRemoveMode == NOACTION_MODE){
                        if(currentTask->_isUsedByThread == false || currentTask == this){
                            currentTask->_isAlreadyExecuted = false;
                        }
                    }
                    else{
                        lowPriorityTaskQueue.erase(lowPriorityTaskQueue.begin()+taskIndex);
                        if(currentTask->_taskRemoveMode == DELETE_MODE){
                            delete currentTask;
                        }
                        taskIndex--;
                    }

                }



                if(lowPrioAddMutex.try_lock()){
                    std::cout<<"Added: ("<<(unsigned int)lowPriorityTaskToAdd.size()<<")\n";
                    for ( uint32_t taskAddIndex = 0; taskAddIndex < lowPriorityTaskToAdd.size(); ++taskAddIndex ){

                        lowPriorityTaskQueue.push_back(lowPriorityTaskToAdd[taskAddIndex]);
                    }
                    lowPriorityTaskToAdd.clear();

                    lowPrioAddMutex.unlock();
                }

                lowPrioCleaning = false;

            }


    } _lowPrioMasterTask;


    bool taskHandler(){
        for(uint32_t taskIndex = highPriorityTaskQueue.size(); highPrioCleaning == false && quitTaskHandling == false && taskIndex > 0; --taskIndex){
            (void)highPriorityTaskQueue[taskIndex-1]->_execute();


        }

        for(uint32_t taskIndex = lowPriorityTaskQueue.size(); _lowPrioMasterTask._isUsedByThread == false && lowPrioCleaning == false && quitTaskHandling == false && taskIndex > 0; --taskIndex){
            if(lowPriorityTaskQueue[taskIndex-1]->_execute() == true && lowPriorityTaskQueue[taskIndex-1] != &_lowPrioMasterTask){
                break;
            }

        }

        return !quitTaskHandling;
    }


}
