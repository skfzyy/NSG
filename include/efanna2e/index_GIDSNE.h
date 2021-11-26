/**
 * @file index_GIDSNE.h
 * @author shenhangke
 * @brief this file is the head file for GIDSNE algo
 *        使用这个单元需要新增提供以下几个文件:
 *        1. 将原图中使用的ID按照从0开始的顺序进行编号,并提供映射文件
 *        2. 原图的图文件
 * @version 0.1
 * @date 2021-11-25
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __INDEX_GIDSNE_H__
#define __INDEX_GIDSNE_H__

#include "distance.h"
#include "index_nsg.h"
#include "neighbor.h"
#include "parameters.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#define logDebug(x) std::cout << (x) << std::endl

namespace QI
{

    using namespace efanna2e;

    class IndexGidsne : public IndexNSG
    {
      private:
        std::map<int, std::set<int>> _pathIndex;
        std::map<int, std::vector<int>> _originGraph;
        std::map<unsigned, unsigned> _originGraphIndex;

      public:
        explicit IndexGidsne(const size_t dimension, const size_t n,
                             Metric m, Index* initializer,
                             std::string originGraphPath,
                             std::string originGraphIndexPath)
            : IndexNSG(dimension, n, m, initializer)
        {
            this->loadOriginGraph(originGraphPath,
                                  originGraphIndexPath);
            this->buildOriginPathIndex();
        };
        std::map<int, std::set<int>> getPathIndex()
        {
            return this->_pathIndex;
        }

        /**
         * @brief
         *
         * @param filePath the origin graph path
         *                  saved as:
         *                  headNode \t tailNode \r
         * @param indexFilePath the origin graph index path
         *                      saved as:
         *                      index \t node \r
         * @author shenhangke
         * @date 2021-11-26
         */
        void loadOriginGraph(std::string filePath,
                             std::string indexFilePath);

        void buildOriginPathIndex();

        void loadOriginIndex(std::string filePath);

        virtual void sync_prune(unsigned q,
                                std::vector<Neighbor>& pool,
                                const Parameters& parameter,
                                boost::dynamic_bitset<>& flags,
                                SimpleNeighbor* cut_graph_) override;
    };
} // namespace QI

#endif