#include "task.h"


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
   // std::cout<<"Sizeof: "<<sizeof(TSWorker::task_ptr)<<'\n';
    getchar();
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
            std::cout<<"I am dinamically allocated task\n";

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

/*


    for(;;){
        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - timeOfStart).count() > 10000){
   
            TSWorker::spawnTaskFunction(taskFn, TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTaskFunction(TASK_LAMBDA(){std::cout<<"This is lambda function&&&&&&&&&&&&&&&&&&&&&&&\n"; thisTask->removeAndDelete();}
                                        ,TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::HIGH_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::LOW_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::LOW_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::LOW_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::LOW_PRIO);
            TSWorker::spawnTask<test2>(TSWorker::Task::LOW_PRIO);

            TSWorker::spawnTask<test3>(TSWorker::Task::HIGH_PRIO,5);
           // (new test2)->subscribe(TSWorker::Task::LOW_PRIO);
            //t->subscribe(TSWorker::Task::LOW_PRIO);
            timeOfStart = std::chrono::steady_clock::now();
        }

        TSWorker::taskHandler();


    }
}
*/