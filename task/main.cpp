//
//  main.cpp
//  task
//
//  Created by Porter, Scott on 30/12/2015.
//

#include <iostream>
#include <functional>
#include <string>

#include "JobSystem.h"

class MyData : public JobSystem::Data<MyData>
{
public:
	
	MyData() : Data() {}
	virtual ~MyData() {}
	std::string Hello() { return std::string("Hello"); }
};

void empty_job(JobSystem::Job* job, MyData* data)
{

}

void root_job(JobSystem::Job* job, MyData* data)
{
	std::cout << "Root Job: " << data->Hello() << std::endl;
}

int main(int argc, const char * argv[]) {
	
	// Main Thread
	std::cout << "Starting Task Scheduler" << std::endl;
	
	JobSystem::Setup();

	std::cout << "Main Thread: " << std::this_thread::get_id() << std::endl;

	unsigned int N = 1024;

	MyData test;

	for (int j = 0; j < 10; j++)
	{
		std::cout << "Enqueuing jobs: " << N << std::endl;

		// Setup Job System for new frame
		JobSystem::StartFrame();
		
		JobSystem::Job* root = JobSystem::CreateJob(test.Bind(root_job));
		
		for (unsigned int i = 0; i < N; ++i)
		{
			JobSystem::Job* job = JobSystem::CreateJobAsChild(root, test.Bind(empty_job));
			
			JobSystem::Run(job);
		}

		// Run the root job
		JobSystem::Run(root);

		// Wait for the children jobs to finish
		JobSystem::Wait(root);

		// Run Frame Cleanup
		JobSystem::EndFrame();
        
        std::cout << "Cycle " << j << " Finished.  Sleeping." << std::endl;
        sleep(1);
	}
	
	JobSystem::Shutdown();
	
	std::cout << "Finished." << std::endl;
    sleep(1);

	return 0;
}
