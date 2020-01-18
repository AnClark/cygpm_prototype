#include "database.h"

inline char *rtrim(char *source)
{
    for (int i = strlen(source) - 1; i >= 0 && isspace(source[i]); i--)
        source[i] = '\0';

    return source;
}

inline bool isInVector_string(vector<string> vector, const char *item)
{
    for (auto i = vector.begin(); i != vector.end(); i++)
        if (i->compare(item) == 0)
            return true;

    return false;
}

int CygpmDatabase::findDependencies(vector<string> &dependency_list, const char *pkg_name, const char *version = NULL)
{
    /**
     * Build SQL statement
     */
    const char *SQL_GET_DEPENDENCIES = R"(
        SELECT DEPENDS_ON FROM DEPENDENCY_MAP WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[100 + strlen(SQL_GET_DEPENDENCIES) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL_GET_DEPENDENCIES, pkg_name, s_version.c_str());

    /**
     * Add the current package into dependency_list
     */

    // (1) If package is not in current dependency list, add it to list
    if (!isInVector_string(dependency_list, pkg_name))
        dependency_list.push_back(string(pkg_name));
    // (2) Or, if existed, no need to add it, just return.
    //     This can prevent infinite dependency. Example: `terminfo` and `terminfo-extra` depends on each other.
    //     Also, make sure that we have only one item in list. This can save time when downloading.
    else
        return 1;

    /**
     * Start query
     */

    /* Variables */
    char **dbResult;   // Results from sqlite3_get_table()
    int nRow, nColumn; // Row/Column count
    int i, j;          // Loop variable

    /**
     * NOTICE: In sqlite3_get_table(), dbResult's column values are successive. 
     * It stores the dimension table (traditional RxC) into a flat array.
     * So, dbResult[0, nColumn - 1] are column names, dbResult[nColumn, len(dbResult)] are column values.
     */
    int nIndex; // Current index of dbResult

    /* Execute SQL statement */
    rc = sqlite3_get_table(db, query, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        SQLITE_ERR_RETURN; // Also exit recursion on error
    }

    /* Process query results */
    nIndex = nColumn; // Initialize nIndex
    for (i = 0; i < nRow; i++)
    {
        // For any dependencies, just do the same:
        //      -> Fetch their dependencies recursively.
        findDependencies(dependency_list, dbResult[nIndex], getNewestVersion(dbResult[nIndex])); // dbResult[nIndex] is the current dependency item

        nIndex++; // Go to the next row
    }

    return 0;
}

char *CygpmDatabase::getNewestVersion(const char *pkg_name)
{
    /* Variables */
    char **dbResult;   // Results from sqlite3_get_table()
    int nRow, nColumn; // Row/Column count

    int nIndex; // Current index of dbResult

    /* SQL query */
    const char *SQL_GET_NEWEST_VERSION = "SELECT VERSION FROM PKG_INFO WHERE PKG_NAME = '%s';";

    char *query = new char[100 + strlen(SQL_GET_NEWEST_VERSION) + strlen(pkg_name)]; // Actual SQL query rendered by sprintf()
    sprintf(query, SQL_GET_NEWEST_VERSION, pkg_name);

    /**
     * Execute SQL statement to get newest version
     */
    rc = sqlite3_get_table(db, query, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        return NULL;
    }

    /**
     * dbResult[index] is what we want.
     */
    nIndex = nColumn;               // Initialize nIndex
    return rtrim(dbResult[nIndex]); // Trim trailing space, as findDependencies() doesn't trim spaces
}

char *CygpmDatabase::getShortDesc(const char *pkg_name)
{
    /* SQL query */
    const char *SQL = R"(
        SELECT SDESC FROM PKG_INFO WHERE PKG_NAME = '%s';
    )";

    char *query = new char[100 + strlen(SQL) + strlen(pkg_name)]; // Actual SQL query rendered by sprintf()
    sprintf(query, SQL, pkg_name);

    return queryOneResult(query);
}

