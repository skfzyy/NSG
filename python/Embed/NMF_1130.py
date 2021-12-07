import csv
from matplotlib import pyplot as plt
import networkx as nx
import numpy as np
from sklearn.decomposition import NMF
import struct
import typing
import os
import __init__
from FileUtil.IndexOriginFile import IndexOriginNode
from FileUtil.BitWrite import WriteVector


GR_INDEX_SUFIX = "_index"
KNN_SUFIX = "_knn"
EMBED_SUFIX = "_embed"
GIDSNE_SUFIX = "_gidsne"


def read_csv(csv_file):
    data = []
    with open(csv_file, 'r', encoding='utf-8-sig') as f:
        # create a list of rows in the CSV file
        f_csv = csv.reader(f)
        for row in f_csv:
            data.append(tuple(row))

    return data


def read_originData(data_file):
    data = []
    nodeDegree = {}
    with open(data_file, "r", encoding="utf8") as file:
        # 这种方法能够读取的文件应当小于运行该程序的机器的内存-1g至少
        # 留给python进程
        lines = file.readlines()
        for line in lines:
            line = line.strip()
            if line.find("#") != -1:
                line = line[0:line.find("#")]
            elements = line.split("\t")
            if len(elements) == 2:
                data.append((int(elements[0]), int(elements[1])))
                if int(elements[0]) not in nodeDegree:
                    nodeDegree[int(elements[0])] = 1
                else:
                    nodeDegree[int(elements[0])] += 1
                if int(elements[1]) not in nodeDegree:
                    nodeDegree[int(elements[1])] = 1
                else:
                    nodeDegree[int(elements[1])] += 1
    return data, nodeDegree


def EmbedGraph(originalFile, dimension):
    edges, nodeDegree = read_originData(originalFile)
    nodeIndex, indexNode = IndexOriginNode(
        originalFile, originalFile+GR_INDEX_SUFIX)
    indexEdges = []
    for head, tail in edges:
        indexEdges.append((nodeIndex[head], nodeIndex[tail]))
    G = nx.DiGraph()
    G.add_edges_from(indexEdges)
    nodes_num = G.number_of_nodes()
    edges_num = G.number_of_edges()

    # A = np.array(nx.adjacency_matrix(G).todense())
    A = np.zeros((nodes_num, nodes_num))
    F = nx.from_numpy_matrix(A, create_using=nx.DiGraph)

    # |D| = number of nodes
    D = len(A[0, :])

    #  count the number of #v_i and #v_j
    # num_v_i = {i: (A[i, :] != 0).sum(0) + (A[:, i] != 0).sum(0)
    #            for i in range(D)}

    # for i in range(D):
    #     for j in range(D):
    #         if i != j and A[i, j] != 0:
    #             A[i, j] = num_v_i[i] * num_v_i[j]
    for head, tail in indexEdges:
        A[head, tail] = nodeDegree[indexNode[head]]*nodeDegree[indexNode[tail]]

    model = NMF(n_components=dimension, init='random', random_state=0)
    U = model.fit_transform(A)
    H = model.components_
    H = H.T
    data = []
    for vector in H:
        data.append((dimension, vector))
    WriteVector(data, originalFile+EMBED_SUFIX)


if __name__ == "__main__":
    EmbedGraph("/root/data/wiki-Vote.txt", 20)
