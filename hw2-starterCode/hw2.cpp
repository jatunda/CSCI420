/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster

  Student username: erichsie
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstring>
#include <vector>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include "Spline.h"


#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#ifdef WIN32
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif



using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;
typedef enum { VERTS, LINES, TRIS, LINESTRIS } DISPLAY_STATE;
DISPLAY_STATE displayState = VERTS;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "DANK CSCI 420 homework 2";

OpenGLMatrix myOGLMatrix;
BasicPipelineProgram pipelineProgram;
GLuint vaoVerts;
GLuint vaoLines;
GLuint vaoTris;

bool autoRotate = false;

int screenshotCounter = 0;

// ASSIGNMENT 2 STUFF HERE
Spline * splines; // the spline array 
int numSplines; // total number of splines 

std::vector<GLuint> splineVaos;
std::vector<std::vector<float> > pos;
std::vector<std::vector<float> > color;

GLuint metalTexture, woodTexture, waterTexture, skyTexture;
GLuint vaoWater, vaoSky;
GLuint debugVAO;
float coasterPos = 1.0f;
Point oldTan(0.0f, 0.0f, 0.0f);
Point oldCamUp(0.0f, 1.0f, 0.0f);

float cam[3][3] = 
{
	{ 1.0f, 2.0f, 3.0f },
	{ 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f }
};

#define DEBUG false




inline const int clampIndex(const int index, const int length)
{
	if (index < 0)
	{
		return 0;
	}
	else if (index > length - 1)
	{
		return 1;
	}
	else if (index > length)
	{
		return 0;
	}
	else
	{
		return index;
	}
}

inline const float getCatmullRomTangent1D(const float t, const float x0, const float x1, const float x2, const float x3)
{
	// derivative of the value formula
	//float a = 2.0f * x1;
	float b = x2 - x0;
	float c = 2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3;
	float d = -x0 + 3.0f * x1 - 3.0f * x2 + x3;
	float pos = 0.5f * ((b)+(2 * c * t) + (3 * d * t * t));
	return pos;

	//return 
	//	0.5f *(
	//		(-x0 + x2) +
	//		(4 * x0 - 10 * x1 + 8 * x2 - x3) * t +
	//		(-3 * x0 + 9 * x1 - 9 * x2 + x3) * t * t
	//	);
}

inline const float getCatmullRomValue(const float t, const float x0, const float x1, const float x2, const float x3)
{
	// do math.
	float a = 2.0f * x1;
	float b = x2 - x0;
	float c = 2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3;
	float d = -x0 + 3.0f * x1 - 3.0f * x2 + x3;
	float pos = 0.5f * (a + (b * t) + (c * t * t) + (d * t * t * t));
	return pos;
}

inline const Point getCatmullRomTangent(const float t, const Point p0, const Point p1, const Point p2, const Point p3)
{
	float x = getCatmullRomTangent1D(t, static_cast<float>(p0.x), static_cast<float>(p1.x), static_cast<float>(p2.x), static_cast<float>(p3.x));
	float y = getCatmullRomTangent1D(t, static_cast<float>(p0.y), static_cast<float>(p1.y), static_cast<float>(p2.y), static_cast<float>(p3.y));
	float z = getCatmullRomTangent1D(t, static_cast<float>(p0.z), static_cast<float>(p1.z), static_cast<float>(p2.z), static_cast<float>(p3.z));
	return Point(x, y, z);
}

inline const Point getCatmullRomPoint(const float t, const Point p0, const Point p1, const Point p2, const Point p3)
{
	float x = getCatmullRomValue(t, static_cast<float>(p0.x), static_cast<float>(p1.x), static_cast<float>(p2.x), static_cast<float>(p3.x));
	float y = getCatmullRomValue(t, static_cast<float>(p0.y), static_cast<float>(p1.y), static_cast<float>(p2.y), static_cast<float>(p3.y));
	float z = getCatmullRomValue(t, static_cast<float>(p0.z), static_cast<float>(p1.z), static_cast<float>(p2.z), static_cast<float>(p3.z));
	return Point(x, y, z);
}

