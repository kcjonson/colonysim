


Input World Properties
* Star being orbited
    -Mass
    -Radius
    -Temperature
    -Age
* Orbit around star - will influence day cycle and create weather on the planet
* Radius
* Mass
* Rate of rotation
* Number of plates - to calculate tectonic plates and generate mountains
* Amount of water - to calculate water features on surface
* Atmosphere strength -  to calculate heat
* Planet age

Input Generator properties
* Title size - this will determine the resolution of terrain sectors when diving the planet sphere. It should be a constant with a range a few thousand to millions
* Seed - the seed will be used to apply randomness 


Generation phases
1) Divide the sphere into a number of plates
2) Assign these plates movement directions and speeds based on real geological principles observed on earth. 
3) Analyze how the plates are interacting to determine if they’re creating Convergent Boundaries, Divergent Boundaries or Transform Boundaries. Store this information about the shared edge between the plates for future use.
4) Divide the world sphere into tiles and use the plate information to determine the average height of that tile. This should result in natural looking mountain ranges and valleys. TODO: determine shape of the tiles? For the game we’ll need sectors to be either hexagons or squares. Not sure how to do this mapping. Store the set of tiles with their heights for the next step
5) Add water to the simulation to create oceans (no rivers yet)
6) Calculate atmospheric circulation patterns for the planet. These winds will determine where rain falls. TODO: should these be stored on the tiles or in their own layer?
7) calculate where rain falls on the planet simulate and create rivers and lakes. This should also wear down the mountains so there are natural ridgelines like real mountains have
