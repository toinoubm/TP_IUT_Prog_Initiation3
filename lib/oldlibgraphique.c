/*******************************************************************************
 * libraphique.c                                                               *
 * Quelques fonctions C basées sur la SDl pour réaliser des opérations         *
 * graphiques simples                                                          *
 *                                                                             *
 * Pour compiler en ligne de commande :                                        *
 * gcc ../lib/libgraphique.c prog.c -o resultat                                *
 *           `sdl-config --libs --cflags` -lm -lSDL                            *
 *                                                                             *
 * où                                                                          *
 *             prog.c : votre code source                                      *
 *             resultat    : nom de l'exécutable                               *
 *******************************************************************************
*/

////////////////////////////////////////////////////////////////////////////////
// 0. directives préprocesseur

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "libgraphique.h"


////////////////////////////////////////////////////////////////////////////////
// 0. variables globales et macros

SDL_Surface *ecran = NULL;                 // totalité de l'écran
SDL_Event lastevent ;                      // utilisé pour gestion événements
Trace_evts trace_evts ;                    // idem
Point dernier_clic = {-1,-1};
int LARGEUR = -1 ;                         // largeur de l'écran en pixels
int HAUTEUR = -1 ;                         // hauteur de l'écran en pixels
#define octets_par_pixel ecran->format->BytesPerPixel
#define largeur_ecran (ecran->pitch / 4)


////////////////////////////////////////////////////////////////////////////////
// 1. Ouvrir et fermer une fenêtre

// ouvrir une fenêtre de taille largeur (x), hauteur (y)
void ouvrir_fenetre(int largeur, int hauteur){
    SDL_Init(SDL_INIT_VIDEO); 
    ecran = SDL_SetVideoMode(largeur, hauteur, 32, SDL_HWSURFACE|SDL_DOUBLEBUF ); 

    //initialisation des variables globales
    LARGEUR = largeur ;
    HAUTEUR = hauteur ;
    //printf("LARGEUR %d HAUTEUR %d\n",LARGEUR,HAUTEUR);

    // pour permettre les répétitions de touche si elles restent enfoncées
    SDL_EnableKeyRepeat(100, 100);

    //initialisation du hasard
    srand(time(NULL));
}


// terminer le programme
void fermer_fenetre(){
    SDL_Quit();
    exit(0);
}

// teste si la fin du programme a été demandée et le termine si nécessaire
void _test_arret() {
    if ((lastevent.type == SDL_QUIT) || 
            ( (lastevent.type == SDL_KEYDOWN )
              && (lastevent.key.keysym.sym == SDLK_ESCAPE)) 
       )
        fermer_fenetre() ;
}


///////////////////////////////////////////////////////////////////////////////
// 2. Fonctions de dessin

// actualise l'affichage des modifications graphiques
// sans appel à cet fonction les modifications sont non apparentes
void actualiser(){
    SDL_PollEvent(&lastevent) ;
    _test_arret();
    SDL_Flip(ecran) ;
}


// fonction de dessin principale
// changer la couleur du Point pix
void changer_pixel(Point pix, Couleur couleur) {
    if ((0 <= pix.x) && (pix.x < LARGEUR) && (0 <= pix.y ) && (pix.y < HAUTEUR))
    {
        *( (Uint32*)ecran->pixels + pix.y * largeur_ecran + pix.x ) = couleur ;
    }
}


// dessine un rectangle de couleur de largeur et hauteur données
// coin est le coin haut, gauche
void dessiner_rectangle(Point coin, int largeur, int hauteur, Couleur couleur) {
    Point p ;
    int bord_droit = coin.x + largeur ;
    int bord_bas = coin.y + hauteur ;
    for (p.x = coin.x; p.x < bord_droit ; ++(p.x)) {
        for (p.y = coin.y ; p.y  < bord_bas ; ++(p.y) ) {
            changer_pixel(p, couleur);
        }
    }
}

