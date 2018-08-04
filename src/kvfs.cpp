#include <kvfs/kvfs.h>

using namespace rocksdb;
using namespace std;

int main(int argc, char *argv[])
{
    DB *db;
    Options options;
    options.create_if_missing = true;
    Status status = DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());
    options.error_if_exists = true;
    string value = "F";
    Slice key1;
    Slice key2;
    Status s = db->Get(rocksdb::ReadOptions(), key1, &value);
    if (s.ok())
        s = db->Put(rocksdb::WriteOptions(), key2, value);
    if (s.ok())
        s = db->Delete(rocksdb::WriteOptions(), key1);

    cout << "Hello" << endl;

    delete db;
    return 0;
}