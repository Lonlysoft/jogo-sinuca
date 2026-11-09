#include <GL/glut.h>
#include <GL/glu.h>
#include "mybib.h"
#include "stb_image.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//declarar PI caso não exista
#ifndef M_PI
#define M_PI 3.1415
#endif

#define TABLE_WIDTH	400.0f
#define TABLE_HEIGHT 200.0f

#define NUMERO_BOLAS 2
#define RAIO_BOLA 8.0f
#define RAIO_BURACO 18.0f
#define FRICTION 0.988f
#define RESTITUTION 0.92f

#define MAX_POWER	500.0f
#define MAX_PULLBACK 50.0f
#define TACO_LENGTH	100.0f

//camera's modes
#define FOLLOWING_MODE 0
#define STATIC_MODE 1

//momentos do jogo
#define TITLE 0
#define MAIN_GAME 1
#define GAME_OVER 2
#define PAUSE 3

//velocidade maxima que as bolas precisam estar para que o jogo conte uma rodada 
#define VEL_PARADO 2.0f

int windowWidth = 800, windowHeight = 600;
int GameMoment = MAIN_GAME;

typedef struct Camera {
	float ax, ay, az;
	float radius, tetha, phi;
	float ux, uy, uz;
	int mode;
} Camera;

Camera camera;

void CAM__create(){
	camera.ax = 0; camera.ay = 0; camera.az = 0;
	camera.radius = 700;
	camera.tetha  = M_PI * 0.5; // equador
	camera.phi	= M_PI / 3.5;   // greenwich
	camera.ux = 0; camera.uy = 0; camera.uz = 1;
	camera.mode = STATIC_MODE;
}

void CAM__update(){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)windowWidth/(double)windowHeight, 0.1, 3000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	float ex = camera.ax + camera.radius * sinf(camera.phi) * cosf(camera.tetha);
	float ey = camera.ay + camera.radius * sinf(camera.phi) * sinf(camera.tetha);
	float ez = camera.az + camera.radius * cosf(camera.phi);
	gluLookAt(ex, ey, ez,
						camera.ax, camera.ay, camera.az,
						camera.ux, camera.uy, camera.uz);
}

typedef struct buraco {
	float x, y, z;
	float raio;
} Buraco;

typedef struct Mesa {
	Buraco* buracos;
	float w, h;
	int quantBuraco;
} Mesa;

typedef struct bola {
	float x, y, z;
	float vx, vy, vz;
	float raio;
	float* cor;
	int ativo;
	int onGround;
} Bola;

const float cores[][3] = {
	{1.00f, 0.85f, 0.00f},  /* amarelo       */
	{0.00f, 0.20f, 1.00f},  /* azul          */
	{1.00f, 0.15f, 0.15f},  /* vermelho      */
	{0.60f, 0.10f, 0.60f},  /* roxo          */
	{1.00f, 0.55f, 0.00f},  /* laranja       */
	{0.00f, 0.60f, 0.00f},  /* verde         */
	{0.50f, 0.25f, 0.05f},  /* marrom        */
	{0.10f, 0.10f, 0.10f},  /* preto         */
	{1.00f, 1.00f, 0.00f},  /* amarelo claro */
	{0.90f, 0.40f, 0.60f},  /* rosa          */
	{0.50f, 0.80f, 1.00f},  /* azul claro    */
	{0.40f, 0.90f, 0.50f},  /* verde claro   */
	{0.95f, 0.60f, 0.30f},  /* salmão        */
	{0.70f, 0.40f, 0.90f},  /* lilás         */
	{0.30f, 0.30f, 0.60f},  /* azul escuro   */
	{0.85f, 0.85f, 0.85f}   /* cinza claro   */
};

typedef struct Cenario {
	Mesa* mesa;
	Bola** bolas;   // bolas[0] = bola branca
	ObjModel fundo;
	int numBolas;
} Cenario;

Cenario cenario;

typedef struct Jogo {
	int rodada, pontuacao, bolasRestantes;
} Jogo;

Jogo jogo;

