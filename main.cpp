#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include "car.h"
#include <glm/glm.hpp>


using namespace glm;
using namespace std;


#define CAMERA_PILOT 0
#define CAMERA_BACK_CAR 1
#define CAMERA_MOUSE 2
#define CAMERA_TYPE_MAX 3

float viewAlpha=20, viewBeta=40; // angoli che definiscono la vista
float eyeDist=5.0; // distanza dell'occhio dall'origine
int scrH=750, scrW=750; // altezza e larghezza viewport (in pixels)
bool useWireframe=false;
bool useEnvmap=true;
bool useHeadlight=false;
bool useShadow=false;
bool fineGioco;
int cameraType=0;

int displayWidth;
int displayHeight;

int carColor = 1;

Car car; // la nostra macchina
int nstep=0; // numero di passi di FISICA fatti fin'ora
const int PHYS_SAMPLING_STEP=10; // numero di millisec che un passo di fisica simula

// Frames Per Seconds
const int fpsSampling = 3000; // lunghezza intervallo di calcolo fps
float fps=0; // valore di fps dell'intervallo precedente
int fpsNow=0; // quanti fotogrammi ho disegnato fin'ora nell'intervallo attuale
Uint32 timeLastInterval=0; // quando e' cominciato l'ultimo intervallo

//funzione esterne dichiarate nella calsse car
extern void drawPista();
extern void drawTabellone();
extern void drawBoard();
extern void drawVia();
extern void drawCone1();
extern void drawCone2();
extern void drawCone3();
extern void drawCone4();
extern void drawCone5();
extern void drawCone6();

// setta le matrici di trasformazione in modo
// che le coordinate in spazio oggetto siano le coord 
// del pixel sullo schemo
void  SetCoordToPixel(){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-1,-1,0);
  glScalef(2.0/scrW, 2.0/scrH, 1);
}

//funzione che permette di caricare una texture
bool LoadTexture(int textbind,char *filename){
  SDL_Surface *s = IMG_Load(filename);
  if (!s) return false;
  glBindTexture(GL_TEXTURE_2D, textbind);
  gluBuild2DMipmaps(
    GL_TEXTURE_2D, 
    GL_RGB,
    s->w, s->h, 
    GL_RGB,
    GL_UNSIGNED_BYTE,
    s->pixels
  );
  glTexParameteri(
  GL_TEXTURE_2D, 
  GL_TEXTURE_MAG_FILTER,
  GL_LINEAR ); 
  glTexParameteri(
  GL_TEXTURE_2D, 
  GL_TEXTURE_MIN_FILTER,
  GL_LINEAR_MIPMAP_LINEAR ); 
  return true;
}

// disegna gli assi nel sist. di riferimento
void drawAxis(){
  const float K=0.10;
  glColor3f(0,0,1);
  glBegin(GL_LINES);
  glVertex3f( -1,0,0 );
  glVertex3f( +1,0,0 );
  glVertex3f( 0,-1,0 );
  glVertex3f( 0,+1,0 );
  glVertex3f( 0,0,-1 );
  glVertex3f( 0,0,+1 );
  glEnd();
  
  glBegin(GL_TRIANGLES);
    glVertex3f( 0,+1  ,0 );
    glVertex3f( K,+1-K,0 );
    glVertex3f(-K,+1-K,0 );

    glVertex3f( +1,   0, 0 );
    glVertex3f( +1-K,+K, 0 );
    glVertex3f( +1-K,-K, 0 );

    glVertex3f( 0, 0,+1 );
    glVertex3f( 0,+K,+1-K );
    glVertex3f( 0,-K,+1-K );
  glEnd();

}

void drawSphere(double r, int lats, int longs) {
int i, j;
  for(i = 0; i <= lats; i++) {
     double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
     double z0  = sin(lat0);
     double zr0 =  cos(lat0);
   
     double lat1 = M_PI * (-0.5 + (double) i / lats);
     double z1 = sin(lat1);
     double zr1 = cos(lat1);
    
     glBegin(GL_QUAD_STRIP);
     for(j = 0; j <= longs; j++) {
        double lng = 2 * M_PI * (double) (j - 1) / longs;
        double x = cos(lng);
        double y = sin(lng);
    
//le normali servono per l'EnvMap
        glNormal3f(x * zr0, y * zr0, z0);
        glVertex3f(r * x * zr0, r * y * zr0, r * z0);
        glNormal3f(x * zr1, y * zr1, z1);
        glVertex3f(r * x * zr1, r * y * zr1, r * z1);
     }
     glEnd();
  }
}

