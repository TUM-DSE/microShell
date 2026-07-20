# Evaluation Scripts

Scripts that produce every figure / table in the µShell paper. Each subdir
is paired 1:1 with `evaluation/data/<same>/` and `evaluation/plots/<same>/`.

> **Step-by-step reproduction instructions** — including bitgen, measurement,
> and plotting per section — live in the top-level
> [REPRODUCE.md](../../REPRODUCE.md). This README is just the index.

| Subdir              | Paper output                                  | Section |
|---------------------|-----------------------------------------------|---------|
| `modularity_2/`     | Figure 1 — Modularity of real-world apps      | §2      |
| `composability_2/`  | Figure 2 — Composability of Vitis Vision      | §2      |
| `effectiveness_2/`  | Figure 3 — Direct communication effectiveness | §2      |
| `scalability_2/`    | Figure 4 — Available FPGA resources / vFPGA   | §2      |
| `reconfig_2/`       | Figure 6 — Reconfiguration overhead           | §2      |
| `e2e_6.1/`          | Figure 11 — End-to-end performance            | §6.1    |
| `scheduling_6.2/`   | Figure 12 — Component-aware scheduling        | §6.2    |
| `deployment_6.3/`   | Figure 13 — Application deployment overheads  | §6.3    |
| `complexity_6.4/`   | Table 5  — Programmability (SLoC + CC)        | §6.4    |
| `resource_usage_6.5/` | Figure 5 + Table 6 — µShell resource usage  | §6.5    |
| `extract_util.tcl`  | Vivado helper (shared by `scalability_2/` + `resource_usage_6.5/`) | — |
