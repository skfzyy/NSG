#include <efanna2e/index_GIDSNE.h>
// #include <iostream>

#define GR_INDEX_SUFIX "_index"
#define KNN_SUFIX "_knn"
#define EMBED_SUFIX "_embed"
#define GIDSNE_SUFIX "_gidsne"

#define COMBINEPATH(x, y)                                            \
    (std::string((x)) + std::string((y))).c_str()

void load_data(const char* filename, float*& data, unsigned& num,
               unsigned& dim)
{ // load data with sift10K pattern
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open())
    {
        std::cout << "open file error" << std::endl;
        exit(-1);
    }
    in.read((char*)&dim, 4);
    in.seekg(0, std::ios::end);
    std::ios::pos_type ss = in.tellg();
    size_t fsize = (size_t)ss;
    num = (unsigned)(fsize / (dim + 1) / 4);
    data = new float[(size_t)num * (size_t)dim];

    in.seekg(0, std::ios::beg);
    for (size_t i = 0; i < num; i++)
    {
        in.seekg(4, std::ios::cur);
        in.read((char*)(data + i * dim), dim * 4);
    }
    in.close();
}

extern "C" void BuildGidsneIndex(
    const wchar_t* originalDataPath,
    unsigned L,
    unsigned R,
    unsigned C)
{
    std::wstring wOriginDataPath(originalDataPath);
    std::string ansiOriginDataPath(wOriginDataPath.begin(),
                                   wOriginDataPath.end());
    float* data_load = NULL;
    unsigned points_num, dim;
    load_data(COMBINEPATH(ansiOriginDataPath, EMBED_SUFIX), data_load,
              points_num, dim);
    std::string nn_graph_path(
        COMBINEPATH(ansiOriginDataPath, KNN_SUFIX));

    // data_load = efanna2e::data_align(data_load, points_num,
    // dim);//one must align the data before build
    QI::IndexGidsne indexGidsne(
        dim, points_num, efanna2e::L2, nullptr,
        ansiOriginDataPath.c_str(),
        COMBINEPATH(ansiOriginDataPath, GR_INDEX_SUFIX));

    logDebug("has create indexGidsne");
    auto s = std::chrono::high_resolution_clock::now();
    efanna2e::Parameters paras;
    paras.Set<unsigned>("L", L);
    paras.Set<unsigned>("R", R);
    paras.Set<unsigned>("C", C);
    paras.Set<std::string>("nn_graph_path", nn_graph_path);
    logDebug("has set the params");
    indexGidsne.Build(points_num, data_load, paras);
    logDebug("has build the index");
    auto e = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = e - s;

    std::cout << "indexing time: " << diff.count() << "\n";
    indexGidsne.Save(COMBINEPATH(ansiOriginDataPath, GIDSNE_SUFIX));
}


int main(int argc, char* argv[])
{
    if (argc != 9)
    {
        std::cout << argv[0]
                  << " data_file nn_graph_path L R C save_graph_file"
                  << std::endl;
        exit(-1);
    }
    float* data_load = NULL;
    unsigned points_num, dim;
    load_data(argv[1], data_load, points_num, dim);

    std::string nn_graph_path(argv[2]);
    unsigned L = (unsigned)atoi(argv[3]);
    unsigned R = (unsigned)atoi(argv[4]);
    unsigned C = (unsigned)atoi(argv[5]);

    // data_load = efanna2e::data_align(data_load, points_num,
    // dim);//one must align the data before build
    QI::IndexGidsne indexGidsne(dim, points_num, efanna2e::L2,
                                nullptr, argv[7], argv[8]);

    auto s = std::chrono::high_resolution_clock::now();
    efanna2e::Parameters paras;
    paras.Set<unsigned>("L", L);
    paras.Set<unsigned>("R", R);
    paras.Set<unsigned>("C", C);
    paras.Set<std::string>("nn_graph_path", nn_graph_path);

    indexGidsne.Build(points_num, data_load, paras);
    auto e = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = e - s;

    std::cout << "indexing time: " << diff.count() << "\n";
    indexGidsne.Save(argv[6]);

    return 0;
}
