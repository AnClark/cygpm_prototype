#ifndef INSTALLER_H
#define INSTALLER_H

#include "database.h"
#include "utils.h"

/* Public constants */
const char *SETUP_INFO_PATH = "/etc/setup/"; // Path where Cygwin stores installation data

class CygpmInstaller
{
private:
    CygpmDatabase *db;

public:
    CygpmInstaller(CygpmDatabase targetDb);
    ~CygpmInstaller();
    void setDatabase(CygpmDatabase targetDb);

    int installPackage(string pkg_name, string version);
    int uninstallPackage(string pkg_name);
    int alterPackage(string pkg_name, string version);

    int printPackageInfo(string pkg_name);
    int queryInstalledFiles(string pkg_name);

private:
    vector<string> extractFileList(string pkg_name);
};

#endif