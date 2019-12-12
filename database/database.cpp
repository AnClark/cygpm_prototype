#include "database.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

inline char *ltrim(char *source)
{
    char *p = source;
    while (*p && isspace(*p))
        p++;

    return p;
}

CygpmDatabase::CygpmDatabase(const char *fileName)
{
    /**
     * Open database
     */
    rc = sqlite3_open(fileName, &db);

    if (rc)
    {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        errorLevel = sqlite3_errcode(db);
        return;
    }
    else
    {
        cerr << "Opened database successfully" << endl;
    }

    /**
     * Optimize database - Store journal in memory
     */

    const char *SQL_PRAGMA_SET_JOURNAL_TO_MEMORY = R"(
        PRAGMA journal_mode = memory;
    )";

    rc = sqlite3_exec(db, SQL_PRAGMA_SET_JOURNAL_TO_MEMORY, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "Failed to set journal to memory: " << zErrMsg << endl;
        errorLevel = sqlite3_errcode(db);
    }
    else
    {
        cerr << "Database: Journal set to memory" << endl;
    }

    // Constructor doesn't support return values. So use errorLevel instead.
    errorLevel = 0;
}

CygpmDatabase::~CygpmDatabase()
{
    sqlite3_close(db);
}

int CygpmDatabase::createTable()
{
    /** 
     * Initialize transaction
     */
    initTransaction();

    /** 
     * Create package info table
     */
    const char *SQL_CREATE_PACKAGE_INFO = R"(
        DROP TABLE IF EXISTS "PKG_INFO";

        CREATE TABLE IF NOT EXISTS "PKG_INFO" (
            "PKG_NAME"	TEXT NOT NULL,
            "SDESC"	TEXT,
            "LDESC"	TEXT,
            "CATEGORY"	TEXT,
            "REQUIRES__RAW"	TEXT,
            "VERSION"	TEXT,
            "_INSTALL__RAW"	TEXT NOT NULL,
            "INSTALL_PAK_PATH"	TEXT,
            "INSTALL_PAK_SIZE"	TEXT,
            "INSTALL_PAK_SHA512"	TEXT,
            "_SOURCE__RAW"	TEXT,
            "SOURCE_PAK_PATH"	TEXT,
            "SOURCE_PAK_SIZE"	TEXT,
            "SOURCE_PAK_SHA512"	TEXT,
            "DEPENDS2__RAW"	TEXT,
            PRIMARY KEY("PKG_NAME")
        );
    )";
    db_transaction_sql << SQL_CREATE_PACKAGE_INFO << endl;

    /**
     *  Create dependency map table
     */
    const char *SQL_CREATE_DEPENDENCY_MAP = R"(
        DROP TABLE IF EXISTS "DEPENDENCY_MAP";

        CREATE TABLE IF NOT EXISTS "DEPENDENCY_MAP" (
	        "PKG_NAME"	TEXT NOT NULL,
            "VERSION"   TEXT NOT NULL,
	        "DEPENDS_ON"	TEXT NOT NULL,
	        FOREIGN KEY("PKG_NAME") REFERENCES "PKG_INFO"("PKG_NAME")
        );
    )";
    db_transaction_sql << SQL_CREATE_DEPENDENCY_MAP << endl;

    /**
     * Create previous versions table
     */
    const char *SQL_CREATE_PREV_VERSIONS_TABLE = R"(
        DROP TABLE IF EXISTS "PREV_VERSIONS";

        CREATE TABLE IF NOT EXISTS "PREV_VERSIONS" (
            "PKG_NAME"	TEXT NOT NULL,
            "VERSION"	TEXT NOT NULL,
            "_INSTALL__RAW"	TEXT NOT NULL,
            "INSTALL_PAK_PATH"	TEXT,
            "INSTALL_PAK_SIZE"	TEXT,
            "INSTALL_PAK_SHA512"	TEXT,
            "_SOURCE__RAW"	TEXT,
            "SOURCE_PAK_PATH"	TEXT,
            "SOURCE_PAK_SIZE"	TEXT,
            "SOURCE_PAK_SHA512"	TEXT,
            "DEPENDS2__RAW"	TEXT,
            FOREIGN KEY ("PKG_NAME") REFERENCES "PKG_INFO"("PKG_NAME")
        );
    )";
    db_transaction_sql << SQL_CREATE_PREV_VERSIONS_TABLE << endl;

    /**
     * Commit transaction & Get result
     */
    errorLevel = commitTransaction();
    if (errorLevel == 0)
        cerr << "Tables created" << endl;
    else
        cerr << "Error while creating tables: " << zErrMsg << endl;

    return errorLevel;
}

