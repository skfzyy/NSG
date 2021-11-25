 #include "efanna2e/index_nsg.h"

#include <omp.h>
#include <bitset>
#include <chrono>
#include <cmath>
#include <boost/dynamic_bitset.hpp>

#include "efanna2e/exceptions.h"
#include "efanna2e/parameters.h"

namespace efanna2e {
#define _CONTROL_NUM 100
IndexNSG::IndexNSG(const size_t dimension, const size_t n, Metric m,
                   Index *initializer)
    : Index(dimension, n, m), initializer_{initializer} {}

IndexNSG::~IndexNSG() {}

void IndexNSG::Save(const char *filename) {
  std::ofstream out(filename, std::ios::binary | std::ios::out);
  assert(final_graph_.size() == nd_);

  out.write((char *)&width, sizeof(unsigned));
  out.write((char *)&ep_, sizeof(unsigned));
  for (unsigned i = 0; i < nd_; i++) {
    unsigned GK = (unsigned)final_graph_[i].size();
    out.write((char *)&GK, sizeof(unsigned));
    out.write((char *)final_graph_[i].data(), GK * sizeof(unsigned));
  }
  out.close();
}

void IndexNSG::Load(const char *filename) {
  std::ifstream in(filename, std::ios::binary);
  in.read((char *)&width, sizeof(unsigned));
  in.read((char *)&ep_, sizeof(unsigned));
  // width=100;
  unsigned cc = 0;
  while (!in.eof()) {
    unsigned k;
    in.read((char *)&k, sizeof(unsigned));
    if (in.eof()) break;
    cc += k;
    std::vector<unsigned> tmp(k);
    in.read((char *)tmp.data(), k * sizeof(unsigned));
    final_graph_.push_back(tmp);
  }
  cc /= nd_;
  // std::cout<<cc<<std::endl;
}

void IndexNSG::Load_nn_graph(const char *filename) {
  //读取上一步预构造的图,看一下这个数据格
  //第一个四字节是元素个数?
  //从第二个开始就是具体的元素
  //这个函数实际上是在读KNN，而不是原始数据
  std::ifstream in(filename, std::ios::binary);
  unsigned k; //k个邻居节点
  //一次读了四个字节?
  in.read((char *)&k, sizeof(unsigned));
  //基地址是结束位置,偏移量0
  in.seekg(0, std::ios::end);
  //返回字节编号?
  std::ios::pos_type ss = in.tellg();

  size_t fsize = (size_t)ss;
  //但是按照他这么整的话,实际上就没有M和Norm的值了
  size_t num = (unsigned)(fsize / (k + 1) / 4);
  in.seekg(0, std::ios::beg);

  final_graph_.resize(num);
  final_graph_.reserve(num);
  unsigned kk = (k + 3) / 4 * 4;
  for (size_t i = 0; i < num; i++) {
    in.seekg(4, std::ios::cur);
    final_graph_[i].resize(k);
    final_graph_[i].reserve(kk);
    in.read((char *)final_graph_[i].data(), k * sizeof(unsigned));
  }
  in.close();
}

void IndexNSG::get_neighbors(const float *query, const Parameters &parameter,
                             std::vector<Neighbor> &retset,
                             std::vector<Neighbor> &fullset) {
  unsigned L = parameter.Get<unsigned>("L");

  retset.resize(L + 1);
  std::vector<unsigned> init_ids(L);
  // initializer_->Search(query, nullptr, L, parameter, init_ids.data());

  boost::dynamic_bitset<> flags{nd_, 0};
  L = 0;
  for (unsigned i = 0; i < init_ids.size() && i < final_graph_[ep_].size(); i++) {
    init_ids[i] = final_graph_[ep_][i];
    flags[init_ids[i]] = true;
    L++;
  }
  while (L < init_ids.size()) {
    unsigned id = rand() % nd_;
    if (flags[id]) continue;
    init_ids[L] = id;
    L++;
    flags[id] = true;
  }

  L = 0;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_) continue;
    // std::cout<<id<<std::endl;
    float dist = distance_->compare(data_ + dimension_ * (size_t)id, query,
                                    (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = 1;
    L++;
  }

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
        if (flags[id]) continue;
        flags[id] = 1;

        float dist = distance_->compare(query, data_ + dimension_ * (size_t)id,
                                        (unsigned)dimension_);
        Neighbor nn(id, dist, true);
        fullset.push_back(nn);
        if (dist >= retset[L - 1].distance) continue;
        int r = InsertIntoPool(retset.data(), L, nn);

        if (L + 1 < retset.size()) ++L;
        if (r < nk) nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
}

void IndexNSG::get_neighbors(const float *query, const Parameters &parameter,
                             boost::dynamic_bitset<> &flags,
                             std::vector<Neighbor> &retset,
                             std::vector<Neighbor> &fullset) {
//这个函数主要针对单个节点，首先先盲猜是用来获取某个节点的邻居节点

//这里从link传入进来的参数是:
/*
  tmp--retset
  pool-fullset
*/

//根据readme中说的，L是个控制NSG质量的参数
  unsigned L = parameter.Get<unsigned>("L");
//tmp.resize(L+1)
  retset.resize(L + 1);

  std::vector<unsigned> init_ids(L);
  // initializer_->Search(query, nullptr, L, parameter, init_ids.data());

  //这里具体的L是控制的什么？
  L = 0;

  //这里实际上就是算法1
  //其中,L就是candidate pool的尺寸
  //伪代码4-8行
  //从导航节点出发,把当前导航节点的所有邻居节点都做一个标记

  //i的值小于L，同时小于导航节点邻居节点数（看是L大还是导航节点的邻居节点数多）
  for (unsigned i = 0; i < init_ids.size() && i < final_graph_[ep_].size(); i++) {
    //记录导航节点的前L个节点
    init_ids[i] = final_graph_[ep_][i];
    //在整个数据集中将导航节点的第i个邻居进行标记
    flags[init_ids[i]] = true;
    //L++是为了获取真实的邻居节点数
    L++;
  }

  //如果导航节点的邻居数比给定的L小，那么在数据集中随机选择一些节点把pool填充到L的尺寸
  while (L < init_ids.size()) {
    unsigned id = rand() % nd_;
    if (flags[id]) continue;
    init_ids[L] = id;
    L++;
    flags[id] = true;
  }

  L = 0;
  
  /*============================================================*/
  /*
    到此为止，上面的代码做的工作就遍历导航节点的邻居节点，然后使用init_ids数据结构
    来保存选中的节点ID,这个选中的节点集合主要包含以下两个方面：
    1. 导航节点的邻居节点
    2. 随机选择的节点，用来填充init_ids
  */
  /*============================================================*/

  //计算候选池中的所有节点到查询节点的距离
  /*============================================================*/
  /*
  计算候选池中所有节点到查询节点之间的距离，然后把相应的id和距离纪录在retset中
  */
  /*============================================================*/
  for (unsigned i = 0; i < init_ids.size(); i++) {
    //如果这里是在使用算法1的话，那么这里就是在遍历导航节点的邻居节点

    //获取候选池中的元素index
    unsigned id = init_ids[i];

    //判断id是不是合法
    if (id >= nd_) continue;
    // std::cout<<id<<std::endl;

    //比较第id个元素和查询元素之间的距离
    //距离字段记录的是到查询节点的距离
    //计算当前节点到查询节点的距离（欧式距离）
    float dist = distance_->compare(data_ + dimension_ * (size_t)id, query,
                                    (unsigned)dimension_);
    
    retset[i] = Neighbor(id, dist, true);
    fullset.push_back(retset[i]);
    // flags[id] = 1;
    L++;
  }

  //按照升序排列池子中的节点
  std::sort(retset.begin(), retset.begin() + L);

  int k = 0;

  //选择前k个节点？此时L的值是原来输入进来的L值
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      //获取池子里所有元素的id
      unsigned n = retset[k].id;

      //查找这个池子中每一个节点的邻居节点
      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];

