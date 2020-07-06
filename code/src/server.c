#include "server.h"

struct th_sock{
  int sock;
  struct sockaddr_in cli;
};

struct req_arg
{
	int *f_des;
	struct th_sock cli_info;
	char *req_file;
};
int sc;				/*socket de connextion*/
int *scom;			/*socket de communication*/
int connec_count = 0;		/*compteur de connexion*/
pthread_mutex_t mutex_mime = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;

/*traitement associé au SIGALRM*/
void handle_alarm (int sig_num)
{
	kill (getpid(), SIGKILL);
}

/* renvoie le type mime correspondant au type de fichier passé en parametre */
const char *getmimetype(char * dest, const char *filetype)
{

	FILE *mfd;
	char buf[BUFF_SIZE];
	char *mimetype = malloc (100*sizeof(char));
	char *extensions;
	char *cur_ext;
	if((mfd = fopen("/etc/mime.types", "r")) == NULL)
	{
		return NULL;
	}

	memset(buf, 0, BUFF_SIZE);
	while(fgets(buf, BUFF_SIZE, mfd) != NULL)
	{
		if(buf[0] == '#' || buf[0] == '\n' || (mimetype = strtok(buf, "\t")) == NULL)
		{
			continue;
		}
		extensions = strtok(NULL, "\t");
		if(extensions == NULL)
			continue;
		cur_ext = strtok(extensions, " \n");
		do
		{
			if(strcmp(cur_ext, filetype) == 0)
			{
				break;
			}
			cur_ext = strtok(NULL, " \n");
		}while(cur_ext != NULL);
		if(cur_ext != NULL)
			break;
	}
	strcpy(dest, mimetype);
	return mimetype;
}

void *rq_func_thread (void *arg)
{
	char ext[5];
	char *type_fic;
	int fd;
	int scom = (((struct req_arg *) arg)->cli_info).sock;
	char *fichier;
	char buffer[BUFF_SIZE];
	int m;
	struct stat st;
	fd = *(((struct req_arg *)arg)->f_des);
	fichier = malloc (strlen(((struct req_arg *)arg)->req_file) * sizeof (char));
	strcpy (fichier, ((struct req_arg *)arg)->req_file);

	stat (fichier, &st);
	

	/*Fichier est une extension*/
	if (strstr (fichier, ".") != NULL)
	{
		type_fic = strrchr(fichier, '.');
		type_fic++;
		memcpy(ext, type_fic, strlen(type_fic));


		char *res = malloc(100*sizeof(char));
		pthread_mutex_lock(&mutex_mime);
		getmimetype(res, ext);
		pthread_mutex_unlock(&mutex_mime);
		sprintf(buffer, "Content-Type: %s\nContent-Length: %u\n\n", res, (int)st.st_size);
		/*enoyer "Content-Type, Content-Length"*/
		if(write(scom, buffer, strlen(buffer)) == -1)
		{
			perror("write");
			pthread_exit((void *) 0);
		}
	}
	lseek (fd, SEEK_SET, 0);
	/*lire le fichier et puis envoyer son contenu au client*/
	while ((m = read (fd, buffer, BUFF_SIZE)) > 0)
	{
		if ((write (scom, buffer, m)) == -1)
		{
			perror ("write \n");
			exit (1);	
		}
	}

	free(arg);
	pthread_exit((void *) 0);
}

