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

void QI::IndexGidsne::buildPathIndex(std::string filePath)
{
    std::ifstream file;
    file.open(filePath.c_str(), std::ios::in);
    if (!file.is_open())
    {
        return;
    }
    std::string strLine;

    //define a function which split string
    auto split=[](std::string str)->std::vector<int>{
        std::vector<int> result;
        str=str.substr(0,str.size());

        while (true)
        {
            size_t index=str.find('\t');
            if(index==str.npos){
                break;
            }
            str=str.substr(0,index);
            result.push_back(std::stoi(str));
        }
        if(result.size()==2){
            return result;
        }else{
            result.clear();
            return result;
        }
        
    };
    while (getline(file, strLine))
    {
        //delte comment
        if(strLine[0]=='#'){
            continue;
        }else{
            std::vector<int> splitResult=split(strLine);
            if(splitResult.size()!=2){
                continue;
            }else{
                if(this->_originGraph.find(splitResult[0])==this->_originGraph.end()){
                    this->_originGraph[splitResult[0]]=new std::vector<int>();
                }
                this->_originGraph[splitResult[0]]->push_back(splitResult[1]);
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
}