#!/usr/bin/bash

PREFIX=aarch64-none-linux-gnu-

if [ $# -eq 0 ]
then
    echo "Expected output directory as the first argument."
    exit 1
fi

OUTPUT_DIR=$1
if [ ! -d $OUTPUT_DIR ] 
then
    echo "Expected output directory as the first argument."
    exit 1
fi

OUTPUT_FILE=${OUTPUT_DIR}/cross_compile.txt
echo $OUTPUT_FILE
if [ -f $OUTPUT_FILE ]
then
    echo "Found $OUTPUT_FILE, removing"
    rm $OUTPUT_FILE
fi

touch ${OUTPUT_DIR}/cross_compile.txt
echo 'Result from -version:' > ${OUTPUT_DIR}/cross_compile.txt
${PREFIX}gcc --version >> ${OUTPUT_DIR}/cross_compile.txt
echo 'Result from -print-sysroot:' >> ${OUTPUT_DIR}/cross_compile.txt
${PREFIX}gcc -print-sysroot >> ${OUTPUT_DIR}/cross_compile.txt
echo 'Result from -v:' >> ${OUTPUT_DIR}/cross_compile.txt
${PREFIX}gcc -v &>> ${OUTPUT_DIR}/cross_compile.txt