void SetupFloorTexture(){
	// facciamo binding con la texture 1
		glBindTexture(GL_TEXTURE_2D,10);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
		glEnable(GL_TEXTURE_GEN_T);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
		glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
		glDisable(GL_LIGHTING);
}

void drawFloor()
{
  const float S=250; // size
  const float H=0;   // altezza
  const int K=250; //disegna K x K quads
  
  // disegna KxK quads
  glBegin(GL_QUADS);
    glColor3f(0, 0.4, 0); // colore uguale x tutti i quads
    glNormal3f(0,1,0);       // normale verticale uguale x tutti
    for (int x=0; x<K; x++)
    for (int z=0; z<K; z++) {
      float x0=-S + 2*(x+0)*S/K;
      float x1=-S + 2*(x+1)*S/K;
      float z0=-S + 2*(z+0)*S/K;
      float z1=-S + 2*(z+1)*S/K;
      glVertex3d(x0, H, z0);
      glVertex3d(x1, H, z0);
      glVertex3d(x1, H, z1);
      glVertex3d(x0, H, z1);
    }
    //SetupFloorTexture();
  glEnd();

}

// setto la posizione della camera
void setCamera(){

        double px = car.px;
        double py = car.py;
        double pz = car.pz;
        double angle = car.facing;
        double cosf = cos(angle*M_PI/180.0);
        double sinf = sin(angle*M_PI/180.0);
        double camd, camh, ex, ey, ez, cx, cy, cz;
        double cosff, sinff;

// controllo la posizione della camera a seconda dell'opzione selezionata
        switch (cameraType) {
        case CAMERA_BACK_CAR:
                camd = 2.5;
                camh = 1.0;
                ex = px + camd*sinf;
                ey = py + camh;
                ez = pz + camd*cosf;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                break;

        case CAMERA_PILOT:
        		camd = 0.05;
                camh = 0.45;
                ex = px + camd*sinf;
                ey = py + camh;
                ez = pz + camd*cosf;
                cx = px - camd*sinf;
                cy = py + camh;
                cz = pz - camd*cosf;
                gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
                break;
        case CAMERA_MOUSE:
                glTranslatef(0,0,-eyeDist);
                glRotatef(viewBeta,  1,0,0);
                glRotatef(viewAlpha, 0,1,0);
                break;
        }
}

void drawSky() {
int H = 100;

  if (useWireframe) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(0,0,0);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    drawSphere(250.0, 20, 20);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
  }
  else
  {
        glBindTexture(GL_TEXTURE_2D,2);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
        glColor3f(1,1,1);
        glDisable(GL_LIGHTING);
        drawSphere(250.0, 20, 20);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
  }

}

void renderString(float x, float y, std::string text){
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, 640, 480, 0.0, -1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glColor3f(0, 0, 0);
  glRasterPos2f(x, y);
  char tab2[1024];
  strncpy(tab2, text.c_str(), sizeof(tab2));
  tab2[sizeof(tab2) - 1] = 0;
  glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char *)tab2);
  glPopAttrib();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

/*Questa funzione viene invocata per visualizzare la schermata di avvio del gioco. Nella prima schermata viene data la possibilita'
 * all'utente di poter selezionare il colore della vettura con cui giocare*/