int CygpmDatabase::parseAndBuildDatabase(const char *setupini_fileName)
{
    int token_type; // Lexer token type

    /**
     * Operation bits - controlling the switch cases' behaviour
     */
    bool is_adding_a_package = false;          // I'm manipulating a package's info
    bool is_adding_a_YAML_section = false;     // I'm manipulating a YAML item
    bool is_adding_a_previous_version = false; // I'm manipulating a package's previous versions ("[prev]")

    /**
     * Buffer objects
     */
    CurrentPackageInfo *pkg_info = new CurrentPackageInfo;              // Current package's info
    CurrentPrevPackageInfo *prev_pkg_info = new CurrentPrevPackageInfo; // Current package's previous version info
    stringstream buff;                                                  // Buffer to build a YAML content
    string last_YAML_section;                                           // The last YAML section to be committed

    numPackages = 0; // Packages' count

    // Initialize transaction
    initTransaction();

    /**
     * Start parsing
     */
    // Open setup.ini
    yyrestart(fopen(setupini_fileName, "r"));

    cerr << "Parsing setup.ini" << endl;

    // Call lexer
    while (token_type = yylex())
    {
        switch (token_type)
        {
        case T_Package_Name:
            numPackages++; // Stat package count

            /**
             * Submit the last package info before start a new one.
             * If is_adding_a_package == true, there's a package to be added and committed.
             */
            if (is_adding_a_package)
            {
                // Remember that when a new package starts, the previous package's
                //  last YAML item is also to be committed.
                // Also should commit its previous version's YAML item.
                if (is_adding_a_YAML_section)
                    if (is_adding_a_previous_version)
                        submitYAMLItem_PrevVersion(last_YAML_section, prev_pkg_info, buff);
                    else
                        submitYAMLItem(last_YAML_section, pkg_info, buff);

                // Now submit our new package info
                insertPackageInfo(pkg_info);

                // Submit the last previous package's prev version info
                if (is_adding_a_previous_version)
                    insertPrevPackageInfo(prev_pkg_info);
            }

            /**
             * Start handling a new package.
             */
            pkg_info = new CurrentPackageInfo; // Create a package info object
            pkg_info->pkg_name = yytext;       // Set package name

            /**
             * Set operation bits.
             */
            is_adding_a_package = true;
            is_adding_a_previous_version = false;
            is_adding_a_YAML_section = false;

            break;

        case T_YAML_Key:
            // Ignore non-package YAML items
            // TODO: Handle setup.ini's metadata
            if (!is_adding_a_package)
                break;

            //cout << "[Current Tag] " << yytext << "; [Last Tag] " << last_YAML_section << endl;

            is_adding_a_YAML_section = true;

            if (is_adding_a_previous_version)
                submitYAMLItem_PrevVersion(last_YAML_section, prev_pkg_info, buff);
            else
                submitYAMLItem(last_YAML_section, pkg_info, buff);

            last_YAML_section = yytext;

            break;

        case T_Prev_Version_Mark:
            /**
             * Remember that when a prev version starts, the last YAML item which positions
             * before indicator "[prev]" is also to be committed.
             */
            if (is_adding_a_YAML_section)
            {
                if (is_adding_a_previous_version)
                    submitYAMLItem_PrevVersion(last_YAML_section, prev_pkg_info, buff); // Submit the last prev version's one
                else
                    submitYAMLItem(last_YAML_section, pkg_info, buff); // Submit the main version's one
            }

            /**
             * Check orphan [prev]. This is not allowed.
             */
            if (pkg_info->pkg_name.length() <= 0)
            {
                cerr << "Parse error: Orphan [prev] at " << yylineno << endl;
                break;
            }

            /**
             * Submit the last previous version info before getting a new one.
             * If is_adding_a_previous_version == true, there's a prev version to be added and committed.
             */
            if (is_adding_a_previous_version)
            {
                // Now submit our prev version info
                insertPrevPackageInfo(prev_pkg_info);
            }

            /**
             * Start handling a new previous version.
             */
            prev_pkg_info = new CurrentPrevPackageInfo;
            prev_pkg_info->pkg_name = pkg_info->pkg_name;

            /**
             * Set operation bits.
             */
            is_adding_a_previous_version = true;
            is_adding_a_YAML_section = false;

            break;

        case T_Word:
            if (is_adding_a_YAML_section)
            {
                /**
                 * Escape single quotation mark "'"
                 */
                for (int i = 0; i < strlen(yytext); i++)
                {
                    switch (yytext[i])
                    {
                    case '\'':
                        buff << "''";
                        break;
                    default:
                        buff << yytext[i];
                    }
                }
                buff << " ";
            }
            //buff << yytext << " ";
            break;
        }
    }

    /**
     * Remember to add the last package
     * NOTICE: This section is same as the one in the lexer loop -> case "T_Package_Name".
     */
    if (is_adding_a_package)
    {
        // Remember that when a new package starts, the previous package's
        //  last YAML item is also to be committed.
        // Also should commit its previous version's YAML item.
        if (is_adding_a_YAML_section)
            if (is_adding_a_previous_version)
                submitYAMLItem_PrevVersion(last_YAML_section, prev_pkg_info, buff);
            else
                submitYAMLItem(last_YAML_section, pkg_info, buff);

        // Now submit our new package info
        insertPackageInfo(pkg_info);

        // Submit the last previous package's prev version info
        if (is_adding_a_previous_version)
            insertPrevPackageInfo(prev_pkg_info);
    }

    /**
     * Commit transaction & Get result
     */
    errorLevel = commitTransaction();
    if (errorLevel == 0)
        cerr << "Built setup.ini database" << endl;
    else
        cerr << "Error while building database: " << zErrMsg << endl;

    return 0;
}

