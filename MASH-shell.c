/*
 * Name: Nooreldean Koteb
 */

#include "logging.h"
#include "shell.h"

/* Constants */
static const char *shell_path[] = {"./", "/usr/bin/", NULL};
static const char *built_ins[] = 
			{"quit", "help", "kill", "jobs", "fg", "bg", NULL};

/* Feel free to define additional functions and update existing lines */
//Defining background and foreground states
#define FOREGROUND 0
#define BACKGROUND 1

//Job Struct - stores individual jobs
typedef struct job_struct{
    Cmd_aux *aux;
	char *cmd;
	char *state;
	int *status;
	pid_t pid;
    int job_id;
}Job;

//Jobs List Struct - stores a list of jobs in a double linked list
typedef struct job_list_struct{
    Job *job;
	struct job_list_struct *next;
	struct job_list_struct *prev; //Idk if I will need this yet
}Job_list;

//Global job list head
Job_list *current_jobs;

//Global Job Count
int job_count = 0;

//Prototypes
void parse(char *cmd_line, char *argv[], Cmd_aux *aux);
void built_in_cmd(char *argv[], Cmd_aux *aux);
void program_cmd(char *argv[], Cmd_aux *aux);
void display_jobs();
void add_fg_job(Job *job);
void add_bg_job(Job *job);
void remove_fg_job();
void remove_bg_job(Job *job);
Job *get_job(pid_t pid);
void fg_to_bg(int job_id, int type);
void bg_to_fg(int job_id, int type);
int next_job_id();
void handler(int sig);

/* main */
/* The entry of your shell program */
int main() {
    char cmdline[MAXLINE]; /* Command line */

    /* Intial Prompt and Welcome */
    log_prompt();
    log_help();

	//Initialize Current Jobs list
	current_jobs = NULL;

	//Handler Setup
	struct sigaction sig;
	memset(&sig, 0, sizeof(sig));
	sig.sa_handler = handler;
	sigaction(SIGINT, &sig, NULL); //2 ctrl-c
	sigaction(SIGTERM, &sig, NULL); //15
	sigaction(SIGCHLD, &sig, NULL); //17
	sigaction(SIGCONT, &sig, NULL); //18
	sigaction(SIGTSTP, &sig, NULL); //20 ctrl-z

    /* Shell looping here to accept user command and execute */
    while (1) {

        char *argv[MAXARGS]; /* Argument list */
        Cmd_aux aux; /* Auxilliary cmd info: check shell.h */
   
	/* Print prompt */
  	log_prompt();

	/* Read a line */
	// note: fgets will keep the ending '\n'
	if (fgets(cmdline, MAXLINE, stdin)==NULL)
	{
	   	if (errno == EINTR)
			continue;
	    	exit(-1); 
	}

	if (feof(stdin)) {
	    	exit(0);
	}

	/* Parse command line */
    	cmdline[strlen(cmdline)-1] = '\0';  /* remove trailing '\n' */
	parse(cmdline, argv, &aux);		

	/* Evaluate command */
	/* add your implementation here */

    } 
}
/* end main */

/* required function as your staring point; 
 * check shell.h for details
 */
