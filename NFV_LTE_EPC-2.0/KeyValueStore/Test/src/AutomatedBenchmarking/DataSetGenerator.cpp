
class DataSetGenerator{
public:
  //Generate a random string of length len
  static string getRandomString(int len){
    // string ret(len,'0');
    // for(int i=0;i<len;i++){
    //   ret[i] = '0'+rand()%75;
    // }
    //
    string ret(len,'0');
    for(int i=0;i<len;i++){
      ret[i] = 'a'+rand()%26;
    }
    return ret;
  }

  //Generates sz uniformly random strings for length len
  static vector<string> getRandomStrings(int sz, int len){
    vector<string> ret;
    for(int i=0;i<sz;i++){
      ret.push_back(getRandomString(len));
    }
    return ret;
  }
};
