echo "initializing..."
mkdir -p reg
../../index/zebraidx -l idx.log -c zebra1.cfg init || exit 1
rm -f records/esdd000[12].grs # these should not be here, will be created later
echo "updating..."
../../index/zebraidx -l idx.log -c zebra1.cfg update records  || exit 2
echo "ok"
