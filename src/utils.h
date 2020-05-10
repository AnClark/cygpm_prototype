#ifndef UTILS_H
#define UTILS_H

#include "stdafx.hpp"
#include "extlib/sha512/sha512.h"

using namespace std;

enum InstallerErrors
{
    CPM_OK = 0,
    CPM_FILE_NOT_EXIST = 16,
    CPM_EXTERNAL_PROGRAM_FAILED,
    CPM_UNEXPECTED_ERROR
};

/**
 * String tricks 
 */
char *ltrim(char *source); // Remove leading spaces
char *rtrim(char *source); // Remove trailing spaces

/**
 * Common-use procedures
 */
bool isInVector_string(vector<string> vector, const char *item); // Check if an string item is in vector
bool isFileExist(const char *fileName);                          // Check if a file exists

/**
 * Data tools
 */
string calculateFileSHA512(string fileName);                           // Calculate a file's SHA512
int extractTextFromGzip(const char *fileName, vector<string> &result); // Extract a gzip-compressed text file's content into vector

#endif