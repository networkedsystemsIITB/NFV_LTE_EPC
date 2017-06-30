g++ -o loader -std=c++11 load_hss.cc -lkvstore -lboost_serialization -Wno-deprecated
sleep 2
./loader 1 <data
./loader <data



