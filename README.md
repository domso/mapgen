# Heightmap-Generator

Generates random-heightmaps for dynammo (https://github.com/domso/dynammo).
Its a reimplementation of an old FreeBASIC-program I wrote years ago.

The fundamental idea behind this generator is to place multiple hills/valleys
randomly on the map. A hill is a cone with random diameter and height, while
a valley is just an inverted hill.

Heightmap with 1 hill:
<p float="left">
  <img src="/steps/0.png" width="45%"/>
  <img src="/steps/0_render.png" width="45%"/> 
</p>

Heightmap after 1000 iterations applied on a random initialized map:
<p float="left">
  <img src="/steps/1.png" width="45%"/>
  <img src="/steps/1_render.png" width="45%"/> 
</p>

This is the raw-heightmap.

The next step is to use gaussian-blur to reduce the overall noise and
smooth the map.

Heightmap after applying gaussian-blur:
<p float="left">
  <img src="/steps/2.png" width="45%"/>
  <img src="/steps/2_render.png" width="45%"/> 
</p>

To make the map more realistic, the generator simulates erosion. This is done
by finding the smallest neighbor of every point and reduce the height of both 
points. Combined with a random error the resulting heightmap looks like this:

Heightmap after applying erosion:
<p float="left">
  <img src="/steps/3.png" width="45%"/>
  <img src="/steps/3_render.png" width="45%"/> 
</p>

The last step is to use gaussian-blur again.

Final heightmap:
<p float="left">
  <img src="/steps/4.png" width="45%"/>
  <img src="/steps/4_render.png" width="45%"/> 
</p>

# Water Simulation

Using the heightmap, a watermap can be created.

Find the primary rivers by using erosion again:
<p float="left">
  <img src="/steps/water_1.png" width="45%"/>
  <img src="/steps/water_render_1.png" width="45%"/> 
</p>

After removing some smaller rivers:
<p float="left">
  <img src="/steps/water_2.png" width="45%"/>
  <img src="/steps/water_render_2.png" width="45%"/> 
</p>

Increase the width of each river:
<p float="left">
  <img src="/steps/water_3.png" width="45%"/>
  <img src="/steps/water_render_3.png" width="45%"/> 
</p>

Set the correct height:
<p float="left">
  <img src="/steps/water_4.png" width="45%"/>
  <img src="/steps/water_render_4.png" width="45%"/> 
</p>

Apply gaussian blur to the surrounding pixels:
<p float="left">
  <img src="/steps/water_5.png" width="45%"/>
  <img src="/steps/water_render_5.png" width="45%"/> 
</p>

Add a baselevel of water:
<p float="left">
  <img src="/steps/water_6.png" width="45%"/>
  <img src="/steps/water_render_6.png" width="45%"/> 
</p>

And the final result:
<p float="left">
  <img src="/steps/water_render_7.png" width="100%"/> 
</p>

Note:
The heightmaps were rendered using blender.
