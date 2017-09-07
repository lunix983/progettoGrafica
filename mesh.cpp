// file mesh.cpp
//
// Implementazione dei metodi di Mesh

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <GL/gl.h>
#include <string>

#include <glm/glm.hpp>

#include "point3.h"
#include "mesh.h"

using namespace std;

extern bool useWireframe; // quick hack: una var globale definita altrove 

void Mesh::ComputeNormalsPerFace()
{
  for (int i=0; i<f.size(); i++)
  {
    f[i].ComputeNormal();
  }
}

// Computo normali per vertice
// (come media rinormalizzata delle normali delle facce adjacenti)
void Mesh::ComputeNormalsPerVertex()
{
  // uso solo le strutture di navigazione FV (da Faccia a Vertice)!
  
  // fase uno: ciclo sui vertici, azzero tutte le normali
  for (int i=0; i<v.size(); i++) {
    v[i].n = Point3(0,0,0);
  }
  
  // fase due: ciclo sulle facce: accumulo le normali di F nei 3 V corrispondenti
  for (int i=0; i<f.size(); i++) {
    f[i].v[0]->n=f[i].v[0]->n + f[i].n;
    f[i].v[1]->n=f[i].v[1]->n + f[i].n;
    f[i].v[2]->n=f[i].v[2]->n + f[i].n;
  }
  
  // fase tre: ciclo sui vertici; rinormalizzo
  // la normale media rinormalizzata e' uguale alla somma delle normnali, rinormalizzata
  for (int i=0; i<v.size(); i++) {
    v[i].n = v[i].n.Normalize();
  }
  
}

// renderizzo la mesh in wireframe
void Mesh::RenderWire(){
  glLineWidth(1.0);
  // (nota: ogni edge viene disegnato due volte. 
  // sarebbe meglio avere ed usare la struttura edge)
  for (int i=0; i<f.size(); i++) 
  {
    glBegin(GL_LINE_LOOP);
      f[i].n.SendAsNormal();    
      (f[i].v[0])->p.SendAsVertex();   
      (f[i].v[1])->p.SendAsVertex();   
      (f[i].v[2])->p.SendAsVertex();
    glEnd();
  }
}




// Render usando la normale per faccia (FLAT SHADING)
void Mesh::RenderNxF()
{
  if (useWireframe) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(.5,.5,.5);
    RenderWire(); 
    glColor3f(1,1,1);
  }
  
  // mandiamo tutti i triangoli a schermo
  glBegin(GL_TRIANGLES);
  for (int i=0; i<f.size(); i++) 
  {
    f[i].n.SendAsNormal(); // flat shading
    
    (f[i].v[0])->p.SendAsVertex();
    
    (f[i].v[1])->p.SendAsVertex();
    
    (f[i].v[2])->p.SendAsVertex();
  }
  glEnd();
}

// Render usando la normale per vertice (GOURAUD SHADING)
void Mesh::RenderNxV()
{
  if (useWireframe) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(.5,.5,.5);
    RenderWire(); 
    glColor3f(1,1,1);
  }
  
  // mandiamo tutti i triangoli a schermo
  glBegin(GL_TRIANGLES);
  for (int i=0; i<f.size(); i++) 
  {    
    (f[i].v[0])->n.SendAsNormal(); // gouroud shading (o phong?)
    (f[i].v[0])->p.SendAsVertex();
    
    (f[i].v[1])->n.SendAsNormal();
    (f[i].v[1])->p.SendAsVertex();
    
    (f[i].v[2])->n.SendAsNormal();
    (f[i].v[2])->p.SendAsVertex();
  }
  glEnd();
}

