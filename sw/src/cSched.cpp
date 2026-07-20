#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <iomanip>
#include <fcntl.h>

#include "cSched.hpp"

#define SAME_OID_BONUS 8

using namespace std::chrono;

namespace fpga
{

	// ======-------------------------------------------------------------------------------
	// cSched management
	// ======-------------------------------------------------------------------------------

	/**
	 * @brief Construct a new cSched, bitstream handler
	 * 
	 * Constructor of the scheduler. Directly creates a new bitstream handler and a request-queue and sets vfid, priority and reorder
	 *
	 * @param vfid - vFPGA id
	 */
	cSched::cSched(int32_t vfid, uint32_t dev, bool priority, bool reorder)
		: cRnfg(dev), vfid(vfid), priority(priority), reorder(reorder),
		  plock(open_or_create, ("vpga_mtx_user_" + std::to_string(vfid)).c_str()),
#ifndef NEW_SCHEDULER
		  request_queue(taskCmprSched(priority, reorder))
#else
		  request_queue(0)
#endif
	{
		DBG3("cSched:  ctor called, vfid " << vfid);

		# ifdef VERBOSE
            std::cout << "cSched: Called constructor with vfid " << vfid << " for dev " << dev << " and with priority " << priority << " allowing to reorder " << reorder << std::endl; 
        # endif

		// Cnfg
		uint64_t tmp[2];

		// System call to driver to enable configuration of programmable region 
		if (ioctl(fd, IOCTL_PR_CNFG, &tmp))
			throw std::runtime_error("ioctl_pr_cnfg() failed, vfid: " + to_string(vfid));

		// Set configuration programmability based on return value from the ioctl-call 
		fcnfg.en_pr = tmp[0];
#ifdef NEW_SCHEDULER
		DBG3("Using SysLab Scheduler.");
#endif
	}

	/**
	 * @brief Destructor cSched
	 *
	 * Set run to false, end the scheduler thread, remove all bitstreams from the list 
	 * 
	 */
	cSched::~cSched()
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called the destructor for the class." << std::endl; 
        # endif

		DBG3("cSched:  dtor called, vfid: " << vfid);
		run = false;

		DBG3("cSched:  joining");
		scheduler_thread.join(); // Stop the scheduling thread 

		syslog(LOG_NOTICE, "Total Reconfigurations: %d", num_reconfigurations);
		syslog(LOG_NOTICE, "Total time for reconfigurations: %f µs", time_reconfigurations);

		// Iterate over all bitstreams and remove them one after each other 
		for (auto &it : bstreams)
		{
			removeBitstream(it.first);
		}

		// Remove the mutex that has been created previously in the constructor 
		named_mutex::remove(("vpga_mtx_user_" + std::to_string(vfid)).c_str());
	}

	/**
	 * @brief Run the thread: Obtain an initial lock, create a scheduler_thread with the function to process requests and wait
	 *
	 */
	void cSched::runSched()
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called runSched to run the scheduler-thread." << std::endl; 
        # endif

        unique_lock<mutex> lck_q(mtx_queue);

		// Thread
		DBG3("cSched:  initial lock");
		syslog(LOG_NOTICE, "cSched: initial lock");

		// Create the scheduler-thread to execute the processRequests-function 
		scheduler_thread = thread(&cSched::processRequests, this);
		DBG3("cSched:  thread started, vfid: " << vfid);
		syslog(LOG_NOTICE, "cSched: thread started, vfig: %d", vfid);

		cv_queue.wait(lck_q);
		DBG3("cSched:  ctor finished, vfid: " << vfid);
		syslog(LOG_NOTICE, "cSched:  ctor finished, vfid: %d", vfid);
	}

#ifdef NEW_SCHEDULER
inline bool compare(const std::unique_ptr<cLoad>& t1, const std::unique_ptr<cLoad>& t2) {
        const uint32_t prio1 = t1->priority + t1->wait_time + t1->same_oid_bonus;
        const uint32_t prio2 = t2->priority + t2->wait_time + t2->same_oid_bonus;

        syslog(LOG_NOTICE, "Comparing %d (%d) -> %d; and %d (%d) -> %d", t1->ctid, t1->oid, prio1, t2->ctid, t2->oid, prio2);
        if (prio1 != prio2) {
            return prio1 > prio2;
        } else if (t1->same_oid_bonus != t2->same_oid_bonus) {
            return t1->same_oid_bonus > t2->same_oid_bonus;
        } else {
            return t1->oid > t2->oid;
        }
    }
