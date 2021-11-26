/**
 * @file index_GIDSNE.cpp
 * @author shenhangke
 * @brief
 * @version 0.1
 * @date 2021-11-25
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <efanna2e/index_GIDSNE.h>
#include <fstream>
#include <queue>

void QI::IndexGidsne::loadOriginGraph(std::string filePath,
                                      std::string indexFilePath)
{
    std::ifstream file;
    std::ifstream indexFile;
    file.open(filePath.c_str(), std::ios::in);
    indexFile.open(filePath.c_str(), std::ios::in);
    if (!file.is_open() || !indexFile.is_open())
    {
        return;
    }
    std::string strLine;

    // define a function which split string
    auto split = [](std::string str) -> std::vector<int> {
        std::vector<int> result;
        str = str.substr(0, str.size());

        size_t index = str.find('\t');
        if (index == str.npos)
        {
            return result;
        }
        // str=str.substr(0,index);
        result.push_back(std::stoi(str.substr(0, index)));
        result.push_back(
            std::stoi(str.substr(index + 1, str.size())));

        if (result.size() == 2)
        {
            return result;
        }
        else
        {
            result.clear();
            return result;
        }
    };

    while (getline(file, strLine))
    {
        // delte comment
        if (strLine[0] == '#')
        {
            continue;
        }
        else
        {
            std::vector<int> splitResult = split(strLine);
            if (splitResult.size() != 2)
            {
                continue;
            }
            else
            {
                this->_originGraph[splitResult[0]].push_back(
                    splitResult[1]);
            }
        }
    }

    while (getline(file, strLine))
    {
        // delte comment
        if (strLine[0] == '#')
        {
            continue;
        }
        else
        {
            std::vector<int> splitResult = split(strLine);
            if (splitResult.size() != 2)
            {
                continue;
            }
            else
            {
                this->_originGraphIndex[splitResult[0]] =
                    splitResult[1];
            }
        }
    }
}

void QI::IndexGidsne::sync_prune(unsigned q,
                                 std::vector<Neighbor>& pool,
                                 const Parameters& parameter,
                                 boost::dynamic_bitset<>& flags,
                                 SimpleNeighbor* cut_graph_)
{
    //遍历过的节点的flags都是true
    //这个函数应该就是应用MRNG算法来从候选池子中选择m个节点进行处理
    unsigned range = parameter.Get<unsigned>("R");
    unsigned maxc = parameter.Get<unsigned>("C");

    width = range;
    unsigned start = 0;

    for (unsigned nn = 0; nn < final_graph_[q].size(); nn++)
    {
        //遍历查询节点的邻居节点
        unsigned id = final_graph_[q][nn];
        //如果是已经在算法1中经历过的节点，就不再进行处理
        if (flags[id])
            continue;

        //计算查询节点与他的邻居节点的距离
        float dist = distance_->compare(
            data_ + dimension_ * (size_t)q,
            data_ + dimension_ * (size_t)id, (unsigned)dimension_);
        //把查询节点在KNN中的邻居节点加入到pool池子中来
        pool.push_back(Neighbor(id, dist, true));
    }

    //按照距离查询节点的大小顺序，把所有的节点进行排序
    std::sort(pool.begin(), pool.end());

    //这个应该是用来保存邻居节点结果的
    std::vector<Neighbor> result;

    //如果在pool中最近的一个节点就是查询节点的话，start++
    if (pool[start].id == q)
        start++;

    //首先把pool中距离查询节点最近的一个节点放入结果中
    result.push_back(pool[start]);

    //这里应该是一个迭代的过程，如果按照伪代码来说的话，maxc是选择的节点数m？
    while (result.size() < range && (++start) < pool.size() &&
           start < maxc)
    {
        //每次选择当前距离查询节点最近的节点（排除已经处理过的节点）
        auto& p = pool[start];

        //遮挡？
        bool occlude = false;

        //这里应该就是条件
        /*
        简单来说，这里的条件要满足如下情况：
        对于当前在pool中距离查询节点最近的节点来说，要加入到结果集合，需要满足如下条件：
        1. 当前节点不能在结果集合中出现过
        2.
        当前节点到查询节点的距离要小于等于当前节点到结果集合（已经被选中在查询节点最近邻节点集合中）中任意一个顶点的距离
          （这一点在论文中有提到，大概就是三角形两边打于第三边之类的）

        */
        for (unsigned t = 0; t < result.size(); t++)
        {
            //对当前结果集中的节点进行诶个处理
            if (p.id == result[t].id)
            {
                //如果在结果集合中找到当前距离查询节点最近的节点，那么就把标志位置true
                occlude = true;
                break;
            }

            //计算当前节点到结果集合中每个节点的距离
            float djk = distance_->compare(
                data_ + dimension_ * (size_t)result[t].id,
                data_ + dimension_ * (size_t)p.id,
                (unsigned)dimension_);

            if (djk < p.distance)
            {
                occlude = true;
                break;
            }
        }

        bool isExistsPath=false;
        int realp=this->_originGraphIndex[p.id];
        int realq=this->_originGraphIndex[q];
        if(this->_pathIndex[realq].count(realp)==1){
            //no path
            isExistsPath=true;
        }


        if (!occlude&&isExistsPath)
            result.push_back(p);
    }

    SimpleNeighbor* des_pool = cut_graph_ + (size_t)q * (size_t)range;

    //这里就是在赋值了
    for (size_t t = 0; t < result.size(); t++)
    {
        des_pool[t].id = result[t].id;
        des_pool[t].distance = result[t].distance;
    }

    //如果最后选中的节点数小于range，那么就把截止的那个节点的距离设置为-1
    if (result.size() < range)
    {
        des_pool[result.size()].distance = -1;
    }
}

void QI::IndexGidsne::buildOriginPathIndex()
{
    // BFS to search
    std::queue<int> queueForBFS;

    auto index = [&](int query) {
        // clear queueForBFS
        queueForBFS = std::queue<int>();

        this->_pathIndex[query] = std::set<int>();

        // init
        std::vector<int> neighbor = this->_originGraph[query];
        for (auto node : neighbor)
        {
            queueForBFS.push(node);
        }

        while (!queueForBFS.empty())
        {
            int currentNode = queueForBFS.front();
            queueForBFS.pop();
            if (this->_pathIndex[query].count(currentNode) == 1)
            {
                continue;
            }
            else
            {
                this->_pathIndex[query].insert(currentNode);
                for (auto nei : this->_originGraph[currentNode])
                {
                    queueForBFS.push(nei);
                }
            }
        }
    };

    for (auto const& node : this->_originGraph)
    {
        index(node.first);
    }
}