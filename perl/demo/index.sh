#!/bin/sh

rm demo/register/* demo/lock/*
../index/zebraidx -c demo/zebra.cfg -g demo1 -v 255 update lib
