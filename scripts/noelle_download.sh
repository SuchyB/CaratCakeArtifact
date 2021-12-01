#!/bin/bash

fullDirName="$1" ;

dirName="`basename $1`"; 
if test -d "$dirName" ; then
  exit 0 ;
fi

git clone "$fullDirName" "$dirName";
