#include <bits/stdc++.h>
using namespace std;

int main(int argc, char* argv[]) {
    string input;
    string line;
    while (getline(cin, line)) {
        input += line;
        input += '\n';
    }

    const string pattern =
        R"(\b(?![A-Za-z0-9._%+-]+@(?:([A-Za-z0-9-]+\.)*spam\.com)\b)([A-Za-z0-9._%+-]+(?:\+[A-Za-z0-9._%+-]+)?@[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)*\.[A-Za-z]{2,})\b)";

    regex re;
    try {
        re = regex(pattern, regex::ECMAScript | regex::icase);
    } catch (const regex_error&) {
        return 2;
    }

    unordered_set<string> seen;
    vector<string> emails;
    smatch m;
    string::const_iterator searchStart(input.cbegin());

    while (regex_search(searchStart, input.cend(), m, re)) {
        string email = m[2].str();
        for (auto &ch : email) ch = static_cast<char>(tolower((unsigned char)ch));

        if (seen.insert(email).second) {
            emails.push_back(email);
            cout << email << '\n';
        }

        searchStart = m.suffix().first;
    }

    return 0;
}