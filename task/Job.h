//
//  Job.h
//  task
//
//  Created by Porter, Scott on 2/01/2016.

#pragma once

namespace JobSystem
{
	struct Job;
	
	typedef void (*JobFunction)(Job*);//, const void*);
	Job* GetJob();
	void Execute(Job* job);
	void Finish(Job* job);
	
	struct Job
	{
		JobFunction function;
		Job* parent;
		std::atomic<int> unfinishedJobs;
		char padding[8];
	};
	

}