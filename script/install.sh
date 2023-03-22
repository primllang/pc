export DESTDIR=${1:-pcdeploy}
cmake --build ./build -- install
