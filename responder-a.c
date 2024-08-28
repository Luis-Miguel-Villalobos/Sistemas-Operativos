/**
 * El programa exige que la variable de entorno LOGNAME esté correctamente
 * inicializada con el nombre del usuario. Esto se puede conseguir incluyendo las dos
 * líneas siguientes en el fichero .profile de configuración de nuestro intérprete de
 * órdenes:
 *       LOGNAME=’logname’
 *       export LOGNAME
 */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define MAX 256
/* Macro para comparar dos cadenas de caracteres. */
#define EQ(str1, str2) (strcmp ((str1), (str2)) == 0)
/* Descriptores de fichero de las tuberías mediante las que nos vamos a comunicar. */
int fifo_12, fifo_21;
char nombre_fifo_12 [MAX], nombre_fifo_21 [MAX];
/* Array para leer los mensajes. */
char mensaje [MAX];

main (int argc, char *argv [])
{
    char *logname, *getenv ();
    void fin_de_transmision ();

    /* Análisis de los argumentos de la línea de órdenes. */
    if (argc != 2) {
    fprintf (stderr, "Forma de uso: %s usuario\n", argv [0]);
    exit (-1);
    }

    /* Lectura del nombre del usuario. */
    logname = getenv ("LOGNAME");
    /* Comprobación para que un usuario no se responda a sí mismo. */
    if (EQ (logname, argv [1])) {
        fprintf (stderr, "Comunicación con uno mismo no permitida\n");
        exit (0);
    }

    /* Formación del nombre de las tuberías de comunicación. */
    sprintf (nombre_fifo_12, "/tmp/%s_%s", argv[1], logname);
    sprintf (nombre_fifo_21, "/tmp/%s_%s", logname, argv[1]);

    /* Apertura de las tuberías. */
    if ((fifo_12 = open (nombre_fifo_12, O_RDONLY)) ==-1 ||
        (fifo_21 = open (nombre_fifo_21, O_WRONLY)) ==-1) {
        perror (nombre_fifo_21);
        exit (-1);
    }

    /* Armamos el manejador de la señal SIGINT. */
    signal (SIGINT, fin_de_transmision);

    /* Bucle de recepción de mensajes. */
    do {
        printf ("==> "); fflush (stdout);
        read (fifo_12, mensaje, MAX);
        printf ("%s", mensaje);
        if (EQ(mensaje, "cambio\n"))
            do {
                printf ("<== ");
                fgets (mensaje, sizeof (mensaje), stdin);
                write (fifo_21, mensaje, strlen(mensaje) + 1);
            } while (!EQ(mensaje,"cambio\n") && !EQ(mensaje,"corto\n"));
    } while (!EQ(mensaje, "corto\n"));
    printf ("FIN DE TRANSMISIÓN.\n");
    close (fifo_12);
    close (fifo_21);
    exit (0);
}

/**
 *  fin_de_transmision: rutina de tratamiento de la señal SIGINT.AlpulsarCtrl+C
 * entramos en esta rutina, que se encarga de enviar el mensaje "corto\n" al usuario
 * con el que estamos hablando.
 */
void fin_de_transmision (int sig)
{
    sprintf (mensaje, "corto\n");
    write (fifo_21, mensaje, strlen(mensaje) + 1);
    printf ("FIN DE TRANSMISIÓN.\n");
    close (fifo_12);
    close (fifo_21);
    exit (0);
}