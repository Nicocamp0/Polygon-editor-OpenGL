//   Projet : Saisie et remplissage d’un polygone 2D

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <GL/gl.h>

#include "Image.h"

Image *img;

/* ---------------------------- Donnees polygone ---------------------------- */

typedef struct Vertex {
    int x, y;
    struct Vertex* prev;
    struct Vertex* next;
} Vertex;

typedef enum { MODE_APPEND=0, MODE_VERTEX=1, MODE_EDGE=2 } Mode;

static Vertex* head = NULL;
static Vertex* tail = NULL;
static int poly_ferme = 0;   //=0 => ligne brisee ouverte =1 =>polygone ferme
static int remplissage = 0;        // =0 => pas rempli =1=> rempli
static Mode mode = MODE_APPEND;

static Vertex* selectedV = NULL;      // en mode vertex
static Vertex* selectedE_A = NULL;    // arête selectionnee = (arrete -> arrete->next)
                                      // si ferme last->next = head


// ---- Prototypes ----
static Vertex* V_new(int x, int y);
static int V_count(void);
static void update_circular_links(void);
static void append_vertex(int x, int y);
static void remove_vertex(Vertex* v);
static void insert_vertex(Vertex* a, Vertex* newv);


/* ---------------------------- Couleurs ---------------------------- */

static Color COL_BG;
static Color COL_EDGE;
static Color COL_EDGE_SEL;
static Color COL_VERTEX_SEL;
static Color COL_FILL;

/* ---------------------------- Utilitaires liste ---------------------------- */

static Vertex* V_new(int x, int y) {
    Vertex* v = (Vertex*)malloc(sizeof(Vertex));
    v->x = x; v->y = y;
    v->prev = NULL; v->next = NULL;
    return v;
}

static int V_count(void) {
    if (!head) return 0;
    int n = 0;
    Vertex* v = head;
    while (v) {
        n++;
        v = v->next;
        if (poly_ferme && v == head) break;
    }
    return n;
}

static void update_circular_links(void) {
    if (!head) { tail = NULL; return; }
    if (!poly_ferme) {
        // recalc tail
        Vertex* v = head;
        while (v->next) v = v->next;
        tail = v;
        head->prev = NULL;
        tail->next = NULL;
    } else {
        // recalc tail: last before coming back to head
        Vertex* v = head;
        while (v->next && v->next != head) v = v->next;
        tail = v;
        tail->next = head;
        head->prev = tail;
    }
}

static void insert_vertex(Vertex* a, Vertex* newv) {
    if (!a || !newv) return;
    Vertex* b = a->next;

    a->next = newv;
    newv->prev = a;
    newv->next = b;
    if (b) b->prev = newv;

    if (!poly_ferme) {
        if (tail == a) tail = newv;
    } else {
        if (a == tail) tail = newv;
        head->prev = tail;
        tail->next = head;
    }
}


static void append_vertex(int x, int y) {
    Vertex* v = V_new(x, y);
    if (!head) {
        head = tail = v;
        selectedV = head;
        selectedE_A = head;
        if (poly_ferme) update_circular_links();
        return;
    }
    if (!poly_ferme) {
        tail->next = v;
        v->prev = tail;
        tail = v;
    } else {
        // insere avant head
        Vertex* old_tail = head->prev;
        old_tail->next = v;
        v->prev = old_tail;
        v->next = head;
        head->prev = v;
        tail = v;
    }
}

static void remove_vertex(Vertex* v) {
    if (!v || !head) return;

    int n = V_count();

    // soit il n'y a qu'un seul sommet:
    if (n == 1) {
        free(v);
        head = tail = NULL;
        selectedV = NULL;
        selectedE_A = NULL;
        poly_ferme = 0;
        remplissage = 0;
        return;
    }

    // soit on supprime et il restera 1 sommet
    if (n == 2) {
        Vertex* other = (v == head) ? (poly_ferme ? v->next : head->next) : head;

        // si ferme other sera head ou next selon ou on est
        if (poly_ferme) {
            other = (v->next != v) ? v->next : v->prev;
        } else {
            other = (v == head) ? head->next : head;
        }
        free(v);

        // reconstruction de la liste ouverte avec le seul sommet restant
        head = other;
        head->prev = NULL;
        head->next = NULL;
        tail = head;

        selectedV = head;
        selectedE_A = head;

        poly_ferme = 0;
        remplissage = 0;
        return;
    }

    //si n est sup ou egal a 3:
    Vertex* nextSel = v->next;
    if (!nextSel) nextSel = v->prev;

    if (!poly_ferme) {
        // liste ouverte
        if (v == head) {
            head = v->next;
            head->prev = NULL;
        } else if (v == tail) {
            tail = v->prev;
            tail->next = NULL;
        } else {
            v->prev->next = v->next;
            v->next->prev = v->prev;
        }
        free(v);
    } else {
        // liste fermee circulaire
        if (v == head) head = v->next;
        v->prev->next = v->next;
        v->next->prev = v->prev;
        free(v);

        // on recalcul tail
        tail = head->prev;
        tail->next = head;
        head->prev = tail;
    }

    // maj de la selection
    selectedV = nextSel;
    if (poly_ferme) selectedE_A = selectedV; 
    else selectedE_A = head;
    // si apres la suppression on a moins de 3 sommets
    // notre polygone ne peut plus être "ferme"
    if (V_count() < 3) {
        poly_ferme = 0;
        remplissage = 0;
        if (head && head->prev) {
            tail = head->prev;
            head->prev = NULL;
            tail->next = NULL;
        }
    }
}

