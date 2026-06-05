#include <bits/stdc++.h>
using namespace std;

int main(int argc, char* argv[]) {
    istream* inptr = &cin;
    ifstream infile;
    if (argc > 1) {
        infile.open(argv[1], ios::in);
        if (!infile) return 1;
        inptr = &infile;
    }

    const string xml_pattern =
        R"(<([A-Za-z_][A-Za-z0-9_]*)>([^<]*)</\1>)";
    regex xml_re(xml_pattern, regex::ECMAScript);

    string line;
    smatch m;
    while (getline(*inptr, line)) {
        string::const_iterator searchStart(line.cbegin());
        while (regex_search(searchStart, line.cend(), m, xml_re)) {
            cout << m[0] << '\n';
            searchStart = m.suffix().first;
        }
    }

    return 0;
}