#from numpy.core.numeric import load
from Embed.NMF_1130 import read_originData
from Embed.NMF_1130 import EmbedGraph
from Embed.NMF_1130 import EMBED_SUFIX
from Embed.NMF_1130 import KNN_SUFIX
from Embed.NMF_1130 import GIDSNE_SUFIX
from Embed.NMF_1130 import QUERY_SUFIX
from Embed.NMF_1130 import SEARCH_RESULT_SUFIX
from Embed.NMF_1130 import NSG_SUFIX
from Embed.NMF_1130 import FANNA_SUFIX

from FileUtil.BitWrite import generateQueryVec
from FileUtil.BitWrite import loadQueryResult
from FileUtil.BitWrite import WriteVector
from FileUtil.BitWrite import loadFvecsData

import sys
import ctypes
import os

import platform

GroudTruthData_sufix = "_grd"


def outSplit(title, iscontain=True):
    if(iscontain):
        print("==========="+title+"=====================")
    else:
        print("==========="+"=====================")


if __name__ == "__main__":

    """
    参数：
    1. 原始文件路径
    2. 维度
    3. [optional] getgroudtruth
       [optional] 查询节点编号，以逗号隔开
    4. 1:表示需要嵌入
       0:不需要嵌入
    5. 表示所要使用的方法,其中:
    可选值为:
    "knn","gidsne","nsg"
    """

    # wiki-vo...txt 20 1,2,5,7,9,....
    data = None
    if sys.argv[4] != "0":
        data = EmbedGraph(sys.argv[1], int(sys.argv[2]))
    else:
        data = loadFvecsData(sys.argv[1]+EMBED_SUFIX)
    if sys.argv[3].lower() == "getgroudtruth":
        # get the groudtruth
        # 使用1/5的数据作为groundTruth
        grdData = [data[i] for i in range(int(len(data)/5))]
        WriteVector(grdData, sys.argv[1]+GroudTruthData_sufix+EMBED_SUFIX)
        exit(0)
    # 第四个参数使用逗号进行分隔，主要输入需要查询的查询节点编号，以index结果为标准
    if sys.argv[3] != "null":
        queryIndexsInt = None
        if sys.argv[3].find(",") != -1:
            queryIndexs = sys.argv[3].split(",")
            queryIndexsInt = [int(queryIndexs[i])
                              for i in range(len(queryIndexs))]
        elif sys.argv[3].find("-") != -1:
            ranges = sys.argv[3].split("-")
            if(len(ranges) == 2):
                queryIndexsInt = range(int(ranges[0]), int(ranges[1]))
            else:
                print("the argument is error")
                exit(-1)
        else:
            print("the argument is error")
            exit(-1)

        queryIndexsInt = [int(queryIndexs[i]) for i in range(len(queryIndexs))]
        # 生成查询数据集
        # outSplit("query data")
        generateQueryVec(data, queryIndexsInt, sys.argv[1]+QUERY_SUFIX)
        # outSplit("query data", iscontain=False)
    dirname, filename = os.path.split(os.path.abspath(sys.argv[0]))
    if os.path.exists(dirname+"/../tools/test_nndescent") or os.path.exists(dirname+"/../tools/test_nndescent_mac"):
        # 生成knn索引
        outSplit("knn generate")
        if platform.system().lower() == "darwin":
            os.system(dirname+"/../tools/test_nndescent_mac"+" "+sys.argv[1]+EMBED_SUFIX+" " +
                      sys.argv[1]+KNN_SUFIX+" "+"200 200 10 10 100")
        else:
            os.system(dirname+"/../tools/test_nndescent"+" "+sys.argv[1]+EMBED_SUFIX+" " +
                      sys.argv[1]+KNN_SUFIX+" "+"200 200 10 10 100")
        outSplit("knn generate", False)

        if os.path.exists(sys.argv[1]+KNN_SUFIX):
            dllHandle = None
            if platform.system().lower() == "darwin":
                dllHandle = ctypes.CDLL(
                    dirname+"/../build/tests/libtest_gidsne_index.dylib")
            else:
                dllHandle = ctypes.CDLL(
                    dirname+"/../build/tests/libtest_gidsne_index.so")
            if dllHandle == None:
                print("cannot load dll")
                exit()
            else:
                generateIndexFileName = ""
                if sys.argv[5] == "gidsne":
                    dllHandle.BuildGidsneIndex(sys.argv[1], 40, 50, 500)
                    generateIndexFileName = sys.argv[1]+GIDSNE_SUFIX
                elif sys.argv[5] == "knn":
                    # the file is exists
                    from FileUtil.BitWrite import writeKNNData
                    from Embed.NMF_1130 import KNN_SEARCH_FILE
                    writeKNNData(sys.argv[1]+KNN_SUFIX)
                    generateIndexFileName = sys.argv[1]+KNN_SUFIX+KNN_SEARCH_FILE
                elif sys.argv[5] == "nsg":
                    # need to generate the nsg files
                    generateIndexFileName = sys.argv[1]+NSG_SUFIX
                    os.system(dirname+"/../build/tests/test_nsg_index "+sys.argv[1]+EMBED_SUFIX+
                              " "+sys.argv[1]+KNN_SUFIX+" "+"40 50 500 "+generateIndexFileName)
                elif sys.argv[5]=="fanng":
                    generateIndexFileName = sys.argv[1]+FANNA_SUFIX
                    os.system(dirname+"/../tools/fanng "+sys.argv[1]+EMBED_SUFIX)
                elif sys.argv[5]=="nssg":
                    from Embed.NMF_1130 import NSSG_SUFIX
                    generateIndexFileName = sys.argv[1]+NSSG_SUFIX
                    os.system(dirname+"/../tools/nssg_index "+sys.argv[1]+EMBED_SUFIX)
                    os.system(dirname+"/../tools/nssg_index "+sys.argv[1]+EMBED_SUFIX+
                              " "+sys.argv[1]+KNN_SUFIX+" "+"100 50 60 "+generateIndexFileName)
                else:
                    print("undefined method")
                    exit(-1)
                if not os.path.exists(generateIndexFileName):
                    print("generate the gidsne file failed")
                    exit()
                else:
                    if os.path.exists(sys.argv[1]+QUERY_SUFIX):
                        if sys.argv[5]!="nssg":
                            os.system(dirname+"/../build/tests/test_nsg_search "+sys.argv[1]+EMBED_SUFIX+" " +
                                    sys.argv[1]+QUERY_SUFIX+" "+generateIndexFileName+" " +
                                    "50 40 "+sys.argv[1]+SEARCH_RESULT_SUFIX)
                        else:
                            os.system(dirname+"/../tools/nssg_search "+sys.argv[1]+EMBED_SUFIX+" " +
                                    sys.argv[1]+QUERY_SUFIX+" "+generateIndexFileName+" " +
                                    "50 40 "+sys.argv[1]+SEARCH_RESULT_SUFIX)
                        print("has success get the search result")
                    else:
                        print("the query dataset is not exists")
    else:
        print("the tools for knn is not exists")
        exit()
