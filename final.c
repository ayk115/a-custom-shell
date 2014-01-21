#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>
#include<wait.h>
#include<sys/utsname.h>
#include<string.h>
#include<sys/types.h>
#include<math.h>
#include<fcntl.h>
#include<sys/stat.h>
typedef struct jobs{   //for storing jobs
	int status;
	char name[100];
	int id;
}jobs;
char *user;
char system_name[50];
char cwd[100];
char home[100];
char path[100];
jobs job[100];
int back=0;
int cmdArgc;
int k=-1;
int all_count=0;
char or[100];
void check(char **ori);
void command();
void r_and_p();
void prompt()
{
	printf("<%s@%s:%s>",user,system_name,cwd);
	fflush(stdout);
}
void ctrlC()                                               //handles ctrl+c
{
	printf("\n<%s@%s:%s>",user,system_name,cwd);
	fflush(stdout);
}
void ctrlZ(int signum)                                    //handles ctrl+z
{
	if(signum==SIGTSTP)
	{
	int i;
	for(i=0;i<=k;i++)
		if(job[i].status==2)
		{
			job[i].status=1;
			break;
		}
	kill(job[i].id,SIGTSTP);
	}
	else 
		prompt();
}
void child(int signum)                                         //handles child
{
	int p;
	int i;
	p=waitpid(WAIT_ANY,NULL,WNOHANG);
	for(i=0;i<=k;i++)
	{
		if(job[i].id==p && job[i].status==1)
		{
			printf("\n%s with pid %d exited normally\n",job[i].name,job[i].id);
			prompt();
			fflush(stdout);
			job[i].status=0;
		}
	}
	signal(SIGCHLD,child);
	return ;
//	prompt();
//	fflush(stdout);
}

int pip=0;
int main()
{
	signal(SIGINT,ctrlC);
	signal(SIGTSTP,ctrlZ);
	signal(SIGCHLD,child);
	gethostname(system_name,50);                     //gets the name of the system
	user=getlogin();                                 // gets the username
	strcpy(cwd,"~");
	getcwd(home,100);				//gets the present working dir
	getcwd(path,100);
	char c='\0';
	prompt();
	fflush(stdout);
	int p=0;
	while(1)
	{
		c=getchar();                           //takes input
		if(c=='\n' && strlen(or)==0)
		{
			prompt();
			fflush(stdout);
		}
		else if(c=='\n')
		{
			int i;
			char *ori[50];
			if(p==1)
			{
				r_and_p(or);                     //redirection and piping
				p=0;
			}
			else 
			{
				char *ori[50],*cmd[50];
				int cmdArgc=parse(or,ori);
				int argc=0,k=-1,j,state=1,in=0,out=1;
				for(j=0;j<cmdArgc;j++)
				{
					if(strcmp(ori[j],">")==0)
					{
						state=0;
						out=open(ori[j+1],O_RDONLY);
					}
					else if(strcmp(ori[j],"<")==0)
					{
						state=0;
						in=open(ori[j+1],O_WRONLY | O_CREAT,S_IRWXU);
					}
					else if(state==1)					
					{
						argc++;
						cmd[++k]=ori[j];
						cmd[k+1]='\0';
					}
				}
	//			for(j=0;j<argc;j++)
	//			printf("%s\n",cmd[j]);
	//			printf("%d %d\n",in,out);
				command(cmd,in,out,argc);                              //implements all commands with redirection
					
			}
			prompt();
			fflush(stdout);
			for(i=0;i<50;i++)
				or[i]='\0';
		}
		else 
		{
			if(c=='|')                                   //checks for piping
				p=1;
			if(c!='&')                                      //checks for background processes
				strncat(or,&c,1);
			else if(c=='&')
				back=1;
		}
	}
	
	return 0;
}
int cno=0;

