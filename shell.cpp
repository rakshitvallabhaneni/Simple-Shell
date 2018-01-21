#include<iostream>
#include<string>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include <vector>


using namespace std;

struct table{	
			string eachCommand;
			int countOfSpaces;
		};

//This method is to divide a command string into arguments and put them into char**
void getCommandArguments(char** commArgs,string commandString, int countOfSpaces)
{
	char* argToken;
	char trial[commandString.length()];
	strcpy(trial,&commandString[0]);
	char* firstChar = (char*)malloc(sizeof(char) * 512);
	int strLen = commandString.length();
	
	for(int i = 0; i < commandString.length() - 1; i++)
	{

		if(commandString[i] == ' ' && commandString[i+1] == ' ')
		{
			commandString.erase(i,1);
			countOfSpaces--;i--;
		}

	}

	strLen = commandString.length()+1;
	for(int i = 0; i < strLen;i++)
	{
		if(commandString[0] == ' '){ 
			commandString.erase(0,1);
			countOfSpaces--;	
		}
		
	}
	if(commandString[commandString.length() - 1] == ' '){ 
			commandString.erase(commandString.length() - 1,1);
			countOfSpaces--;	
		}

	argToken = strtok(&commandString[0]," ");
	commArgs[0] = (char*)malloc(sizeof(char) * 512);
	strcpy(commArgs[0],argToken);
	for(int k = 1; k < countOfSpaces+1; k++)
	{
		commArgs[k] = (char*)malloc(sizeof(char) * 512);
		argToken = '\0';
		argToken = strtok(NULL," ");
		strcpy(commArgs[k],argToken);
		
	}
	commArgs[countOfSpaces+1] = (char *)malloc(sizeof(char) * 512);
	commArgs[countOfSpaces+1] = NULL;



	
}

//This method counts the number of slashes in case address of the binary is also given.
int countSlash(char* command)
{
	int countOfSlashes = 0;
	string commandWithPath(command);
	for(int i = 0; i < commandWithPath.length(); i++)
		if(commandWithPath[i] == '/')
			countOfSlashes++;

	return countOfSlashes;
}

//This method gives the command path of the given command
//if the path is given along with the command it doesnt find the path in the list of directories
char* getCommandPath(char* command,char** directories,int countOfPaths)
{
	string commandWithPath(command);
	int error = 1;
	//cout <<"the command with path is = "<<commandWithPath<<endl;
	if(commandWithPath[0] == '/')
	{
		//cout <<"It is an address"<<endl;
		char* pathToken = (char*)malloc(sizeof(char) * 512);
		char* givenPath = (char*)malloc(sizeof(char) * 512);
		int noOfSlashes = countSlash(command);


		strcpy(givenPath,"/");
		pathToken = strtok(&commandWithPath[0],"/");
		//cout <<"pathToken[0] = "<<pathToken<<endl;
		strcat(givenPath,pathToken);		

		for(int i = 0; i < noOfSlashes - 1; i++)
		{
			
			pathToken = strtok(NULL,"/");				
			strcat(givenPath,"/");
			strcat(givenPath,pathToken);			
			if(i == noOfSlashes - 2)	
				strcpy(command,pathToken);
								
			
		}
		return givenPath;
		
	}
	
	
	char* commandPath;
	commandPath = (char*)malloc(sizeof(char) * 512);
	for(int i = 0; i < countOfPaths; i++)
	{
	char* path;
	commandPath = (char*)malloc(sizeof(char) * 512);
	path = (char*)malloc(sizeof(char) * 512);
	strcpy(path,directories[i]);
	strcat(path,"/");
	strcat(path,command);
	if(access(path,X_OK) == 0)
	{	
		strcpy(commandPath,path);
		error = 0;		
		break;
	}
	
	bzero(path,sizeof(char) * 512);
	}	
	if(error == 0)
		return commandPath;
	else
	{
		cout <<command<<": command not found"<<endl;
		exit(0);
	}
}

//This method is for n-1 commands of a line if there are n commands connected with pipes
void executePipe(int in, int out,char** commArgs,char* commandPath)
{
	pid_t pid;
	if((pid = fork()) == 0)
	{
		// This is because the first command will get input from stdin 0
		if(in != 0) 
		{
			dup2(in,0); //dup into stdin and close stdin
			close(in);  //good practice
		}
		
			dup2(out,1); //dup into stdout and close stdout
			close(out);  //good practice
		execv(commandPath,commArgs);
	} 
	
}

