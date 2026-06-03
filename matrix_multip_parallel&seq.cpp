#include <bits/stdc++.h>
#include <thread>
using namespace std;

using Mat = vector<vector<long long>>;

void checkMat(const Mat& a, const Mat& b) {
    if (a.empty() || b.empty()) throw invalid_argument("matrix empty");
    if (a[0].empty() || b[0].empty()) throw invalid_argument("matrix empty");

    int ca = a[0].size();
    for (auto& r : a)
        if ((int)r.size() != ca) throw invalid_argument("matrix a ragged");

    int cb = b[0].size();
    for (auto& r : b)
        if ((int)r.size() != cb) throw invalid_argument("matrix b ragged");

    if (ca != (int)b.size())
        throw invalid_argument("cannot multiply: a.cols != b.rows");
}

Mat readMat(int r, int c, const string& nm) {
    Mat m(r, vector<long long>(c));
    cout << "Enter " << nm << " (" << r << " x " << c << "):\n";
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) cin >> m[i][j];
    }
    return m;
}

void printMat(const Mat& m) {
    for (auto& r : m) {
        for (auto x : r) cout << x << ' ';
        cout << '\n';
    }
}

// row-wise work for a thread
void rowMul(const Mat& a, const Mat& b, Mat& c, int l, int r) {
    int p = a[0].size();
    int n = b[0].size();

    for (int i = l; i < r; i++) {
        for (int j = 0; j < n; j++) {
            long long s = 0;
            for (int k = 0; k < p; k++) s += a[i][k] * b[k][j];
            c[i][j] = s;
        }
    }
}

// naive O(n^3) sequential
Mat mulSeq(const Mat& a, const Mat& b) {
    checkMat(a, b);

    int m = a.size(), p = a[0].size(), n = b[0].size();
    Mat c(m, vector<long long>(n, 0));

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            long long s = 0;
            for (int k = 0; k < p; k++) s += a[i][k] * b[k][j];
            c[i][j] = s;
        }
    }
    return c;
}

// row-wise parallel using std::thread
Mat mulPar(const Mat& a, const Mat& b, int tno) {
    checkMat(a, b);

    int m = a.size(), n = b[0].size();
    Mat c(m, vector<long long>(n, 0));

    tno = max(1, min(tno, m));
    int per = (m + tno - 1) / tno;

    vector<thread> th;
    th.reserve(tno);

    for (int t = 0; t < tno; t++) {
        int l = t * per;
        int r = min(m, l + per);
        if (l >= m) break;
        th.emplace_back(rowMul, cref(a), cref(b), ref(c), l, r);
    }

    for (auto& x : th) x.join();
    return c;
}

int main() {
    try {
        int r1, c1, r2, c2;
        cout << "Enter rows and cols of matrix A: ";
        cin >> r1 >> c1;
        cout << "Enter rows and cols of matrix B: ";
        cin >> r2 >> c2;

        Mat a(r1, vector<long long>(c1));
        Mat b(r2, vector<long long>(c2));

        cout << "Enter matrix A:\n";
        for (int i = 0; i < r1; i++)
            for (int j = 0; j < c1; j++)
                cin >> a[i][j];

        cout << "Enter matrix B:\n";
        for (int i = 0; i < r2; i++)
            for (int j = 0; j < c2; j++)
                cin >> b[i][j];

        checkMat(a, b);

        int tno = thread::hardware_concurrency();
        if (!tno) tno = 4;

        // sequential multiply + output right after
        Mat cs = mulSeq(a, b);
        cout << "\nSequential result:\n";
        printMat(cs);

        // parallel multiply + output right after
        Mat cp = mulPar(a, b, tno);
        cout << "\nParallel result:\n";
        printMat(cp);
    }
    catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
    }
}