#include "database.h"

const char* DATABASE_NAME = "./cygpm.db";

int main()
{
    CygpmDatabase db(DATABASE_NAME);
    db.createTable();
    db.parseAndBuildDatabase("../test/setup.ini");
}
