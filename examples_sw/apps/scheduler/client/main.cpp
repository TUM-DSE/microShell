#include "types.hpp"

#include <thread>
#include <future>
#include <math.h>
#include <unordered_map>
#include <random>
#include <cmath> 

#include "virtual_worker.hpp"
#define HW_TEST
#define DEBUG



int n_regions = 4;                  // Number of vFPGAs
int n_real_regions = 8;             // Number of actual vFPGAs without PR
int def_policy = 0;                 // default FIFO policy
int def_tasks = 8;                  // default FIFO policy


/* Signal handler */
std::atomic<bool> stalled(false);
void gotInt(int) {
    stalled.store(true);
}

std::mutex tasks_mutex;

// Struct for time benchmarks
struct Time {
    double cmpl;
    double turn;
    double wait;

    Time() : cmpl(0.), turn(0.), wait(0.) {}
    Time(double a, double b, double c) : cmpl(a), turn(b), wait(c) {}
};

struct TaskInfo {
    std::vector<uint32_t> oids;
    uint32_t priority;
    uint32_t region_idx;
    std::vector<uint32_t> coverage;
    uint32_t time;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    double response;
    double deadline;
};

// component to vFPGA mapping
std::unordered_map<int, std::vector<uint32_t>> app_vFPGA_mapping = {
    {1, {0, 5}},    // RLE
    {2, {1, 3}},    // RSA
    {3, {2, 6}},    // SHA256
    {4, {4, 7}}     // FFT
};


std::vector<std::unique_ptr<cThread<std::any>>> cthread; // Coyote threads
void* hMem[8];
sgEntry sg[8];

WorkerPool pool(8);
VirtualWorkerManager vwm(pool);

uint32_t reconfiguration_count = 0;

// the tasks priority array
// supporting 3 priorities for the moment
std::vector<std::vector<TaskInfo>> tasks_arrays(3);
// simulating the vFPGAs, storing currently running tasks
std::vector<TaskInfo> current_tasks;
std::vector<TaskInfo> tasks_scheduled;
std::vector<TaskInfo> vFPGA_status;

std::vector<uint32_t> find_overlap(const std::vector<uint32_t>& arr1, const std::vector<uint32_t>& arr2) {
    std::vector<uint32_t> overlap;
    std::vector<uint32_t> arr2_copy = arr2;
    for (uint32_t num1 : arr1) {
        for (uint32_t num2 : arr2_copy) {
            if (num1 == num2) {
                overlap.push_back(num1);
                auto it = std::find(arr2_copy.begin(), arr2_copy.end(), num2); // Step 1: Find the element
                if (it != arr2_copy.end()) {
                    arr2_copy.erase(it); // Step 2: Erase if found
                }
                break; // Move to next element in arr1
            }
        }
    }
    return overlap;
}

int select_task(std::vector<TaskInfo>& tasks,
    const std::vector<uint32_t>& current_oid
    ) {
    std::cout << "in select_task" << std::endl;

    std::vector<int> coverage;

    for (auto& task : tasks) {
        // calculate coverage
        std::vector<uint32_t> overlap = find_overlap(task.oids, current_oid);
        task.coverage = overlap;
        coverage.push_back(overlap.size());
#ifdef DEBUG
        std::cout << "task is: " << task.oids[0] << ", " << task.oids[1] << std::endl;
        std::cout << "overlap size is: " << overlap.size() << std::endl;
        for (auto& a : overlap) {
            std::cout << a << ", ";
        }
        std::cout << std::endl;
#endif
        // std::cout << "coverage[0]: " << coverage[0] << std::endl;
    }
#ifdef DEBUG
    std::cout << "Coverage size after push_back: " << coverage.size() << std::endl;
#endif
    // std::cout << "coverage[0] is " << coverage[0] << std::endl;

    // find the maximum coverage task
    if (!coverage.empty()) {
        int max_value = coverage[0];
        int max_index = 0;

        for (int i = 1; i < coverage.size(); ++i) {
            if (coverage[i] > max_value) {
                max_value = coverage[i];
                max_index = i;
            }
        }

        // std::cout << "Max value: " << max_value << ", Index in coverage array: " << max_index << std::endl;
        return max_index;
    }
    else {
        std::cout << "coverage array is 0" << std::endl;
        return -1;
    }
}


