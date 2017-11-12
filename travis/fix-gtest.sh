# build gtest (https://askubuntu.com/questions/145887/why-no-library-files-installed-for-google-test?newreg=b9a4644b541d4d99aac52be1822d9b2b)
mkdir /tmp/.build
cd /tmp/.build
cmake -DCMAKE_BUILD_TYPE=RELEASE /usr/src/gtest/
make
sudo mv libg* /usr/lib/
rm -rf /tmp/.build
