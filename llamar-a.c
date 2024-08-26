/**
 *El programa exige que la variable de entorno LOGNAME esté correctamente
 *inicializada con el nombre del usuario. Esto se puede conseguir incluyendo las dos
 *líneas siguientes en el fichero .profile de configuración de nuestro intérprete de órdenes:
 *      LOGNAME=’logname’
 *      export LOGNAME
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utmp.h"
#define MAX 256

/*Macro para comparar dos cadenas de caracteres. */
#define EQ(str1, str2) (strcmp ((str1), (str2)) == 0)

/*Descriptores de fichero de las tuberías con nombre mediante las cuales vamos a
comunicarnos. */
int fifo_12, fifo_21;
char nombre_fifo_12[MAX], nombre_fifo_21[MAX];

/* Array para leer los mensajes. */
char mensaje [MAX];

main (int argc, char *argv []){
 	int tty;
 	char terminal [MAX], *logname, *getenv ();
 	struct utmp *utmp, *getutent ();
 	void fin_de_transmision ();
	/* Análisis de los argumentos de la línea de órdenes. */
 	if (argc != 2) {
	          fprintf (stderr, "Forma de uso: %s usuario\n", argv [0]);
	          exit (-1);
 	}
	/* Lectura de nuestro nombre de usuario. */
 	logname = getenv ("LOGNAME");

	/* Comprobación para que un usuario no se llame a sí mismo. */
 	if (EQ (logname, argv [1])) {
 	        fprintf (stderr, "Comunicación con uno mismo no permitida\n");
	    exit (0);
 	}

	/* Consultamos si el usuario ha iniciado una sesión. */
 	while ((utmp = getutent ()) != NULL &&
	    strncmp (utmp->ut_user, argv [1], sizeof (utmp->ut_user)) != 0);
    if (utmp == NULL) {
        printf ("EL USUARIO %s NO HA INICIADO SESIÓN.\n", argv [1]);
        exit (0);
    }
    /* Formación de los nombres de las tuberías de comunicación. */
    sprintf (nombre_fifo_12, "/tmp/%s_%s", logname, argv[1]);
    sprintf (nombre_fifo_21, "/tmp/%s_%s", argv[1], logname);
    /* Creación y apertura de las tuberías. */
    /* Primero borramos las tuberías, para que la llamada a mknod no falle. */
    unlink (nombre_fifo_12);
    unlink (nombre_fifo_21);
    /* Cambiamos la máscara de permisos por defecto para este proceso. Esto
    permitirá crear las tuberías con los permisos rw-rw-rw-=0666.*/
    umask (~0666);
    /* Creamos las tuberías para que la llamada a open no falle. */
    if (mkfifo (nombre_fifo_12, 0666) ==-1) {
        perror (nombre_fifo_12);
        exit (-1);
    }
    if (mkfifo (nombre_fifo_21, 0666) ==-1) {
        perror (nombre_fifo_21);
        exit (-1);
    }

    /* Apertura del terminal del usuario. */
    sprintf (terminal, "/dev/%s", utmp->ut_line);
    if ((tty = open (terminal, O_WRONLY)) ==-1) {
        perror (terminal);
        exit (-1);
    }

    /* Aviso al usuario con el que queremos comunicarnos. */
    sprintf (mensaje,
        "\n\t\tLLAMADA PROCEDENTE DEL USUARIO %s\07\07\07\n"
        "\t\tRESPONDER ESCRIBIENDO: responder-a %s\n\n",
        logname, logname);
    write (tty, mensaje, strlen (mensaje) + 1);
    close (tty);

    printf ("Esperando respuesta...\n");
    /* Apertura de las tuberías. Una de ellas para escribir mensajes y la otra
    para leerlos. */
    if ((fifo_12 = open (nombre_fifo_12, O_WRONLY)) ==-1 ||
        (fifo_21 = open (nombre_fifo_21, O_RDONLY)) ==-1) {
        if (fifo_12 ==-1)
            perror (nombre_fifo_12);
        else
            perror (nombre_fifo_21);
        exit (-1);
    }
    /* A este punto llegamos cuando nuestro interlocutor responde a nuestra
    llamada. */
    printf ("LLAMADA ATENDIDA. \07\07\07\n");

    /* Armamos el manejador de la señal SIGINT. Esta señal se genera al
    pulsar Ctrl+C. */
    signal (SIGINT, fin_de_transmision);
    /* Bucle de envío de mensajes. */
    do {
        printf ("<== ");
        fgets (mensaje, sizeof (mensaje), stdin);
        write (fifo_12, mensaje, strlen(mensaje) + 1);
        /* Bucle de recepción de mensajes. */
        if (EQ(mensaje, "cambio\n"))
            do {
                printf ("==> "); fflush (stdout);
                read (fifo_21, mensaje, MAX);
                printf ("%s", mensaje);
            } while (!EQ(mensaje,"cambio\n") && !EQ(mensaje,"corto\n"));
    } while (!EQ(mensaje, "corto\n"));
        
    /* Fin de la transmisión. */
    printf ("FIN DE TRANSMISIÓN.\n");
    close (fifo_12);
    close (fifo_21);

    exit (0);
}