module add cmake/3.15.5
module add gcc/5.5.0

export CMAKE_PREFIX_PATH=/afs/psi.ch/user/w/wakonig_k/public/qt5lib/:$CMAKE_PREFIX_PATH

cmake ./ -B../release

cd ../release
make -j4

module add datacatalog/1.1.9

