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
// Nev    : Levai Marcell
// Neptun : ZCH7OS
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

const float EPSILON = 0.001f;
const int MAXDEPTH = 5;

void getErrorInfo(unsigned int handle) {
	int logLen, written;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

// check if shader could be compiled
void checkShader(unsigned int shader, const char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) {
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

// check if shader could be linked
void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) {
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	out vec2 texcoord;

	void main() {
		texcoord = (vertexPosition + vec2(1, 1))/2;							// -1,1 to 0,1
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	in  vec2 texcoord;			// interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = texture(textureUnit, texcoord); 
	}
)";

// 3D point in Cartesian coordinates or RGB color
struct vec3 {
	float x, y, z;
	vec3(float x0 = 0, float y0 = 0, float z0 = 0) { x = x0; y = y0; z = z0; }
	vec3 operator*(float a) const { return vec3(x * a, y * a, z * a); }
	vec3 operator/(float d) const { return vec3(x / d, y / d, z / d); }
	vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
	void operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; }
	vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
	vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
	vec3 operator-() const { return vec3(-x, -y, -z); }
	vec3 normalize() const { return (*this) * (1 / (Length() + 0.000001f)); }
	float Length() const { return sqrtf(x * x + y * y + z * z); }
	operator float*() { return &x; }
};