        //如果当前的这个邻居的邻居已经在第一个池子里面了，就跳过这个节点
        if (flags[id]) continue;
        flags[id] = true;

        //计算当前节点到查询节点的距离
        float dist = distance_->compare(query, data_ + dimension_ * (size_t)id,
                                        (unsigned)dimension_);
        Neighbor nn(id, dist, true);
        //这里可以看出，fullset是用来记录所有路径，以及所经过路径的所有邻居节点
        //而retset是记录在遍历的过程中与查询节点距离最近的那些节点
        fullset.push_back(nn);

        //从这里可以看出，reset实际上就是一个堆（小顶堆）
        if (dist >= retset[L - 1].distance) continue;
        //这里直接通过调试看一下具体做了什么
        //难道这个是个堆？然后往里面加内容？
        int r = InsertIntoPool(retset.data(), L, nn);

        if (L + 1 < retset.size()) ++L;
        if (r < nk) nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
}

void IndexNSG::init_graph(const Parameters &parameters) {
  float *center = new float[dimension_];
  //计算中心
  for (unsigned j = 0; j < dimension_; j++) center[j] = 0;
  for (unsigned i = 0; i < nd_; i++) {
    for (unsigned j = 0; j < dimension_; j++) {
      center[j] += data_[i * dimension_ + j];
    }
  }
  for (unsigned j = 0; j < dimension_; j++) {
    center[j] /= nd_;
  }


  std::vector<Neighbor> tmp, pool;
  ep_ = rand() % nd_;  // random initialize navigating point
  get_neighbors(center, parameters, tmp, pool);
  ep_ = tmp[0].id;
}

