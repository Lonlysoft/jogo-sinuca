#include "mybib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <float.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Adicione o arquivo stb_image.h ao mesmo diretório do código.


#define MAX_FILENAME_LENGTH 256 // Tamanho máximo esperado para os nomes dos arquivos


// Função para calcular o produto vetorial
void crossProduct(Vertex v1, Vertex v2, Vertex *result) {
    result->x = v1.y * v2.z - v1.z * v2.y;
    result->y = v1.z * v2.x - v1.x * v2.z;
    result->z = v1.x * v2.y - v1.y * v2.x;
}

// Função para normalizar um vetor
void normalize(Vertex *v) {
    float length = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    if (length != 0.0f) {
        v->x /= length;
        v->y /= length;
        v->z /= length;
    }
}

Vertex barycentricCoord(Vertex v1, Vertex v2, Vertex v3) {
    Vertex v;
    v.x = (v1.x+v2.x+v3.x)/3;
    v.y = (v1.y+v2.y+v3.y)/3;
    v.z = (v1.z+v2.z+v3.z)/3;
    return v;
}

void scaleVector(Vertex *v, float k) {
    v->x *= k;
    v->y *= k;
    v->z *= k;
}

void multVector(Vertex *v, Vertex n, float k) {
    v->x += n.x * k;
    v->y += n.y * k;
    v->z += n.z * k;
}

void copyVector(Vertex *v1, Vertex v2) {
    v1->x = v2.x;
    v1->y = v2.y;
    v1->z = v2.z;
}

Vertex vector3(float x, float y, float z) {
    Vertex v = {x,y,z};
    return v;
}

// Função para ler o arquivo e processar os dados
FileList readFileData(const char *filePath) {
    FileList fileList = {0, NULL}; // Inicializa a estrutura
    FILE *file = fopen(filePath, "r");
    
    if (!file) {
        printf("Erro ao abrir o arquivo: %s\n", filePath);
        return fileList; // Retorna estrutura vazia em caso de erro
    }

    char buffer[MAX_FILENAME_LENGTH + 50]; // Buffer para cada linha
    while (fgets(buffer, sizeof(buffer), file)) {
        // Realoca memória para mais uma entrada
        fileList.data = realloc(fileList.data, (fileList.count + 1) * sizeof(FileData));
        if (!fileList.data) {
            printf("Erro ao alocar memória.\n");
            fclose(file);
            return fileList;
        }

        // Processa a linha lida
        FileData *current = &fileList.data[fileList.count];
        current->name = malloc(MAX_FILENAME_LENGTH * sizeof(char));
        if (!current->name) {
            printf("Erro ao alocar memória para o nome.\n");
            fclose(file);
            return fileList;
        }

        // Faz o parse da linha
        if (sscanf(buffer, "%s %f %f %f", current->name, &current->x, &current->y, &current->z) != 4) {
            printf("Formato inválido na linha: %s\n", buffer);
            free(current->name);
            continue;
        }

        fileList.count++;
    }

    fclose(file);
    return fileList;
}

// Função para liberar a memória da estrutura
void freeFileList(FileList *fileList) {
    for (int i = 0; i < fileList->count; i++) {
        free(fileList->data[i].name); // Libera o nome
    }
    free(fileList->data); // Libera o array de entradas
    fileList->data = NULL;
    fileList->count = 0;
}

void freeFace(Face *f) {
    if (f != NULL) {
        //free(f->material);
        free(f);
        f = NULL;
    }
}

void freeMaterial(Material *m) {
    if (m != NULL) {
        /*free(m->name); 
        free(m->Ka); 
        free(m->Kd); 
        free(m->Ks); 
        free(m->Ke); */
        free(m); 
        m = NULL;         
    }
  
}

void freeTexture(Texture *t) {
    if (t != NULL) {
        free(t->name);
        free(t);
        t = NULL;
    }
}

