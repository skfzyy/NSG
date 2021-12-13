from FileUtil.BitWrite import loadQueryResult
import sys

def testRecall(groundtruth:str,testData:str):
    groundtruthList=loadQueryResult(groundtruth)
    testDataList=loadQueryResult(testData)

    recall=0.0
    max=-1.0

    for i in range(len(testDataList)):
        total=0
        for ele in testDataList[i]:
            if ele in groundtruthList[i]:
                total+=1
        tempRecall=total/len(testDataList[i])
        if tempRecall>max:
            max=tempRecall
        recall+=tempRecall

    # for average
    print("================="+"the average recall is: =================")
    print(recall/len(testDataList))
    print("================="+"the max recall is: =================")
    print(max)
    return recall/len(testDataList)

if __name__=="__main__":
    testRecall(sys.argv[1],sys.argv[2])
        

