#ifndef EFANNA2E_INDEX_NSG_H
#define EFANNA2E_INDEX_NSG_H

#include "util.h"
#include "parameters.h"
#include "neighbor.h"
#include "index.h"
#include <cassert>
#include <unordered_map>
#include <string>
#include <sstream>
#include <boost/dynamic_bitset.hpp>
#include <stack>

namespace efanna2e {

class IndexNSG : public Index {
 public:
  explicit IndexNSG(const size_t dimension, const size_t n, Metric m, Index *initializer);


  virtual ~IndexNSG();

  virtual void Save(const char *filename)override;
  virtual void Load(const char *filename)override;


  virtual void Build(size_t n, const float *data, const Parameters &parameters) override;

  virtual void Search(
      const float *query,
      const float *x,
      size_t k,
      const Parameters &parameters,
      unsigned *indices) override;
  void SearchWithOptGraph(
      const float *query,
      size_t K,
      const Parameters &parameters,
      unsigned *indices);
  void OptimizeGraph(float* data);

  protected:
    typedef std::vector<std::vector<unsigned > > CompactGraph;
    typedef std::vector<SimpleNeighbors > LockGraph;
    typedef std::vector<nhood> KNNGraph;

    //这个图应该是使用邻接矩阵来保存的KNNG
    CompactGraph final_graph_;

    Index *initializer_;
    void init_graph(const Parameters &parameters);
    
    void get_neighbors(
        const float *query,
        const Parameters &parameter,
        std::vector<Neighbor> &retset,
        std::vector<Neighbor> &fullset);
    /**
     * @brief 这个函数的本质就是算法1，通过不断的找到距离查询节点更近的节点，来获取路径以及相关信息
     * 
     * @param query query node(vector)
     * @param parameter 
     * @param flags (save whether is neighbors?)
     * @param retset 记录在使用算法1遍历的过程中距离查询节点最近的节点（这个集合的数目是L）
     * @param fullset 记录在使用算法1遍历的过程中所有经过的节点以及他们的邻居节点
     * @author shenhangke
     * @date 2021-11-19
     */
    void get_neighbors(
        const float *query,
        const Parameters &parameter,
        boost::dynamic_bitset<>& flags,
        std::vector<Neighbor> &retset,
        std::vector<Neighbor> &fullset);
    //void add_cnn(unsigned des, Neighbor p, unsigned range, LockGraph& cut_graph_);
    void InterInsert(unsigned n, unsigned range, std::vector<std::mutex>& locks, SimpleNeighbor* cut_graph_);
    void sync_prune(unsigned q, std::vector<Neighbor>& pool, const Parameters &parameter, boost::dynamic_bitset<>& flags, SimpleNeighbor* cut_graph_);
    void Link(const Parameters &parameters, SimpleNeighbor* cut_graph_);
    /**
     * @brief 读取预定义的图
     * 输出图的数据格式:
     *  源数据格式为fvecs
     * 保存图的数据形式:(邻接矩阵,保存在final_graph_里面)
     *  以行序优先的方式保存数据
     *  + 每一行的前4个bytes(一个int)数据保存k的值(前k个邻居)
     *  + 接下来的4字节存M????M是啥子
     *  + 在接下来存这个点的范式
     *  + 接下来用k*sizeof(int)存索引
     * @param filename 
     * @author shenhangke
     * @date 2021-11-18
     */
    void Load_nn_graph(const char *filename);
    void tree_grow(const Parameters &parameter);
    void DFS(boost::dynamic_bitset<> &flag, unsigned root, unsigned &cnt);
    void findroot(boost::dynamic_bitset<> &flag, unsigned &root, const Parameters &parameter);


  private:
    unsigned width;

    /**
     * @brief 表示导航节点
     * 
     * @author shenhangke
     * @date 2021-11-19
     */
    unsigned ep_;
    std::vector<std::mutex> locks;
    char* opt_graph_;
    size_t node_size;
    size_t data_len;
    size_t neighbor_len;
    KNNGraph nnd_graph;
};
}

#endif //EFANNA2E_INDEX_NSG_H
