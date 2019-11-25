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

CygpmDatabase::CygpmDatabase(const char *fileName)
{
    rc = sqlite3_open(fileName, &db);

    if (rc)
    {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        errorLevel = sqlite3_errcode(db);
    }
    else
    {
        cerr << "Opened database successfully" << endl;
        errorLevel = 0;
    }

    // Constructor doesn't support return values. So use errorLevel instead.
}

CygpmDatabase::~CygpmDatabase()
{
    sqlite3_close(db);
}

int CygpmDatabase::createTable()
{
    /** Create package info table **/

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

    rc = sqlite3_exec(db, SQL_CREATE_PACKAGE_INFO, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);

        SQLITE_ERR_RETURN
    }
    else
    {
        cerr << "Table PKG_INFO created successfully" << endl;
    }

    /** Create dependency table **/

    const char *SQL_CREATE_DEPENDENCY_MAP = R"(
        DROP TABLE IF EXISTS "DEPENDENCY_MAP";

        CREATE TABLE IF NOT EXISTS "DEPENDENCY_MAP" (
	        "PKG_NAME"	TEXT NOT NULL,
	        "DEPENDS_ON"	TEXT NOT NULL,
	        PRIMARY KEY("PKG_NAME"),
	        FOREIGN KEY("PKG_NAME") REFERENCES "PKG_INFO"("PKG_NAME")
        );
    )";

    rc = sqlite3_exec(db, SQL_CREATE_DEPENDENCY_MAP, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << zErrMsg << endl;
        sqlite3_free(zErrMsg);

        SQLITE_ERR_RETURN
    }
    else
    {
        cerr << "Table DEPEDENCY_MAP created successfully" << endl;
    }

    return 0;
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
    CurrentPrevPackageInfo *pkg_info_prev = new CurrentPrevPackageInfo; // Current package's previous version info
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
                if (is_adding_a_YAML_section)
                    submitYAMLItem(last_YAML_section, pkg_info, buff);

                // Now submit our new package info
                insertPackageInfo(pkg_info);
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
            // TODO: Handle previous version item
            if (is_adding_a_previous_version)
                break;

            // Ignore non-package YAML items
            // TODO: Handle setup.ini's metadata
            if (!is_adding_a_package)
                break;

            //cout << "[Current Tag] " << yytext << "; [Last Tag] " << last_YAML_section << endl;

            is_adding_a_YAML_section = true;

            submitYAMLItem(last_YAML_section, pkg_info, buff);

            last_YAML_section = yytext;

            break;

        case T_Prev_Version_Mark:
            //cout << "Found a previous version" << endl;
            is_adding_a_previous_version = true;
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
     */
    if (is_adding_a_package)
    {
        if (is_adding_a_YAML_section)
            submitYAMLItem(last_YAML_section, pkg_info, buff);
        insertPackageInfo(pkg_info);
    }

    // Commit transaction
    commitTransaction();

    return 0;
}

inline void CygpmDatabase::submitYAMLItem(string YAML_section, CurrentPackageInfo *pkg_info, stringstream &buff)
{
#define YAML_SECTION_IS(x) YAML_section.compare(x) == 0

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

void CygpmDatabase::insertPackageInfo(CurrentPackageInfo *packageInfo)
{
    // Pre-define SQL query
    const char *SQL_INSERT_PACKAGE_INFO = R"(
        INSERT INTO "PKG_INFO" (PKG_NAME, SDESC, LDESC, CATEGORY, REQUIRES__RAW, VERSION, 
                                _INSTALL__RAW, INSTALL_PAK_PATH, INSTALL_PAK_SIZE, INSTALL_PAK_SHA512, 
                                _SOURCE__RAW, SOURCE_PAK_PATH, SOURCE_PAK_SIZE, SOURCE_PAK_SHA512, 
                                DEPENDS2__RAW)
        VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s', '%s');
    )";

    // Final SQL to generate
    char *target_sql;

    // Preprocess entries

    // Calculate how much space should target_sql have
    int len_target_sql = 100 + strlen(SQL_INSERT_PACKAGE_INFO) + packageInfo->pkg_name.length() + packageInfo->category.length() + packageInfo->depends2__raw.length() + packageInfo->install__raw.length() + packageInfo->ldesc.length() + packageInfo->requires__raw.length() + packageInfo->sdesc.length() + packageInfo->source__raw.length() + packageInfo->version.length();
    target_sql = new char[len_target_sql];

    sprintf(target_sql, SQL_INSERT_PACKAGE_INFO,
            packageInfo->pkg_name.c_str(),
            packageInfo->sdesc.c_str(),
            packageInfo->ldesc.c_str(),
            packageInfo->category.c_str(),
            packageInfo->requires__raw.c_str(),
            packageInfo->version.c_str(),
            packageInfo->install__raw.c_str(),
            "INSTALL_PAK_PATH",
            "INSTALL_PAK_SIZE",
            "INSTALL_PAK_SHA512",
            packageInfo->source__raw.c_str(),
            "SOURCE_PAK_PATH",
            "SOURCE_PAK_SIZE",
            "SOURCE_PAK_SHA512",
            packageInfo->depends2__raw.c_str());

    db_transaction_sql << target_sql;

#if 0
    /** INSTALL/SOURCE's data format: 
     *     1. Package path: Any characters without space
     *     2. Package size: An integer
     *     3. SHA512 checksum: Only contains [A-Za-z0-9]  TODO: Must have len of 128.
     */
    regex raw_data_splitter(R"(([^ ]+)\s+(\d+)\s+([A-Za-z0-9]+))");
    smatch result;

    char *install_pak_path, *install_pak_size, *install_pak_sha512;
    char *source_pak_path, *source_pak_size, *source_pak_sha512;

    // TODO: Check before parsing!
    // Parse _INSTALL__RAW
    string str_install__raw(install__raw);
    regex_search(str_install__raw, result, raw_data_splitter);

    // Parse _SOURCE__RAW

    char* sql_output;
    sprintf(sql_output, SQL_INSERT_PACKAGE_INFO, 
            packageInfo->)
#endif
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
        sqlite3_free(zErrMsg);

        SQLITE_ERR_RETURN
    }
    else
    {
        cerr << "Transaction committed" << endl;
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