int CygpmDatabase::buildDependencyMap()
{
    /* Variables */
    char **dbResult; // Results from sqlite3_get_table()
    int nRow;        // Row count
    int nColumn;     // Column count
    int i, j;        // Loop variable

    /**
     * NOTICE: In sqlite3_get_table(), dbResult's column values are successive. 
     * It stores the dimension table (traditional RxC) into a flat array.
     * So, dbResult[0, nColumn - 1] are column names, dbResult[nColumn, len(dbResult)] are column values.
     */
    int nIndex; // Current index of dbResult

    /* SQL query */
    const char *SQL_GET_REQUIRES__RAW = R"(
        SELECT PKG_NAME,VERSION,REQUIRES__RAW FROM PKG_INFO;
    )";
    const char *SQL_GET_PREV_DEPENDS2__RAW = R"(
        SELECT PKG_NAME,VERSION,DEPENDS2__RAW FROM PREV_VERSIONS;
    )";

    /* Initialize transaction */
    initTransaction();

    cerr << "Building dependency map" << endl;

    ///////////////////////////// CURRENT VERSION /////////////////////////////

    /**
     * Execute SQL statement to get requires__raw data 
     */
    rc = sqlite3_get_table(db, SQL_GET_REQUIRES__RAW, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        SQLITE_ERR_RETURN;
    }

    /**
     * Start parsing requires__raw.
     * Parse each package's requires__raw respectively.
     */
    nIndex = nColumn; // Initialize nIndex
    for (i = 0; i < nRow; i++)
    {
        // Run parser
        // There will be only three columns: PKG_NAME, VERSION, REQUIRES_RAW.
        parseRequiresRaw(dbResult[nIndex], dbResult[nIndex + 1], dbResult[nIndex + 2]);

        nIndex += 3; // Go to the next row
    }

    ///////////////////////////// PREV VERSION /////////////////////////////

    /**
     * Execute SQL statement to get depends2__raw data 
     */
    rc = sqlite3_get_table(db, SQL_GET_PREV_DEPENDS2__RAW, &dbResult, &nRow, &nColumn, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        SQLITE_ERR_RETURN;
    }

    /**
     * Start parsing depends2__raw.
     * Parse each package's depends2__raw respectively.
     */
    nIndex = nColumn; // Initialize nIndex
    for (i = 0; i < nRow; i++)
    {
        // Run parser
        // There will be only three columns: PKG_NAME, VERSION, DEPENDS2_RAW.
        parseDepends2(dbResult[nIndex], dbResult[nIndex + 1], dbResult[nIndex + 2]);

        nIndex += 3; // Go to the next row
    }

    //////////////////////////////// FINALIZE ////////////////////////////////

    /**
     * Commit transaction & Get result
     */
    errorLevel = commitTransaction();
    if (errorLevel == 0)
        cerr << "Dependency map built" << endl;
    else
        cerr << "Error while building dependency map: " << zErrMsg << endl;

    return errorLevel;
}