float dot(const vec3& v1, const vec3& v2) {
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

vec3 cross(const vec3& v1, const vec3& v2) {
	return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

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
	operator float*() { return &m[0][0]; }
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

float sign(float f) {
	if (f < 0) {
		return -1.0f;
	}
	return 1.0f;
}

// handle of the shader program
unsigned int shaderProgram;

class Material {
	vec3   F0;	// F0
	vec3 kd, ks;
	float n;	// n: why not spectrum?
	vec3 k;
	float  shininess;
public:
	bool rough, reflective, refractive;
	vec3 ka;
	Material(vec3 _ka, vec3 _kd, vec3 _ks, float _shininess, bool _rough, bool _reflective, bool _refractive) : ka(_ka), kd(_kd), ks(_ks) {
		shininess = _shininess;
		rough = _rough;
		reflective = _reflective;
		refractive = _refractive;

		if (reflective || refractive) {
			float r = (powf(0.17 - 1, 2) + powf(3.1, 2)) / (powf(0.17 + 1, 2) + powf(3.1, 2));
			float g = (powf(0.35 - 1, 2) + powf(2.7, 2)) / (powf(0.35 + 1, 2) + powf(2.7, 2));
			float b = (powf(1.5 - 1, 2) + powf(1.9, 2)) / (powf(1.5 + 1, 2) + powf(1.9, 2));
			F0 = vec3(r, g, b);
		}
	}
	vec3 shade(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 inRad) {
		vec3 reflRad(0, 0, 0);
		float cosTheta = dot(normal, lightDir);
		if (cosTheta < 0) return reflRad;
		reflRad = inRad * kd * cosTheta;
		vec3 halfway = (viewDir + lightDir).normalize();
		float cosDelta = dot(normal, halfway);
		if (cosDelta < 0) return reflRad;
		return reflRad + inRad * ks * pow(cosDelta, shininess);
	}
	vec3 reflect(vec3 inDir, vec3 normal) {
		return inDir - normal * dot(normal, inDir) * 2.0f;
	}
	vec3 refract(vec3 inDir, vec3 normal) {
		float ior = n; // depends on whether from inside or outside
		float cosa = -dot(normal, inDir); // from inside?
		if (cosa < 0) { cosa = -cosa; normal = -normal; ior = 1 / n; }
		float disc = 1 - (1 - cosa * cosa) / ior / ior;
		if (disc < 0) return reflect(inDir, normal); // magic !!!
		return inDir / ior + normal * (cosa / ior - sqrt(disc));
	}
	vec3 Fresnel(vec3 inDir, vec3 normal) { // in/out irrelevant
		float cosa = fabs(dot(normal, inDir));
		return F0 + (vec3(1, 1, 1) - F0) * pow(1 - cosa, 5);
	}
};

struct Hit {
	float t;
	vec3 position;
	vec3 normal;
	Material * material;
	Hit() { t = -1; }
};

struct Ray {
	vec3 start, dir;
	Ray(vec3 _start, vec3 _dir) { start = _start; dir = _dir.normalize(); }
};


class Intersectable {
protected:
	Material * material;
public:
	virtual Hit intersect(const Ray& ray) = 0;
};

//struct Room

struct Sphere : public Intersectable {
	vec3 center;
	float radius;

	Sphere(const vec3& _center, float _radius, Material* _material) {
		center = _center;
		radius = _radius;
		material = _material;
	}
	Hit intersect(const Ray& ray) {
		Hit hit;
		vec3 dist = ray.start - center;
		float b = dot(dist, ray.dir) * 2.0f;
		float a = dot(ray.dir, ray.dir);
		float c = dot(dist, dist) - radius * radius;
		float discr = b * b - 4.0f * a * c;
		if (discr < 0) return hit;
		float sqrt_discr = sqrtf(discr);
		float t1 = (-b + sqrt_discr) / 2.0f / a;
		float t2 = (-b - sqrt_discr) / 2.0f / a;
		if (t1 <= 0 && t2 <= 0) return hit;
		if (t1 <= 0 && t2 > 0)       hit.t = t2;
		else if (t2 <= 0 && t1 > 0)  hit.t = t1;
		else if (t1 < t2)            hit.t = t1;
		else                         hit.t = t2;
		hit.position = ray.start + ray.dir * hit.t;
		hit.normal = (hit.position - center) / radius;
		hit.material = material;
		return hit;
	}
};

// http://www.bmsc.washington.edu/people/merritt/graphics/quadrics.html
class QuadricSurfaces : public Intersectable {
protected:
	float A, B, C, D, E, F, G, H, I, J;
public:
	Hit intersect(const Ray& ray) {
		float Aq = A * powf(ray.dir.x, 2) + B * powf(ray.dir.y, 2) + C * powf(ray.dir.z, 2) +
			D * ray.dir.x * ray.dir.y +
			E * ray.dir.x * ray.dir.z +
			F * ray.dir.y * ray.dir.z;

		float Bq = 2 * A * ray.start.x * ray.dir.x + 2 * B * ray.start.y * ray.dir.y + 2 * C * ray.start.z * ray.dir.z +
			D * (ray.start.x * ray.dir.y + ray.start.y * ray.dir.x) +
			E * (ray.start.x * ray.dir.z + ray.start.z * ray.dir.x) +
			F * (ray.start.y * ray.dir.z + ray.start.z * ray.dir.y) +
			G * ray.dir.x + H * ray.dir.y + I * ray.dir.z;

		float Cq = A * powf(ray.start.x, 2) + B * powf(ray.start.y, 2) + C * powf(ray.start.z, 2) +
			D * ray.start.x * ray.start.y +
			E * ray.start.x * ray.start.z +
			F * ray.start.y * ray.start.z +
			G * ray.start.x + H * ray.start.y + I * ray.start.z + J;

		float disc = Bq * Bq - 4 * Aq * Cq;

		float t;
		if (fabsf(Aq) < EPSILON) {
			t = -Cq / Bq;
		}
		else {
			if (disc < 0) {
				return Hit();
			}

			t = (-Bq - sqrtf(disc)) / (2 * Aq);

			if (t < 0) {
				t = (-Bq + sqrtf(disc)) / (2 * Aq);
			}
		}

		vec3 intersection = ray.start + ray.dir.normalize() * t;
		vec3 normal = vec3(2 * A * intersection.x + D * intersection.y + E * intersection.z + G,
			2 * B * intersection.y + D * intersection.x + F * intersection.z + H,
			2 * C * intersection.z + E * intersection.x + F * intersection.y + I).normalize();

		Hit hit;
		hit.normal = normal;
		hit.position = intersection;
		hit.t = t;
		hit.material = material;

		return hit;
	}
};

class Ellipsoid : public QuadricSurfaces {
	vec3 center, i;
	float a, b;
public:
	Ellipsoid(vec3 _center, float _a, float _b, vec3 _i, Material* _material) {
		center = _center;
		a = _a;
		b = _b;
		i = _i;
		material = _material;

		float f = sqrtf(a*a - b * b);

		vec3 F1 = center + i * f;
		vec3 F2 = center - i * f;

		float x1 = F1.x;
		float y1 = F1.y;
		float z1 = F1.z;
		float x2 = F2.x;
		float y2 = F2.y;
		float z2 = F2.z;

		float F1LS = F1.Length() * F1.Length();
		float F2LS = F2.Length() * F2.Length();

		A = powf(x1 - x2, 2) - 4 * a*a;
		B = powf(y1 - y2, 2) - 4 * a*a;
		C = powf(z1 - z2, 2) - 4 * a*a;
		D = 2 * (x1 - x2) * (y1 - y2);
		E = 2 * (x1 - x2) * (z1 - z2);
		F = 2 * (y1 - y2) * (z1 - z2);
		G = 4 * a*a*(x1 + x2) + F1LS * (x2 - x1) + F2LS * (x1 - x2);
		H = 4 * a*a*(y1 + y2) + F1LS * (y2 - y1) + F2LS * (y1 - y2);
		I = 4 * a*a*(z1 + z2) + F1LS * (z2 - z1) + F2LS * (z1 - z2);
		J = (powf(F1LS - F2LS, 2) / 4.0f) + 2 * a * a * (2 * a*a - F1LS + F2LS - 2 * (x2*x2 + y2 * y2 + z2 * z2));

		if (A != 0) {
			B /= A;
			C /= A;
			D /= A;
			E /= A;
			F /= A;
			G /= A;
			H /= A;
			I /= A;
			J /= A;
			A /= A;
		}
	}

};

class Paraboloid : public QuadricSurfaces {
public:
	Paraboloid(vec3 center, float a, Material* _material) {
		material = _material;
		A = D = E = F = 0;
		B = C = 1;

		G = a * a;
		H = -2 * center.y;
		I = -2 * center.z;
		J = -a * a*center.x + center.y*center.y + center.z*center.z;
	}
};

class Camera {
	vec3 eye, lookat, right, up;
public:
	void set(vec3 _eye, vec3 _lookat, vec3 vup, double fov) {
		eye = _eye;
		lookat = _lookat;
		vec3 w = eye - lookat;
		float f = w.Length();
		right = cross(vup, w).normalize() * f * tanf((float)fov / 2);
		up = cross(w, right).normalize() * f * tanf((float)fov / 2);
	}
	Ray getray(int X, int Y) {
		vec3 dir = lookat + right * (2.0f * (X + 0.5f) / windowWidth - 1) + up * (2.0f * (Y + 0.5f) / windowHeight - 1) - eye;
		return Ray(eye, dir);
	}
};

struct Light {
	vec3 direction;
	vec3 Le;
	Light(vec3 _direction, vec3 _Le) {
		direction = _direction.normalize();
		Le = _Le;
	}
};

class Scene {
	std::vector<Intersectable *> objects;
	std::vector<Light *> lights;
	Camera camera;
	vec3 La;
public:
	void build() {
		vec3 eye = vec3(0, 0, 2);
		vec3 vup = vec3(0, 1, 0);
		vec3 lookat = vec3(0, 0, 0);
		float fov = 45 * (float)M_PI / 180;
		camera.set(eye, lookat, vup, fov);

		La = vec3(0.1f, 0.1f, 0.1f);
		lights.push_back(new Light(vec3(1, 1, 1), vec3(3, 3, 3)));
		lights.push_back(new Light(vec3(-1, 0, 1), vec3(0.5, 3, 0.5)));

		vec3 kd, ks, ka;
		kd = vec3(0.4f, 0.1f, 0.4f);
		ks = vec3(2, 2, 2);
		ka = kd * (float)M_PI;
		objects.push_back(new Sphere(vec3(-1, 0.5, -2), 0.5f, new Material(ka, kd, ks, 50, true, false, false)));
		kd = vec3(0.5f, 0.05f, 0.05f);
		ks = vec3(3, 3, 3);
		ka = kd * (float)M_PI;
		objects.push_back(new Paraboloid(vec3(1, -0.7, -5), 0.7f, new Material(ka, kd, ks, 50, true, false, false)));
		kd = vec3(0.1f, 0.3f, 0.3f);
		ks = vec3(4, 4, 4);
		ka = kd * (float)M_PI;
		objects.push_back(new Ellipsoid(vec3(0, 0.5, -1.5), 1.0f, 0.5f, vec3(1, 0, 0), new Material(ka, kd, ks, 50, true, false, false)));
	}

	void render(vec3 image[]) {
#pragma omp parallel for
		for (int Y = 0; Y < windowHeight; Y++) {
			for (int X = 0; X < windowWidth; X++) image[Y * windowWidth + X] = trace(camera.getray(X, Y));
		}
	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		for (Intersectable * object : objects) {
			Hit hit = object->intersect(ray); //  hit.t < 0 if no intersection
			if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))  bestHit = hit;
		}
		return bestHit;
	}

	vec3 trace(Ray ray, int depth = 0) {
		if (depth > MAXDEPTH) return La;
		Hit hit = firstIntersect(ray);
		if (hit.t < 0) return La;
		vec3 outRadiance;
		if (hit.material->rough) {
			outRadiance = hit.material->ka * La;
			for (Light * light : lights) {
				Ray shadowRay(hit.position + hit.normal * EPSILON, light->direction);
				Hit shadowHit = firstIntersect(shadowRay);
				if (shadowHit.t < 0 || shadowHit.t > fabs((hit.position - light->direction).Length())) {
					outRadiance += hit.material->shade(hit.normal, -ray.dir, light->direction, light->Le);
				}
			}
		}
		if (hit.material->reflective) {
			vec3 reflectionDir = hit.material->reflect(ray.dir, hit.normal);
			Ray reflectedRay(hit.position + hit.normal * EPSILON * sign(dot(hit.normal, -ray.dir)), reflectionDir);
			outRadiance += trace(reflectedRay, depth + 1) * hit.material->Fresnel(-ray.dir, hit.normal);
		}
		if (hit.material->refractive) {
			vec3 refractionDir = hit.material->refract(ray.dir, hit.normal);
			Ray refractedRay(hit.position - hit.normal * EPSILON * sign(dot(hit.normal, -ray.dir)), refractionDir);
			outRadiance += trace(refractedRay, depth + 1) * (vec3(1, 1, 1) - hit.material->Fresnel(-ray.dir, hit.normal));
		}

		return outRadiance;
	}
};

Scene scene;

class FullScreenTexturedQuad {
	unsigned int vao, textureId;	// vertex array object id and texture id
public:
	void Create(vec3 image[]) {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo;		// vertex buffer objects
		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects

								// vertex coordinates: vbo0 -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };	// two triangles forming a quad
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

																	  // Create objects by setting up their vertex data on the GPU
		glGenTextures(1, &textureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, textureId);    // binding
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, image); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		int location = glGetUniformLocation(shaderProgram, "textureUnit");
		if (location >= 0) {
			glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureId);	// connect the texture to the sampler
		}
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};


// The virtual world: single quad
FullScreenTexturedQuad fullScreenTexturedQuad;
vec3 image[windowWidth * windowHeight];	// The image, which stores the ray tracing result

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	scene.build();
	scene.render(image);
	fullScreenTexturedQuad.Create(image);

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
	glUseProgram(shaderProgram); 	// make this program run
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
	fullScreenTexturedQuad.Draw();
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