#endif


	// ======-------------------------------------------------------------------------------
	// (Thread) Process requests
	// ======-------------------------------------------------------------------------------
	
	// Function to run in the scheduler_thread to process the scheduled tasks 
	void cSched::processRequests()
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called processRequests, which is the function running in the scheduler-thread." << std::endl; 
        # endif

		unique_lock<mutex> lck_q(mtx_queue);
		unique_lock<mutex> lck_r(mtx_rcnfg);
		run = true; // Set run to true 
		bool recIssued = false;
		int32_t curr_oid = -1;
		cv_queue.notify_one();
		lck_q.unlock();
		lck_r.unlock();
		;

		// Busy-loop: Keep processing while run is true or the request_queue still has elements that need to be processed 
		while (run || !request_queue.empty())
		{
			lck_q.lock();

			// Check if there are still requests in the queue 
			if (!request_queue.empty())
			{

#ifndef NEW_SCHEDULER
				// Grab next reconfig request from the top of the queue 
				auto curr_req = std::move(const_cast<std::unique_ptr<cLoad> &>(request_queue.top()));
				request_queue.pop();
#else
				// Increment wait time and adjust same_oid_bonus
				syslog(LOG_NOTICE, "%lu tasks currently scheduled, curr_oid=%d", request_queue.size(), curr_oid);
				for (auto& task : request_queue) {
					task->wait_time += 1;
					if (task->oid == curr_oid) task->same_oid_bonus = SAME_OID_BONUS;
					else task->same_oid_bonus = 0;
					syslog(LOG_NOTICE, "Task %d (%d) has priority %d, wait time %d and bonus %d", task->ctid, task->oid, task->priority, task->wait_time, task->same_oid_bonus);
				}

				// Get task with highest value of (priority + wait_time + no_reschedule)
				int max_index = 0;
				for (int i = 1; i < request_queue.size(); ++i) {
					if (compare(request_queue[i], request_queue[max_index])) max_index = i;
					// if (*request_queue[i] > *request_queue[max_index]) max_index = i;
				}
				auto curr_req = std::move(request_queue[max_index]);
				request_queue.erase(request_queue.begin() + max_index);
				syslog(LOG_NOTICE, "Task %d (%d) scheduled next", curr_req->ctid, curr_req->oid);	
#endif

				lck_q.unlock();

				# ifdef VERBOSE
            		std::cout << "cSched: Get a request from the request-queue." << std::endl; 
        		# endif

				// Obtain vFPGA-lock
				plock.lock();

				// Check whether reconfiguration is possible
				if (isReconfigurable())
				{
					syslog(LOG_NOTICE, "Checking reconfiguration: %d ->? %d", curr_oid, curr_req->oid);
					// Check if the current operation ID is different to the one pulled from the request queue. Only then a reconfiguration is actually required. 
					// if (curr_oid != curr_req->oid)
					// {
						# ifdef VERBOSE
            				std::cout << "cSched: Trigger a reconfiguration." << std::endl; 
        				# endif
						syslog(LOG_NOTICE, "Performing reconfiguration: %d -> %d", curr_oid, curr_req->oid);

						// change here when using software simulation
           				auto begin_time = chrono::high_resolution_clock::now();
						reconfigure(curr_req->oid); // If reconfiguration is possible and oid has changed, start a reconfiguration 
						auto end_time = chrono::high_resolution_clock::now();
            			double time = chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
						std::cout << "cSched: Trigger a reconfiguration." << time << std::endl; 
            			syslog(LOG_NOTICE, "Reconfiguration done in %f µs", time);
						time_reconfigurations += time;

						recIssued = true;
						curr_oid = curr_req->oid; // Update current operator ID 
					// }
					// else
					// {
					// 	syslog(LOG_NOTICE, "No reconfiguration required");
					// 	recIssued = false;
					// }
				}

				// Notify
				lck_r.lock();
				curr_ctid = curr_req->ctid;
				curr_run = true;
				lck_r.unlock();
				cv_rcnfg.notify_all();

				// Wait for task completion
				unique_lock<mutex> lck_c(mtx_cmplt);
				if (cv_cmplt.wait_for(lck_c, cmplTimeout, [=]
									  { return curr_run == false; }))
				{
					syslog(LOG_NOTICE, "Task completed, %s, ctid %d, oid %d, priority %d\n",
						   (recIssued ? "operator loaded, " : "operator present, "), curr_req->ctid, curr_req->oid, curr_req->priority);

					# ifdef VERBOSE
            			std::cout << "cSched: Task completed with ctid " << curr_req->ctid << " and oid " << curr_req->oid << " and priority " << curr_req->priority << std::endl; 
       	 			# endif
				}
				else
				{
					syslog(LOG_NOTICE, "Task failed, ctid %d, oid %d, priority %d\n",
						   curr_req->ctid, curr_req->oid, curr_req->priority);
				}

				plock.unlock();
			}
			else
			{
				lck_q.unlock();
			}

			nanosleep(&PAUSE, NULL);
		}
	}

	// Place a new load in the request_queue
	void cSched::pLock(int32_t ctid, int32_t oid, uint32_t priority)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called pLock to place a new load in the request-queue." << std::endl; 
        # endif
		syslog(LOG_NOTICE, "Adding new task with ctid %d, oid %d, priority %d", ctid, oid, priority);
		unique_lock<std::mutex> lck_q(mtx_queue);
