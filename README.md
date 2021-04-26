# Shared Memory C++ OpenMP Slicer

This repository is a repository which implements the various slicing algorithms which can find the program slice of `C++` programs written using `OpenMP` constructs. This repository also has a GUI interface written using `python 3`.

## Installation
The algorithms can be executed in two ways, one using the algorithms file standalone or using the application. In both cases, we need a `GCC C++` Compiler (on Windows system ensure that `pthreads` is installed along with the compiler). \
For executing the application, we would need to ensure certain libraries are installed so the application can be executed. The `requirements.txt` file has all the required libraries. So execute the following command on the command line

```bash
$ py -m pip install -r requirements.txt
```

## Usage
To execute the C++ algorithm files, we can do that by executing the particular algorithm file 
```bash
$ g++ -o test.o algorithm_file.cpp
$ test.o
```
To execute the application move to the `app` folder and execute the python script `app.py`
```bash
$ py app.py
```
## Acknowledgement
We would like to thank Prof. Jayprakash Lalchandani for the guidance and the valuable feedback throughout the project.

## Authors 
* [Harshil Patel](https://github.com/harshil1111999)
* [Rahul Purohit](https://github.com/rahulpurohit29)