typedef struct Mouse {
	int x; int y;
	int lmb, mmb;
	int lastMidX, lastMidY;
	float anguloMira, pullback;
	float worldX, worldY;
} Mouse;

Mouse mouse = {
	0, 0, //x, y
	0, 0, //left mouse button, middle mouse button
	0, 0, //lastMidX, lastMidY
	0, 0, //anguloMira, pullback
	0, 0  //worldX, worlY
};

void BOLA__create(int i, float x, float y, float r, float g, float b){
	Bola* bola  = (Bola*)malloc(sizeof(Bola));
	bola->x = x; bola->y = y; bola->z = RAIO_BOLA;
	bola->vx = bola->vy = bola->vz = 0.0f;
	bola->raio = RAIO_BOLA;
	bola->cor  = (float*)malloc(3 * sizeof(float));
	bola->cor[0] = r; bola->cor[1] = g; bola->cor[2] = b;
	bola->ativo = 1;
	cenario.bolas[i] = bola;
}

void CAM__follow(Bola* b){
	camera.ax = b->x;
	camera.ay = b->y;
	//camera.az = b->z;
}

void CAM__limit(){
	if(camera.phi <= 0){
		camera.phi = 0;
	}
}

void SCENE__create(){
	cenario.mesa = (Mesa*)malloc(sizeof(Mesa));
	Mesa* m = cenario.mesa;
	m->w = TABLE_WIDTH;
	m->h = TABLE_HEIGHT;
	m->quantBuraco = 6;
	m->buracos = (Buraco*)malloc(6 * sizeof(Buraco));

	m->buracos[0] = (Buraco){-m->w/2, -m->h/2, 0, RAIO_BURACO};
	m->buracos[1] = (Buraco){ m->w/2, -m->h/2, 0, RAIO_BURACO};
	m->buracos[2] = (Buraco){-m->w/2, m->h/2, 0, RAIO_BURACO};
	m->buracos[3] = (Buraco){ m->w/2, m->h/2, 0, RAIO_BURACO};
	m->buracos[4] = (Buraco){0, -m->h/2, 0, RAIO_BURACO};
	m->buracos[5] = (Buraco){0, m->h/2, 0, RAIO_BURACO};
	
	int totalBalls = NUMERO_BOLAS;
	cenario.numBolas = totalBalls;
	cenario.bolas = (Bola**)malloc(totalBalls * sizeof(Bola*));

	//bola branca
	BOLA__create(0, -120.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	
	const float startX = 100.0f;
	const float passo  = RAIO_BOLA * 2.05f;
	int idx	= 1;
	int placed = 0;
	int row	= 0;
	int numBolasColoridas = NUMERO_BOLAS - 1;
	while (placed < numBolasColoridas){
		int capacity = row + 1;
		int toPlace  = capacity;
		if (placed + toPlace > numBolasColoridas)
			toPlace = numBolasColoridas - placed;
		float px = startX + row * passo;
		for(int col = 0; col < toPlace; col++){
			float py;
			if (toPlace == capacity){
				py = (col - row / 2.0f) * passo;
			} else {
				py = (col - (toPlace - 1) / 2.0f) * passo;
			}
			int ci = (idx - 1) % 16;
			BOLA__create(idx++, px, py,
					  cores[ci][0], cores[ci][1], cores[ci][2]);
		}

		placed += toPlace;
		row++;
	}
	jogo.rodada = 1;
	jogo.pontuacao = 0;
	jogo.bolasRestantes = totalBalls - 1;


	//declarar fundo
	loadOBJ("background.obj", "background.mtl", &cenario.fundo);
}

void BOLA__desenhar(Bola* b){
	if (!b->ativo) return;
	glPushMatrix();
		glTranslatef(b->x, b->y, b->z);
		GLfloat amb[]  = {b->cor[0]*0.25f, b->cor[1]*0.25f, b->cor[2]*0.25f, 1};
		GLfloat dif[]  = {b->cor[0], b->cor[1], b->cor[2], 1};
		GLfloat spec[] = {1,1,1,1};
	
		glMaterialfv(GL_FRONT, GL_AMBIENT,  amb);
		glMaterialfv(GL_FRONT, GL_DIFFUSE,  dif);
		glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
		glMaterialf (GL_FRONT, GL_SHININESS, 64.0f);
	
		glutSolidSphere(b->raio, 22, 22);
	glPopMatrix();
}

void MESA__desenhar(Mesa* m){

	//desenhar feltro verde
	//desabilitar a luz por enquanto.
	glDisable(GL_LIGHTING);
	glColor3f(0.0f, 0.42f, 0.16f);
	glBegin(GL_QUADS);
		glVertex3f(-m->w/2, -m->h/2, 0);
		glVertex3f( m->w/2, -m->h/2, 0);
		glVertex3f( m->w/2,  m->h/2, 0);
		glVertex3f(-m->w/2,  m->h/2, 0);
	glEnd();
	
	
	// desenhar bordas
	const float bw = 20.0f;
	glColor3f(0.35f, 0.18f, 0.05f);
	glBegin(GL_QUADS);
		//topo
		glVertex3f(-m->w/2-bw, m->h/2, 1);
		glVertex3f( m->w/2+bw, m->h/2, 1);
		glVertex3f( m->w/2+bw, m->h/2 + bw, 1);
		glVertex3f(-m->w/2-bw, m->h/2 + bw, 1);
		//base
		glVertex3f(-m->w/2-bw, -m->h/2,	1);
		glVertex3f( m->w/2+bw, -m->h/2,	1);
		glVertex3f( m->w/2+bw, -m->h/2 - bw, 1);
		glVertex3f(-m->w/2-bw, -m->h/2 - bw, 1);
		//esquerda
		glVertex3f(-m->w/2,	-m->h/2, 1);
		glVertex3f(-m->w/2, m->h/2, 1);
		glVertex3f(-m->w/2 - bw, m->h/2, 1);
		glVertex3f(-m->w/2 - bw,-m->h/2, 1);
		//direita
		glVertex3f(m->w/2, -m->h/2, 1);
		glVertex3f(m->w/2, m->h/2, 1);
		glVertex3f(m->w/2 + bw, m->h/2, 1);
		glVertex3f(m->w/2 + bw, -m->h/2, 1);
	glEnd();
	//bordas
	glPushMatrix();
	glRotatef(90, 1, 0, 0);
	glBegin(GL_QUADS);
		//topo
		glVertex3f(-m->w/2-bw, m->h/2, 1);
		glVertex3f( m->w/2+bw, m->h/2, 1);
		glVertex3f( m->w/2+bw, m->h/2 + bw, 1);
		glVertex3f(-m->w/2-bw, m->h/2 + bw, 1);
		//base
		
	glEnd();
	glPopMatrix();
	glEnable(GL_LIGHTING);
}

void BURACO__desenhar(Mesa* m){
	glDisable(GL_LIGHTING);
	glColor3f(0, 0, 0);
	for(int i = 0; i < m->quantBuraco; i++){
		Buraco* b = &m->buracos[i];
		glBegin(GL_TRIANGLE_FAN);
			glVertex3f(b->x, b->y, 0.2f);
			for(int a = 0; a <= 24; a++){
				float ang = a * 2.0f * (float)M_PI / 24.0f;
				glVertex3f(b->x + cosf(ang) * b->raio,
						   b->y + sinf(ang) * b->raio, 0.2f);
			}
		glEnd();
	}
	glEnable(GL_LIGHTING);
}

//O taco é desenhado atrás da bola branca, na direção oposta ao "aim".
void TACO__desenhar(Bola* ball, float ang, float pull){
	float dx = cosf(ang);
	float dy = sinf(ang);
	float px = -dy;   /* perpendicular no plano XY */
	float py =  dx;
	
	float tipDist = ball->raio + pull + 0.5f;
	float tipX = ball->x - dx * tipDist;
	float tipY = ball->y - dy * tipDist;
	float tipZ = ball->z;

	const float tiltZ = 10.0f;
	float endX = tipX - dx * TACO_LENGTH;
	float endY = tipY - dy * TACO_LENGTH;
	float endZ = tipZ + tiltZ;

	// vetor do taco normalizado (do cabo até a ponta)
	float vx = tipX - endX;
	float vy = tipY - endY;
	float vz = tipZ - endZ;
	float len = sqrtf(vx*vx + vy*vy + vz*vz);
	vx /= len; vy /= len; vz /= len;

	// material do taco (madeira)
	GLfloat amb[]  = {0.15f, 0.08f, 0.02f, 1.0f};
	GLfloat dif[]  = {0.72f, 0.50f, 0.20f, 1.0f};
	GLfloat spec[] = {0.30f, 0.25f, 0.20f, 1.0f};
	glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,   dif);
	glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
	glMaterialf (GL_FRONT, GL_SHININESS, 40.0f);

	// quadric para os cilindros
	GLUquadric* q = gluNewQuadric();
	gluQuadricNormals(q, GLU_SMOOTH);

	// cilindro com base fina para o corpo
	glPushMatrix();
		glTranslatef(endX, endY, endZ);
		
		float axisX =  vy, axisY = -vx;
		float sina  = sqrtf(axisX*axisX + axisY*axisY);
		float hori = atan2f(vy, vx);
		glRotatef(hori * 180.0f / (float)M_PI, 0, 0, 1);
		float pitch = atan2f(vz, sqrtf(vx*vx + vy*vy));
		glRotatef(-pitch * 180.0f / (float)M_PI, 0, 1, 0);
		glRotatef(90.0f, 0, 1, 0);
	
		// base mais grossa, topo mais fino
		gluCylinder(q, 3.2f, 1.2f, TACO_LENGTH, 20, 4);
		// tampa do cabo
		glPushMatrix();
			glRotatef(180.0f, 1, 0, 0);
			gluDisk(q, 0.0f, 3.2f, 20, 1);
		glPopMatrix();
	glPopMatrix();

	// ponteira branca (fita)
	GLfloat tipAmb[] = {0.4f, 0.4f, 0.4f, 1.0f};
	GLfloat tipDif[] = {0.95f, 0.95f, 0.95f, 1.0f};
	glMaterialfv(GL_FRONT, GL_AMBIENT,  tipAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,  tipDif);
	glMaterialf (GL_FRONT, GL_SHININESS, 80.0f);
	
	float wX = tipX - vx * 6.0f;
	float wY = tipY - vy * 6.0f;
	float wZ = tipZ - vz * 6.0f;

	glPushMatrix();
		glTranslatef(wX, wY, wZ);
		float hori2 = atan2f(vy, vx);
		glRotatef(hori2 * 180.0f / (float)M_PI, 0, 0, 1);
		float pitch2 = atan2f(vz, sqrtf(vx*vx + vy*vy));
		glRotatef(-pitch2 * 180.0f / (float)M_PI, 0, 1, 0);
		glRotatef(90.0f, 0, 1, 0);
		gluCylinder(q, 1.35f, 1.15f, 6.0f, 20, 2);
	glPopMatrix();

	// --- ponta de couro (escura) ---
	GLfloat tAmb[] = {0.02f, 0.01f, 0.00f, 1.0f};
	GLfloat tDif[] = {0.12f, 0.07f, 0.03f, 1.0f};
	glMaterialfv(GL_FRONT, GL_AMBIENT,  tAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE,  tDif);
	glMaterialf (GL_FRONT, GL_SHININESS, 10.0f);

	glPushMatrix();
		glTranslatef(tipX - vx * 2.0f, tipY - vy * 2.0f, tipZ - vz * 2.0f);
		glRotatef(hori2 * 180.0f / (float)M_PI, 0, 0, 1);
		glRotatef(-pitch2 * 180.0f / (float)M_PI, 0, 1, 0);
		glRotatef(90.0f, 0, 1, 0);
		gluCylinder(q, 1.15f, 1.05f, 2.5f, 20, 2);
	glPopMatrix();

	gluDeleteQuadric(q);
}

