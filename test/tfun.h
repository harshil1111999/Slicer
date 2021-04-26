#include<stdio.h>
#include<conio.h>
#include<iostream>
#include<omp.h>
#include<vector>
using namespace std;
int fun(int x, int *y)
{
    int k = 10;
    int c = 2;
#pragma omp parallel
    {
        if (x > *y)
        {
            k = x + *y;
            *y = x + *y;
        }
        else
        {
            *y = x * *y;
        }
        #pragma omp barrier
#pragma omp for
        for (int i = 0; i < c; i = i + 1)
        {
            k = k + i;
        }
#pragma omp critical
        {
            x = x + k;
        }
    }
    return x;
}