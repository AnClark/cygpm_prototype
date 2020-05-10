#include "database.h"
#include <cerrno>
#include <fstream>
#include "utils.h"

const char *DATABASE_NAME = "./cygpm.db";
const char *DATABASE_JOURNAL = "./cygpm.db-journal";

void removeOldDatabase();

int main()
{
    //removeOldDatabase();

    CygpmDatabase db(DATABASE_NAME);
    if (db.getErrorLevel() != 0)
    {
        cerr << "ERROR: Failed to open database. Abort." << endl;
        return -1;
    }
#if 1
    db.createTable();
    db.parseAndBuildDatabase("../test/setup.ini");
    db.buildDependencyMap();

    cout << "Added " << db.getNumPackages() << " packages" << endl;
#endif

    cout << db.getInstallPakSHA512("bash", NULL) << endl;
    cout << calculateFileSHA512("../test/bash-4.4.12-3.tar.xz") << endl;

    //cout << decompressGzipFileData("../test/cmake.lst.gz") << endl;

    return 0;
}

void removeOldDatabase()
{
    cerr << "Removing old database and journal" << endl;

    if (isFileExist(DATABASE_NAME)) // Remove database file
    {
        if (remove(DATABASE_NAME) == 0)
            cerr << "Removed old database" << endl;
        else
        {
            cerr << "ERROR: Failed to remove database. Will use current database" << endl;
            return; // Keep journal if database still exist
        }
    }
    if (isFileExist(DATABASE_JOURNAL)) // Remove SQLite database journal
    {
        if (remove(DATABASE_JOURNAL) == 0)
            cerr << "Removed old journal" << endl;
        else
            cerr << "ERROR: Failed to remove journal" << endl;
    }
}