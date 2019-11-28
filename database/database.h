#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <regex>
#include <sqlite3.h>

#include "lex.export.h"
#include "tokens.h"

using namespace std;

#define SQLITE_ERR_RETURN             \
    errorLevel = sqlite3_errcode(db); \
    return sqlite3_errcode(db);

#define STR_EQUAL(x, y) strcmp(x, y) == 0
#define YAML_SECTION_IS(x) YAML_section.compare(x) == 0

struct CurrentPackageInfo
{
    string pkg_name;
    string sdesc;
    string ldesc;
    string category;
    string requires__raw;
    string version;
    string install__raw;
    string source__raw;
    string depends2__raw;
};

struct CurrentPrevPackageInfo
{
    string pkg_name;
    string version;
    string install__raw;
    string source__raw;
    string depends2__raw;
};

class CygpmDatabase
{
private:
    sqlite3 *db;                     // Database handler object
    char *zErrMsg = 0;               // Error message returned by sqlite3_exec()
    stringstream db_transaction_sql; // To build transaction SQL query
    int rc;                          // Return state
    int errorLevel = 0;              // Error state. Only for constructors (or fallback).
                                     // Other non-constructors can directly return error code.

    int numPackages = 0;      // Count of packages
    int numAddedPackages = 0; // Count of added packages

public:
    CygpmDatabase(const char *fileName);
    ~CygpmDatabase();

    int createTable();                                        // Create basic table
    int parseAndBuildDatabase(const char *setupini_fileName); // Parse setup.ini, adding its data into database
    int buildDependencyMap();                                 // Parse dependency list, then build dependency map
    int generateInsSrcData();                                     // Parse _INSTALL__RAW and _SOURCE__RAW after building database

    int getErrorLevel();
    int getErrorCode();
    const char *getErrorMsg();
    int getNumPackages();
    int getNumAddedPackages();

    /** TODO:
     * - Querying database
     * - Manipulating dependencies
     */

protected:
    void insertPackageInfo(CurrentPackageInfo *packageInfo);
    void insertPrevPackageInfo(CurrentPrevPackageInfo *prevPackageInfo);
    inline void submitYAMLItem(string YAML_section, CurrentPackageInfo *pkg_info, stringstream &buff);
    inline void submitYAMLItem_PrevVersion(string YAML_section, CurrentPrevPackageInfo *prev_pkg_info, stringstream &buff);
    inline void parseRequiresRaw(char *pkg_name, char *requires__raw);
    inline void parseInsSrcRaw(char *pkg_name, char *install__raw, char *source__raw);
    void initTransaction();
    int commitTransaction();
};
