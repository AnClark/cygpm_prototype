#include "database.h"

const char *DATABASE_NAME = "./cygpm.db";

int main()
{
    CygpmDatabase db(DATABASE_NAME);
    if (db.getErrorLevel() != 0)
    {
        cerr << "ERROR: Failed to open database. Abort." << endl;
        return -1;
    }

    db.createTable();
    db.parseAndBuildDatabase("../test/setup.ini");
    db.buildDependencyMap();
    db.generateInsSrcData();

    return 0;
}
