// wppp wings ppp dialer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <winlib.h>
#include <xmldom.h>
#include <exception.h>
#include <winforms.h>

typedef struct Account {
TNode tnode;
char *Name;
char *Username;
char *Password;
char *Number;
} Account;

extern char* getappdir();
DOMElement * configdoc;
DOMElement * configxml;

HTMLTable * MainTable;
HTMLTable * AccTable;
JWin *AccWindow;

XMLGuiMap AccMap[] = {
    {"@name", "name", OFFSET16(Account, Name), T_STRING},
    {"username", "username", OFFSET16(Account, Username), T_STRING},
    {"password", "password", OFFSET16(Account, Password), T_STRING},
    {"number", "number", OFFSET16(Account, Number), T_STRING},
};

void updateInfo(Account *account);


void changedAccount(TNode *tnode)
{
    Account *account = (Account *)tnode;
    printf("Changed to '%s'\n", account->Name);
    updateInfo(account);
}

void enterNew()
{
    printf("Hello!\n");
    JWinShow(AccWindow);
}

void custom(HTMLCell *Cell, void *Accounts)
{
    JCombo *combo;
    if (!strcmp("account", Cell->Name))
    {
	combo = JComboInit(NULL, (TModel *)Accounts, (uint32)OFFSET(Account, Name), JColF_STRING);
	combo->Changed = changedAccount;
	Cell->Win = combo;
    }
    else if (!strcmp("new", Cell->Name))
    {
	JBut *but = Cell->Win;
	but->Clicked = enterNew;
    }
}

void updateInfo(Account *account)
{    
    JMapBind(MainTable, &AccMap[1], 2);    
    JMapToGUI(account, &AccMap[1], 2);
}

void loadAccounts(DOMElement *accounts, JLModel *accmodel)
{
    DOMElement *account;
    Account *newacc;
    for (account = accounts->Elements; account; account = XMLnextElem(account))
    {
	newacc = calloc(sizeof(Account), 1);
	JMapFromXML(newacc, account, AccMap, 4);
	JLModelAppend(accmodel, (TNode *)newacc);
    }
}

void loadConfig(char *name)
{
    int ex;
    void *exp;
    
    Try
    {
        configdoc = XMLloadFile(name);
    }
    Catch2 (ex, exp)
    {
	configdoc = XMLnewDoc();
	errexc(ex, exp);
    }
    configxml = XMLgetNode(configdoc, "xml");
    if (!configxml)
    {
	configxml = XMLnewNode(NodeType_Element, "xml", "");
	XMLinsert(configdoc, NULL, configxml);
    }
}


void main(int argc, char *argv[])
{
    JW *app, *mainwin, *accwin;
    int ex;
    void *exp;
    HTMLTable *Table;
    HTMLForms *Forms;
    SizeHints sizes;
    Try
    {
	JLModel *Accounts;
	DOMElement *AccRoot;
	
	chdir(getappdir());
	Accounts = JLModelInit(NULL);
	
	loadConfig("config.xml");
	
	AccRoot = XMLgetNode(configxml, "accounts");
	if (!AccRoot)
	{
	    AccRoot = XMLnewNode(NodeType_Element, "accounts", "");
	    XMLinsert(configxml, NULL, AccRoot);
	}
	loadAccounts(AccRoot, Accounts);
	
	Forms = JFormLoad("layout.xml");
	app = JAppInit(NULL, 0);

	accwin = JWndInit(NULL, "Edit Account", JWndF_Resizable);
	AccTable = JFormGetTable(Forms, "account");
	JCntAdd(accwin, JFormCreate(AccTable, NULL, NULL));
	JWinGetHints(accwin, &sizes);
	JWSetBounds(accwin, accwin->X, accwin->Y, sizes.PrefX, sizes.PrefY);
	AccWindow = accwin;
	
	
	mainwin = JWndInit(NULL, "Wings PPP Dialer", JWndF_Resizable);
	Table = JFormGetTable(Forms, "main");	
	JCntAdd(mainwin, JFormCreate(Table, custom, Accounts));
	JWinGetHints(mainwin, &sizes);
	JWSetBounds(mainwin, mainwin->X, mainwin->Y, sizes.PrefX, sizes.PrefY);
	MainTable = Table;
	
	
	JWinShow(mainwin);
	JAppLoop(app);
    }
    Catch2 (ex,exp)
    {
    	errexc(ex, exp);
    }

}