/* ---------------------------- Dessin : Bresenham ---------------------------- */

static void draw_line_bresenham(int x0, int y0, int x1, int y1, Color c) {
    int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        I_plotColor(img, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_selected_vertex_marker(Vertex* v) {
    if (!v) return;
    int r = 3;
    for (int dx=-r; dx<=r; dx++) {
        I_plotColor(img, v->x + dx, v->y - r, COL_VERTEX_SEL);
        I_plotColor(img, v->x + dx, v->y + r, COL_VERTEX_SEL);
    }
    for (int dy=-r; dy<=r; dy++) {
        I_plotColor(img, v->x - r, v->y + dy, COL_VERTEX_SEL);
        I_plotColor(img, v->x + r, v->y + dy, COL_VERTEX_SEL);
    }
}

static void draw_texte(int x, int y, const char* texte){
    glColor3f(1.0f,1.0f,1.0f);
    glRasterPos2i(x,y);
    for(const char* c= texte; *c !='\0'; c++){
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*c);
    }
}

static const char* mode_texte(void){
    switch(mode){
        case MODE_APPEND: return "Mode: Append";
        case MODE_VERTEX: return "Mode: Vertex";
        case MODE_EDGE: return "Mode: Edge";
        default: return "Mode:";
    }
}

/* ---------------------------- Remplissage scan-line ---------------------------- */

static int edge_intersects_scanline(Vertex* a, Vertex* b, int y) {
    // on ignore horizontale
    if (a->y == b->y) return 0;
    int ymin = (a->y < b->y) ? a->y : b->y;
    int ymax = (a->y > b->y) ? a->y : b->y;
    return (y >= ymin && y < ymax);
}

static int cmp_int(const void* p1, const void* p2) {
    int a = *(const int*)p1;
    int b = *(const int*)p2;
    return (a > b) - (a < b);
}

static void fill_scanline(void) {
    if (!poly_ferme || !head) return;
    int n = V_count();
    if (n < 3) return;

    int ymin = head->y, ymax = head->y;
    Vertex* v = head;
    while (v) {
        if (v->y < ymin) ymin = v->y;
        if (v->y > ymax) ymax = v->y;
        v = v->next;
        if (poly_ferme && v == head) break;
    }
    if (ymin < 0) ymin = 0;
    if (ymax >= img->_height) ymax = img->_height - 1;

    // Pour chaque "scanline y" on prend les intersection
    for (int y = ymin; y <= ymax; y++) {
        int xs_capacity = 64;
        int xs_count = 0;
        int* xs = (int*)malloc(xs_capacity * sizeof(int));

        Vertex* a = head;
        while (a) {
            Vertex* b = a->next;
            if (!b) break; // ouvert ne devrait pas arriver si poly_ferme

            if (edge_intersects_scanline(a, b, y)) {
                double t = (double)(y - a->y) / (double)(b->y - a->y);
                double x = a->x + t * (double)(b->x - a->x);
                int xi = (int)lround(x);

                if (xs_count >= xs_capacity) {
                    xs_capacity *= 2;
                    xs = (int*)realloc(xs, xs_capacity * sizeof(int));
                }
                xs[xs_count++] = xi;
            }

            a = a->next;
            if (poly_ferme && a == head) break;
        }

        if (xs_count >= 2) {
            qsort(xs, xs_count, sizeof(int), cmp_int);

            for (int i = 0; i + 1 < xs_count; i += 2) {
                int x0 = xs[i];
                int x1 = xs[i+1];
                if (x0 > x1) { int tmp = x0; x0 = x1; x1 = tmp; }
                if (x0 < 0) x0 = 0;
                if (x1 >= img->_width) x1 = img->_width - 1;

                for (int x = x0; x <= x1; x++) {
                    I_plotColor(img, x, y, COL_FILL);
                }
            }
        }

        free(xs);
    }
}

/* ---------------------------- Selection souris : distances ---------------------------- */

static double dist2(double ax, double ay, double bx, double by) {
    double dx = ax - bx, dy = ay - by;
    return dx*dx + dy*dy;
}

static Vertex* closest_vertex(int x, int y) {
    if (!head) return NULL;
    Vertex* best = head;
    double bestd = dist2(x,y, head->x, head->y);

    Vertex* v = head->next;
    while (v) {
        double d = dist2(x,y, v->x, v->y);
        if (d < bestd) { bestd = d; best = v; }
        v = v->next;
        if (poly_ferme && v == head) break;
    }
    return best;
}

static double point_segment_dist2(double px, double py, double ax, double ay, double bx, double by) {
    double vx = bx - ax, vy = by - ay;
    double wx = px - ax, wy = py - ay;
    double c1 = vx*wx + vy*wy;
    if (c1 <= 0) return dist2(px,py, ax,ay);
    double c2 = vx*vx + vy*vy;
    if (c2 <= c1) return dist2(px,py, bx,by);
    double t = c1 / c2;
    double projx = ax + t * vx;
    double projy = ay + t * vy;
    return dist2(px,py, projx,projy);
}

static Vertex* closest_edge(int x, int y) {
    // renvoie A tel que l'arrete est (arrete -> arrete->next)
    if (!head) return NULL;
    if (!poly_ferme && !head->next) return NULL;

    Vertex* bestA = head;
    Vertex* a = head;

    // init
    Vertex* b = a->next;
    if (!b) return NULL;
    double bestd = point_segment_dist2(x,y, a->x,a->y, b->x,b->y);

    while (a) {
        b = a->next;
        if (!b) break;

        double d = point_segment_dist2(x,y, a->x,a->y, b->x,b->y);
        if (d < bestd) { bestd = d; bestA = a; }

        a = a->next;
        if (poly_ferme && a == head) break;
    }

    return bestA;
}

/* ---------------------------- Redraw scene ---------------------------- */

static void redraw_scene(void) {
    I_fill(img, COL_BG);

    if (!head) return;

    if (remplissage && poly_ferme) {
        fill_scanline();
    }
    Vertex* v = head;
    while (v) {
        Vertex* w = v->next;
        if (!w) break;

        Color c = COL_EDGE;
        if (mode == MODE_EDGE && selectedE_A == v) c = COL_EDGE_SEL;

        draw_line_bresenham(v->x, v->y, w->x, w->y, c);

        v = v->next;
        if (poly_ferme && v == head) break;
    }
    if (mode == MODE_VERTEX) draw_selected_vertex_marker(selectedV);
}

/* ---------------------------- Callbacks GLUT ---------------------------- */

void display_CB(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    redraw_scene();
    I_draw(img);
    draw_texte(10, img->_height-20,mode_texte());
    glutSwapBuffers();
}

void mouse_CB(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) { glutPostRedisplay(); return; }

    int xi = x;
    int yi = img->_height - 1 - y;


    if (button == GLUT_LEFT_BUTTON) {
        if (mode == MODE_APPEND) {
            append_vertex(xi, yi);
            if (!selectedV) selectedV = tail ? tail : head;
            if (!selectedE_A) selectedE_A = head;
        } else if (mode == MODE_VERTEX) {
            selectedV = closest_vertex(xi, yi);
        } else if (mode == MODE_EDGE) {
            selectedE_A = closest_edge(xi, yi);
        }
    }

    if (button == GLUT_MIDDLE_BUTTON) {
        if (mode == MODE_EDGE && selectedE_A && selectedE_A->next) {
            // on coupe l'arête en 2 et insere un sommet au milieu
            Vertex* a = selectedE_A;
            Vertex* b = a->next;

            int mx = (a->x + b->x) / 2;
            int my = (a->y + b->y) / 2;

            Vertex* nv = V_new(mx, my);
            insert_vertex(a, nv);

            // apres insertion on peut selectionner le nouveau sommet
            selectedV = nv;
            selectedE_A = a;
        }
    }

    glutPostRedisplay();
}

