
def IndexOriginNode(inFilePath:str,outFilePath):
    """[summary]

    Args:
        inFilePath ([type]): [description]
        outFilePath ([type]): [description]
    """    
    allNodes=set()
    nodeIndex={}
    indexNode={}
    
    with open(inFilePath,"r") as file:
        lines=file.readlines()
        for line in lines:
            if line[0]=="#":
                continue
            line=line.strip()
            result=line.split("\t")
            if(len(result)==2):
                allNodes.add(int(result[0]))
                allNodes.add(int(result[1]))
    sorted(allNodes)
    with open(outFilePath,"w") as file:
        index=0
        for node in allNodes:
            file.write(str(index)+"\t"+str(node)+"\r")
            indexNode[index]=node
            nodeIndex[node]=index
            index+=1
    return nodeIndex,indexNode
    

    
if __name__=="__main__":
    IndexOriginNode("/root/data/wiki-Vote.txt","/root/data/wiki-Vote-index.txt")