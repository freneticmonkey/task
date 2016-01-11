//
//  JobSystem.cpp
//  task
//
//  Created by Porter, Scott on 11/01/2016.
//
//

#include <iostream>

#include "JobSystem.h"


namespace JobSystem
{
    
    int g_workerThreadCount;
    bool g_workerThreadActive;
    bool g_workerCleanJobs;
    
    std::vector<WorkStealingQueue*> g_jobQueues;
    std::vector<std::thread> g_threads;
    
    std::atomic<unsigned int> g_poolMissCount;
    std::atomic<unsigned int> g_jobFinishCount;
    unsigned int g_createdJobs;
    
    std::vector<unsigned int> g_threadJobsFinished;
    
    JobProcThread * mainThreadJobProc;
    
    JobProcThread::JobProcThread(int threadId)
    {
        g_allocatedJobs = 0;
        
        g_rand = std::uniform_int_distribution<int>(0, g_workerThreadCount - 1);
        m_threadId = threadId;
    }
    
    void JobProcThread::WorkerProcessJobs()
    {
        std::cout << "Processing Jobs on Thread: " << m_threadId << " id: " << std::this_thread::get_id() << std::endl;
        
        while ( g_workerThreadActive )
        {
            ProcessJobs();
        }
    }
    
    void JobProcThread::ProcessJobs()
    {
        Job* job = GetJob();
        if ( job )
        {
            Execute(job);
        }
        
        if (g_workerCleanJobs)
        {
            CleanJobs();
        }
    }
    
    bool JobProcThread::HasJobCompleted(const Job* job)
    {
        return job->unfinishedJobs < 1;
    }
    
    void JobProcThread::CleanJobs()
    {
        for (auto& job : g_jobAllocator)
        {
            job.unfinishedJobs = 0;
        }
    }
    
    Job* JobProcThread::AllocateJob()
    {
        unsigned int index = g_allocatedJobs++;
        Job* retJob = &g_jobAllocator[(index - 1u) & (MAX_JOB_COUNT - 1u)];
        
        // If the job hasn't finished, search for one that has.
        while (!HasJobCompleted(retJob) && ( g_allocatedJobs < (index+ MAX_JOB_COUNT) ) )
        {
            g_allocatedJobs++;
            retJob = &g_jobAllocator[(g_allocatedJobs - 1u) & (MAX_JOB_COUNT - 1u)];
        }
        
        if (retJob->unfinishedJobs != 0)
        {
            retJob = nullptr;
            g_poolMissCount++;
        }
        
        return retJob;
    }
    
    WorkStealingQueue* JobProcThread::GetWorkerThreadQueue()
    {
        return g_jobQueues[m_threadId];
    }
    
    bool JobProcThread::IsEmptyJob(Job* job)
    {
        return (job == nullptr);
    }
    
    Job * JobProcThread::GetJob(void)
    {
        WorkStealingQueue* queue = GetWorkerThreadQueue();
        
        Job* job = queue->Pop();
        if ( IsEmptyJob(job) )
        {
            unsigned int randomIndex = g_rand(g_rd);
            WorkStealingQueue* stealQueue = g_jobQueues[randomIndex];
            if ( stealQueue == queue)
            {
                // Can't steal from ourselves
                std::this_thread::yield();
                return nullptr;
            }
            
            Job* stolenJob = stealQueue->Steal();
            if ( IsEmptyJob(stolenJob) )
            {
                // Can't steal a job from another thread
                std::this_thread::yield();
                return nullptr;
            }
            
            return stolenJob;
        }
        
        return job;
    }
    
    void JobProcThread::Execute(Job* job)
    {
        job->function(job);
        Finish(job);
    }
    
    void JobProcThread::Finish(Job* job)
    {
        // atomic decrement storing the result locally so that the value is consistent for the entire function
        const unsigned int unfinishedJobs = --job->unfinishedJobs;
        
        if  ( ( unfinishedJobs == 0 ) && (job->parent) )
        {
            if ( job->parent )
            {
                Finish(job->parent);
            }
            g_jobFinishCount++;
            g_threadJobsFinished[m_threadId]++;
        }
    }
    
