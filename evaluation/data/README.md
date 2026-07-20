# Evaluation data

CSVs and logs consumed by the plot scripts under `evaluation/scripts/`.
One subdir per paper figure / table — see [REPRODUCE.md](../../REPRODUCE.md)
for the script that produces and consumes each file.

| Dir                   | Paper output        | Contents                                              |
|-----------------------|---------------------|-------------------------------------------------------|
| `effectiveness_2/`    | Figure 3            | `effectiveness.csv` (paper + measured rows)           |
| `scalability_2/`      | Figure 4            | `scalability.csv`                                     |
| `reconfig_2/`         | Figure 6            | `sched_motive.csv`                                    |
| `e2e_6.1/`            | Figure 11           | `e2e.csv` (paper + measured rows)                     |
| `scheduling_6.2/`     | Figure 12           | `sched_{latency,reconfig,resp_avg,resp_95,deadline}.csv` |
| `deployment_6.3/`     | Figure 13           | `reconfig_{cap,pr}.log` + `reconfig_times.csv`        |
| `complexity_6.4/`     | Table 5             | `complexity_{baseline,ushell}_results.csv`            |
| `resource_usage_6.5/` | Figure 5 + Table 6  | `module_resource_usage.csv` + `util_{coyote,ushell,inter_4,inter_6,inter_8}.csv` |
