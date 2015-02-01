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

void enqueue(char* command[]) {
  char **copyCommand = malloc(sizeof(char*)*MAX_LINE/+1);
  // printf("i: %d strlen(args[0]): %lu\n", 0, strlen(args));
  // printf("lenght of args: %lu", sizeof(args)/sizeof(args[0]));
  for (int i = 0; i < MAX_LINE/2; i++) {
    if (command[i] == NULL) {
      break;
    }
    copyCommand[i] = malloc(strlen(command[i]) + 1);
    strcpy(copyCommand[i], command[i]);
  }

  if (head == NULL) {
    // printf("null head\n");
    head = (struct node *)malloc(1*sizeof(struct node));
    head -> id = 1;
    head -> data = copyCommand;
    head -> next = NULL;

    tail = head;
    // printf("command(LL HEAD): %s, param: %s\n", head->data[0], head->data[1]);
    numCommands = 1;
  } else {
    //same command pointer??
    // printf("head command 1(LL): %s, param: %s\n", head->data[0], head->data[1]);

    temp = (struct node *)malloc(1*sizeof(struct node));

    temp -> id = tail -> id + 1;
    // printf("head command 2(LL): %s, param: %s\n", head->data[0], head->data[1]);

    tail -> next = temp;
    // printf("head command 3(LL): %s, param: %s\n", head->data[0], head->data[1]);

    temp -> data = copyCommand;
    temp -> next = NULL;
    // printf("head command 4(LL): %s, param: %s\n", head->data[0], head->data[1]);

    // printf("command(LL): %s, param: %s\n", temp->data[0], temp->data[1]);

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
    printf("No History Yet.");
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
  printf("before appending to hist");
  enqueue(command);
  if (numCommands > 11) { //11 because after numCommands incremented
    dequeue();
  }
  printf("appended to hist");
}

void executeCommand(char* command[]) {
  printf("before executing command");
  appendToHist(command);
  printf("command: %s, first param: %s", command[0], command[1]);

  execvp(command[0], command);
}

/**
 * setup() reads in the next command line, separating it into distinct
 * tokens using whitespace (space or tab) as delimiters. setup() sets
 * the args parameter as a null-terminated string.
 */
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


    printf("command: %s, first param: %s", args[0], args[1]);

    // int childPid = fork();

    // if (childPid == 0) {


      executeCommand(args);



      // showHistory();

    // } else if (background == 0) {
    //   int status;
    //   waitpid(childPid, &status, 0);
    // } else {
      // printf("in parent");
    // }
  }

  return 0;

}
