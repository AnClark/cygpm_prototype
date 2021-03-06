#ifndef DATABASE_H
#define DATABASE_H

#include "stdafx.hpp"

#include "lex.export.h"
#include "tokens.h"
#include "utils.h"

using namespace std;

#define SQLITE_ERR_RETURN             \
    errorLevel = sqlite3_errcode(db); \
    return sqlite3_errcode(db);
#define SQLITE_BIND_MY_COLUMN(zName, data) \
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, zName), data, -1, SQLITE_STATIC); // Macro for sqlite3_bind_text() to make code more clearly,                                      \
                                                                                                 // as I'll bind more than 10 parameters.                                                         \
                                                                                                 // @param1: A statement object.                                                                  \
                                                                                                 // @param2: Index of bound parameter index. This is specified by sqlite3_bind_parameter_index(). \
                                                                                                 //          @zName is the index placehoder. Format is ":name".                                                                                     \
                                                                                                 // @param3: Text data to fill with. Will be preprocessed automatically. \
                                                                                                 // @param4: Length of text data. Set to negative value to let SQLite calculate it.                     \
                                                                                                 // @param5: a destructor used to dispose of the BLOB or string                                      \
                                                                                                 //          when SQLite finishes with it. Use SQLITE_STATIC here.

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
    sqlite3 *db;       // Database handler object
    char *zErrMsg = 0; // Error message returned by sqlite3_exec()
    //stringstream db_transaction_sql; // To build transaction SQL query
    int rc;             // Return state
    int errorLevel = 0; // Error state. Only for constructors (or fallback).
                        // Other non-constructors can directly return error code.

public:
    CygpmDatabase(const char *fileName);
    ~CygpmDatabase();
    void open(const char *fileName); // Manually open database

    int createTable();                                        // Create basic table
    int parseAndBuildDatabase(const char *setupini_fileName); // Parse setup.ini, adding its data into database
    int buildDependencyMap();                                 // Parse dependency list, then build dependency map

    int findDependencies(vector<string> &dependency_list, const char *pkg_name, const char *version); // Find dependencies

    char *getNewestVersion(const char *pkg_name);
    char *getShortDesc(const char *pkg_name);
    char *getLongDesc(const char *pkg_name);
    char *getCategory(const char *pkg_name);
    char *getInstallPakPath(const char *pkg_name, const char *version);
    char *getInstallPakSize(const char *pkg_name, const char *version);
    char *getInstallPakSHA512(const char *pkg_name, const char *version);
    char *getSourcePakPath(const char *pkg_name, const char *version);
    char *getSourcePakSize(const char *pkg_name, const char *version);
    char *getSourcePakSHA512(const char *pkg_name, const char *version);
    vector<const char *> getPrevVersions(const char *pkg_name);

    int getErrorLevel();
    int getErrorCode();
    const char *getErrorMsg();
    int getNumPackages();

    /** TODO:
     * - Querying database
     * - Manipulating dependencies
     */

private:
    void insertPackageInfo(CurrentPackageInfo *packageInfo);
    void insertPrevPackageInfo(CurrentPrevPackageInfo *prevPackageInfo);
    inline void submitYAMLItem(string YAML_section, CurrentPackageInfo *pkg_info, stringstream &buff);
    inline void submitYAMLItem_PrevVersion(string YAML_section, CurrentPrevPackageInfo *prev_pkg_info, stringstream &buff);
    inline void parseRequiresRaw(char *pkg_name, char *version, char *requires__raw);
    inline void parseDepends2(char *pkg_name, char *version, char *depends2__raw);
    int initTransaction();
    int commitTransaction();
    void execTransactionSQL(const char *sql_statement);
    inline char *queryOneResult(const char *sql_statement);
};

#endif