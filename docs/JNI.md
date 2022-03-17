# JNI

## Changing JNI Code

1. Change Java Code
2. Compile Header with `gradle compileJava`
3. Copy Header to C++ JNI Folder and implement the code
4. Create Libraray from that C++ code -> Java code will import it that way
5. To use with YCSB, copy changed Java Code to `/ycsb/drdb/src`
