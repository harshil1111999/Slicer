#include<stdio.h>
#include <string>
#include<conio.h>
#include<iostream>
#include<omp.h>
#include<vector>
using namespace std;
float subdomain(int istart, int ipoints)
{
    float x = 0;
    int i;
    for (i = 0; i < ipoints; i++)
    {
        x = x + 123.456;
    }
    return x;
}

float sub(int npoints)
{
    float x = 1;
    int iam, nt, ipoints, istart;
    omp_set_num_threads(2);
#pragma omp parallel private(iam,nt,ipoints,istart)
    {
        iam = omp_get_thread_num();
        nt = omp_get_num_threads();
        // size of partition
        ipoints = npoints / nt;
        // starting array index
        istart = iam * ipoints;
        // last thread may do more
        if (iam == nt - 1)
        {
            ipoints = npoints - istart;
        }
        x = subdomain(istart, ipoints);
    }
    return x;
}
int main()
{
    float x;
    int c;
    cin>>c;
    x = sub(c);
    cout<<x<<endl;
    return 0;
}