int loadSplines(char * argv)
{
	char * cName = (char *)malloc(128 * sizeof(char));
	FILE * fileList;
	FILE * fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file at <%s>\n", argv);
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file at <%s>\n", cName);
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point *)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

																		// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

													   // set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}
	// query for any errors
	GLenum errCode0 = glGetError();
	if (errCode0 != 0)
	{
		printf("Texture initialization error 0. Error code: %d.\n", errCode0);
		return -1;
	}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// query for any errors
	GLenum errCode1 = glGetError();
	if (errCode1 != 0)
	{
		printf("Texture initialization error 1. Error code: %d.\n", errCode1);
		return -1;
	}

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// query for any errors
	GLenum errCode2 = glGetError();
	if (errCode2 != 0)
	{
		printf("Texture initialization error 2. Error code: %d.\n", errCode2);
		return -1;
	}

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// query for any errors
	GLenum errCode3 = glGetError();
	if (errCode3 != 0)
	{
		printf("Texture initialization error 3. Error code: %d.\n", errCode3);
		return -1;
	}


	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		std::cout << "File " << filename << " saved successfully." << std::endl;
	else std::cout << "Failed to save file " << filename << '.' << std::endl;

	delete[] screenshotData;
}

void displayFunc()
{
	// render some stuff
	glClearColor(0.05f, 0.15f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	/// COMPUTING THE MODELVIEW MATRIX
	{
		myOGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
		myOGLMatrix.LoadIdentity();
		float zStudent = 3 + 5933593825 / 10000000000;
		myOGLMatrix.LookAt(cam[0][0], cam[0][1], cam[0][2], cam[1][0], cam[1][1], cam[1][2], cam[2][0], cam[2][1], cam[2][2]);
		myOGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
		myOGLMatrix.Rotate(landRotate[0] / 2, 1, 0, 0);
		myOGLMatrix.Rotate(landRotate[1] / 2, 0, 1, 0);
		myOGLMatrix.Rotate(landRotate[2] / 2, 0, 0, 1);
		myOGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
	}


	/// Bind pipeline program
	glUseProgram(pipelineProgram.GetProgramHandle());


	// Get the Modelview Matrix
	// upload modelview matrix to GPU
	{
		GLuint program = pipelineProgram.GetProgramHandle();
		GLint h_modelviewMatrix = glGetUniformLocation(program, "modelViewMatrix");
		float m[16];
		myOGLMatrix.GetMatrix(m);
		GLboolean isRowMajor = GL_FALSE;
		glUniformMatrix4fv(h_modelviewMatrix, 1, isRowMajor, m);
	}

	// get the projection matrix
	float p[16];
	myOGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	myOGLMatrix.GetMatrix(p);

	// upload proj matrix to GPU
	{
		GLuint program = pipelineProgram.GetProgramHandle(); // get handle to program
		GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix"); // get projMatrix handle
		GLboolean isRowMajor = GL_FALSE;
		glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
	}

	// Display Water
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, waterTexture);
		//glEnable(GL_TEXTURE_2D);
		glBindVertexArray(vaoWater);
		GLint first = 0;
		GLsizei count = 1 * 2 * 3;
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}

	// Display Skybox
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skyTexture);
		//glEnable(GL_TEXTURE_2D);
		glBindVertexArray(vaoSky);
		GLint first = 0;
		GLsizei count = 5*2*3;
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}

	// Display splines
	for (unsigned i = 0; i < splineVaos.size(); i++)
	{
		glBindVertexArray(splineVaos[i]);
		GLint first = 0;
		GLsizei count = pos[i].size() / 3;
		glDrawArrays(GL_LINE_STRIP, first, count);
		glBindVertexArray(0);
	}

	// always need this at end of display function
	glutSwapBuffers();
}

