/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include "Vector3.h"
#include "Ray.h"
#include "Color.h"
#include <cfloat>
#include <vector>
#include <string>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char* gFilename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int gMode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480
#define ASPECT_RATIO (WIDTH/static_cast<double>(HEIGHT))

//the field of view of the camera
#define fov 60.0

#define DEBUG_INPUT false
bool gVerbose = true;
bool gDebug = false;

#define PHONG_SHADING 1
#define BLINN_PHONG_SHADING 2
int gShadingMode = PHONG_SHADING;


bool gAntiAliasing = true;

unsigned char gBuffer[HEIGHT][WIDTH][3];

struct Vertex
{
	Vector3 position;
	Color color_diffuse;
	Color color_specular;
	Vector3 normal;
	double shininess;

	Vertex()
		: position(0.0, 0.0, 0.0)
		, color_diffuse(0)
		, color_specular(0)
		, normal(0.0, 0.0, 0.0)
		, shininess(0.0)
	{}

	Vertex(Vector3 pos, Color dif, Color spec, Vector3 norm, double shine)
		: position(pos)
		, color_diffuse(dif)
		, color_specular(spec)
		, normal(norm)
		, shininess(shine)
	{}

	static bool isNothing(const Vertex& vert)
	{
		return abs(vert.normal.MagSquared()) < 0.0001;
	}

	static Vertex Nothing;
};
Vertex Vertex::Nothing = Vertex();

struct Triangle
{
	Vertex v[3];
};

struct Sphere
{
	Vector3 position;
	Color color_diffuse;
	Color color_specular;
	double shininess;
	double radius;
};

struct Light
{
	Vector3 position;
	Color color;
};

typedef Ray Plane;
typedef Vertex Intersection;


Triangle gTriangles[MAX_TRIANGLES];
Sphere gSpheres[MAX_SPHERES];
Light gLights[MAX_LIGHTS];
double gAmbientLight[3];

int gNumTriangles = 0;
int gNumSpheres = 0;
int gNumLights = 0;

Vector3 gCamPos;
double gViewportMinX, gViewportMinY, gViewportMaxX, gViewportMaxY;
double gViewportHeight, gViewportWidth;

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

void CalculateImageCorners()
{
	gViewportMaxX = ASPECT_RATIO * tan(fov / 2 * M_PI / 180);
	gViewportMinX = -gViewportMaxX;
	gViewportMaxY = tan(fov / 2 * M_PI / 180);
	gViewportMinY = -gViewportMaxY;

	//printf("x<%f, %f> y<%f, %f>", gViewportMaxX, gViewportMinX, gViewportMaxY, gViewportMinY);

	gViewportHeight = gViewportMaxY - gViewportMinY;
	gViewportWidth = gViewportMaxX - gViewportMinX;
}

inline Ray GetCameraRayTowardsPixel(const double x, const double y)
{
	double&& tempX = gViewportMinX + (gViewportWidth * ((x + 0.5) / static_cast<double>(WIDTH)));
	double&& tempY = gViewportMinY + (gViewportHeight * ((y + 0.5) / static_cast<double>(HEIGHT)));
	Vector3 pixelPos(tempX, tempY, -1.0);

	return Ray(gCamPos, Vector3::Normalize(pixelPos - gCamPos));
}

inline Vector3 GetTriangleNormal(const Triangle& triangle)
{
	return Vector3::Normalize(
		Vector3::Cross(
			triangle.v[2].position - triangle.v[1].position, // a-b
			triangle.v[0].position - triangle.v[1].position)); // c-b
}

inline Plane GetTrianglePlane(const Triangle& triangle)
{
	return Plane(triangle.v[0].position, GetTriangleNormal(triangle));
}

inline double GetRayPlaneIntersection(const Ray& ray, const Plane& plane)
{
	if (abs(Vector3::Dot(plane.direction, ray.direction)) < 0.00001) {
		return false; // No intersection, the line is parallel to the plane
	}
	// get d value
	double&& d = Vector3::Dot(plane.direction, plane.position);
	// Compute the X value for the directed line ray intersecting the plane
	return (d - Vector3::Dot(plane.direction, ray.position)) / Vector3::Dot(plane.direction, ray.direction);
}

