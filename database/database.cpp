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
        CREATE TABLE IF NOT EXISTS "PKG_INFO" (
            "PKG_NAME"	TEXT NOT NULL UNIQUE,
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
    int token_type;                      // Lexer token type
    bool is_operating_a_package = false; // Mark that if I'm manipulating a package's info

    int num_packages = 0;

    // Open setup.ini
    yyrestart(fopen(setupini_fileName, "r"));

    // Start parsing
    while (token_type = yylex())
    {
        switch (token_type)
        {
        case T_Package_Name:
            cout << "Package: " << yytext << endl;
            num_packages++;
            
            break;

        case T_YAML_Key:
            
            break;

        case T_Quotation_Mark:

            break;

        case T_Prev_Version_Mark:
            
            break;

        case T_Word:
            
            break;
        }
    }

    cout << "Packages count: " << num_packages << endl;
    return 0;
}


int CygpmDatabase::getErrorLevel()
{
    return errorLevel;
}

int CygpmDatabase::getErrorCode()
{
    return sqlite3_errcode(db);
}

const char* CygpmDatabase::getErrorMsg()
{
    return sqlite3_errmsg(db);
}
