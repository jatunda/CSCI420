/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: erichsie
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include "HeightmapPoint.h"

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
typedef enum { VERTS, LINES, TRIS, LINESTRIS} DISPLAY_STATE;
DISPLAY_STATE displayState = VERTS;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "DANK CSCI 420 homework I";



OpenGLMatrix myOGLMatrix;
BasicPipelineProgram pipelineProgram;
GLuint vaoVerts;
GLuint vaoLines;
GLuint vaoTris;
int rows, cols;
int numVerts, numLines, numTris;

bool autoRotate = false;


int vertsPerLine = 2;
int vertsPerTri = 3;
int floatsPerVert = 3;
int floatsPerColor = 4;

int screenshotCounter = 0;


// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void displayFunc()
{
	// render some stuff
	//glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.05f, 0.15f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	/// COMPUTING THE MODELVIEW MATRIX
	{
		myOGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
		myOGLMatrix.LoadIdentity();
		float zStudent = 3 + 5933593825 / 10000000000;
		myOGLMatrix.LookAt(1.0f, 2, zStudent, 0, 0, 0, 0, 1, 0);
		myOGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
		myOGLMatrix.Rotate(landRotate[0] / 2, 1, 0, 0);
		myOGLMatrix.Rotate(landRotate[1] / 2, 0, 1, 0);
		myOGLMatrix.Rotate(landRotate[2] / 2, 0, 0, 1);
		myOGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
	}

	/// COMPUTING THE PROJECTION MATRIX
	{
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


	//	Display the vao
	if (displayState == VERTS)
	{
		glBindVertexArray(vaoVerts); // bind the VAO
		GLint first = 0;
		GLsizei count = numVerts;
		glDrawArrays(GL_POINTS, first, count);
		glBindVertexArray(0); // unbind the VAO
	}

	else if (displayState == LINES)
	{
		glBindVertexArray(vaoLines);
		GLint first = 0;
		GLsizei count = numLines * 2;
		glDrawArrays(GL_LINES, first, count);
		glBindVertexArray(0);
	}

	else if (displayState == TRIS)
	{
		glBindVertexArray(vaoTris);
		GLint first = 0;
		GLsizei count = numTris * 3;
		glDrawArrays(GL_TRIANGLES, first, count);
		glBindVertexArray(0);
	}
	else if (displayState == LINESTRIS)
	{
		GLint first = 0;
		GLsizei count = 0;

		// display a mesh with a wireframe on it
		glBindVertexArray(vaoTris);
		first = 0;
		count = numTris * 3;
		glPolygonOffset(0.0f, 0.0f);
		glDrawArrays(GL_TRIANGLES, first, count);

		glBindVertexArray(vaoLines);
		first = 0;
		count = numLines * 2;
		glPolygonOffset(0.0f, 1.0f);
		glDrawArrays(GL_LINES, first, count);

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
		cout << "You pressed the spacebar." << endl;
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

void initScene(int argc, char *argv[])
{
	//printf("start InitScene\n");


	ImageIO * heightmapImage;
	HeightmapPoint** heightmapPoints;

	// for actual heightmap stuff
	{
		// load the image from a jpeg disk file to main memory
		heightmapImage = new ImageIO();
		if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
		{
			cout << "Error reading image " << argv[1] << "." << endl;
			exit(EXIT_FAILURE);
		}
		// for 2d array of height points
		rows = heightmapImage->getHeight();
		cols = heightmapImage->getWidth();
		heightmapPoints = new HeightmapPoint*[rows*cols];

		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {
				float height = heightmapImage->getPixel(c, r, 0);
				//if((i*cols+j)%(cols*rows/16)==0) 
					//printf("%d: r<%d>, c<%d>,h<%f>\n", i*cols+j, i, j, height);
				heightmapPoints[r*cols + c] = new HeightmapPoint(r, c, height);
			}
		}
	}

	//printf("Heightmap :: Rows:<%d>, Cols<%d>\n", rows, cols);
	//printf("first point loc?: <%d, %f, %d>\n", heightmapPoints[0]->row, heightmapPoints[0]->height, heightmapPoints[0]->col);
	numVerts = rows*cols;

	//printf("numVerts: <%d>\n", numVerts);
	float scaleX = static_cast<float>(2.0f / sqrt(numVerts));
	float scaleY = 0.001f;
	float scaleZ = scaleX;


	float* heightmapVerts = nullptr;
	float* heightmapVertsColors = nullptr;
	{
		heightmapVerts = new float[numVerts * 3];
		heightmapVertsColors = new float[numVerts * 4];
		for (int i = 0; i < numVerts; i++) {
			heightmapVerts[i * 3] = scaleX * (static_cast<float>(rows / 2 - heightmapPoints[i]->row)); //x
			heightmapVerts[i * 3 + 1] = scaleY * heightmapPoints[i]->height; //y
			heightmapVerts[i * 3 + 2] = scaleZ * static_cast<float>(cols / 2 - heightmapPoints[i]->col); //z

			// TODO: colors that aren't just white
			heightmapVertsColors[i * 4] = (static_cast<float>(heightmapPoints[i]->row) / static_cast<float>(rows));
			heightmapVertsColors[i * 4 + 1] = (static_cast<float>(heightmapPoints[i]->col) / static_cast<float>(cols));
			heightmapVertsColors[i * 4 + 2] = heightmapPoints[i]->height / 255.0f;
			heightmapVertsColors[i * 4 + 3] = 0.0f;
		}
	}


	///*
	float* heightmapLines = nullptr;
	float* heightmapLinesColors = nullptr;
	numLines = (cols - 1) * rows + (rows - 1) * cols;
	{
		heightmapLines = new float[numLines * vertsPerLine * floatsPerVert];
		heightmapLinesColors = new float[numLines * vertsPerLine * floatsPerColor];
		// build lines
		int counter = 0;
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {

				int vertsIndex = r*cols + c;
				float x1 = scaleX * (static_cast<float>(rows / 2 - heightmapPoints[vertsIndex]->row));
				float y1 = scaleY * heightmapPoints[vertsIndex]->height;
				float z1 = scaleZ * static_cast<float>(cols / 2 - heightmapPoints[vertsIndex]->col);

				float heightColor = heightmapPoints[vertsIndex]->height / 255.0f;

				// horizontal lines
				//  only build if not the last col
				if (c != cols - 1) {
					// position
					heightmapLines[counter * vertsPerLine * floatsPerVert + 0] = x1;
					heightmapLines[counter * 3 * 2 + 1] = y1;
					heightmapLines[counter * 3 * 2 + 2] = z1;
					// color
					heightmapLinesColors[counter * vertsPerLine * floatsPerColor + 0] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 1] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 2] = 1.0f;
					heightmapLinesColors[counter * 4 * 2 + 3] = 0.0f;

					// second point
					int vertsIndex = r*cols + c + 1;
					float x2 = scaleX * (static_cast<float>(rows / 2 - heightmapPoints[vertsIndex]->row));
					float y2 = scaleY * heightmapPoints[vertsIndex]->height;
					float z2 = scaleZ * static_cast<float>(cols / 2 - heightmapPoints[vertsIndex]->col);
					float heightColor = heightmapPoints[vertsIndex]->height / 255.0f;
					//position
					heightmapLines[counter * 3 * 2 + 3] = x2;
					heightmapLines[counter * 3 * 2 + 4] = y2;
					heightmapLines[counter * 3 * 2 + 5] = z2;
					// color
					heightmapLinesColors[counter * 4 * 2 + 4] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 5] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 6] = 1.0f;
					heightmapLinesColors[counter * 4 * 2 + 7] = 0.0f;

					// finished line
					++counter;


				}
				// vertical lines
				//  only build if not the last row
				if (r != rows - 1) {
					// position
					heightmapLines[counter * 3 * 2 + 0] = x1;
					heightmapLines[counter * 3 * 2 + 1] = y1;
					heightmapLines[counter * 3 * 2 + 2] = z1;
					// color
					heightmapLinesColors[counter * 4 * 2 + 0] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 1] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 2] = 1.0f;
					heightmapLinesColors[counter * 4 * 2 + 3] = 0.0f;

					// second point
					int vertsIndex = (r + 1) *cols + c;
					float x2 = scaleX * (static_cast<float>(rows / 2 - heightmapPoints[vertsIndex]->row));
					float y2 = scaleY * heightmapPoints[vertsIndex]->height;
					float z2 = scaleZ * static_cast<float>(cols / 2 - heightmapPoints[vertsIndex]->col);
					float heightColor = heightmapPoints[vertsIndex]->height / 255.0f;
					// position
					heightmapLines[counter * 3 * 2 + 3] = x2;
					heightmapLines[counter * 3 * 2 + 4] = y2;
					heightmapLines[counter * 3 * 2 + 5] = z2;
					// color
					heightmapLinesColors[counter * 4 * 2 + 4] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 5] = heightColor;
					heightmapLinesColors[counter * 4 * 2 + 6] = 1.0f;
					heightmapLinesColors[counter * 4 * 2 + 7] = 0.0f;
					// we made a line!
					++counter;
				}
			}
		}
		//printf("counter: <%d>, numLines: <%d>\n", counter, numLines);
	}
	//*/

	float * heightmapTris = nullptr;
	float * heightmapTrisColors = nullptr;

	numTris = (rows - 1) * (cols - 1) * 2;
	{
		heightmapTris = new float[numTris * vertsPerTri * floatsPerVert];
		heightmapTrisColors = new float[numTris * vertsPerTri * floatsPerColor];
		int counter = 0;
		int indices[4];
		float pos[4][3];
		float col[4][4];

		// for each square
		for (int r = 0; r < rows - 1; r++) {
			for (int c = 0; c < cols - 1; c++) {
				// build two triangles

				indices[0] = r*cols + c;
				indices[1] = r*cols + c + 1;
				indices[2] = (r + 1) * cols + c;
				indices[3] = (r + 1) * cols + c + 1;

				// get the data for the 4 points
				for (int i = 0; i < 4; i++) {
					pos[i][0] = scaleX * (static_cast<float>(rows / 2 - heightmapPoints[indices[i]]->row));
					pos[i][1] = scaleY * heightmapPoints[indices[i]]->height;
					pos[i][2] = scaleZ * (static_cast<float>(cols / 2 - heightmapPoints[indices[i]]->col));

					float heightColor = heightmapPoints[indices[i]]->height / 255.0f;
					col[i][0] = (heightColor >= 0.5f) ? 2 - heightColor*2 : 1.0f;
					col[i][1] =	(heightColor <= 0.5f) ? heightColor*2 : 1.0f;
						//0.0f;
					col[i][2] = 0.0f;
					col[i][3] = 1.0f;
				}

				// data => triangles in the array
				heightmapTris[counter * 6 * 3 + 0] = pos[0][0];
				heightmapTris[counter * 6 * 3 + 1] = pos[0][1];
				heightmapTris[counter * 6 * 3 + 2] = pos[0][2];

				heightmapTris[counter * 6 * 3 + 3] = pos[1][0];
				heightmapTris[counter * 6 * 3 + 4] = pos[1][1];
				heightmapTris[counter * 6 * 3 + 5] = pos[1][2];

				heightmapTris[counter * 6 * 3 + 6] = pos[2][0];
				heightmapTris[counter * 6 * 3 + 7] = pos[2][1];
				heightmapTris[counter * 6 * 3 + 8] = pos[2][2];

				heightmapTris[counter * 6 * 3 + 9] = pos[1][0];
				heightmapTris[counter * 6 * 3 + 10] = pos[1][1];
				heightmapTris[counter * 6 * 3 + 11] = pos[1][2];

				heightmapTris[counter * 6 * 3 + 12] = pos[2][0];
				heightmapTris[counter * 6 * 3 + 13] = pos[2][1];
				heightmapTris[counter * 6 * 3 + 14] = pos[2][2];

				heightmapTris[counter * 6 * 3 + 15] = pos[3][0];
				heightmapTris[counter * 6 * 3 + 16] = pos[3][1];
				heightmapTris[counter * 6 * 3 + 17] = pos[3][2];

				// hella colors
				heightmapTrisColors[counter * 6 * 4 + 0] = col[0][0];
				heightmapTrisColors[counter * 6 * 4 + 1] = col[0][1];
				heightmapTrisColors[counter * 6 * 4 + 2] = col[0][2];
				heightmapTrisColors[counter * 6 * 4 + 3] = col[0][3];

				heightmapTrisColors[counter * 6 * 4 + 4] = col[1][0];
				heightmapTrisColors[counter * 6 * 4 + 5] = col[1][1];
				heightmapTrisColors[counter * 6 * 4 + 6] = col[1][2];
				heightmapTrisColors[counter * 6 * 4 + 7] = col[1][3];

				heightmapTrisColors[counter * 6 * 4 + 8] = col[2][0];
				heightmapTrisColors[counter * 6 * 4 + 9] = col[2][1];
				heightmapTrisColors[counter * 6 * 4 + 10] = col[2][2];
				heightmapTrisColors[counter * 6 * 4 + 11] = col[2][3];

				heightmapTrisColors[counter * 6 * 4 + 12] = col[1][0];
				heightmapTrisColors[counter * 6 * 4 + 13] = col[1][1];
				heightmapTrisColors[counter * 6 * 4 + 14] = col[1][2];
				heightmapTrisColors[counter * 6 * 4 + 15] = col[1][3];

				heightmapTrisColors[counter * 6 * 4 + 16] = col[2][0];
				heightmapTrisColors[counter * 6 * 4 + 17] = col[2][1];
				heightmapTrisColors[counter * 6 * 4 + 18] = col[2][2];
				heightmapTrisColors[counter * 6 * 4 + 19] = col[2][3];

				heightmapTrisColors[counter * 6 * 4 + 20] = col[3][0];
				heightmapTrisColors[counter * 6 * 4 + 21] = col[3][1];
				heightmapTrisColors[counter * 6 * 4 + 22] = col[3][2];
				heightmapTrisColors[counter * 6 * 4 + 23] = col[3][3];


				// finished 2 triangles
				++counter;
			}
		}
	}

	//printf("first point loc: <%f, %f, %f>\n", heightmapVerts[0], heightmapVerts[1], heightmapVerts[2]);
	//printf("first point color: <%f, %f, %f, %f>\n", 
		//heightmapColors[0], heightmapColors[1], heightmapColors[2], heightmapColors[3]);
	//printf("we here fam\n");

	// Assignment 1 Milestone
	//float positions[3][3] = 
	//{ 
	//	{ 0, 0, -1},
	//	{ 1, 0, -1},
	//	{ 0, 1, -1} 
	//};

	//float colors[3][4] = 
	//{
	//	{ 1, 0, 0, 0},
	//	{ 0, 1, 0, 0},
	//	{ 0, 0, 1, 0}
	//};

	//float positions[6][3] = { { -1.0, -1.0, -1.0 },{ 1.0, -1.0, -1.0 },{ 1.0, 1.0, -1.0 },{ -1.0, -1.0, -1.0 },{ 1.0, 1.0, -1.0 },{ -1.0, 1.0, -1.0 } };
	//float colors[6][4] = { { 0.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 1.0, 1.0, 0.0, 1.0 },{ 1.0, 0.0, 1.0, 1.0 } };

	// initialize vboVerts
	GLuint vboVerts;
	glGenBuffers(1, &vboVerts);
	glBindBuffer(GL_ARRAY_BUFFER, vboVerts);
	// init buffer’s size, but don’t assign any data to it
	// glBufferData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(colors), NULL, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numVerts * (3 + 4), NULL, GL_STATIC_DRAW);
	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numVerts * 3, heightmapVerts);
	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*numVerts * 3, sizeof(float)*numVerts * 4, heightmapVertsColors);
	//printf("we here fam\n");

	// initialize vboLines
	GLuint vboLines;
	glGenBuffers(1, &vboLines);
	glBindBuffer(GL_ARRAY_BUFFER, vboLines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numLines * 2 * (3 + 4), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numLines * 2 * 3, heightmapLines);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*numLines * 2 * 3, sizeof(float)*numLines * 2 * 4, heightmapLinesColors);

	// initialize vboTris
	GLuint vboTris;
	glGenBuffers(1, &vboTris);
	glBindBuffer(GL_ARRAY_BUFFER, vboTris);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numTris * 3 * (3 + 4), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numTris * 3 * 3, heightmapTris);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*numTris * 3 * 3, sizeof(float)*numTris * 3 * 4, heightmapTrisColors);


	pipelineProgram.Init("../openGLHelper-starterCode");
	pipelineProgram.Bind();


	GLuint loc;
	const void * offset;
	GLsizei stride;
	GLboolean normalized;
	GLuint program = pipelineProgram.GetProgramHandle();

	// initialize vaoVerts
	glGenVertexArrays(1, &vaoVerts);
	// bind the VAO
	glBindVertexArray(vaoVerts);
	// bind the VBO (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, vboVerts);
	// get location index of the “position” shadervariable
	loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	offset = static_cast<const void*>(0);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	/// get the location index of the “color” shadervariable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	offset = (const void*)(sizeof(float)*numVerts * 3);
	stride = 0;
	normalized = GL_FALSE;
	/// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO

	// init vaoLines
	glGenVertexArrays(1, &vaoLines);
	glBindVertexArray(vaoLines);
	// lines
	glBindBuffer(GL_ARRAY_BUFFER, vboLines);
	loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	offset = static_cast<const void*>(0);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	// colors
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*)(sizeof(float)*numLines * 2 * 3);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind VAO

	// init vaoTris
	glGenVertexArrays(1, &vaoTris);
	glBindVertexArray(vaoTris);
	// tris
	glBindBuffer(GL_ARRAY_BUFFER, vboTris);
	loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	offset = static_cast<const void*>(0);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	// colors
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc);
	offset = (const void*)(sizeof(float)*numTris * 3 * 3);
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind that VAO


	//printf("end Init \n");
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

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
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	cout << "initializing scene" << endl;
	initScene(argc, argv);

	cout << "starting main loop" << endl;
	// sink forever into the glut loop
	glutMainLoop();
}


