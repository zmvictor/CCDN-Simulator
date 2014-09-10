#include "ns3/nstime.h"
#include "ns3/log.h"

#include "global-content-manager.h"

#include "task-recorder.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("TaskRecorder");

NS_OBJECT_ENSURE_REGISTERED (TaskRecorder);

TaskRecorder::TaskRecorder(char* filename)
{
    m_filename = filename;
    m_list = new std::list<Task*>(0);

    std::ofstream file;
    file.open(filename);
    //file<<"Simulation result"<<std::endl;
    file.flush();
    file.close();

    //Simulator::Schedule(Seconds(0.0), &ns3::TaskRecorder::ReviewTask, this);
}


void
TaskRecorder::RegisterTask(unsigned local, uint64_t content)
{
    Task *task = new Task;
    task->m_local = local;
    task->m_content = content;
    task->m_starttime = Simulator::Now().GetSeconds();
    task->m_lastreloadtime = task->m_starttime;
    task->m_state = 0;

    task->m_remote = 0;
    task->m_finishtime = 0;

    m_list->push_back(task);
}


void
TaskRecorder::UpdateTask(unsigned local, unsigned remote, uint64_t content)
{
    Task *task = GetTask(local, content);
    if (task == 0)
    {
        return;
    }

    task->m_state = 1;
    task->m_remote = remote;
}

void
TaskRecorder::FinishTask(unsigned local, uint64_t content)
{
    Task *task = GetTask(local, content);
    if (task == 0)
    {
        return;
    }

    task = RemoveTask(local, content);
    task->m_finishtime = Simulator::Now().GetSeconds();
    WriteTask(task);
    delete task;
}

Task*
TaskRecorder::GetTask(unsigned local, uint64_t content)
{
    for(std::list<Task*>::iterator iter = m_list->begin(); iter != m_list->end(); iter ++)
    {
        Task *task = *iter;
        if (task->m_local == local && task->m_content == content)
        {
            return task;
        }
    }
    return 0;
}

Task*
TaskRecorder::RemoveTask(unsigned local, uint64_t content)
{
    for(std::list<Task*>::iterator iter = m_list->begin(); iter != m_list->end(); iter ++)
    {
        Task *task = *iter;
        if (task->m_local == local && task->m_content == content)
        {
            m_list->erase(iter);
            return task;
        }
    }
    return 0;
}

void
TaskRecorder::WriteTask(Task *task)
{
    std::ofstream file;
    file.open(m_filename, std::ios::app);
    file<<task->m_content<<" from "<<task->m_remote<<" to "<<task->m_local<<" start "<<task->m_starttime<<" end "<<task->m_finishtime<<std::endl;
    file.flush();
    NS_LOG_LOGIC("Task finished and written");
    file.close();
}

void
TaskRecorder::ReviewTask()
{
    for(std::list<Task*>::iterator iter = m_list->begin(); iter != m_list->end(); iter ++)
    {
        Task *task = *iter;
        if (task->m_state == 0)
        {
            double now = Simulator::Now().GetSeconds();
            if (now >= task->m_lastreloadtime + m_timeout)
            {
                task->m_lastreloadtime = now;
                ReloadRequire(task->m_local, task->m_content);
            }
        }
    }
}

};
