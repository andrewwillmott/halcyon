cat VLDefs.h
echo "#include <stdlib.h>"
echo "#include <math.h>"
gcc -E -x c++ -nostdinc -I. -I../h $1 | grep -v "^#.*" 
