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
	
	int g_workerThreadCount;
	bool g_workerThreadActive;
	bool g_workerCleanJobs;
	
	std::vector<WorkStealingQueue*> g_jobQueues;
	std::vector<std::thread> g_threads;

	std::atomic<unsigned int> g_poolMissCount;
	std::atomic<unsigned int> g_jobFinishCount;
	unsigned int g_createdJobs;

	std::vector<unsigned int> g_threadJobsFinished;
	
	// Great! Apple is a piece of shit. thread_local isn't supported.

	// Job Pool
	static thread_local Job g_jobAllocator[MAX_JOB_COUNT];
	static thread_local unsigned int g_allocatedJobs = 0;
	thread_local int threadId;

	// Generate Random Number
	thread_local std::default_random_engine g_rd;
	thread_local std::uniform_int_distribution<int> g_rand;
	
	bool HasJobCompleted(const Job* job)
	{
		return job->unfinishedJobs < 1;
	}

	void CleanJobs()
	{
		for (auto& job : g_jobAllocator)
		{
			job.unfinishedJobs = 0;
		}
	}
	
	Job* AllocateJob()
	{
		unsigned int index = g_allocatedJobs++;
		unsigned int startingIndex = index;
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
	
	WorkStealingQueue* GetWorkerThreadQueue()
	{
		return g_jobQueues[threadId];
	}
	
	bool IsEmptyJob(Job* job)
	{
		return (job == nullptr);
	}
	
	Job * GetJob(void)
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
	
	void Execute(Job* job)
	{
		job->function(job);
		Finish(job);
	}
	
	void Finish(Job* job)
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
			g_threadJobsFinished[threadId]++;
		}
	}
	
	Job* CreateJob(JobFunction function)
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
	
	Job* CreateJobAsChild(Job *parent, JobFunction function)
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
		
	void Run(Job* job)
	{
		if (job)
		{
			WorkStealingQueue* queue = GetWorkerThreadQueue();
			queue->Push(job);
		}
	}
	
	void Wait(const Job* job)
	{
		g_rand = std::uniform_int_distribution<int>(0, g_workerThreadCount - 1);
		threadId = 0;
		std::cout << "Main Thread: " << std::this_thread::get_id() << std::endl;

		// wait until completed
		while (!HasJobCompleted(job))
		{
			Job* nextJob = GetJob();
			if ( nextJob )
			{
				Execute(nextJob);
			}
		}
	}
	
	void tmain(int tId)
	{
		g_rand = std::uniform_int_distribution<int>(0, g_workerThreadCount - 1);
		threadId = tId;
		std::cout << "Thread: " << threadId << " id: " << std::this_thread::get_id() << std::endl;
		
		while ( g_workerThreadActive )
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

		CleanJobs();

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