char *CygpmDatabase::getLongDesc(const char *pkg_name)
{
    /* SQL query */
    const char *SQL = R"(
        SELECT LDESC FROM PKG_INFO WHERE PKG_NAME = '%s';
    )";

    char *query = new char[100 + strlen(SQL) + strlen(pkg_name)]; // Actual SQL query rendered by sprintf()
    sprintf(query, SQL, pkg_name);

    return queryOneResult(query);
}

char *CygpmDatabase::getCategory(const char *pkg_name)
{
    /* SQL query */
    const char *SQL = R"(
        SELECT CATEGORY FROM PKG_INFO WHERE PKG_NAME = '%s';
    )";

    char *query = new char[100 + strlen(SQL) + strlen(pkg_name)]; // Actual SQL query rendered by sprintf()
    sprintf(query, SQL, pkg_name);

    return queryOneResult(query);
}

char *CygpmDatabase::getInstallPakPath(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT INSTALL_PAK_PATH FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

char *CygpmDatabase::getInstallPakSize(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT INSTALL_PAK_SIZE FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

char *CygpmDatabase::getInstallPakSHA512(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT INSTALL_PAK_SHA512 FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

char *CygpmDatabase::getSourcePakPath(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT SOURCE_PAK_PATH FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

char *CygpmDatabase::getSourcePakSize(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT SOURCE_PAK_SIZE FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

char *CygpmDatabase::getSourcePakSHA512(const char *pkg_name, const char *version = NULL)
{
    /** Build SQL statement */
    const char *SQL = R"(
        SELECT SOURCE_PAK_SHA512 FROM %s WHERE PKG_NAME = '%s' AND VERSION LIKE '%s';
    )";

    string s_version(version == NULL ? getNewestVersion(pkg_name) : version);
    s_version += "%"; // Use wildcard to match possible trailing spaces

    char *query = new char[120 + strlen(SQL) + strlen(pkg_name) + (version == NULL ? 0 : strlen(version))];
    sprintf(query, SQL,
            version == NULL ? "PKG_INFO" : "PREV_VERSIONS",
            pkg_name,
            s_version.c_str());

    return queryOneResult(query);
}

vector<const char *> CygpmDatabase::getPrevVersions(const char *pkg_name)
{
    /* Variables */
    vector<const char *> result; // Prev version list to be returned
    char **dbResult;             // Results from sqlite3_get_table()
    int nRow, nColumn;           // Row/Column count

    int nIndex; // Current index of dbResult

    /**
     * Build SQL statement
     */
    const char *SQL_GET_PREV_VERSIONS = R"(
        SELECT VERSION FROM PREV_VERSIONS WHERE PKG_NAME = '%s';
    )";
    char *query = new char[100 + strlen(SQL_GET_PREV_VERSIONS) + strlen(pkg_name)];
    sprintf(query, SQL_GET_PREV_VERSIONS, pkg_name);

    /**
     * Execute SQL statement to get previous versions
     */
    rc = sqlite3_get_table(db, query, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        return result;
    }

    /**
     * Parse dbResult.
     */
    nIndex = nColumn; // Initialize nIndex
    for (int i = 0; i < nRow; i++)
    {
        result.push_back(rtrim(dbResult[nIndex])); // Add version number to list, trimming trailing spaces

        nIndex++;
    }

    return result;
}

inline char *CygpmDatabase::queryOneResult(const char *sql_statement)
{
    /* Variables */
    char **dbResult;   // Results from sqlite3_get_table()
    int nRow, nColumn; // Row/Column count

    int nIndex; // Current index of dbResult

    /**
     * Execute SQL statement to get required data
     * In this method, you can only query one column and one row.
     */
    rc = sqlite3_get_table(db, sql_statement, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        return NULL;
    }

    /**
     * dbResult[index] is what we want.
     */
    nIndex = nColumn;               // Initialize nIndex
    return rtrim(dbResult[nIndex]); // Trim trailing space, as findDependencies() doesn't trim spaces
}