/**
 * @brief 
 * 
 * @param q 查询节点
 * @param pool 使用算法1搜索时所有经过的点（包括邻居节点）
 * @param parameter 
 * @param flags 在算法1搜索时，探索过的点都会被标记为true
 * @param cut_graph_ 上一层传进来的，看名字也不知道有啥用
 *                  但是整体的大小是一个图中的每一个节点对应拥有一个range大小（R）的数组
 *                  数组中存储的是SimpleNeibor类型的节点
 * @author shenhangke
 * @date 2021-11-21
 */
void IndexNSG::sync_prune(unsigned q, std::vector<Neighbor> &pool,
                          const Parameters &parameter,
                          boost::dynamic_bitset<> &flags,
                          SimpleNeighbor *cut_graph_) {
  //遍历过的节点的flags都是true
  //这个函数应该就是应用MRNG算法来从候选池子中选择m个节点进行处理
  unsigned range = parameter.Get<unsigned>("R");
  unsigned maxc = parameter.Get<unsigned>("C");

  width = range;
  unsigned start = 0;

  for (unsigned nn = 0; nn < final_graph_[q].size(); nn++) {
    //遍历查询节点的邻居节点
    unsigned id = final_graph_[q][nn];
    //如果是已经在算法1中经历过的节点，就不再进行处理
    if (flags[id]) continue;

    //计算查询节点与他的邻居节点的距离
    float dist =
        distance_->compare(data_ + dimension_ * (size_t)q,
                           data_ + dimension_ * (size_t)id, (unsigned)dimension_);
    //把查询节点在KNN中的邻居节点加入到pool池子中来
    pool.push_back(Neighbor(id, dist, true));
  }

  //按照距离查询节点的大小顺序，把所有的节点进行排序
  std::sort(pool.begin(), pool.end());

  //这个应该是用来保存邻居节点结果的
  std::vector<Neighbor> result;

  //如果在pool中最近的一个节点就是查询节点的话，start++
  if (pool[start].id == q) start++;

  //首先把pool中距离查询节点最近的一个节点放入结果中
  result.push_back(pool[start]);

  //这里应该是一个迭代的过程，如果按照伪代码来说的话，maxc是选择的节点数m？
  while (result.size() < range && (++start) < pool.size() && start < maxc) {
    //每次选择当前距离查询节点最近的节点（排除已经处理过的节点）
    auto &p = pool[start];

    //遮挡？
    bool occlude = false;

    //这里应该就是条件
    /*
    简单来说，这里的条件要满足如下情况：
    对于当前在pool中距离查询节点最近的节点来说，要加入到结果集合，需要满足如下条件：
    1. 当前节点不能在结果集合中出现过
    2. 当前节点到查询节点的距离要小于等于当前节点到结果集合（已经被选中在查询节点最近邻节点集合中）中任意一个顶点的距离
      （这一点在论文中有提到，大概就是三角形两边打于第三边之类的）
    
    */
    for (unsigned t = 0; t < result.size(); t++) {
      //对当前结果集中的节点进行诶个处理
      if (p.id == result[t].id) {
        //如果在结果集合中找到当前距离查询节点最近的节点，那么就把标志位置true
        occlude = true;
        break;
      }
      
      //计算当前节点到结果集合中每个节点的距离
      float djk = distance_->compare(data_ + dimension_ * (size_t)result[t].id,
                                     data_ + dimension_ * (size_t)p.id,
                                     (unsigned)dimension_);
      
      if (djk < p.distance /* dik */) {
        occlude = true;
        break;
      }
    }


    if (!occlude) result.push_back(p);
  }


  SimpleNeighbor *des_pool = cut_graph_ + (size_t)q * (size_t)range;

  //这里就是在赋值了
  for (size_t t = 0; t < result.size(); t++) {
    des_pool[t].id = result[t].id;
    des_pool[t].distance = result[t].distance;
  }

  //如果最后选中的节点数小于range，那么就把截止的那个节点的距离设置为-1
  if (result.size() < range) {
    des_pool[result.size()].distance = -1;
  }
}

