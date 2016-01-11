//
//  JobSystem.h
//  task
//
//  Created by Porter, Scott on 2/01/2016.
//

#pragma once

#include <thread>
#include <vector>
#include <atomic>

#include <random>

#include "Job.h"
#include "WorkStealingQueue.h"


namespace JobSystem
{
    const int MAX_JOB_COUNT = 4096;

    Job* CreateJob(JobFunction function);
    Job* CreateJobAsChild(Job *parent, JobFunction function);
    
    void Run(Job *job);
    void Wait(const Job* job);
    
    void Setup();
    void StartFrame();
    void EndFrame();
    void Shutdown();
    
    class JobProcThread
    {
    public:
        JobProcThread(int threadId = 0);
        
        void WorkerProcessJobs();

        void ProcessJobs();
        
        bool HasJobCompleted(const Job* job);

        void CleanJobs();
        
        Job* AllocateJob();
        
        WorkStealingQueue* GetWorkerThreadQueue();
        
        bool IsEmptyJob(Job* job);
        
        Job * GetJob(void);
        
        void Execute(Job* job);
        
        void Finish(Job* job);
        
        Job* CreateJob(JobFunction function);
        
        Job* CreateJobAsChild(Job *parent, JobFunction function);
            
        void Run(Job* job);
        
        void Wait(const Job* job);
        
    private:
        // Job Pool
        Job g_jobAllocator[MAX_JOB_COUNT];
        unsigned int g_allocatedJobs;
        int m_threadId;
        
        // Generate Random Number
        std::default_random_engine g_rd;
        std::uniform_int_distribution<int> g_rand;
    };
}