inline double TriangleAreaXY(const Vector3& a, const Vector3& b, const Vector3& c) {
	return (0.5) * ((b.x - a.x)*(c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

inline double TriangleAreaYZ(const Vector3& a, const Vector3& b, const Vector3& c) {
	return (0.5) * ((b.y - a.y)*(c.z - a.z) - (c.y - a.y) * (b.z - a.z));
}

inline double TriangleAreaXZ(const Vector3& a, const Vector3& b, const Vector3& c) {
	return (0.5) * ((b.x - a.x)*(c.z - a.z) - (c.x - a.x) * (b.z - a.z));
}

Intersection GetRayTriangleIntersection(Ray& ray, const Triangle& triangle)
{
	Plane&& trianglePlane = GetTrianglePlane(triangle);

	// get ray/plane intersection point
	double t = GetRayPlaneIntersection(ray, trianglePlane);
	if (t <= 0.00001)
	{
		// no collision with trianglePlane
		return Intersection::Nothing;
	}

	// cast triangle and new found point onto a plane
	/*Triangle castTri(triangle);
	Vector3 castPoint(ray.position + ray.direction * t);*/

	double alpha, beta, gamma;
	const Vector3& c = ray.position + ray.direction * t;
	const Vector3& c0 = triangle.v[0].position;
	const Vector3& c1 = triangle.v[1].position;
	const Vector3& c2 = triangle.v[2].position;
	// make sure triangle is not perpendicular to plane
	if (!(abs(Vector3::Dot(trianglePlane.direction, Vector3::Back)) < 0.00001))
	{
		// xy plane work

		// check if point is in triangle using BARYCENTRIC TECHNIQUE
		// we don't need to cast to plane, because our plane specific area functions autocast.
		double&& totalArea = TriangleAreaXY(c0, c1, c2);
		alpha = TriangleAreaXY(c, c1, c2) / totalArea;
		beta = TriangleAreaXY(c0, c, c2) / totalArea;
		gamma = TriangleAreaXY(c0, c1, c) / totalArea;
	}
	else if (!(abs(Vector3::Dot(trianglePlane.direction, Vector3::Up)) < 0.00001))
	{
		// xz plane (flat plane)

		// check if point is in triangle using BARYCENTRIC TECHNIQUE
		// we don't need to cast to plane, because our plane specific area functions autocast.
		double&& totalArea = TriangleAreaXZ(c0, c1, c2);
		alpha = TriangleAreaXZ(c, c1, c2) / totalArea;
		beta = TriangleAreaXZ(c0, c, c2) / totalArea;
		gamma = TriangleAreaXZ(c0, c1, c) / totalArea;
	}
	else
	{
		// we can assume that yz plane works (cuz one plane always works)

		// check if point is in triangle using BARYCENTRIC TECHNIQUE
		// we don't need to cast to plane, because our plane specific area functions autocast.
		double&& totalArea = TriangleAreaYZ(c0, c1, c2);
		alpha = TriangleAreaYZ(c, c1, c2) / totalArea;
		beta = TriangleAreaYZ(c0, c, c2) / totalArea;
		gamma = TriangleAreaYZ(c0, c1, c) / totalArea;
	}

	if (alpha < 0.0000 || beta < 0.0000 || gamma < 0.0000
		|| alpha + beta + gamma > 1.00001)
	{
		// no collision
		return Intersection::Nothing;
	}


	Vector3&& finalPoint = alpha * triangle.v[0].position
		+ beta * triangle.v[1].position
		+ gamma * triangle.v[2].position;

	Color&& finalDiffuse = alpha * triangle.v[0].color_diffuse
		+ beta * triangle.v[1].color_diffuse
		+ gamma * triangle.v[2].color_diffuse;

	Color&& finalSpecular = alpha * triangle.v[0].color_specular
		+ beta * triangle.v[1].color_specular
		+ gamma * triangle.v[2].color_specular;

	Vector3&& finalNormal = alpha * triangle.v[0].normal
		+ beta * triangle.v[1].normal
		+ gamma * triangle.v[2].normal;

	double&& finalShininess = alpha * triangle.v[0].shininess
		+ beta * triangle.v[1].shininess
		+ gamma * triangle.v[2].shininess;

	return Intersection(finalPoint, finalDiffuse, finalSpecular, finalNormal, finalShininess);
}

Intersection GetRaySphereIntersection(const Ray& ray, const Sphere& sphere)
{
	const Vector3& s = ray.position;
	const Vector3& d = Vector3::Normalize(ray.direction);
	const Vector3& c = sphere.position;
	const double& r = sphere.radius;

	//// Calculate ray start's offset from the sphere center
	const Vector3&& p = s - c;

	const double&& rSquared = r*r;
	const double&& p_d = Vector3::Dot(p, d);

	//// The sphere is behind or surrounding the start point.
	if (p_d > 0 || Vector3::Dot(p, p) < rSquared)
	{
		//return NO_COLLISION;
		return Intersection::Nothing;
	}

	//// Flatten p into the plane passing through c perpendicular to the ray.
	//// This gives the closest approach of the ray to the center.
	const Vector3&& a = p - (d * p_d);

	const double&& aSquared = Vector3::Dot(a, a);

	//// Closest approach is outside the sphere.
	if (aSquared > rSquared)
	{
		// return NO_COLLISION;
		return Intersection::Nothing;
	}

	//// Calculate distance from plane where ray enters/exits the sphere.    
	const double&& h = sqrt(rSquared - aSquared);

	//// Calculate intersection point relative to sphere center.
	const Vector3 subIntersection = a - (d * h);

	//// We've taken a shortcut here to avoid a second square root.
	//// Note numerical errors can make the normal have length slightly different from 1.
	//// If you need higher precision, you may need to perform a conventional normalization.
	//normal.Normalize();

	return Intersection(c + subIntersection, sphere.color_diffuse, sphere.color_specular, Vector3::Normalize(subIntersection / r), sphere.shininess);
}

// returns -1 if no intersecting object
// ID's are in order of Tris,Spheres,Lights
// saves intersection ray in intersection
int GetClosestIntersectObjectID(Ray& ray, Intersection& intersectionOut = Intersection::Nothing)
{
	double closestDist = DBL_MAX;
	int retVal = -1;
	intersectionOut = Intersection::Nothing;

	// for each triangle, then each sphere
	for (int i = 0; i < gNumTriangles; ++i)
	{

		Intersection&& intersection = GetRayTriangleIntersection(ray, gTriangles[i]);
		if (Intersection::isNothing(intersection))
		{
			// no interesection with this ray/sphere combo
			continue;
		}

		Vector3&& diff = intersection.position - ray.position;
		double&& diffDist = diff.Mag();
		if (diffDist < closestDist)
		{
			// we found a new best!
			closestDist = diffDist;
			retVal = i;
			intersectionOut = std::move(intersection);
		}
	}

	for (int i = 0; i < gNumSpheres; ++i)
	{
		Intersection intersection = GetRaySphereIntersection(ray, gSpheres[i]);
		if (Intersection::isNothing(intersection))
		{
			// no interesection with this ray/sphere combo
			continue;
		}

		Vector3 diff = intersection.position - ray.position;
		double diffDist = diff.Mag();
		if (diffDist < closestDist)
		{
			// we found a new best!
			closestDist = diffDist;
			retVal = i + gNumTriangles;
			intersectionOut = std::move(intersection);
		}
	}

	return retVal;
}

void AddDiffuseSpecularLight(Color& colorOut, Intersection& intersection)
{
	// for each light
	for (int i = 0; i < gNumLights; ++i)
	{
		Light& light = gLights[i];

		Vector3& n = Vector3::Normalize(intersection.normal);
		Vector3&& v = Vector3::Normalize(gCamPos - intersection.position);

		Intersection shadowIntersection;
		Ray&& pointToLight = Ray(intersection.position, light.position - intersection.position);


		int shadowObjID = GetClosestIntersectObjectID(pointToLight, shadowIntersection);
		double distanceFromLightToFirstObject = (light.position - shadowIntersection.position).MagSquared();
		double distanceFromLightToTargetSurface = (light.position - intersection.position).MagSquared();
		//distanceFromLightToFirstObject < distanceFromLightToTargetSurface - smallValue
		if (shadowObjID != -1 // if there is something in the way of the ray
			&& distanceFromLightToFirstObject < distanceFromLightToTargetSurface - 0.0001
			) // if ray trace from intersection pos -> light is blocked (we are in shadow)
		{
			// we are in shadow; skip adding diffuse/specular for this light
			continue;
		}
		else // if not in shadow
		{

			Vector3&& l = Vector3::Normalize(light.position - intersection.position);

			double nl = Vector3::Dot(n, l);
			if (nl < 0.0) { nl = 0.0; }

			colorOut += (intersection.color_diffuse * nl) * light.color;


			if (gShadingMode == BLINN_PHONG_SHADING)
			{
				Vector3&& h = Vector3::Normalize(l + v);
				double nh = Vector3::Dot(n, h);
				if (nh < 0.0) { nh = 0.0; }
				colorOut += (intersection.color_specular * pow(nh, intersection.shininess * 4)) * light.color;
			}
			else // Default : PHONG_SHADING
			{
				Vector3& r = Vector3::Normalize((2.0 * nl * n) - l);
				double vr = Vector3::Dot(v, r);
				if (vr < 0.0) { vr = 0.0; }
				colorOut += (intersection.color_specular * pow(vr, intersection.shininess)) * light.color;
			}
		}
	}
}

void draw_scene()
{
	CalculateImageCorners();

	printf("Starting Drawing...\n"); fflush(stdout);

	// [x][y]
	std::vector<std::vector< Color > > antiAliasingPixels;
	antiAliasingPixels.resize(WIDTH+1);
	for (int i = 0; i < WIDTH; i++)
	{
		std::vector<Color> newVect;
		newVect.resize(HEIGHT+1);
		antiAliasingPixels.push_back(newVect);
	}

	int yLoops = HEIGHT + (gAntiAliasing ? 1 : 0);
	int xLoops = WIDTH + (gAntiAliasing ? 1 : 0);
	printf("Finished Preparing...\n"); fflush(stdout);

	for (int y = 0; y < yLoops; y++)
	{
		if(gVerbose) printf("Drawing row %d/%d...\n", y, yLoops-1); fflush(stdout);

		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (int x = 0; x < xLoops; x++)
		{
			gDebug = false;
			if (y == HEIGHT / 2 - 1) gDebug = true;
			// for each pixel
			// get the closest intersection
			Ray camRay;
			if (gAntiAliasing == false) { camRay = GetCameraRayTowardsPixel(x, y); }
			else { camRay = GetCameraRayTowardsPixel(x - 0.5, y - 0.5); }
			Intersection intersection;
			int objID = GetClosestIntersectObjectID(camRay, intersection);

			Color pixelColor(Color::White); // default bkgd fill color

			if (objID != -1)
			{
				// actually intersects, so we should calculate lighting
				pixelColor = Color::Black;
				// debug color stuff
				//pixelColor = Color(255 - static_cast<int>(Vector3::AngleDeg(Vector3::Forward, intersection.direction)));

				// ambient light
				Color ambientColor(gAmbientLight[0], gAmbientLight[1], gAmbientLight[2]);
				pixelColor += ambientColor;
				AddDiffuseSpecularLight(pixelColor, intersection);
			}
			double r = pixelColor.r * 255;
			double g = pixelColor.g * 255;
			double b = pixelColor.b * 255;
			if (r > 255) { r = 255; }
			else if (r < 0) { r = 0; }
			if (b > 255) { b = 255; }
			else if (b < 0) { b = 0; }
			if (g > 255) { g = 255; }
			else if (g < 0) { g = 0; }

			if (gAntiAliasing)
			{
				// only need to build the array of pixels, and we will later do the antialiasing.
				antiAliasingPixels[x].push_back(Color(r,g,b));
			}

				plot_pixel(x, y, r, g, b);
			
		}
		glEnd();
		glFlush();
	}
	printf("Starting antialiasing...\n"); fflush(stdout);

	// do the actual antialiasing
	if (gAntiAliasing)
	{
		for (unsigned int x = 0; x < WIDTH; x++)
		{
			glPointSize(2.0);
			glBegin(GL_POINTS);
			for (unsigned int y = 0; y < HEIGHT; y++)
			{
				// average [x][y], ... , [x+1][y+1]
				Color final = antiAliasingPixels[x][y]
					+ antiAliasingPixels[x + 1][y ]
					+ antiAliasingPixels[x][y + 1]
					+ antiAliasingPixels[x + 1][y + 1];
				final /= 4.0;
				plot_pixel(x, y, final.r, final.g, final.b);
			}
			glEnd();
			glFlush();
		}
	}
	printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
	glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	gBuffer[y][x][0] = r;
	gBuffer[y][x][1] = g;
	gBuffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	plot_pixel_display(x, y, r, g, b);
	if (gMode == MODE_JPEG)
		plot_pixel_jpeg(x, y, r, g, b);
}

void save_jpg()
{
	printf("Saving JPEG file: %s\n", gFilename);

	ImageIO img(WIDTH, HEIGHT, 3, &gBuffer[0][0][0]);
	if (img.save(gFilename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		printf("Error in Saving\n");
	else
		printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
	if (DEBUG_INPUT && strcasecmp(expected, found))
	{
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}
}

void parse_doubles(FILE* file, const char *check, Color& c)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	double R, G, B;
	fscanf(file, "%lf %lf %lf", &R, &G, &B);
	c = Color(R, G, B);
	if (DEBUG_INPUT) printf("%s %f %f %f\n", check, c.r, c.g, c.b);
}

void parse_doubles(FILE* file, const char *check, Vector3& p)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	if (DEBUG_INPUT) printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	if (DEBUG_INPUT) printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE *file, double *r)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%lf", r);
	if (DEBUG_INPUT) printf("rad: %f\n", *r);
}

void parse_shi(FILE *file, double *shi)
{
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%lf", shi);
	if (DEBUG_INPUT) printf("shi: %f\n", *shi);
}

int loadScene(char *argv)
{
	FILE * file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	if (DEBUG_INPUT) printf("number of objects: %i\n", number_of_objects);

	parse_doubles(file, "amb:", gAmbientLight);

	for (int i = 0; i < number_of_objects; i++)
	{
		fscanf(file, "%s\n", type);
		if (DEBUG_INPUT) printf("%s\n", type);
		if (strcasecmp(type, "triangle") == 0)
		{
			if (DEBUG_INPUT) printf("found triangle\n");
			for (int j = 0; j < 3; j++)
			{
				parse_doubles(file, "pos:", t.v[j].position);
				parse_doubles(file, "nor:", t.v[j].normal);
				parse_doubles(file, "dif:", t.v[j].color_diffuse);
				parse_doubles(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (gNumTriangles == MAX_TRIANGLES)
			{
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			gTriangles[gNumTriangles++] = t;
		}
		else if (strcasecmp(type, "sphere") == 0)
		{
			if (DEBUG_INPUT) printf("found sphere\n");

			parse_doubles(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_doubles(file, "dif:", s.color_diffuse);
			parse_doubles(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (gNumSpheres == MAX_SPHERES)
			{
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			gSpheres[gNumSpheres++] = s;
		}
		else if (strcasecmp(type, "light") == 0)
		{
			if (DEBUG_INPUT) printf("found light\n");
			parse_doubles(file, "pos:", l.position);
			parse_doubles(file, "col:", l.color);

			if (gNumLights == MAX_LIGHTS)
			{
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			gLights[gNumLights++] = l;
		}
		else
		{
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	return 0;
}

void display()
{
}

void init()
{
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

#include <chrono>
#include "Timer.h"
void idle()
{
	//hack to make it only draw once
	static int once = 0;

	if (!once)
	{
		Timer t;
		t.start();
		draw_scene();
		if (gMode == MODE_JPEG)
		{
			save_jpg();
		}
		printf("Time taken: <%f>\n", t.getElapsed());
	}
	once = 1;
}

int main(int argc, char ** argv)
{
	if ((argc < 2) || (argc > 3))
	{
		printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 3)
	{
		gMode = MODE_JPEG;
		gFilename = argv[2];
		printf(">Load file: %s\n>Output file: %s\n", argv[1], argv[2]);
	}
	else if (argc == 2)
	{
		gMode = MODE_DISPLAY;
		gFilename = "";
		printf("Load file: %s, Display only.", argv[1]);
	}
	glutInit(&argc, argv);
	loadScene(argv[1]);

	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	std::string title = "Ray Tracer: ";
	title += argv[1];
	if (argc == 3)
	{
		title += " -> ";
		title += argv[2];
	}
	std::cout << title << std::endl;
	int window = glutCreateWindow(title.c_str());
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	printf("Start glutMainLoop\n");
	glutMainLoop();
}