void IndexNSG::InterInsert(unsigned n, unsigned range,
                           std::vector<std::mutex> &locks,
                           SimpleNeighbor *cut_graph_) {
  SimpleNeighbor *src_pool = cut_graph_ + (size_t)n * (size_t)range;
  for (size_t i = 0; i < range; i++) {
    if (src_pool[i].distance == -1) break;

    SimpleNeighbor sn(n, src_pool[i].distance);
    size_t des = src_pool[i].id;
    SimpleNeighbor *des_pool = cut_graph_ + des * (size_t)range;

    std::vector<SimpleNeighbor> temp_pool;
    int dup = 0;
    {
      LockGuard guard(locks[des]);
      for (size_t j = 0; j < range; j++) {
        if (des_pool[j].distance == -1) break;
        if (n == des_pool[j].id) {
          dup = 1;
          break;
        }
        temp_pool.push_back(des_pool[j]);
      }
    }
    if (dup) continue;

    temp_pool.push_back(sn);
    if (temp_pool.size() > range) {
      std::vector<SimpleNeighbor> result;
      unsigned start = 0;
      std::sort(temp_pool.begin(), temp_pool.end());
      result.push_back(temp_pool[start]);
      while (result.size() < range && (++start) < temp_pool.size()) {
        auto &p = temp_pool[start];
        bool occlude = false;
        for (unsigned t = 0; t < result.size(); t++) {
          if (p.id == result[t].id) {
            occlude = true;
            break;
          }
          float djk = distance_->compare(data_ + dimension_ * (size_t)result[t].id,
                                         data_ + dimension_ * (size_t)p.id,
                                         (unsigned)dimension_);
          if (djk < p.distance /* dik */) {
            occlude = true;
            break;
          }
        }
        if (!occlude) result.push_back(p);
      }
      {
        LockGuard guard(locks[des]);
        for (unsigned t = 0; t < result.size(); t++) {
          des_pool[t] = result[t];
        }
      }
    } else {
      LockGuard guard(locks[des]);
      for (unsigned t = 0; t < range; t++) {
        if (des_pool[t].distance == -1) {
          des_pool[t] = sn;
          if (t + 1 < range) des_pool[t + 1].distance = -1;
          break;
        }
      }
    }
  }
}

