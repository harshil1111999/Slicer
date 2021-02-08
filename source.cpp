#include<stdio.h>
#include<conio.h>
#include<iostream>
using namespace std;
int main() 
{
    int x,y,z,n;
    cin>>x;
    cin>>y;
    cin>>z;
    cin>>n;
    int k = 19;
    y = z;
    x = y + z;
    int i;
    for(i=0;i<n;i=i+1)
    {
        cout<<i<<endl;
        x = x + 1;
    }
    cout<<i<<endl;
    if(x > 0)
    {
        y = y + 1;
        cout<<y<<endl;
    }
    else if(x < 0)
    {
        x = x + k;
        cout<<x<<endl;
    }
    z = x + y;
}