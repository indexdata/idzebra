#!/bin/sh

rm demo/register/* demo/lock/*
../index/zebraidx -c demo/zebra.cfg -g demo1 update /usr/lib/perl5
