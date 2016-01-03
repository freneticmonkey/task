//
//  main.cpp
//  task
//
//  Created by Porter, Scott on 30/12/2015.
//

#include <iostream>

#include "JobSystem.h"

void empty_job(JobSystem::Job* job)// , const void *)
{

}

void root_job(JobSystem::Job* job)// , const void *)
{
	std::cout << "Root Job" << std::endl;
}

int main(int argc, const char * argv[]) {
	
	// Main Thread
	std::cout << "Starting Task Scheduler" << std::endl;
	
	JobSystem::Setup();

	std::cout << "Main Thread: " << std::this_thread::get_id() << std::endl;

	unsigned int N = 65000;

	for (int j = 0; j < 10; j++)
	{
		std::cout << "Enqueuing jobs: " << N << std::endl;

		// Setup Job System for new frame
		JobSystem::StartFrame();

		JobSystem::Job* root = JobSystem::CreateJob(&root_job);

		for (unsigned int i = 0; i < N; ++i)
		{
			JobSystem::Job* job = JobSystem::CreateJobAsChild(root, &empty_job);
			JobSystem::Run(job);
		}

		// Run the root job
		JobSystem::Run(root);

		// Wait for the children jobs to finish
		JobSystem::Wait(root);

		// Run Frame Cleanup
		JobSystem::EndFrame();

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	JobSystem::Shutdown();
	
	std::cout << "Finished." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	return 0;
}