int schedule_oid_vFPGA(int oid_to_schedule) {
    int choosen_vid;
    for (auto& vid : app_vFPGA_mapping[oid_to_schedule]) {
        // if (pool.is_busy(vid) == false) {
        if (vwm.is_actual_free(vid)) {
            choosen_vid = vid;
            break;
        }
    } 
    return choosen_vid;
}

// the main function to schedule applications
int32_t schedule_vfpga(TaskInfo& task, bool new_task, uint32_t policy) {

    TaskInfo choosen_task;
    // if there's new task to schedule
    if (new_task)
        tasks_arrays[task.priority].push_back(task);

    uint8_t array_index = 0;
    std::vector<TaskInfo> tasks = tasks_arrays[0];

    for (auto& task_array : tasks_arrays) {
        if (task_array.size()) {
            tasks = task_array;
            break;
        }
        array_index++;
    }

    std::cout << "priority level " << static_cast<int>(array_index) << std::endl;
    std::cout << "tasks waiting to schedule" << std::endl; 

    for (int i = 0; i < tasks.size(); ++i) {
        std::cout << tasks[i].oids[0] << ", " << tasks[i].oids[1] << ". time: " << tasks[i].time << std::endl;
    }

#ifdef DEBUG
    std::cout << "actual vFPGA status" << std::endl; 
    for (int i = 0; i < vFPGA_status.size(); ++i) {
        std::cout << "vFPGA " << i << ": " << vFPGA_status[i].oids[0] << ". busy: " << vwm.is_actual_busy(i) << std::endl;
    }
#endif

    std::vector<uint32_t> current_oid;
    // store free virtual vFPGAs last executed components
    std::vector<uint32_t> empty_vFPGA;
    // store index of free virtual vFPGAs
    std::vector<uint32_t> empty_vFPGA_index;

    // stores currently executing task component IDs
    for (int i = 0; i < current_tasks.size(); ++i) {
        current_oid.push_back(current_tasks[i].oids[0]);
    }

    std::cout << "empty vfpga" << std::endl;

    // check how many vfpgas are free
    // std::cout << "check vfPGA 0, module " << (pool.is_busy(0) == false) << std::endl;
    for (int i = 0; i < current_tasks.size(); ++i) {
        // if (current_tasks[i].oids[0] == 0 || current_tasks[i].time == 0) {
        // if (current_tasks[i].oids[0] == 0 || pool.is_busy(i) == false) {
        if (current_tasks[i].oids[0] == 0 || vwm.is_free(i)) {
            empty_vFPGA.push_back(current_tasks[i].oids[0]);
            empty_vFPGA_index.push_back(i);
            std::cout << "free vFPGA " << i << ", module " << current_tasks[i].oids[0] << std::endl;
        }
    }

    // // custom scheduling algorithm
    // int next = select_task(tasks, empty_vFPGA);

    // if (policy == 0) {
    //     // default scheduling algorithm
    //     next = 0;
    // }   


    // default scheduling algorithm
    int next = 0;

    if (policy == 1) {
        // custom scheduling algorithm
        next = select_task(tasks, empty_vFPGA);
    }    


    int scheduled = 1;
    choosen_task = tasks[next];

    if (choosen_task.coverage.size() + empty_vFPGA.size() < choosen_task.oids.size()) {
        next = -1;
        std::cout << "vFPGA not enough" << std::endl;
    }

    if (next != -1) {
        std::vector<uint32_t> oids_no_overlap;

        std::cout << "selected task ";
        for (int i = 0; i < choosen_task.oids.size(); i++) {
            std::cout << choosen_task.oids[i] << ", ";
        }
        std::cout << std::endl;

        oids_no_overlap = choosen_task.oids;

        for (auto& e : choosen_task.coverage) {
            for (int i = 0; i < oids_no_overlap.size(); i++) {
                if (oids_no_overlap[i] == e) {
                    oids_no_overlap[i] = 0;
                    break;
                }
            }
        }
        oids_no_overlap.erase(std::remove(oids_no_overlap.begin(), oids_no_overlap.end(), 0), oids_no_overlap.end());

#ifdef DEBUG
        std::cout << "non overlap module ";
        for (auto& e : oids_no_overlap) {
            std::cout << e << ", ";
        }
        std::cout << std::endl;
#endif

        // Task generator
        auto create_task = [](int id, TaskInfo task, std::vector<TaskInfo>& tasks, std::mutex& tasks_mutex, int reconfigure) {
            return [id, task, &tasks, &tasks_mutex, reconfigure] mutable {
                std::cout << "****" << id << "****\n";
                std::cout << "vFPGA " << id << " starting task\n";
                if (reconfigure) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(34));
                }
#ifdef HW_TEST 
                cthread[id]->memCap(MemCapa::BASE_ADDRESS, MemCapa::END_ADDRESS, MemCapa::ALL_PASS);
                cthread[id]->ioSwitch(IODevs::Inter_6_TO_HOST_0);
                for(int i = 0; i < 32; i++) {
                    cthread[id]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[id], {true, true, false});
                    while(cthread[id]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
                        if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");           
                }
                // std::this_thread::sleep_for(std::chrono::milliseconds(20));
#else
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
#endif
                {
                    std::lock_guard<std::mutex> lock(tasks_mutex);
                    try {
                        // std::cout << "Tasks size: " << tasks.size() << std::endl;
                        task.end_time = std::chrono::high_resolution_clock::now();
                        tasks.push_back(task);
                    } catch (const std::bad_alloc& e) {
                        std::cerr << "Allocation failed: " << e.what() << std::endl;
                    }
                }
                std::cout << "Worker " << id << " completed task\n";
            };
        };

        int choose_vFPGA_id = 0;
        // deploy the task by updating the current_tasks vector
        // deside how to deploy the tasks
        for (int i = 0; i < choosen_task.coverage.size(); ++i) {
            int found = -1;
            // search if component exists, the component needs to finish execution
            for (int j = 0; j < empty_vFPGA_index.size(); j++) {
                // if (current_tasks[empty_vFPGA_index[j]].oids[0] == choosen_task.coverage[i] && current_tasks[empty_vFPGA_index[j]].time == 0) {
                // if (current_tasks[empty_vFPGA_index[j]].oids[0] == choosen_task.coverage[i] && pool.is_busy(empty_vFPGA_index[j]) == false) {
                if (current_tasks[empty_vFPGA_index[j]].oids[0] == choosen_task.coverage[i] && vwm.is_free(empty_vFPGA_index[j])) {
                    // updating component time as it's new
                    // current_tasks[empty_vFPGA_index[j]].time = choosen_task.time;
                    found = empty_vFPGA_index[j];
                    // for (int k = 0; k < choosen_task.overlap.size(); k++) {
                    //     if (current_tasks[j].oids[0] == choosen_task.overlap[k]) {
                    //         choosen_task.overlap.erase(choosen_task.overlap.begin() + k);
                    //     }
                    // }
                    std::cout << "reusing vFPGA " << found << ", module: " << choosen_task.coverage[i] << std::endl;
                    TaskInfo task_copy = choosen_task;
                    // pool.assign_task(found, create_task(found, task_copy, tasks_scheduled, tasks_mutex, 0));
                    vwm.map(found, found);
                    vwm.assign_task(found, create_task(found, task_copy, tasks_scheduled, tasks_mutex, 0));
                    empty_vFPGA_index.erase(empty_vFPGA_index.begin() + j);

                    break;
                }
            }
        }


        for (int i = 0; i < oids_no_overlap.size(); ++i) {
            // component does not exist, find a new slot to run
            // if (found == -1 && empty_vFPGA_index.size() > 0) {
            if (empty_vFPGA_index.size() > 0) {
                std::cout << "need to reconfigure" << std::endl;
                std::cout << "vFPGA index " << empty_vFPGA_index[0] << ", module: " << oids_no_overlap[i] << std::endl;
                choose_vFPGA_id = schedule_oid_vFPGA(oids_no_overlap[i]);
                std::cout << "with PR, vFPGA index " << choose_vFPGA_id << ", module: " << oids_no_overlap[i] << std::endl;
                current_tasks[empty_vFPGA_index[0]].oids[0] = oids_no_overlap[i];
                // current_tasks[empty_vFPGA_index[0]].time = choosen_task.time;
                // reconfigure(empty_vFPGA[0]);
                // std::this_thread::sleep_for(std::chrono::milliseconds(17));
                TaskInfo task_copy = choosen_task;
                // pool.assign_task(empty_vFPGA_index[0], create_task(empty_vFPGA_index[0], task_copy, tasks_scheduled, tasks_mutex, 1));
                vwm.map(empty_vFPGA_index[0], choose_vFPGA_id);
                vwm.assign_task(empty_vFPGA_index[0], create_task(choose_vFPGA_id, task_copy, tasks_scheduled, tasks_mutex, 1));
                // pool.assign_task(choose_vFPGA_id, create_task(choose_vFPGA_id, task_copy, tasks_scheduled, tasks_mutex, 1));
                empty_vFPGA_index.erase(empty_vFPGA_index.begin() + 0);
                reconfiguration_count += 1;
            }
        }
        if (scheduled) {
            tasks_arrays[array_index].erase(tasks_arrays[array_index].begin() + next);
            // add task to current_oid
            std::cout << "task found " << next << std::endl;
            // std::cout << "tasks size " << tasks.size() << std::endl; 

            // pool.assign_task(i, create_task(i));
        }
    }
    else {
        scheduled = 0;
        std::cout << "no task found " << next << std::endl;
    }

