# Heightmap Generator

A tool to generate large (10k x 10k) heightmaps with rivers and biomes.

## How it works

The generation happens in 5 steps:

### 1. Base Noise
Takes small random height patches (hills and valleys) and combines them into a large noise map.

### 2. Shape Detection
Uses thresholds on the noise to get basic terrain shapes.

### 3. Terrain Generation
Applies simple erosion by simulating water flow on the shapes.

### 4. Rivers & Lakes
- Makes river regions using Voronoi
- Computes river paths
- Adds some lakes
- Adjusts terrain so rivers flow downhill

### 5. Biomes
Combines:
- Temperature 
- Moisture 
- Height
to create biome regions.

## Output

<p float="left">
  <img src="/example/terrain.png" width="45%"/>
  <img src="/example/rivers.png" width="45%"/> 
</p>

<p float="left">
  <img src="/example/cells.png" width="45%"/>
  <img src="/example/altitude.png" width="45%"/> 
</p>

<p float="left">
  <img src="/example/temperature.png" width="45%"/>
  <img src="/example/moisture.png" width="45%"/> 
</p>
