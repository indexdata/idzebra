rm -f idx.log
echo "init..."
../../index/zebraidx -l idx.log init || exit 1

echo "update 1..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2

echo "update 2..."
../../index/zebraidx -l idx.log -c zebra2.cfg update records || exit 2

echo ok