void HUD__desenhar(){
	glDisable(GL_LIGHTING);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, windowWidth, 0, windowHeight);
	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();

			glColor3f(1, 1, 1);
			char buf[256];
			sprintf(buf,
				"Pontos: %d   Bolas restantes: %d   Forca: %.0f%%",
				jogo.pontuacao, jogo.bolasRestantes,
				(mouse.pullback / MAX_PULLBACK) * 100.0f
			);
			glRasterPos2f(10, windowHeight - 22);
			for(char* c = buf; *c; c++){
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
			}
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_LIGHTING);
}

void desenharBackground(){
	glPushMatrix();
		glScalef(70,70,70);
		glTranslatef(1.0, 1.0, -1);
		glRotatef(90, 0, 0, 1);
		drawModel(&cenario.fundo);
	glPopMatrix();
}

void screenToWorld(Mouse* m){
	GLdouble mv[16], proj[16];
	GLint	vp[4];
	glGetDoublev(GL_MODELVIEW_MATRIX,  mv);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, vp);
	GLdouble nx, ny, nz, fx, fy, fz;
	gluUnProject(m->x, windowHeight - m->y, 0.0, mv, proj, vp, &nx, &ny, &nz);
	gluUnProject(m->x, windowHeight - m->y, 1.0, mv, proj, vp, &fx, &fy, &fz);
	
	if (fabs(fz - nz) < 1e-6){
		m->worldX = m->worldY = 0;
		return;
	}
	float t = (float)(-nz / (fz - nz));
	m->worldX = (float)(nx + t * (fx - nx));
	m->worldY = (float)(ny + t * (fy - ny));
}

