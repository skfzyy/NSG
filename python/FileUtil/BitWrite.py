import struct
import typing

def WriteVector(matrix:typing.List[typing.Tuple[int,float]],filePath):
    with open(filePath,"wb") as fvecsFile:
        for dimension,vector in matrix:
            for ele in vector:
                    dimensionData=struct.pack("<i",dimension)
                    # print(hex(eleData));
                    fvecsFile.write(dimensionData);
                    fvecsFile.write(struct.pack("<f",ele));



if __name__=="__main__":
        data=[(128,[12.,21.])]
        WriteVector(data,"./test_data.fvecs")

