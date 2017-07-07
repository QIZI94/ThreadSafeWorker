
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
    std::thread::id __TF_DefaultDhreadID;

    std::atomic<bool> quitTaskHandling(false);

    /**Low priority handlers**/
    std::vector<Task*> lowPriorityTaskQueue;
    std::vector<Task*> lowPriorityTaskToAdd;
    std::vector<Task*> lowPriorityTaskToRemove;
    std::atomic<bool> lowPrioCleaning(false);
    std::mutex lowPrioAddMutex;
    std::mutex lowPrioRemoveMutex;



    /** Task functions implemetation **/
    Task::Task(){

        _dependentTask = nullptr;
        _taskRemoveMode = REMOVE_MODE;
        _bindedThread = __TF_DefaultDhreadID;
    }


    void Task::bindToThread(std::thread::id threadID){
        _bindedThread = threadID;
    }
    void Task::unbind(){
        _bindedThread = __TF_DefaultDhreadID;
    }


    void Task::removeFromChain(){

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
                //to be implemented
            }
            else{

                lowPrioRemoveMutex.lock();
                _taskRemoveMode = REMOVE_MODE;
                lowPrioRemoveMutex.unlock();

            }
        }


    }

    void Task::removeAndDelete(){
        if(_taskRemoveMode == NOACTION_MODE){
            if(_taskPriority == HIGH_PRIO){
                //to be implemented
            }
            else{

                lowPrioRemoveMutex.lock();
                _taskRemoveMode = DELETE_MODE;
                lowPrioRemoveMutex.unlock();

            }
        }

    }

    bool Task::isEnabled() const{
        return _isEnabled;
    }

    Task::~Task()
    {
        if(_taskRemoveMode != DELETE_MODE){/// if true master task will handle deletion from main task queue
            std::cerr<<"Warning: deletion of task detected outside of master task\n.";
        }
    }





    bool Task::_execute(){
        std::unique_lock<std::mutex> uniqueTaskMutex(_taskMutex,std::defer_lock);

        if(uniqueTaskMutex.try_lock()){
            /// reasons not to execute
            if(_taskRemoveMode != Task::NOACTION_MODE || lowPrioCleaning || _isUsedByThread ||  _isAlreadyExecuted || !_isEnabled || _isExecutedByDependency){

                return false;
            }

            _isUsedByThread = true;
            _isAlreadyExecuted = true;
          //  timeOfStart = std::chrono::steady_clock::now();
            if(_dependentTask != nullptr){//disabled for now
                if(_dependentTask->_isExecutedByDependency == false){
                    _dependentTask = nullptr;

                }
                else{

                    _dependentTask->run();
                }
            }


            run();
            _isUsedByThread = false;



            return true;
        }
        return false;
    }





    bool taskHandler(){
        for(uint32_t taskIndex = lowPriorityTaskQueue.size(); lowPrioCleaning == false && quitTaskHandling == false && taskIndex > 0; taskIndex--){
            if(lowPriorityTaskQueue[taskIndex-1]->_execute() == true){
                break;
            }

        }

        return !quitTaskHandling;
    }








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
            void run(){

                for(uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); taskIndex++){
                    Task* currentTask = lowPriorityTaskQueue[taskIndex];
                    if(currentTask != this &&  (true) && (currentTask->_isAlreadyExecuted == false || currentTask->_isUsedByThread == true)){
                        _isAlreadyExecuted = false;

                        return;
                    }

                }


                std::cout<<"Cleaning low prio\n";

                lowPrioCleaning = true;


                if(lowPrioRemoveMutex.try_lock()){
                    for ( uint32_t taskIndex = 0; taskIndex < lowPriorityTaskQueue.size(); taskIndex++ ){
                        Task* currentTask = lowPriorityTaskQueue[taskIndex];
                        if(currentTask->_taskRemoveMode == NOACTION_MODE){
                            currentTask->_isAlreadyExecuted = false;
                        }
                        else{
                            lowPriorityTaskQueue.erase(lowPriorityTaskQueue.begin()+taskIndex);
                            if(currentTask->_taskRemoveMode == DELETE_MODE){
                                delete currentTask;
                            }
                            taskIndex--;
                        }

                    }
                    lowPrioRemoveMutex.unlock();

                }


                if(lowPrioAddMutex.try_lock()){
                    std::cout<<"Added: ("<<(unsigned int)lowPriorityTaskToAdd.size()<<")\n";
                    for ( uint32_t taskAddIndex = 0; taskAddIndex < lowPriorityTaskToAdd.size(); taskAddIndex++ ){

                        lowPriorityTaskQueue.push_back(lowPriorityTaskToAdd[taskAddIndex]);
                    }
                    lowPriorityTaskToAdd.clear();

                    lowPrioAddMutex.unlock();
                }

                lowPrioCleaning = false;

            }


    } _lowPrioMasterTask;

}
