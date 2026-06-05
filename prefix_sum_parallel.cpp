#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
using namespace std;

typedef long long ll;

void local_sum(vector<int>& vec, int l, int r, int& sum) {
    for (int i = l + 1; i < r; ++i) {
        vec[i] += vec[i - 1];
    }
    sum = vec[r - 1];
}

void offset_fun(vector<int>& a, int l, int r, int off) {
    for (int i = l; i < r; ++i) {
        a[i] += off;
    }
}

int main() {
    int n;
    cin >> n;
    vector<int> v(n);
    for (int i = 0; i < n; ++i) {
        cin >> v[i];
    }

    int noft = 4; // number of threads
    int b_size = (n + noft - 1) / noft; // blocksize
    vector<int> b_sum(noft, 0);
    vector<thread> threads;

    for (int i = 0; i < noft; ++i) {
        int l = i * b_size;
        int r = min(n, l + b_size);
        threads.emplace_back(local_sum, ref(v), l, r, ref(b_sum[i]));
    }

    for (thread& t : threads) t.join();

    vector<int> offset(noft, 0);
    for (int i = 1; i < noft; ++i) {
        offset[i] = offset[i - 1] + b_sum[i - 1];
    }

    threads.clear();
    for (int i = 1; i < noft; ++i) {
        int l = i * b_size;
        int r = min(n, l + b_size);
        threads.emplace_back(offset_fun, ref(v), l, r, offset[i]);
    }

    for (thread& t : threads) t.join();

    for (int s : v) cout << s << " ";

    return 0;
}