// Função para liberar a memória da estrutura
void freeObjModel(ObjModel *obj) {
    if (obj == NULL) return;  //verifica se o ponteiro é nulo
    // Libera os vértices
    if (obj->vertices) {
        free(obj->vertices);
        obj->vertices = NULL;
    }

    // Libera as faces
    if (obj->faces) {
        /*for (int i = 0; i < obj->faceCount; i++) {
            free(obj->faces[i].material); // Libera o material associado à face
        }*/
        free(obj->faces); // Libera o array de faces
        obj->faces = NULL;
    }
    // Libera as normais
    if (obj->normals) {
        free(obj->normals);
        obj->normals = NULL;
    }
    // // Libera os materiais
    if (obj->materials) {
        /*for (int i = 0; i < obj->materialCount; i++) {
            freeMaterial(&obj->materials[i]); // Libera cada material individualmente
        }*/
        if (obj->materials) {
            free(obj->materials);
            obj->materials = NULL;
        }
    }
    // Libera as coordenadas de textura
    if (obj->texCoords) {
        free(obj->texCoords);
        obj->texCoords = NULL;
    }

    // Libera as texturas
    if (obj->textures) {
        /*for (int i = 0; i < obj->textureCount; i++) {
            free(obj->textures[i].name); // Libera o nome da textura
        }*/
        free(obj->textures); // Libera o array de texturas
        obj->textures = NULL;
    }
    // O ponteiro `obj` em si não deve ser liberado aqui, pois ele é passado por referência.
}

void geraBox(ObjModel *model) {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    minX = minY = minZ = FLT_MAX;
    maxX = maxY = maxZ = -FLT_MAX;

    for (int i = 0; i < model->vertexCount; i++) {
        if (model->vertices[i].x < minX) minX = model->vertices[i].x;
        if (model->vertices[i].y < minY) minY = model->vertices[i].y;
        if (model->vertices[i].z < minZ) minZ = model->vertices[i].z;
        if (model->vertices[i].x > maxX) maxX = model->vertices[i].x;
        if (model->vertices[i].y > maxY) maxY = model->vertices[i].y;
        if (model->vertices[i].z > maxZ) maxZ = model->vertices[i].z;
    }
    model->box.minX = minX;
    model->box.minY = minY;
    model->box.minZ = minZ;
    model->box.maxX = maxX;
    model->box.maxY = maxY;
    model->box.maxZ = maxZ;
}

int intersectBox(Box a, Box b) {

    return (
        a.minX <= b.maxX && a.maxX >= b.minX && // Sobreposição em X
        a.minY <= b.maxY && a.maxY >= b.minY && // Sobreposição em Y
        a.minZ <= b.maxZ && a.maxZ >= b.minZ    // Sobreposição em Z
    );
}

int intersectObj(ObjModel a, ObjModel b) {
    return intersectBox(a.box, b.box);
}


// Função para gerar uma textura procedural
GLuint generateProceduralTexture() {
    const int size = 64;
    unsigned char data[size][size][3];
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int checker = (i / 8 + j / 8) % 2;
            data[i][j][0] = checker ? 255 : 0; // R
            data[i][j][1] = checker ? 255 : 0; // G
            data[i][j][2] = checker ? 255 : 0; // B
        }
    }

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureId;
}

void setMaterial(
    GLfloat Ka[3], GLfloat Kd[3], GLfloat Ks[3], GLfloat Ke[3], 
    GLfloat Ns, GLfloat d, GLint illum) 
{
    GLfloat ambient[] = { Ka[0], Ka[1], Ka[2], d };
    GLfloat diffuse[] = { Kd[0], Kd[1], Kd[2], d };
    GLfloat specular[] = { Ks[0], Ks[1], Ks[2], 1.0f };
    GLfloat emission[] = { Ke[0], Ke[1], Ke[2], 1.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);

    if (illum >= 2) {
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, Ns);
    } else {
        GLfloat noSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, noSpecular);
    }

    glMaterialfv(GL_FRONT, GL_EMISSION, emission);

    if (d < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    glColor4fv(diffuse);
}

