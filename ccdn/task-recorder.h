#ifndef TASK_RECORDER_H
#define TASK_RECORDER_H

#include <inttypes.h>
#include <list>
#include <fstream>
#include "ns3/object-factory.h"

#include "global-content-manager.h"

namespace ns3
{

struct Task
{
    unsigned m_local;
    unsigned m_remote;
    uint64_t m_content;
    uint8_t m_state;
    double m_lastreloadtime;
    double m_starttime;
    double m_finishtime;
};

class TaskRecorder : public Object
{

public:

    static TypeId GetTypeId (void) {return TypeId ("ns3::TaskRecorder");};
    TaskRecorder(char* filename);
    ~TaskRecorder() {};


    // Start up a new task, add it into the list and set is as unhandled.
    void RegisterTask(unsigned local, uint64_t content);
    // You'll do this when you received an reply. It means you task will begin in no time.
    void UpdateTask(unsigned local, unsigned remote, uint64_t content);
    // You'll do this when you received an finish. The task will be removed after then.
    void FinishTask(unsigned local, uint64_t content);
    // Get the task. If none, return 0.
    Task* GetTask(unsigned local, uint64_t content);
    Task* RemoveTask(unsigned local, uint64_t content);
    void WriteTask(Task *task);

    // Review all tasks in the list. For each task, if the task is not responced for a long time, then you may reactivate it.
    void ReviewTask();


private:

	std::list<Task*> *m_list;
	void (*ReloadRequire)(unsigned, uint64_t);
	char* m_filename;

	static const double m_timeout = 0.5;
};
};


#endif
