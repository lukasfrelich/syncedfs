#!/bin/bash

mkdir -p /var/run/syncedfs/fs
mkdir -p /var/run/syncedfs/client
mkdir -p /var/run/syncedfs/server

chown -R lfr.lfr /var/run/syncedfs