void renderFirtScene(SDL_Window *win, int color){
	  fpsNow++;
	  glLineWidth(5); // linee larghe
      //configuro il colore dello sfondo
	  glClearColor(1,1,1,1);
	  // settiamo la matrice di proiezione
	  glMatrixMode( GL_PROJECTION );
	  glLoadIdentity();
	  gluPerspective( 80, //fovy,
			((float)scrW) / scrH,//aspect Y/X,
			0.25,//zNear,
			100  //zFar
	  );
	  glMatrixMode( GL_MODELVIEW );
	  glLoadIdentity();
	  // riempe tutto lo screen buffer di pixel color sfondo
	  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	  // setto la posizione luce
	  float tmpv[4] = {0,1,2,  0}; // ultima comp=0 => luce direzionale
	  // settiamo matrice di vista
	  glTranslatef(0,0,-eyeDist);
	  glRotatef(viewBeta,  1,0,0);
	  glRotatef(viewAlpha, 0,1,0);
	  //drawAxis(); // disegna assi frame MONDO
	  static float tmpcol[4] = {1,1,1,1};
	  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
	  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);
	  //disegno la vattuara posizionata al centro dello schermo
	  glViewport(0,0, displayWidth, displayHeight);
	  car.Render(color);

    // mette al centro dello schermo

	//glViewport(0,scrW, scrW, 100);
	glViewport(displayWidth*0.4,displayHeight*0.8,50,50);
	renderString(0.0,0.0, "Progetto Esame Grafica 2017");
	glViewport(displayWidth*0.25,displayHeight*0.7,30,30);
	renderString(0.0,0.0, "Premi il tasto \"1\", \"2\" o \"3\" per scegliere il colore dell'auto e poi INVIO per iniziare a giocare");
	glViewport(0,0, scrW, scrH/2);
	glFinish();

	// ho finito: buffer di lavoro diventa visibile
	SDL_GL_SwapWindow(win);
}


/* Esegue il Rendering della scena */
void renderingGame(SDL_Window *win){
  // un frame in piu'!!!
  fpsNow++;
  glLineWidth(3); // linee larghe
  // settiamo il viewport
  glViewport(0,0, displayWidth - displayWidth/4, displayHeight);
  // colore sfondo = bianco
  glClearColor(1,1,1,1);
  // settiamo la matrice di proiezione
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluPerspective( 70, //fovy,
		((float)scrW) / scrH,//aspect Y/X,
		0.2,//distanza del NEAR CLIPPING PLANE in coordinate vista
		1000  //distanza del FAR CLIPPING PLANE in coordinate vista
  );
  glMatrixMode( GL_MODELVIEW ); 
  glLoadIdentity();
  // riempe tutto lo screen buffer di pixel color sfondo
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  // setto la posizione luce
  float tmpv[4] = {0,1,2,  0}; // ultima comp=0 => luce direzionale
  //float tmpv[4] = {0,0,0,  0};
  glLightfv(GL_LIGHT0, GL_POSITION, tmpv );

  setCamera();
  //drawAxis(); // disegna assi frame MONDO
  static float tmpcol[4] = {1,1,1,  1};
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);
  
  glEnable(GL_LIGHTING);

  //invoco tutte le funzioni che disegnano i rispettivi oggetti grafici collocati all'inerno della scena
  drawCone1(); // disegna i coni
  drawCone2();
  drawCone3();
  drawCone4();
  drawCone5();
  drawVia();// disegna la linea del traguardo
  drawSky(); // disegna il cielo come sfondo
  drawFloor(); // disegna il suolo
  drawPista(); // disegna la pista
  drawTabellone(); // disegna il tabellone
  drawBoard(); // disegna lo sfondo del tabellone su cui è applicata l'iimagine di sfondo
  car.Render(carColor); // disegna la macchina

  // attendiamo la fine della rasterizzazione di 
  // tutte le primitive mandate 
  
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
 
