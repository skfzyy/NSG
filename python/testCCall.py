from Embed.NMF_1130 import read_originData
from Embed.NMF_1130 import EmbedGraph
from Embed.NMF_1130 import EMBED_SUFIX
from Embed.NMF_1130 import KNN_SUFIX
from Embed.NMF_1130 import GIDSNE_SUFIX
from Embed.NMF_1130 import QUERY_SUFIX
from Embed.NMF_1130 import SEARCH_RESULT_SUFIX
import sys
import ctypes
import os


if __name__=="__main__":
    if len(sys.argv)!=3:
        print("the argc is less than 2")
        exit()
    
    EmbedGraph(sys.argv[1],int(sys.argv[2]))
    dirname, filename = os.path.split(os.path.abspath(sys.argv[0])) 
    if os.path.exists(dirname+"/../tools/test_nndescent"):
        os.system(dirname+"/../tools/test_nndescent"+" "+sys.argv[1]+EMBED_SUFIX+" "+
                  sys.argv[1]+KNN_SUFIX+" "+"200 200 10 10 100")
        if os.path.exists(sys.argv[1]+KNN_SUFIX):
            dllHandle=ctypes.CDLL(dirname+"/../build/tests/libtest_gidsne_index.so")
            if dllHandle==None:
                print("cannot load dll")
                exit()
            else:
                dllHandle.BuildGidsneIndex(sys.argv[1],40,50,500)
                if not os.path.exists(sys.argv[1]+GIDSNE_SUFIX):
                    print("generate the gidsne file failed")
                    exit()
                else:
                    if os.path.exists(sys.argv[1]+QUERY_SUFIX):
                        os.system(dirname+"/../build/tests/test_nsg_search "+sys.argv[1]+EMBED_SUFIX+" "+
                                 sys.argv[1]+QUERY_SUFIX+" "+sys.argv[1]+GIDSNE_SUFIX+" "+
                                 "50 40 "+sys.argv[1]+SEARCH_RESULT_SUFIX)
                    else:
                        print("the query dataset is not exists")
    else:
        print("the tools for knn is not exists")
        exit()
    