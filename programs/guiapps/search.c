#include <winlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <wgslib.h>
#include <stdlib.h>

//These Widget handles are global... so they can be accessed
//primitively by different routines without the need to pass them
//as part of a widgets data pointer.

void *txtbar, *display, *mainwin, *scroll, *txtcard;
char *param;
int total = 0;

unsigned char app_icon[] = {
60,66,153,133,129,129,66,61,
0,0,0,0,0,0,0,128,
1,0,0,0,0,0,0,0,
192,224,112,56,28,14,4,0,
0x01,0x01,0x01,0x01
};

//Forward declaration of some functions.

void beginsearch();
void search();

//This function is triggered by an overridden widget event, 
//and will start the function "beginsearch()" running in a new thread.

void startthread() {
  newThread(beginsearch, 1024, NULL); 
} 

void main(int argc, char * argv[]) {
  void *appl, *searchbut, *exitbut, *inputcnt;
  int region,ch = 0;
  RegInfo props;
  JMeta * metadata = malloc(sizeof(JMeta));

  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = "Search";
  metadata->icon = app_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  appl    = JAppInit(NULL,0);
  mainwin = JWndInit(NULL, metadata->title, JWndF_Resizable,metadata);
  JAppSetMain(appl, mainwin);

  JWSetBounds(mainwin,80,40,168,56);
  JWSetMin(mainwin,112,40);
  JWndSetProp(mainwin);  

  region = ((JW*)mainwin)->RegID;
  JRegInfo(region,&props);

  //Containers can be either vertical or horizontal. 
  //as widgets are added to a container they line up one after the next.
  //Containers of differing orientation are nested to create elaborate
  //layouts of widgets on a window.

  //by default the orientation of a widget is to be horizontal.
  //So, override the window, (caste as a container) to be a vertical
  //container.

  ((JCnt*)mainwin)->Orient = JCntF_TopBottom;

  //initialize a new container. Which is horizontal by default.

  inputcnt = JCntInit(NULL);

  //initialize a multi-line text area.

  display  = JTxtInit(NULL);

  //Initialize a scrollable area with the textarea inside.

  scroll = JScrInit(NULL, display, 0);

  //Set the background and forground colors for the text area. 

  JWSetBack(display, COL_LightGreen);
  JWSetPen(display, COL_Black);

  //add the horizontal container to the window container First. This will
  //put the horizontal container at the top. Add the scrollable multiline
  //textarea to the window container second.. this puts the textarea below
  //the horizontal container.

  JCntAdd(mainwin, inputcnt);
  JCntAdd(mainwin, scroll);

  //initialize a TextInput line.

  txtbar = JTxfInit(NULL);
  JWSetPen(txtbar,COL_Black);
  JWSetBack(txtbar,COL_White);

  JWSetMax(txtbar,32767,16);

  //add the text input line to the horizontal container. This puts the 
  //textbar the upper left corner of the window.

  JCntAdd(inputcnt, txtbar);

  //Override the "Entered" event of the widget whose handle is "txtbar", 
  //and whose class is "JTxf", to run the function "startthread()". So if 
  //you click the text bar, type some characters and press return, it 
  //will run the function "startthread". This function was at the top of
  //the source and will infact start a new thread.

  JWinCallback(txtbar, JTxf, Entered, startthread);

  //initialize a button, with the text "Search" on it.

  searchbut = JButInit(NULL, "Search"); 

  //Add the search button the horizontal container. Which places the 
  //button at the top of the window, but to the right of the text bar.

  JCntAdd(inputcnt, searchbut);

  //Override the "Clicked" event of the widget "searchbut", whose class is
  //"JBut", and run the function "startthread". Clicking the search button
  //does the same as pressing return after typing the text to search for.

  JWinCallback(searchbut, JBut, Clicked, startthread);  

  //Show the window, returnexit() and start the JApp Event loop.

  JWinShow(mainwin);
  JWReqFocus(txtbar);
  retexit(1);
  JAppLoop(appl);
} 

//Beginsearch will eventually run in a new thread, when one of the 
//overridden events is triggered from above.

void beginsearch() {
  char * matches;

  //start by clearing all the text from the multiline text area.

  JTxtClear(display);

  //set param as a pointer to a section of memory where we put a copy of 
  //the text on in the textbar. We make a copy so if the user changes the
  //textbar contents while the search is going on... it won't cause any
  //problems.

  param = strdup(JTxfGetText(txtbar));

  //if the string length of the param is 0, print into the multiline text
  //area the message about how to use the program.

  if(strlen(param) < 1) {
    JTxtAppend(display, "You must enter a search parameter.\n");

  //Otherwise, Run the search function... remember this is all being done
  //in a seperate thread from the GUI's User Interface thread. So the GUI 
  //can update it's display even while the program is busy doing the search.

  } else {
    search("/");

  //When the search is complete, print to the end of the textarea that the
  //search is complete, and compose a string that says how many matches 
  //were found and add that to the end as well.

    JTxtAppend(display, "Search Complete");
    matches = malloc(strlen(" (  matches found)") + 20);
    sprintf(matches, " (%d matches found)\n", total);
    JTxtAppend(display, matches);

  //Reset the total number of matches to zero, in preparation for the 
  //next search. 

    total = 0;
  }

  //Free the memory used for the copy of the text in the textbar.

  free(param);
}

//The meat and potatoes Search function.

void search(char * dirpath) {
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char * fullname = NULL;
  char * nextdir  = NULL;

  //open the directory path passed in as an argument.

  dir = opendir(dirpath);

  //if the directory fails to open, skip all this.

  if(dir != NULL) {

    //Until you reach the end of the directory, keep looping and 
    //handling the directory entries one at a time. 

    while(entry = readdir(dir)) {

      //Do a string inside string compare. If the directory entry matches
      //The search parameter, increment the total matches, print the 
      //current directory path, then the directory entry name, and a
      //newline character to the textarea... Which is a global variable 
      //remember.

      if(strcasestr(entry->d_name, param)) {
        total++;
        JTxtAppend(display, dirpath);
        JTxtAppend(display, entry->d_name);
        JTxtAppend(display, "\n");
      }

      //If the current entry is another directory, compose the new path 
      //to search in, and recurse and repeat. 

      if(entry->d_type == 6) {
        nextdir = (char *)malloc(strlen(dirpath) + strlen(entry->d_name) +3);
        sprintf(nextdir, "%s%s/", dirpath, entry->d_name);

        search(nextdir);
        free(nextdir);
      }
    }

    //close the directories as you come to the end of them.

    closedir(dir);
  }

  //Eventually we make it through all directories in the FileSystem, 
  //and every search match will be listed with complete path, in the 
  //textarea, and the total number of matches will be displayed as well.
}

//The program only quits when you kill the main window by clicking its 
//close icon.
