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
    result.clear(); // Initialize result

    FILE *fp;                            // File handler
    char *buffer;                        // Per-string buffer
    char gzip_command[] = "gzip -adcq "; // Param -a: Output as ASCII
                                         // Param -d: Decompress instead of compress
                                         // Param -c: Decompress to stdout with original file untouched
                                         // Param -q: Supress any possible warnings

    /* Check if file exists. Unexisted files will stuck popen(). */
    if (!isFileExist(fileName))
    {
        perror("Target GZip file doesn't exist.");
        return CPM_FILE_NOT_EXIST;
    }

    /* Use popen() to obtain GZip's output */
    if ((fp = popen(strcat(gzip_command, fileName), "r")) == NULL)
    {
        perror("Failed to execute gzip. Have you installed gzip?");
        return CPM_EXTERNAL_PROGRAM_FAILED;
    }

    /* Read in decompressed data as string */
    while (fgets(buffer, 1024, fp))
        result.push_back(buffer);

    /* Close file */
    fclose(fp);

    return 0;
}