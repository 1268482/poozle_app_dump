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

    ofstream outfile("cleaned.log");
    if (!outfile) return 2;

    ofstream ipfile("crashed_ips.txt");
    if (!ipfile) return 3;

    const string ip_pattern =
        R"(\b([0-9]{1,3}\.){3}[0-9]{1,3}\b)";
    regex ip_re(ip_pattern, regex::ECMAScript);

    string line;
    smatch ip_match;
    while (getline(*inptr, line)) {
        bool has_error_or_critical =
            line.find("ERROR") != string::npos ||
            line.find("CRITICAL") != string::npos;

        if (!has_error_or_critical) {
            outfile << line << '\n';
            continue;
        }

        if (!regex_search(line, ip_re)) {
            outfile << line << '\n';
            continue;
        }
        if (regex_search(line, ip_match, ip_re)) {
            ipfile << ip_match[0] << '\n';
        }
    }

    return 0;
}