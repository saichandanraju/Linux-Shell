/*
Program Name : tash.c
Authors:Sai Chandan Raju Padmaraju
Created On: 09/18/2021
Description : This program implements a basic shell named tash, which can run both in interactive mode & batch mode.
	      It also has features of redirection and Parallel Processes creation. It supports the cd, exit, and path built in commands.
Functions:
	1) print_error: Prints the one and only error message
	2) promptTash: Prints tash> to the console which signifies interactive mode
	3) readInput:   - Uses getline to read the input both in interactive mode and batch mode. 
			- It removes new line character from every getline command
			- It converts tabs to spaces. Spaces will be handled by strtok in extractParams.
			- If only spaces or tabs are present in the input line, then it skips and asks for new commands
			- It prints error if each `&` character is not preceeded by a command
	4) extractParams: Using the delimiter argument, it bifurcates the command into an array of words and stores it
					  into the char* param argument. When space is passed as delimiter, additional spaces are
					  handled by strtok
	5) builtInCommands: It supports 3 built in commands namely exit, cd, and path. exit doesn't support any arguments to it.
						cd takes only one argument. path supports any number of arguments including zero.
	6) main: Using the above mentioned funtions, main creates child processes from parent process in a loop till
			 EOF is reached or exit command is passed. Supports batch and interactive mode. Commands seperated by `&` character
			 will be processed parallelly. If `>` character is present output is redirected to the file mentioned.

*/


// Required header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strtok(), strcat(), strlen(), and strcmp()
#include <sys/wait.h> // For wait funtion
#include <unistd.h>// For fork, execv, access, dup2
#include <fcntl.h> // open or close files
#include <stdbool.h> // For boolean variables


#define MAXCHARS 1000000
#define MAXPARAMS 100000



// Prints the one and only error message
void print_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}




// Prints tash> to the console which signifies interactive mode
void promptTash(){
	printf("\ntash>");
}



// To read input
char* readInput(){
	
	size_t numOfCharacters;
	size_t sizeOfInput = MAXCHARS;

	char* input;

	input = (char *) malloc(sizeOfInput + 1);

	numOfCharacters = getline(&input, &sizeOfInput, stdin);

	if(numOfCharacters < 0) { //If any error in accessing the input itself
		print_error();
	}

	else{
		if(numOfCharacters == 0 || numOfCharacters == 1){ // If only new line character is passed
			input = NULL;
			return input;
		}

		
		// Removes the new line character and terminates the string
		if(input[numOfCharacters-1] == '\n'){
			input[numOfCharacters-1] = '\0';
		}

		
		// Converts tabs to spaces
		unsigned int i = 0;
		while(input[i] != '\0'){
			if(input[i] == '\t') input[i] = ' ';
			i++;
		}


		int charFlag = 0;			// Counts the number of characters other than space, new line, and `&`
		int parallelFlag = 0;		// If no command preceded by `&` this flag is set to one
		bool charIsPresent = false;


		// To check if no command is preceeded by `&` character
		for(i=0; i<strlen(input); i++ ){

			if(input[i] == '&'){
				if(charIsPresent == false){
					parallelFlag = 1;
					break;
				}

				charIsPresent = false;
			}

			if(input[i] != ' ' && input[i] != '&' && input[i] != '\n'){
				charIsPresent = true;
				charFlag++;
			}
		}


		// Prints the error if no command preceded by `&`
		if (parallelFlag == 1)
		{
			print_error();
			return NULL;	// return NULL is treated as continue in the main function
		}

		// Continues if no significant input is received
		if (charFlag == 0)
		{
			return NULL;    // return NULL is treated as continue in the main function
		}

		return input;
	}
}

// Extracts the required parameters
void  extractParams(char *input, char* param[], char* delimiter){

	char * token;
	int i = 0;


	// Using the delimiter argument, it bifurcates the command into an array of words.
	// When space is passed as delimiter, additional spaces are handled by strtok
	token = strtok(input, delimiter);

	while(token != NULL){ // Adds each token received into param
		param[i++] = strdup(token);
		token = strtok(NULL, delimiter);
	}

	// Makes sure to end the char* with NULL
	param[i] = NULL;
}


// Defines Built in Commands
bool builtInCommands(char* par[], char* customPath[]){

	if(strcmp(par[0], "exit") == 0){ // exit built in command
		if(par[1] == NULL){ // makes sure no argument is passed to exit
			exit(0);
		}else{
			print_error();
			return true;
		}
	}

	if(strcmp(par[0],"path") == 0){ // path built in command
		customPath[0] = NULL;
		int i = 1;
		while(par[i++] != NULL){
			customPath[i-2] = par[i-1]; // Assigns user defined path values to customPath
		}
		customPath[i-2] = NULL; 
		return true;
	}
	
	else if(strcmp(par[0], "cd") == 0){ // cd built in command
		if(par[2] != NULL || par[1] == NULL){ // Makes sure only one argument is passed to cd
			print_error();
			return true;
		}

		int cdSuccess = chdir(strdup(par[1])); // changing the directory

		// If chdir is unsuccessfull, prints the error
		if(cdSuccess == -1){
			// printf("Error Reached\n");
			print_error();
		}
		
		return true;
	}

	// If no built in command was requested, continues in main
	return false;
}

