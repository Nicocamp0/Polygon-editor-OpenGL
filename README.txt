/*====================================================*\
  Arash HABIBI
  README.txt
\*====================================================*/

Le programme plot permet de créer une image vierge et
d'y colorier les pixels ou, éventuellement, de partir
d'une image préexistant et de la modifier pixel par pixel.

Pour compiler le programme :

     make

Pour travailler sur une image 800 x 600 vierge (noire) :

     plot 800 600

Pour travailler sur une image préexistant dans un fichier au format ppm :

     plot image2.ppm


Ce programme permet également d'examiner l'image en zoomant sur
un détail en particulier. Pour cela, cliquer sur le détail en
question et presser la touche 'z' (comme zoom) autant de fois que
nécessaire. Pour s'éloigner, presser la touche 'Z' et pour revenir
à l'état initial, presser 'i'.

Par ailleurs, les fonctions qui sont à votre disposition pour modifier
les images, sont dans le fichier Image.h. En particulier :

// Pour créer une image vierge
Image* I_new(int _width, int _height);

// Pour créer une image à partir d'un fichier ppm existant :
Image* I_read(char *ppmfilename);

// Pour remplir l'image avec la couleur c
void I_fill       (Image *img, Color c);

// Pour remplir l'image avec un damier dont les cases font step pixels
// et sont de couleurs c1 et c2 :
void I_checker    (Image *img, Color c1, Color c2, int step);

// Change la couleur courante
void I_changeColor(Image *img, Color c);

// Colorie le pixel x,y avec la couleur courante
void I_plot       (Image *img, int x, int y);

// Colorie le pixel x,y avec la couleur c sans changer la couleur courante
void I_plotColor  (Image *img, int x, int y, Color c);
