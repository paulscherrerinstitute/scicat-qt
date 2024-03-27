# Building

Prerequisites:
```
module load cmake gcc Qt
```

Configure & compile:
```
cd PSIArchiver
cmake ./ -B../release
cd ../release
make -j4
```

# Debugging

For debugging information, remove the line setting `QT_NO_DEBUG_OUTPUT` in
`PSIArchiver/CMakeLists.txt`.