inline void CygpmDatabase::submitYAMLItem(string YAML_section, CurrentPackageInfo *pkg_info, stringstream &buff)
{
    if (YAML_SECTION_IS("sdesc:"))
    {
        pkg_info->sdesc = buff.str();
    }
    else if (YAML_SECTION_IS("ldesc:"))
    {
        pkg_info->ldesc = buff.str();
    }
    else if (YAML_SECTION_IS("category:"))
    {
        pkg_info->category = buff.str();
    }
    else if (YAML_SECTION_IS("requires:"))
    {
        pkg_info->requires__raw = buff.str();
    }
    else if (YAML_SECTION_IS("version:"))
    {
        pkg_info->version = buff.str();
    }
    else if (YAML_SECTION_IS("install:"))
    {
        pkg_info->install__raw = buff.str();
    }
    else if (YAML_SECTION_IS("source:"))
    {
        pkg_info->source__raw = buff.str();
    }
    else if (YAML_SECTION_IS("depends2:"))
    {
        pkg_info->depends2__raw = buff.str();
    }

    buff.str("");
    if (buff.str().length() > 0)
        cerr << "BUFF IS NOT CLEAN!" << endl;
}

inline void CygpmDatabase::submitYAMLItem_PrevVersion(string YAML_section, CurrentPrevPackageInfo *prev_pkg_info, stringstream &buff)
{
    if (YAML_SECTION_IS("version:"))
    {
        prev_pkg_info->version = buff.str();
    }
    else if (YAML_SECTION_IS("install:"))
    {
        prev_pkg_info->install__raw = buff.str();
    }
    else if (YAML_SECTION_IS("source:"))
    {
        prev_pkg_info->source__raw = buff.str();
    }
    else if (YAML_SECTION_IS("depends2:"))
    {
        prev_pkg_info->depends2__raw = buff.str();
    }

    buff.str("");
    if (buff.str().length() > 0)
        cerr << "BUFF IS NOT CLEAN!" << endl;
}

void CygpmDatabase::insertPackageInfo(CurrentPackageInfo *packageInfo)
{
    /* Pre-define SQL query */
    const char *SQL_INSERT_PACKAGE_INFO = R"(
        INSERT INTO "PKG_INFO" (PKG_NAME, SDESC, LDESC, CATEGORY, REQUIRES__RAW, VERSION, 
                                _INSTALL__RAW, INSTALL_PAK_PATH, INSTALL_PAK_SIZE, INSTALL_PAK_SHA512, 
                                _SOURCE__RAW, SOURCE_PAK_PATH, SOURCE_PAK_SIZE, SOURCE_PAK_SHA512, 
                                DEPENDS2__RAW)
        VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s', '%s');
    )";

    /* Final SQL to generate */
    char *target_sql;

    /* Preprocess package name data */
    char pkg_name[packageInfo->pkg_name.length() + 1];
    sscanf(packageInfo->pkg_name.c_str(), "@ %s", pkg_name);

    /* Preprocess install/source data */
    const int LEN_INSTALL__RAW = packageInfo->install__raw.length() + 1,
              LEN_SOURCE__RAW = packageInfo->source__raw.length() + 1;
    char install_pak_path[LEN_INSTALL__RAW], install_pak_size[LEN_INSTALL__RAW], install_pak_sha512[LEN_INSTALL__RAW];
    char source_pak_path[LEN_SOURCE__RAW], source_pak_size[LEN_SOURCE__RAW], source_pak_sha512[LEN_SOURCE__RAW];

    sscanf(packageInfo->install__raw.c_str(), "%s %s %s", install_pak_path, install_pak_size, install_pak_sha512);
    sscanf(packageInfo->source__raw.c_str(), "%s %s %s", source_pak_path, source_pak_size, source_pak_sha512);

    /* Calculate how much space should target_sql have */
    int len_target_sql = 500 + LEN_INSTALL__RAW * 3 + LEN_SOURCE__RAW * 3 + strlen(SQL_INSERT_PACKAGE_INFO) + packageInfo->pkg_name.length() + packageInfo->category.length() + packageInfo->depends2__raw.length() + packageInfo->install__raw.length() + packageInfo->ldesc.length() + packageInfo->requires__raw.length() + packageInfo->sdesc.length() + packageInfo->source__raw.length() + packageInfo->version.length();
    target_sql = new char[len_target_sql];

    /* Concatenate SQL query */
    sprintf(target_sql, SQL_INSERT_PACKAGE_INFO,
            pkg_name,
            packageInfo->sdesc.c_str(),
            packageInfo->ldesc.c_str(),
            packageInfo->category.c_str(),
            packageInfo->requires__raw.c_str(),
            packageInfo->version.c_str(),
            packageInfo->install__raw.c_str(),
            install_pak_path,
            install_pak_size,
            install_pak_sha512,
            packageInfo->source__raw.c_str(),
            source_pak_path,
            source_pak_size,
            source_pak_sha512,
            packageInfo->depends2__raw.c_str());

    /* Commit SQL query */
    db_transaction_sql << target_sql;
}