// Função para carregar texturas usando stb_image
void loadTextures(Texture **textures, int *textureIDCount) {

    if (*textureIDCount == 0) return;
    glEnable(GL_TEXTURE_2D); // Habilita uso de texturas
    GLuint textureIds[*textureIDCount]; // Um array para armazenar os identificadores das texturas.
    glGenTextures(*textureIDCount, textureIds); // Gera os identificadores de textura.
    for (int i = 0; i < *textureIDCount; i++) {

        Texture *tex = &(*textures)[i];

        int width, height, channels;
        unsigned char *data = stbi_load(tex->name, &width, &height, &channels, 0);
        if (!data) {
            printf("Erro ao carregar a imagem de textura. \n");
            return;
        }

        glBindTexture(GL_TEXTURE_2D, textureIds[i]); // Vincula o identificador atual.

        if (!glIsTexture(textureIds[i])) {
            printf("Falha ao gerar textura. Verifique o contexto OpenGL.\n");
            stbi_image_free(data);
            return;
        }
    
        // Configura os parâmetros da textura, como filtro e modo de repetição.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Carrega dados para a textura (exemplo com glTexImage2D).   
        glTexImage2D(GL_TEXTURE_2D, 0, (channels == 4) ? GL_RGBA : GL_RGB, width, height, 0, 
                    (channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

}


void loadMTL(const char *filename, Material **materials, int *materialCount, Texture **textures, int *texturesIDCount) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Erro ao abrir arquivo MTL: %s\n", filename);
        return;
    }

    // Garante que os ponteiros foram inicializados corretamente
    if (*materials == NULL) {
        *materials = (Material *) malloc(sizeof(Material));
        if (*materials == NULL) {
            printf("Erro ao alocar memória inicial para materiais.\n");
            fclose(file);
            return;
        }
        *materialCount = 0;  // Inicializa contador
    }

    if (*textures == NULL) {
        *textures = (Texture *) malloc(sizeof(Texture));
        if (*textures == NULL) {
            printf("Erro ao alocar memória inicial para texturas.\n");
            fclose(file);
            return;
        }
        *texturesIDCount = 0;
    }

    char line[256]; 
    Material *currentMaterial = NULL;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "newmtl ", 7) == 0) {
            (*materialCount)++;

            // Realloc seguro
            Material *temp = (Material *) realloc(*materials, (*materialCount) * sizeof(Material));
            if (temp == NULL) {
                printf("Erro ao alocar memória para materiais.\n");
                fclose(file);
                return;
            }
            *materials = temp;

            // Inicializa o novo material
            currentMaterial = &(*materials)[*materialCount - 1];
            memset(currentMaterial, 0, sizeof(Material)); // Garante que não há lixo na estrutura
            sscanf(line, "newmtl %127s", currentMaterial->name); // Garante que não estoura o buffer
            currentMaterial->textureID = 0;
        } 
        else if (currentMaterial != NULL) {  // Certifica que já há um material em processamento
            if (strncmp(line, "Ka", 2) == 0) {
                sscanf(line, "Ka %f %f %f", &currentMaterial->Ka[0], &currentMaterial->Ka[1], &currentMaterial->Ka[2]);
            } else if (strncmp(line, "Kd", 2) == 0) {
                sscanf(line, "Kd %f %f %f", &currentMaterial->Kd[0], &currentMaterial->Kd[1], &currentMaterial->Kd[2]);
            } else if (strncmp(line, "Ks", 2) == 0) {
                sscanf(line, "Ks %f %f %f", &currentMaterial->Ks[0], &currentMaterial->Ks[1], &currentMaterial->Ks[2]);
            } else if (strncmp(line, "Ke", 2) == 0) {
                sscanf(line, "Ke %f %f %f", &currentMaterial->Ke[0], &currentMaterial->Ke[1], &currentMaterial->Ke[2]);
            } else if (strncmp(line, "Ns", 2) == 0) {
                sscanf(line, "Ns %f", &currentMaterial->Ns);
            } else if (strncmp(line, "d", 1) == 0 || strncmp(line, "Tr", 2) == 0) {
                sscanf(line, "%*s %f", &currentMaterial->d);
            } else if (strncmp(line, "illum", 5) == 0) {
                sscanf(line, "illum %d", &currentMaterial->illum);
            } else if (strncmp(line, "map_Kd ", 7) == 0) {
                char textureFile[128];
                sscanf(line, "map_Kd %127s", textureFile);

                (*texturesIDCount)++;
                currentMaterial->textureID = *texturesIDCount;

                // Realloc seguro para as texturas
                Texture *tempTex = (Texture *) realloc(*textures, (*texturesIDCount) * sizeof(Texture));
                if (tempTex == NULL) {
                    printf("Erro ao alocar memória para texturas.\n");
                    fclose(file);
                    return;
                }
                *textures = tempTex;

                // Inicializa a nova textura
                Texture *currentTex = &(*textures)[*texturesIDCount - 1];
                memset(currentTex, 0, sizeof(Texture));
                //strncpy(currentTex->name, textureFile, sizeof(currentTex->name) - 1);
                //currentTex->name[sizeof(currentTex->name) - 1] = '\0';
                snprintf(currentTex->name, sizeof(currentTex->name), "%s", textureFile);
                currentTex->textureID = *texturesIDCount;
            }
        }
    }
    fclose(file);
}

