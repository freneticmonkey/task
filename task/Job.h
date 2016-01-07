//
//  Job.h
//  task
//
//  Created by Porter, Scott on 2/01/2016.

#pragma once

#include <functional>

namespace JobSystem
{
	struct Job;
	
	typedef std::function<void(Job*)> JobFunction;

    template<class T>
	class Data
	{
	public:
		typedef std::function<void(JobSystem::Job*, T*)> DataFunction;

		virtual ~Data() {};

		JobFunction Bind(const DataFunction &func)
		{
			return std::bind(func, std::placeholders::_1, reinterpret_cast<T*>(this));
		}
	};

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