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
#include <limits>
#include <fstream>
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

std::vector<GLuint> splinesVaos;
std::vector<int> splinesCount;
std::vector<GLuint> splinesCrossbarVaos;
std::vector<int> splinesCrossbarCount;
std::vector<std::vector<Point*> > splinesPoints;

GLuint metalTexture, woodTexture, grassTexture, skyTexture;
GLuint vaoGrass, vaoSky;
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

const float coasterSpeed = 0.7f;
const float resolution = 0.6f;

#define DEBUG false
#define SCREENSHOT_MODE false

inline int clampIndex(const int index, const int length)
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
		return length;
	}
	else
	{
		return index;
	}
}

inline float getCatmullRomTangent1D(const float t, const float x0, const float x1, const float x2, const float x3)
{
	// derivative of the value formula
	float b = x2 - x0;
	float c = 2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3;
	float d = -x0 + 3.0f * x1 - 3.0f * x2 + x3;
	float pos = 0.5f * ((b)+(2 * c * t) + (3 * d * t * t));
	return pos;
}

inline float getCatmullRomValue(const float t, const float x0, const float x1, const float x2, const float x3)
{
	// do math.
	float a = 2.0f * x1;
	float b = x2 - x0;
	float c = 2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3;
	float d = -x0 + 3.0f * x1 - 3.0f * x2 + x3;
	float pos = 0.5f * (a + (b * t) + (c * t * t) + (d * t * t * t));
	return pos;
}

inline Point getCatmullRomTangent(const float t, const Point& p0, const Point& p1, const Point& p2, const Point& p3)
{
	float x = getCatmullRomTangent1D(t, static_cast<float>(p0.x), static_cast<float>(p1.x), static_cast<float>(p2.x), static_cast<float>(p3.x));
	float y = getCatmullRomTangent1D(t, static_cast<float>(p0.y), static_cast<float>(p1.y), static_cast<float>(p2.y), static_cast<float>(p3.y));
	float z = getCatmullRomTangent1D(t, static_cast<float>(p0.z), static_cast<float>(p1.z), static_cast<float>(p2.z), static_cast<float>(p3.z));
	return Point(x, y, z);
}

inline Point getCatmullRomPoint(const float t, const Point& p0, const Point& p1, const Point& p2, const Point& p3)
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);


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

	//// Display Grass
	//{
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, grassTexture);
	//	//glEnable(GL_TEXTURE_2D);
	//	glBindVertexArray(vaoGrass);
	//	GLint first = 0;
	//	GLsizei count = 1 * 2 * 3;
	//	glDrawArrays(GL_TRIANGLES, first, count);
	//	glBindVertexArray(0);
	//}

	// Display Skybox
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skyTexture);
		//glEnable(GL_TEXTURE_2D);
		glBindVertexArray(vaoSky);
		GLint first = 0;
		GLsizei count = 6 * 2 * 3;
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}

	// Display splines - metal
	for (unsigned i = 0; i < splinesVaos.size(); i++)
	{
		glBindTexture(GL_TEXTURE_2D, metalTexture);
		glBindVertexArray(splinesVaos[i]);
		GLint first = 0;
		GLsizei count = splinesCount[i];
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}

	// Display splines - wood
	for (unsigned i = 0; i < splinesVaos.size(); i++)
	{
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glBindVertexArray(splinesCrossbarVaos[i]);
		GLint first = 0;
		GLsizei count = splinesCrossbarCount[i];
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}

	// always need this at end of display function
	glutSwapBuffers();
}

