points=0
nb_test=0

echo "Good :" > test/report.txt
for file in test/good/* ; do
let nb_test=nb_test+1
./bin/tpcc < $file
res=$?
echo $file $res >> test/report.txt
if [ $res -eq 0 ]
then
    let points=points+1
fi
done

echo "" >> test/report.txt
echo "Syn-err :" >> test/report.txt
for file in test/syn-err/* ; do
let nb_test=nb_test+1
./bin/tpcc < $file
res=$?
echo $file $res >> test/report.txt
if [ $res -eq 1 ]
then
    let points=points+1
fi
done

echo "" >> test/report.txt
echo "Sem-err :" >> test/report.txt
for file in test/sem-err/* ; do
let nb_test=nb_test+1
./bin/tpcc < $file
res=$?
echo $file $res >> test/report.txt
if [ $res -eq 2 ]
then
    let points=points+1
fi
done

echo "" >> test/report.txt
echo "Warn :" >> test/report.txt
for file in test/warn/* ; do
let nb_test=nb_test+1
./bin/tpcc < $file
res=$?
echo $file $res >> test/report.txt
if [ $res -eq 0 ]
then
    let points=points+1
fi
done

echo "" >> test/report.txt
echo "Score : " $points "/" $nb_test >> test/report.txt