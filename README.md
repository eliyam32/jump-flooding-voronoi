Jump Flooding Voronoi
=====================
**Author:** Renato Farias (renatomdf@gmail.com)  
**Created on:** April 13th, 2012  
**Last updated on:** June 13th, 2012  
**Purpose:** To implement the Jump Flooding algorithm and use it to generate Voronoi diagrams. This technique is from the paper "Jump Flooding in GPU With Applications to Voronoi Diagram and Distance Transform" [Rong 2006].  

**CPU Implementation**  
The CPU implementation is simpler by far, but very slow.  
- The left mouse button places seeds. (If a Voronoi diagram has already been generated, you must clear it before placing new seeds.)  
- The right mouse button opens the pop-up menu.  
- 'c' clears the current diagram (if it has been created) and the seeds.  
- 'e' executes the Jump Flooding algorithm.  
- 'f' enters and leaves fullscreen mode.  

**GPU Implementation**  
The GPU implementation uses render-to-texture and shaders. It is fast enough to continuously update the Voronoi diagram when we apply a velocity to each seed so that it moves about the screen, which is nice to look at.  
- The right mouse button opens the pop-up menu.  
- 'r' generates a new set of random seeds.  
- If run from the command line, the first parameter can be used to specify how many seeds to generate. (This number is overriden if you regenerate later.)  
