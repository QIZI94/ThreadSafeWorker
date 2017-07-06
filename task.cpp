
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

#include "task.h"




namespace TSWorker{

    struct TaskQueue : std::vector <Task*> {
        void remove(const Task* task){
            for(auto i = 0; i < size(); i++){
                if((*this)[i] == task){

                    erase(begin()+i);
                    break;
                }
            }
        }


        void add(const std::vector <Task*> taskList){

            for(auto i = 0; i < taskList.size(); i++){
                push_back(taskList[i]);


            }
        }

    };


    std::atomic<bool> quitTaskHandling(false);

    /**Low priority handlers**/
    TaskQueue lowPriorityTaskQueue;
    std::vector<Task*> lowPriorityTaskToAdd;
    std::vector<Task*> lowPriorityTaskToRemove;
    std::atomic<bool> lowPrioCleaning(false);
    std::mutex lowPrioAddMutex;
    std::mutex lowPrioRemoveMutex;



    /** Task functions implemetation **/

    template<typename ...Dependency>
    void Task::setDependency(Dependency... dependecies){
        std::array<Task*, sizeof...(Dependency)> deps = {{dependecies ...}};
        //upcomming implementation
    }


    void Task::enable(){
        _isEnabled = true;
    }
    void Task::disable(){
        _isEnabled = false;
    }

    void Task::subscribe(const TaskPriority taskPriority){
        _taskPriority       = taskPriority;
        _isUsedByThread     = false;
        _isAlreadyExecuted  = false;
        _isEnabled          = true;
        _taskRemoveMode     = NOACTION_MODE;

        if(taskPriority == HIGH_PRIO){

        }
        else {

            lowPrioAddMutex.lock();
            lowPriorityTaskToAdd.push_back(this);
            lowPrioAddMutex.unlock();

        }






    }

    void Task::remove(){


        if(_taskPriority == HIGH_PRIO){
            //to be implemented
        }
        else{

            lowPrioRemoveMutex.lock();
            _taskRemoveMode = REMOVE_MODE;
            //_isEnabled = false;

            lowPrioRemoveMutex.unlock();

        }


    }

    void Task::removeAndDelete(){
        if(_taskPriority == HIGH_PRIO){
            //to be implemented
        }
        else{

            lowPrioRemoveMutex.lock();
            _taskRemoveMode = DELETE_MODE;
            //_isEnabled = false;

            lowPrioRemoveMutex.unlock();

        }

    }

    bool Task::isEnabled() const{
        return _isEnabled;
    }

    Task::~Task()
    {
        /*if(_isGoingToBeDeleted == false){/// if true master task will handle deletion from main task queue
            remove();
        }*/
    }





    bool Task::_execute(){
        std::unique_lock<std::mutex> uniqueTaskMutex(_taskMutex,std::defer_lock);

        if(uniqueTaskMutex.try_lock()){
            /// reasons not to execute
            if(_taskRemoveMode != Task::NOACTION_MODE || lowPrioCleaning || _isUsedByThread ||  _isAlreadyExecuted ||    !_isEnabled || _isExecutedByDependency){

                return false;
            }

            _isUsedByThread = true;
            _isAlreadyExecuted = true;
          //  timeOfStart = std::chrono::steady_clock::now();
            /** if(dependentTask != nullptr){//disabled for now
                dependentTask->execute();
            }*/


            run();
            _isUsedByThread = false;



            return true;
        }
        return false;
    }

    bool taskHandler(){
        for(int i = lowPriorityTaskQueue.size()-1; lowPrioCleaning == false && quitTaskHandling == false && i >= 0; i--){
            if(lowPriorityTaskQueue[i]->_execute() == true){
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

            for(int i = 0; i < lowPriorityTaskQueue.size(); i++){

                if(lowPriorityTaskQueue[i]!= this &&  (true) && (lowPriorityTaskQueue[i]->_isAlreadyExecuted == false || lowPriorityTaskQueue[i]->_isUsedByThread == true)){
                    _isAlreadyExecuted = false;

                    return;
                }

            }


            std::cout<<"Cleaning low prio\n";

            lowPrioCleaning = true;

/*
            if(lowPrioRemoveMutex.try_lock()){
                std::cout<<"To remove: ("<<(unsigned int)lowPriorityTaskToRemove.size()<<")\n";
                for ( auto i = 0;   i < lowPriorityTaskToRemove.size(); i++ ){

                    lowPriorityTaskQueue.remove(lowPriorityTaskToRemove[i]);

                    if(lowPriorityTaskToRemove[i]->_taskRemoveMode == DELETE_MODE){

                       delete lowPriorityTaskToRemove[i];
                    }
                }
                lowPriorityTaskToRemove.clear();
                lowPrioRemoveMutex.unlock();
            }*/
            if(lowPrioRemoveMutex.try_lock()){
                for ( auto i = 0; i < lowPriorityTaskQueue.size(); i++ ){
                    Task* currentTask = lowPriorityTaskQueue[i];
                    if(currentTask->_taskRemoveMode == NOACTION_MODE){
                        currentTask->_isAlreadyExecuted = false;
                    }
                    else{
                        lowPriorityTaskQueue.erase(lowPriorityTaskQueue.begin()+i);
                        if(currentTask->_taskRemoveMode == DELETE_MODE){
                            delete currentTask;
                        }
                    }

                }
                lowPrioRemoveMutex.unlock();

            }



            if(lowPrioAddMutex.try_lock()){
                std::cout<<"Added: ("<<(unsigned int)lowPriorityTaskToAdd.size()<<")\n";
                for ( auto i = 0; i < lowPriorityTaskToAdd.size(); i++ ){

                    lowPriorityTaskQueue.push_back(lowPriorityTaskToAdd[i]);
                }
                lowPriorityTaskToAdd.clear();

                lowPrioAddMutex.unlock();
            }

            lowPrioCleaning = false;

        }


    } _lowPrioMasterTask;

}
