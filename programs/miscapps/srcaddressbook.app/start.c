#include <string.h>
#include <stdio.h>
#include <xmldom.h>
#include <stdlib.h>
#include <unistd.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <fcntl.h>
extern char * getappdir();

#define GET_ATTRIB      225
#define GET_ALL_LIST    226
#define MAKE_ENTRY      227
#define PUT_ATTRIB      228
#define DEL_ATTRIB      229
#define DEL_ENTRY       230

#define NOENTRY  -2
#define ERROR    -1
#define SUCCESS   0

#define EXACT    0
#define NEAREST  1

#define BACKWARD    -1
#define NODIRECTION  0 
#define FORWARD      1

typedef struct msgpass_s {
  int code;
  char * lastname;
  char * firstname;
  char * type;
  char * buf;
  int bufsize;
} msgpass;

DOMElement * xmlfile, * xmlelem;
msgpass * msg;
int direction;

DOMElement * queryforelement(char *lastname, char *firstname, int nearest);
DOMElement * pinpointelement(DOMElement *, char * firstname, char * lastname, int searchtype); 

int getattrib();
int getalllist();
int makeentry();
int putattrib();
int deleteentry();
int deleteattrib();

void main(int argc, char * argv[]) {
  int channel, rcvid, returncode;

  xmlfile = XMLloadFile(fpathname("data/addressbook.xml", getappdir(), 1));
  xmlelem = XMLgetNode(xmlfile, "xml");

  channel = makeChanP("/sys/addressbook");

  retexit(1);

  while(1) {
    rcvid = recvMsg(channel,(void *)&msg); 

    switch(msg->code) {
      case GET_ATTRIB:
        returncode = getattrib();
      break;
      case GET_ALL_LIST:
        returncode = getalllist();
      break;
      case MAKE_ENTRY:
        returncode = makeentry();
      break;
      case PUT_ATTRIB:
        returncode = putattrib();
      break;
      case DEL_ENTRY:
        returncode = deleteentry();
      break;
      case DEL_ATTRIB:
        returncode = deleteattrib();
      break;
      case IO_OPEN:
        if(*(int *)( ((char*)msg) +6) & (O_PROC|O_STAT))
          returncode = makeCon(rcvid, 1);
        else 
          returncode = -1;
      break;
    }

    replyMsg(rcvid, returncode);    
  }
} 

int getattrib() {
  DOMElement * element;

  element = queryforelement(msg->lastname, msg->firstname, EXACT);
  if(!element)
    return(NOENTRY);
  
  if((strlen(XMLgetAttr(element, msg->type))+1) > msg->bufsize) 
    return(ERROR);

  strcpy(msg->buf, XMLgetAttr(element, msg->type));
  return(SUCCESS);
}

int getalllist() {
  int total = 2;
  DOMElement * currentelem;

  currentelem = XMLgetNode(xmlelem, "entry");

  //fprintf(stderr, "%s %s\n", XMLgetAttr(currentelem, "firstname"),XMLgetAttr(currentelem, "lastname"));
  total = strlen(XMLgetAttr(currentelem, "firstname")) + strlen(XMLgetAttr(currentelem, "lastname")) + 2 + total;

  currentelem = currentelem->NextElem; 

  while(currentelem->FirstElem != 1) {
    //fprintf(stderr, "%s %s\n", XMLgetAttr(currentelem, "firstname"),XMLgetAttr(currentelem, "lastname"));
    total = strlen(XMLgetAttr(currentelem, "firstname")) + strlen(XMLgetAttr(currentelem, "lastname")) + 2 + total;
    currentelem = currentelem->NextElem;
  }

  if(msg->bufsize < total) {
    sprintf(msg->buf, "%d", total);
    return(ERROR);
  } else {
    currentelem = XMLgetNode(xmlelem, "entry");

    msg->buf[0] = 0; //initialize string.

    strcat(msg->buf, XMLgetAttr(currentelem, "firstname"));
    strcat(msg->buf, " ");
    strcat(msg->buf, XMLgetAttr(currentelem, "lastname"));

    currentelem = currentelem->NextElem; 

    while(currentelem->FirstElem != 1) {
      strcat(msg->buf, ",");
      strcat(msg->buf, XMLgetAttr(currentelem, "firstname"));
      strcat(msg->buf, " ");
      strcat(msg->buf, XMLgetAttr(currentelem, "lastname"));
      currentelem = currentelem->NextElem;
    }    
    return(SUCCESS);
  }
}

int makeentry() {
  DOMElement * element, * newentry;

  element = queryforelement(msg->lastname, msg->firstname, NEAREST);

  if(element) {
    if(! strcasecmp(XMLgetAttr(element, "lastname"), msg->lastname)) {
  
      //if firstname is blank , and the lastname found, return an error code.

      if(! strlen(msg->firstname))
        return(ERROR);

      //if the firstname and lastname both already match, return an error code

      if(! strcasecmp(XMLgetAttr(element, "firstname"), msg->firstname))
        return(ERROR);  
    }
  }

  newentry = XMLnewNode(NodeType_Element, "entry", "");
  XMLinsert(xmlelem, element, newentry);

  XMLsetAttr(newentry, "lastname", strdup(msg->lastname));
  XMLsetAttr(newentry, "firstname", strdup(msg->firstname));

  XMLsaveFile(xmlelem, fpathname("data/addressbook.xml", getappdir(), 1));
  return(SUCCESS);
}