// for naming the frames
std::string intToStr(int n)
{
	char c[4];
	c[0] = '0' + (n / 100);
	c[1] = '0' + ((n / 10) % 10);
	c[2] = '0' + (n % 10);
	c[3] = '\0';
	return c;
}

void idleFunc()
{
	// do some stuff... 
	if (leftMouseButton || middleMouseButton || rightMouseButton)
	{
		autoRotate = false;
	}
	else if (autoRotate)
	{
		landRotate[1] += 0.5f;
		std::string filename = "screenshots/Frame" + intToStr(++screenshotCounter) + ".jpg";
		saveScreenshot(filename.c_str());
	}

	if (DEBUG == false)
	{
		// CALCULATE NORMAL, TANGENT, BINORMAL
		float integerTemp;
		float fraction = modf(coasterPos, &integerTemp);
		int integer = static_cast<int> (integerTemp);
		Point pos = getCatmullRomPoint(fraction, splines[0].points[integer - 1], splines[0].points[integer], 
			splines[0].points[integer + 1], splines[0].points[integer + 2]);
		
		Point newTan = getCatmullRomTangent(fraction, splines[0].points[integer - 1], splines[0].points[integer],
			splines[0].points[integer + 1], splines[0].points[integer + 2]);


		Point tanDif(newTan.x - oldTan.x, newTan.y - oldTan.y, newTan.z - oldTan.z);

		tanDif.Normalize();

		//Point camUp = Point(oldCamUp.x + tanDif.x, oldCamUp.y + tanDif.y, oldCamUp.z + tanDif.z);
		Point camUp = Point::Lerp(oldCamUp, camUp, 0.03f);
		camUp = newTan.Orthogonalize(camUp);
		camUp.Normalize();

		// SET CAMERA LOOK AT

		Point finalPos = pos + (camUp * 0.5f);
		cam[0][0] = static_cast<float>(finalPos.x);
		cam[0][1] = static_cast<float>(finalPos.y);
		cam[0][2] = static_cast<float>(finalPos.z);

		Point lookAt = finalPos + newTan;
		cam[1][0] = static_cast<float>(lookAt.x);
		cam[1][1] = static_cast<float>(lookAt.y);
		cam[1][2] = static_cast<float>(lookAt.z);

		cam[2][0] = camUp.x;
		cam[2][1] = camUp.y;
		cam[2][2] = camUp.z;


		// PREPARE FOR NEXT ITERATION
		oldTan = newTan;
		oldCamUp = camUp;
		if (coasterPos + 0.01f < splines[0].numControlPoints - 2)
		{
			coasterPos += 0.01f;
		}
	}

	// make the screen update 
	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	// comput projection matrix
	myOGLMatrix.SetMatrixMode(OpenGLMatrix::MatrixMode::Projection);
	myOGLMatrix.LoadIdentity();
	myOGLMatrix.Perspective(45, (1280.0f / 720.0f), 0.01f, 100.0f);

	// good habit to reset matrix mode
	myOGLMatrix.SetMatrixMode(OpenGLMatrix::MatrixMode::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.01f;
			landTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (rightMouseButton)
		{
			// control x,z scaling via right mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// if CTRL and SHIFT are not pressed, we are in rotate mode
	default:
		controlState = ROTATE;
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		std::cout << "You pressed the spacebar." << std::endl;
		break;

	case 'x':
		// take a screenshot
		saveScreenshot("screenshots/manual_screenshot.jpg");
		break;
	case '1':
		displayState = VERTS;
		break;
	case '2':
		displayState = LINES;
		break;
	case '3':
		displayState = TRIS;
		break;
	case '4':
		displayState = LINESTRIS;
		break;
	case 'r':
		autoRotate = !(autoRotate);
		break;
	}
}

std::vector<Point> splinePointsToCurve(const Spline& spline, float resolution)
{
	std::vector<Point> points;
	int numLoops = static_cast<int> (1.0f / resolution);
	// for each group of 4 on the spline
	for (int i = 0; i < spline.numControlPoints; i++)
	{
		if (i == 0 || i == spline.numControlPoints - 2 || i == spline.numControlPoints - 1)
		{
			continue;
		}

		Point p0 = spline.points[clampIndex(i - 1, spline.numControlPoints)];
		Point p1 = spline.points[i];
		Point p2 = spline.points[clampIndex(i + 1, spline.numControlPoints)];
		Point p3 = spline.points[clampIndex(i + 2, spline.numControlPoints)];


		for (int j = 0; j < numLoops; j++)
		{
			float t = j * resolution;
			points.push_back(getCatmullRomPoint(t, p0, p1, p2, p3));
		}
	}
	return points;
}

void initScene(int argc, char *argv[])
{
	//printf("start InitScene\n");
	loadSplines(argv[1]);
	for (int i = 0; i < numSplines; i++)
	{
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
	}
	float largestX, largestY, largestZ;
	float smallestX, smallestY, smallestZ;
	// for each spline
	for (int i = 0; i < numSplines; i++)
	{
		std::vector<Point> points = splinePointsToCurve(splines[i], 0.2f);
		std::vector<float> posInner;
		std::vector<float> colorInner;
		pos.push_back(posInner);
		color.push_back(colorInner);

		float r = (i + 1) / 1 % 2 == 0;
		float g = (i + 1) / 2 % 2 == 0;
		float b = (i + 1) / 4 % 2 == 0;
		// for each point in spline
		//for (int j = 0; j < splines[i].numControlPoints; j++)

		for (unsigned j = 0; j < points.size(); j++)
		{
			// add to pos
			//pos[i].push_back(static_cast<float>(splines[i].points[j].x));
			//pos[i].push_back(static_cast<float>(splines[i].points[j].y));
			//pos[i].push_back(static_cast<float>(splines[i].points[j].z));
			pos[i].push_back(static_cast<float>(points[j].x));
			pos[i].push_back(static_cast<float>(points[j].y));
			pos[i].push_back(static_cast<float>(points[j].z));

			if (points[j].x > largestX) largestX = static_cast<float>(points[j].x);
			else if (points[j].x < smallestX) smallestX = static_cast<float>(points[j].x);
			if (points[j].y > largestY) largestY = static_cast<float>(points[j].y);
			else if (points[j].y < smallestY) smallestY = static_cast<float>(points[j].y);
			if (points[j].z > largestZ) largestZ = static_cast<float>(points[j].z);
			else if (points[j].z < smallestZ) smallestZ = static_cast<float>(points[j].z);

			// add to color
			color[i].push_back(r);
			color[i].push_back(g);
			color[i].push_back(b);
			color[i].push_back(0.0f);
		}
		//printf("pos[%d].size() = %d; color[%d].size() = %d\n", i, pos[i].size(), i, color[i].size());
	}






	//printf("pos.size() = %d\n", pos.size());
	/*int x = 6;
	printf("%f, %f, %f\n", pos[2].data()[x + 0], pos[2].data()[x + 1], pos[2].data()[x + 2]);*/


	//pipelineProgram.Init("../openGLHelper-starterCode", false);
	pipelineProgram.Init("../openGLHelper-starterCode", true);

	pipelineProgram.Bind();
	GLuint program = pipelineProgram.GetProgramHandle();



	// iniialize VBOs and VAOs
	for (int i = 0; i < numSplines; i++)
	{
		// VBO
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (pos[i].size() + color[i].size()), NULL, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (pos[i].size()), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*pos[i].size(), pos[i].data());
		//glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * pos[i].size(), sizeof(float) * color[i].size(), color[i].data()

		// VAO
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		GLuint loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc); // enable the "position" attribute
		const void* offset = static_cast<const void*>(0);
		GLsizei stride = 0;
		GLboolean normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		//loc = glGetAttribLocation(program, "color");
		//glEnableVertexAttribArray(loc); // enable the "color" attribute
		//offset = (const void*)(sizeof(float) * pos[i].size());
		//stride = 0;
		//normalized = GL_FALSE;
		//glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);

		splineVaos.push_back(vao);
		glBindVertexArray(0); // unbind VAO
	}


	// LOAD TEXTURES
	{
		std::string waterFilename = "water.jpg";
		std::string metalFilename = "metal.jpg";
		std::string woodFilename = "wood.jpg";
		std::string skyFilename = "sky.jpg";

		glGenTextures(1, &waterTexture);
		glGenTextures(1, &metalTexture);
		glGenTextures(1, &woodTexture);
		glGenTextures(1, &skyTexture);

		int code = initTexture(waterFilename.c_str(), waterTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", waterFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", waterFilename.c_str(), code);
		}


		code = initTexture(metalFilename.c_str(), metalTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", waterFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", metalFilename.c_str(), code);
		}


		code = initTexture(woodFilename.c_str(), woodTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", woodFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", woodFilename.c_str(), code);
		}


		code = initTexture(skyFilename.c_str(), skyTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", waterFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", skyFilename.c_str(), code);
		}
	}


	// MAKE WATER AND SKYBOX

	{
		// amount of padding we get around the rollercoaster (x,y,z)
		const float padding[3] = { 3.0f, 1.0f, 3.0f };
		const float bigX = largestX + padding[0];
		const float smallX = smallestX - padding[0];
		const float topY = largestY + padding[1];
		const float floorY = smallestY < 0.0f ? smallestY - padding[1] : 0.0f;
		const float bigZ = largestZ + padding[2];
		const float smallZ = smallestZ - padding[2];
		printf("big: %f %f %f\n", bigX, topY, bigZ);
		printf("small: %f %f %f\n", smallX, floorY, smallZ);

		const float waterPos[2][3][3] =
		{
			// 2 triangles
			{
				// each triangle has 3 points
				// each point has 3 floats
				{bigX, floorY, bigZ},
				{bigX, floorY, smallZ},
				{smallX, floorY, bigZ}
			}
			,
			{
				{ smallX, floorY, smallZ},
				{ bigX, floorY, smallZ},
				{ smallX, floorY, bigZ}
			}
		};
		const float skyPos[5 * 2 * 3][3] =
		{
			// 5 sides, 2 tris, 3 points, with 3 floats each
			// top
			{bigX, topY, bigZ},
			{bigX, topY, smallZ},
			{smallX, topY, bigZ},

			{ smallX, topY, smallZ },
			{ bigX, topY, smallZ },
			{ smallX, topY, bigZ },

			//, // x forward
			{ bigX, topY, bigZ },
			{ bigX, topY, smallZ },
			{ bigX, floorY, bigZ },

			{ bigX, floorY, smallZ },
			{ bigX, topY, smallZ },
			{ bigX, floorY, bigZ },

			//, // x backward
			{ smallX, topY, bigZ },
			{ smallX, topY, smallZ },
			{ smallX, floorY, bigZ },

			{ smallX, floorY, smallZ },
			{ smallX, topY, smallZ },
			{ smallX, floorY, bigZ },

			//, // z forward
			{ bigX,topY,bigZ },
			{ bigX,floorY,bigZ },
			{ smallX,topY,bigZ },

			{ smallX,floorY,bigZ },
			{ bigX,floorY,bigZ },
			{ smallX,topY,bigZ },

			//, // z backward
			{ bigX,topY,smallZ },
			{ bigX,floorY,smallZ },
			{ smallX,topY,smallZ },

			{ smallX,floorY,smallZ },
			{ bigX,floorY,smallZ },
			{ smallX,topY,smallZ }
		};


		const float waterUVs[2][3][2] =
		{
			// 2 triangles
			{
				// each triangle has 3 points
				// each point has 2 floats
				{ 1.0f, 1.0f },
				{ 1.0f, -1.0f },
				{ -1.0f, 1.0f }
			}
			,
			{
				{ -1.0f, -1.0f },
				{ 1.0f, -1.0f },
				{ -1.0f, 1.0f }
			}
		};

		const float SkyUVs[5 * 2 * 3][2] =
		{
			// 5 sides, 2 tris, 3 points, with 2 floats each
			// top
			{ 1.0f, 1.0f },
			{ 1.0f, -1.0f },
			{ -1.0f, 1.0f },

			{ -1.0f, -1.0f },
			{ 1.0f, -1.0f },
			{ -1.0f, 1.0f },

			//, // x forward
			{ 1.0f, 1.0f },
			{ 1.0f, -1.0f},
			{ -1.0f, 1.0f },

			{ -1.0f, -1.0f },
			{ 1.0f, -1.0f },
			{ -1.0f, 1.0f },

			//, // x backward
			{ 1.0f, 1.0f},
			{ 1.0f, -1.0f },
			{ -1.0f, 1.0f },

			{ -1.0f, -1.0f},
			{ 1.0f, -1.0f },
			{ -1.0f, 1.0f },

			//, // z forward
			{ 1.0f,1.0f },
			{ 1.0f,-1.0f },
			{ -1.0f,1.0f },
			
			{ -1.0f,-1.0f },
			{ 1.0f,-1.0f },
			{ -1.0f,1.0f},

			//, // z backward
			{ 1.0f,1.0f },
			{ 1.0f,-1.0f },
			{ -1.0f,1.0f },

			{ -1.0f,-1.0f },
			{ 1.0f,-1.0f },
			{ -1.0f,1.0f}
		};

		// WATER VBO
		GLuint waterVBO;
		//std::cout << "sizeof pos: " << sizeof(pos) << "\nsizeof tex: " << sizeof(tex) << std::endl;
		glGenBuffers(1, &waterVBO);
		glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(waterPos) + sizeof(waterUVs)), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(waterPos), waterPos);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(waterPos), sizeof(waterUVs), waterUVs);

		// SKY VBO
		GLuint skyVBO;
		glGenBuffers(1, &skyVBO);
		glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(skyPos) + sizeof(SkyUVs)), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(skyPos), skyPos);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(skyPos), sizeof(SkyUVs), SkyUVs);
		
		// WATER VAO
		glGenVertexArrays(1, &vaoWater);
		glBindVertexArray(vaoWater);
		glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
		GLuint loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc); // enable the "position" attribute
		const void* offset = static_cast<const void*>(0);
		GLsizei stride = 0;
		GLboolean normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		loc = glGetAttribLocation(program, "texCoord");
		glEnableVertexAttribArray(loc); // enable the "textCoord" attribute
		offset = (const void*)(sizeof(waterPos));
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);
		glBindVertexArray(0); // unbind VAO

		// SKY VAO
		glGenVertexArrays(1, &vaoSky);
		glBindVertexArray(vaoSky);
		glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
		loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc); // enable the "position" attribute
		offset = static_cast<const void*>(0);
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		loc = glGetAttribLocation(program, "texCoord");
		glEnableVertexAttribArray(loc); // enable the "textCoord" attribute
		offset = (const void*)(sizeof(skyPos));
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);

		glBindVertexArray(0); // unbind VAO
	}




	

	//printf("end Init \n");
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Arguments incorrect.\n");
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	//cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	//cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	//cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	//cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	//cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK)
	{
		std::cout << "error: " << glewGetErrorString(result) << std::endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	//cout << "initializing scene" << endl;
	initScene(argc, argv);

	std::cout << "starting main loop" << std::endl;
	// sink forever into the glut loop
	glutMainLoop();
}


