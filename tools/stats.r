#! /usr/bin/env Rscript
d<-scan("stdin", quiet=TRUE)
cat(min(d), max(d), median(d), mean(d), sd(d), sep=",")