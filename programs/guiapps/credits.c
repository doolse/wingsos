#include <winlib.h>

int main() {
	void *Appl,*Window,*TxtArea;
	
	Appl = JAppInit(NULL,0);
	Window = JWndInit(NULL, NULL, 0, "Welcome to the gui", JWndF_Resizable);
	JAppSetMain(Appl, Window);
	JWinGeom(Window, 32, 32, 32, 32, GEOM_TopLeft | GEOM_BotRight2);
	
	TxtArea = JTxtInit(NULL, Window, 0, "");
	JWinGeom(TxtArea, 0, 0, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
	JTxtAppend(TxtArea, 
	"\nThis is the gui in a very very alpha stage!\n"
	"So there isn't much point comparing it to GEOS just yet.\n"
	"Out of all the code in Jos, the GUI would have been worked on only about "
	"10% of the time, and some parts have undergone rewrites a few times now.\n"
	"For example none of the window buttons work at the moment, nor do the file "
	"browser icons, and you can only select a window by clicking on it's "
	"titlebar. Also you must use a 1351 compatible mouse in port 1.\n\n"
	"While I'm here, it's a good opportunity to show the credits and some "
	"thankyou's\n\n"
	"Code:\n\n"
	"CMD HD Drivers - Doc Bacardi/The Dreams\n"
	"html to text program - Soci/Singular\n"
	"uptime, guestbook - Warlock\n"
	"mvp - Craig Bruce\n"
	"Everything else - Jolse Maginnis aka Jolz/Onslaught\n\n"
	"Hardware support:\n\n"
	"Josef Soucek and Tomas Pribyl - IDE64 and Duart (driver help too)\n"
	"Greg Nacu (Greg\DAC) - CMD HD\n"
	"Colin J Thomson - SIMM RAM\n"
	"Dave Ross (Watson on IRC) - 1351 Mouse\n\n"
	"Authors of tools I use:\n\n"
	"Andre Fachat - XA cross assembler and .o65 binary format\n"
	"?? - LCC 4.1\n"
	"Doc Bacardi - Dreamon\n"
	"Steve Judd - Jammon\n"
	"Marko Makela  - prlink and cbmconvert\n"
	"Olaf Seibert - prlink\n"
	"Michael Klein - CBM4Linux\n"
	"Pasi Ojala - pucrunch\n\n"
	"Additional help:\n"
	"Nate Dannenberg (Nate\DAC) - 80 Column font, the small gui font, and "
	"debugging help on IRC\n"
	"Wolfram (Ninja/The Dreams) - Testing\n"
	"Nathan Smith (Stryyker/Onslaught) - Almost writing FD drivers :)\n"
	"I gotta say a big thankyou to Greg Nacu, who was the first person to have "
	"the same vision of what the future may hold for our CBM's and has thrown "
	"his full support behind the project, spent many hours helping me debug Jos "
	"at all hours of the night, and even sent me his CMD HD to lend, "
	"so I could finally release Jos in this early alpha. Thanks Greg!\n\n"
	"Also thanks must go to Amund Gjerde Gjendem for setting up "
	"http://c64.nvg.org\n\n"
	"People who've helped to inspire by writing me emails:\n"
	"(Looking through old mails..)\n"
	"Marco Baye - ACME\n"
	"Alan R Dickey\n"
	"Maurice Randall\n"
	"Craig S. Levay\n"
	"Danny Todd\n"
	"Jazzcat\Onslaught\n"
	"Errol Smith\n"
	"Dustin Chambers\n"
	"Donald Zerbe\n"
	"David Wood (Jbev)\n"
	"Jeri Ellsworth\n"
	"K Dale Sidebottom\n"
	"Myke Carter\n"
	"Per Olofsson (MagerValp)\n"
	"Professor Dredd\n"
	"Richard Balkins\n"
	"Rowan Jeff\n"
	"Todd Elliot\n"
	"Ullrich Von Bassewitz\n"
	"Anyone on the Jos Discussion List!\n\n"
	"Last but certainly not least, I'd like to thank my girlfriend Anna "
	"(Astra!), who has been with for almost 3 years now, and has kept me sane "
	"and extremely happy :)\n"
	"\nJolse Maginnis\n"
	);
	JWinSetBack(TxtArea, COL_MedGrey);
	JBarSetVal(JTxtVBar(TxtArea), 0L, 1);
	JWinShow(Window);
	JAppLoop(Appl);
}

