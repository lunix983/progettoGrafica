// classe Vertex: 
// i vertici della mesh
#include <glm/glm.hpp>
class Vertex
{ 
public: 
  Point3 p; // posizione

  // attributi per verice
  Vector3 n; // normale (per vertice)
};

class Edge
{
public: 
  Vertex* v[2]; // due puntatori a Vertice (i due estremi dell'edge)
  // attributi per edge:
};

class Face
{
public: 
  Vertex* v[3]; // tre puntatori a Vertice (i tre vertici del triangolo)
  
  // costruttore
  Face(Vertex* a, Vertex* b, Vertex* c){
    v[0]=a; v[1]=b; v[2]=c;
  }
  
  // attributi per faccia
  Vector3 n; // normale (per faccia)
  
  // computa la normale della faccia
  void ComputeNormal() {
    n= -( (v[1]->p - v[0]->p) % (v[2]->p - v[0]->p) ).Normalize();
  }
  
  // attributi per wedge
  
};

class Mesh
{
  std::vector<Vertex> v; // vettore di vertici
  std::vector<Face> f;   // vettore di facce
  std::vector<Edge> e;   // vettore di edge (per ora, non usato)




                           
public:  


 float positionCone[6];
 int idCono;
 bool colpito;

  // costruttore con caricamento
  Mesh(char *filename){
	LoadFromObj(filename);
    ComputeNormalsPerFace();
    ComputeNormalsPerVertex();
    ComputeBoundingBox();
  }
  
  Mesh(const char * path,std::vector < glm::vec3 > & out_vertices,std::vector < glm::vec2 > & out_uvs,std::vector < glm::vec3 > & out_normals){
	  LoadObjWithTexture(path,out_vertices,out_uvs,out_normals);

  }

  Mesh (){

  }

  // metodi
  void RenderNxF(); // manda a schermo la mesh Normali x Faccia
  void RenderNxV(); // manda a schermo la mesh Normali x Vertice
  void RenderWire(); // manda a schermo la mesh in wireframe
  
  bool LoadFromObj(char* filename); //  carica la mesh da un file OFF
  
  bool LoadObjWithTexture(const char * path,std::vector < glm::vec3 > & out_vertices,std::vector < glm::vec2 > & out_uvs,std::vector < glm::vec3 > & out_normals);

  void ComputeNormalsPerFace();
  void ComputeNormalsPerVertex();
  
  void ComputeBoundingBox();
  void setPosCone(float x, float z);

  bool isColpito();
  void setColpito(bool colpito);

  // centro del axis aligned bounding box
  Point3 Center(){ return (bbmin+bbmax)/2.0; };
  
  Point3 bbmin,bbmax; // bounding box 
};