void keyboard_CB(unsigned char key, int x, int y) {
    (void)x; (void)y;

    switch (key) {
        case 27: exit(0); break;

        case 'a': mode = MODE_APPEND; break;
        case 'v': mode = MODE_VERTEX; break;
        case 'e': mode = MODE_EDGE; break;
		case 'i':
		case 'I':
			if (mode == MODE_EDGE && selectedE_A && selectedE_A->next) {
				Vertex* a = selectedE_A;
				Vertex* b = a->next;

				int mx = (a->x + b->x) / 2;
				int my = (a->y + b->y) / 2;

				Vertex* nv = V_new(mx, my);
				insert_vertex(a, nv);

				selectedV = nv;
				selectedE_A = nv; // maintenant l'arrete selectionnee devient (nv -> b)
			}
			break;

        case 'c': {
			if (!head) break;
			int n = V_count();
		
			if (!poly_ferme) {
				if (n >= 3) {
					poly_ferme = 1;
					update_circular_links();
				}
			} else {
				poly_ferme = 0;
		
				if (head && head->prev) {
					tail = head->prev;
					head->prev = NULL;
					tail->next = NULL;
				}
		
				remplissage = 0;
			}
		} break;

        case 'f': {
            if (poly_ferme && V_count() >= 3) remplissage = !remplissage;
        } break;

        case 8:      // backspace
		case 'x':    // touche de secours si backspace nn reconnu
		case 'X':
			if (mode == MODE_VERTEX && selectedV) {
				remove_vertex(selectedV);
				if (V_count() < 3 && poly_ferme) {
					poly_ferme = 0;
					update_circular_links();
				}
			}break;
        case 127: {
            if (mode == MODE_VERTEX && selectedV) {
                remove_vertex(selectedV);
                if (V_count() < 3 && poly_ferme) { poly_ferme = 0; update_circular_links(); }
            }
        } break;

        default:
            break;
    }

    glutPostRedisplay();
}

