#!/bin/bash

echo "Number of tasks,Deadline misses,Policy" > sched_deadline.csv
echo "Number of tasks,Latency[ms],Policy" > sched_latency.csv
echo "Number of tasks,Reconfig count,Policy" > sched_reconfig.csv
echo "Number of tasks,Latency[ms],Policy" > sched_resp_95.csv
echo "Number of tasks,Latency[ms],Policy" > sched_resp_avg.csv

echo "CSV files created successfully."