
#include <kvstore/KVStoreHeader_v2.h>
#include <unistd.h>
using namespace kvstore;

class Experiment{
private:
  vector<string> key;
  vector<string> value;
  int thread_count;
  int run_time;
  double readp;
  int dataSz;
  vector<Measure> m;
  bool run;
  vector<string> conn;
  int connsz;
  string tablename = "TestTable123_977";
  vector<string> apnd = {"1"};//,"2","3","4","5","6"};
  string dsname = "See folder name";
  int distribution;
  RandomNumberGenerator *rng;
public:
  Experiment(vector<string> &k, vector<string> &v, vector<string> con, int tc, int rt, int dist){
    key = k;
    value = v;
    dataSz = k.size();
    thread_count = tc;
    run_time = rt;
    readp = 0.5;
    m.resize(tc,Measure(run_time));
    run = false;
    conn = con;
    connsz = conn.size();
    distribution = dist;
    rng = new RandomNumberGenerator(0,dataSz-1);
    loadData(); //? to be invoked here or not?
  }

  ~Experiment(){
    delete rng;
  }

  void setRuntime(int rt){
    run_time = rt;
    m.resize(thread_count,Measure(run_time));
  }

  void setThreadCount(int tc){
    thread_count = tc;
    m.resize(tc,Measure(run_time));
  }

  void setData(vector<string> &k, vector<string> &v) {
    key = k;
    value = v;
    dataSz = k.size();
    loadData(); //? to be invoked here or not?
  }

  void loadData(){
    for(int ii=0;ii<6;ii++){
      KVStore<string,string> k;
      KVData<string> kd;
      k.bind(conn[0],tablename+apnd[ii]);
      for(int i=0;i<dataSz;i++){
        kd = k.put(key[i],value[i]);
        if(kd.ierr != 0){
          cerr << "Failure in loading data at index " << i << " Error:" << kd.serr << endl;
        }
      }
    }
  }

  void setReadProb(double rp){
    readp = rp;
  }

  double getReadProb(){
    return readp;
  }

  void worker(int id){
    KVStore<string,string> k;
    k.bind(conn[id%connsz],tablename+apnd[id%6]);

    double rp = readp * RAND_MAX;
    int r1,r2;
    bool succ;
    KVData<string> kd;

    while(!run);
    //TRACE(cout<<"Tid "<<tid<<" started"<<endl;)

    while(run){
      r1 = rand();
      switch (distribution) {
        case 0:
          r2 = rng->uniform();
          break;
        case 1:
          r2 = rng->zipf();
          break;
        default:
          std::cerr << "Invalid value for distribution" << std::endl;
      }
      #ifdef THINKTIME
      usleep(THINKTIME);
      #endif

      if(r1<rp){
        //Do read
        m[id].start();
        kd = k.get(key[r2]);
        m[id].end();
        if(kd.ierr != 0 || kd.value != value[r2]){
          m[id].incfcount();
        }
      } else {
        //Do Write
        m[id].start();
        kd = k.put(key[r2],value[r2]);
        m[id].end();
        if(kd.ierr != 0){
          m[id].incfcount();
        }
      }
    }

    //TRACE(cout<<"Tid "<<tid<<" ended"<<endl;)
  }

  void runExperiment(string filename, string desc_p=""){
    vector<thread> td(thread_count);

    run = false;
    for(int i=0;i<thread_count;i++){
      m[i].reset();
      // KVStore<string,string> k;
      // k.bind(conn,tablename);
      td[i] = thread(&Experiment::worker,this,i);
      pinThreadToCPU(&td[i],i);
    }
    run=true;
    sleep(run_time);
    run=false;
    for (int i = 0; i < thread_count; i++) {
      if (td[i].joinable()) {
        td[i].join();
      }
    }

    string desc = desc_p + string("Data Store,Work Load,Key Size(bytes),Value Size(bytes),Thread Count,Run Time") + string("\n") + string(dsname) +","+to_string(distribution)+" ReadProb:"+to_string(getReadProb())+","+to_string(key[0].size())+","+to_string(value[0].size())+","+to_string(thread_count)+","+to_string(run_time);

    Measure merged(run_time);
    merged.mergeAll(m);
    merged.saveToFile(desc,filename,true);
    merged.print(desc);
  }

};
