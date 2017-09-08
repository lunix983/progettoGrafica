// car.cpp
// implementazione dei metodi definiti in car.h

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <vector> // la classe vector di STL 

#include "car.h"
#include "point3.h"
#include "mesh.h"
#include <glm/glm.hpp>

#include <iostream>




// var globale di tipo mesh

Mesh carlinga((char *)"A8_custom.obj");
Mesh wheelBR1((char *)"wheel_metal_back_A8.obj");
Mesh wheelFR1((char *)"wheel_metal_front_A8.obj");
Mesh pista((char *)"pista_media_new.obj");
Mesh vetri((char *)"glass_A8.obj");
Mesh tabellone((char *)"tabellone.obj");
Mesh boardFoto((char *)"boardFoto.obj");
Mesh via((char *)"via.obj");
Mesh cone1((char *)"cone_scale.obj");
Mesh cone2((char *)"cone_scale.obj");
Mesh cone3((char *)"cone_scale.obj");
Mesh cone4((char *)"cone_scale.obj");
Mesh cone5((char *)"cone_scale.obj");
Mesh cone6((char *)"cone_scale.obj");

float posCone1[6];
float posCone2[6];
float posCone3[6];
float posCone4[6];
float posCone5[6];

bool colpitoCone1 = false;
bool colpitoCone2 = false;
bool colpitoCone3 = false;
bool colpitoCone4 = false;
bool colpitoCone5 = false;


using namespace std;


extern bool useEnvmap; // var globale esterna: per usare l'evnrionment mapping
extern bool useHeadlight; // var globale esterna: per usare i fari
extern bool useShadow; // var globale esterna: per generare l'ombra

// da invocare quando e' stato premuto/rilasciato il tasto numero "keycode"
void Controller::EatKey(int keycode, int* keymap, bool pressed_or_released)
{
  for (int i=0; i<NKEYS; i++){
    if (keycode==keymap[i]) key[i]=pressed_or_released;
  }
}

// da invocare quando e' stato premuto/rilasciato un jbutton
void Controller::Joy(int keymap, bool pressed_or_released)
{
    key[keymap]=pressed_or_released;
}

// Funzione che prepara tutto per usare un env map
void SetupEnvmapTexture(int carColor)
{
	// facciamo binding con la texture 1
	glBindTexture(GL_TEXTURE_2D,5);
	switch (carColor) {
		case 1:
			glBindTexture(GL_TEXTURE_2D,5);
			break;
		case 2:
			glBindTexture(GL_TEXTURE_2D,6);
			break;
		case 3:
			glBindTexture(GL_TEXTURE_2D,7);
			break;
	}
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
	glEnable(GL_TEXTURE_GEN_T);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
	glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
	glDisable(GL_LIGHTING); // disabilito il lighting OpenGL standard (lo faccio con la texture)
}

// funzione che prepara tutto per creare le coordinate texture (s,t) da (x,y,z)
// Mappo l'intervallo [ minY , maxY ] nell'intervallo delle T [0..1]
//     e l'intervallo [ minZ , maxZ ] nell'intervallo delle S [0..1]
void SetupWheelTexture(Point3 min, Point3 max){
  glBindTexture(GL_TEXTURE_2D,0);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  // ulilizzo le coordinate OGGETTO
  // cioe' le coordnate originali, PRIMA della moltiplicazione per la ModelView
  // in modo che la texture sia "attaccata" all'oggetto, e non "proiettata" su esso
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  float sz=1.0/(max.Z() - min.Z());
  float ty=1.0/(max.Y() - min.Y());
  float s[4]={0,0,sz,  - min.Z()*sz };
  float t[4]={0,ty,0,  - min.Y()*ty };
  glTexGenfv(GL_S, GL_OBJECT_PLANE, s); 
  glTexGenfv(GL_T, GL_OBJECT_PLANE, t); 
}


void SetupConeTexture(){
	// facciamo binding con la texture 1
		glBindTexture(GL_TEXTURE_2D,9);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
		glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
		glDisable(GL_LIGHTING);
}

