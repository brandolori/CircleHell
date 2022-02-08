# include <iostream>
# include "ShaderMaker.h"
# include <GL/glew.h>
# include <GL/freeglut.h>
# include <deque>
# include <glm/glm.hpp>
# include <glm/gtc/matrix_transform.hpp>
# include <glm/gtx/transform.hpp>
# include <glm/gtc/type_ptr.hpp>
# include <math.h>

using namespace glm;

static unsigned int programId;
#define PI 3.14159265358979323846

unsigned int VAO, shipVAO;
unsigned int VBO, shipVBO, projMatrix, modelMatrix;

mat4 Projection;  //Matrice di proiezione
mat4 Model; //Matrice per il cambiamento di sistema di riferimento: da Sistema diriferimento dell'oggetto a sistema di riferimento nel Mondo

typedef struct {
	float x, y, z, r, g, b, a;
} Point;

typedef struct {
	float length;
	float offset;
	float radius;
	float depth;
} ArcData;

// costanti
const int arcSize = 100;
const int arcTime = 2000;
const int arcSpeed = 2;
const float shipOrbitRadius = 220;
const float shipMovementForce = 0.01;
const float shipMaxSpeed = 4 * PI / 60;
const float timeStep = 1000 / 60;
const int width = 1280;
const int height = 720;

// stato della partita
deque<ArcData> arcs;
bool pressingRight = false;
bool pressingLeft = false;
float shipAngle = PI / 2;
int score = 0;
double timePassed = 0;
float shipSpeed = 0;
Point* points = new Point[arcSize];

float randFloat(float range) {
	return float(rand()) / float((RAND_MAX)) * range;
}

void generateArc(float length, int nPoints, float innerRadius, float outerRadius, Point* points) {
	float angle, distance;
	bool external = true;
	float pass = length / nPoints;
	for (int i = 0; i < nPoints; i++) {
		angle = i * pass;
		distance = external ? outerRadius : innerRadius;
		points[i].x = cos(angle) * distance;
		points[i].y = sin(angle) * distance;
		points[i].z = 1.0;
		points[i].r = external ? 1.0 : 0.5;
		points[i].g = 1.0;
		points[i].b = external ? 1.0 : 0.5;
		points[i].a = 1.0;

		external = !external;
	}
}

void addArc(float radius, float depth) {

	ArcData* arc = new ArcData;

	arc->radius = radius;
	arc->depth = depth;
	arc->offset = randFloat(2 * PI);
	arc->length = randFloat(PI) + PI / 2;

	arcs.push_front(*arc);
}

float shipVertices[] = {
	-2.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
	1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0,
	1.0, -1.0, 0.0, 1.0, 1.0, 1.0, 1.0
};

