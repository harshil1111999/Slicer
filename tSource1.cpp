#include<stdio.h>
#include<conio.h>
#include<iostream>
#include<omp.h>
using namespace std;
int fun(int x, int *y)
{
    int k = 10;
    int c = 10;
    static int a = 10;
#pragma omp threadprivate(a)
#pragma omp parallel
    {
        a = a + 2;
        if (x > *y)
        {
            k = x + *y;
            *y = x + *y;
        }
        else if (x < *y)
        {
            *y = x * *y;
        }
#pragma omp barrier
#pragma omp for reduction(+:k)
        for (int i = 0; i < 2; i = i + 1)
        {
            k = k + i;
        }
        x = x + k;
    }
    return x;
}

int main()
{
    int x, y, z, n;
    cin >> x;
    cin >> y;
    cin >> z;
    cin >> n;
    int k = 19;
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