void r_and_p(char input[])
{
	char *temp = NULL, *pipeCommands[1000], *cmdArgs[1000],*newcmdArgs[1000];
	int newPipe[2], oldPipe[2], pipesCount, aCount=0, i, status,newaCount=0,in,out; //newPipe[0]=read; newPipe[1]=write
	pid_t pid;
	pipesCount = 0; 					// no of pipe commands to be executed, NOTE : not pipe count :P
	char *token=strtok(input,"|");
	while(token!=NULL)
	{
		token[strlen(token)]='\0';
		pipeCommands[pipesCount]=token;
		pipesCount++;
		token=strtok(NULL,"|");
	}
	pipeCommands[pipesCount++]=token;
	pipesCount--;						//pipeCount has been increased by more than one , hence decreasing
	//int j;
	//for(j=0;j<pipesCount;j++)
	//	printf("%s\n",pipeCommands[j]);
	int stdin_copy, stdout_copy;				// creating copy of stdin and stdout to restore later on 
	stdin_copy=dup(STDIN_FILENO);	
	stdout_copy=dup(STDOUT_FILENO);

	for(i = 0; i < pipesCount; i++) /* For each command */
	{
		int inflag=0,outflag=0,inindex=-1,outindex=-1,j; 
		for(j=0;j<strlen(pipeCommands[i]);j++)
		{
			if(pipeCommands[i][j]=='<')		// check if it has input redirection
			{
				inflag=1;
				inindex=i;
			}
			if(pipeCommands[i][j]=='>')		//check if it has output redirection
			{
				outflag=1;
				outindex=i;
			}
		}
		aCount = 0;                                   // argument count

		/* parsing in case of inflag */

		if(inflag==1)
		{

			token=strtok(pipeCommands[i],"<");	// parsing , tokenising along <
			while(token!=NULL)
			{
				token[strlen(token)]='\0';
				cmdArgs[aCount]=token;
				aCount++;
				token=strtok(NULL,"<");
			}
			cmdArgs[aCount++]=token;		//cmdArgs[1] is for sure a file name.		

			newaCount=0;

			token=strtok(cmdArgs[0]," ");		// tokenising cmdArgs[0] , which is a command and may have flags .
			while(token!=NULL)
			{
				token[strlen(token)]='\0';
				newcmdArgs[newaCount]=token;
				newaCount++;
				token=strtok(NULL," ");
			}
			newcmdArgs[newaCount++]=token;		// newcmdArgs has all the flags in it (along with the command name).

			in = open(cmdArgs[1], O_RDONLY , 0664);	// open the file with fd = in.
			if (in < 0) {
				perror(cmdArgs[1]);		// error if file doesn't exist.
				exit(1);
			}
				dup2(in, STDIN_FILENO);
			
		}

		/* parsing in case of outflag  */

		else if(outflag==1)
		{
//	dd	printf("no\n");
			token=strtok(pipeCommands[i],">");
			while(token!=NULL)
			{
				token[strlen(token)]='\0';
				cmdArgs[aCount]=token;
				aCount++;
				token=strtok(NULL,">");
			}
			cmdArgs[aCount++]=token;
			cmdArgs[2]=NULL;
			newaCount=0;
			token=strtok(cmdArgs[0]," ");
			while(token!=NULL)
			{
				token[strlen(token)]='\0';
				newcmdArgs[newaCount]=token;
				newaCount++;
				token=strtok(NULL," ");
			}
			newcmdArgs[newaCount++]=token;
			out = open(cmdArgs[1], O_CREAT | O_WRONLY, S_IRWXU);
			if (out < 0) {
				perror(cmdArgs[1]);
				exit(1);
			}
					//dup2(out, STDOUT_FILENO);
		//			printf("no\n");
					
		}
		/* Parsing command & arguments in case of only pipes*/
		else
		{
			token=strtok(pipeCommands[i]," ");
			while(token!=NULL)
			{
				token[strlen(token)]='\0';
				cmdArgs[aCount]=token;
				aCount++;
				token=strtok(NULL," ");
			}
			cmdArgs[aCount++]=token;
		}
	//	printf("%d %d\n",inflag,outflag);
	//	printf("%d\n",pipesCount);
		/* If there still are commands to be executed */
		if(i < pipesCount-1) 
			pipe(newPipe); /* create a pipe */
		pid = fork();

		if(i>0 && i<pipesCount-1)	//for !first && !last command.
		{
			dup2(oldPipe[0], 0); //oldpipe[0] is the new stdin; ie writes happen at oldpipe[0]
			close(oldPipe[1]);
			close(oldPipe[0]);

		}
		else if(i==pipesCount-1)	//for last command.
		{
			dup2(oldPipe[0], 0); //oldpipe[0] is the new stdin; ie writes happen at oldpipe[0]
			
			close(oldPipe[1]);
			close(oldPipe[0]);

		}

		if(pid == 0)  /* Child */
		{
			if(i==0)
			{
				dup2(newPipe[1], 1); // stdout goes into newpipe[1]
				close(newPipe[0]);
				close(newPipe[1]);

			}

			
			if(i>0 && i<pipesCount-1)
			{
				dup2(newPipe[1], 1); // stdout goes into newpipe[1]
				close(newPipe[0]);
				close(newPipe[1]);

			}
			if(!(inflag || outflag))		// normal execvp with cmdArgs[0] and cmdArgs as input.
			{
				int res = execvp(cmdArgs[0], cmdArgs);
				if (res == -1)
					printf("Error. Command not found: %s\n", cmdArgs[0]);
				exit(1);
			}

			if(outflag==1)				// execvp with cmdArgs[0] nad newcmdArgs as input. ( newcmdArgs has flags)
			{
		//			printf("out%d\n",out);
		//			printf("%s\n",cmdArgs[0]);
					
		//			for(j=0;newcmdArgs[j]!='\0';j++)
		//			{
		//				printf("%s\n",newcmdArgs[j]);
		//			}
					dup2(out,1);
					close(out);
					if (execvp(cmdArgs[0],newcmdArgs) < 0) {
						perror("execvp");
						exit(1);
					}
		//			printf("su\n");
			}
			if(inflag==1)				// execvp with cmdArgs[0] and newcmdArgs as input.
			{
				if (execvp(cmdArgs[0],newcmdArgs) < 0) {
					perror("execvp");
					exit(1);
				}
			}
		} 
		else /* Father */
		{		
			waitpid(pid, &status, 0);			// wait for each child to die.

		/* to recover pids later on for use in pid commands */ 

		//	all[all_count].pid=pid;
		//	strcpy(all[all_count].name,cmdArgs[0]);
		//	all[all_count++].status=0;

		/* saving pipes to take input from these later on if needed .*/

			if(i < pipesCount-1) 
			{
				oldPipe[0] = newPipe[0];
				oldPipe[1] = newPipe[1];
			}
		}
	}

	/* close all pipes and restore stdin and stdout */

	close(oldPipe[0]);
	close(newPipe[0]);
	close(oldPipe[1]);
	close(newPipe[1]);
	// restore stdin, stdout
	dup2(stdin_copy, 0);
	dup2(stdout_copy, 1);
	close(stdin_copy);
	close(stdout_copy);
}


