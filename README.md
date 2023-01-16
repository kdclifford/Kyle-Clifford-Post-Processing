# Kyle-Clifford-Post-Processing

# Introduction

This project is my university 3rd year Graphics assignment. The intention of the project was to show off Post processing effects. My favourite being the God rays.
We had to take basic scene project and add new HLSL shaders and add the ability to chain shaders to create new effects. 

# Features

- Ping Ponging (Effect Chaining)
- imgui
- Multiple camera post processing

## Shader effects

- God Rays (Volumetric Lighting)
- Gaussian Blur
- Cell Shaded
- Spiral
- Under water
- Pixelated
- CRT TV 
- Bloom
- And more...

# God Rays (Volumetric Lighting)

My Implementation of God rays required Ping Ponging this because multiple effects were needed to create the rays. Firstly, I needed to discard all the dark areas in the render target which would leave me with the brightest pixels (This is like the Bloom Shader). Secondly the manipulated render target would get passed to the God ray shader which would sample the render target make it bigger and layer it over the original, the number of times its samples/Layers can be set in imgui. Lastly, I added one more shader pass which was Gaussian Blur. This last shader pass was due to being able to see the individual layers of the god rays this could be fixed by increasing the sample size but would cause a lot of hanging and slow down. Or you could do a last shader pass of Gaussian Blur which would blend all the layers together. I added examples below. 

### Without Gaussian blur
[![](https://github.com/kdclifford/Kyle-Clifford-Post-Processing/blob/master/PostProcessingArea/Imgs/GodRaysNoBlur.jpg)](https://youtu.be/P1aXFleQw-s "YouTube Link")

### With Gaussian Blur
[![](https://github.com/kdclifford/Kyle-Clifford-Post-Processing/blob/master/PostProcessingArea/Imgs/GodRaysBlur.jpg)](https://youtu.be/P1aXFleQw-s "YouTube Link")

#Needs turning into a Gif
[![](https://github.com/kdclifford/Kyle-Clifford-Post-Processing/blob/master/PostProcessingArea/Imgs/GodRays.jpg)](https://youtu.be/P1aXFleQw-s "YouTube Link")

YouTube Link: https://youtu.be/P1aXFleQw-s
