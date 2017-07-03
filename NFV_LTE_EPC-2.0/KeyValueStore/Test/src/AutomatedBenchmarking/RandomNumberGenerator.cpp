
#include <cstdlib> //rand()
#include <cmath>

class RandomNumberGenerator{
private:
 vector<double> cdf;
 int start;
 int end;
 public:
   RandomNumberGenerator(int s,int n){
    start = s;
    end = n;
    //For zipfain
    int N = end-start+1;
    double sum = 0;
    for(double i=1;i<=N;i+=1){
     sum+=1/i;
     cdf.push_back(sum);
    }
    //Normalize
    for(int i=0;i<N;i++){
     cdf[i] = cdf[i]/sum;
    }
   }
  long long uniform(){
    int diff=end-start+1;
    return rand()%diff+start;
  }
  long long zipf(){
    double z = double(rand())/RAND_MAX;
    return std::lower_bound(cdf.begin(), cdf.end(),z) - cdf.begin() + start;
  }

  // static int zipfain(double alpha, int n)
  // {
  //   //http://www.csee.usf.edu/~kchriste/tools/toolpage.html
  //   //http://www.csee.usf.edu/~kchriste/tools/genzipf.c
  //   int TRUE = 1;
  //   int FALSE = 0;
  //   static int first = TRUE;      // Static first time flag
  //   static double c = 0;          // Normalization constant
  //   double z;                     // Uniform random number (0 < z < 1)
  //   double sum_prob;              // Sum of probabilities
  //   double zipf_value;            // Computed exponential value to be returned
  //   int    i;                     // Loop counter
  //
  //   static int N=0;
  //   if(N!=n){
  //     first = TRUE;
  //   }
  //   // Compute normalization constant on first call only
  //   if (first == TRUE)
  //   {
  //     N=n;
  //     c=0;
  //     for (i=1; i<=n; i++)
  //     first = FALSE;
  //   }
  //
  //   // Pull a uniform random number (0 < z < 1)
  //   do
  //   {
  //     z = double(rand())/RAND_MAX;
  //   }
  //   while ((z == 0) || (z == 1));
  //
  //   // Map z to the value
  //   sum_prob = 0;
  //   for (i=1; i<=n; i++)
  //   {
  //     sum_prob = sum_prob + c / pow((double) i, alpha);
  //     if (sum_prob >= z)
  //     {
  //       zipf_value = i;
  //       break;
  //     }
  //   }
  //   return(zipf_value);
  // }
};

// int main(){
//   return 0;
// }
