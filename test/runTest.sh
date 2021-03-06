#! /bin/bash

set -u

error_exit()
{
    echo "Error: $1"
    exit 1
}

# Example (inside of 'build' dir):  ../src/test/runTest.sh ./ (Optional: test src file, ex: 'abs.c')

[ $# -ge 1 ] || error_exit "Expected 1 argument(build dir), or more (<build dir> <list of tests to run: c source>), $# passed"

buildDir=$(readlink -f $1)
test -d $buildDir || error_exit "builddir $buildDir inexistent"

testSrcs=""
if [ $# -gt 1 ]; then
    testSrcs=$(printf "${@:2}")
fi

TOPDIR=$(dirname $(readlink -f $0))

cd $buildDir

rm -rf "test" || error_exit "Failed to remove existint 'test' dir"
mkdir -p "test/tmpRunDir" || error_exit "Making 'tmpRunDir'"

cp -r $TOPDIR/operator "test" || error_exit "Failed to copy $TOPDIR/operator into 'test'"

#enter
cd test/tmpRunDir

#-----------------------------------
#CLANGC=clang
#LLVM_DIS=llvm-dis-3.4  #llvm-dis
llvmvers=$($buildDir/../tools/mart -version 2>&1 | grep "LLVM version " | grep -o -E "[0-9].[0-9]")
    
tmpLLVM_COMPILER_PATH=$($buildDir/../tools/mart -version 2>&1 | grep "LLVM tools dir:" | cut -d':' -f2 | sed 's|^ ||g')     #temporary, change this later
CLANGC=$tmpLLVM_COMPILER_PATH/clang-$llvmvers
LLVM_DIS=$tmpLLVM_COMPILER_PATH/llvm-dis-$llvmvers
test -f $LLVM_DIS || LLVM_DIS=$tmpLLVM_COMPILER_PATH/llvm-dis
#------------------------------------

## compile
if [ "$testSrcs" = "" ]; then
    testSrcs=$(find ../operator -type f -name "*.c")
else
    testSrcs=$(printf "$testSrcs" | tr ' ' '\n' | grep -v "^$" | sed 's|^|../operator/|g' | tr '\n' ' ')
fi

for src in $testSrcs
do
    filep=$(basename $src)
    filep=${filep%.c}
    
    trap "echo 'cmd: ../../tools/mart -mutant-config ../operator/mutconf.conf $filep.bc 2>&1'" INT
    
    echo -n "> $filep...  compiling...   "
    $CLANGC -O0 -g -c -emit-llvm -o $filep.bc $src || error_exit "Failed to compile $src"
    echo -n "mutation...   "
    
    options="-print-preTCE-Meta -write-mutants"
    #options="$options -mutant-config $(readlink -f ../operator/mutconf.conf)"
    ( $buildDir/../tools/mart $options $filep.bc 2>&1 ) > $filep.info || { printf "\n---\n"; cat $filep.info; echo "---"; error_exit "mutation Failed for $src. cmd: `readlink -f  $buildDir/../tools/mart` $options $(readlink -f $filep.bc) 2>&1"; }
    mv mart-out-0 $filep-out || error_exit "Failed to store output"
    mv $filep.info $filep-out || error_exit "Failed to move info to output"
    echo "llvm-dis..."
    $LLVM_DIS -o $filep-out/$filep.MetaMu.ll $filep-out/$filep.MetaMu.bc || error_exit "llvm-dis failed on $filep-out/$filep.MetaMu.bc"
    $LLVM_DIS -o $filep-out/$filep.preTCE.MetaMu.ll $filep-out/$filep.preTCE.MetaMu.bc || error_exit "llvm-dis failed on $filep-out/$filep.preTCE.MetaMu.bc"
    test -f $filep-out/$filep.WM.bc && { $LLVM_DIS -o $filep-out/$filep.WM.ll $filep-out/$filep.WM.bc || error_exit "llvm-dis failed on $filep-out/wm-$filep.bc" ; }
done  

