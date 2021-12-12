from FileUtil.BitWrite import loadQueryResult
import sys

def testRecall(groundtruth:str,testData:str):
    groundtruthList=loadQueryResult(groundtruth)
    testDataList=loadQueryResult(testData)

    recall=0.0

    for i in len(testDataList):
        total=0
        for ele in testDataList[i]:
            if ele in groundtruthList[i]:
                total+=1
        tempRecall=total/len(testDataList[i])
        recall+=tempRecall

    # for average
    return recall/len(testDataList)

if __name__=="__main__":
    testRecall(sys.argv[0],sys.argv[1])
        