// for naming the frames
inline std::string intToStr(int n)
{
	char c[5];
	c[0] = '0' + (n / 1000 % 10);
	c[1] = '0' + (n / 100 % 10);
	c[2] = '0' + ((n / 10) % 10);
	c[3] = '0' + (n % 10);
	c[4] = '\0';
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

		Point pos = Point::Lerp(splinesPoints[0][integer - 1][0], splinesPoints[0][integer][0], fraction);
		Point tan = Point::Lerp(splinesPoints[0][integer - 1][1], splinesPoints[0][integer][1], fraction);
		Point norm = Point::Lerp(splinesPoints[0][integer - 1][2], splinesPoints[0][integer][2], fraction);

		// SET CAMERA LOOK AT
		Point finalPos = pos + (norm / 15.0f);
		cam[0][0] = static_cast<float>(finalPos.x);
		cam[0][1] = static_cast<float>(finalPos.y);
		cam[0][2] = static_cast<float>(finalPos.z);

		Point lookAt = finalPos + tan;
		cam[1][0] = static_cast<float>(lookAt.x);
		cam[1][1] = static_cast<float>(lookAt.y);
		cam[1][2] = static_cast<float>(lookAt.z);

		cam[2][0] = static_cast<float>(norm.x);
		cam[2][1] = static_cast<float>(norm.y);
		cam[2][2] = static_cast<float>(norm.z);

		if (coasterPos + coasterSpeed/resolution < splinesPoints[0].size() - 1)
		{
			coasterPos += coasterSpeed/resolution;
		}
	}

	if (SCREENSHOT_MODE)
	{
		std::string filename = "screenshots/Frame" + intToStr(++screenshotCounter) + ".jpg";
		saveScreenshot(filename.c_str());
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
	myOGLMatrix.Perspective(45, (1280.0f / 720.0f), 0.01f, 1000.0f);

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
		if (leftMouseButton && DEBUG)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.01f;
			landTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton && DEBUG)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton && DEBUG)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton && DEBUG)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton && DEBUG)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton && DEBUG)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (rightMouseButton && DEBUG)
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

std::vector<Point*> splinePointsToCurve(const Spline& spline, float resolution)
{
	std::vector<Point*> points;
	ofstream myfile;
	myfile.open("output.csv");
	myfile << "angleChange,distanceChange,xChange,yChange,zChange" << std::endl;
	

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


		float mag = 0.0f;
		int sampleRate = 4;
		for (int j = 0; j < sampleRate-1; j++)
		{
			Point diff = getCatmullRomPoint( j * (1.0f/static_cast<float>(sampleRate)), p0, p1, p2, p3)
				- getCatmullRomPoint((j +1) * (1.0f / static_cast<float>(sampleRate)), p0, p1, p2, p3);
			mag += diff.Mag();
		}

		int numLoops = static_cast<int> (mag / resolution) * 10;
		//float step = 

		for (int j = 0; j < numLoops; j++)
		{
			float t = j / static_cast<float>(numLoops);
			Point* p = new Point[4];
			p[0] = getCatmullRomPoint(t, p0, p1, p2, p3); // pos
			p[1] = getCatmullRomTangent(t, p0, p1, p2, p3);
			p[1].Normalize(); // normalize the tan
			p[2] = Point();
			p[3] = Point();


			points.push_back(p);
		}
	}



	// at this point, all of the positions/tangents are calculated, and the Norm/Binorms are zero vectors
	
	// calculate all normals, binormals
	// set first normal to up vector, calc it's binormal
	points[0][2] = Point::Cross(Point::Cross(points[0][1], Point::Up), points[0][1]);
	points[0][2].Normalize();
	points[0][3] = Point::Cross(points[0][1], points[0][2]);
	points[0][3].Normalize();
	// for each point
	for (unsigned i = 1; i < points.size(); ++i)
	{
		// NORMAL CALCULATION
		// norm = CROSS(oldBinorm, currTan)
		Point temp = Point::Cross(points[i-1][3], points[i][1]);
		Point upwardFacingNormal = Point::Cross(Point::Cross(points[i][1], Point::Up), points[i][1]);
		points[i][2] = points[i - 1][2] * 0.3f + temp * 0.4f + upwardFacingNormal* 0.3f;
		points[i][2].Normalize();
		// BINORMAL CALCULATION
		points[i][3] = Point::Cross(points[i][1], points[i][2]);
		points[i][3].Normalize();

		if (points.size() > 0)
		{
			float ang = Point::AngleDeg(points[i-1][2], points[i][2]);
			Point diff = (points[i - 1][0] - points[i][0]);
			myfile << ang 
				<< ',' << diff.Mag()
				<< ',' << diff.x
				<< ',' << diff.y
				<< ',' << diff.z
				<< std::endl;
		}
	}
	myfile.close();

	return points;
}

