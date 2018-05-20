#!/bin/bash

for i in {0..99}
do
	../build/mapgen
	convert export.ppm export_$i.png
done

rm export.ppm
