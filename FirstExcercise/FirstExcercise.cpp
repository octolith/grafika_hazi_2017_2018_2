//=============================================================================================
// Szamitogepes grafika hazi feladat keret. Ervenyes 2018-tol.
// A //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// sorokon beluli reszben celszeru garazdalkodni, mert a tobbit ugyis toroljuk.
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : 
// Neptun : 
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#define _USE_MATH_DEFINES		// Van M_PI
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		// must be downloaded 
#include <GL/freeglut.h>	// must be downloaded unless you have an Apple
#endif

const unsigned int windowWidth = 600, windowHeight = 600;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// You are supposed to modify the code from here...

// OpenGL major and minor versions
int majorVersion = 3, minorVersion = 3;

void getErrorInfo(unsigned int handle) {
	int logLen;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		int written;
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

// check if shader could be compiled
void checkShader(unsigned int shader, const char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) { printf("%s!\n", message); getErrorInfo(shader); }
}

// check if shader could be linked
void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) { printf("Failed to link shader program!\n"); getErrorInfo(program); }
}

// vertex shader in GLSL
const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1
	out vec3 color;									// output attribute

	void main() {
		color = vertexColor;														// copy color from input to output
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * fragmentSource = R"(
	#version 330
    precision highp float;

	in vec3 color;				// variable input: interpolated color of vertex shader
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";

// 2D point in Cartesian coordinates
struct vec2 {
	float v[2];

	vec2(float x = 0, float y = 0) { v[0] = x; v[1] = y; }
	vec2 operator-(vec2& right) {
		vec2 res(v[0] - right.v[0], v[1] - right.v[1]);
		return res;
	}
	float operator[] (int idx) const{
		return v[idx];
	}
	float Length() { return sqrtf(v[0] * v[0] + v[1] * v[1]); }
};

// 3D point in Cartesian coordinates or RGB color
struct vec3 {
	float v[3];

	vec3(float x = 0, float y = 0, float z = 0) { v[0] = x; v[1] = y; v[2] = z; }
	vec3 operator*(float s) {
		vec3 res(v[0] * s, v[1] * s, v[2] * s);
		return res;
	}
};

// row-major matrix 4x4
struct mat4 {
	float m[4][4];
public:
	mat4() {}
	mat4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) {
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	mat4 operator*(const mat4& right) const {
		mat4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
			}
		}
		return result;
	}
	operator float*() {
		return &m[0][0];
	}
};

// 3D point in homogeneous coordinates
struct vec4 {
	float x, y, z, w;
	vec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) { x = _x; y = _y; z = _z; w = _w; }

	vec4 operator*(const mat4& mat) const {
		return vec4(x * mat.m[0][0] + y * mat.m[1][0] + z * mat.m[2][0] + w * mat.m[3][0],
			x * mat.m[0][1] + y * mat.m[1][1] + z * mat.m[2][1] + w * mat.m[3][1],
			x * mat.m[0][2] + y * mat.m[1][2] + z * mat.m[2][2] + w * mat.m[3][2],
			x * mat.m[0][3] + y * mat.m[1][3] + z * mat.m[2][3] + w * mat.m[3][3]);
	}
};

// handle of the shader program
unsigned int shaderProgram;

class Triangle {
	unsigned int vao;	// vertex array object id
	float phi;			// rotation
	vec2 vertices[3];
	vec3 vertexColors[3];
public:
	Triangle(vec2 a = vec2(-2, -2), vec2 b = vec2(0, 2), vec2 c = vec2(2, -2), float phi = float(0), vec3 color = vec3(1,0,0)) {
		vertices[0] = a;
		vertices[1] = b;
		vertices[2] = c;
		this->phi = phi;
		for (int i = 0; i < 3; i++) {
			vertexColors[i] = color;
		}

		Animate(phi);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo[2];		// vertex buffer objects
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

									// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			sizeof(vertices), // number of the vbo in bytes
			vertices,		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
								   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			2, GL_FLOAT,  // components/attribute, component type
			GL_FALSE,		// not in fixed point format, do not normalized
			0, NULL);     // stride and offset: it is tightly packed

						  // vertex colors: vbo[1] -> Attrib Array 1 -> vertexColor of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	// copy to the GPU

																							// Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);  // Vertex position
									   // Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}

	void Animate(float t) {
		phi = t;
	}

