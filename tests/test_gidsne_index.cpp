#include <efanna2e/index_GIDSNE.h>

int main(int argc, char const *argv[])
{
    using namespace QI;
    IndexGidsne* index=new IndexGidsne();
    index->loadOriginGraph("/Users/shenhangke/Downloads/wiki-Vote.txt");
    return 0;

}
