#include <mutex>

#include <vector>
#include <array>

#include <iostream>

#include "task.h"




namespace TSWorker{

    struct TaskQueue : std::vector <Task*> {
        void remove(const Task* task){
            //for ( auto trit = begin(); empty() == false && trit != end(); ++trit ){
            for(int i = 0; i < size(); i++){
                //if(((Task*)*trit) == task){
                    if((*this)[i] == task){
                    //erase(trit);
                    erase(begin()+i);
                    break;
                }
            }
        }


        void add(const std::vector <Task*> taskList){
            for ( auto trit = taskList.begin(); taskList.empty() == false && trit != taskList.end(); ++trit ){
            //for(auto i = 0; i < taskList.size(); i++){
                //push_back(taskList[i]);
                push_back(((Task*)*trit));

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
    Task::Task(const TaskPriority taskPriority)
    {
        assign(taskPriority);
    }

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

    void Task::assign(const TaskPriority taskPriority){
        _isHighPriority = (bool)taskPriority;


        if(taskPriority == HIGH_PRIO){

        }
        else {

            while(lowPrioCleaning == true);
            lowPrioAddMutex.lock();
            lowPriorityTaskToAdd.push_back(this);
            //printf("low prio added: size:%d  tid:%d \n",lowPriorityTaskToAdd.size(),std::this_thread::get_id());
            lowPrioAddMutex.unlock();

        }




        _isUsedByThread = false;
        _isMainThreaded = false;
        _isAlreadyExecuted = false;
        _isEnabled  = true;
        _isGoingToBeDeleted = false;
        _isExecutedByDependency = false;
        _isGoingToBeRemoved = false;
    }

    void Task::remove(){
       // _isEnabled = false;

        if(_isHighPriority){
            //to be implemented
        }
        else{

            while(lowPrioCleaning == true);
            lowPrioRemoveMutex.lock();
            _isGoingToBeRemoved = true;
            lowPriorityTaskToRemove.push_back(this);
            lowPrioRemoveMutex.unlock();

        }


    }

    void Task::removeAndDelete(){
        remove();
        _isGoingToBeDeleted = true;
    }

    bool Task::isEnabled() const{
        return _isEnabled;
    }

    Task::~Task()
    {
        if(_isGoingToBeDeleted == false){/// if true master task will handle deletion from main task queue
            remove();
        }
    }


    bool Task::_execute(){
        /// reasons not to execute
        if(!_isEnabled || _isUsedByThread || _isAlreadyExecuted || _isExecutedByDependency || lowPrioCleaning){
            return false;
        }

        _isUsedByThread = true;
      //  timeOfStart = std::chrono::steady_clock::now();
        /** if(dependentTask != nullptr){//disabled for now
            dependentTask->execute();
        }*/

        _isAlreadyExecuted = true;
        run();
        _isUsedByThread = false;

        return true;
    }

    bool taskHandler(){
        for(int i = lowPriorityTaskQueue.size()-1; quitTaskHandling == false && lowPrioCleaning == false && i >= 0; i--){
            if(lowPriorityTaskQueue[i]->_execute() == true){
                break;
            }

        }

        return !quitTaskHandling;
    }








    /**Low priority master task**/


    class LowPriotityMasterTask : public Task{
    public:
        LowPriotityMasterTask() : Task(LOW_PRIO){

            lowPriorityTaskQueue.push_back(this);
            lowPriorityTaskToAdd.clear();
        }
        ~LowPriotityMasterTask(){
            quitTaskHandling = true;
        }
    private:
        void run(){

            for(int i = 0; i < lowPriorityTaskQueue.size(); i++){

                if(lowPriorityTaskQueue[i]!= this &&  (true) && (lowPriorityTaskQueue[i]->_isAlreadyExecuted == false || lowPriorityTaskQueue[i]->_isUsedByThread == true)){
                    _isAlreadyExecuted.store(false);

                    return;
                }

            }


            std::cout<<"Cleaning low prio\n";

            lowPrioCleaning = true;


            if(lowPrioRemoveMutex.try_lock()){
                for ( auto i = 0;   i < lowPriorityTaskToRemove.size(); i++ ){

                    lowPriorityTaskQueue.remove(lowPriorityTaskToRemove[i]);
                    if(_isGoingToBeDeleted == true){
                        delete lowPriorityTaskToRemove[i];
                    }
                }
                lowPriorityTaskToRemove.clear();
                lowPrioRemoveMutex.unlock();
            }

            for ( auto i = 0; i < lowPriorityTaskQueue.size(); i++ ){
                if(lowPriorityTaskQueue[i]->_isGoingToBeRemoved || lowPriorityTaskQueue[i]->_isGoingToBeDeleted);

                else{
                    lowPriorityTaskQueue[i]->_isAlreadyExecuted = false;
                }
            }




            if(lowPrioAddMutex.try_lock()){
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
