# CAACO_for_TSP
协同注意力蚁群优化算法求解大规模旅行商问题
Collaborative Attention Ant Colony Optimization for Large-scale Traveling Salesman Problem

## 📖 项目简介 | Introduction
### 中文
本项目实现 **CAACO（协同注意力蚁群优化算法）**，面向大规模TSP（旅行商问题）。
算法在MMAS（最大最小蚁群系统）基础上引入多项创新改进：
1. **轻量化图自注意力机制**：计算城市相对方向向量的余弦相似度，为蚂蚁提供全局拓扑启发信息，弥补传统蚁群算法仅依赖局部欧氏距离带来的视野局限问题。
2. **随机错位分治框架**：对TSP完整回路做环形移位，消除分块优化产生的边界盲区；采用多线程并行对各个子块执行MMAS求解，提升大规模场景的优化效率。
3. **子块精英注入策略**：输入子路径经过2-opt局部搜索精修后，作为子迭代的初始精英解，防止子路径优化过程退化。
4. **阶梯式迭代局部搜索ILS**，搭配双桥4-opt扰动与非劣容忍接受准则，帮助算法有效跳出局部最优。

算法使用 **TSPLIB标准基准数据集** 完成性能验证。

### English
This repository implements **CAACO (Collaborative Attention Ant Colony Optimization)** for large-scale Traveling Salesman Problem (TSP).
Based on MAX-MIN Ant System (MMAS), several innovative improvements are proposed:
1. **Lightweight graph self-attention mechanism**. It calculates the cosine similarity of cities' relative direction vectors to provide global topological heuristic information, solving the short-sight limitation of traditional ACO which only uses local Euclidean distance.
2. **Random-shift divide-and-conquer framework**. Cyclically shift the whole TSP tour to eliminate boundary blind spots caused by block optimization. Multi-thread parallel MMAS is used for sub-block optimization to accelerate solving for large-scale instances.
3. **Sub-block elite injection strategy**. The input subpath refined by 2-opt local search is set as the initial elite solution of sub-iteration to avoid subpath optimization degradation.
4. **Staged Iterated Local Search (ILS)** with double-bridge 4-opt perturbation and non-inferior tolerance acceptance criterion, helping the algorithm escape from local optima effectively.

The algorithm is validated on standard TSPLIB benchmark datasets.

### 依赖 Requirements
- C++17
- OpenMP（用于多线程并行）