void MakeSplinePlane(Point& p0, Point& p1, Point& p2, Point& p3, std::vector<float>& trisVect, std::vector<float>& uvsVect, int& vertCount)
{
	// create 2 triangles, add to trisVect
	// also set UVs and add those to uvsVect
	trisVect.push_back(static_cast<float>(p0.x));
	trisVect.push_back(static_cast<float>(p0.y));
	trisVect.push_back(static_cast<float>(p0.z));
	uvsVect.push_back(1.0f);
	uvsVect.push_back(1.0f);
	++vertCount;
	trisVect.push_back(static_cast<float>(p1.x));
	trisVect.push_back(static_cast<float>(p1.y));
	trisVect.push_back(static_cast<float>(p1.z));
	uvsVect.push_back(1.0f);
	uvsVect.push_back(0.0f); 
	++vertCount;
	trisVect.push_back(static_cast<float>(p2.x));
	trisVect.push_back(static_cast<float>(p2.y));
	trisVect.push_back(static_cast<float>(p2.z));
	uvsVect.push_back(0.0f);
	uvsVect.push_back(0.0f);
	++vertCount;

	// triangle 2
	trisVect.push_back(static_cast<float>(p0.x));
	trisVect.push_back(static_cast<float>(p0.y));
	trisVect.push_back(static_cast<float>(p0.z));
	uvsVect.push_back(1.0f);
	uvsVect.push_back(1.0f);
	++vertCount;
	trisVect.push_back(static_cast<float>(p3.x));
	trisVect.push_back(static_cast<float>(p3.y));
	trisVect.push_back(static_cast<float>(p3.z));
	uvsVect.push_back(0.0f);
	uvsVect.push_back(1.0f);
	++vertCount;
	trisVect.push_back(static_cast<float>(p2.x));
	trisVect.push_back(static_cast<float>(p2.y));
	trisVect.push_back(static_cast<float>(p2.z));
	uvsVect.push_back(0.0f);
	uvsVect.push_back(0.0f);
	++vertCount;
}