// The main function
int main(int argCount, char **args){
	
	// Variables to store input, parameters, paths and their arguments
	char *input, *parampapa[MAXPARAMS], *customPath[MAXPARAMS], *param[MAXPARAMS];
	char *redirectionParam[MAXPARAMS], *outputFileName[MAXPARAMS];
	char cmd[] = "";
	
	// Initial path definition
	char tmpPath[] = "/bin";
	customPath[0] = tmpPath;
	customPath[1] = NULL;
	
	// Delimiters used 
	char majorDelimiter[] = " ";
	char lieutinentDelimiter[] = "&";
	char redirectionDelimiter[] = ">";
	
	// flag used to open a file in the first loop of batch mode
	bool isFirstTime = true;
	
	//Additional vars required
	int status = 0;
	int j = 0;
	
	// To check for batch mode
	bool isBatchMode = false;

	if(argCount == 2){ // Batch mode takes only one input file argument hence argCount should be 2
		isBatchMode = true;
	}else if(argCount > 2){// More than one input file argument prints the error
		print_error();
		exit(1);
	}
	
	// The tash loop
	while(j < 10000){

		if(!isBatchMode){

			promptTash(); // Interactive Mode
			
			
		}else if(isFirstTime){
			// Opens the input file
			int fd3 = open(args[1], O_RDONLY, S_IRWXU | S_IRWXG );

			if(fd3 < 0){ // Bad input file given
				print_error();
				exit(1);
			}

			int ret3 = dup2(fd3, 0);

			if(ret3 < 0){ // Unable to change stdin stream
				print_error();
				exit(0);
			}

			isFirstTime = false;
		}

		// Reads the input line by line
		input = readInput();

		
		// if(input == "/EOF")
		if(input == NULL) continue;
		
		// Uses the `&` delimiter to get all the parallel commands to be processed
		extractParams(input, parampapa , lieutinentDelimiter);

		
		// Executing the parallel commands. If a built in command is received all other commands will
		// execute only after the built in command is executed
		int paramcounter = -1;
		while(parampapa[++paramcounter] != NULL){
			// If no file is present after redirection char... prints the error
			char lastChar = parampapa[paramcounter][(strlen(parampapa[paramcounter]) - 1)];
			char firstChar = parampapa[paramcounter][0];
			if(lastChar == '>' || firstChar == '>' ){
				print_error();
				continue;
			}
			
			// Checks different redirection chars if present
			extractParams(parampapa[paramcounter], redirectionParam, redirectionDelimiter);
			
			// Counting the number of redirections received
			int redirectCount = 0;
			while(redirectionParam[redirectCount] != NULL){
				redirectCount++;
			}

			if(redirectCount == 1) { // No redirection is present. Remember we already checked first and last corner cases
				extractParams(parampapa[paramcounter], param, majorDelimiter);
			} else if(redirectCount == 2){ // Redirection is present
				
				// Stores the command parameters to param which is present before the redirection command
				extractParams(redirectionParam[0], param, majorDelimiter);
				// Gets the output files mentioned after the direction command
				extractParams(redirectionParam[1], outputFileName, majorDelimiter);
				// Checks if more than one output file name is present
				int outputCount = 0;
				while(outputFileName[outputCount] != NULL){
					outputCount++;
				};


				if(outputCount > 1){ //If more than one output file is mentioned, prints the error
					print_error();
					continue;
				}

			} else{ // If more than one redirections are present, prints the error
				print_error();
				continue;
			}

			// Runs the inbuilt commands
			if(builtInCommands(param, customPath)) continue;
			// Carries over the filename to the child process
			char *finalOutputFile=outputFileName[0];

			//Creates a child process
			if(fork() == 0){
				// Enters the child process
				

					//Checks if any of the paths can execute the required command
					int i = 0;
					int accessFlag = -1;
					while(customPath[i++] != NULL){//loops through all the commands
						
						//Temporary path to the executable is created
						char *tPath = strdup(customPath[i-1]);
						strcat(tPath, "/");
						strcat(tPath, param[0]);
						
						// Access to the path is checked
						accessFlag = access(tPath, X_OK);

						if(accessFlag == 0){ //If access available, path is copied to cmd
							strcpy(cmd, tPath);
							break;
						}
					}

					// printf("%s redirection param2\n", outputFileName[0]);

					// If no path contains the executable command, prints the error
					if(accessFlag == -1){
						print_error();
						exit(0);
					}

					int fd = 1, fd2 = 2;
					// if redirection is present then open the files to stream the output to
					if(redirectCount == 2){
						// printf("%s output file name\n", outputFileName[0]);
						// Create file if doesn't exist. No appending to the previous file.
						fd = open(finalOutputFile, O_CREAT | O_WRONLY , S_IRWXU );

						if(fd < 0){ // Some error in opening or creating the file
							print_error();
							exit(0);
						}

						int ret = dup2(fd, 1); //changing the stdout stream

						if(ret < 0){ // Error in changing the output stream
							close(fd2);
							close(fd);
							print_error();
							exit(0);
						}

						int ret2 = dup2(fd, 2); // changing the stderr stream


						if(ret2 < 0){ // Error in changing the error output stream
							close(fd2);
							close(fd);
							print_error();
							exit(0);
						}


					}
					
					// Finally executing the command while passing its arguments
					execv(cmd, param);
					close(fd2);
					close(fd);
					print_error();
					exit(0);
			}


		// child process ends here
		}

		// Parent waits for all the childs to complete before moving forward with next line
		while(wait(&status) > 0);

		j++;
	}

	return 0;
}


