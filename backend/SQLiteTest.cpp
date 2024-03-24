#include <iostream>

#include "sqlite/sqlite3.h"

using namespace std;
/*
int main()
{

    sqlite3* dbp = nullptr;
    sqlite3_stmt* curr = nullptr;
    const char* tail = nullptr;
    int rc;


    rc = sqlite3_open("test.db", &dbp);
    if (rc != SQLITE_OK) {
        cout << "Error opening SQLite database: " << sqlite3_errmsg(dbp) << endl;
        sqlite3_close(dbp);
        return 1;
    }
    else {
        cout << "Great success in opening SQLite database!" << endl;
    }

    sqlite3_prepare_v2(dbp,
        u8"CREATE TABLE IF NOT EXISTS test (col1 int, col2 varchar(50))",
        -1, &curr, &tail);

    sqlite3_step(curr);

    sqlite3_finalize(curr);

    sqlite3_prepare_v2(dbp,
        u8"SELECT * FROM test",
        -1, &curr, &tail);

    while (sqlite3_step(curr) == SQLITE_ROW) {
        cout << sqlite3_column_text(curr, 0);
        cout << sqlite3_column_text(curr, 1);
        cout << endl;
    }

    sqlite3_finalize(curr);

    sqlite3_close(dbp);

    return 0;
}
*/
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