#ifdef DEBUG
    std::cout << "current status: " << std::endl;
    for (int i = 0; i < current_tasks.size(); i++) {
        std::cout << "vFPGA " << i << ": " << current_tasks[i].oids[0] << std::endl;
        // std::cout << "vFPGA " << i << ": " << current_tasks[i].oids[0] << ", time: " << current_tasks[i].time << std::endl;
    }
#endif
    // // for every cycle, increase the current time's task 
    // for (int j = 0; j < current_tasks.size(); j++) {
    //     if (current_tasks[j].time > 0)
    //         current_tasks[j].time -= 1;
    // }

    for (auto& task_array : tasks_arrays) {
        if (task_array.size()) {
            for (auto& task : task_array) {
                task.time ++;
            }
        }
    }

    if (policy == 1) {
        // custom scheduling algorithm
        for (int i = 1; i < 3; i++) {
            auto& current_queue = tasks_arrays[i];
            auto& target_queue = tasks_arrays[i - 1];

            // Use iterator so we can safely erase while iterating
            for (auto it = current_queue.begin(); it != current_queue.end(); ) {
                if (it->time >= 3) {
                    it->time = 0;
                    target_queue.push_back(*it);       // move task
                    it = current_queue.erase(it);      // erase and advance iterator
                    std::cout << "promoting task" << std::endl;
                } else {
                    ++it;                              // move to next
                }
            }
        }
    }
    
    if (scheduled) {
        std::cout << "task scheduled" << std::endl;
        return next;
    }
    else {
        std::cout << "no task scheduled" << std::endl;
        return -1;

    }
}