void *cn_func_thread (void *arg)
{
	char buffer[BUFF_SIZE];
	int n, m;
	pthread_t *tid;
	struct req_arg *rq_arg_th;
	int scom = ((struct th_sock *) arg)->sock;
	char *fichier_tmp;
	int fd;
	struct sockaddr_in cli = ((struct th_sock *) arg)->cli;
	int rep_code;
	time_t curr_time;
	
	n = read (scom, buffer, BUFF_SIZE);
	while (n != 0)
	{
		char *frst_rq_line; 
		frst_rq_line = malloc (strlen(strtok (buffer, "\n")) * sizeof (char));
		strcpy (frst_rq_line, buffer);
		frst_rq_line[strlen (frst_rq_line) - 1] = '\0';
		fichier_tmp = strtok (buffer, " ");
		fichier_tmp = strtok (NULL, " ");
		fichier_tmp++;
		char *fichier;
		fichier = malloc (strlen(fichier_tmp)*sizeof(char));
		strcpy (fichier, fichier_tmp);
		if ((fd = open (fichier, O_RDONLY)) == -1)
		{
			perror ("open");
			exit (EXIT_FAILURE);
		}
		struct stat st;
		memset(buffer, 0, BUFF_SIZE);
		if ((fd != -1) | ((stat (fichier, &st) == 0) && (st.st_mode & S_IXUSR)))
		{
			rep_code = 200;    
			sprintf(buffer, "HTTP/1.1 200 OK\n");
		}
		else
		{
			if (errno == EACCES){
				rep_code = 403;
				sprintf(buffer, "HTTP/1.1 403 Forbidden\n");
			}
			else if (errno == ENOENT)
			{
				rep_code = 404;
				sprintf(buffer, "HTTP/1.1 404 Not Found\n");
			}
		}

		/*enoyer "HTTP/1.1 nnn message"*/
		if(write(scom, buffer, strlen(buffer)) == -1)
		{
			perror("write");
			pthread_exit((void *) 0);
		}

		/*récupérer la date de la requête*/	
		time (&curr_time);
		char *rq_date = malloc ((strlen(ctime (&curr_time))) * sizeof (char));
		strcpy (rq_date, ctime (&curr_time));
		rq_date[strlen (rq_date) - 1] = '\0';

		if(rep_code == 200)
		{
			if (st.st_mode & S_IXUSR)
			{
				/*s'il s'agit d'un exécutable*/
				/*on va créer un processus fils*/	
				/*le tube pour communiquer entre fils et père*/
				int out_p[2];
				pipe (out_p);

				int chld_pid = fork();
				if (chld_pid == 0)
				{
					/*fils*/
					int exe_err;

					/*rediriger le sortie standard vers le tube*/
					close (1);
					dup2 (out_p[1], 1);
					close (out_p[0]);

					signal (SIGALRM, handle_alarm );
					alarm (10);
					
					/*exécuter l'exécutable*/
					exe_err = execl (fichier, fichier, NULL);

					exit (exe_err);
				}
				else
				{
					/*père*/
					int status;
					char out_buff[BUFF_SIZE];

					/*fichier contenant les flots de sorties du fils*/
					int cat_fl_des;
					int rep_size;
					char name_f_buff[BUFF_SIZE];
					sprintf(name_f_buff, "%s_%d", fichier, chld_pid);

					cat_fl_des = open (name_f_buff, O_CREAT | O_RDWR | O_TRUNC, 0700);
					close (out_p[1]);

					/*lire le tube et puis écrire dans le fichier*/
					while (waitpid (chld_pid, &status, WNOHANG) == 0)
					{
						m = read (out_p[0], out_buff, BUFF_SIZE);
						if (strlen (out_buff) != 0)
						{
							write (cat_fl_des, out_buff, m);	
						}
					}
		
					stat (name_f_buff, &st);
				
					/*envoyer le longeur de la réponse au client*/
						
					if (WIFSIGNALED (status) || (status != 0))
					{
						rep_size = strlen ("HTTP/1.1 500 Internal Server Error\n");
						sprintf(buffer, "Content-Type:  \nContent-Length: %d\n\n", rep_size );
					}
					if (status == 0)
					{
						sprintf(buffer, "Content-Type:  \nContent-Length: %d\n\n", (int)st.st_size);
					}
					if(write(scom, buffer, strlen(buffer)) == -1)
					{
						perror("write");
						pthread_exit((void *) 0);
					}

					memset (buffer, 0, strlen(buffer));
					lseek (cat_fl_des, SEEK_SET, 0);
	
					/*l'exécution ne s'est pas terminé au bout de 10 seconds*/
					if (WIFSIGNALED (status) || (status != 0))
					{
						rep_code = status;
						sprintf(buffer, "HTTP/1.1 500 Internal Server Error\n");
						if(write(scom, buffer, strlen(buffer)) == -1)
						{
							perror("write");
							pthread_exit((void *) 0);
						}
						/*journalisation*/
						pthread_mutex_lock(&log_mtx);
						FILE *log_f = fopen ("/tmp/http3261765.log", "a+");
						fprintf (log_f, "%s %s %d %d %s %d %d \n", inet_ntoa (cli.sin_addr), rq_date, chld_pid, chld_pid, frst_rq_line, rep_code, rep_size);
						fclose (log_f);
						pthread_mutex_unlock(&log_mtx);
					}
					
					/*l'exécution s'arrête avec la valeur de retou 0*/
					if (status == 0)
					{
						/*envoyer la réponse capturéé dans le fichier*/
						/* et puis l'envoyer au client*/
						while ((m = read (cat_fl_des, buffer, BUFF_SIZE)) > 0)
						{
							if(write(scom, buffer, strlen(buffer)) == -1)
							{
								perror("write");
								pthread_exit((void *) 0);
							}
						}
						/*journalisation*/
						pthread_mutex_lock(&log_mtx);
						FILE *log_f = fopen ("/tmp/http3261765.log", "a+");
						fprintf (log_f, "%s %s %d %d %s %d %d \n", inet_ntoa (cli.sin_addr), rq_date, chld_pid, chld_pid, frst_rq_line, rep_code, (int)st.st_size);
						fclose (log_f);
						pthread_mutex_unlock(&log_mtx);
					}

					/*libère la ressource*/
					close (cat_fl_des);
					unlink (name_f_buff);
					free (fichier);
					

					free (rq_date);
				}
			}
			else
			{
				/*s'il ne s'agit pas d'un exécutable*/
				/*on va créer une thread*/
				tid = malloc (sizeof (pthread_t));
			
				/*journalisation*/	
				pthread_mutex_lock(&log_mtx);
				FILE *log_f = fopen ("/tmp/http3261765.log", "a+");
				fprintf (log_f, "%s %s %d %d %s %d %d \n", inet_ntoa (cli.sin_addr), rq_date, (int)getpid(), (int)*tid, frst_rq_line, rep_code, (int)st.st_size);
				fclose (log_f);
				pthread_mutex_unlock(&log_mtx);
				
				rq_arg_th = malloc (sizeof (struct req_arg));
				rq_arg_th->req_file = malloc (strlen(fichier) * sizeof (char));
				strcpy (rq_arg_th->req_file, fichier);
				rq_arg_th->f_des = malloc (sizeof (int));
				*(rq_arg_th->f_des) = fd;
				(rq_arg_th->cli_info).sock = scom;
				(rq_arg_th->cli_info).cli = ((struct th_sock *) arg)->cli;

				/*créer une thread qui va traiter la requête*/
				if (pthread_create (tid, NULL, rq_func_thread, rq_arg_th) != 0)
				{
					perror ("pthread_create");
					exit (1);
				}

				if (pthread_join (*tid, NULL) != 0)
				{
					perror ("pthread_join");
					exit (1);
				}


				/*libère des ressources*/
				free (rq_date);
			}
		}
			
		n = read (scom, buffer, BUFF_SIZE);
	}
	/*fermer la connextion*/	
	shutdown(scom, 2);

	/*liberer la resource*/
	pthread_mutex_lock(&count);
	connec_count++;
	pthread_mutex_unlock(&count);
	free (arg);
	pthread_exit((void *) 0);
}


