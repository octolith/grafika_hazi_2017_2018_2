//=============================================================================================
// Image Viewer
// File name can be input from the console
// Right click will enable the magnifyer
//=============================================================================================
#define _USE_MATH_DEFINES
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
const char * vertexSource = R"(
	#version 330
	precision highp float;

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = (vertexPosition + vec2(1, 1)) / 2;						// from clipping to texture space
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// already in clipping space
	}
)";

// fragment shader in GLSL
const char * fragmentSources[] = {
	// Lens effect fragment shader
	R"(
	#version 330
	precision highp float;

	uniform sampler2D textureUnit;
	uniform vec2 texCursor;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		const float maxRadius2 = 0.05f; 
		float radius2 = dot(texCoord - texCursor, texCoord - texCursor);
		float scale = (radius2 < maxRadius2) ? radius2 / maxRadius2 : 1;
		vec2 transfTexCoord = (texCoord - texCursor) * scale + texCursor;
		fragmentColor = texture(textureUnit, transfTexCoord);
	}
)",
// Blackhole effect fragment shader
R"(
	#version 330
	precision highp float;

	uniform sampler2D textureUnit;
	uniform vec2 texCursor;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		const float eventRadius = 0.06f, ds = 0.01;
		vec3 point = vec3(texCoord, 0), dir = vec3(0, 0, 1), blackhole = vec3(texCursor, 0.5);
		for(int step = 0; step < 200; step++) {
			float distance2 = dot(blackhole - point, blackhole - point);
			if (distance2 < eventRadius * eventRadius) { fragmentColor = vec4(0, 0, 0, 1); break; }
			if (point.z > 1) { fragmentColor = texture(textureUnit, vec2(point.x, point.y)); break; }
			dir = dir * ds +  (blackhole - point) / sqrt(distance2) * eventRadius / 4 / distance2 * ds * ds;
			point += dir;
			dir = normalize(dir);
		} 
	}
)",
// Gaussian Blur fragment shader
R"(
	#version 330
	precision highp float;

	uniform sampler2D textureUnit;
	uniform vec2 texCursor;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		const int filterSize = 9;
		const float ds = 0.003f;
		float sigma2 = (dot(texCoord-texCursor, texCoord-texCursor)/5 + 0.001f) * ds;

		fragmentColor = vec4(0, 0, 0, 0);
		float totalWeight = 0f;
		for(int X = -filterSize; X <= filterSize; X++) {
			for(int Y = -filterSize; Y <= filterSize; Y++) {
				vec2 offset = vec2(X * ds, Y * ds);
				float weight = exp(-dot(offset, offset) / 2 / sigma2 );
				fragmentColor += texture(textureUnit, texCoord + offset) * weight;
				totalWeight += weight;
			}
		}
		fragmentColor /= totalWeight;
	}
)",
// Spiral fragment shader
R"(
	#version 330
	precision highp float;

	uniform sampler2D textureUnit;
	uniform vec2 texCursor;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		float angle = length(texCoord - texCursor) * 2;
		mat2 rotationMatrix = mat2(cos(angle), sin(angle), - sin(angle), cos(angle));
		vec2 transformedTexCoord = (texCoord - texCursor) * rotationMatrix + texCursor;
		fragmentColor += texture(textureUnit, transformedTexCoord);
	}
)"
};

// 2D point in Cartesian coordinates 
struct vec2 {
	float v[2];
	vec2(float x = 0, float y = 0) { v[0] = x; v[1] = y; }
};

// 3D point in Cartesian coordinates or RGB color
struct vec3 {
	float v[3];
	vec3(float x = 0, float y = 0, float z = 0) { v[0] = x; v[1] = y; v[2] = z; }
};

std::vector<vec3> CheckerBoard(int& width, int& height) {	// read image as BMP files 
	width = height = 128;
	std::vector<vec3> image(width * height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			image[y * width + x] = (((x / 16) % 2) ^ ((y / 16) % 2)) ? vec3(1, 1, 1) : vec3(0, 0, 0);
		}
	}
	return image;
}