// trova l'AXIS ALIGNED BOUNDIG BOX
void Mesh::ComputeBoundingBox(){
  // basta trovare la min x, y, e z, e la max x, y, e z di tutti i vertici
  // (nota: non e' necessario usare le facce: perche?) 
  if (!v.size()) return;
  bbmin=bbmax=v[0].p;
  for (int i=0; i<v.size(); i++){
    for (int k=0; k<3; k++) {
      if (bbmin.coord[k]>v[i].p.coord[k]) bbmin.coord[k]=v[i].p.coord[k];
      if (bbmax.coord[k]<v[i].p.coord[k]) bbmax.coord[k]=v[i].p.coord[k];
    }
  }
}

void Mesh::setPosCone(float x, float z){
	positionCone[0] = x;
	positionCone[1] = z;
}

// carica la mesh da un file in formato Obj
//   Nota: nel file, possono essere presenti sia quads che tris
//   ma nella rappresentazione interna (classe Mesh) abbiamo solo tris.
//
bool Mesh::LoadFromObj(char* filename)
{
  
  FILE* file = fopen(filename, "rt"); // apriamo il file in lettura
  if (!file) return false;

  //make a first pass through the file to get a count of the number
  //of vertices, normals, texcoords & triangles 
  char buf[128];
  int nv,nf,nt;
  float x,y,z;
  int va,vb,vc,vd;
  int na,nb,nc,nd;
  int ta,tb,tc,td;

  nv=0; nf=0; nt=0;
  while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               // comment
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               // v, vn, vt
            switch(buf[1]) {
            case '\0':          // vertex
                // eat up rest of line 
                fgets(buf, sizeof(buf), file);
                nv++;
                break;
            default:
                break;
            }
            break;
         case 'f':               // face
                fscanf(file, "%s", buf);
                // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
                if (strstr(buf, "//")) {
                    // v//n
                    sscanf(buf, "%d//%d", &va, &na);
                    fscanf(file, "%d//%d", &vb, &nb);
                    fscanf(file, "%d//%d", &vc, &nc);
                    nf++;
                    nt++;
                    while(fscanf(file, "%d//%d", &vd, &nd) > 0) {
                        nt++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
                    // v/t/n
                    fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
                    fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
                    nf++;
                    nt++;
                    while(fscanf(file, "%d/%d/%d", &vd, &td, &nd) > 0) {
                        nt++;
                    }
                 } else if (sscanf(buf, "%d/%d", &va, &ta) == 2) {
                    // v/t
                    fscanf(file, "%d/%d", &vb, &tb);
                    fscanf(file, "%d/%d", &vc, &tc);
                    nf++;
                    nt++;
                    while(fscanf(file, "%d/%d", &vd, &td) > 0) {
                        nt++;
                    }
                } else {
                    // v
                    fscanf(file, "%d", &va);
                    fscanf(file, "%d", &vb);
                    nf++;
                    nt++;
                    while(fscanf(file, "%d", &vc) > 0) {
                        nt++;
                    }
                }
                break;

            default:
                // eat up rest of line
                fgets(buf, sizeof(buf), file);
                break;
        }
   }
 
