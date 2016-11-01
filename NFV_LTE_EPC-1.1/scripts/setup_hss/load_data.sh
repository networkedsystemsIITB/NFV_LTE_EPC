g++ -std=c++11 fill_rc.cc -lkvstore -lboost_serialization -Wno-deprecated
sleep 2
./a.out 1 <data
./a.out <data



