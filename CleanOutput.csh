#!/bin/csh -f

echo -n "Are you sure that you want to delete all logs, reports and production ROOT files? (y/n): "
set yn = "$<"

while ( "$yn" != "y" && "$yn" != "n" )
    echo -n "Please answer y or n: "
    set yn = "$<"
end

if ( "$yn" == "y" ) then
    set nonomatch

    rm -f ./csh/*
    rm -f ./err/*
    rm -f ./list/*
    rm -f ./log/*
    rm -f ./out/*
    rm -f ./report/*
    rm -f ./production/*

    rm -f sched*.dataset
    rm -f sched*.session.xml
    rm -f *dataset.tmp
    rm -f D0Standalone*.dataset
    rm -f D0Standalone*.session.xml
    rm -rf D0Standalone*.package

    unset nonomatch
    echo "It is cleaned up."
else
    echo "Ok, I'll leave it alone."
endif