int main(int argc, char* argv[])
{
    // Sig handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // ---------------------------------------------------------------
    // Args 
    // ---------------------------------------------------------------

    // Default args
    uint32_t cs_dev = def_dev;              // FPGA device
    uint32_t policy = def_policy;             // start vFPGA index
    uint32_t n_tasks = def_tasks;                   // Task duplicator
    uint32_t n_apps = total_apps;   // Amount of different hll variation to run
    uint32_t n_tuples = 8 * 1024;
    uint32_t n_features = 5;
    std::string task_order = "rng";
    uint32_t task_order_mode = 2;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show this help message")
        ("device,d", po::value<uint32_t>(&cs_dev)->default_value(cs_dev), "Target device")
        ("policy,p", po::value<uint32_t>(&policy)->default_value(policy), "Policy")
        // ("regions,g", po::value<uint32_t>(&n_regions)->default_value(n_regions), "Number of vFPGAs")
        ("tasks,t", po::value<uint32_t>(&n_tasks)->default_value(n_tasks), "Number of tasks total")
        ("taskorder,o", po::value<std::string>(&task_order)->default_value(task_order), "Task ordering mode (seq|grp|rng)")
        ("nconfig,n", po::value<uint32_t>(&n_apps)->default_value(n_apps), "Number of HyperLogLog variations to use")
        ("tuples", po::value<uint32_t>(&n_tuples)->default_value(n_tuples), "Number of tuples")
        ("features", po::value<uint32_t>(&n_features)->default_value(n_features), "Number of features")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Usage: options_description [options]\n";
        std::cout << desc;
        return 0;
    }

    // Parse task order
    if (vm.count("taskorder")) {
        if (task_order == "seq") task_order_mode = 0;
        else if (task_order == "grp") task_order_mode = 1;
        else if (task_order == "rng") task_order_mode = 2;
        else {
            std::cout << "Invalid ordering mode. Available sequential (seq), grouped (grp) or random (rng)" << std::endl;
            task_order = "seq";
            task_order_mode = 0;
        }
    }

    if (n_apps <= 0 || n_apps > total_apps) {
        std::cout << "Invalid configurations count. Max: " << total_apps << std::endl;
        return 1;
    }

    PR_HEADER("PARAMS");
    std::cout << "Device:             " << cs_dev << std::endl;
    std::cout << "Number of regions:  " << n_regions << std::endl;
    std::cout << "Number of tasks:    " << n_tasks << std::endl;
    std::cout << "Task order mode:    " << task_order << std::endl;
    // std::cout << "HLL Variations:     " << n_apps << std::endl;
    // std::cout << "Number of tuples:   " << n_tuples << std::endl;
    // std::cout << "Number of features: " << n_features << std::endl;
    if (policy == 1)
        std::cout << "Policy:             " << "Custom" << std::endl;
    else
        std::cout << "Policy:             " << "FIFO" << std::endl;
    std::cout << std::endl;

    
    // ---------------------------------------------------------------
    // Init 
    // ---------------------------------------------------------------