void SetupTrackTexture(Point3 min, Point3 max){
	glBindTexture(GL_TEXTURE_2D,10);
	  glEnable(GL_TEXTURE_2D);
	  glEnable(GL_TEXTURE_GEN_S);
	  glEnable(GL_TEXTURE_GEN_T);
	  // ulilizzo le coordinate OGGETTO
	  // cioe' le coordnate originali, PRIMA della moltiplicazione per la ModelView
	  // in modo che la texture sia "attaccata" all'oggetto, e non "proiettata" su esso
	  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
	  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
	  float sz=1.0/(max.Z() - min.Z());
	  float ty=1.0/(max.Y() - min.Y());
	  float s[4]={0,0,sz,  - min.Z()*sz };
	  float t[4]={0,ty,0,  - min.Y()*ty };
	  glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
	  glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
}


void checkColpito(Mesh *cone,float carMinX, float carMinZ,float carMaxX, float carMaxZ){
	//calcolo le coordinate X e Z dell'oggetto cono
	float coneCentX = cone->Center().X() + cone->positionCone[0]; // X
	float coneCentZ = cone->Center().Z() + cone->positionCone[1]; // Z

	if ((coneCentX > carMinX) && (coneCentX < carMaxX) && (coneCentZ > carMinZ) && (coneCentZ < carMaxZ)) {

	  switch (cone->idCono) {
		case 1:
			colpitoCone1 = true;
			break;
		case 2:
			colpitoCone2 = true;
			break;
		case 3:
			colpitoCone3 = true;
			break;
		case 4:
			colpitoCone4 = true;
			break;
		case 5:
			colpitoCone5 = true;
			break;
		default:
			break;
	}

	}
}

bool checkWin(){
	if(colpitoCone1 && colpitoCone2 && colpitoCone3 && colpitoCone4 && colpitoCone5){
		return true;
	}else{
		return false;
	}
}
/*Questo metodo conta quanti oggetti cono NON sono stati colpiti*/
int Car::contaColpito(){
	//int soglia = 5;
	int count = 0;
	if(!colpitoCone1){
		count++;
	}
	if(!colpitoCone2){
			count++;
		}
	if(!colpitoCone3){
			count++;
		}
	if(!colpitoCone4){
			count++;
		}
	if(!colpitoCone5){
			count++;
		}

	return count;
}

void Car::rallenta(){
controller.key[Controller::LEFT]=0;
controller.key[Controller::RIGHT]=0;
controller.key[Controller::ACC]=0;
controller.key[Controller::DEC]=0;
}

// DoStep: facciamo un passo di fisica (a delta_t costante)
//
// Indipendente dal rendering.
//
// ricordiamoci che possiamo LEGGERE ma mai SCRIVERE
// la struttura controller da DoStep
void Car::DoStep(){
  // computiamo l'evolversi della macchina
  static int i=5;
  
  float vxm, vym, vzm; // velocita' in spazio macchina
  
  // da vel frame mondo a vel frame macchina
  float cosf = cos(facing*M_PI/180.0);
  float sinf = sin(facing*M_PI/180.0);
  vxm = +cosf*vx - sinf*vz;
  vym = vy;
  vzm = +sinf*vx + cosf*vz;
  
  // gestione dello sterzo
  if (controller.key[Controller::LEFT]) sterzo+=velSterzo;
  if (controller.key[Controller::RIGHT]) sterzo-=velSterzo;
  sterzo*=velRitornoSterzo; // ritorno a volante dritto
  
  if (controller.key[Controller::ACC]) vzm-=accMax; // accelerazione in avanti 
  if (controller.key[Controller::DEC]) vzm+=accMax; // accelerazione indietro
  
  // attirti (semplificando)
  vxm*=attritoX;  
  vym*=attritoY;
  vzm*=attritoZ;
  
  // l'orientamento della macchina segue quello dello sterzo
  // (a seconda della velocita' sulla z)
  facing = facing - (vzm*grip)*sterzo;
  
  // rotazione mozzo ruote (a seconda della velocita' sulla z)
  float da ; //delta angolo
  da=(360.0*vzm)/(2.0*M_PI*raggioRuotaA);
  mozzoA+=da;
  da=(360.0*vzm)/(2.0*M_PI*raggioRuotaP);
  mozzoP+=da;
  
  // ritorno a vel coord mondo
  vx = +cosf*vxm + sinf*vzm;
  vy = vym;
  vz = -sinf*vxm + cosf*vzm;
  
  // posizione = posizione + velocita * delta t (ma delta t e' costante)
  px+=vx;
  py+=vy;
  pz+=vz;
  // calcolo le coordinate della vettura attraverso i // bounding box
  float carMinX = (carlinga.bbmin.X() / 3) +px;
  float carMinZ = (carlinga.bbmin.Z() / 3) +pz;
  float carMaxX = (carlinga.bbmax.X() / 3) +px;
  float carMaxZ = (carlinga.bbmax.Z() / 3) +pz;

  checkColpito(&cone1,carMinX,carMinZ,carMaxX,carMaxZ);
  checkColpito(&cone2,carMinX,carMinZ,carMaxX,carMaxZ);
  checkColpito(&cone3,carMinX,carMinZ,carMaxX,carMaxZ);
  checkColpito(&cone4,carMinX,carMinZ,carMaxX,carMaxZ);
  checkColpito(&cone5,carMinX,carMinZ,carMaxX,carMaxZ);

} 

