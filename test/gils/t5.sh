rm -f idx.log
../../index/zebraidx -l idx.log init || exit 1
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2
