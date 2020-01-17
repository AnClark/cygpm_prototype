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