//void drawCube(); // questa e' definita altrove (quick hack)
void drawAxis(); // anche questa

void drawCone1() {
        glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(22.75,0.05,-2.0);
        glScalef(0.12,0.15,0.14);
        cone1.setPosCone(22.75,-2.0);
        cone1.idCono = 1;
       // SetupConeTexture();
        cone1.RenderNxV();
        glPopMatrix();
        SetupConeTexture();
}

void drawCone2() {
        glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(100,0.05,-1.0);
        glScalef(0.12,0.15,0.14);
        cone2.setPosCone(100,-1.0);
        cone2.idCono = 2;
        cone2.RenderNxV();
        glPopMatrix();
        SetupConeTexture();
}

void drawCone3() {
        glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(100,0.05,65);
        glScalef(0.12,0.15,0.14);
        cone3.setPosCone(100,65);
        cone3.idCono = 3;
        cone3.RenderNxV();
        glPopMatrix();
        SetupConeTexture();
}

void drawCone4() {
        glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(-3.0,0.05,20.0);
        glScalef(0.12,0.15,0.14);
        cone4.setPosCone(-3.0,20.0);
        cone4.idCono = 4;
        cone4.RenderNxV();
        glPopMatrix();
        SetupConeTexture();
}


void drawCone5() {
        glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(-40.0,0.05,2.0);
        glScalef(0.12,0.15,0.14);
        cone5.setPosCone(-40,2.0);
        cone5.idCono = 5;
        cone5.RenderNxV();
        glPopMatrix();
        SetupConeTexture();
}

void drawCone6() {
        glPushMatrix();
        glColor3f(1, 1, 1);
//        glTranslatef(22.75,0.05,-2.0);
//        glScalef(0.12,0.15,0.14);
        //cone6.setPosCone(22.75,-2.0);
        cone6.idCono = 6;
        //glRotatef(90,0,1,0);
        //SetupTreeTexture(tree.bbmin,tree.bbmax);
        //tree.ComputeBoundingBox();
        SetupConeTexture();
        cone6.RenderNxV();
        glPopMatrix();
}

void drawPista () {
        glPushMatrix();
        glColor3f(0.3,0.3,0.3);
        glScalef(0.75, 1.0, 0.75);
        glTranslatef(0,0.01,0);
        pista.RenderNxF();
        glPopMatrix();
}

void SetupParabrezzaTexture(Point3 min, Point3 max){

	// facciamo binding con la texture 1
		glBindTexture(GL_TEXTURE_2D,11);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
		  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
		  float tz=1.0/(max.Z() - min.Z());
		   float sx=1.0/(max.X() - min.X());
		  float t[4]={0,0,tz,   -min.Z()*tz };
		  float s[4]={-sx,0,0, min.X()*sx };
		  glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
		  glTexGenfv(GL_T, GL_OBJECT_PLANE, t);


}

void drawParabrezza () {
        glPushMatrix();
        // abilita funzione blend alpha
        glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,1,1,0.5); // usa color4f per avere l'ultimo parametro che imposta l'opacita'
        vetri.RenderNxF();
        glPopMatrix();
}

