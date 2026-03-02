#include <bits/stdc++.h>

using namespace std;

int main()
{
    ifstream arquivo("input.txt");
    if (!arquivo.is_open()) return 1;

    char c;
    while (arquivo.get(c)) {
        cout << c;
    }
    cout << endl;

    arquivo.close();
    return 0;
}
