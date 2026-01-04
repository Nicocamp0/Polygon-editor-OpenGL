/*====================================================*\
  Arash HABIBI
  Image.c
\*====================================================*/

#include "Image.h"

//------------------------------------------------------------------------

Color C_new(float red, float green, float blue)
{
	Color c;
	c._red = red;
	c._green = green;
	c._blue = blue;
	return c;
}

//------------------------------------------------------------------------

void C_check(Color c, char *message)
{
	fprintf(stderr,"%s : %f %f %f\n",message,c._red,c._green,c._blue);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------

Image* I_new(int width, int height)
{
	Image *img_new = (Image*)malloc(sizeof(Image));
	img_new->_width = width;
	img_new->_height = height;
	img_new->_xzoom = 0;
	img_new->_yzoom = 0;
	img_new->_zoom = 1.0;

	img_new->_xoffset=0;
	img_new->_yoffset=0;

	img_new->_current_color = C_new(255,255,255);

	img_new->_buffer = (Color**)calloc(width,sizeof(Color*));

	int x;
	for(x=0;x<width;x++)
		img_new->_buffer[x] = (Color*)calloc(height,sizeof(Color));

	return img_new;
}

//------------------------------------------------------------------------

static void _plot(Image *img, int x, int y, Color c)
{
	img->_buffer[x][y] = c;
}

//-----

static int _isPpm(char *imagefilename)
{
	FILE *imagefile;
	imagefile = fopen(imagefilename,"r");
	if(imagefile==NULL) {perror(imagefilename); exit(1); }

	else
	{
		int c1 = fgetc(imagefile);
		int c2 = fgetc(imagefile);
		fclose(imagefile);

		if((c1=='P')&&(c2=='6'))	return 1;
		else						return 0;
	}
}

//-----

Image* I_read(char *imagefilename)
{
	Image *img;
	char command[100];

	if(_isPpm(imagefilename))	sprintf(command,"cp %s input.ppm",imagefilename);
	else					sprintf(command,"convert %s input.ppm",imagefilename);

	int stat = system(command);
	if(stat!=0)
	{
		fprintf(stderr,"Convert : %s -> input.ppm impossible conversion.\n", imagefilename);
		exit(1);
	}
	else
	{
		Ppm ppm = PPM_nouv("input.ppm", PPM_LECTURE);
		system("rm input.ppm");

		fprintf(stderr,"%d x %d\n",PPM_largeur(ppm),PPM_hauteur(ppm));

		if(ppm!=NULL)
		{
			img = I_new(PPM_largeur(ppm),PPM_hauteur(ppm));
			int nb_bits=ppm->_nb_bits;
			int valmax = ppm->_valmax;

			int nb_pixels = img->_width*img->_height;

			if(nb_bits <= 8)
			{
				unsigned char *donnees = (unsigned char*)calloc(3*nb_pixels,sizeof(unsigned char));
				PPM_lectureDonneesChar(ppm, donnees);

				int x,y;
				for(y=0;y<img->_height;y++)
					for(x=0;x<img->_width;x++)
					{
						int indice = (img->_height-y)*img->_width + x;
						Color c = C_new((1.0*donnees[3*indice  ])/valmax,
										(1.0*donnees[3*indice+1])/valmax,
										(1.0*donnees[3*indice+2])/valmax);
						_plot(img,x,y,c);
					}
			}
			else
			{
				unsigned short *donnees = (unsigned short*)calloc(3*nb_pixels,sizeof(unsigned short));
				PPM_lectureDonneesShort(ppm, donnees);
				int x,y;
				for(y=0;y<img->_height;y++)
					for(x=0;x<img->_width;x++)
					{
						int indice = (img->_height-y)*img->_width + x;
						Color c = C_new((1.0*donnees[3*indice  ])/valmax,
										(1.0*donnees[3*indice+1])/valmax,
										(1.0*donnees[3*indice+2])/valmax);
						img->_buffer[x][y] = c;
					}
			}
			PPM_fermeture(ppm);
			return(img);
		}
		else
			return(NULL);
	}
}

//------------------------------------------------------------------------

void I_fill(Image *img, Color c)
{
	int x,y;
	for(x=0;x<img->_width;x++)
		for(y=0;y<img->_height;y++)
			img->_buffer[x][y]=c;
}

//------------------------------------------------------------------------

void I_checker(Image *img, Color c1, Color c2, int step)
{
	int x,y;
	for(x=0;x<img->_width;x++)
		for(y=0;y<img->_height;y++)
		{
			int n_x = x/step;
			int n_y = y/step;
			if((n_x+n_y)%2==0)	_plot(img,x,y,c1);
			else				_plot(img,x,y,c2);
		}
}

//------------------------------------------------------------------------

void I_changeColor(Image *img, Color c)
{
	img->_current_color = c;
}

//------------------------------------------------------------------------

void I_plot(Image *img, int x, int y)
{
	if((x>=0)&&(x<img->_width)&&
	   (y>=0)&&(y<img->_height))
		img->_buffer[x][y] = img->_current_color;
	else
	{
		fprintf(stderr,"I_plot : ERROR !!!\n");
		fprintf(stderr,"x (=%d) must be in the [%d,%d] range and\n", x, 0, img->_width);
		fprintf(stderr,"y (=%d) must be in the [%d,%d] range\n", y, 0, img->_height);
	}
}

//------------------------------------------------------------------------

void I_plotColor(Image *img, int x, int y, Color c)
{
	if((x>=0)&&(x<img->_width)&&
	   (y>=0)&&(y<img->_height))
		img->_buffer[x][y] = c;
	else
	{
		fprintf(stderr,"I_plotColor : ERROR !!!\n");
		fprintf(stderr,"x (=%d) must be in the [%d,%d] range and\n", x, 0, img->_width);
		fprintf(stderr,"y (=%d) must be in the [%d,%d] range\n", y, 0, img->_height);
	}
}

//------------------------------------------------------------------------
// Changement de repère

static void _windowToImage(Image *img, int xwin, int ywin, int *ximg, int *yimg)
{

	*ximg = img->_xoffset + img->_xzoom + (xwin-img->_xzoom) / img->_zoom;
	*yimg = img->_yoffset + img->_yzoom + (ywin-img->_yzoom) / img->_zoom;
}

//-----
// Changement de repère inverse
/*
static void _imageToWindow(Image *img, int ximg, int yimg, int *xwin, int *ywin)
{

	*xwin = img->_xoffset + img->_xzoom + (ximg-img->_xzoom-img->_xoffset) * img->_zoom;
	*ywin = img->_yoffset + img->_yzoom + (yimg-img->_yzoom-img->_yoffset) * img->_zoom;
}
*/
//-----

void I_focusPoint(Image *img, int xwin, int ywin)
{
	int dx = xwin - img->_xzoom;
	int dy = ywin - img->_yzoom;
	img->_xoffset -= dx*(1-1.0/img->_zoom);
	img->_yoffset -= dy*(1-1.0/img->_zoom);
	img->_xzoom = xwin;
	img->_yzoom = ywin;
}

//------------------------------------------------------------------------

void I_zoomInit(Image *img)
{
	img->_xoffset = 0;
	img->_yoffset = 0;
	img->_zoom = 1.0;
}

//------------------------------------------------------------------------

void I_zoom(Image *img, double zoom_coef)
{
	img->_zoom = img->_zoom * zoom_coef;
}

//------------------------------------------------------------------------

void I_move(Image *img, int x, int y)
{
	img->_xoffset += x;
	img->_yoffset += y;
}

//------------------------------------------------------------------------

void I_draw(Image *img)
{
	glBegin(GL_POINTS);
	int xwin, ywin, ximg, yimg;
	for(xwin=0;xwin<img->_width;xwin++)
		for(ywin=0;ywin<img->_height;ywin++)
		{
			_windowToImage(img, xwin, ywin, &ximg, &yimg);
			Color c;
			if((ximg>=0)&&(ximg<img->_width)&&
			   (yimg>=0)&&(yimg<img->_height))
				c = img->_buffer[ximg][yimg];
			else
				c = C_new(0,0,0);

			glColor3f(c._red,c._green,c._blue);
			glVertex2i(xwin,ywin);
		}
	glEnd();
}

//------------------------------------------------------------------------