// disegnamo i fps (frame x sec) come una barra a sinistra.
// (vuota = 0 fps, piena = 100 fps)
  SetCoordToPixel();
  
  glBegin(GL_QUADS);
  float y=scrH*fps/100;
  float ramp=fps/100;
  glColor3f(1-ramp,0,ramp);
  glVertex2d(10,0);
  glVertex2d(10,y);
  glVertex2d(0,y);
  glVertex2d(0,0);
  glEnd();
  
  //Gestione del viewport laterale della vista dall'alto pista
  	// settiamo il viewport vista dall'alto pista
  	glViewport(displayWidth - displayWidth / 4, 0, displayWidth / 1.5, displayHeight / 1.2);
  	// settiamo la matrice di proiezione
  	glMatrixMode( GL_PROJECTION);
  	glLoadIdentity();
  	gluPerspective(70, //fovy,
  			((float) displayWidth) / displayHeight, //aspect Y/X,
  			0.2, //distanza del NEAR CLIPPING PLANE in coordinate vista
  			1000  //distanza del FAR CLIPPING PLANE in coordinate vista
  			);

  	glMatrixMode( GL_MODELVIEW);
  	glLoadIdentity();

  	// settiamo matrice di vista
  	glTranslatef(-134, -33, -140.0);
  	glRotatef(90, 1, 0, 0);
  	glRotatef(90, 0, 1, 0);

  	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
  	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);
  	//
  	drawFloor(); // disegna il suolo nel viewport laterale vista dall'alto pista
  	drawPista(); // disegna la pista nel viewport laterale vista dall'alto pista
  	drawVia();
  	car.Render(carColor); // disegna la macchina nel viewport laterale vista dall'alto pista
  	// attendiamo la fine della rasterizzazione di
  	// tutte le primitive mandate
  	glDisable(GL_DEPTH_TEST);
  	glDisable(GL_LIGHTING);
  	SetCoordToPixel();
  	glColor3f(0, 0, 0);
  	glBegin(GL_LINE_LOOP);
  	glVertex2d(displayWidth, 0);
  	glVertex2d(displayWidth, displayHeight);
  	glVertex2d(0, displayHeight);
  	glVertex2d(0, 0);
  	glEnd();

  	//Gestione dello score board
  	// settiamo il viewport  dello score
	glViewport(displayWidth - displayWidth / 4, displayHeight / 1.2 , displayWidth / 1.5, displayHeight - displayHeight / 1.2);
	SetCoordToPixel();
	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP);

	glVertex2d(displayWidth, 0);
	glVertex2d(displayWidth, displayHeight);
	glVertex2d(0, displayHeight);
	glVertex2d(0, 0);

	glEnd();
	drawCone6();
	int numCono = 5 - car.contaColpito();
	ostringstream convert;
	convert << numCono;
	// gestione dei feedback visivi nello score board
	if(numCono == 5){
		renderString(80, 200, "HAI VINTO");
		fineGioco = true;
	}else{
		renderString(80, 200, "Score: " + convert.str());
	}
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glFinish();
  // ho finito: buffer di lavoro diventa visibile
  SDL_GL_SwapWindow(win);
}

void redraw(){
  // ci automandiamo un messaggio che (s.o. permettendo)
  // ci fara' ridisegnare la finestra
  SDL_Event e;
  e.type=SDL_WINDOWEVENT;
  e.window.event=SDL_WINDOWEVENT_EXPOSED;
  SDL_PushEvent(&e);
}

int startMenuIniziale (Uint32 windowID,SDL_Window *win){
	bool done=0;

	while (!done) {


		  SDL_Event event;
	      // guardo se c'e' un evento:
	      if (SDL_PollEvent(&event)) {
	       // se si: processa evento
	    	  switch (event.type) {
	    	  	  case SDL_QUIT:
	    	  		  done=1;
	    	  		  	SDL_DestroyWindow(win);
	    	  			SDL_Quit ();
	    	  			return EXIT_FAILURE;
	    	  		  break;
	    	  }
	    	  switch (event.key.keysym.sym) {
	    	  	  case SDLK_1: {
	    	  		  	carColor=1;
	    	  			renderFirtScene(win,carColor);
	    	  	  }
	    	  	  break;
	    	  	  case SDLK_2: {
	    	  		  	carColor=2;
	    	  	    	renderFirtScene(win,carColor);
	    	  	  }
	    	  	  break;
	    	  	  case SDLK_3: {
	    	  		  	carColor=3;
	    	  	    	renderFirtScene(win,carColor);
	    	  	  }
	    	  	  break;
	    	  	  case SDLK_RETURN: {
						done = true;
				  }
				  break;

	    	  	  //break;
	    	  	  case SDL_WINDOWEVENT:
	    	  		  // dobbiamo ridisegnare la finestra
	    	  		  if (event.window.event==SDL_WINDOWEVENT_EXPOSED){

	    	  			  done=0;
	    	  		  }else{
	    	  			  windowID = SDL_GetWindowID(win);
	    	  		  if (event.window.windowID == windowID)  {
	    	  			  switch (event.window.event)  {
	    	  			  case SDL_WINDOWEVENT_SIZE_CHANGED:  {
	    	  				  scrW = event.window.data1;
	    	  				  scrH = event.window.data2;
	    	  				  glViewport(0,0,scrW,scrH);
	    	  				  renderFirtScene(win,carColor);
	    	  				  //redraw(); // richiedi ridisegno
	                      break;
	    	  			  }
	    	  			  }
	    	  		  }
	    	  		  }
	    	  	break;
	  }
	  } else {
	        // nessun evento: siamo IDLE

		   Uint32 timeNow=SDL_GetTicks(); // che ore sono?

	        if (timeLastInterval+fpsSampling<timeNow) {
	          // e' tempo di ricalcolare i Frame per Sec
	          fps = 1000.0*((float)fpsNow) /(timeNow-timeLastInterval);
	          fpsNow=0;
	          timeLastInterval = timeNow;
	        }

	        bool doneSomething=false;
	        int guardia=0; // sicurezza da loop infinito

	        // finche' il tempo simulato e' rimasto indietro rispetto
	        // al tempo reale...
	        while (nstep*PHYS_SAMPLING_STEP < timeNow ) {

	        viewAlpha+=0.3;

	          nstep++;
	          doneSomething=true;
	          timeNow=SDL_GetTicks();
	          if (guardia++>1000) {done=true; break;} // siamo troppo lenti!
	        }

	        if (doneSomething){
	        	renderFirtScene(win,carColor);

	        } else {
	          // tempo libero!!!
	        }
	      }

	  }

	//SDL_GL_DeleteContext(mainContext);
}