// trace une ligne du Point p1 au point p2 dela couleur donnée
// utilise l'algorithme de Bresenham
void dessiner_ligne(Point p1, Point p2, Couleur couleur)
{
    // signes deplacement x et y
    int dx, dy ;

    if (p1.x < p2.x)
        dx = 1 ;
    else if (p1.x > p2.x)
        dx = -1 ;
    else
        dx = 0 ;

    if (p1.y < p2.y)
        dy = 1 ;
    else if (p1.y > p2.y)
        dy = -1 ;
    else
        dy = 0 ;

    // valeur absolue pente
    float pente_abs ; 

    float erreur = 0;
    Point p ; // point courant

    //lignes horizontales et certicales : plus rapide
    if (dy == 0) 
    {
        p.y = p1.y ;
        for(p.x = p1.x ; p.x != p2.x ; p.x += dx)
            changer_pixel(p,couleur);
    }
    else if (dx == 0)
    {  
        p.x = p1.x ;
        for(p.y = p1.y ; p.y != p2.y ; p.y += dy)
            changer_pixel(p,couleur);
    }
    else
    {

        pente_abs  = (p2.y - p1.y)/((float)(p2.x - p1.x)) ;
        if (pente_abs < 0)
            pente_abs = - pente_abs ;

        if (pente_abs <=1 )
        {
            {
                p.y = p1.y ; 
                for(p.x = p1.x ; p.x != p2.x ; p.x += dx)
                {
                    changer_pixel(p, couleur);
                    erreur += pente_abs ;
                    if (erreur > 0.5)
                    {
                        erreur -= 1 ;
                        p.y += dy ;
                    }
                }
            }
        }
        else 
        {
            pente_abs = 1/pente_abs ;
            p.x = p1.x ; 
            for(p.y = p1.y ; p.y != p2.y ; p.y += dy)
            {
                changer_pixel(p, couleur);
                erreur += pente_abs ;
                if (erreur > 0.5)
                {
                    erreur -= 1 ;
                    p.x += dx ;
                }
            }
        }
    }
    changer_pixel(p2,couleur);
}


// affiche l'image sous forme .bmp (bitmap), contenue dans le même dossier
// nom est une chaine de caracteres qui est le nom (complet) du fichier image
// coin est le coin haut, gauche voulu pour l'image à afficher dans l'ecran
void afficher_image(char *nom, Point coin){
    SDL_Surface *img = SDL_LoadBMP(nom) ;
    SDL_Rect position_img ;
    position_img.x = coin.x;
    position_img.y = coin.y;
    SDL_BlitSurface(img,NULL,ecran,&position_img);
}


////////////////////////////////////////////////////////////////////////////////
// 3. Gestion des événements

// renvoie le code SDLK de la prochaine touche pressée
// fonction bloquante
int attendre_touche(void){
    do {
        SDL_WaitEvent(&lastevent) ;
        _test_arret() ;
    }
    while (lastevent.type != SDL_KEYDOWN ) ;
    return lastevent.key.keysym.sym;
}

// renvoie les coordonnees du prochain clic (gauche ou droite) de souris
// fonction bloquante
Point attendre_clic() {
    do {
        SDL_WaitEvent(&lastevent) ;
        _test_arret();
    }
    while (lastevent.type != SDL_MOUSEBUTTONDOWN) ;
    Point p ;
    p.x = lastevent.button.x ;
    p.y = lastevent.button.y ;
    return p;
}


////////////////////////////////////////////////////////////////////////////////
// 3 bis : fonctions optionnelles pour les événements, non bloquantes


//reinitialise la mémoire des événements à 0
void reinitialiser_evenements(void)
{
    memset(trace_evts.touches, 0, sizeof(trace_evts.touches)) ;
    dernier_clic.x = -1;
    dernier_clic.y = -1;
}

// memorise les evenements ayant eu lieu depuis la derniere
// reinitialisation
void traiter_evenements(void)
{

    while(SDL_PollEvent(&lastevent))
    {
        switch(lastevent.type)
        {
            case SDL_KEYDOWN:
                trace_evts.touches[lastevent.key.keysym.sym]=1 ;
                break ;
            case SDL_MOUSEBUTTONDOWN:
                dernier_clic.x = lastevent.motion.x ;
                dernier_clic.y = lastevent.motion.y ;
        }

    }

}

// indique si la touche de code SDL en question a été pressée 
// entre la derniere reinitialisation et le dernier traitement
int touche_a_ete_pressee(int code)
{   
    return trace_evts.touches[code] ;
}

//renvoie les coordonnees du dernier clic entre la
// entre la derniere reinitialisation et le dernier traitement
// Point (-1,-1) si pas de clic
Point clic_a_eu_lieu()
{
    Point res = dernier_clic ;
    dernier_clic.x = -1;
    dernier_clic.y = -1 ;
    return res;
}

////////////////////////////////////////////////////////////////////////////////
// 4. Autres

// renvoie une Couleur (UInt32) RGB avec les valeurs entre 0 et 255 données
// en rouge r, vert g et bleu b
Couleur fabrique_couleur(int r, int g, int b)
{
    Couleur C;
    return ((r%256)<<16) + ((g%256)<<8) + (b%256);
    return C;
}

// pause le programme pour une duree en millisecondes
void attente(int duree_ms)
{
    SDL_Delay(duree_ms);
}


//renvoie la couleur du point donné ; renvoie noir si le point est hors de l'écran
Couleur couleur_point(Point p)
{

    if ((0 <= p.x) && (p.x < LARGEUR) && (0 <= p.y ) && (p.y < HAUTEUR))
    {
        return *( (Uint32*)ecran->pixels + p.y * largeur_ecran + p.x ) ;
    }
    else
        return noir ;
}


//renvoie un entier au hasard entre 0 et n-1
int entier_aleatoire(int n)
{
    return rand() % n ;
}