//Função para carregar um modelo / objeto
int loadOBJ(const char *fileOBJ, const char *fileMTL, ObjModel *model) {
    FILE *file = fopen(fileOBJ, "r");
    if (!file) {
        printf("Erro ao abrir arquivo OBJ: %s\n", fileOBJ);
        return 0;
    }

    Material *objMaterials = NULL;
    int objMaterialCount = 0;

    Texture *objTextures = NULL;
    int objTexturesCount = 0;

    model->materials = NULL;
    model->materialCount = 0;

    model->textures = NULL;
    model->textureCount = 0;

    // Carregar materiais e texturas do objeto
    loadMTL(fileMTL, &objMaterials, &objMaterialCount, &objTextures, &objTexturesCount);
    loadTextures(&objTextures, &objTexturesCount);

    model->materialCount = objMaterialCount;

    // Verificar se model->materialCount é válido
    if (model->materialCount <= 0) {
        printf("Erro: model->materialCount inválido (%d)\n", model->materialCount);
        fclose(file);
        return 0;
    }

    // Verificar se model->materials foi corrompido
    if (model->materials != NULL && (uintptr_t)model->materials < 0x1000) {
        printf("Erro: Ponteiro model->materials parece inválido!\n");
        fclose(file);
        return 0;
    }

    // Realloc seguro
    Material *tempM = (Material *) realloc(model->materials, model->materialCount * sizeof(Material));
    if (tempM == NULL) {
        printf("Erro ao alocar memória temporária para materiais.\n");
        fclose(file);
        return 0;
    }
    model->materials = tempM;

    if (!model->materials) {
        printf ("Não Alocou Material!\n");
        return 0;
    }
    for (int i=0; i<objMaterialCount; i++) {
        if (model->materials == NULL) {
            printf("Erro: model->materials é NULL!\n");
            exit(1);
        }
        /*if (strcmp(objMaterials[i].name,"") {
            printf("Erro: objMaterials[%d].name é NULL!\n", i);
            exit(1);
        }*/
        strcpy(model->materials[i].name,objMaterials[i].name);
        model->materials[i].textureID = objMaterials[i].textureID;
        model->materials[i].Ka[0] = objMaterials[i].Ka[0];
        model->materials[i].Ka[1] = objMaterials[i].Ka[1];
        model->materials[i].Ka[2] = objMaterials[i].Ka[2];
        model->materials[i].Kd[0] = objMaterials[i].Kd[0];
        model->materials[i].Kd[1] = objMaterials[i].Kd[1];
        model->materials[i].Kd[2] = objMaterials[i].Kd[2];
        model->materials[i].Ks[0] = objMaterials[i].Ks[0];
        model->materials[i].Ks[1] = objMaterials[i].Ks[1];
        model->materials[i].Ks[2] = objMaterials[i].Ks[2];
        model->materials[i].Ke[0] = objMaterials[i].Ke[0];
        model->materials[i].Ke[1] = objMaterials[i].Ke[1];
        model->materials[i].Ke[2] = objMaterials[i].Ke[2];
        model->materials[i].Ns = objMaterials[i].Ns;
        model->materials[i].d = objMaterials[i].d;
        model->materials[i].illum = objMaterials[i].illum;
    }
    model->textureCount = objTexturesCount;

    // Verificar se model->textureCount é válido
    if (model->textureCount < 0) {
        printf("Erro: model->textureCount inválido (%d)\n", model->textureCount);
        fclose(file);
        return 0;
    }

    // Verificar se model->textures foi corrompido
    if (model->textures != NULL && (uintptr_t)model->textures < 0x1000) {
        printf("Erro: Ponteiro model->materials parece inválido!\n");
        fclose(file);
        return 0;
    }

    if (model->textureCount > 0) {
        // Realloc seguro para as texturas
        Texture *tempTex = (Texture *) realloc(model->textures, model->textureCount * sizeof(Texture));
        if (tempTex == NULL) {
            printf("Erro ao alocar memória temporária para texturas.\n");
            fclose(file);
            return 0;
        }
        model->textures = tempTex;

        for (int i=0; i<objTexturesCount; i++) {
            strcpy(model->textures[i].name,objTextures[i].name);
            model->textures[i].textureID = objTextures[i].textureID;
        }
    }

    
    char line[128], currentMaterial[64] = "";
    model->vertices = NULL;
    model->texCoords = NULL;
    model->faces = NULL;
    model->normals = NULL;
    model->vertexCount = 0;
    model->texCoordCount = 0;
    model->faceCount = 0;
    model->normalCount = 0;
 
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "v ", 2) == 0) {

            //carregar vértices
            model->vertexCount++;
            model->vertices = (Vertex *)realloc(model->vertices, model->vertexCount * sizeof(Vertex));
            if (!model->vertices) {
                printf("Erro ao alocar memória para vértices.\n");
                fclose(file);
                return 0;
            }
            sscanf(line, "v %f %f %f", &model->vertices[model->vertexCount - 1].x,
                                       &model->vertices[model->vertexCount - 1].y,
                                       &model->vertices[model->vertexCount - 1].z);
        } else if (strncmp(line, "vt ", 3) == 0) {
            // Carregar coordenadas de textura
            model->texCoordCount++;
            model->texCoords = (TexCoord *)realloc(model->texCoords, model->texCoordCount * sizeof(TexCoord));
            if (!model->texCoords) {
                printf("Erro ao alocar memória para coordenadas de textura.\n");
                fclose(file);
                return 0;
            }
            sscanf(line, "vt %f %f", &model->texCoords[model->texCoordCount - 1].u,
                                    &model->texCoords[model->texCoordCount - 1].v);
        } else if (strncmp(line, "vn ", 3) == 0) {
            // Carregar normais
            model->normalCount++;
            model->normals = (Vertex *)realloc(model->normals, model->normalCount * sizeof(Vertex));
            if (!model->normals) {
                printf("Erro ao alocar memória para normais.\n");
                fclose(file);
                return 0;
            }
            sscanf(line, "vn %f %f %f", &model->normals[model->normalCount - 1].x,
                                        &model->normals[model->normalCount - 1].y,
                                        &model->normals[model->normalCount - 1].z);
        } else if (strncmp(line, "usemtl ", 7) == 0) {
            // Atualizar material corrente
            sscanf(line, "usemtl %s", currentMaterial);                                    
        } else if (strncmp(line, "f ", 2) == 0) {
            //Carregar faces
            model->faceCount++;
            model->faces = (Face *)realloc(model->faces, model->faceCount * sizeof(Face));
            if (!model->faces) {
                printf("Erro ao alocar memória para faces.\n");
                fclose(file);
                return 0;
            }
            Face *face = &model->faces[model->faceCount - 1];

            FaceVertex fv[3];

            int matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
                                         &fv[0].v, &fv[0].vt, &fv[0].vn,
                                         &fv[1].v, &fv[1].vt, &fv[1].vn,
                                         &fv[2].v, &fv[2].vt, &fv[2].vn);
            if (matches == 9) {   //formato v/vt/vn
                face->v1=fv[0].v; face->t1=fv[0].vt; face->n1=fv[0].vn;
                face->v2=fv[1].v; face->t2=fv[1].vt; face->n2=fv[1].vn;
                face->v3=fv[2].v; face->t3=fv[2].vt; face->n3=fv[2].vn;

            } else if ((matches = sscanf(line, "f %d/%d %d/%d %d/%d",     // Formato: v/vt   - sem normais
                                         &fv[0].v, &fv[0].vt,
                                         &fv[1].v, &fv[1].vt,
                                         &fv[2].v, &fv[2].vt)) == 6) {
                face->v1=fv[0].v; face->t1=fv[0].vt; face->n1=0;
                face->v2=fv[1].v; face->t2=fv[1].vt; face->n2=0;
                face->v3=fv[2].v; face->t3=fv[2].vt; face->n3=0;

            } else if ((matches = sscanf(line, "f %d %d %d",     // Formato: v   - sem texturas e sem normais
                                         &fv[0].v, 
                                         &fv[1].v,
                                         &fv[2].v)) == 3) {
                face->v1=fv[0].v; face->t1=0; face->n1=0;
                face->v2=fv[1].v; face->t2=0; face->n2=0;
                face->v3=fv[2].v; face->t3=0; face->n3=0;

            }
            strcpy(face->material,currentMaterial);
        }
    }

    geraBox(model);

    fclose(file);
    if (objMaterials){
        free(objMaterials);
    }
    if (objTextures) {
        free(objTextures);
    }
    return 1;
}

