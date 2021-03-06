//
//  WorkStealingQueue.h
//  task
//
//  Created by Porter, Scott on 2/01/2016.
//

#pragma once

#include <atomic>

#include "Job.h"


namespace JobSystem
{
	static const unsigned int NUMBER_OF_JOBS = 4096u;
	static const unsigned int MASK = NUMBER_OF_JOBS - 1u;

	class WorkStealingQueue
	{
	public:
		WorkStealingQueue() : m_bottom(0), m_top(0) {}
		
		void Push(Job* job)
		{
			long b = m_bottom;
			m_jobs[b & MASK] = job;
			m_bottom = b + 1;
		}
		
		Job* Pop()
		{
			
			long b = m_bottom - 1;
			m_bottom = b;
			
			long t = m_top;
			
			if ( t <= b )
			{
				Job* job = m_jobs[b & MASK];
				if ( t != b )
				{
					// there are still jobs left
					return job;
				}
				
                if ( !m_top.compare_exchange_weak(t, t+1) )
				{
					// the last job was stolen by another thread
					job = nullptr;
				}
				m_bottom = t + 1;
				return job;
			}
			else
			{
				// queue is already empty
				m_bottom = t;
				return nullptr;
			}
		}
		
		Job* Steal()
		{
			long t = m_top;
			long b = m_bottom;
			
			if ( t < b )
			{
				Job* job = m_jobs[t & MASK];
                if ( !m_top.compare_exchange_weak(t, t+1) )
				{
					// Another thread has modified the job queue
					return nullptr;
				}
				return job;
				
			}
			else
			{
				// empty queue
				return nullptr;
			}
			
		}
		
	private:
		Job* m_jobs[NUMBER_OF_JOBS];
		std::atomic<long> m_bottom;
        std::atomic<long> m_top;
	};
	
}