void initScene(int argc, char *argv[])
{
	loadSplines(argv[1]);
	for (int s = 0; s < numSplines; s++)
	{
		printf("Num control points in spline %d: %d.\n", s, splines[s].numControlPoints);
	}

	float largestX = std::numeric_limits<float>::min();
	float largestY = std::numeric_limits<float>::min();
	float largestZ = std::numeric_limits<float>::min();
	float smallestX = std::numeric_limits<float>::max();
	float smallestY = std::numeric_limits<float>::max();
	float smallestZ = std::numeric_limits<float>::max();
	std::vector<std::vector<float> > splinesTris;
	std::vector<std::vector<float> > splinesUVs;
	std::vector<std::vector<float> > splinesCrossbarTris;
	std::vector<std::vector<float> > splinesCrossbarUVs;
	// for each spline
	for (int s = 0; s < numSplines; s++)
	{
		splinesPoints.push_back(splinePointsToCurve(splines[s], resolution));
		splinesTris.push_back(std::vector<float>());
		splinesUVs.push_back(std::vector<float>());
		splinesCount.push_back(0);
		splinesCrossbarTris.push_back(std::vector<float>());
		splinesCrossbarUVs.push_back(std::vector<float>());
		splinesCrossbarCount.push_back(0);

		// for each pair of points in spline
		for (unsigned p = 1; p < splinesPoints[s].size(); p++)
		{
			// (use p-1 and p)
			// generate planes
			Point pos0 = splinesPoints[s][p - 1][0];
			Point pos1 = splinesPoints[s][p - 0][0];
			Point tan0 = splinesPoints[s][p - 1][1];
			Point tan1 = splinesPoints[s][p - 0][1];
			Point norm0 = splinesPoints[s][p - 1][2];
			Point norm1 = splinesPoints[s][p - 0][2];
			Point binorm0 = splinesPoints[s][p - 1][3];
			Point binorm1 = splinesPoints[s][p - 0][3];
			float n = 20.0f;
			float m = 100.0f;
			{
				// left box thing
				std::vector<Point> planePoints;
				planePoints.push_back(pos0 + binorm0 / m + norm0 / m - binorm0 / n); // p0 upright left
				planePoints.push_back(pos0 - binorm0 / m + norm0 / m - binorm0 / n); // p0 upleft left
				planePoints.push_back(pos0 - binorm0 / m - norm0 / m - binorm0 / n); // p0 downleft left
				planePoints.push_back(pos0 + binorm0 / m - norm0 / m - binorm0 / n); // p0 downright left

				planePoints.push_back(pos1 + binorm1 / m + norm1 / m - binorm1 / n); // p1 upright left
				planePoints.push_back(pos1 - binorm1 / m + norm1 / m - binorm1 / n); // p1 upleft left
				planePoints.push_back(pos1 - binorm1 / m - norm1 / m - binorm1 / n); // p1 downleft left
				planePoints.push_back(pos1 + binorm1 / m - norm1 / m - binorm1 / n); // p1 downright left

				// actually generate planes and put them into splinesTris and splinesUVs
				MakeSplinePlane(planePoints[0], planePoints[1], planePoints[5], planePoints[4], splinesTris[s], splinesUVs[s], splinesCount[s]); // top plane
				MakeSplinePlane(planePoints[1], planePoints[2], planePoints[6], planePoints[5], splinesTris[s], splinesUVs[s], splinesCount[s]); // left plane
				MakeSplinePlane(planePoints[2], planePoints[3], planePoints[7], planePoints[6], splinesTris[s], splinesUVs[s], splinesCount[s]); // bot plane
				MakeSplinePlane(planePoints[3], planePoints[0], planePoints[4], planePoints[7], splinesTris[s], splinesUVs[s], splinesCount[s]); // right plane
			}
			{
				// right box thing
				std::vector<Point> planePoints;
				planePoints.push_back(pos0 + binorm0 / m + norm0 / m + binorm0 / n); // p0 upright right
				planePoints.push_back(pos0 - binorm0 / m + norm0 / m + binorm0 / n); // p0 upleft right
				planePoints.push_back(pos0 - binorm0 / m - norm0 / m + binorm0 / n); // p0 downleft right
				planePoints.push_back(pos0 + binorm0 / m - norm0 / m + binorm0 / n); // p0 downright right

				planePoints.push_back(pos1 + binorm1 / m + norm1 / m + binorm1 / n); // p1 upright right
				planePoints.push_back(pos1 - binorm1 / m + norm1 / m + binorm1 / n); // p1 upleft right
				planePoints.push_back(pos1 - binorm1 / m - norm1 / m + binorm1 / n); // p1 downleft right
				planePoints.push_back(pos1 + binorm1 / m - norm1 / m + binorm1 / n); // p1 downright right

				// actually generate planes and put them into splinesTris and splinesUVs
				MakeSplinePlane(planePoints[0], planePoints[1], planePoints[5], planePoints[4], splinesTris[s], splinesUVs[s], splinesCount[s]); // top plane
				MakeSplinePlane(planePoints[1], planePoints[2], planePoints[6], planePoints[5], splinesTris[s], splinesUVs[s], splinesCount[s]); // left plane
				MakeSplinePlane(planePoints[2], planePoints[3], planePoints[7], planePoints[6], splinesTris[s], splinesUVs[s], splinesCount[s]); // bot plane
				MakeSplinePlane(planePoints[3], planePoints[0], planePoints[4], planePoints[7], splinesTris[s], splinesUVs[s], splinesCount[s]); // right plane
			}
			{
				// left box thing
				std::vector<Point> planePoints;
				planePoints.push_back(pos0 + binorm0 / m + norm0 / m - binorm0 / n); // p0 upright left
				planePoints.push_back(pos0 - binorm0 / m + norm0 / m - binorm0 / n); // p0 upleft left
				planePoints.push_back(pos0 - binorm0 / m - norm0 / m - binorm0 / n); // p0 downleft left
				planePoints.push_back(pos0 + binorm0 / m - norm0 / m - binorm0 / n); // p0 downright left

				planePoints.push_back(pos1 + binorm1 / m + norm1 / m - binorm1 / n); // p1 upright left
				planePoints.push_back(pos1 - binorm1 / m + norm1 / m - binorm1 / n); // p1 upleft left
				planePoints.push_back(pos1 - binorm1 / m - norm1 / m - binorm1 / n); // p1 downleft left
				planePoints.push_back(pos1 + binorm1 / m - norm1 / m - binorm1 / n); // p1 downright left

				// actually generate planes and put them into splinesTris and splinesUVs
				MakeSplinePlane(planePoints[0], planePoints[1], planePoints[5], planePoints[4], splinesTris[s], splinesUVs[s], splinesCount[s]); // top plane
				MakeSplinePlane(planePoints[1], planePoints[2], planePoints[6], planePoints[5], splinesTris[s], splinesUVs[s], splinesCount[s]); // left plane
				MakeSplinePlane(planePoints[2], planePoints[3], planePoints[7], planePoints[6], splinesTris[s], splinesUVs[s], splinesCount[s]); // bot plane
				MakeSplinePlane(planePoints[3], planePoints[0], planePoints[4], planePoints[7], splinesTris[s], splinesUVs[s], splinesCount[s]); // right plane
			}
			int modMe = static_cast<int>(2.0 / resolution);
			if(p % modMe == 0)
			{
				// crossbar
				std::vector<Point> planePoints;
				planePoints.push_back(pos0 - tan0/m - binorm0 / m + norm0 / m/2 + binorm0 / n); // p0 upleft right
				planePoints.push_back(pos0 - tan0 / m - binorm0 / m - norm0 / m/2 + binorm0 / n); // p0 downleft right
				planePoints.push_back(pos0 + tan0 / m - binorm0 / m - norm0 / m/2 + binorm0 / n); // p1 downleft right
				planePoints.push_back(pos0 + tan0 / m - binorm0 / m + norm0 / m/2 + binorm0 / n); // p1 upleft right

				planePoints.push_back(pos0 - tan0/m + binorm0 / m + norm0 / m/2 - binorm0 / n); // p0 upright left
				planePoints.push_back(pos0 - tan0/m + binorm0 / m - norm0 / m/2 - binorm0 / n); // p0 downright left
				planePoints.push_back(pos0 + tan0/m + binorm0 / m - norm0 / m/2 - binorm0 / n); // p1 downright left
				planePoints.push_back(pos0 + tan0/m + binorm0 / m + norm0 / m/2 - binorm0 / n); // p1 upright left

				// actually generate planes and put them into splinesTris and splinesUVs
				MakeSplinePlane(planePoints[0], planePoints[1], planePoints[5], planePoints[4], splinesCrossbarTris[s], splinesCrossbarUVs[s], splinesCrossbarCount[s]); // top plane
				MakeSplinePlane(planePoints[1], planePoints[2], planePoints[6], planePoints[5], splinesCrossbarTris[s], splinesCrossbarUVs[s], splinesCrossbarCount[s]); // left plane
				MakeSplinePlane(planePoints[2], planePoints[3], planePoints[7], planePoints[6], splinesCrossbarTris[s], splinesCrossbarUVs[s], splinesCrossbarCount[s]); // bot plane
				MakeSplinePlane(planePoints[3], planePoints[0], planePoints[4], planePoints[7], splinesCrossbarTris[s], splinesCrossbarUVs[s], splinesCrossbarCount[s]); // right plane
			}
			// calculating how big our skybox should be
			if (pos1.x > largestX) largestX = static_cast<float>(pos1.x);
			else if (pos1.x < smallestX) smallestX = static_cast<float>(pos1.x);
			if (pos1.y > largestY) largestY = static_cast<float>(pos1.y);
			else if (pos1.y < smallestY) smallestY = static_cast<float>(pos1.y);
			if (pos1.z > largestZ) largestZ = static_cast<float>(pos1.z);
			else if (pos1.z < smallestZ) smallestZ = static_cast<float>(pos1.z);
		} // for each point pair in spline s
		
	} // for each spline

	pipelineProgram.Init("../openGLHelper-starterCode", true);

	pipelineProgram.Bind();
	GLuint program = pipelineProgram.GetProgramHandle();

	// iniialize splines VBOs and VAOs
	for (int s = 0; s < numSplines; s++)
	{
		// VBO - metal
		GLuint vboMetal;
		glGenBuffers(1, &vboMetal);
		glBindBuffer(GL_ARRAY_BUFFER, vboMetal);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (splinesTris[s].size() + splinesUVs[s].size()), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*splinesTris[s].size(), splinesTris[s].data());
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*splinesTris[s].size(), sizeof(float)*splinesUVs[s].size(), splinesUVs[s].data());

		// VBO - wood
		GLuint vboWood;
		glGenBuffers(1, &vboWood);
		glBindBuffer(GL_ARRAY_BUFFER, vboWood);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (splinesCrossbarTris[s].size() + splinesCrossbarUVs[s].size()), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*splinesCrossbarTris[s].size(), splinesCrossbarTris[s].data());
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*splinesCrossbarTris[s].size(), sizeof(float)*splinesCrossbarUVs[s].size(), splinesCrossbarUVs[s].data());

		// VAO - metal
		GLuint vaoMetal;
		glGenVertexArrays(1, &vaoMetal);
		glBindVertexArray(vaoMetal);
		glBindBuffer(GL_ARRAY_BUFFER, vboMetal);
		GLuint loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc); // enable the "position" attribute
		const void* offset = static_cast<const void*>(0);
		GLsizei stride = 0;
		GLboolean normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		loc = glGetAttribLocation(program, "texCoord");
		glEnableVertexAttribArray(loc); // enable the "texCoord" attribute
		offset = (const void*)(sizeof(float)*splinesTris[s].size());
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);
		splinesVaos.push_back(vaoMetal);
		glBindVertexArray(0); // unbind VAO

		// VAO - wood
		GLuint vaoWood;
		glGenVertexArrays(1, &vaoWood);
		glBindVertexArray(vaoWood);
		glBindBuffer(GL_ARRAY_BUFFER, vboWood);
		loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc);
		offset = static_cast<const void*> (0);
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		loc = glGetAttribLocation(program, "texCoord");
		glEnableVertexAttribArray(loc);
		offset = (const void*)(sizeof(float)*splinesCrossbarTris[s].size());
		stride = 0;
		normalized = GL_FALSE;
		glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);
		splinesCrossbarVaos.push_back(vaoWood);
		glBindVertexArray(0); // unbind VAO
	}


	// LOAD TEXTURES
	{
		std::string grassFilename = "grass.jpg";
		std::string metalFilename = "metal.jpg";
		std::string woodFilename = "wood.jpg";
		std::string skyFilename = "skybox.jpg";
		int switcher = rand() % 4;
		switcher = 1;
		switch (switcher)
		{
		case 0:
			skyFilename = "skybox.jpg";
			break;
		case 1:
			skyFilename = "skybox1.jpg";
			break;
		case 2:
			skyFilename = "skybox3.jpg";
			break;
		case 3:
			skyFilename = "skyboxSpace1.jpg";
			break;
		}

		glGenTextures(1, &grassTexture);
		glGenTextures(1, &metalTexture);
		glGenTextures(1, &woodTexture);
		glGenTextures(1, &skyTexture);

		int code = initTexture(grassFilename.c_str(), grassTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", grassFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", grassFilename.c_str(), code);
		}

		code = initTexture(metalFilename.c_str(), metalTexture);
		if (code != 0)
		{
			printf(">Error loading the texture image @ %s.\n", metalFilename.c_str());
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
			printf(">Error loading the texture image @ %s.\n", skyFilename.c_str());
			exit(EXIT_FAILURE);
		}
		else
		{
			printf(">loaded %s, with code %d.\n", skyFilename.c_str(), code);
		}
	}


	// MAKE GRASS AND SKYBOX
	{
		// amount of padding we get around the rollercoaster (x,y,z)
		const float padding[3] = { 3.0f, 1.0f, 3.0f };
		float extent = abs(largestX - smallestX) + padding[0]*2;
		if(abs(largestY-smallestY) + padding[1]*2 > extent)
			extent = abs(largestY - smallestY) + padding[1]*2;
		if (abs(largestZ - smallestZ) + padding[2]*2 > extent)
			extent = abs(largestZ - smallestZ) + padding[2]*2;
		extent *= 2;
		Point center((largestX+smallestX)/2.0, (largestY+smallestY)/2.0, (largestZ+smallestZ)/2.0);
		const float bigX = static_cast<float>(center.x + extent);
		const float smallX = static_cast<float>(center.x - extent);
		const float topY = static_cast<float>(center.y + extent);
		const float floorY = static_cast<float>(center.y - extent);
		const float bigZ = static_cast<float>(center.z + extent);
		const float smallZ = static_cast<float>(center.z - extent);

		const float grassPos[2*3][3] =
		{
			// 2 triangles
			// each triangle has 3 points
			// each point has 3 floats
			{bigX, floorY, bigZ},
			{bigX, floorY, smallZ},
			{smallX, floorY, bigZ},
			
			{ smallX, floorY, smallZ},
			{ bigX, floorY, smallZ},
			{ smallX, floorY, bigZ}
			
		};
		const float skyPos[6 * 2 * 3][3] =
		{
			// 6 sides, 2 tris, 3 points, with 3 floats each
			// bot
			{ bigX, floorY, bigZ },
			{ bigX, floorY, smallZ },
			{ smallX, floorY, bigZ },

			{ smallX, floorY, smallZ },
			{ bigX, floorY, smallZ },
			{ smallX, floorY, bigZ },
			// top
			{bigX, topY, bigZ},
			{bigX, topY, smallZ},
			{smallX, topY, bigZ},

			{ smallX, topY, smallZ },
			{ bigX, topY, smallZ },
			{ smallX, topY, bigZ },

			//, // x forward
			{ bigX, topY, smallZ },
			{ bigX, floorY, smallZ },
			{ bigX, topY, bigZ },

			{ bigX, floorY, bigZ },
			{ bigX, floorY, smallZ },
			{ bigX, topY, bigZ },

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


		const float grassUVs[2*3][2] =
		{
			// 2 triangles
			// each triangle has 3 points
			// each point has 2 floats
			{ 5.0f, 5.0f },
			{ 5.0f, -5.0f },
			{ -5.0f, 5.0f },
		
			{ -5.0f, -5.0f },
			{ 5.0f, -5.0f },
			{ -5.0f, 5.0f }
			
		};

		const float SkyUVs[6 * 2 * 3][2] =
		{
			// 6 sides, 2 tris, 3 points, with 2 floats each
			// bot
			{ 0.5f, 1/3.0f },
			{ 0.5f, 0.0f },
			{ 0.25f, 1 / 3.0f },

			{ 0.25f, 0.0f },
			{ 0.5f, 0.0f },
			{ 0.25f, 1 / 3.0f },

			// top
			{ 0.5f, 2 / 3.0f }, // 4
			{ 0.5f, 1.0f }, // 1
			{ 0.25f, 2 / 3.0f }, // 3

			{ 0.25f, 1.0f }, // 2
			{ 0.5f, 1.0f }, // 1
			{ 0.25f, 2 / 3.0f }, // 3

			//, // x forward
			{ 0.75f, 2 / 3.0f },
			{ 0.75f, 1 / 3.0f },
			{ 0.5f, 2 / 3.0f },

			{ 0.5f, 1 / 3.0f },
			{ 0.75f, 1 / 3.0f },
			{ 0.5f, 2 / 3.0f }, // yes

			//, // x backward
			{ 0.25f, 2 / 3.0f }, // 1
			{ 0.0f, 2 / 3.0f }, // 2
			{ 0.25f, 1 / 3.0f }, // 4

			{ 0.0f, 1 / 3.0f }, // 3
			{ 0.0f, 2 / 3.0f }, // 2
			{ 0.25f, 1 / 3.0f }, // 4

			//, // z forward
			{ 0.5f, 2 / 3.0f }, // 1
			{ 0.5f, 1 / 3.0f }, // 4
			{ 0.25f, 2 / 3.0f }, // 2
			
			{ 0.25f, 1 / 3.0f }, // 3
			{ 0.5f, 1 / 3.0f }, // 4
			{ 0.25f, 2 / 3.0f }, // 2

			//, // z backward
			{ 0.75f, 2 / 3.0f }, // 2
			{ 0.75f, 1 / 3.0f }, // 3
			{ 1.0f, 2 / 3.0f }, // 1

			{ 1.0f, 1 / 3.0f }, // 4
			{ 0.75f, 1 / 3.0f }, // 3
			{ 1.0f, 2 / 3.0f } // 1
		};

		// GRASS VBO
		GLuint grassVBO;
		glGenBuffers(1, &grassVBO);
		glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(grassPos) + sizeof(grassUVs)), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(grassPos), grassPos);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(grassPos), sizeof(grassUVs), grassUVs);

		// SKY VBO
		GLuint skyVBO;
		glGenBuffers(1, &skyVBO);
		glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
		glBufferData(GL_ARRAY_BUFFER, (sizeof(skyPos) + sizeof(SkyUVs)), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(skyPos), skyPos);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(skyPos), sizeof(SkyUVs), SkyUVs);
		
		// GRASS VAO
		glGenVertexArrays(1, &vaoGrass);
		glBindVertexArray(vaoGrass);
		glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
		GLuint loc = glGetAttribLocation(program, "position");
		glEnableVertexAttribArray(loc); // enable the "position" attribute
		const void* offset = static_cast<const void*>(0);
		GLsizei stride = 0;
		GLboolean normalized = GL_FALSE;
		glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
		loc = glGetAttribLocation(program, "texCoord");
		glEnableVertexAttribArray(loc); // enable the "textCoord" attribute
		offset = (const void*)(sizeof(grassPos));
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
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Arguments incorrect.\n");
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	glutInit(&argc, argv);

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

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
	initScene(argc, argv);

	std::cout << "starting main loop" << std::endl;
	// sink forever into the glut loop
	glutMainLoop();
}


