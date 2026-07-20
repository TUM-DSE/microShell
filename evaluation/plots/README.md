# Evaluation plots

Rendered figures, one subdir per paper figure / table. The plot scripts
under `evaluation/scripts/<same-name>/` write here. See
[REPRODUCE.md](../../REPRODUCE.md) for the command that regenerates each.

| Dir                   | Paper output        | Files                                              |
|-----------------------|---------------------|----------------------------------------------------|
| `modularity_2/`       | Figure 1            | `application_modularity_analysis.{pdf,png}`        |
| `composability_2/`    | Figure 2            | `correlation_heatmap.pdf`                          |
| `effectiveness_2/`    | Figure 3            | `direct_comm_effectiveness.pdf`                    |
| `scalability_2/`      | Figure 4            | `scalability_analysis.{pdf,png}`                   |
| `reconfig_2/`         | Figure 6            | `reconf_analysis.pdf`                              |
| `e2e_6.1/`            | Figure 11           | `e2e.{pdf,png}`                                    |
| `scheduling_6.2/`     | Figure 12           | `sched.{pdf,png}`                                  |
| `deployment_6.3/`     | Figure 13           | `reconfig_overhead.{pdf,png}`                      |
| `complexity_6.4/`     | Table 5             | `complexity.{pdf,png}`                             |
| `resource_usage_6.5/` | Figure 5 + Table 6  | `resource_usage.{pdf,png}` (Table 6 is stdout)     |