int COL__AABB(int* a, int* b){
	int x = 0, y = 1, w = 2, h = 3;
	return (a[x] < b[x] + b[w]  && a[x] + a[w] > b[x])
		&& (a[y] < b[y] + b[h] && a[y] + a[h] > b[y]);
}

//aqui é uma colisao 2D simples. desconsiderando o z.
int COL__circleCircle(Bola* a, Bola* b){
	float dx = b->x - a->x;
	float dy = b->y - a->y;
	float distanciaAB = dx*dx + dy*dy;
	float somaDosRaios = a->raio+b->raio;
	return distanciaAB < somaDosRaios*somaDosRaios;
}
void resolverColisaoBolaBola(Bola* a, Bola* b){
	if (!COL__circleCircle(a, b)) return;
	float dx = b->x - a->x;
	float dy = b->y - a->y;
	float d2 = dx*dx + dy*dy;
	float soma = a->raio + b->raio;

	float d  = sqrtf(d2);
	float nx = dx / d;
	float ny = dy / d;

	float overlap = soma - d;
	a->x -= nx * overlap * 0.5f;
	a->y -= ny * overlap * 0.5f;
	b->x += nx * overlap * 0.5f;
	b->y += ny * overlap * 0.5f;

	float dvx = b->vx - a->vx;
	float dvy = b->vy - a->vy;
	float dvn = dvx*nx + dvy*ny;
	if (dvn > 0) return;

	float j = -(1.0f + RESTITUTION) * dvn * 0.5f;
	a->vx -= j * nx; a->vy -= j * ny;
	b->vx += j * nx; b->vy += j * ny;
}
void COL__update(float dt){
	Mesa* m = cenario.mesa;
	for(int i = 0; i < cenario.numBolas; i++){
		Bola* b = cenario.bolas[i];
		if (!b->ativo) continue;

		b->x += b->vx * dt;
		b->y += b->vy * dt;
		b->z += b->vz * dt;

		float s2 = b->vx*b->vx + b->vy*b->vy;
		if (s2 > 1e-3){
			float s = sqrtf(s2);
			float ns = s * powf(FRICTION, dt * 60.0f);
			if (ns < 0.5f){ b->vx = b->vy = 0; }
			else {
				b->vx = b->vx / s * ns;
				b->vy = b->vy / s * ns;
			}
		} else {
			b->vx = b->vy = 0;
		}
	}

	// colisões bola-bola
	for(int i = 0; i < cenario.numBolas; i++){
		if (!cenario.bolas[i]->ativo) continue;
		for(int j = i+1; j < cenario.numBolas; j++){
			if (!cenario.bolas[j]->ativo) continue;
			resolverColisaoBolaBola(cenario.bolas[i], cenario.bolas[j]);
		}
	}
	
	for(int i = 0; i < cenario.numBolas; i++){
		Bola* b = cenario.bolas[i];
		if (!b->ativo) continue;
		float L = -m->w/2 + b->raio;
		float R =  m->w/2 - b->raio;
		float B = -m->h/2 + b->raio;
		float T =  m->h/2 - b->raio;
		
		if (b->x < L){
			b->x = L;
			b->vx = -b->vx * RESTITUTION;
		}
		if (b->x > R){
			b->x = R;
			b->vx = -b->vx * RESTITUTION;
		}
		if (b->y < B){
			b->y = B;
			b->vy = -b->vy * RESTITUTION;
		}
		if (b->y > T){
			b->y = T;
			b->vy = -b->vy * RESTITUTION;
		}
		int encacapada = 0;
		int buracoEncacapado = 0;
		for(int p = 0; p < m->quantBuraco; p++){
			float dx = b->x - m->buracos[p].x;
			float dy = b->y - m->buracos[p].y;
			if (dx*dx + dy*dy < m->buracos[p].raio * m->buracos[p].raio){
				b->vz = -150;
				encacapada = 1;
				buracoEncacapado = p;
				break;
			}
		}

		if(encacapada){
			float dx = b->x - m->buracos[buracoEncacapado].x;
			float dy = b->y - m->buracos[buracoEncacapado].y;
			if (!(dx*dx + dy*dy < m->buracos[buracoEncacapado].raio * m->buracos[buracoEncacapado].raio)){
				b->vx *= -1;
				b->vy *= -1;
			}
		}

		if (b->z < -RAIO_BOLA){
			if (i == 0){
				//reespawnar a bola branca
				b->x = -120; b->y = 0; b->z = RAIO_BOLA;
				b->vx = b->vy = 0;
				b->vz = 0;
			} else {
				b->ativo = 0;
				jogo.pontuacao += 10;
				jogo.bolasRestantes--;
			}
		}
	}
}

