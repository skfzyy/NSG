/**
 * @file index_GIDSNE.h
 * @author shenhangke
 * @brief this file is the head file for GIDSNE algo
 * @version 0.1
 * @date 2021-11-25
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __INDEX_GIDSNE_H__
#define __INDEX_GIDSNE_H__

#include "index_nsg.h"
#include "neighbor.h"
#include "parameters.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "distance.h"

#define logDebug(x) std::cout << (x) << std::endl

namespace QI
{

    using namespace efanna2e;

    class IndexGidsne : public IndexNSG
    {
      private:
        std::map<int, std::vector<int>> _pathIndex;
        std::map<int,std::vector<int>*> _originGraph;

      public:
        explicit IndexGidsne():IndexNSG(0,0,(Metric)0,NULL){};
        std::map<int, std::vector<int>> getPathIndex()
        {
            return this->_pathIndex;
        }

        /**
         * @brief DFS to search the knn,which want to get the
         *        connection relation between the nodes
         *        the result will be built in _pathIndex
         *
         *        this function is called after load Load_nn_graph
         *
         * @author shenhangke
         * @date 2021-11-25
         */
        void buildPathIndex(std::string filePath);

        virtual void sync_prune(unsigned q,
                                std::vector<Neighbor>& pool,
                                const Parameters& parameter,
                                boost::dynamic_bitset<>& flags,
                                SimpleNeighbor* cut_graph_) override;
    };
} // namespace QI

#endif