//Parses the given cmd_line
void parse(char *cmd_line, char *argv[], Cmd_aux *aux){
	//Initializes Cmd_aux structure
	Cmd_aux *current_aux = (struct cmd_aux_struct*)malloc(sizeof(struct cmd_aux_struct));
	current_aux->in_file = (char *)malloc(sizeof(char)* MAXARGS);
	current_aux->in_file = NULL;
	current_aux->out_file = (char *)malloc(sizeof(char)* MAXARGS);
	current_aux->out_file = NULL;

	//Tokenizing cmd_line
	char *token = strtok(cmd_line, " ");
	
	//variable Initializations
	int arg_type = 0;
	int arg_size = 0;
	current_aux->is_append = -1;
	current_aux->is_bg = 0;
	
	//Parsing cmd_line
	while (token != NULL){ 
		//Checking for Special Charachters
		if (strcmp(token, "<") == 0){
			arg_type = 1;
		}else if (strcmp(token, ">") == 0){
			arg_type = 2;
		}else if (strcmp(token, ">>") == 0){
			arg_type = 3;
		}else if(strcmp(token, "&") == 0){
			current_aux->is_bg = 1;
		}else{
			//Initializing command values, and adding to argv
			switch (arg_type){
				case 0:
					argv[arg_size] = token;
					arg_size += 1;
					break;
				case 1:
					current_aux->in_file = token;
					arg_type = 0;
					break;
				case 2:
					current_aux->out_file = token;
					if (current_aux->is_append == -1){
						current_aux->is_append = 0;
					}
					arg_type = 0;
					break;
				case 3:
					current_aux->out_file = token;
					if (current_aux->is_append == -1 || current_aux->is_append == 0){
						current_aux->is_append = 1;
					}
					arg_type = 0;
					break;
			}
		}
		//Iterate through tokens
		token = strtok(NULL, " ");
	}
	//Indicating end of argv
	argv[arg_size] = NULL;
	
	//Checking if cmd_line is empty
	if (argv[0] == NULL){
		return;
	}

	//Checking if built-in command and running it
	int i = 0;
	while (built_ins[i] != NULL){
		if (strcmp(argv[0], built_ins[i]) == 0){
			built_in_cmd(argv, current_aux);
			return;
		}
		i++;
	}

	//Running program command
	program_cmd(argv, current_aux);
}

//Runs built in cmds
void built_in_cmd(char *argv[], Cmd_aux *aux){
	//Help command
	if (strcmp(argv[0], "help") == 0){
		log_help();

	//Quit command
	}else if (strcmp(argv[0], "quit") == 0){
		log_quit();
		exit(0);

	//Jobs command
	}else if (strcmp(argv[0], "jobs") == 0){
		log_job_number(job_count);// # of jobs
		display_jobs();

	//Kill command
	}else if (strcmp(argv[0], "kill") == 0){
		kill((int)strtol(argv[2], (char **)NULL, 10), (int)strtol(argv[1], (char **)NULL, 10));
		log_kill((int)strtol(argv[1], (char **)NULL, 10), (int)strtol(argv[2], (char **)NULL, 10));

	//Background to foreground command
	}else if (strcmp(argv[0], "fg") == 0){
		bg_to_fg((int)strtol(argv[1], (char **)NULL, 10), 0);

		
	//Foreground to background command
	}else if (strcmp(argv[0], "bg") == 0){
		fg_to_bg((int)strtol(argv[1], (char **)NULL, 10), 0);
	}
}