int todasBolasParadas(){
	for(int i = 0; i < cenario.numBolas; i++){
		Bola* b = cenario.bolas[i];
		if (!b->ativo) continue;
		if (b->vx*b->vx + b->vy*b->vy > VEL_PARADO)
			return 0;
	}
	return 1;
}

void displayMainGame(){
	CAM__limit();
	CAM__update();
	//CAM__follow(cenario.bolas[0]);

	//atualizar posição do mouse nas coordenadas do mundo
	screenToWorld(&mouse);

	Bola* white = cenario.bolas[0];
	float dx = mouse.worldX - white->x;
	float dy = mouse.worldY - white->y;
	float dist = sqrtf(dx*dx + dy*dy);

	if (dist > 0.001f)
		mouse.anguloMira = atan2f(dy, dx);

	//força só é atualizada enquanto segura o botão esquerdo
	if (mouse.lmb){
		float pb = (dist - (white->raio + 5.0f)) * 0.5f;
		if (pb < 0) pb = 0;
		if (pb > MAX_PULLBACK) pb = MAX_PULLBACK;
		mouse.pullback = pb;
	}

	//luzes
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat lp[]  = {150, 250, 500, 1};
	GLfloat lAmb[]= {0.45f, 0.45f, 0.45f, 1};
	GLfloat lDif[]= {0.85f, 0.85f, 0.85f, 1};
	glLightfv(GL_LIGHT0, GL_POSITION, lp);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDif);

	MESA__desenhar(cenario.mesa);
	BURACO__desenhar(cenario.mesa);

	for(int i = 0; i < cenario.numBolas; i++)
		BOLA__desenhar(cenario.bolas[i]);

	// taco só aparece quando tudo está parado
	if (white->ativo && todasBolasParadas()){
		TACO__desenhar(white, mouse.anguloMira, mouse.pullback);
	}

	desenharBackground();

	HUD__desenhar();
}

