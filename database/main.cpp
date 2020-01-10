#include "database.h"
#include <cerrno>
#include <fstream>

const char *DATABASE_NAME = "./cygpm.db";
const char *DATABASE_JOURNAL = "./cygpm.db-journal";

void removeOldDatabase();

int main()
{
    removeOldDatabase();

    CygpmDatabase db(DATABASE_NAME);
    if (db.getErrorLevel() != 0)
    {
        cerr << "ERROR: Failed to open database. Abort." << endl;
        return -1;
    }

    db.createTable();
    db.parseAndBuildDatabase("../test/setup.ini");
    db.buildDependencyMap();

    cout << "Added " << db.getNumPackages() << " packages" << endl;

    return 0;
}

void removeOldDatabase()
{
    cerr << "Removing old database and journal" << endl;

    ifstream ftest_dbname(DATABASE_NAME), ftest_dbjournal(DATABASE_JOURNAL);

    if (ftest_dbname) // Remove database file
    {
        ftest_dbname.close();

        if (remove(DATABASE_NAME) == 0)
            cerr << "Removed old database" << endl;
        else
        {
            cerr << "ERROR: Failed to remove database. Will use current database" << endl;
            return; // Keep journal if database still exist
        }
    }
    if (ftest_dbjournal) // Remove SQLite database journal
    {
        ftest_dbjournal.close();

        if (remove(DATABASE_JOURNAL) == 0)
            cerr << "Removed old journal" << endl;
        else
            cerr << "ERROR: Failed to remove journal" << endl;
    }
}