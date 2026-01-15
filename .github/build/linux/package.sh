#!/bin/sh
mkdir -p package
cd package
rm -f *
cp ../README.md ./
cp ../LICENSE ./

for f in SOF2MP; do
  cp ../bin/release-$f/x86/sof2gt_qmm_$f.so ./
  cp ../bin/release-$f/x86_64/sof2gt_qmm_x86_64_$f.so ./
done

cd ..
