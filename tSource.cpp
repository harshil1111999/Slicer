#include<stdio.h>
#include<conio.h>
#include<iostream>
#include<omp.h>
#include<vector>
#include"tfun.h"
using namespace std;

int main()
{
    int x, y, z, n;
    cin >> x;
    cin >> y;
    cin >> z;
    cin >> n;
    int k = 19;
    string temp;
    temp = to_string(k);
    cout<<temp<<endl;
    y = z;
    x = y + z;
    for (int i = 0; i < n; i = i + 1)
    {
        x = fun(x, &y);
        cout << i << endl;
        x = x + 1;
    }
    cout << x << endl;
    if (x > 0)
    {
        y = y + 1;
        cout << y << endl;
    }
    else if (x < 0)
    {
        x = x + k;
        cout << x << endl;
    }
    z = x + y;
}