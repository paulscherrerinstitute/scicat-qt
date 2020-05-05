module add cmake/3.15.5
module add gcc/5.5.0

cmake ./ -B../release

cd ../release
make