void SetupBoard(Point3 min, Point3 max){
  glBindTexture(GL_TEXTURE_2D,8);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  float tz=1.0/(max.Z() - min.Z());
  float sx=1.0/(max.X() - min.X());
  float t[4]={0,0,tz,   -min.Z()*tz };
  float s[4]={-sx,0,0, min.X()*sx };
  glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
  glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
}

void drawTabellone () {
        glPushMatrix();
        glColor3f(0.5,0.5,0.5);
        glScalef(0.75, 0.75, 0.75);
        tabellone.RenderNxF();
        glPopMatrix();
}

void drawBoard () {
        glPushMatrix();
        glColor3f(0.5,0.5,0.5);
        glTranslatef(0.1,-0.2,0.0);
        glScalef(0.35, 0.39, 0.38);
        glRotatef(270,0.0,1.0,0.0);
        SetupBoard(boardFoto.bbmin,boardFoto.bbmax);
        boardFoto.RenderNxF();
        glPopMatrix();
}

void SetupViaTexture(Point3 min, Point3 max){

  glBindTexture(GL_TEXTURE_2D,3);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
  float tz=1.0/(max.Z() - min.Z());
  float sx=1.0/(max.X() - min.X());
  float t[4]={0,0,tz,   -min.Z()*tz };
  float s[4]={-sx,0,0, min.X()*sx };
  glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
  glTexGenfv(GL_T, GL_OBJECT_PLANE, t);


}

void drawVia() {
        glPushMatrix();
        glColor3f(1, 1, 1);

        glTranslatef(2.75,0.05,0.0);
        glScalef(0.12,0.15,0.14);

        glRotatef(90,0,1,0);

        SetupViaTexture(via.bbmin,via.bbmax);
        via.RenderNxV();

        glPopMatrix();
}

void Controller::Init(){
  for (int i=0; i<NKEYS; i++) key[i]=false;
}

void Car::Init(){
  // inizializzo lo stato della macchina
  //px=pz=facing=0; // posizione e orientamento
  px=0;
  pz=0;
  facing=-90;
  py=0.0;
  
  mozzoA=mozzoP=sterzo=0;   // stato
  vx=vy=vz=0;      // velocita' attuale
  // inizializzo la struttura di controllo
  controller.Init();
  
  //velSterzo=3.4;         // A
  velSterzo=2.4;         // A
  velRitornoSterzo=0.93; // B, sterzo massimo = A*B / (1-B)
  
  accMax = 0.0011;
  
  // attriti: percentuale di velocita' che viene mantenuta
  // 1 = no attrito
  // <<1 = attrito grande
  attritoZ = 0.991;  // piccolo attrito sulla Z (nel senso di rotolamento delle ruote)
  attritoX = 0.8;  // grande attrito sulla X (per non fare slittare la macchina)
  attritoY = 1.0;  // attrito sulla y nullo
  
  // Nota: vel max = accMax*attritoZ / (1-attritoZ)
  
  raggioRuotaA = 0.25;
  raggioRuotaP = 0.35;
  
  grip = 0.45; // quanto il facing macchina si adegua velocemente allo sterzo
}

// attiva una luce di openGL per simulare un faro della macchina
void Car::DrawHeadlight(float x, float y, float z, int lightN, bool useHeadlight) const{
  int usedLight=GL_LIGHT1 + lightN;

  if(useHeadlight)
  {
  glEnable(usedLight);
  

  float col0[4]= {0.8,0.8,0.0,  1};
  glLightfv(usedLight, GL_DIFFUSE, col0);
  
  float col1[4]= {0.5,0.5,0.0,  1};
  glLightfv(usedLight, GL_AMBIENT, col1);
   
  float tmpPos[4] = {x,y,z,  1}; // ultima comp=1 => luce posizionale
  glLightfv(usedLight, GL_POSITION, tmpPos );
  
  float tmpDir[4] = {0,0,-1,  0}; // ultima comp=1 => luce posizionale
  glLightfv(usedLight, GL_SPOT_DIRECTION, tmpDir );
  
  glLightf (usedLight, GL_SPOT_CUTOFF, 30);
  glLightf (usedLight, GL_SPOT_EXPONENT,5);
  
  glLightf(usedLight,GL_CONSTANT_ATTENUATION,0);
  glLightf(usedLight,GL_LINEAR_ATTENUATION,1);
  }
  else
   glDisable(usedLight);
}


