#include "gzip_cpp.h"
#include <stdio.h>
#include <string.h>

const char *const SOURCE_STRING = "abcdefghijklmnopqrstuvwxyz";
gzip::DataList g_compressed_data;

void compress_test()
{
    gzip::Comp comp;
    g_compressed_data =
        comp.Process(SOURCE_STRING, strlen(SOURCE_STRING) + 1, true);
}

void decompress_test()
{
    gzip::Decomp decomp;
    bool succ;
    gzip::DataList out_data_list;
    for (const gzip::Data &data : g_compressed_data)
    {
        gzip::DataList this_out_data_list;
        std::tie(succ, this_out_data_list) = decomp.Process(data);
        std::copy(this_out_data_list.begin(), this_out_data_list.end(),
                  std::back_inserter(out_data_list));
    }
    gzip::Data out_data = gzip::ExpandDataList(out_data_list);

    printf("Decompressed data: %s\n", out_data->ptr);
}

void gzip_file_decompress_test()
{
    FILE *pFile = fopen("test.gz", "rb");
    char *buffer;
    long lSize;
    size_t result;

    /* 获取文件大小 */
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    /* 分配内存存储整个文件 */
    buffer = (char *)malloc(sizeof(char) * lSize);
    if (buffer == NULL)
    {
        fputs("Memory error", stderr);
        exit(2);
    }

    /* 将文件拷贝到buffer中 */
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize)
    {
        fputs("Reading error", stderr);
        exit(3);
    }

    printf("%d\n", lSize);
#if 0    
    for (int i = 0; i < lSize; ++i)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
#endif

    gzip::Data data = gzip::AllocateData(lSize);
    gzip::Decomp decomp;
    data->ptr = buffer;

    bool succ;
    gzip::DataList out_data_list;
    std::tie(succ, out_data_list) = decomp.Process(data);
    if (!succ)
    {
        // Failed to decompress data.
        return;
    }
    gzip::Data out_data = gzip::ExpandDataList(out_data_list);

    printf("%s\n", out_data->ptr);
}

int main()
{
    compress_test();
    decompress_test();

    gzip_file_decompress_test();
}