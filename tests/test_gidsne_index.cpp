#include <efanna2e/index_GIDSNE.h>

int main(int argc, char const *argv[])
{
    using namespace QI;
    IndexGidsne* index=new IndexGidsne();
    index->buildPathIndex("/root/data/wiki-Vote.txt");
    return 0;

}