void listObject(ObjModel *obj) {
    printf("Material Count: %d\n",obj->materialCount);
    printf("Vertex Count: %d\n",obj->vertexCount);
    printf("TexCoord Count: %d\n",obj->texCoordCount);
    printf("Face Count: %d\n",obj->faceCount);
    printf("Normal Count: %d\n",obj->normalCount);
    printf("Texture Count: %d\n",obj->textureCount);

    for (int i=0; i<obj->materialCount;i++) {
        printf("Nome[%d]: %s\n",i,obj->materials[i].name);
        printf("Ka[%d]: %f,%f,%f\n",i,obj->materials[i].Ka[0],obj->materials[i].Ka[1],obj->materials[i].Ka[2]);
        printf("textureID[%d]: %d\n",i,obj->materials[i].textureID);
    }
    for (int i=0; i<obj->vertexCount;i++) {
        printf("Vertex[%d]: %f,%f,%f\n",i,obj->vertices->x,obj->vertices->y,obj->vertices->z);
    }
}



void drawNormals(ObjModel *model) {   // Vertex *normal, *v1, *v2, *v3) {

    Vertex base, v;

    if (!model->vertices || !model->faces) {
        printf("Dados do modelo incompletos. Não é possível calcular normais.\n");
        return;
    }

    glColor3f(0.0,1.0,1.0);

    glBegin(GL_LINES);

    for (int i = 0; i < model->faceCount; i++) {
        Face face = model->faces[i];

        if (face.v1 > model->vertexCount || face.v2 > model->vertexCount || face.v3 > model->vertexCount) {
            printf("Índice de face fora do intervalo.\n");
            exit(EXIT_FAILURE);
        }

        base = barycentricCoord(model->vertices[face.v1 - 1], model->vertices[face.v2 - 1], model->vertices[face.v3 - 1]);
        copyVector(&v,base);
        

        if (face.n1 > 0) {  //tem normais.  Obs.: n1 == n2 == n3 -> (vide formato .OBJ)
            multVector(&v,
                       vector3(model->normals[face.n1 - 1].x,model->normals[face.n1 - 1].y,model->normals[face.n1 - 1].z),
                       0.05);
            //glNormal3f(model->normals[face.v1 - 1].x, model->normals[face.v1 - 1].y, model->normals[face.v1 - 1].z);
        } else {  //calcula as normais, considerando os vetores da face:
            // Calcular os vetores da face
            Vertex edge1 = {model->vertices[face.v2 - 1].x  -  model->vertices[face.v1 - 1].x, 
                            model->vertices[face.v2 - 1].y  -  model->vertices[face.v1 - 1].y,
                            model->vertices[face.v2 - 1].z  -  model->vertices[face.v1 - 1].z};
            Vertex edge2 = {model->vertices[face.v3 - 1].x  -  model->vertices[face.v1 - 1].x, 
                            model->vertices[face.v3 - 1].y  -  model->vertices[face.v1 - 1].y,
                            model->vertices[face.v3 - 1].z  -  model->vertices[face.v1 - 1].z};

            // Calcular a normal
            Vertex normal;
            crossProduct(edge2, edge1, &normal);

            // Normalizar a normal
            normalize(&normal);

            multVector(&v,normal,0.05);
            //glNormal3f(normal.x, normal.y, normal.z);
        }


        glVertex3f(base.x, base.y, base.z);
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();  //GL_LINES

}

// Função para desenhar o modelo
void drawModel(ObjModel *model) {

    if (!model->vertices || !model->texCoords || !model->faces) {
        printf("Dados do modelo estão incompletos.\n");
        return;
    }

    glBegin(GL_TRIANGLES);

    for (int i = 0; i < model->faceCount; i++) {
        Face face = model->faces[i];

        if (face.v1 > model->vertexCount || face.t1 > model->texCoordCount) {
            printf("Índice de face fora do intervalo.\n");
            exit(EXIT_FAILURE);
        }

        // Ativar textura, se existir, e configurar o material
        for (int m = 0; m < model->materialCount; m++) {
            if (strcmp(face.material,model->materials[m].name)) {
                if (model->textures) {
                    //glBindTexture(GL_TEXTURE_2D, proceduralTexture); // Substituir conforme necessário
                    glBindTexture(GL_TEXTURE_2D, model->materials[m].textureID);
                } 
                setMaterial(model->materials[m].Ka,
                                model->materials[m].Kd,
                                model->materials[m].Ks,
                                model->materials[m].Ke,
                                model->materials[m].Ns,
                                model->materials[m].d,
                                model->materials[m].illum);
                break;
            }
        }

        if (face.n1 > 0) {  //tem normais
            glNormal3f(model->normals[face.n1 - 1].x, model->normals[face.n2 - 1].y, model->normals[face.n3 - 1].z);
        } else {  //calcula as normais, considerando os vetores da face:
            // Calcular os vetores da face
            Vertex edge1 = {model->vertices[face.v2 - 1].x  -  model->vertices[face.v1 - 1].x, 
                            model->vertices[face.v2 - 1].y  -  model->vertices[face.v1 - 1].y,
                            model->vertices[face.v2 - 1].z  -  model->vertices[face.v1 - 1].z};
            Vertex edge2 = {model->vertices[face.v3 - 1].x  -  model->vertices[face.v1 - 1].x, 
                            model->vertices[face.v3 - 1].y  -  model->vertices[face.v1 - 1].y,
                            model->vertices[face.v3 - 1].z  -  model->vertices[face.v1 - 1].z};

            // Calcular a normal
            Vertex normal;
            crossProduct(edge1, edge2, &normal);

            // Normalizar a normal
            normalize(&normal);

            glNormal3f(normal.x, normal.y, normal.z);
        }


        if (model->textures) glTexCoord2f(model->texCoords[face.t1 - 1].u, model->texCoords[face.t1 - 1].v);
        glVertex3f(model->vertices[face.v1 - 1].x, model->vertices[face.v1 - 1].y, model->vertices[face.v1 - 1].z);
        if (model->textures) glTexCoord2f(model->texCoords[face.t2 - 1].u, model->texCoords[face.t2 - 1].v);
        glVertex3f(model->vertices[face.v2 - 1].x, model->vertices[face.v2 - 1].y, model->vertices[face.v2 - 1].z);
        if (model->textures) glTexCoord2f(model->texCoords[face.t3 - 1].u, model->texCoords[face.t3 - 1].v);
        glVertex3f(model->vertices[face.v3 - 1].x, model->vertices[face.v3 - 1].y, model->vertices[face.v3 - 1].z);
    }
    glEnd();  //GL_TRIANGLES

}

void drawBox(Box b) {
    glColor3f(1.0f, 0.0f, 0.2f);  //rosa
    glPushMatrix();
        glBegin(GL_LINE_LOOP);
            glVertex3f(b.minX,b.minY,b.minZ);
            glVertex3f(b.maxX,b.minY,b.minZ);
            glVertex3f(b.maxX,b.maxY,b.minZ);
            glVertex3f(b.minX,b.maxY,b.minZ);
        glEnd();
        glBegin(GL_LINE_LOOP);
            glVertex3f(b.minX,b.minY,b.maxZ);
            glVertex3f(b.maxX,b.minY,b.maxZ);
            glVertex3f(b.maxX,b.maxY,b.maxZ);
            glVertex3f(b.minX,b.maxY,b.maxZ);
        glEnd();
        glBegin(GL_LINES);
            glVertex3f(b.minX,b.minY,b.minZ);
            glVertex3f(b.minX,b.minY,b.maxZ);
            glVertex3f(b.minX,b.maxY,b.minZ);
            glVertex3f(b.minX,b.maxY,b.maxZ);
            glVertex3f(b.maxX,b.minY,b.minZ);
            glVertex3f(b.maxX,b.minY,b.maxZ);
            glVertex3f(b.maxX,b.maxY,b.minZ);
            glVertex3f(b.maxX,b.maxY,b.maxZ);
        glEnd();
    glPopMatrix();
}


// Inicializa a matriz de rotação como identidade
void InicializaMatriz(GLfloat *mat) {
    for (int i = 0; i < 16; i++)
        mat[i] = (i % 5 == 0) ? 1.0f : 0.0f; // Diagonal principal = 1, restante = 0
}

// Função para multiplicar a matriz acumulada por uma nova rotação
void MultiplicaMatriz(GLfloat *orig, GLfloat *mat) {
    GLfloat result[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += orig[i * 4 + k] * mat[k * 4 + j];
            }
        }
    }
    // Copia o resultado de volta para a matriz de rotação
    for (int i = 0; i < 16; i++)
        orig[i] = result[i];
}

// Gera uma matriz de rotação em torno de um eixo arbitrário
void GeraMatrizRotacao(float angle, float x, float y, float z, GLfloat *mat) {
    float rad = angle * M_PI / 180.0f;
    float c = cos(rad);
    float s = sin(rad);
    float t = 1 - c;

    mat[0] = t * x * x + c;    mat[4] = t * x * y - s * z; mat[8] = t * x * z + s * y; mat[12] = 0;
    mat[1] = t * x * y + s * z; mat[5] = t * y * y + c;     mat[9] = t * y * z - s * x; mat[13] = 0;
    mat[2] = t * x * z - s * y; mat[6] = t * y * z + s * x; mat[10] = t * z * z + c;    mat[14] = 0;
    mat[3] = 0;                 mat[7] = 0;                 mat[11] = 0;                mat[15] = 1;
}

void AtualizaRotacao(GLfloat *mat, float angle, float x, float y, float z) {
    GLfloat m[16];
    GeraMatrizRotacao(angle, x, y, z, m);
    MultiplicaMatriz(mat, m);
}




