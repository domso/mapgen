# Heightmap-Generator

Generates random-heightmaps for dynammo (https://github.com/domso/dynammo).

The fundamental idea behind this generator is to place multiple hills/valleys
randomly on the map. A hill is a cone with random diameter and height, while
a valley is just an inverted hill.

Heightmap with 1 hill:
![0](https://github.com/domso/mapgen/blob/master/steps/0.png)
![0render](https://github.com/domso/mapgen/blob/master/steps/0_render.png)

Heightmap after 1000 iterations applied on a random initialized map:
![1](https://github.com/domso/mapgen/blob/master/steps/1.png)
![1render](https://github.com/domso/mapgen/blob/master/steps/1_render.png)

This is the raw-heightmap.

The next step is to use gaussian-blur to reduce the overall noise and
smooth the map.

Heightmap after applying gaussian-blur:
![2](https://github.com/domso/mapgen/blob/master/steps/2.png)
![2render](https://github.com/domso/mapgen/blob/master/steps/2_render.png)

To make the map more realistic, the generator simulates erosion. This is done
by finding the smallest neighbor of every point and reduce the height of both 
points. Combined with a random error the resulting heightmap looks like this:

Heightmap after applying erosion:
![3](https://github.com/domso/mapgen/blob/master/steps/3.png)
![3render](https://github.com/domso/mapgen/blob/master/steps/3_render.png)

The last step is to use gaussian-blur again.

Final heightmap:
![4](https://github.com/domso/mapgen/blob/master/steps/4.png)
![4render](https://github.com/domso/mapgen/blob/master/steps/4_render.png)



Note:
The heightmaps were rendered using blender.
