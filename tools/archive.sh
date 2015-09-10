#!/bin/sh

course="t-110-5150"
year=2014
nprojects=26

projects=""
for i in `seq -f %02g 1 $nprojects`; do
    projects="$projects group${i}_${year}"
done

# cloning
succs=""
fails=""
for project in $projects; do
    cmd="git clone git@git.niksula.hut.fi:${course}/${project}.git"
    echo $cmd
    $cmd
    if [ $? -eq 0 ]; then
        succs="$succs $project"
    else
        fails="$fails $project"
    fi
    echo
done

if [ -n "$fails" ]; then
    echo "\nFollowing groups are failed when cloning from git:"
    echo $fails
fi

# archiving
if [ -n "$succs" ]; then
    dir=${course}_${year}
    mkdir $dir
    mv $succs $dir
    tar zcf ${dir}.tar.gz $dir
    rm -fr $dir
fi

