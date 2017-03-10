Subject 	: CSCI420 - Computer Graphics 
Assignment 2: Simulating a Roller Coaster
Author		: Eric Hsieh
USC ID 		: 5933583825

Description: In this assignment, we use Catmull-Rom splines along with OpenGL texture mapping to create a roller coaster simulation.

Core Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Uses OpenGL core profile, version 3.2 or higher - Y

2. Completed all Levels:
  Level 1 : - Y
  level 2 : - Y
  Level 3 : - Y
  Level 4 : - Y
  Level 5 : - Y

3. Used Catmull-Rom Splines to render the Track - Y

4. Rendered a Rail Cross Section - Y

5. Rendered the camera at a reasonable speed in a continuous path/orientation - Y

6. Run at interactive frame rate (>15fps at 1280 x 720) - Y

7. Understandably written, well commented code - Y

8. Attached an Animation folder containing not more than 1000 screenshots - Attached a video instead

9. Attached this ReadMe File - Y

Extra Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Render a T-shaped rail cross section - N

2. Render a Double Rail - Y

3. Made the track circular and closed it with C1 continuity - N

4. Added OpenGl lighting - N

5. Any Additional Scene Elements: Wooden Textured Rail Crossbeams

6. Generate track from several sequences of splines - N

7. Draw splines using recursive subdivision - Draw splines based on a rough sampling of their arc distance from one another (instead of using brute force). Chose to use an iterative solution instead of a recursive one to optimize memory usage. 

8. Modify velocity with which the camera moves - Constant velocity based on a rough "actual distance traveled"

9. Create tracks that mimic a real world coaster - N

10. Render environment in a better manner - Skybox position is centered on the entirity of the splines loaded, and skybox extents are calculated for good fit.

Additional Features: (Please document any additional features you may have implemented other than the ones described above)
1. Output various statistics about the spline to a .CSV, for a more analytic approach to testing different methods to determine coaster normals

Open-Ended Problems: (Please document approaches to any open-ended problems that you have tackled)
1. Determining Track normals:
	Coaster Normals are decided as a weighted average of 
		1. Previous coaster normal
		2. Coaster normal as calculated based on Sloans Method
		3. An "upward normal" which is Cross( Cross(tangent, global.Up), tangent)

Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
1. n/a

Names of the .cpp files you made changes to:
1. hw2.cpp
2. Spline.h (kept adding functionality to the Point class until it became equivalent to a Vector3 class)

Comments : N/A