#ifdef HW_TEST        
    bool mapped = true;
    bool huge = true;
    bool stream = 1;
    uint32_t max_size = 16 * 1024 * 1024;
    uint32_t curr_size = 16 * 1024 * 1024;

    // std::vector<std::unique_ptr<cThread<std::any>>> cthread; // Coyote threads
    // void* hMem[n_regions];
    // sgEntry sg[n_regions];
    
    // Obtain resources
    for (int i = 0; i < n_real_regions; i++) {
        cthread.emplace_back(new cThread<std::any>(i, getpid(), cs_dev));
        hMem[i] = mapped ? (cthread[i]->getMem({huge ? CoyoteAlloc::HPF : CoyoteAlloc::REG, max_size})) 
                         : (huge ? (mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0))
                                 : (malloc(max_size)));
    }


    for(int i = 0; i < n_real_regions; i++) {
        // SG entries
        memset(&sg[i], 0, sizeof(localSg));
        sg[i].local.src_addr = hMem[i]; sg[i].local.src_len = curr_size; sg[i].local.src_stream = stream;
        sg[i].local.dst_addr = hMem[i]; sg[i].local.dst_len = curr_size; sg[i].local.dst_stream = stream;
    }

    for (int j = 0; j < n_real_regions; j++) {
        for (int i = 0; i < max_size / 8; i++) {
            ((uint32_t *)hMem[j])[i] = 0;
        }
    }
