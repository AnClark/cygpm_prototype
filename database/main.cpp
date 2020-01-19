#include "database.h"
#include <cerrno>
#include <fstream>

const char *DATABASE_NAME = "./cygpm.db";
const char *DATABASE_JOURNAL = "./cygpm.db-journal";

/** Internal functions **/
inline void removeOldDatabase();

int main()
{
    //removeOldDatabase();

    CygpmDatabase db(DATABASE_NAME);
    if (db.getErrorLevel() != 0)
    {
        cerr << "ERROR: Failed to open database. Abort." << endl;
        return -1;
    }

#if 0
    db.createTable();
    db.parseAndBuildDatabase("../test/setup.ini");
    db.buildDependencyMap();

    cout << "Added " << db.getNumPackages() << " packages" << endl;
#endif

#if 0
    cout << db.getNewestVersion("bash") << endl;
    
    vector<string> dl;
    db.findDependencies(dl, "bash", NULL);

    for(auto i = dl.begin(); i != dl.end(); i++)
        cout << *i << ' ';
    cout << endl;
#endif
#define TEST_PACKAGE "python3"

    cout << "SDESC: " << db.getShortDesc(TEST_PACKAGE) << endl;
    cout << "LDESC: " << db.getLongDesc(TEST_PACKAGE) << endl;
    cout << "Category: " << db.getCategory(TEST_PACKAGE) << endl;

    cout << "Install pak path: " << db.getInstallPakPath(TEST_PACKAGE, NULL) << endl;
    cout << "Install pak size: " << db.getInstallPakSize(TEST_PACKAGE, NULL) << endl;
    cout << "Install pak SHA512: " << db.getInstallPakSHA512(TEST_PACKAGE, NULL) << endl;
    cout << "Source pak path: " << db.getSourcePakPath(TEST_PACKAGE, NULL) << endl;
    cout << "Source pak size: " << db.getSourcePakSize(TEST_PACKAGE, NULL) << endl;
    cout << "Source pak SHA512: " << db.getSourcePakSHA512(TEST_PACKAGE, NULL) << endl;

    cout << "Available versions: " << db.getNewestVersion(TEST_PACKAGE) << ' ';
    vector<const char *> pvs = db.getPrevVersions(TEST_PACKAGE);
    for (auto i = pvs.begin(); i != pvs.end(); i++)
        cout << *i << ' ';
    cout << endl;

    return 0;
}

inline void removeOldDatabase()
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