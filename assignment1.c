#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

int numCommands = 0;
struct node
{
    int id;
    char** data;
    struct node *next;
}*head,*tail,*temp;

char** copyStrArray(char* command[]) {
  char** copyCommand = malloc(sizeof(char*)*MAX_LINE/+1);
  // char *copyCommand[MAX_LINE/+1];
  // printf("i: %d strlen(args[0]): %lu\n", 0, strlen(args));
  // printf("lenght of args: %lu", sizeof(args)/sizeof(args[0]));
  for (int i = 0; i < MAX_LINE/2; i++) {
    if (command[i] == NULL) {
      break;
    }
    copyCommand[i] = malloc(strlen(command[i]) + 1);
    strcpy(copyCommand[i], command[i]);
  }
  return copyCommand;
}

void copyExactStrArr(char* copyCommand[], char* originalCommand[]) {
  for (int i = 0; i < MAX_LINE/2; i++) {
    if (originalCommand[i] == NULL) {
      copyCommand[i] = NULL;
    } else {
      copyCommand[i] = malloc(strlen(originalCommand[i]) + 1);
      strcpy(copyCommand[i], originalCommand[i]);
    }
  }
}

void enqueue(char* command[]) {
  char** copyCommand = copyStrArray(command);

  if (head == NULL) {
    head = (struct node *)malloc(1*sizeof(struct node));
    head -> id = 1;
    head -> data = copyCommand;
    head -> next = NULL;
    tail = head;
    numCommands = 1;
  } else {
    temp = (struct node *)malloc(1*sizeof(struct node));
    temp -> id = tail -> id + 1;
    tail -> next = temp;
    temp -> data = copyCommand;
    temp -> next = NULL;
    tail = temp;
  }
  numCommands++; //total number of commands in history increments
}

void freeNode(struct node* nodeToBeFreed) {
  nodeToBeFreed -> next = NULL;

  char** data = nodeToBeFreed->data;
  for (int i = 0; i < MAX_LINE/2; i++) {
    if (data[i] == NULL) {
      break;
    }
    free(data[i]);
  }
  free(nodeToBeFreed);
}

//Assume there is more than one node (head != tail)
void dequeue() {
  temp = head;
  head = head -> next;
  freeNode(temp);
}

void printCommand(char** command) {
  for (int i = 0; i < MAX_LINE/2; i++) {
    if (command[i] == NULL) {
      printf("\n");
      return;
    }
    printf("%s ", command[i]);
  }
}

void showHistory() {
  if (head == NULL) {
    printf("No History Yet.\n");
  } else {
    temp = head;
    while (temp != NULL) {
      printf("%d ", temp -> id);
      printCommand(temp -> data);
      temp  = temp -> next;
    }
  }
}

void appendToHist(char* command[]) {
  printf("appending\n");
  enqueue(command);
  if (numCommands > 11) { //11 because after numCommands incremented
    dequeue();
  }
}

/**
 * setup() reads in the next command line, separating it into distinct
 * tokens using whitespace (space or tab) as delimiters. setup() sets
 * the args parameter as a null-terminated string.
 */
//TODO: RETURN SUCCESS VS FAILURE
void setup(char inputBuffer[], char *args[], int *background)
{
  int length, /* # of characters in the command line */
    i,        /* loop index for accessing inputBuffer array */
    start,    /* index where beginning of next command parameter is */
    ct;       /* index of where to place the next parameter into args[] */

  ct = 0;

  /* read what the user enters on the command line */
  length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

  start = -1;
  if (length == 0) {
    /* ctrl-d was entered, quit the shell normally */
    printf("\n");
    exit(0);
  }
  if (length < 0) {
    /* somthing wrong; terminate with error code of -1 */
    perror("Reading the command");
    exit(-1);
  }

  /* examine every character in the inputBuffer */
  for (i = 0; i < length; i++) {
    switch (inputBuffer[i]){
    case ' ':
    case '\t':               /* argument separators */
      if(start != -1){
        args[ct] = &inputBuffer[start];    /* set up pointer */
        ct++;
      }
      inputBuffer[i] = '\0'; /* add a null char; make a C string */
      start = -1;
      break;
    case '\n':                 /* should be the final char examined */
      if (start != -1){
        args[ct] = &inputBuffer[start];
  	    ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL; /* no more arguments to this command */
      break;
    default :             /* some other character */
      if (inputBuffer[i] == '&'){
        *background  = 1;
        inputBuffer[i] = '\0';
      } else if (start == -1)
        start = i;
      }
  }
  args[ct] = NULL; /* just in case the input line was > MAX_LINE */
}

int changeDirectory(char* path) {
  int status = 0;
  if (path == NULL) { //"cd" (default to home)
    status = chdir(getenv("HOME"));
  } else {
    status = chdir(path);
  }
  return status;
}

int main(void) {
  char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
  int background;
  char *args[MAX_LINE/+1];
  /* equals 1 if a command is followed by '&' */
  /* command line (of 80) has max of 40 arguments */
  while (1){
    background = 0;
    printf(" COMMAND->\n");
    setup(inputBuffer,args,&background);
    /* Program terminates normally inside setup */ /* get next command */
    /* the steps are:
    (1) fork a child process using fork()
    (2) the child process will invoke execvp()
    (3) if background == 1, the parent will wait,
    otherwise returns to the setup() function. */
    // appendToHist(args);

    int shouldAddCommand = 1;
    int shouldExecuteLater = 1;

    if (strcmp(args[0], "r") == 0) {
      if (args[1] == NULL) { //"r" command
        copyExactStrArr(args, tail->data);
      } else { //'r x" command'
        char** lastCommandStartingWithX = NULL;
        temp = head;
        while(temp != NULL) {
          if (temp -> data[0][0] == args[1][0]) {
            lastCommandStartingWithX = temp -> data;
          }
          temp = temp -> next;
        }
        if (lastCommandStartingWithX == NULL) {
          printf("No Command Starting with '%s'.\n", args[1]);
          shouldAddCommand = 0;
        } else {
          copyExactStrArr(args, lastCommandStartingWithX);
        }
      }
    }

    if (strcmp(args[0], "cd") == 0) {
      //TODO: cd ~, cd -?
      // printf("Custom CD\n");
      shouldExecuteLater = 0;
      if (changeDirectory(args[1]) == -1) {
        printf("Error changing directory.\n");
      } else {
        //TODO: PWD because CD successful
      }
    }

    if (strcmp(args[0], "history") == 0) {
      showHistory();
    }
    if (shouldAddCommand) {
      appendToHist(args);
    }

    printf("command: %s, first param: %s\n", args[0], args[1]);

    int childPid = fork();

    if (childPid == 0) {


      // executeCommand(args);
      //
      if (shouldExecuteLater) {
        execvp(args[0], args);
      }



      // showHistory();

    } else if (background == 0) {
      int status;
      waitpid(childPid, &status, 0);
    } else {
      printf("in parent");
    }


    // showHistory();
  }

  return 0;

}
