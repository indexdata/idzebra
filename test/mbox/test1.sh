# test 1 
pp=${srcdir:-"."}

../../index/zebraidx -c ${pp}/zebra.cfg init
../../index/zebraidx -c ${pp}/zebra.cfg update onemail