void CygpmDatabase::insertPrevPackageInfo(CurrentPrevPackageInfo *prevPackageInfo)
{
    /* Pre-define SQL query */
    const char *SQL_INSERT_PREV_PACKAGE_INFO = R"(
        INSERT INTO "PREV_VERSIONS" (PKG_NAME, VERSION, 
                                    _INSTALL__RAW, INSTALL_PAK_PATH, INSTALL_PAK_SIZE, INSTALL_PAK_SHA512, 
                                    _SOURCE__RAW, SOURCE_PAK_PATH, SOURCE_PAK_SIZE, SOURCE_PAK_SHA512, 
                                    DEPENDS2__RAW)
        VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');
    )";

    /* Final SQL to generate */
    char *target_sql;

    /* Preprocess package name data */
    char pkg_name[prevPackageInfo->pkg_name.length() + 1];
    sscanf(prevPackageInfo->pkg_name.c_str(), "@ %s", pkg_name);

    /* Preprocess install/source data */
    const int LEN_INSTALL__RAW = prevPackageInfo->install__raw.length() + 1,
              LEN_SOURCE__RAW = prevPackageInfo->source__raw.length() + 1;
    char install_pak_path[LEN_INSTALL__RAW], install_pak_size[LEN_INSTALL__RAW], install_pak_sha512[LEN_INSTALL__RAW];
    char source_pak_path[LEN_SOURCE__RAW], source_pak_size[LEN_SOURCE__RAW], source_pak_sha512[LEN_SOURCE__RAW];

    sscanf(prevPackageInfo->install__raw.c_str(), "%s %s %s", install_pak_path, install_pak_size, install_pak_sha512);
    sscanf(prevPackageInfo->source__raw.c_str(), "%s %s %s", source_pak_path, source_pak_size, source_pak_sha512);

    /* Calculate how much space should target_sql have */
    int len_target_sql = 500 + LEN_INSTALL__RAW * 3 + LEN_SOURCE__RAW * 3 + strlen(SQL_INSERT_PREV_PACKAGE_INFO) + prevPackageInfo->depends2__raw.length() + prevPackageInfo->install__raw.length() + prevPackageInfo->pkg_name.length() + prevPackageInfo->source__raw.length() + prevPackageInfo->version.length();
    target_sql = new char[len_target_sql];

    /* Concatenate SQL query */
    sprintf(target_sql, SQL_INSERT_PREV_PACKAGE_INFO,
            pkg_name,
            prevPackageInfo->version.c_str(),
            prevPackageInfo->install__raw.c_str(),
            install_pak_path,
            install_pak_size,
            install_pak_sha512,
            prevPackageInfo->source__raw.c_str(),
            source_pak_path,
            source_pak_size,
            source_pak_sha512,
            prevPackageInfo->depends2__raw.c_str());

    /* Commit SQL query */
    db_transaction_sql << "    -- [prev] " << prevPackageInfo->version << endl;
    db_transaction_sql << target_sql;
}