#ifndef NEW_SCHEDULER
		request_queue.emplace(std::unique_ptr<cLoad>(new cLoad{ctid, oid, priority}));
#else
		request_queue.push_back(std::unique_ptr<cLoad>(new cLoad{ctid, oid, priority, 0, 0}));
#endif
		syslog(LOG_NOTICE, "Request Queue size: %lu", request_queue.size());
		lck_q.unlock();

		unique_lock<std::mutex> lck_r(mtx_rcnfg);
		cv_rcnfg.wait(lck_r, [=]
					  { return ((curr_run == true) && (curr_ctid == ctid)); });
	}

	void cSched::pUnlock(int32_t ctid)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called pUnlock." << std::endl; 
        # endif

		unique_lock<std::mutex> lck_c(mtx_cmplt);
		if (curr_ctid == ctid)
		{
			curr_run = false;
			cv_cmplt.notify_one();
		}
	}

	// ======-------------------------------------------------------------------------------
	// Reconfiguration
	// ======-------------------------------------------------------------------------------

	/**
	 * @brief Reconfiguration IO - calls the bitstream handler to trigger a reconfiguration of the FPGA
	 *
	 * @param oid - operator id
	 */
	void cSched::reconfigure(int32_t oid)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called reconfigure to trigger a hardware reconfiguration for oid " << oid << std::endl; 
        # endif

		if (bstreams.find(oid) != bstreams.end())
		{
			auto bstream = bstreams[oid];
			reconfigureBase(std::get<0>(bstream), std::get<1>(bstream), vfid);
			++num_reconfigurations;
		}
	}

	/**
	 * @brief Add a bitstream to the map
	 *
	 * @param name - path
	 * @param oid - operator ID
	 */
	void cSched::addBitstream(std::string name, int32_t oid)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called addBitstream to add the bitstream " << name << " with oid " << oid << std::endl; 
        # endif

		// Check that the new bitstream (identified by the operator ID) is not yet stored in the bitstream-map
		if (bstreams.find(oid) == bstreams.end())
		{
			// Create an input-stream of the bitstream, from it's original file 
			syslog(LOG_NOTICE, "loading bitstream %s", name.c_str());
			ifstream f_bit(name, ios::ate | ios::binary);
			if (!f_bit)
				throw std::runtime_error("Shell bitstream could not be opened");
			
			// Read bitstream from the input-stream 
			bStream bstream = readBitstream(f_bit);
			f_bit.close();
			DBG3("Bitstream loaded, oid: " << oid);
			syslog(LOG_NOTICE, "Bitstream loaded, oid: %d", oid);
			
			// Insert the new bitstream with the operator ID in the bitstream-map 
			bstreams.insert({oid, bstream});
			return;
		}

		// Error if the bitstream with this operator ID is already present 
		throw std::runtime_error("bitstream with same operation ID already present");
	}

	/**
	 * @brief Remove a bitstream from the map
	 *
	 * @param: oid - Operator ID
	 */
	void cSched::removeBitstream(int32_t oid)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called removeBitstream to remove the bitstream with oid " << oid << std::endl; 
        # endif

		// Check if the operator ID of the bitstream to be removed can actually be found in the Bitstream-Map
		if (bstreams.find(oid) != bstreams.end())
		{
			auto bstream = bstreams[oid];
			freeMem(bstream.first);	// memory for the bitstream is freed
			bstreams.erase(oid); // entry is erased from the from bitstream-map 
		}
	}

	/**
	 * @brief Check if bitstream is present
	 *
	 * @param oid - Operator ID
	 */
	bool cSched::checkBitstream(int32_t oid)
	{
		# ifdef VERBOSE
            std::cout << "cSched: Called checkBitstream to check the bitstream with oid " << oid << std::endl; 
        # endif

		// Check bitstream-map with the operator-ID 
		if (bstreams.find(oid) != bstreams.end())
		{
			return true;
		}
		return false;
	}

}