std::vector<vec3> ReadBMP(const char * pathname, int& width, int& height) {	// read image as BMP files 
	FILE * file = fopen(pathname, "r");
	if (!file) {
		printf("%s does not exist\n", pathname);
		return CheckerBoard(width, height);
	}
	unsigned short bitmapFileHeader[27];					// bitmap header
	fread(&bitmapFileHeader, 27, 2, file);
	if (bitmapFileHeader[0] != 0x4D42) {   // magic number
		printf("Not bmp file\n");
		return CheckerBoard(width, height);
	}
	if (bitmapFileHeader[14] != 24) {
		printf("Only true color bmp files are supported\n");
		return CheckerBoard(width, height);
	}
	width = bitmapFileHeader[9];
	height = bitmapFileHeader[11];
	unsigned int size = (unsigned long)bitmapFileHeader[17] + (unsigned long)bitmapFileHeader[18] * 65536;
	fseek(file, 54, SEEK_SET);

	std::vector<byte> byteImage(size);
	fread(&byteImage[0], 1, size, file); 	// read the pixels
	fclose(file);

	std::vector<vec3> image(width * height);

	// Swap R and B since in BMP, the order is BGR
	int i = 0;
	for (int imageIdx = 0; imageIdx < size; imageIdx += 3) {
		image[i++] = vec3(byteImage[imageIdx + 2] / 256.0f, byteImage[imageIdx + 1] / 256.0f, byteImage[imageIdx] / 256.0f);
	}
	return image;
}

// handle of the shader program
const int nEffects = 4;
enum Effect { LENS, BLACKHOLE, GAUSSIAN, SPIRAL };
Effect effect = LENS;
unsigned int shaderPrograms[nEffects];

vec2 texLensPosition(0, 0);

class TexturedQuad {
	unsigned int vao, vbo, textureId;	// vertex array object id and texture id
	vec2 vertices[4];
public:
	TexturedQuad() {
		vertices[0] = vec2(-1, -1);
		vertices[1] = vec2(1, -1);
		vertices[2] = vec2(1, 1);
		vertices[3] = vec2(-1, 1);
	}
	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects

								// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);	   // copy to that part of the memory which is not modified 
																					   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		int width = 128, height = 128;
		std::vector<vec3> image = ReadBMP("image.bmp", width, height);

		// Create objects by setting up their vertex data on the GPU
		glGenTextures(1, &textureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, textureId);    // binding

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source

		int location = glGetUniformLocation(shaderPrograms[effect], "texCursor");
		if (location >= 0) glUniform2f(location, texLensPosition.v[0], texLensPosition.v[1]); // set uniform variable MVP to the MVPTransform
		else printf("texCursor cannot be set\n");

		location = glGetUniformLocation(shaderPrograms[effect], "textureUnit");
		if (location >= 0) {
			glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureId);	// connect the texture to the sampler
		}
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};

// popup menu event handler
void processMenuEvents(int option) {
	effect = (Effect)option;
	glUseProgram(shaderPrograms[effect]);
}

// The virtual world: a single full screen quad
TexturedQuad quad;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	// create the menu and tell glut that "processMenuEvents" will handle the events
	int menu = glutCreateMenu(processMenuEvents);
	//add entries to our menu
	glutAddMenuEntry("Lens effect", LENS);
	glutAddMenuEntry("Black hole ", BLACKHOLE);
	glutAddMenuEntry("Gaussian", GAUSSIAN);
	glutAddMenuEntry("Spiral", SPIRAL);
	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// create the full screen quad
	quad.Create();

	// Create vertex shader from string
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) {
		printf("Error in vertex shader creation\n");
		exit(1);
	}
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	for (int eff = 0; eff < nEffects; eff++) {
		// Create fragment shader from string
		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) {
			printf("Error in fragment shader creation\n");
			exit(1);
		}
		glShaderSource(fragmentShader, 1, &fragmentSources[eff], NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "Fragment shader error");

		// Attach shaders to a single program
		shaderPrograms[eff] = glCreateProgram();
		if (!shaderPrograms[eff]) {
			printf("Error in shader program creation\n");
			exit(1);
		}
		glAttachShader(shaderPrograms[eff], vertexShader);
		glAttachShader(shaderPrograms[eff], fragmentShader);

		// Connect the fragmentColor to the frame buffer memory
		glBindFragDataLocation(shaderPrograms[eff], 0, "fragmentColor");	// fragmentColor goes to the frame buffer memory

																			// program packaging
		glLinkProgram(shaderPrograms[eff]);
		checkLinking(shaderPrograms[eff]);
	}
	// make this program run
	glUseProgram(shaderPrograms[LENS]);
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen

	quad.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

bool mouseLeftPressed = false;

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
	if (mouseLeftPressed) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		texLensPosition.v[0] = (float)pX / windowWidth;	// flip y axis
		texLensPosition.v[1] = 1.0f - (float)pY / windowHeight;
	}
	glutPostRedisplay();     // redraw
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) mouseLeftPressed = true;
		else					mouseLeftPressed = false;
	}
	onMouseMotion(pX, pY);
}


// Idle event indicating that some time elapsed: do animation here
void onIdle() {
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
	return 1;
}