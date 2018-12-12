
#!/bin/bash
minimum="$(head -n 1 1dtmp.txt)"
awk -v m="$minimum" '{ print m/$1 }' 1dtmp.txt > 1doutputtmp.txt
cat -n 1doutputtmp.txt > 1doutput.txt #add numbering for cores
rm 1doutputtmp.txt
