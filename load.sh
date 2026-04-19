make
if [ -e e:/DSKA0009-BAS.HFE ] ; then
 hfe=e:/DSKA0009-BAS.HFE
else
 hfe=d:/DSKA0009-BAS.HFE
fi
python ../../lifutils.py put $hfe ehbasic.bin SYSTEM_ c001
python ../../lifutils.py put $hfe ehbasic.bin ehbasic c001
echo $hfe