// funzione che disegna tutti i pezzi della macchina
// (carlinga, + 4 route)
// (da invocarsi due volte: per la macchina, e per la sua ombra)
// (se usecolor e' falso, NON sovrascrive il colore corrente
//  e usa quello stabilito prima di chiamare la funzione)
void Car::RenderAllParts(bool usecolor, int carColor) const{
  //drawCarlinga(); // disegna la carliga con pochi parallelepidedi
  // disegna la carliga con una mesh
  glPushMatrix();
  glScalef(-0.5,0.5,-0.5);// patch: riscaliamo la mesh di 1/10
  // disegna il parabrezza
  drawParabrezza();

  if (!useEnvmap)
  {
    if (usecolor) glColor3f(1,0,0);     // colore rosso, da usare con Lighting
  }
  else {
    if (usecolor) SetupEnvmapTexture(carColor);
  }

  carlinga.RenderNxV(); // rendering delle mesh carlinga usando normali per vertice
  if (usecolor) glEnable(GL_LIGHTING);
  
  for (int i=0; i<2; i++) {
    // i==0 -> disegno ruote destre.
    // i==1 -> disegno ruote sinistre.
    int sign;
    if (i==0) sign=1; 
    else sign=-1;
    glPushMatrix();
    if (i==1) {
        glTranslatef(0,+wheelFR1.Center().Y(), 0);
        glRotatef(180, 0,0,1 );
        glTranslatef(0,-wheelFR1.Center().Y(), 0);
    }
      
    glTranslate(  wheelFR1.Center() );
    glRotatef( sign*sterzo,0,1,0);
    glRotatef(-sign*mozzoA,1,0,0);
    glTranslate( -wheelFR1.Center() );
      
    if (usecolor) glColor3f(.6,.6,.6);
    if (usecolor) SetupWheelTexture(wheelFR1.bbmin,wheelFR1.bbmax);
    wheelFR1.RenderNxF(); // la ruota viene meglio FLAT SHADED - normali per faccia
                            // provare x credere
    glDisable(GL_TEXTURE_2D);
    if (usecolor) glColor3f(0.9,0.9,0.9);
    //wheelFR2.RenderNxV();
    glPopMatrix();
  
    glPushMatrix();
    if (i==1) {
        glTranslatef(0,+wheelBR1.Center().Y(), 0);
        glRotatef(180, 0,0,1 );
        glTranslatef(0,-wheelBR1.Center().Y(), 0);
    }

    glTranslate(  wheelBR1.Center() );
    glRotatef(-sign*mozzoA,1,0,0);
    glTranslate( -wheelBR1.Center() );

    if (usecolor) glColor3f(.6,.6,.6);
    if (usecolor) SetupWheelTexture(wheelBR1.bbmin,wheelBR1.bbmax);
    wheelBR1.RenderNxF();
    glDisable(GL_TEXTURE_2D);
    if (usecolor) glColor3f(0.9,0.9,0.9);
    //wheelBR2.RenderNxV();
    glPopMatrix();
  }

  glPopMatrix(); 
}


 
// disegna a schermo
void Car::Render(int carColor) const{
  // sono nello spazio mondo
  
  //drawAxis(); // disegno assi spazio mondo
  glPushMatrix();
     
  glTranslatef(px,py,pz);
  glRotatef(facing, 0,1,0);

  // sono nello spazio MACCHINA
  //drawAxis(); // disegno assi spazio macchina
  
  DrawHeadlight(-0.3,0,-1, 0, useHeadlight); // accendi faro sinistro
  DrawHeadlight(+0.3,0,-1, 1, useHeadlight); // accendi faro destro

  RenderAllParts(true,carColor);
  
  // ombra!
  if(useShadow)
  {
    glColor3f(0.4,0.4,0.4); // colore fisso
    glTranslatef(0,0.01,0); // alzo l'ombra di un epsilon per evitare z-fighting con il pavimento
    glScalef(1.01,0,1.01);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X 
    glDisable(GL_LIGHTING); // niente lighing per l'ombra
    RenderAllParts(false, carColor);  // disegno la macchina appiattita

    glEnable(GL_LIGHTING);
  } 
  glPopMatrix(); 
  
  glPopMatrix();
}