void init(void) {
	GLenum ErrorCheckValue = glGetError();

	char* vertexShader = (char*)"vertexShader_C_M.glsl";
	char* fragmentShader = (char*)"fragmentShader_C_M.glsl";

	programId = ShaderMaker::createProgram(vertexShader, fragmentShader);

	glUseProgram(programId);

	srand((unsigned)time(0));

	glClearColor(0.0, 0.0, 0.0, 1.0);

	//Genero il VAO della navicella
	glGenVertexArrays(1, &shipVAO);
	glBindVertexArray(shipVAO);
	glGenBuffers(1, &shipVBO);
	glBindBuffer(GL_ARRAY_BUFFER, shipVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(shipVertices), shipVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	projMatrix = glGetUniformLocation(programId, "Projection");
	modelMatrix = glGetUniformLocation(programId, "Model");
}

void drawScene(void) {

	float scaleX = 1 + cos(timePassed / 3) / 5;
	float scaleY = 1 + sin(timePassed / 2) / 5;

	float shipScale = 1 + sin(timePassed * 2.33 * 2 * PI) / 5;

	glClear(GL_COLOR_BUFFER_BIT);

	Projection = ortho(-float(width) * scaleX / 2, float(width) * scaleX / 2, -float(height) * scaleY / 2, float(height) * scaleY / 2);

	glUniformMatrix4fv(projMatrix, 1, GL_FALSE, value_ptr(Projection));

	// Genera tutti gli archi
	for (int i = 0; i < arcs.size(); i++) {
		ArcData* arc = &arcs[i];

		//Genero il VAO
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		generateArc(arc->length, arcSize, arc->radius, arc->radius + arc->depth, points);
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, arcSize * sizeof(Point), &points[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		Model = mat4(1.0);
		Model = rotate(Model, arcs[i].offset, vec3(0.0, 0.0, 1.0));
		glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, value_ptr(Model));

		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, arcSize);
		glBindVertexArray(0);

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	// preparo le matrici per la navicella e la disegno
	Model = mat4(1.0);
	Model = translate(Model, vec3(cos(shipAngle) * shipOrbitRadius, sin(shipAngle) * shipOrbitRadius, 0));
	Model = scale(Model, vec3(12.0 * shipScale, 12.0 * shipScale, 1.0));
	Model = rotate(Model, shipAngle, vec3(0.0, 0.0, 1.0));
	glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, value_ptr(Model));

	glBindVertexArray(shipVAO);
	glDrawArrays(GL_TRIANGLES, 0, sizeof(shipVertices));

	glutSwapBuffers();
}

void onKeyboardPressed(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		pressingLeft = true;
		break;
	case 'd':
		pressingRight = true;
		break;
	}
}

void onKeyboardReleased(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		pressingLeft = false;
		break;
	case 'd':
		pressingRight = false;
		break;
	}
}

void updateArcs(int a) {
	addArc(800, 50);
	glutTimerFunc(arcTime, updateArcs, 0);
}

void update(int a) {

	timePassed += timeStep / 1000;

	for (auto& arc : arcs) {
		arc.radius -= arcSpeed;
	}

	if (arcs.size() > 0 && arcs[arcs.size() - 1].radius < 5) {
		arcs.pop_back();
		score += 1;
		cout << "punteggio = " << score << endl;
	}

	//ship movement
	if (pressingLeft) {
		shipSpeed += shipMovementForce;
	}

	if (pressingRight) {
		shipSpeed -= shipMovementForce;
	}

	if (abs(shipSpeed) > shipMaxSpeed) {
		shipSpeed = sign(shipSpeed) * shipMaxSpeed;
	}

	if (shipSpeed > 0) {
		shipSpeed -= shipMovementForce / 2;
		if (shipSpeed < 0)
			shipSpeed = 0;
	}

	if (shipSpeed < 0) {
		shipSpeed += shipMovementForce / 2;
		if (shipSpeed > 0)
			shipSpeed = 0;
	}

	shipAngle = fmod(shipAngle + 2 * PI + shipSpeed, 2 * PI);

	//collisions
	bool hit = false;
	for (auto& arc : arcs) {
		if (arc.radius < shipOrbitRadius && arc.radius + arc.depth > shipOrbitRadius) {
			if (arc.offset + arc.length > 2 * PI) {
				if (shipAngle > arc.offset)
					hit = true;
				else if (shipAngle < fmod(arc.offset + arc.length, 2 * PI))
					hit = true;
			}
			else if (arc.offset < shipAngle && arc.offset + arc.length > shipAngle) {
				hit = true;
			}
		}
	}

	if (hit) {
		arcs.clear();
		score = 0;
		cout << "punteggio = " << score << endl;
	}

	glutPostRedisplay();

	glutTimerFunc(timeStep, update, 0);
}

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);

	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(width, height);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("CircleHell");

	glutDisplayFunc(drawScene);

	glutKeyboardFunc(onKeyboardPressed);
	glutKeyboardUpFunc(onKeyboardReleased);

	glutTimerFunc(timeStep, update, 0);
	glutTimerFunc(arcTime, updateArcs, 0);

	glewInit();

	init();

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glutMainLoop();
}