#include <iostream>
#include "Scanner.h"
#include "tokens.h"

using namespace std;

int main()
{
    int numPackages = 0;

    Scanner scanner; // define a Scanner object

    while (int token = scanner.lex()) // get all tokens
    {
        string const &text = scanner.matched();
        switch (token)
        {
        case T_Package_Name:
            cout << "====================================================" << endl;
            cout << "Get package: " << text << endl;
            numPackages++;
            break;

        case T_YAML_Key:
            cout << "Get YAML key: " << text << endl;
            break;

        case T_Prev_Version_Mark:
            cout << "* Get a previous version" << endl;
            break;

        case T_Word:
            cout << "WORD: " << text << endl;
            break;
        }
    }

    cout << endl << "Package count: " << numPackages << endl;
}
