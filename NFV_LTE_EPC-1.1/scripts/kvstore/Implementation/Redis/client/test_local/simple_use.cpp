/*
//Uses https://github.com/shawn246/redis_client/
g++ -std=c++11 simple_use.cpp src/RedisClient.cpp -I./test -lhiredis -pthread

g++ -MMD -pg -g -ggdb -O0 -D_DEBUG -D_PRINT_HANDLE_TIME_ -Wno-deprecated  -Wall -m64 -pipe -std=c++11 -I. -I./common -I/usr/local/mysql/include -c ../src/RedisClient.cpp -o ../src/RedisClientd.o
g++ -pg -g -ggdb -O0 -D_DEBUG -D_PRINT_HANDLE_TIME_ -Wno-deprecated  -Wall -m64 -pipe -std=c++11   -Wno-deprecated -o client_debug  maind.o TestBased.o TestClientd.o TestConcurd.o TestGenericd.o TestHashd.o TestListd.o TestSetd.o TestStringd.o TestZsetd.o ../src/RedisClientd.o  -L./lib/linux -lhiredis -lpthread
*/

#include "redis/RedisClient.hpp"
#include <vector>

using namespace std;

int main()
{
    CRedisClient redisCli;
    if (!redisCli.Initialize("127.0.0.1", 7000, 10, 20))
    {
        cout << "connect to redis failed" << endl;
        return -1;
    }

    string strKey = "key_1";
    string strVal;
    redisCli.Set(strKey, "ABCD");
    // redisCli.Del(strKey);


    if (redisCli.Get(strKey, &strVal) == RC_SUCCESS && !strVal.empty())
    {
        cout << "key_1 has value " << strVal << endl;
        //return 0;
    }
    else
    {
        cout << "request failed" << endl;
        //return -1;
    }

    vector<string> key_vec={"key_1","key_1_abc","key_2"};
    vector<string> vvec={"V1","V2",""};
    redisCli.Mset(key_vec,vvec);
    vector<string> val_vec(3);
    redisCli.Mget(key_vec,&val_vec);
    for(auto v:val_vec){
      cout<<v<<endl;
    }
    cout<<"End"<<endl;
}