	void Draw() {
		mat4 MVPTransform(0.1f * cosf(phi), 0.1f * sinf(phi), 0.0f, 0.0f,
			-0.1f * sin(phi), 0.1f * cosf(phi), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLES, 0, 3);	// draw a single triangle with vertices defined in vao
	}
};

class Quad {
	vec2 topleft;
	float side;
	unsigned int vao;
	float phi;
	vec2 vertices[4];
	vec3 vertexColors[4];
public:
	Quad(vec2 topleft = vec2(0,0), float side = float(2), float phi = float(0), vec3 color = vec3(1,0,0)) {
		vertices[0] = vec2(topleft[0], topleft[1]);
		vertices[1] = vec2(topleft[0]+side, topleft[1]);
		vertices[2] = vec2(topleft[0] + side, topleft[1]+side);
		vertices[3] = vec2(topleft[0], topleft[1] + side);

		for (int i = 0; i < 4; i++) {
			vertexColors[i] = color;
		}

		Animate(phi);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo[2];		// vertex buffer objects
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

									// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array

		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			sizeof(vertices), // number of the vbo in bytes
			vertices,		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
								   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			2, GL_FLOAT,  // components/attribute, component type
			GL_FALSE,		// not in fixed point format, do not normalized
			0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	// copy to the GPU

																							// Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);  // Vertex position
									   // Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}
	void Animate(float t) { phi = t; }

	void Draw() {
		mat4 MVPTransform(0.1f * cosf(phi), 0.1f * sinf(phi), 0.0f, 0.0f,
			-0.1f * sin(phi), 0.1f * cosf(phi), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw a single triangle with vertices defined in vao
	}
};

class Circle {
	unsigned int vao;
	float phi;
	vec2 center;
	float radius;
	vec2 vertices[32 + 2];
	vec3 vertexColors[32 +2];
public:
	Circle(vec2 center = vec2(0, 0), float radius = float(1), float phi = float(0), vec3 color = vec3(1, 1, 0)) {
		this->center = center;
		this->radius = radius;
		this->phi = phi;
		vertices[0] = center;
		for (int i = 1; i < 32 + 2; i++) {
			vertices[i] = vec2(center[0]+radius*cosf(float(i) / float(32)*float(2)*float(M_PI)), center[1]+radius*sinf(float(i) / float(32)*float(2)*float(M_PI)));
		}

		for (int i = 0; i < 32 + 2; i++) {
			vertexColors[i] = color;
		}

		Animate(phi);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo[2];		// vertex buffer objects
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

									// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array

		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			sizeof(vertices), // number of the vbo in bytes
			vertices,		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
								   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			2, GL_FLOAT,  // components/attribute, component type
			GL_FALSE,		// not in fixed point format, do not normalized
			0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	// copy to the GPU

																							// Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);  // Vertex position
									   // Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}
	void Animate(float t) { phi = float(t); }

	void Draw() {
		mat4 MVPTransform(0.1f * cosf(phi), 0.1f * sinf(phi), 0.0f, 0.0f,
			-0.1f * sinf(phi), 0.1f * cosf(phi), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLE_FAN, 0, 32 + 2);	// draw a single triangle with vertices defined in vao
	}
};

class Flower {
	Circle circle;
	vec2 center;
	float radius;
	float outerRadius;
	float phi;
	int numTriangles;
	std::vector<Triangle> triangles;
public:
	Flower(vec2 center = vec2(0,0), int numTriangles = 5) {
		this->center = center;
		radius = float(0.5);
		outerRadius = float(0.8);
		phi = float(0);
		this->numTriangles = numTriangles;
		for (int i = 0; i < numTriangles; i++) {
			Triangle triangle = Triangle(
				vec2(	center[0] + (radius * cosf(float(i) / float(numTriangles)*float(2)*float(M_PI))),
						center[1] + (radius * sinf(float(i) / float(numTriangles)*float(2)*float(M_PI)))),
				vec2(	center[0] + (radius * cosf(float(i+1) / float(numTriangles)*float(2)*float(M_PI))),
						center[1] + (radius * sinf(float(i+1) / float(numTriangles)*float(2)*float(M_PI)))),
				vec2(	center[0] + (outerRadius * cosf(float(i+0.5) / float(numTriangles)*float(2)*float(M_PI))),
						center[1] + (outerRadius * sinf(float(i + 0.5) / float(numTriangles)*float(2)*float(M_PI)))));
			triangles.push_back(triangle);
		}
		circle = Circle(center, radius, phi, vec3(1, 1, 0));
		Animate(phi);
	}

	void Create() {
		for (int i = 0; i < numTriangles; i++) {
			triangles[i].Create();
		}
		circle.Create();
	}

	void Animate(float t) {
		phi = float(t);
		for (int i = 0; i < numTriangles; i++) {
			triangles[i].Animate(phi);
		}
		circle.Animate(phi);
	}

	void Draw() {
		for (int i = 0; i < numTriangles; i++) {
			triangles[i].Draw();
		}
		circle.Draw();
	}

};

class ButterflyPart {
	unsigned int vao;
	float phi;
	vec2 center;
	float a, b;
	vec2 vertices[32 + 2];
	vec3 vertexColors[32 + 2];
public:
	ButterflyPart(vec2 center = vec2(0,0), float a = float(0.2), float b = float(1), float phi = float(0), vec3 color = vec3(1,0,1)) {
		this->center = center;
		this->a = a;
		this->b = b;
		this->phi = phi;
		vertices[0] = center;
		for (int i = 1; i < 32 + 2; i++) {
			vertices[i] = vec2(	center[0] + a * cosf(float(i) / float(32)*float(2)*float(M_PI)),
								center[1] + b * sinf(float(i) / float(32)*float(2)*float(M_PI)));
		}

		for (int i = 0; i < 32 + 2; i++) {
			vertexColors[i] = color;
		}

		Animate(phi);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo[2];		// vertex buffer objects
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

									// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array

		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			sizeof(vertices), // number of the vbo in bytes
			vertices,		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
								   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			2, GL_FLOAT,  // components/attribute, component type
			GL_FALSE,		// not in fixed point format, do not normalized
			0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	// copy to the GPU

																							// Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);  // Vertex position
									   // Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}
	void Animate(float t) { phi = float(t); }

	void Move(vec2 center) {
		this->center = center;
	}

	void Draw() {
		mat4 MVPTransform(0.1f * cosf(phi), 0.1f * sinf(phi), 0.0f, 0.0f,
			-0.1f * sinf(phi), 0.1f * cosf(phi), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			center[0], center[1], 0.0f, 1.0f);

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		int location = glGetUniformLocation(shaderProgram, "MVP");
		if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVPTransform); // set uniform variable MVP to the MVPTransform
		else printf("uniform MVP cannot be set\n");

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLE_FAN, 0, 32 + 2);	// draw a single triangle with vertices defined in vao
	}

};


// The virtual world
const int numFlowers = 7;
Flower flowers[numFlowers];
ButterflyPart butterflyBody = ButterflyPart();

// Initialization, create an OpenGL context
void onInitialization() {
	int fib1 = 1;
	int fib2 = 1;
	for (int i = 0; i < numFlowers; i++) {
		int temp = fib1 + fib2;
		fib1 = fib2;
		fib2 = temp;
		flowers[i] = Flower(
			vec2(	float(i*2) * cosf(float(i) / float(numFlowers)*float(3)*float(M_PI)),
					float(i*2) * sinf(float(i) / float(numFlowers)*float(3)*float(M_PI))),
			temp);
	}
	glViewport(0, 0, windowWidth, windowHeight);

	// Create objects by setting up their vertex data on the GPU
	for (int i = 0; i < numFlowers; i++) {
		flowers[i].Create();
	}
	butterflyBody.Create();

	// Create vertex shader from string
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	// Create fragment shader from string
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "Fragment shader error");

	// Attach shaders to a single program
	shaderProgram = glCreateProgram();
	if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Connect the fragmentColor to the frame buffer memory
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");	// fragmentColor goes to the frame buffer memory

																// program packaging
	glLinkProgram(shaderProgram);
	checkLinking(shaderProgram);
	// make this program run
	glUseProgram(shaderProgram);
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 1, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
	for (int i = 0; i < numFlowers; i++) {
		flowers[i].Draw();
	}
	butterflyBody.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / windowHeight;
		butterflyBody.Move(vec2(cX, cY));
		glutPostRedisplay();     // redraw
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	float sec = time / 1000.0f;				// convert msec to sec
	glutPostRedisplay();					// redraw the scene
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Do not touch the code below this line

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_3_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow(argv[0]);

#if !defined(__APPLE__)
	glewExperimental = true;	// magic
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);                // Register event handlers
	glutMouseFunc(onMouse);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMotionFunc(onMouseMotion);

	glutMainLoop();
	onExit();
	return 1;
}