//cut_graph_的数目是节点数*range,所以相当于range就是规定一个元素最多可以有多少个邻居节点?
void IndexNSG::Link(const Parameters &parameters, SimpleNeighbor *cut_graph_) {
  /*
  std::cout << " graph link" << std::endl;
  unsigned progress=0;
  unsigned percent = 100;
  unsigned step_size = nd_/percent;
  std::mutex progress_lock;
  */
  unsigned range = parameters.Get<unsigned>("R");
  std::vector<std::mutex> locks(nd_);

//指定下面的部分使用多线程
#pragma omp parallel
  {
    // unsigned cnt = 0;
    //猜测这里的pool是不是就是候选节点池
    std::vector<Neighbor> pool, tmp;
    //这里的flags应该是对应每一个S中的节点的检查标识位
    boost::dynamic_bitset<> flags{nd_, 0};
//每100个做一次动态任务分配
#pragma omp for schedule(dynamic, 100)
    for (unsigned n = 0; n < nd_; ++n) {
      //这里是针对每一个节点而言
      pool.clear();
      tmp.clear();
      flags.reset();
      //从参数看,第一个参数表示数据集中的第几个元素
      //这个函数是不是就是已经在找邻居节点了
      //寻找候选的邻居节点集合?
      //获取n节点的邻居节点(使用算法1)
      //在每一次参数传进来的时候都是被初始化为空
      /*============================================================*/
      /*
        这个函数的作用就是使用算法1来遍历从导航节点开始到查询节点路径上的所有的经过的点
        以及他们的邻居节点
      */
      /*============================================================*/
      get_neighbors(data_ + dimension_ * n, parameters, flags, tmp, pool);
      
      //用减肢策略减少某些不必要的边?
      //pool是所有从导航节点到查询节点经历过的边
      sync_prune(n, pool, parameters, flags, cut_graph_);
      /*
    cnt++;
    if(cnt % step_size == 0){
      LockGuard g(progress_lock);
      std::cout<<progress++ <<"/"<< percent << " completed" << std::endl;
      }
      */
    }
  }

#pragma omp for schedule(dynamic, 100)
  for (unsigned n = 0; n < nd_; ++n) {
    InterInsert(n, range, locks, cut_graph_);
  }
}

void IndexNSG::Build(size_t n, const float *data, const Parameters &parameters) {
  std::string nn_graph_path = parameters.Get<std::string>("nn_graph_path");
  unsigned range = parameters.Get<unsigned>("R");
  Load_nn_graph(nn_graph_path.c_str());
  data_ = data;
  //确定导航节点
  //赋值给_ep
  init_graph(parameters);

  //数组中的元素个数是*nd_*range
  //这个数组更像是一个[nd_][range]的数组(或者说就是) 
  SimpleNeighbor *cut_graph_ = new SimpleNeighbor[nd_ * (size_t)range];
  //这个link是干啥的?这里应该是做了什么操作?
  //所以这里能不能这样猜测:
  //整个函数实际上使用data_的数据,构造实际的连接结构,但是这边的输入实际上已经是
  //经过KNN的了啊- -
  /*
    1.如果是邻居节点的话,那么其SimpleNeighbor对应索引的元素的id或者距离会被设置
  */
 //从导航节点出发,进行搜索,将搜索结果放在cut_graph_中
  Link(parameters, cut_graph_);

  //load_nn_graph的时候就resize过一次,这里为什么要再resize一次?难道nd_在link的时候有改变?
  //经过检查,没有变化过
  final_graph_.resize(nd_);

  //遍历所有的节点
  //对应算法2的4行
  for (size_t i = 0; i < nd_; i++) {
    //第i个元素的候选池?    
    //难道这里就是要在实体集合中给节点i选择最多m个邻居?
    //这里range应该就是对应算法中的m
    //对应算法第6-8行
    SimpleNeighbor *pool = cut_graph_ + i * (size_t)range;
    //接下来应该是要在全局节点中选择range个节点
    unsigned pool_size = 0;
    //遍历所有元素的候选池
    //这个应该是在计算pool_size的大小
    //确定针对这个节点而言,pool的大小
    for (unsigned j = 0; j < range; j++) {
      //这个的关键是要确定什么时候这个distance会变成-1
      //在第i个元素的候选池中的第j个元素到i的距离是-1?(distance记录的是到谁的距离?)
      //这里应该是不可达的节点
      if (pool[j].distance == -1) break;
      pool_size = j;
    }
    //当前遍历到的元素对应的池子++
    pool_size++;

    //第i个节点的最邻近节点改成pool里面的节点
    //直接改变邻居结构构造
    final_graph_[i].resize(pool_size);
    for (unsigned j = 0; j < pool_size; j++) {
      final_graph_[i][j] = pool[j].id;
    }
  }

  tree_grow(parameters);

  unsigned max = 0, min = 1e6, avg = 0;
  for (size_t i = 0; i < nd_; i++) {
    auto size = final_graph_[i].size();
    max = max < size ? size : max;
    min = min > size ? size : min;
    avg += size;
  }
  avg /= 1.0 * nd_;
  printf("Degree Statistics: Max = %d, Min = %d, Avg = %d\n", max, min, avg);

  has_built = true;
}