int main (int argc, char *argv[])
{
	struct sockaddr_in sin;			/*nom de la socket de connection*/
	struct th_sock *exp;			/*nom de l'expediteur*/
	socklen_t fromlen = sizeof (exp);
	int connec_max = atoi (argv[2]);
	pthread_t *tid;

	if (argc != 4)
	{
		perror ("argc");
		exit (1);
	}

	/*creation de la socket*/
	if ((sc = socket (AF_INET, SOCK_STREAM, 0)) < 0){
	perror ("socket \n");
	exit (1);
	}

	/*remplir le nom*/
	memset ((char *)&sin, 0, sizeof (struct sockaddr_in));	
	sin.sin_family = AF_INET;
	sin.sin_port = htons (atoi (argv[1]));
	sin.sin_addr.s_addr = htonl (INADDR_ANY); 

	/*nommage*/
	if (bind (sc, (struct sockaddr *)&sin, sizeof (sin)) == -1){
	perror ("bind");
	exit (1);
	}

	/*attendre une demande*/
	listen (sc, MAX_PENDING);
	connec_count = connec_max;	
	while (1)
	{
		if(connec_count != 0)
		{

			exp = malloc (sizeof (struct th_sock));
			tid = malloc (sizeof (pthread_t));


			/*attente connection du client*/
			if ((exp->sock = accept (sc, (struct sockaddr *)&(exp->cli), &fromlen)) == -1){
				perror ("accept");
				exit (1);
			}
			/*ajouter mutex*/	
			pthread_mutex_lock(&count);
			connec_count--;
			pthread_mutex_unlock(&count);
			/*creation d'une thread pour une connection*/
			if (pthread_create (tid, NULL, cn_func_thread, exp) != 0){
				perror ("pthread_create");
				exit (1);
			}

		}
	  }
	  
	  pthread_mutex_destroy(&mutex_mime);
	  pthread_mutex_destroy(&count);
	  pthread_mutex_destroy(&log_mtx);
	  close (sc);
	  return EXIT_SUCCESS; 
}
