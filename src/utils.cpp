#include "utils.h"

char *ltrim(char *source)
{
    char *p = source;
    while (*p && isspace(*p))
        p++;

    return p;
}

char *rtrim(char *source)
{
    for (int i = strlen(source) - 1; i >= 0 && isspace(source[i]); i--)
        source[i] = '\0';

    return source;
}

bool isInVector_string(vector<string> vector, const char *item)
{
    for (auto i = vector.begin(); i != vector.end(); i++)
        if (i->compare(item) == 0)
            return true;

    return false;
}

bool isFileExist(const char *fileName)
{
    ifstream ftest(fileName);

    if (ftest)
    {
        ftest.close();
        return true;
    }

    return false;
}

string calculateFileSHA512(string fileName)
{
    ifstream in(fileName, ios::binary);
    stringstream file_data; // Use stringstream to store file content

    if (!in)
        return string("error"); // Return string "error" if file not exist

    file_data << in.rdbuf(); // Read in the whole file at once

    in.close();
    return sha512(file_data.str());
}

int extractTextFromGzip(const char *fileName, vector<string> &result)
{
    /**
     * Clear result list
     */
    result.clear();

    /**
     * Read in the whole GZip file
     */
    ifstream input_file;
    char *buffer;
    long length;

    /* Open & check if opening error */
    input_file.open(fileName, ios_base::binary);
    if (!input_file)
        return CPM_FILE_ACCESS_ERROR;

    /* Get file size */
    input_file.seekg(0, ios::end);
    length = input_file.tellg();
    input_file.seekg(0, ios::beg);

    /* Read file into buffer */
    buffer = new char[length];
    input_file.read(buffer, length);
    input_file.close();

    /**
     * Decompress via libgzip_cpp
     */
    /* Allocate objects for decompression via gzip_cpp */
    gzip::Data data = gzip::AllocateData(length); // Allocate data storage. You must use AllocateData() here.
    gzip::Decomp decomp;
    data->ptr = buffer;

    /* Start decompression */
    bool succ;
    gzip::DataList out_data_list;
    std::tie(succ, out_data_list) = decomp.Process(data); // C++ feature. You can return more than 1 values via std::tuple,
                                                          // then use std::tie() to receive each return value,
                                                          // storing them to variables correspondingly.
    if (!succ)
    {
        // Failed to decompress data.
        fputs("Failed to decompress!", stderr);
        return CPM_DECOMPRESS_ERROR;
    }
    gzip::Data out_data = gzip::ExpandDataList(out_data_list); // Expand element list to one element

    /**
     * Split decompressed text by lines
     * 
     * Algorithm referenced from [pezy@SegmentFault]: https://segmentfault.com/a/1190000002483483
     */
    istringstream iss(string(out_data->ptr)); // Push decompressed data into stringstream

    for (string item; getline(iss, item, '\n');) // Use getline() to parse stream into lines
        if (item.empty())
            continue;
        else
            result.push_back(item);

    return 0;
}