int main(int argc, char* argv[])
{
SDL_Window *win;
SDL_GLContext mainContext;
Uint32 windowID;
SDL_Joystick *joystick;
static int keymap[Controller::NKEYS] = {SDLK_a, SDLK_d, SDLK_w, SDLK_s};
  glutInit(&argc, argv);
  // inizializzazione di SDL
  SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

  SDL_JoystickEventState(SDL_ENABLE);
  joystick = SDL_JoystickOpen(0);

  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); 
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  displayWidth = DM.w;
  displayHeight = DM.h;
  // facciamo una finestra di scrW x scrH pixels
  win=SDL_CreateWindow(argv[0], 0, 0, displayWidth, displayHeight, SDL_WINDOW_OPENGL);

  //Create our opengl context and attach it to our window
  mainContext=SDL_GL_CreateContext(win);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE); // opengl, per favore, rinormalizza le normali 
                          // prima di usarle
  //glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW); // consideriamo Front Facing le facce ClockWise
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_POLYGON_OFFSET_FILL); // caro openGL sposta i 
                                    // frammenti generati dalla
                                    // rasterizzazione poligoni
  glPolygonOffset(1,1);             // indietro di 1
  //Gestione del mapping delle texture
  if (!LoadTexture(0,(char *)"logo.jpg")) return 0;
  if (!LoadTexture(1,(char *)"envmap_flipped.jpg")) return 0;
  if (!LoadTexture(2,(char *)"sky_ok.jpg")) return -1;
  if (!LoadTexture(3,(char *)"via.jpg")) return 0;
  if (!LoadTexture(4,(char *)"via.jpg")) return 0;
  LoadTexture(5,(char *)"purple_ball.jpg");
  LoadTexture(6,(char *)"green_ball.jpg");
  LoadTexture(7,(char *)"blue_ball.jpg");
  LoadTexture(8,(char *)"20170716_192807.jpg");
  LoadTexture(9,(char *)"textureCone.jpg");
  LoadTexture(10,(char *)"track.jpg");
  LoadTexture(11,(char *)"glass_right.png");

  int exitProgram = startMenuIniziale(windowID, win);

  if (exitProgram == 1){
	  return 0;
  }
 
  bool done=0;
  while (!done) {
    
    SDL_Event e;
    
    // guardo se c'e' un evento:
    if (SDL_PollEvent(&e)) {
     // se si: processa evento
     switch (e.type) {
      case SDL_KEYDOWN:
        car.controller.EatKey(e.key.keysym.sym, keymap , true);
        if (e.key.keysym.sym==SDLK_F1) cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
        //gestione dei tasti funzione
        if(fineGioco){
        	car.rallenta();
        }
        break;
      case SDL_KEYUP:
        car.controller.EatKey(e.key.keysym.sym, keymap , false);
        break;
      case SDL_QUIT:  
          done=1;   break;
      case SDL_WINDOWEVENT:
         // dobbiamo ridisegnare la finestra
          if (e.window.event==SDL_WINDOWEVENT_EXPOSED)
            renderingGame(win);
          else{
           windowID = SDL_GetWindowID(win);
           if (e.window.windowID == windowID)  {
             switch (e.window.event)  {
                  case SDL_WINDOWEVENT_SIZE_CHANGED:  {
                     scrW = e.window.data1;
                     scrH = e.window.data2;
                     glViewport(0,0,scrW,scrH);
                     renderingGame(win);
                     //redraw(); // richiedi ridisegno
                     break;
                  }
             }
           }
         }
      break;
        
      case SDL_MOUSEMOTION:
        if (e.motion.state & SDL_BUTTON(1) & cameraType==CAMERA_MOUSE) {
          viewAlpha+=e.motion.xrel;
          viewBeta +=e.motion.yrel;
//          if (viewBeta<-90) viewBeta=-90;
          if (viewBeta<+5) viewBeta=+5; //per non andare sotto la macchina
          if (viewBeta>+90) viewBeta=+90;
          // redraw(); // richiedi un ridisegno (non c'e' bisongo: si ridisegna gia' 
	               // al ritmo delle computazioni fisiche)
        }
        break; 
       
     case SDL_MOUSEWHEEL:
       if (e.wheel.y < 0 ) {
         // avvicino il punto di vista (zoom in)
         eyeDist=eyeDist*0.9;
         if (eyeDist<1) eyeDist = 1;
       };
       if (e.wheel.y > 0 ) {
         // allontano il punto di vista (zoom out)
         eyeDist=eyeDist/0.9;
       };
     break;

     case SDL_JOYAXISMOTION: /* Handle Joystick Motion */
        if( e.jaxis.axis == 0)
         {
            if ( e.jaxis.value < -3200  )
             {
              car.controller.Joy(0 , true);
              car.controller.Joy(1 , false);                 
//	      printf("%d <-3200 \n",e.jaxis.value);
             }
            if ( e.jaxis.value > 3200  )
            {
              car.controller.Joy(0 , false); 
              car.controller.Joy(1 , true);
//	      printf("%d >3200 \n",e.jaxis.value);
            }
            if ( e.jaxis.value >= -3200 && e.jaxis.value <= 3200 )
             {
              car.controller.Joy(0 , false);
              car.controller.Joy(1 , false);                 
//	      printf("%d in [-3200,3200] \n",e.jaxis.value);
             }            
	    renderingGame(win);
            //redraw();
        }
        break;
      case SDL_JOYBUTTONDOWN: /* Handle Joystick Button Presses */
        if ( e.jbutton.button == 0 )
        {
           car.controller.Joy(2 , true);
//	   printf("jbutton 0\n");
        }
        if ( e.jbutton.button == 2 )
        {
           car.controller.Joy(3 , true);
//	   printf("jbutton 2\n");
        }
        break; 
      case SDL_JOYBUTTONUP: /* Handle Joystick Button Presses */
           car.controller.Joy(2 , false);
           car.controller.Joy(3 , false);
        break; 
     }
    } else {
      // nessun evento: siamo IDLE
      
      Uint32 timeNow=SDL_GetTicks(); // che ore sono?
      
      if (timeLastInterval+fpsSampling<timeNow) {
        fps = 1000.0*((float)fpsNow) /(timeNow-timeLastInterval);
        fpsNow=0;
        timeLastInterval = timeNow;
      }
      
      bool doneSomething=false;
      int guardia=0; // sicurezza da loop infinito

      // finche' il tempo simulato e' rimasto indietro rispetto
      // al tempo reale...
      while (nstep*PHYS_SAMPLING_STEP < timeNow ) {
        car.DoStep();
        nstep++;
        doneSomething=true;
        timeNow=SDL_GetTicks();
        if (guardia++>1000) {done=true; break;} // siamo troppo lenti!
      }
      
      if (doneSomething) {
    	  renderingGame(win);
                //redraw();
      } else {
	  // tempo libero!!!
      }

    }
  }
SDL_GL_DeleteContext(mainContext);
SDL_DestroyWindow(win);
SDL_Quit ();
return (0);
}