void displayTitle(){
	
}

void displayGameOver(){
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, windowWidth, 0, windowHeight);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			glColor3f(1, 1, 1);
			char buf[256];
			sprintf(buf, "FIM DE JOGO");
			glTranslatef(windowWidth/4, windowHeight/2, 0);
			glScalef(0.5, 0.7, 1);
			glLineWidth(6);
			for(char* c = buf; *c; c++)
				glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		glPopMatrix();
		glPushMatrix();
			glLoadIdentity();
			glColor3f(1, 1, 1);
			char lit[128];
			sprintf(
				lit, "%d pontos em %d rodadas", 
				jogo.pontuacao, jogo.rodada
			);
			glTranslatef(windowWidth/4, windowHeight/3, 0);
			glScalef(0.2, 0.3, 1);
			glLineWidth(3);
			for(char* c = lit; *c; c++)
				glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_LIGHTING);
}

void displayPause(){
	displayMainGame();
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, windowWidth, 0, windowHeight);
		glMatrixMode(GL_MODELVIEW);
		glColor4f(0, 0, 0, 0.5);
		glBegin(GL_QUADS);
			glVertex2f(-1.0, -1.0);
			glVertex2f(1.0, -1.0);
			glVertex2f(1.0, 1.0);
			glVertex2f(-1.0, 1.0);
		glEnd();
	
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_LIGHTING);
}