void IndexNSG::Search(const float *query, const float *x, size_t K,
                      const Parameters &parameters, unsigned *indices) {
  const unsigned L = parameters.Get<unsigned>("L_search");
  data_ = x;
  std::vector<Neighbor> retset(L + 1);
  std::vector<unsigned> init_ids(L);
  boost::dynamic_bitset<> flags{nd_, 0};
  // std::mt19937 rng(rand());
  // GenRandom(rng, init_ids.data(), L, (unsigned) nd_);

  unsigned tmp_l = 0;
  for (; tmp_l < L && tmp_l < final_graph_[ep_].size(); tmp_l++) {
    init_ids[tmp_l] = final_graph_[ep_][tmp_l];
    flags[init_ids[tmp_l]] = true;
  }

  while (tmp_l < L) {
    unsigned id = rand() % nd_;
    if (flags[id]) continue;
    flags[id] = true;
    init_ids[tmp_l] = id;
    tmp_l++;
  }

  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    float dist =
        distance_->compare(data_ + dimension_ * id, query, (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = true;
  }

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
        if (flags[id]) continue;
        flags[id] = 1;
        float dist =
            distance_->compare(query, data_ + dimension_ * id, (unsigned)dimension_);
        if (dist >= retset[L - 1].distance) continue;
        Neighbor nn(id, dist, true);
        int r = InsertIntoPool(retset.data(), L, nn);

        if (r < nk) nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
  for (size_t i = 0; i < K; i++) {
    indices[i] = retset[i].id;
  }
}

void IndexNSG::SearchWithOptGraph(const float *query, size_t K,
                                  const Parameters &parameters, unsigned *indices) {
  unsigned L = parameters.Get<unsigned>("L_search");
  DistanceFastL2 *dist_fast = (DistanceFastL2 *)distance_;

  std::vector<Neighbor> retset(L + 1);
  std::vector<unsigned> init_ids(L);
  // std::mt19937 rng(rand());
  // GenRandom(rng, init_ids.data(), L, (unsigned) nd_);

  boost::dynamic_bitset<> flags{nd_, 0};
  unsigned tmp_l = 0;
  unsigned *neighbors = (unsigned *)(opt_graph_ + node_size * ep_ + data_len);
  unsigned MaxM_ep = *neighbors;
  neighbors++;

  for (; tmp_l < L && tmp_l < MaxM_ep; tmp_l++) {
    init_ids[tmp_l] = neighbors[tmp_l];
    flags[init_ids[tmp_l]] = true;
  }

  while (tmp_l < L) {
    unsigned id = rand() % nd_;
    if (flags[id]) continue;
    flags[id] = true;
    init_ids[tmp_l] = id;
    tmp_l++;
  }

  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_) continue;
    _mm_prefetch(opt_graph_ + node_size * id, _MM_HINT_T0);
  }
  L = 0;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_) continue;
    float *x = (float *)(opt_graph_ + node_size * id);
    float norm_x = *x;
    x++;
    float dist = dist_fast->compare(x, query, norm_x, (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    flags[id] = true;
    L++;
  }
  // std::cout<<L<<std::endl;

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      _mm_prefetch(opt_graph_ + node_size * n + data_len, _MM_HINT_T0);
      unsigned *neighbors = (unsigned *)(opt_graph_ + node_size * n + data_len);
      unsigned MaxM = *neighbors;
      neighbors++;
      for (unsigned m = 0; m < MaxM; ++m)
        _mm_prefetch(opt_graph_ + node_size * neighbors[m], _MM_HINT_T0);
      for (unsigned m = 0; m < MaxM; ++m) {
        unsigned id = neighbors[m];
        if (flags[id]) continue;
        flags[id] = 1;
        float *data = (float *)(opt_graph_ + node_size * id);
        float norm = *data;
        data++;
        float dist = dist_fast->compare(query, data, norm, (unsigned)dimension_);
        if (dist >= retset[L - 1].distance) continue;
        Neighbor nn(id, dist, true);
        int r = InsertIntoPool(retset.data(), L, nn);

        // if(L+1 < retset.size()) ++L;
        if (r < nk) nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
  for (size_t i = 0; i < K; i++) {
    indices[i] = retset[i].id;
  }
}

void IndexNSG::OptimizeGraph(float *data) {  // use after build or load

  data_ = data;
  data_len = (dimension_ + 1) * sizeof(float);
  neighbor_len = (width + 1) * sizeof(unsigned);
  node_size = data_len + neighbor_len;
  opt_graph_ = (char *)malloc(node_size * nd_);
  DistanceFastL2 *dist_fast = (DistanceFastL2 *)distance_;
  for (unsigned i = 0; i < nd_; i++) {
    char *cur_node_offset = opt_graph_ + i * node_size;
    float cur_norm = dist_fast->norm(data_ + i * dimension_, dimension_);
    std::memcpy(cur_node_offset, &cur_norm, sizeof(float));
    std::memcpy(cur_node_offset + sizeof(float), data_ + i * dimension_,
                data_len - sizeof(float));

    cur_node_offset += data_len;
    unsigned k = final_graph_[i].size();
    std::memcpy(cur_node_offset, &k, sizeof(unsigned));
    std::memcpy(cur_node_offset + sizeof(unsigned), final_graph_[i].data(),
                k * sizeof(unsigned));
    std::vector<unsigned>().swap(final_graph_[i]);
  }
  CompactGraph().swap(final_graph_);
}

void IndexNSG::DFS(boost::dynamic_bitset<> &flag, unsigned root, unsigned &cnt) {
  unsigned tmp = root;
  std::stack<unsigned> s;
  s.push(root);
  if (!flag[root]) cnt++;
  flag[root] = true;
  while (!s.empty()) {
    unsigned next = nd_ + 1;
    for (unsigned i = 0; i < final_graph_[tmp].size(); i++) {
      if (flag[final_graph_[tmp][i]] == false) {
        next = final_graph_[tmp][i];
        break;
      }
    }
    // std::cout << next <<":"<<cnt <<":"<<tmp <<":"<<s.size()<< '\n';
    if (next == (nd_ + 1)) {
      s.pop();
      if (s.empty()) break;
      tmp = s.top();
      continue;
    }
    tmp = next;
    flag[tmp] = true;
    s.push(tmp);
    cnt++;
  }
}

void IndexNSG::findroot(boost::dynamic_bitset<> &flag, unsigned &root,
                        const Parameters &parameter) {
  unsigned id = nd_;
  for (unsigned i = 0; i < nd_; i++) {
    if (flag[i] == false) {
      id = i;
      break;
    }
  }

  if (id == nd_) return;  // No Unlinked Node

  std::vector<Neighbor> tmp, pool;
  get_neighbors(data_ + dimension_ * id, parameter, tmp, pool);
  std::sort(pool.begin(), pool.end());

  unsigned found = 0;
  for (unsigned i = 0; i < pool.size(); i++) {
    if (flag[pool[i].id]) {
      // std::cout << pool[i].id << '\n';
      root = pool[i].id;
      found = 1;
      break;
    }
  }
  if (found == 0) {
    while (true) {
      unsigned rid = rand() % nd_;
      if (flag[rid]) {
        root = rid;
        break;
      }
    }
  }
  final_graph_[root].push_back(id);
}
void IndexNSG::tree_grow(const Parameters &parameter) {
  unsigned root = ep_;
  boost::dynamic_bitset<> flags{nd_, 0};
  unsigned unlinked_cnt = 0;
  while (unlinked_cnt < nd_) {
    DFS(flags, root, unlinked_cnt);
    // std::cout << unlinked_cnt << '\n';
    if (unlinked_cnt >= nd_) break;
    findroot(flags, root, parameter);
    // std::cout << "new root"<<":"<<root << '\n';
  }
  for (size_t i = 0; i < nd_; ++i) {
    if (final_graph_[i].size() > width) {
      width = final_graph_[i].size();
    }
  }
}
}
