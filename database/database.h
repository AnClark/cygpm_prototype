#include <cstdio>
#include <cstring>
#include <iostream>
#include <sqlite3.h>

#include "lex.export.h"
#include "tokens.h"

using namespace std;

#define SQLITE_ERR_RETURN \
    errorLevel = sqlite3_errcode(db); \
    return sqlite3_errcode(db);

class CygpmDatabase
{
private:
    sqlite3 *db;        // Database handler object
    char *zErrMsg = 0;  // Error message returned by sqlite3_exec()
    int rc;             // Return state
    int errorLevel = 0; // Error state. Only for constructors (or fallback).
                        // Other non-constructors can directly return error code.

public:
    CygpmDatabase(const char *fileName);
    ~CygpmDatabase();

    int createTable();                                        // Create basic table
    int parseAndBuildDatabase(const char *setupini_fileName); // Parse setup.ini, adding its data into database

    int getErrorLevel();
    int getErrorCode();
    const char *getErrorMsg();

    /** TODO:
     * - Querying database
     * - Manipulating dependencies
     */
};
