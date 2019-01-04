// 指定使用的数据库
#include <iostream>
#include <unistd.h>
using namespace std;
int main(int argc, char **argv) {
    char *dbname;
    char command[80] = "chdir ";
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }
    // The database name is the second argument
    dbname = argv[1];
    // Create a subdirectory for the database
    system(strcat(command, dbname));
    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }
    // Create the system catalogs

}