//Runs programs and file redirection
void program_cmd(char *argv[], Cmd_aux *aux){
	//Creating job
	Job *current_job = (struct job_struct*)malloc(sizeof(struct job_struct)) ;
	
	//Sets Cmd_aux
	current_job->aux = aux;

	//concatenates cmd and saves it in job
	current_job->cmd = (char *)malloc(sizeof(char)* MAXARGS);
	int i = 0;
	while(argv[i] != NULL){
		strcat(current_job->cmd, argv[i]);
		strcat(current_job->cmd, " ");
		i++;
	}

	//Creates State
	current_job->state = (char *)malloc(sizeof(char) * 7);
	current_job->state = "Running";

	//Initializing Paths for incoming command
	char path_one[strlen(shell_path[0])+strlen(argv[0])];
	strcpy(path_one, shell_path[0]);
	char path_two[strlen(shell_path[1])+strlen(argv[0])];
	strcpy(path_two, shell_path[1]);
	//Concatenating cmd to the two paths
	strcat(path_one, argv[0]);
	strcat(path_two, argv[0]);

	//Running Program, Fork, and Execute, log Error if not found
	if ((current_job->pid = fork()) == 0){
		
		//File Redirection programs
		if (current_job->aux->in_file != NULL || current_job->aux->out_file != NULL){
			
			//Opens File
			int new_file = -1;
			if (current_job->aux->out_file != NULL){
				if (current_job->aux->is_append == 1){
					//Open Append file >>
					if( (new_file = open(current_job->aux->out_file, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1){
						log_file_open_error(current_job->aux->out_file);
					}
				}else{
					//Open write file >
					if( (new_file = open(current_job->aux->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
						log_file_open_error(current_job->aux->out_file);
					}
				}
			}
			
			//Duplicates in file into out file
			dup2(new_file, STDOUT_FILENO);

			//Runs program redirection, checking both paths
			if (execl(path_one, argv[0], current_job->aux->in_file, NULL) < 0){
				if (execl(path_two, argv[0], current_job->aux->in_file, NULL) < 0){
					log_command_error(argv[0]);
					return;
				}
			}

		}else{
			
			//Runs non-file redirection commands, checks both paths
			if (execv(path_one, argv) < 0){
				if (execv(path_two, argv) < 0){
					log_command_error(argv[0]);
					return;
				}
			}
		}

	}

	//bg and fg program options
	if (aux->is_bg == 1){
		current_job->job_id = next_job_id();
		strcat(current_job->cmd, "&");
		log_start_bg(current_job->pid, argv[0]);//pid, argv[0]

		//Adds job to Job list
		add_bg_job(current_job);
	}else{
		current_job->job_id = 0;
		log_start_fg(current_job->pid, argv[0]);//pid, argv[0]
		//Adds job to Job list
		add_fg_job(current_job);

		//Waits on foreground job to finish
		if (waitpid(-1, NULL, WCONTINUED) > 0){
			log_job_fg_term(current_job->pid, current_job->cmd);//fg
			remove_fg_job();
		}
	}
}

//Handles Signals
void handler(int sig){ 
	pid_t current_pid;

	//Switch case for 4 signals, SIGINT, SIGTSTP, SIGCHILD, SIGTERM
	switch (sig){
		//Ctrl-c
		case 2:
			//Terminates foreground program
			if (current_jobs != NULL && current_jobs->job->aux->is_bg == 0){
				log_ctrl_c();
				log_job_fg_term_sig(current_jobs->job->pid, current_jobs->job->cmd);
				remove_fg_job();
			}
			break;
		
		//Kill cmd
		case 15:
			//Terminates background command
			if ((current_pid = waitpid(-1, NULL, WNOHANG)) > 0){
				Job *current_job = get_job(current_pid);
				log_job_bg_term_sig(current_job->pid, current_job->cmd);//current_jobs->job->pid, current_jobs->job->cmd);
			}
			break;

		//Sigchild
		case 17:
			//terminates foreground and background programs that terminate naturally
			if ((current_pid = waitpid(-1, NULL, WNOHANG)) > 0){
				Job *current_job = get_job(current_pid);
				if (current_job != NULL){
					if (current_job->aux->is_bg == 1){
						log_job_bg_term(current_job->pid, current_job->cmd);//current_jobs->job->pid, current_jobs->job->cmd);//bg
						remove_bg_job(current_job);
					}else{
						log_job_fg_term(current_job->pid, current_job->cmd);//fg
						remove_fg_job();
					}
				}
			}

			//Stops programs in background and "foreground"(Doesnt work)
			if ((current_pid = waitpid(-1, NULL, WUNTRACED)) < -1){
				Job *current_job = get_job(current_pid);
				
				if (current_job != NULL){
					current_job->state = "Stopped";

					if (current_job->aux->is_bg == 1){
						log_job_bg_stopped(current_job->pid, current_job->cmd);//current_jobs->job->pid, current_jobs->job->cmd);//bg stopped
					}else{
						log_job_fg_stopped(current_job->pid, current_job->cmd);//current_jobs->job->pid, current_jobs->job->cmd);//bg stopped
					}

					fg_to_bg(current_job->job_id, 1);
				}
				
			}
			break;

		//Continue Not Working
		case 18:
			//"continues"(doesnt work) stopped command
			if ((current_pid = waitpid(-1, NULL, WNOHANG)) > -1){
				Job *current_job = get_job(current_pid);
				if (current_job != NULL){
					current_job->state = "Running";
					if (current_jobs->job->aux->is_bg == 1){
						log_job_bg_cont(current_job->pid, current_job->cmd);//current_jobs->job->pid, current_jobs->job->cmd);//bg
					}else{
						log_job_fg_cont(current_job->pid, current_job->cmd);//fg
						// if (waitpid(current_job->pid, NULL, WNOHANG) > 0){
						// 	log_job_fg_term(current_job->pid, current_job->cmd);//fg
						// 	remove_fg_job();
						// }
					}
				}
			}
			
			break;


		//Ctrl-z
		case 20:
			//Stops foreground running command
			if (current_jobs != NULL && current_jobs->job->aux->is_bg == 0){
				log_ctrl_z();
				current_jobs->job->state = "Stopped";
				log_job_fg_stopped(current_jobs->job->pid, current_jobs->job->cmd);
				fg_to_bg(current_jobs->job->job_id, 1);
			}

			break;
	}
}

//Displays # of jobs and job details
void display_jobs(){
	Job_list *temp = current_jobs;
	//Checks head first
	if (temp != NULL){
		log_job_details(temp->job->job_id, temp->job->pid, temp->job->state, temp->job->cmd);
		
		//Iterate through jobs_list using cmd below
		while(temp->next != NULL){
			log_job_details(temp->next->job->job_id, temp->next->job->pid, temp->next->job->state, temp->next->job->cmd);
			temp = temp->next;
		}
	}
}

//Gets a job from the jobs list using pid
Job *get_job(pid_t pid){
	Job_list *temp = current_jobs;
	//Checks head first
	if (temp != NULL){
		if (temp->job->pid == pid){
			return temp->job;
		}

		//Iterate through jobs_list using cmd below
		while(temp->next != NULL){
			if (temp->next->job->pid == pid){
				return temp->job;
			}
			temp = temp->next;
		}
	}
	return NULL;//If not found
}

//Adds a foreground job to the jobs list
void add_fg_job(Job *job){
	//Increments job count
	job_count++;

	//New job to be added
	Job_list *new_job = (struct job_list_struct*)malloc(sizeof(struct job_list_struct));
	new_job->job = job;
	new_job->next = NULL;
	new_job->prev = NULL;

	//Adds New Job to the head of the list
	if (current_jobs == NULL){
		current_jobs = new_job;
	}else{
		new_job->next = current_jobs;
		current_jobs->prev = new_job;
		current_jobs = new_job;
	}
}

//Adds a background job to the jobs list
void add_bg_job(Job *job){
	//Iteration pointer
	Job_list *iter = current_jobs;

	//New job to be added
	Job_list *new_job = (struct job_list_struct*)malloc(sizeof(struct job_list_struct));
	new_job->job = job;
	new_job->next = NULL;
	new_job->prev = NULL;

	//Increments job count
	job_count++;
	//Adds the job to the front of the linked list
	if (current_jobs == NULL){
		current_jobs = new_job;
	}else{
		while(iter->next != NULL){
			iter = iter->next;
		}
		iter->next = new_job;
	}
}

//remove a foreground job from the jobs list
void remove_fg_job(){
	//decrements job count
	job_count--;
	
	//removing from job list
	Job_list *term_job = current_jobs;
	if (term_job != NULL){

		if (term_job->next != NULL){
			term_job->next->prev = NULL;
			current_jobs = term_job->next;
		}else{
			current_jobs = NULL;
		}

		//Freeing Job Allocated Memory
		free(term_job->job->aux->in_file);
		free(term_job->job->aux->out_file);
		free(term_job->job->aux);
		free(term_job->job->cmd);
		// free(term_job->job->state);
		free(term_job->job);
		free(term_job);
	}
}

//removes a background job from the jobs list
void remove_bg_job(Job *job){
	//decrements job count
	job_count--;

	if (job->aux->is_bg == 1){
		//Finding Job in job list
		Job_list * temp = current_jobs;
		if (temp != NULL){
			if (temp->job == job){
				if (temp->next != NULL){
					current_jobs = temp->next;
					temp->next->prev = NULL; 
				}else{
					current_jobs = NULL;
				}
				
			}else{
				while (temp->next != NULL){
					if (temp->next->job == job){
						if(temp->next != NULL){
							temp->prev->next = temp->next;
							temp->next->prev = temp->prev;
						}else{
							temp->prev->next = NULL;
						}

						temp->next = NULL;
						temp->prev = NULL;
						break;
					}
					temp = temp->next;
				}
			}
		}
		//Freeing Allocated Memory
		free(temp);
	}
	//Freeing Job Allocated Memory
	free(job->aux->in_file);
	free(job->aux->out_file);
	free(job->aux);
	free(job->cmd);
	// free(job->state);
	free(job);
}

//Finds the next avalible Job_Id
int next_job_id(){
	//Initializaiton
	int next_id = 0;
	Job_list *temp = current_jobs;

	//Checks head
	if (temp != NULL){
		next_id = temp->job->job_id;
	
		//Iterate through jobs_list using cmd below
		while(temp->next != NULL){
			if (temp->next->job->job_id > next_id){
				next_id = temp->next->job->job_id;
			}
			temp = temp->next;
		}
	}
	return (next_id + 1);
}

//Transfers job from foreground to background
void fg_to_bg(int job_id, int type){
	Job_list *temp = current_jobs;
	//checks head
	if (temp != NULL){
		//Checks if already in foreground
		if (current_jobs->job->job_id == job_id){
			
			//Changes job id to next open job id if going 
			//from foreground to background for first time
			if (current_jobs->job->job_id == 0){
				current_jobs->job->job_id = next_job_id();
			}
			
			current_jobs->job->aux->is_bg = 1;

			//used if called from built-in cmds
			if (type == 0){
				log_job_bg(current_jobs->job->pid, current_jobs->job->cmd);
			}

			return;

		}else{
			//Iterate through jobs_list using cmd below
			while(temp->next != NULL){
				if (temp->next->job->job_id == job_id){
						
						//Changes job id to next open job id if going 
						//from foregroundto background for first time
						if (temp->job->job_id == 0){
							temp->job->job_id = next_job_id();
						}

					temp->job->aux->is_bg = 1;
					
					//used if called from built-in cmds
					if (type == 0){
						log_job_bg(temp->job->pid, temp->job->cmd);
					}

					return;
					}
				
				temp = temp->next;
				}

			}
	}
	//Job not found
	log_jobid_error(job_id);
	
}

//Transfers job from background to foreground
void bg_to_fg(int job_id, int type){
	Job_list *temp = current_jobs;

	//checks head
	if (temp != NULL){
		//Checks if already in foreground
		if (current_jobs->job->job_id == job_id){
			
			current_jobs->job->aux->is_bg = 0;

			//Used when called from built-in cmds
			if (type == 0){
				log_job_fg(current_jobs->job->pid, current_jobs->job->cmd);
			}

			return;

		}else{
			//Iterate through jobs_list using cmd below
			while(temp->next != NULL){
				if (temp->next->job->job_id == job_id){

					temp->job->aux->is_bg = 0;

					//Used when called from built-in cmds
					if (type == 0){
						log_job_fg(temp->job->pid, temp->job->cmd);
					}

					return;
					}
				
				temp = temp->next;
				}
				
				
			}
	}
	//Job not found
	log_jobid_error(job_id);
}