void display(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(GameMoment == MAIN_GAME){
		displayMainGame();
	} else if(GameMoment == GAME_OVER){
		displayGameOver();
	} else if(GameMoment == PAUSE){
		displayPause();
	}
	glutSwapBuffers();
}

void reshape(int w, int h){
	windowWidth = w; windowHeight = h;
	if(h == 0) h = 1;
	glViewport(0, 0, w, h);
}

void checkWinning(Jogo* j){
	if(j->bolasRestantes == 0){
		GameMoment = GAME_OVER;
	}
}

void timerCB(int v){
	//updatePhysics(1.0f / 60.0f);
	if(GameMoment == MAIN_GAME){
		COL__update(1.0/60.0);
		checkWinning(&jogo);
	}
	glutPostRedisplay();
	glutTimerFunc(16, timerCB, 0);
}

void mouseMotion(int x, int y){
	if (mouse.mmb){
		int dx = x - mouse.lastMidX;
		int dy = y - mouse.lastMidY;
		camera.tetha -= dx * 0.010f;
		camera.phi   -= dy * 0.010f;
		if (camera.phi < 0.10f) camera.phi = 0.10f;
		if (camera.phi > (float)M_PI-0.10f) camera.phi = (float)M_PI-0.10f;
		mouse.lastMidX = x;
		mouse.lastMidY = y;
	}
	mouse.x = x;
	mouse.y = y;
}

void mouseButton(int button, int state, int x, int y){
	mouse.x = x; mouse.y = y;

	if (button == GLUT_LEFT_BUTTON){
		if (state == GLUT_DOWN){
			mouse.lmb = 1;
		} else {
			if (mouse.lmb){
				Bola* white = cenario.bolas[0];
				if (white->ativo && todasBolasParadas()){
					float pot = (mouse.pullback / MAX_PULLBACK) * MAX_POWER;
					if (pot > 0.4f){
						white->vx = cosf(mouse.anguloMira) * pot;
						white->vy = sinf(mouse.anguloMira) * pot;
					}
					jogo.rodada++;
				}
			}
			mouse.lmb = 0;
			mouse.pullback = 0.0f;
		}
	} else if (button == GLUT_MIDDLE_BUTTON){
		if (state == GLUT_DOWN){
			mouse.mmb = 1;
			mouse.lastMidX = x;
			mouse.lastMidY = y;
		} else {
			mouse.mmb = 0;
		}
	}
}

void keyboard(unsigned char key, int x, int y){
	if (key == 27) {
		if(GameMoment == MAIN_GAME){
			GameMoment = PAUSE;
		} else if(GameMoment == PAUSE){
			GameMoment = MAIN_GAME;
		}
	}
}

int main(int argc, char** argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Jogo de sinuca");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	CAM__create();
	SCENE__create();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mouseMotion);
	glutMouseFunc(mouseButton);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(16, timerCB, 0);

	glutMainLoop();
	return 0;
}