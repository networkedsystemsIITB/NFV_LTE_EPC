#include "KVStore.h"
#define ldb_path "10.129.28.188:8090"

using namespace kvstore;
using namespace std;

class Authinfo {
public:
    uint64_t key_id;
    uint64_t rand_num;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

template <class Archive>
void Authinfo::serialize(Archive& ar, const unsigned int version)
{
    ar& key_id& rand_num;
}

int main(int argc, char* argv[])
{

    long value1;
    long value2;
    long value3;
    KVStore<uint64_t, Authinfo> k;
    k.bind(ldb_path, "ds_autn_info");
    bool allok = true;
    while (1) {
        cin >> value1 >> value2 >> value3;

        if (value1 == -25)
            break;

        Authinfo obj;
        obj.key_id = value2;
        obj.rand_num = value3;

        if (argc > 1) {
            //cout << endl
                 //<< "First put()" << endl;
            auto kvd = k.put(value1, obj);
            if (kvd.ierr < 0) {allok = false;
                cout << "Error in putting data" << endl
                     << kvd.serr << endl;
            }
            else {
               // cout << "Data written successfully" << endl;
            }
        }
        else {

            auto kvd = k.get(value1);
            if (kvd.ierr < 0) {
                cout << "Error in getting data" << endl
                     << kvd.serr << endl;allok = false;
            }
            else {
                Authinfo obj2 = kvd.value;

                if (obj2.key_id == obj.key_id && obj2.rand_num == obj.rand_num) {
                   // cout << "Read OK" << endl;
                }
                else {
                    cout << "error" << endl;
                    break;
                }
            }
        }
    }
  if( allok &&  argc == 1 ) cout<<"Data inserted and checked successfully"<<endl;
}
