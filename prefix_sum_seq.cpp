#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
int main() {
    int n;
    cout<<"the size of the vector:";cin>>n;
    vector<ll> vec(n);
   cout<<"enter the vector:" ;
   for(int i=0;i<n;++i){cin>>vec[i];}
   vector<ll> p_sum(n,0);
   for(int i=1;i<n;++i){
    vec[i]+=vec[i-1];
   }
cout<<"the prefix sum vector:";
for(int i=0;i<n;++i){
    cout<<vec[i]<<" ";
}

    return 0;
}