int putattrib() {
  DOMElement * element;
  char * type, * data;

  element = queryforelement(msg->lastname, msg->firstname, EXACT);
  if(!element)
    return(NOENTRY);
  
  type = strdup(msg->type);
  data = strdup(msg->buf);

  XMLsetAttr(element, type, data);
  XMLsaveFile(xmlelem, fpathname("data/addressbook.xml", getappdir(), 1));
  return(SUCCESS);
}

int deleteentry() {
  DOMElement * element;

  element = queryforelement(msg->lastname, msg->firstname, EXACT);
  if(!element)
    return(NOENTRY);
  
  XMLremNode(element);
  XMLsaveFile(xmlelem, fpathname("data/addressbook.xml", getappdir(), 1));
  return(SUCCESS);
}

int deleteattrib() {
  DOMElement * element;
  DOMNode * attrib;

  element = queryforelement(msg->lastname, msg->firstname, EXACT);
  if(!element)
    return(NOENTRY);
  
  attrib = XMLfindAttr(element, msg->type);
  if(attrib) {
    XMLremNode(attrib);
    XMLsaveFile(xmlelem, fpathname("data/addressbook.xml", getappdir(), 1));
  }
  return(SUCCESS);
}

DOMElement * pinpointelement(DOMElement *entryelemptr, char * firstname, char * lastname, int searchtype) {
  DOMElement * spansfromentry, * spanstoentry;

  //Search for multiple same lastnames in Previous entries
  spansfromentry = entryelemptr;
  while(!strcasecmp(lastname, XMLgetAttr(spansfromentry->PrevElem, "lastname"))) 
    spansfromentry = spansfromentry->PrevElem;
	
  //Search for multiple same lastnames in Next entries
  spanstoentry = entryelemptr;
  while(!strcasecmp(lastname, XMLgetAttr(spanstoentry->NextElem, "lastname")))
    spanstoentry = spanstoentry->NextElem;	

  entryelemptr = NULL;
	
  if(strlen(firstname)) {
    while(spansfromentry != spanstoentry->NextElem) {
      if(!strcasecmp(firstname, XMLgetAttr(spansfromentry, "firstname"))) {
        entryelemptr = spansfromentry;
        break;
      }
      if(spansfromentry->NextElem->FirstElem)
        break;
      spansfromentry = spansfromentry->NextElem;
    } 
  }
	
  if((searchtype == NEAREST && entryelemptr == NULL) || !strlen(firstname)) 
    entryelemptr = spansfromentry;

  return(entryelemptr);
}

DOMElement * queryforelement(char * lastname, char * firstname, int searchtype) {
  DOMElement * entryelem, * tempelem;
  unsigned int numlefttoscan;
  int i; 

  //The Default starting values. 

  numlefttoscan = xmlelem->NumElements; 
  entryelem     = XMLgetNode(xmlelem, "entry");
  tempelem      = NULL;
  direction     = FORWARD;
  
  while(direction) {
    
    direction = strcasecmp(lastname, XMLgetAttr(entryelem, "lastname"));
  
    if(direction == NODIRECTION) {
      entryelem = pinpointelement(entryelem, firstname, lastname, searchtype);
      break;
    } else if(direction < 0) {
      direction = BACKWARD;
    } else {
      direction = FORWARD;
    }

    numlefttoscan /= 2;

    if(numlefttoscan) {

      //There are more to scan... so find the next midpoint. 

      if(direction == FORWARD) {
        for(i = 0; i<numlefttoscan; i++) 
          entryelem = entryelem->NextElem;  
      } else {
        for(i = 0; i<numlefttoscan; i++) {
          if(entryelem->FirstElem) {
            numlefttoscan = 0;
            break;
          } 
          entryelem = entryelem->PrevElem;
        }
      }
			
    //Scanning is finished... account for inaccuracies.
			
    } else {
      if(direction == FORWARD) {
      	while(1) {
          if(entryelem->NextElem->FirstElem) {
            entryelem = NULL;
            break;  
          }
          entryelem = entryelem->NextElem;  
          direction = strcasecmp(lastname, XMLgetAttr(entryelem, "lastname"));
          if(!direction) {
            entryelem = pinpointelement(entryelem, firstname, lastname, searchtype);
	    break;					
          }

          //If the direction changes... we're at the insert point for nearest
          if(direction < 0) {
            if(searchtype == EXACT)
              entryelem = NULL;
            break;
          }
        }
      } else {
        while(1) {
          if(entryelem->FirstElem) {
            if(searchtype == EXACT)
              entryelem = NULL;  
            break;
          }

          entryelem = entryelem->PrevElem;
          direction = strcasecmp(lastname, XMLgetAttr(entryelem, "lastname"));
          if(!direction) {
            entryelem = pinpointelement(entryelem, firstname, lastname, searchtype);
            break;					
          } 

          //If the direction changes... then we've gone too far.
          if(direction > 0) {
            if(searchtype == EXACT)
              entryelem = NULL;
            else
              entryelem = entryelem->NextElem;
            break;
          }
        }
      }

      //Since there are none left to scan, break out of the while loop.
      break;
    }

  } //End of While(direction);

  return(entryelem);
}