void special_CB(int key, int x, int y) {
    (void)x; (void)y;

    int step = 5;

    switch (key) {
        // deplacement du sommet selectionne
        case GLUT_KEY_UP:
            if (mode == MODE_VERTEX && selectedV) selectedV->y += step;
            break;
        case GLUT_KEY_DOWN:
            if (mode == MODE_VERTEX && selectedV) selectedV->y -= step;
            break;
        case GLUT_KEY_LEFT:
            if (mode == MODE_VERTEX && selectedV) selectedV->x -= step;
            break;
        case GLUT_KEY_RIGHT:
            if (mode == MODE_VERTEX && selectedV) selectedV->x += step;
            break;

        case GLUT_KEY_PAGE_UP:
            if (mode == MODE_VERTEX && selectedV) {
                if (selectedV->prev) selectedV = selectedV->prev;
                else if (!poly_ferme) selectedV = tail;
            } else if (mode == MODE_EDGE && selectedE_A) {
                if (selectedE_A->prev) selectedE_A = selectedE_A->prev;
                else if (!poly_ferme) selectedE_A = tail ? tail->prev : head;
            }
            break;

        case GLUT_KEY_PAGE_DOWN:
            if (mode == MODE_VERTEX && selectedV) {
                if (selectedV->next) selectedV = selectedV->next;
                else if (!poly_ferme) selectedV = head;
            } else if (mode == MODE_EDGE && selectedE_A) {
                if (selectedE_A->next && (poly_ferme || selectedE_A->next->next)) selectedE_A = selectedE_A->next;
                else if (!poly_ferme) selectedE_A = head;
            }
            break;

        default:
            break;
    }

    glutPostRedisplay();
}

/* ---------------------------- Main ---------------------------- */

int main(int argc, char **argv) {
    if ((argc != 3) && (argc != 2)) {
        fprintf(stderr,"\n\nUsage \t: %s <width> <height>\nou", argv[0]);
        fprintf(stderr,"\t: %s <ppmfilename> \n\n", argv[0]);
        exit(1);
    }

    int largeur, hauteur;
    if (argc == 2) {
        img = I_read(argv[1]);
        largeur = img->_width;
        hauteur = img->_height;
    } else {
        largeur = atoi(argv[1]);
        hauteur = atoi(argv[2]);
        img = I_new(largeur, hauteur);
    }

    COL_BG        = C_new(0.05f, 0.05f, 0.08f);
    COL_EDGE      = C_new(0.95f, 0.95f, 0.95f);
    COL_EDGE_SEL  = C_new(1.00f, 0.30f, 0.20f);
    COL_VERTEX_SEL= C_new(0.20f, 1.00f, 0.20f);
    COL_FILL      = C_new(1.f, 0.f, 1.f);

    int windowPosX = 100, windowPosY = 100;

    glutInitWindowSize(largeur, hauteur);
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInit(&argc, argv);
    glutCreateWindow(argv[0]);

    glViewport(0, 0, largeur, hauteur);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, largeur, 0, hauteur, -1, 1);

    glutDisplayFunc(display_CB);
    glutKeyboardFunc(keyboard_CB);
    glutSpecialFunc(special_CB);
    glutMouseFunc(mouse_CB);

    glutMainLoop();
    return 0;
}