inline void CygpmDatabase::parseRequiresRaw(char *pkg_name, char *version, char *requires__raw)
{
    /* SQL statements */
    const char *SQL_INSERT_DEPENDENCY_MAP_ITEM = R"(
        INSERT INTO "DEPENDENCY_MAP" (PKG_NAME, VERSION, DEPENDS_ON)
        VALUES ('%s', '%s', '%s');
    )";               // Pre-defined SQL query
    char *target_sql; // Final SQL to generate

    /**
     * Parse requires__raw
     */
    const char *splitter = " "; // strtok()'s splitter
    char *token;                // Child string

    /* strtok() will modify its source! So we'd better make a copy. */
    char *tmp = new char[strlen(requires__raw) + 1];
    strcpy(tmp, requires__raw);

    /* Split the requires__raw */
    token = strtok(tmp, splitter); // Get the first child string

    // Get the remainders
    while (token != NULL)
    {
        // Calculate how much space should target_sql have
        target_sql = new char[100 + strlen(SQL_INSERT_DEPENDENCY_MAP_ITEM) + strlen(pkg_name) + strlen(token)];

        // Form SQL statement
        sprintf(target_sql, SQL_INSERT_DEPENDENCY_MAP_ITEM, pkg_name, version, token);

        // Add generated SQL to buffer
        db_transaction_sql << target_sql;

        // Get the next remainder
        token = strtok(NULL, splitter);
    }
}

inline void CygpmDatabase::parseDepends2(char *pkg_name, char *version, char *depends2__raw)
{
    /* SQL statements */
    const char *SQL_INSERT_DEPENDENCY_MAP_ITEM = R"(
        INSERT INTO "DEPENDENCY_MAP" (PKG_NAME, VERSION, DEPENDS_ON)
        VALUES ('%s', '%s', '%s');
    )";               // Pre-defined SQL query
    char *target_sql; // Final SQL to generate

    /**
     * Parse depends2__raw
     */
    const char *splitter = ","; // strtok()'s splitter
    char *token;                // Child string

    /* strtok() will modify its source! So we'd better make a copy. */
    char *tmp = new char[strlen(depends2__raw) + 1];
    strcpy(tmp, depends2__raw);

    /* Split the requires__raw */
    token = strtok(tmp, splitter); // Get the first child string

    // Get the remainders
    while (token != NULL)
    {
        // Calculate how much space should target_sql have
        target_sql = new char[100 + strlen(SQL_INSERT_DEPENDENCY_MAP_ITEM) + strlen(pkg_name) + strlen(token)];

        // Form SQL statement
        sprintf(target_sql, SQL_INSERT_DEPENDENCY_MAP_ITEM, pkg_name, version, ltrim(token));

        // Add generated SQL to buffer
        db_transaction_sql << target_sql;

        // Get the next remainder
        token = strtok(NULL, splitter);
    }
}

void CygpmDatabase::initTransaction()
{
    // Clear transaction buffer
    db_transaction_sql.str("");

    // Add init sentence
    db_transaction_sql << "BEGIN;" << endl;
}

int CygpmDatabase::commitTransaction()
{
    // Add commit sentence
    db_transaction_sql << endl
                       << "COMMIT;" << endl;

    // TODO: Allow output SQL queries when debug switch is on
    //cout << db_transaction_sql.str() << endl;

    // Execute SQL
    rc = sqlite3_exec(db, db_transaction_sql.str().c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;

        SQLITE_ERR_RETURN
    }
    else
    {
        cerr << "> Transaction committed" << endl;
    }

    errorLevel = 0;
    return errorLevel;
}

int CygpmDatabase::getErrorLevel()
{
    return errorLevel;
}

int CygpmDatabase::getErrorCode()
{
    return sqlite3_errcode(db);
}

const char *CygpmDatabase::getErrorMsg()
{
    return sqlite3_errmsg(db);
}

int CygpmDatabase::getNumPackages()
{
    return numPackages;
}

int CygpmDatabase::getNumAddedPackages()
{
    // TODO: Use SQL query to calculate how much packages are actually added
    return numAddedPackages;
}