#!/bin/sh

set -e

echo "Check dwarfstat for normal binary"
./dwarfstat dwarfzip > /dev/null

echo "Check dwarfzip for normal binary"
./dwarfzip dwarfzip /tmp/dwarfzip.dz

echo "Check dwarfstat for compressed binary"
./dwarfstat /tmp/dwarfzip.dz > /dev/null

echo "Check dwarfzip for compressed binary"
./dwarfzip -d /tmp/dwarfzip.dz /tmp/dwarfzip.orig

echo "Check the integrity"
cmp dwarfzip /tmp/dwarfzip.orig

rm -f /tmp/dwarfzip.dz /tmp/dwarfzip.orig

echo
echo "PASS"
