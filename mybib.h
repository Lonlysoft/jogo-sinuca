#ifndef MYBIB_H
#define MYBIB_H

#include <GL/glut.h>

// Estrutura para armazenar os dados de cada linha
typedef struct FileData {
    char *name;    // Nome do arquivo
    float x, y, z; // Valores float associados
} FileData;

// Estrutura para armazenar o conjunto de linhas
typedef struct FileList {
    int count;      // Número de entradas
    FileData *data; // Array de entradas
} FileList;

// Estruturas para armazenar dados do OBJ
typedef struct Vertex {
    float x, y, z;
} Vertex;

typedef struct TexCoord {
    float u, v;
} TexCoord;

typedef struct Face {
    int v1, v2, v3; // Índices dos vértices
    int t1, t2, t3; // Índices das coordenadas de textura
    int n1, n2, n3;
    char material[128];
} Face;

typedef struct FaceVertex {
    int v, vt, vn;
} FaceVertex;

typedef struct Texture {
    char name[128];
    int textureID;
} Texture;


// Definição de uma BOX
typedef struct Box {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
} Box;

typedef struct Material {
    char name[128];
    GLuint textureID;
    GLfloat Ka[3];  //Representa a cor ambiente do material (Ambient Color), 
                    //em valores RGB. Esses valores são multiplicados pela luz ambiente.
    GLfloat Kd[3];  //Define a cor difusa do material (Diffuse Color), também em valores RGB. 
                    //Essa é a cor principal que vemos sob iluminação direta.
    GLfloat Ks[3];  //Define a cor especular (Specular Color) do material, em valores RGB. 
                    //Controla a cor dos reflexos especulares.
    GLfloat Ke[3];  //Representa a emissão (Emission), ou seja, a luz que o material parece emitir, em valores RGB.
    GLfloat Ns; // Especifica o "coeficiente de especularidade" (Shininess) do material, 
                //que controla o brilho especular. 
                //Valores mais altos produzem reflexos mais brilhantes e menores áreas reflexivas.
    GLfloat d;  //Define a opacidade do material.
    int illum;  //Modelo de iluminação utilizado: 1= Apenas cores difusas; 2= Difuso e especular; 
                //3= Difuso, especular e reflexão baseada em ambiente.
    //Não utilizado em OpenGL 1.x:            
    //GLfloat Ni; Índice de refração (Optical Density) do material. Controla como a luz é refratada ao atravessar 
                //o material. Valores comuns: 1.0= Ar; 1.33= Água; 1.5= Vidro;

} Material;


typedef struct ObjModel {
    Vertex *vertices;
    TexCoord *texCoords;
    Face *faces;
    Vertex *normals;
    Material *materials;
    Texture *textures;

    int vertexCount;
    int texCoordCount;
    int faceCount;
    int materialCount;
    int normalCount;
    int textureCount;

    Box box;
    //float cor[4];
} ObjModel;


typedef struct {
    int x,y;
} Node;


void crossProduct(Vertex v1, Vertex v2, Vertex *result);
void normalize(Vertex *v);
Vertex barycentricCoord(Vertex v1, Vertex v2, Vertex v3);
void scaleVector(Vertex *v, float k);
void multVector(Vertex *v, Vertex n, float k);
void copyVector(Vertex *v1, Vertex v2);
FileList readFileData(const char *filePath);

void freeFileList(FileList *fileList);
void freeFace(Face *f);
void freeMaterial(Material *m);
void freeTexture(Texture *t);
void freeObjModel(ObjModel *obj);

void geraBox(ObjModel *model);
int intersectBox(Box a, Box b);
int intersectObj(ObjModel a, ObjModel b);
//GLuint loadTexture(const char *filename);
GLuint generateProceduralTexture();

void loadTextures(Texture **textures, int *textureIDCount);
//void loadMTL(const char *filename, Material **materials, int *materialCount);
void loadMTL(const char *filename, Material **materials, int *materialCount, Texture **textures, int *texturesIDCount);
//int loadOBJ(const char *filename, ObjModel *model);
int loadOBJ(const char *fileOBJ, const char *fileMTL, ObjModel *model);
//int loadOBJ(const char *filename, ObjModel *model, Material **materials, int *materialCount, Texture **textures, int *texturesIDCount);
void listObject(ObjModel *obj);
void setMaterial(GLfloat Ka[3], GLfloat Kd[3], GLfloat Ks[3], GLfloat Ke[3],GLfloat Ns, GLfloat d, GLint illum);

void drawNormals(ObjModel *model);
void drawModel(ObjModel *model);
void drawBox(Box b);
void InicializaMatriz(GLfloat *mat);
void MultiplicaMatriz(GLfloat *orig, GLfloat *mat);
void GeraMatrizRotacao(float angle, float x, float y, float z, GLfloat *mat);
void AtualizaRotacao(GLfloat *mat, float angle, float x, float y, float z);


#endif // MYBIB_H