    Job* JobProcThread::CreateJob(JobFunction function)
    {
        Job* job = AllocateJob();
        
        if (job)
        {
            job->function = function;
            job->parent = nullptr;
            job->unfinishedJobs = 1;
            g_createdJobs++;
        }
        
        return job;
    }
    
    Job* JobProcThread::CreateJobAsChild(Job *parent, JobFunction function)
    {
        std::atomic_fetch_add_explicit(&parent->unfinishedJobs, 1, std::memory_order_relaxed);
        
        Job* job = AllocateJob();
        
        if (job)
        {
            job->function = function;
            job->parent = parent;
            job->unfinishedJobs = 1;
            g_createdJobs++;
        }
        else
        {
            // If the job creation failed, decrement the parents dependencies
            std::atomic_fetch_sub_explicit(&parent->unfinishedJobs, 1, std::memory_order_relaxed);
        }
        
        return job;
    }
    
    void JobProcThread::Run(Job* job)
    {
        if (job)
        {
            WorkStealingQueue* queue = GetWorkerThreadQueue();
            queue->Push(job);
        }
    }
    
    void JobProcThread::Wait(const Job* job)
    {
        std::cout << "Processing Jobs on Thread: " << m_threadId << " id: " << std::this_thread::get_id() << std::endl;
        
        // wait until completed
        while (!HasJobCompleted(job))
        {
            ProcessJobs();
        }
    }
    
    void tmain(int tId)
    {
        JobProcThread jobProc(tId);
        
        jobProc.WorkerProcessJobs();
    }
    
    Job* CreateJob(JobFunction function)
    {
        return mainThreadJobProc->CreateJob(function);
    }
    
    Job* CreateJobAsChild(Job *parent, JobFunction function)
    {
        return mainThreadJobProc->CreateJobAsChild(parent, function);
    }
    
    void Run(Job *job)
    {
        mainThreadJobProc->Run(job);
    }
    
    void Wait(const Job* job)
    {
        mainThreadJobProc->Wait(job);
    }
    
    void Setup()
    {
        g_workerThreadCount = std::thread::hardware_concurrency();
        
        // This doesn't seem right.
        for (auto i = 0; i < g_workerThreadCount; i++)
        {
            g_jobQueues.push_back(new WorkStealingQueue());
        }
        
        for (auto i = 1; i < g_workerThreadCount; i++)
        {
            g_threads.push_back(std::thread(std::bind(tmain, i)));
        }
        
        for (auto i = 0; i < g_workerThreadCount; i++)
        {
            g_threadJobsFinished.push_back(0);
        }
        
        mainThreadJobProc = new JobProcThread();
        
        g_workerThreadActive = true;
    }
    
    
    
    void StartFrame()
    {
        g_poolMissCount = 0u;
        g_jobFinishCount = 0u;
        
        g_createdJobs = 0u;
        g_workerCleanJobs = false;
    }
    
    void EndFrame()
    {
        std::cout << "Pool allocation misses: " << g_poolMissCount << std::endl;
        std::cout << "Total Jobs created: " << g_createdJobs << std::endl;
        std::cout << "Total Jobs finished: " << g_jobFinishCount << std::endl;
        
        for (auto i = 0; i < g_workerThreadCount; i++)
        {
            std::cout << "Thread " << i << " processed " << g_threadJobsFinished[i] << " jobs. " << std::endl;
            g_threadJobsFinished[i] = 0;
        }
        
        g_workerCleanJobs = true;
        
        mainThreadJobProc->CleanJobs();
        
        std::cout << std::endl;
    }
    
    void Shutdown()
    {
        g_workerThreadActive = false;
        
        for (auto& thread : g_threads)
        {
            thread.join();
        }
        
        // This doesn't seem right.
        for ( auto workQueue : g_jobQueues)
        {
            delete workQueue;
        }
        
    }
}
