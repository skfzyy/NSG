# this script should run in correct enviorment
# the arguments list as below:
# 1. the original data(graph format as txt) ps:test data
# 2. the dimention of vevtor

export DIR="$(cd "$(dirname "$0")" && pwd)"
export GRDSUFIX="_grd"
export SEARCHRETNAME="_searchResult"


rm -rf $1_*
echo $1_*
echo "deleted all files in data path" 
if [ $# -lt 2 ]; then
    echo "the arguments is less then 2"
else
    echo $1${GRDSUFIX}
    echo $1${GRDSUFIX}${SEARCHRETNAME}
    echo $1${SEARCHRETNAME}
    # generate the testData search result
    python ${DIR}/../python/testCCall.py $1 $2 getgroudtruth 1 $3
    echo "get groundtruth and embed finished"

    cp $1 $1_grd

    # generate the groundtruth search result
    python ${DIR}/../python/testCCall.py $1_grd $2 0,1,2,3,4,5,6,7,8,9,10 0 $3
    echo "generate the groudtruth search result finish"

    #generate the testData search result
    python ${DIR}/../python/testCCall.py $1 $2 0,1,2,3,4,5,6,7,8,9,10 0 $3
    echo "generate the testData search result finish"

    #get recall
    echo $1${GRDSUFIX}${SEARCHRETNAME}
    echo $1${SEARCHRETNAME}
    python ${DIR}/../python/testRecall.py $1${GRDSUFIX}${SEARCHRETNAME} $1${SEARCHRETNAME}
    echo "get recall finish"
fi