//printf("dopo FirstPass nv=%d nf=%d nt=%d\n",nv,nf,nt);
  
  // allochiamo spazio per nv vertici
  v.resize(nv);

  // rewind to beginning of file and read in the data this pass
  rewind(file);
  
  //on the second pass through the file, read all the data into the
  //allocated arrays

  nv = 0;
  nt = 0;
  while(fscanf(file, "%s", buf) != EOF) {
      switch(buf[0]) {
      case '#':               // comment
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
      case 'v':               // v, vn, vt
            switch(buf[1]) {
            case '\0':          // vertex
                fscanf(file, "%f %f %f", &x, &y, &z);
                v[nv].p = Point3( x,y,z );
                nv++;
                break;
            default:
                break;
            }
            break;
     case 'f':               // face
           fscanf(file, "%s", buf);
           // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
           if (strstr(buf, "//")) {
              // v//n
              sscanf(buf, "%d//%d", &va, &na);
              fscanf(file, "%d//%d", &vb, &nb);
              fscanf(file, "%d//%d", &vc, &nc);
              va--; vb--; vc--;
              Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
              f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
              nt++;
              vb=vc;
              while(fscanf(file, "%d//%d", &vc, &nc) > 0) {
                 vc--;
                 Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                 f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                 nt++;
                 vb=vc;
              }
           } else if (sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
                // v/t/n
                fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
                fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
                va--; vb--; vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d/%d/%d", &vc, &tc, &nc) > 0) {
                   vc--;
                   Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                   f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                   nt++;
                   vb=vc;
                }
           } else if (sscanf(buf, "%d/%d", &va, &ta) == 2) {
                // v/t
                fscanf(file, "%d/%d", &va, &ta);
                fscanf(file, "%d/%d", &va, &ta);
                va--; vb--; vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d/%d", &vc, &tc) > 0) {
                   vc--;
                   Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                   f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                   nt++;
                   vb=vc;
                }
           } else {
                // v
                sscanf(buf, "%d", &va);
                fscanf(file, "%d", &vb);
                fscanf(file, "%d", &vc);
                va--; vb--; vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d", &vc) > 0) {
                   vc--;
                   Face newface( &(v[va]), &(v[vc]), &(v[vb]) ); // invoco il costruttore di faccia
                   f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                   nt++;
                   vb=vc;
                }
            }
            break;

            default:
                // eat up rest of line
                fgets(buf, sizeof(buf), file);
                break;
    }
  }

//printf("dopo SecondPass nv=%d nt=%d\n",nv,nt);
  
  fclose(file);
  return true;
}

bool Mesh::LoadObjWithTexture(const char * path,vector < glm::vec3 > & out_vertices,vector < glm::vec2 > & out_uvs,vector < glm::vec3 > & out_normals){
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	FILE * file = fopen(path, "r");
	if( file == NULL ){
	    printf("Impossible to open the file !\n");
	    return false;
	}

	while( 1 ){

	    char lineHeader[128];
	    // read the first word of the line
	    int res = fscanf(file, "%s", lineHeader);
	    if (res == EOF)
	        break; // EOF = End Of File. Quit the loop.

	    if ( strcmp( lineHeader, "v" ) == 0 ){
	        glm::vec3 vertex;
	        fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
	        temp_vertices.push_back(vertex);

	    }else if ( strcmp( lineHeader, "vt" ) == 0 ){
	        glm::vec2 uv;
	        fscanf(file, "%f %f\n", &uv.x, &uv.y );
	        temp_uvs.push_back(uv);
	    }else if ( strcmp( lineHeader, "vn" ) == 0 ){
	        glm::vec3 normal;
	        fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
	        temp_normals.push_back(normal);
	    }else if ( strcmp( lineHeader, "f" ) == 0 ){
	        string vertex1, vertex2, vertex3;
	        unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
	        int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
	        if (matches != 9){
	            printf("File can't be read by our simple parser : ( Try exporting with other options\n");
	            return false;
	        }
	        vertexIndices.push_back(vertexIndex[0]);
	        vertexIndices.push_back(vertexIndex[1]);
	        vertexIndices.push_back(vertexIndex[2]);
	        uvIndices.push_back(uvIndex[0]);
	        uvIndices.push_back(uvIndex[1]);
	        uvIndices.push_back(uvIndex[2]);
	        normalIndices.push_back(normalIndex[0]);
	        normalIndices.push_back(normalIndex[1]);
	        normalIndices.push_back(normalIndex[2]);

	    }

	}

	// For each vertex of each triangle
	    for( unsigned int i=0; i<vertexIndices.size(); i++ ){
	    	unsigned int vertexIndex = vertexIndices[i];
	    	glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
	    	out_vertices.push_back(vertex);
	    }

	    return true;
}

bool Mesh::isColpito(){
	return colpito;
}

void Mesh::setColpito(bool setColpito){
	colpito = setColpito;
}
