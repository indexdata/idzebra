../../index/zebraidx -l idx.log init || exit 1
rm -f records/esdd000[12].grs
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 2