#endif

    std::unordered_map<int, int> vFPGA_app_mapping = {
        {0, 1},    // RLE
        {1, 2},    // RSA
        {2, 3},    // SHA256
        {3, 2},    // RSA
        {4, 4},    // FFT
        {5, 1},    // RLE
        {6, 3},    // SHA256
        {7, 4},    // FFT
    };

    std::unordered_map<int, std::vector<uint32_t>> app_mapping = {
        // signed compression (RLE, RSA)
        {1, {1, 2}},
        // digital signature (SHA256, RSA)
        {2, {3, 2}},
        // audio processing (FFT, RLE)
        {3, {4, 1}}
    };

    // Assign tasks
    int32_t taskOID[n_tasks];
    if (n_tasks == 0) {
        DBG3("Nothing to run.");
        std::cout << "See --help to view available options." << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::vector<TaskInfo> tasks_to_schedule;
    // random for tasks' oids
    srand(n_tasks * 7 + n_features);  // Set same seed for random to allow to compare results

    // random for deadlines
    std::random_device rd;                        // seed
    std::mt19937 gen(rd());                       // Mersenne Twister engine
    std::uniform_int_distribution<> distr(20, 50); // range: [1, 100]

    for (int32_t i = 0; i < n_tasks; ++i) {
        if (task_order_mode == 0) {
            // Sequential order: 1,2,3,1,2,3,...
            taskOID[i] = (i % n_apps) + 1;  // OID is from 1..4
        }
        else if (task_order_mode == 1) {
            // Grouped order: 1,1,2,2,3,3,...
            taskOID[i] = (i / n_apps) + 1;
        }
        else if (task_order_mode == 2) {
            // Random order: 1,3,1,3,2,3,2,2,...
            taskOID[i] = (rand() % n_apps) + 1;
            TaskInfo task;
            std::cout << "taskOID[i] is " << taskOID[i] << std::endl;
            // task.oids.push_back((rand() % n_apps) + 1);
            // task.oids.push_back((rand() % n_apps) + 1);
            task.oids = app_mapping.at(taskOID[i]);
            // std::cout << "mapping " << app_mapping.at(taskOID[i])[0] << std::endl;
            task.priority = (rand() % 3);
            task.time = 0;
            task.deadline = (40 + 80) * (100 + distr(gen)) / 100 * (task.priority + 1);
            tasks_to_schedule.push_back(task);
            std::cout << "input is " << task.oids[0] << ", " << task.oids[1] << std::endl;
            std::cout << "task priority: " << task.priority << std::endl;
            std::cout << "task deadline: " << task.deadline << std::endl;
            // std::cout << "task array size: " << tasks_to_schedule.size() << std::endl;
        }
        else {
            DBG3("Invalid ordering mode.");
            exit(EXIT_FAILURE);
        }
    }

    // store empty tasks into the vector
    for (int i = 0; i < n_regions; i++) {
        TaskInfo task;
        task.oids.push_back(0);
        task.time = 0;
        current_tasks.push_back(task);
    }

    // store empty tasks into the vector
    for (int i = 0; i < n_real_regions; i++) {
        TaskInfo task;
        task.oids.push_back(vFPGA_app_mapping.at(i));
        task.time = 0;
        vFPGA_status.push_back(task);
    }

    // Start total time benchmark
    auto sched_begin = std::chrono::high_resolution_clock::now();
    int32_t result = 0;
    uint32_t cycle = 0;

#ifdef HW_TEST
    cthread[0]->memCap(MemCapa::BASE_ADDRESS, MemCapa::END_ADDRESS, MemCapa::ALL_PASS);
    cthread[0]->ioSwitch(IODevs::Inter_6_TO_HOST_0);
    for(int i = 0; i < 32; i++) {
        cthread[0]->invoke(CoyoteOper::LOCAL_TRANSFER, &sg[0], {true, true, false});
        while(cthread[0]->checkCompleted(CoyoteOper::LOCAL_WRITE) != 1) 
            if(stalled.load()) throw std::runtime_error("Stalled, SIGINT caught");           
    }

    auto sched_middle = std::chrono::high_resolution_clock::now();
    double duration_middle;
    duration_middle = (std::chrono::duration_cast<std::chrono::microseconds>(sched_middle - sched_begin).count() / 1000.0);
    std::cout<< "Experiment current (ms): " << duration_middle << std::endl;
#endif

    sched_begin = std::chrono::high_resolution_clock::now();
    int limit = 0;
    while (tasks_to_schedule.size() > 0) {
        std::cout << "---------------------------------------------------------------" << std::endl;
        std::cout << "tasks_to_schedule size is " << tasks_to_schedule.size() << std::endl;
        std::cout << "input is " << tasks_to_schedule[0].oids[0] << ", " << tasks_to_schedule[0].oids[1] << std::endl;

        cycle++;
        tasks_to_schedule[0].start_time = std::chrono::high_resolution_clock::now();
        std::cout << "starting time: " << (std::chrono::duration_cast<std::chrono::microseconds>(tasks_to_schedule[0].start_time - sched_begin).count() / 1000.0) << std::endl;

        result = schedule_vfpga(tasks_to_schedule[0], true, policy);

        tasks_to_schedule.erase(tasks_to_schedule.begin() + 0);
        if (result >= 0) {
            std::cout << "scheduling some task " << result << std::endl;
        }
        // limit++;
        // if (limit > 10) {
        //     std::cout << "can't finish scheduling" << std::endl;
        //     break;
        // }
#ifdef HW_TEST 
#ifdef DEBUG
        sched_middle = std::chrono::high_resolution_clock::now();
        duration_middle = (std::chrono::duration_cast<std::chrono::microseconds>(sched_middle - sched_begin).count() / 1000.0);
        std::cout<< "Experiment current (ms): " << duration_middle << std::endl;
#endif
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    std::cout << "schedule finished. Cycle: " << cycle << std::endl;

    int remain_task = 0;
    int limit_2 = 0;
    for (int i = 0; i < tasks_arrays.size(); ++i) {
        remain_task += tasks_arrays[i].size();
    }

    TaskInfo task_empty;
    while (remain_task > 0) {
        std::cout << "===============================================================" << std::endl;
        std::cout << "scheduling remain tasks " << remain_task << std::endl;
        cycle++;

        result = schedule_vfpga(task_empty, false, policy);

        if (result >= 0) {
            std::cout << "scheduling some task " << result << std::endl;
        }
        // limit_2++;
        // if (limit_2 > 10) {
        //     std::cout << "can't finish scheduling 2" << std::endl;
        //     break;
        // }
        remain_task = 0;
        for (int i = 0; i < tasks_arrays.size(); ++i) {
            remain_task += tasks_arrays[i].size();
        }
#ifdef HW_TEST 
#ifdef DEBUG
        sched_middle = std::chrono::high_resolution_clock::now();
        duration_middle = 0.0;
        duration_middle = (std::chrono::duration_cast<std::chrono::microseconds>(sched_middle - sched_begin).count() / 1000.0);
        std::cout<< "Experiment current (ms): " << duration_middle << std::endl;

#endif
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::cout << "in schedule remain end" << std::endl;
    }

    std::cout << "schedule remain finished" << std::endl;

    bool all_completed = true;

    while (true) {
        all_completed = true;
        // Check for completed workers
        for (int i = 0; i < n_real_regions; ++i) {
            // if (pool.is_busy(i)) {
            if (vwm.is_busy(i)) {
                all_completed = false;
                std::cout << "Busy vFPGA: " << i << std::endl;
            }
        }
        if (all_completed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }


    // Evaluation
    std::cout << "All task done." << std::endl;

    double durationMs = 0.0;
    double durationMs_final = 0.0;

    auto sched_end = std::chrono::high_resolution_clock::now();
    durationMs_final = (std::chrono::duration_cast<std::chrono::microseconds>(sched_end - sched_begin).count() / 1000.0);

    std::vector<double> response_time;
    int deadline_miss_count = 0;
    for (auto& task : tasks_scheduled) {
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << "task components: " << task.oids[0] << ", " << task.oids[1] << std::endl;
        std::cout << "starting time: " << (std::chrono::duration_cast<std::chrono::microseconds>(task.start_time - sched_begin).count() / 1000.0) << std::endl;
        std::cout << "end time: " << (std::chrono::duration_cast<std::chrono::microseconds>(task.end_time - sched_begin).count() / 1000.0) << std::endl;
        durationMs = (std::chrono::duration_cast<std::chrono::microseconds>(task.end_time - task.start_time).count() / 1000.0);
        response_time.push_back(durationMs);
        std::cout << "duration: " << durationMs << " ms" << std::endl;
        if (task.deadline < durationMs) {
            deadline_miss_count ++;
        }
    }


    double sum = std::accumulate(response_time.begin(), response_time.end(), 0.0);
    double average = sum / response_time.size();

    // std::cout << "experiment starting time: " << sched_begin << std::endl;
        
    // std::cout << "experiment end time: " << sched_end << std::endl;

    // Sort the vector from largest to smallest
    std::sort(response_time.begin(), response_time.end(), std::greater<double>());

    std::cout << "--------------------------------------------------" << std::endl;

    // Print the sorted vector
    std::cout << "Sorted (largest to smallest): ";
    for (const double &time : response_time) {
        std::cout << time << " ";
    }
    std::cout << std::endl;

    std::cout << "Experiment finished duration (ms): " << durationMs_final << std::endl;

    std::cout << "Reconfiguration count: " << reconfiguration_count << std::endl;

    std::cout << "Average response time: " << average << std::endl;

    std::cout << "Response time (95%): " << response_time[static_cast<int>(std::ceil(n_tasks/20))] << std::endl;

    std::cout << "Deadline misses: " << deadline_miss_count << std::endl;

    std::cout << "--------------------------------------------------" << std::endl;
    
    std::cout << durationMs_final << std::endl;

    std::cout << reconfiguration_count << std::endl;

    std::cout << average << std::endl;
    
    std::cout << response_time[static_cast<int>(std::ceil(n_tasks/20))] << std::endl;

    std::cout << deadline_miss_count << std::endl;

    std::cout << "--------------------------------------------------" << std::endl;

    std::string policy_name = (policy == 0) ? "Coyote" :
                            (policy == 1) ? "uShell" :
                            "Unknown";

    // File name + value to write
    std::vector<std::pair<std::string, double>> outputs = {
        {"sched_deadline.csv",  deadline_miss_count},
        {"sched_latency.csv",   durationMs_final},
        {"sched_reconfig.csv",  reconfiguration_count},
        {"sched_resp_95.csv",   response_time[static_cast<int>(std::ceil(n_tasks/20))]},
        {"sched_resp_avg.csv",  average}
    };

    for (const auto& [filename, value] : outputs) {
        std::ofstream file(filename, std::ios::app);

        if (file.is_open()) {
            file << n_tasks << ","
                << value << ","
                << policy_name << "\n";
        }
    }

    
    return (EXIT_SUCCESS);
}