void command(char **cmd,int in,int out,int argc)                 //executes all commands
{
//	printf("eter\n");
//	printf("%d %d %d\n",in,out,argc);
	int i=0;
	if(strcmp(cmd[0],"quit")==0)                                   //executes quit
	{
		exit(0);
	}
	else if(strcmp(cmd[0],"pinfo")==0)			//executes pinfo
	{
		char buff[100];
		char str[100];
		if(cmdArgc==1)
		{
			int pid=getpid();
			sprintf(buff,"%d",pid);
		}
		else
			strcpy(buff,cmd[1]);
		strcpy(str,"/proc/");
		strcat(str,buff);
		strcat(str,"/status");
		FILE *fp=fopen(str,"r");
		char line[1000];
		int count=12;
		while(count--)
		{
			fgets(line,1000,fp);
			fputs(line,stdout);
		}
		fclose(fp);
	}
	else if(strcmp(cmd[0],"cd")==0)				//executes cd  all cases
	{
		if(cmdArgc==1)
		{
			strcpy(cwd,"~");
			chdir(home);
			getcwd(path,100);
		}
		else if(strcmp(cmd[1],"~")==0)
		{
			strcpy(cwd,"~");
			chdir(home);
			getcwd(path,100);
		}
		else
		{
			int re=chdir(cmd[1]);
			if(re==-1)
				printf("No such directory exists\n");
			else
			{
				getcwd(path,1000);
				if(strcmp(path,home)==0)
				{
					strcpy(cwd,"~");
				}
				else if(strcmp(path,home)>0)
				{
					int len=strlen(path)-strlen(home);
					strcpy(cwd,"~");
					char *tmp;
					int ho=strlen(home);
					tmp=strndup(path+ho,len);
					strcat(cwd,tmp);
				}
				else if(strcmp(path,home)<0)
				{
					char tmp[100];
					int save,i;
					for(i=1;i<strlen(path)-1;i++)
					{
						if(path[i]==47)
							save=i;
					}
					for(i=save;i<strlen(path);i++)
					{
						tmp[i-save]=path[i];
					}
					tmp[i-save]='\0';
					strcpy(cwd,tmp);
				}
			}
		}
	}
	else if(strcmp(cmd[0],"jobs")==0)				//executes jobs
	{
		//printf("yes\n");
		int i;
		for(i=0;i<=k;i++)
		{
			if(job[i].status==1)
				printf("[%d] %s [%d]\n",i+1,job[i].name,job[i].id);
		}
	}
	else if(strcmp(cmd[0],"kjob")==0)                             //executes kjob
	{
//		if(cmdArgc<=2)
//			printf("Command usage 'kjob <job ID> <signal>\n");
//		else 
//		{
			long int i=strtol(cmd[1],NULL,10);
			int j,count=0;
			long int sig=strtol(cmd[2],NULL,10);
			long int p=job[i-1].id;
			int re=kill(p,sig);
			if(re==-1)
				printf("The job ID does not exists\n");
//		}
	}
	else if(strcmp(cmd[0],"overkill")==0)                           //executes overkill
	{	
		int i;
		for(i=0;i<=k;i++)
		{
			if(job[i].status==1)
			{
				kill(job[i].id,9);
				job[i].status=0;
			}
		}
	}
	else if(strcmp(cmd[0],"fg")==0)					//executes fg
	{
		if(cmdArgc==1)
			printf("Please enter job ID\n");
		else 
		{
			long int pro=strtol(cmd[1],NULL,10);
			int p=job[pro-1].id;
			int status=0;
			job[pro-1].status=1;
			int a=getpgid(p);
		//	printf("%d\n",a);
		//	kill(p,SIGCONT);
			int pi=waitpid(p,&status,0),i;
//			printf("%d\n",pi);
			for(i=0;i<=k;i++)
			{
				if(job[i].id==pi)
					job[i].status=0;
			}
		}
	}
	else                              				//all other commands
	{		
//		printf("%s\n",cmd[0]);
//		printf("%d %d\n",in,out);	
		strcpy(job[++k].name,cmd[0]);
		int i;
		for(i=1;i<argc;i++)
		{
		
			strcat(job[k].name," ");
			strcat(job[k].name,cmd[i]);
		}
		job[k].status=2;
		if(back==1)
		{
			job[k].status=1;
		}
		int ret=0;
		int pid=fork();
		job[k].id=pid;
		if(pid==0)                                              //child
		{	
			dup2(in,0);                                              //changing input and output modes
			dup2(out,1);	
			ret=execvp(*cmd,cmd);
			if(ret==-1)
			{
				printf("Not a valid command\n");
				_exit(0);
			}

		}
		else                                                       //parent
		{
		
			if(back==0)
			{
				wait(NULL);
				//waitpid(pid,&s,WUNTRACED);
			}
		}
		back=0;
	}
}

int parse(char *inputString, char *cmdArgv[])                                  //parse function
{
	int cmdArgc = 0, terminate = 0;
	char *srcPtr = inputString;
	while(*srcPtr==' ' || *srcPtr=='\t')
		srcPtr++;

	//printf("parse fun%sends", inputString);
	while(*srcPtr != '\0' && terminate == 0)
	{
		*cmdArgv = srcPtr;
		cmdArgc++;
		//printf("parse fun2%sends", *cmdArgv);
		while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\0' && *srcPtr != '\n' && terminate == 0)
		{

			srcPtr++;
		}
		while((*srcPtr == ' ' || *srcPtr == '\t' || *srcPtr == '\n') && terminate == 0)
		{
			*srcPtr = '\0';
			srcPtr++;
		}
		cmdArgv++;
	}
	/*srcPtr++;
	 *srcPtr = '\0';
	 destPtr--;*/
	*cmdArgv = '\0';
	return cmdArgc;
}


