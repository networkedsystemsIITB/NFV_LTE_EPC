
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <algorithm> //sort()
#include <numeric> //accumulate(v.begin(), v.end(), 0);

#ifndef __TEST_UTILS__
#define __TEST_UTILS__

#define ll long long
#define ull unsigned long long

using namespace std;
using namespace std::chrono;


/*
* Gives Date and Time in given format
* Visit http://en.cppreference.com/w/cpp/chrono/c/strftime for more information about date/time format
* Ref: http://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
*/
const std::string currentDateTime(string fmt="%Y-%m-%d.%X") {
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), fmt.c_str(), &tstruct);
  return buf;
}

/*
* Gives current time in milliseconds since 1st Jan 1970
*/
inline unsigned long long currentMilis(){
  milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
  return ms.count();
}

/*
* Gives current time in microseconds since 1st Jan 1970
*/
inline unsigned long long currentMicros(){
  microseconds ms = duration_cast< microseconds >(system_clock::now().time_since_epoch());
  return ms.count();
}


void pinThreadToCPU(thread *th,int i){
  long num_cpus;
  cpu_set_t cpuset;
  int rc;
  num_cpus = std::thread::hardware_concurrency();
  CPU_ZERO(&cpuset);
  CPU_SET(i%num_cpus, &cpuset);
  rc = pthread_setaffinity_np(th->native_handle(), sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
  }
}


class Measure {
  //private:
public:
  long long min=numeric_limits<long long>::max(),max=0,sum=0,dur;
  double avg;
  long long fcount=0;
  uint64_t st,ed;
  int count=0;
  high_resolution_clock::time_point t1,t2;
  vector<long long> diff_entries;
  long long thread_count=1;
  long long avgthreadruntime;
  long long tput;
  long long run_time;
  // vector<uint64_t> start_entries;//(10000);
  // vector<uint64_t> end_entries;//(10000);
public:
  Measure(long long rt){
    run_time = rt;
    diff_entries.reserve(4e6);
  }

  void reset(){
    min=numeric_limits<long long>::max();
    max=0;
    avg=0;
    sum=0;
    count=0;
    fcount=0;
    diff_entries.clear();
    thread_count = 1;
    // start_entries.clear();
    // end_entries.clear();
  }

  //Though all functions defined within class are by default inline. We have
  // declared inline explicitly here to make a note that these functions must be
  // inline.
  inline void start(){
    t1=high_resolution_clock::now();
  }
  inline void end(){
    //record end time and calc min,max,sum
    t2=high_resolution_clock::now();
    dur = duration_cast<microseconds>(t2 -t1).count();
    // st = duration_cast<microseconds>(t1.time_since_epoch()).count();
    // ed = duration_cast<microseconds>(t2.time_since_epoch()).count();
    diff_entries.push_back(dur);
    // start_entries.push_back(st);
    // end_entries.push_back(ed);
    // count++;// diff_entries.size()
    // sum+=dur;
    // if(min>dur){
    //   min=dur;
    // } else if(max<dur){
    //   max=dur;
    // }
  }

  // double getAvg(){
  //   return double(sum/(double)count);
  // }

  void print(string desc){
      cout<<desc<<endl;
      cout<<"Min\t"<<min<<"us"<<endl;
      cout<<"Max\t"<<max<<"us"<<endl;
      cout<<"Avg\t"<<avg<<"us"<<endl;
      cout<<"Count\t"<<count<<endl;
      cout<<"Fail\t"<<fcount<<endl;
      cout<<"Fail%\t"<<fcount*100/count<<endl;
      cout<<"Tput\t"<<tput<<" ops"<<endl;
      cout<<"BTput\t"<<(count*1e6/avgthreadruntime)<<" ops"<<endl;
    }

  inline void incfcount(){
    fcount++;
  }

  void saveToFile(string desc,string filename,bool overridetime=false){
    ofstream file;
    if(overridetime){
      file.open(filename);
    } else {
      int sep = filename.find_last_of("/\\");
      string dir="";
      string fn="";
      if(sep<0){
        fn=filename;
      } else {
        dir = filename.substr(0,sep+1);
        fn = filename.substr(sep+1);
      }
      file.open(dir+currentDateTime()+"_"+fn);
    }


    vector<long long> sorted_diff = diff_entries;
    sort(sorted_diff.begin(),sorted_diff.end());
    count = diff_entries.size();
    min = sorted_diff[0];
    max = *(sorted_diff.end()-1);
    sum = std::accumulate(sorted_diff.begin(), sorted_diff.end(), 0L);
    avgthreadruntime = sum/thread_count;
    avg = sum/((double)count);
    // tput = (count*1e6/avgthreadruntime);
    tput = (count/run_time);

    file << desc << "\n";
    file << "Min (in microseconds),Max (us),Avg (us),Count,Sum,Failure Count,Throughput\n";
    file << min << "," << max << "," << avg << "," << count << "," << sum << "," << fcount << "," << tput << "\n";
    file << "\n";
    // file << "Duration,Start Time,End Time\n";
    // for(ll i=0;i<diff_entries.size();i++){
    //   file << diff_entries[i] << "," << start_entries[i] << "," << end_entries[i] << "\n";
    // }

    file << "Duration,Sorted duration\n";
    for(ll i=0;i<diff_entries.size();i++){
      file << diff_entries[i] << "," << sorted_diff[i] << "\n";
    }
    file.close();
  }

  void mergeAll(vector<Measure> vm){
    // reset();
    thread_count = vm.size();
    for(int i=0;i<thread_count;i++){
      fcount+=vm[i].fcount;
      // count+=vm[i].count;
      // sum+=vm[i].sum;
      // min = min>vm[i].min ? vm[i].min : min;
      // max = max<vm[i].max ? vm[i].max : max;
      diff_entries.insert(diff_entries.end(), vm[i].diff_entries.begin(), vm[i].diff_entries.end());
    }
  }
};

#endif