//This method calls executePipe in case pipes are present in the command.
//In case pipes are present, this is also responsible for executing the last command.
//In case there are no pipes in the command, this method executes the command.
void forkPipes(vector<struct table> catalog,char* directories[100],int countOfPaths)
{
	//cout <<"ENTERED FORK PIPES"<<endl;
	pid_t pid;
	int in, fd[2];
	in = 0;
	
	int noOfCommands = catalog.size();
	int i;
	for(i = 0; i < catalog.size()-1; i++)
		{
			pipe(fd);
			struct table currentCommand = catalog[i];
			char* commArgs[currentCommand.eachCommand.size()];
			getCommandArguments(commArgs,currentCommand.eachCommand,currentCommand.countOfSpaces);	
			
			char* commandPath;
			commandPath = (char*)malloc(sizeof(char) * 512);
			commandPath = getCommandPath(commArgs[0],directories,countOfPaths);
			executePipe(in,fd[1],commArgs,commandPath);
			close(fd[1]);
			in = fd[0];
			
		}
	
	//This is for the case when the input line contains pipes,
	//and to execute the final command of the pipe.
	//For this, we would require input from pipe.
	if(in != 0)
	{
		dup2(in, 0);
		close(in);
	}

	//This is where the last command of a pipe or if the command doesnt have a pipe is executed.
	struct table currentCommand = catalog[catalog.size()-1];
	char* commArgs[currentCommand.eachCommand.size()];
	
	getCommandArguments(commArgs,currentCommand.eachCommand,currentCommand.countOfSpaces);	
	
	char* commandPath;
	commandPath = (char*)malloc(sizeof(char) * 512);
	commandPath = getCommandPath(commArgs[0],directories,countOfPaths);
	execv(commandPath,commArgs);	
}

int main()
{
	char* directories[100];
	char* shell = getenv("PATH");
	string path(shell);
	char* pathToken;
	int countOfPaths=1;
	for(int i = 0; i < path.size(); i++)
	{
		if(path[i] == ':')
			countOfPaths++;
		
	}
	pathToken = strtok(&path[0],":");
	directories[0] = (char*)malloc(sizeof(char) * 512);
	strcpy(directories[0],pathToken);

	for(int i = 1; i < countOfPaths; i++)
	{
		directories[i] = (char*)malloc(sizeof(char) * 512);
		pathToken = '\0';
		pathToken = strtok(NULL,":"); 
		strcpy(directories[i],pathToken);
	}

	string line;


	cout <<"Welcome to SHELL"<<endl;
	int pid = 1;	
	int loop = 1;
	string exitString = "exit";
	while(strcmp(&line[0],"exit") != 0)
	{

		cout <<"MyShell: ";
		getline(cin,line);
		if(line.empty())
			continue;

		
		if (line.find(exitString) != std::string::npos) 
		{
			continue;			
		}
		if(strcmp(&line[0],"exit") ==0)
		{
			exit(0);
		}

		int countOfPipes=1;
		for (int i = 0; i < line.size(); i++)
    			if (line[i] == '|') 
				countOfPipes++;

		char* command[countOfPipes];
		char* commToken;
		if(countOfPipes > 1)
		{
			commToken = strtok(&line[0],"|");
			command[0] = (char*)malloc(sizeof(char) * 512);
			strcpy(command[0],commToken);
			
			for(int i = 1; i < countOfPipes;i++)
			{
				command[i]=(char*)malloc(sizeof(char)*512);
				commToken = '\0';
				commToken = strtok(NULL,"|");
				strcpy(command[i],commToken);
			}
		}
		else
		{
			command[0] = (char*)malloc(sizeof(char) * 512);
			strcpy(command[0],&line[0]);
		}

		// Each line is split at '|' and stored in the vector catalog - This is a vector of structs containing 
		//command string and the numberOfSpaces in the string.
		// now each element of array command[] is going to be split at ' ' (space),
		// so as to find the arguments and the respective commands and their arguments
		// now each command of a single line is going to be in commArgs[0] & the rest are their arguments

		vector<struct table> catalog;		

		for(int i = 0; i < countOfPipes;i++)
		{	
			int countOfSpaces = 0;
			string commandString(command[i]);
			for (int j = 0; j < commandString.size(); j++)
    				if (commandString[j] == ' ') 
					countOfSpaces++;


			struct table nextCommand;
			nextCommand.eachCommand = commandString;
			nextCommand.countOfSpaces = countOfSpaces;
			catalog.push_back(nextCommand);
		}

		if(catalog.size()==1)
		{
			struct table currentCommand = catalog[0];
			char* commArgs[currentCommand.eachCommand.size()];
			getCommandArguments(commArgs,currentCommand.eachCommand,currentCommand.countOfSpaces);
			if(strcmp(commArgs[0],"cd") == 0)
					{
						chdir(commArgs[1]);
						continue;
					}
		}		
	
	int parent = fork();
	while(wait(0) != parent)
		forkPipes(catalog,directories,countOfPaths);
				
	}
}
