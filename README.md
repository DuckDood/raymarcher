# OpenGL ray marcher
## Ray marcher written in C, SDL, and OpenGL 4.3

![Hero screenshot of the program](screenshot.png)

## Try it out

Demo video [here](https://duckdood.github.io/terra/raymarcher_new), or try out an older demo version [here](https://duckdood.github.io/terra/raymarcher).

## Building locally (Tested with GCC on GNU/Linux)

You need SDL3 headers.
```
git clone https://github.com/DuckDood/raymarcher.git
cd raymarcher
make
```

This should create a binary in the build/ directory.

## Credits
[Inigo Quilez](https://iquilezles.org/)'s website has a bunch of stuff relating to raymarching, such as smooth functions and distance functions for a ton of primitives, and [Sebastian Lague](https://www.youtube.com/watch?v=Cp5WWtMoeKg)'s raymarching video for an overview on the technique. I also used [LearnOpenGL](https://learnopengl.com/) for some quick reminders about some OpenGL functions and Blinn-Phong lighting, and for learning to use 3D textures and images for the first time.
[Mike Turitzin's video](https://www.youtube.com/watch?v=il-TXbn5iMA) was also a huge inspiration for this project, even if I couldn't get it anywhere near as close to his engine.

## Technical Info
This is a raymarcher that uses a signed distance field (SDF) to march along a ray's path until it collides with something. This allows you to really easily create boolean operations on shapes, and even use smooth versions of them as well. This also uses a system I created that lets you define combinations of objects as data, rather than hardcoding how objects should combine. It also creates shadows by sending a ray in the direction of the sun from the hit point of an object.
It can also cache distance values into a 3D texture and only reevaluate the field where things change.

## AI use
I used AI if I was stuck on a problem to diagnose issues in my code, but it never writes any code, just give me pointers or suggestions and then I implement the logic myself.
