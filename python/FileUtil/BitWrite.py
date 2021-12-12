import struct
import typing


def WriteVector(matrix: typing.List[typing.Tuple[int, float]], filePath):
    with open(filePath, "wb") as fvecsFile:
        for dimension, vector in matrix:
            print("write function ,the dimension is: "+str(dimension))
            dimensionData = struct.pack("<i", int(dimension))
            fvecsFile.write(dimensionData)
            for ele in vector:
                fvecsFile.write(struct.pack("<f", ele))


def generateQueryVec(matrix: typing.List[typing.Tuple[int, float]], indexs: typing.List[int], filePath: str):
    if(len(matrix) < 0):
        print("the input matirx is not correct")
        return
    queryMat = [matrix[i] for i in indexs]
    WriteVector(queryMat, filePath)


def loadQueryResult(filePath: str):
    result = []
    with open(filePath, "rb") as searchFile:
        # 判断文件中最邻近的节点数目
        while(True):
            tempK = searchFile.read(4)
            if(not tempK):
                break
            else:
                tempK=struct.unpack("<i",tempK)[0]
                unpackStr = "<"+str(int(tempK))+"i"
                result.append(struct.unpack(unpackStr,searchFile.read(4*tempK)))
    return result


def loadFvecsData(filePath):
    result=[]
    with open(filePath,"rb") as file:
        while(True):
            dimension=file.read(4)
            if not dimension:
                break
            else:
                dimension=struct.unpack("<i",dimension)[0]
                unpackStr="<"+str(int(dimension))+"f"
                realData=struct.unpack(unpackStr,file.read(4*int(dimension)))
                result.append((dimension,list(realData)))
    return result


if __name__ == "__main__":
    data = [(128, [12., 21.])]
    WriteVector(data, "./test_data.fvecs")
