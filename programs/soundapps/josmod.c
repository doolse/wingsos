#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <string.h>
#include <sys/types.h>

/* 
Josmod by Jolse Maginnis
Some tables and structures are ripped from Judas(XM/S3M) player.
*/

#define HIGHPERIOD 108
#define LOWPERIOD 29015

#define SBUFSIZE 32000L
// #define DEBUG
// #define DEBUGSAMP
// #define DEBSAMOUT
// #define SHOWNOTES

typedef struct {
	char *Samp;
	char *End;
	uint32 Replen;
	uint32 Speed;
	int Low;
	int Repeats;
	int *VolTab;
	int Active;
} MixChan;

typedef struct {
        char songname[28];
        uchar byte1a;
        uchar filetype;
        uint16 unused1;
        uint16 ordernum;
        uint16 instnum;
        uint16 pattnum;
        uint16 flags;
        uint16 createdwith;
        uint16 fileversion;
        char id[4];
        uchar globalvol;
        uchar initialspeed;
        uchar initialtempo;
        uchar mastermult;
        uchar ultraclicks;
        uchar panningmagic;
        uchar unused[8];
        uint16 special;
        uchar chsettings[32];
} S3MDisk;

typedef struct {
        uchar type;
        char dosname[12];
        uchar paraptrhigh;
        uint16 paraptr;
        uint32 length;
        uint32 loopstart;
        uint32 loopend;
        char vol;
        uchar disknum;
        uchar pack;
        uchar flags;
        uint32 c2rate;
        uint32 unused;
        uint16 guspos;
        uint16 int512;
        uint32 lastused;
        char name[28];
        char id[4];
} S3MInst;

typedef struct {
	char Name[28];
	char *Samp;
	char *End;
	uint Replen;
	int Looped;
	int Finetune;
	uint32 FilePos;
	uint Length;
	uint Volume;
} S3MSamp;

typedef struct {
	uchar note;
	uchar voleffect;
	uchar instrument;
	uchar effect;
	uchar effectdata;
} S3MNote;

typedef struct {
	uint x;
	uint y;
} Env;

#define XMSF_16BIT	16
#define XMSF_PING	2
#define XMSF_LOOPED	1

#define XMKEYOFF	97

#define ENV_ON		1
#define ENV_SUS		2
#define ENV_LOOP	4

typedef struct {
	uint32 length;
	uint32 loopstart;
	uint32 looplength;
	uchar vol;
	char finetune;
	uchar type;
	uchar panning;
	char relative;
	char nothing;
	char name[22];
} XMSampHead;

typedef struct {
	XMSampHead Hd;
	char *Samp;
	char *End;
	uint32 Replen;
	int Looped;
} XMSamp;

typedef struct {
	uint32 headersize;
	uchar sampnum[96];
	Env VolEnv[12];
	Env PanEnv[12];
	uchar numvol;
	uchar numpan;
	uchar volsustain;
	uchar volstart;
	uchar volend;
	uchar pansustain;
	uchar panstart;
	uchar panend;
	uchar voltype;
	uchar pantype;
	uchar vibtype;
	uchar vibsweep;
	uchar vibdepth;
	uchar vibrate;
	uint16 volfade;
} XMInstHead2;

typedef struct {
	uint32 headersize;
	char name[22];
	uchar type;
	uint numsamples;
} XMInstHead;

typedef struct {
	XMInstHead Hd;
	XMInstHead2 Hd2;
	XMSamp *Samples;
	uchar VEnv[1024];
	int VStart;
	int VEnd;
	int type;
} XMInst;

typedef struct {
        uint32 headersize;
        uchar packingtype;
        uint16 rows;
        uint16 packsize;
} XMPatHead;

typedef struct {
        char id[17];
        char modname[20];
        uchar idbyte;
        char trackername[20];
        uint16 version;
} XMId;

typedef struct {
        uint32 headersize;
        uint16 songlength;
        uint16 restartpos;
        uint16 channels;
        uint16 patterns;
        uint16 instruments;
        uint16 uselinear;
        uint16 defaulttempo;
        uint16 defaultbpmtempo;
        uchar order[256];
} XMDisk;

typedef struct {
        uchar note;
        uchar instrument;
        uchar voleffect;
        uchar effect;
        uchar effectdata;
} XMNote;

typedef struct {
	uint rows;
	XMNote *notes;
} XMPatPtr;

typedef struct {
	uint LastInst;
	uint Effect;
	uint EffParm;
	int Volume;
	int LVolume;
	int Period;
	int LPeriod;
	int Target;
	uint TpSpeed;
	uint PortSpeed;
	uint PuSpeed;
	uint PdSpeed;
	int Slide;
	int SlideNow;
	int Porta;
	XMInst *LastXM;
	S3MSamp *LastS3M;
	int Flags;
	int Keyon;
	uint VEnvUp;
	uchar VibUp;
	uchar IVibUp;
	uint VibSpeed;
	int VibDepth;
	int VibOn;
	int SlideOn;
	uint VibType;
} Channel;

typedef struct {
	S3MDisk Hd;
	uchar Orders[256];
	uint16 InstPtr[100];
	uint16 PatPtr[100];
	S3MSamp *Instruments;
	S3MNote *Patterns;
	uint PatSize;
	uint LineSize;
	uint Hz;
	uint BufSize;
	char *SoundBuf[2];
	int Init;
	uint Channels;
	Channel Chans[32];
	MixChan MChans[32];
} S3MHead;

typedef struct {
	XMId ID;
	XMDisk Hd;
	XMPatPtr *Patterns;
	XMInst *Instruments;
	Channel Chans[32];
	MixChan MChans[32];
	char *SoundBuf[2];
	uint BufSize;
	int Init;
	uint Hz;
	uint32 *linTable;
} XMHead;

typedef struct ModHeader {
	char Name[21];
	char Type[5];
	int NumOrders;
	int NumPatterns;
	int Channels;
	int PatSize;
	int LineSize;
	int Init;
	uchar Orders[128];
	uchar *Patterns;
	char *SoundBuf[2];
	uint BufSize;
	uint Hz;
	S3MSamp Samples[31];
	Channel Chans[8];
	MixChan MChans[8];
} ModHead;

typedef struct IdentStruct {
	char *String;
	int channels;
} Ident;

static Ident idents[] = {
        {"2CHN", 2},
        {"M.K.", 4},
        {"M!K!", 4},
        {"FLT4", 4},
        {"4CHN", 4},
        {"6CHN", 6},
        {"8CHN", 8},
        {"CD81", 8}
};

char vibratotable[4][256] =
{
        {
                0, -2, -3, -5, -6, -8, -9, -11, -12, -14, -16, -17, -19, -20, -22, -23,
                -24, -26, -27, -29, -30, -32, -33, -34, -36, -37, -38, -39, -41, -42, -43, -44,
                -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56, -56, -57, -58, -59,
                -59, -60, -60, -61, -61, -62, -62, -62, -63, -63, -63, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -63, -63, -63, -62, -62, -62, -61, -61, -60, -60,
                -59, -59, -58, -57, -56, -56, -55, -54, -53, -52, -51, -50, -49, -48, -47, -46,
                -45, -44, -43, -42, -41, -39, -38, -37, -36, -34, -33, -32, -30, -29, -27, -26,
                -24, -23, -22, -20, -19, -17, -16, -14, -12, -11, -9, -8, -6, -5, -3, -2,
                0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
                24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
                45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
                59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
                59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
                45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
                24, 23, 22, 20, 19, 17, 16, 14, 12, 11, 9, 8, 6, 5, 3, 2
        },
        {
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
                -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64
        },
        {
                0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
                8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
                16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
                24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31,
                32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39,
                40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
                48, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55,
                56, 56, 57, 57, 58, 58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63,
                -64, -64, -63, -63, -62, -62, -61, -61, -60, -60, -59, -59, -58, -58, -57, -57,
                -56, -56, -55, -55, -54, -54, -53, -53, -52, -52, -51, -51, -50, -50, -49, -49,
                -48, -48, -47, -47, -46, -46, -45, -45, -44, -44, -43, -43, -42, -42, -41, -41,
                -40, -40, -39, -39, -38, -38, -37, -37, -36, -36, -35, -35, -34, -34, -33, -33,
                -32, -32, -31, -31, -30, -30, -29, -29, -28, -28, -27, -27, -26, -26, -25, -25,
                -24, -24, -23, -23, -22, -22, -21, -21, -20, -20, -19, -19, -18, -18, -17, -17,
                -16, -16, -15, -15, -14, -14, -13, -13, -12, -12, -11, -11, -10, -10, -9, -9,
                -8, -8, -7, -7, -6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1
        },
        {
                0, 0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7,
                -8, -8, -9, -9, -10, -10, -11, -11, -12, -12, -13, -13, -14, -14, -15, -15,
                -16, -16, -17, -17, -18, -18, -19, -19, -20, -20, -21, -21, -22, -22, -23, -23,
                -24, -24, -25, -25, -26, -26, -27, -27, -28, -28, -29, -29, -30, -30, -31, -31,
                -32, -32, -33, -33, -34, -34, -35, -35, -36, -36, -37, -37, -38, -38, -39, -39,
                -40, -40, -41, -41, -42, -42, -43, -43, -44, -44, -45, -45, -46, -46, -47, -47,
                -48, -48, -49, -49, -50, -50, -51, -51, -52, -52, -53, -53, -54, -54, -55, -55,
                -56, -56, -57, -57, -58, -58, -59, -59, -60, -60, -61, -61, -62, -62, -63, -63,
                64, 64, 63, 63, 62, 62, 61, 61, 60, 60, 59, 59, 58, 58, 57, 57,
                56, 56, 55, 55, 54, 54, 53, 53, 52, 52, 51, 51, 50, 50, 49, 49,
                48, 48, 47, 47, 46, 46, 45, 45, 44, 44, 43, 43, 42, 42, 41, 41,
                40, 40, 39, 39, 38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 33, 33,
                32, 32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 25, 25,
                24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17,
                16, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9,
                8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1
        }
};

uint32 linearfreqs[] =
{
        535232, 534749, 534266, 533784, 533303, 532822, 532341, 531861, 531381, 530902,
        530423, 529944, 529466, 528988, 528511, 528034, 527558, 527082, 526607, 526131,
        525657, 525183, 524709, 524236, 523763, 523290, 522818, 522346, 521875, 521404,
        520934, 520464, 519994, 519525, 519057, 518588, 518121, 517653, 517186, 516720,
        516253, 515788, 515322, 514858, 514393, 513929, 513465, 513002, 512539, 512077,
        511615, 511154, 510692, 510232, 509771, 509312, 508852, 508393, 507934, 507476,
        507018, 506561, 506104, 505647, 505191, 504735, 504280, 503825, 503371, 502917,
        502463, 502010, 501557, 501104, 500652, 500201, 499749, 499298, 498848, 498398,
        497948, 497499, 497050, 496602, 496154, 495706, 495259, 494812, 494366, 493920,
        493474, 493029, 492585, 492140, 491696, 491253, 490809, 490367, 489924, 489482,
        489041, 488600, 488159, 487718, 487278, 486839, 486400, 485961, 485522, 485084,
        484647, 484210, 483773, 483336, 482900, 482465, 482029, 481595, 481160, 480726,
        480292, 479859, 479426, 478994, 478562, 478130, 477699, 477268, 476837, 476407,
        475977, 475548, 475119, 474690, 474262, 473834, 473407, 472979, 472553, 472126,
        471701, 471275, 470850, 470425, 470001, 469577, 469153, 468730, 468307, 467884,
        467462, 467041, 466619, 466198, 465778, 465358, 464938, 464518, 464099, 463681,
        463262, 462844, 462427, 462010, 461593, 461177, 460760, 460345, 459930, 459515,
        459100, 458686, 458272, 457859, 457446, 457033, 456621, 456209, 455797, 455386,
        454975, 454565, 454155, 453745, 453336, 452927, 452518, 452110, 451702, 451294,
        450887, 450481, 450074, 449668, 449262, 448857, 448452, 448048, 447644, 447240,
        446836, 446433, 446030, 445628, 445226, 444824, 444423, 444022, 443622, 443221,
        442821, 442422, 442023, 441624, 441226, 440828, 440430, 440033, 439636, 439239,
        438843, 438447, 438051, 437656, 437261, 436867, 436473, 436079, 435686, 435293,
        434900, 434508, 434116, 433724, 433333, 432942, 432551, 432161, 431771, 431382,
        430992, 430604, 430215, 429827, 429439, 429052, 428665, 428278, 427892, 427506,
        427120, 426735, 426350, 425965, 425581, 425197, 424813, 424430, 424047, 423665,
        423283, 422901, 422519, 422138, 421757, 421377, 420997, 420617, 420237, 419858,
        419479, 419101, 418723, 418345, 417968, 417591, 417214, 416838, 416462, 416086,
        415711, 415336, 414961, 414586, 414212, 413839, 413465, 413092, 412720, 412347,
        411975, 411604, 411232, 410862, 410491, 410121, 409751, 409381, 409012, 408643,
        408274, 407906, 407538, 407170, 406803, 406436, 406069, 405703, 405337, 404971,
        404606, 404241, 403876, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
        400970, 400609, 400247, 399886, 399525, 399165, 398805, 398445, 398086, 397727,
        397368, 397009, 396651, 396293, 395936, 395579, 395222, 394865, 394509, 394153,
        393798, 393442, 393087, 392733, 392378, 392024, 391671, 391317, 390964, 390612,
        390259, 389907, 389556, 389204, 388853, 388502, 388152, 387802, 387452, 387102,
        386753, 386404, 386056, 385707, 385359, 385012, 384664, 384317, 383971, 383624,
        383278, 382932, 382587, 382242, 381897, 381552, 381208, 380864, 380521, 380177,
        379834, 379492, 379149, 378807, 378466, 378124, 377783, 377442, 377102, 376762,
        376422, 376082, 375743, 375404, 375065, 374727, 374389, 374051, 373714, 373377,
        373040, 372703, 372367, 372031, 371695, 371360, 371025, 370690, 370356, 370022,
        369688, 369355, 369021, 368688, 368356, 368023, 367691, 367360, 367028, 366697,
        366366, 366036, 365706, 365376, 365046, 364717, 364388, 364059, 363731, 363403,
        363075, 362747, 362420, 362093, 361766, 361440, 361114, 360788, 360463, 360137,
        359813, 359488, 359164, 358840, 358516, 358193, 357869, 357547, 357224, 356902,
        356580, 356258, 355937, 355616, 355295, 354974, 354654, 354334, 354014, 353695,
        353376, 353057, 352739, 352420, 352103, 351785, 351468, 351150, 350834, 350517,
        350201, 349885, 349569, 349254, 348939, 348624, 348310, 347995, 347682, 347368,
        347055, 346741, 346429, 346116, 345804, 345492, 345180, 344869, 344558, 344247,
        343936, 343626, 343316, 343006, 342697, 342388, 342079, 341770, 341462, 341154,
        340846, 340539, 340231, 339924, 339618, 339311, 339005, 338700, 338394, 338089,
        337784, 337479, 337175, 336870, 336566, 336263, 335959, 335656, 335354, 335051,
        334749, 334447, 334145, 333844, 333542, 333242, 332941, 332641, 332341, 332041,
        331741, 331442, 331143, 330844, 330546, 330247, 329950, 329652, 329355, 329057,
        328761, 328464, 328168, 327872, 327576, 327280, 326985, 326690, 326395, 326101,
        325807, 325513, 325219, 324926, 324633, 324340, 324047, 323755, 323463, 323171,
        322879, 322588, 322297, 322006, 321716, 321426, 321136, 320846, 320557, 320267,
        319978, 319690, 319401, 319113, 318825, 318538, 318250, 317963, 317676, 317390,
        317103, 316817, 316532, 316246, 315961, 315676, 315391, 315106, 314822, 314538,
        314254, 313971, 313688, 313405, 313122, 312839, 312557, 312275, 311994, 311712,
        311431, 311150, 310869, 310589, 310309, 310029, 309749, 309470, 309190, 308911,
        308633, 308354, 308076, 307798, 307521, 307243, 306966, 306689, 306412, 306136,
        305860, 305584, 305308, 305033, 304758, 304483, 304208, 303934, 303659, 303385,
        303112, 302838, 302565, 302292, 302019, 301747, 301475, 301203, 300931, 300660,
        300388, 300117, 299847, 299576, 299306, 299036, 298766, 298497, 298227, 297958,
        297689, 297421, 297153, 296884, 296617, 296349, 296082, 295815, 295548, 295281,
        295015, 294749, 294483, 294217, 293952, 293686, 293421, 293157, 292892, 292628,
        292364, 292100, 291837, 291574, 291311, 291048, 290785, 290523, 290261, 289999,
        289737, 289476, 289215, 288954, 288693, 288433, 288173, 287913, 287653, 287393,
        287134, 286875, 286616, 286358, 286099, 285841, 285583, 285326, 285068, 284811,
        284554, 284298, 284041, 283785, 283529, 283273, 283017, 282762, 282507, 282252,
        281998, 281743, 281489, 281235, 280981, 280728, 280475, 280222, 279969, 279716,
        279464, 279212, 278960, 278708, 278457, 278206, 277955, 277704, 277453, 277203,
        276953, 276703, 276453, 276204, 275955, 275706, 275457, 275209, 274960, 274712,
        274465, 274217, 273970, 273722, 273476, 273229, 272982, 272736, 272490, 272244,
        271999, 271753, 271508, 271263, 271018, 270774, 270530, 270286, 270042, 269798,
        269555, 269312, 269069, 268826, 268583, 268341, 268099, 267857
};

uint fineTab[] = {
        7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280, 8363, 8413, 8463, 8529,
        8581, 8651, 8723, 8757
};

uchar convert[32];
uint32 s3mscale;
int VolumeTab[65][256];
char *buf1;
char *buf2;
int globvol;
ModHead globmod;
S3MHead globs3m;
XMHead globxm;

extern void conv2Note(uchar *patp);
extern void mix(char *buf, int size, MixChan *mix, int chans);
extern void prepTables(int *voltab);
extern void getMixer();
extern getPatData(uchar *patp, uint *note, uint *instrument, uint *effect, uint *effpar);
extern long S3MPeriod[254];
extern short AmigaPeriod[296];

uint32 time;

void startTimer() {
	time = getTimer();
}

void endTimer() {
//	printf("%ld milliseconds\n",getTimer()-time);
}

#ifdef SHOWNOTES
char *namenote(uint note) {
	switch(note) {
	case 0: return "c-";
	case 1: return "c#";
	case 2: return "d-";
	case 3: return "d#";
	case 4: return "e-";
	case 5: return "f-";
	case 6: return "f#";
	case 7: return "g-";
	case 8: return "g#";
	case 9: return "a-";
	case 10: return "a#";
	case 11: return "b-";
	}
	return "FU";
}
#endif

void read32(int fd, void *upto, uint32 amount) {
	uint thistime;
	
	while (amount) {
		thistime = 0xff00;
		if (amount < thistime)
			thistime = amount;
		read(fd, upto, thistime);
		amount -= thistime;
		upto = (char *) upto + thistime;
	}
}

void fixSamp(int looped, char *upto, uint32 repsize, uint left) {
	char *from = upto - repsize;
	
	if (looped) {
		while (left) {
			uint thistime;
			if (repsize < left) {
				left -= repsize;
				thistime = repsize;
			} else {
				thistime = left;
				left=0;
			}
			memcpy(upto, from, thistime);
			upto += thistime;
		}
	} else memset(upto, 0, left);
}

void freeS3M(S3MHead *s3m) {
}

void freeMod(ModHead *mp) {
	uint i;
	void *tofree;
	S3MSamp *upto = &mp->Samples[0];
	
	for (i=0;i<31;i++) {
		tofree = upto->Samp;
		if (tofree)
			free(tofree);
		upto++;
	}
	free(mp->Patterns);
}

void playS3amp(MixChan *mix, S3MSamp *sam, uint offset) {
	if (sam->Samp) {
		mix->Active = 1;
		mix->Repeats = sam->Looped;
		mix->Samp = sam->Samp+offset;
		mix->End = sam->End;
		mix->Low = 0;
		mix->Replen = sam->Replen;
	} else mix->Active = 0;
}

uint readWord(FILE *fp) {
	uint t;
	
	t = fgetc(fp) * 0x100;
	return t + fgetc(fp);
}

int loadMod(char *name, ModHead *mp) {

	FILE *fp;
	S3MSamp *samp;
	uchar *patp;
	uint i,j,temp,numpat,temp2;
	long patsize;
	char *samdata;
	Ident *idp;
	
	fp = fopen(name, "rb");
	if (!fp) {
		perror("josmod"); 
		exit(1);
	}
	fread(mp->Name,1,20,fp);
	mp->Name[20]=0;

#ifdef DEBUG
	printf("Mod name is %s\n",mp->Name);
#endif

	samp = &mp->Samples[0];
	for (i=0;i<31;i++) {
		fread(samp->Name,1,22,fp);
		samp->Name[22]=0;
		samp->Length = readWord(fp) << 1;
		samp->Finetune = fgetc(fp);
		if (samp->Finetune>=8)
			samp->Finetune -= 16;
		samp->Volume = fgetc(fp);
		temp = readWord(fp) << 1;
		samp->Replen = readWord(fp) << 1;
		if (samp->Replen > 2) {
			samp->Looped = 1;
			samp->End = (char *) temp + samp->Replen;
			if ((uint) samp->End > samp->Length)
				samp->End = (char *) samp->Length;
		} else {
			samp->End = (char *) samp->Length;
			samp->Looped = 0;
		}
		samp++;
	}
	mp->NumOrders = fgetc(fp); fgetc(fp);
	numpat=0;
	for (i=0;i<128;i++) {
		 temp = fgetc(fp);
		 if (temp > numpat)
		 	numpat = temp;
		 mp->Orders[i] = temp;
	}
	numpat++;
	mp->NumPatterns = numpat;
	
	
	/* Identify type of mod */
	
	fread(mp->Type,1,4,fp);
	mp->Type[4]=0;

	mp->Channels=0;
	idp = &idents[0];
	for (i=0;i<8;i++) {
		if (!strncmp(idp->String,mp->Type,4)) {
			mp->Channels = idp->channels;
			break;
		}
		idp++;
	}
	if (!mp->Channels) {
		fclose(fp);
		return 0;
	} else {
		printf("Channels %d\n",mp->Channels);
	}
	mp->LineSize = 4 * mp->Channels;
	mp->PatSize = 64 * mp->LineSize;
	patsize = mp->PatSize * numpat;
	patp = malloc(patsize);
	mp->Patterns = patp;
	
	/* Load patterns */

	for (i=0;i<numpat;i++) {
		for (j=mp->PatSize/4;j;j--) {
			* (uint *) (patp+2) = readWord(fp);
			* (uint *) patp = readWord(fp);
			conv2Note(patp);
			patp += 4;
		}
	}	

	/* Load Samples */

	samp = &mp->Samples[0];
	for (i=0;i<31;i++) {
		printf("Sample %d: %s\n",i,samp->Name);
		if (samp->Length) {
			samdata = malloc((long) samp->Length + 128);
			fread(samdata,1,samp->Length,fp);
			samp->Samp = samdata;
			samp->End = (uint32) samp->End + samdata;
			fixSamp(samp->Looped, samp->End, samp->Replen, 128);
#ifdef DEBSAMOUT
			{
			int temp2 = open("/dev/mixer",O_READ|O_WRITE);
			sendCon(temp2, IO_CONTROL, 0xc0, 8, 22000, 1, 0);
			write(temp2, samdata, temp);
			close(temp2);
			}
#endif

		} else samp->Samp = NULL; 
		samp++;
	}
   	fclose(fp);
	return  1;
}


void initMod(ModHead *mp, uint HzSpeed) {

	s3mscale = 14317056 / HzSpeed;
	s3mscale <<= 16;
	if (!mp->Init) {
		mp->Init = 1;
		mp->Hz = HzSpeed;
		mp->BufSize = SBUFSIZE;
		mp->SoundBuf[0] = buf1;
		mp->SoundBuf[1] = buf2;
	}
}
				
uint portaEffect(Channel *Chan, MixChan *MChan) {
	int yes=0;
	if (Chan->Period < Chan->Target) {
		Chan->Period += Chan->PortSpeed;
		if (Chan->Period >= Chan->Target)
			yes = 1;
	} else {
		Chan->Period -= Chan->PortSpeed;
		if (Chan->Period <= Chan->Target)
			yes = 1;
	}
	if (yes) {
		Chan->Period = Chan->Target;
		Chan->Porta=0;
	}
	return Chan->Period;
}

void playMod(ModHead *mp) {	
	int fd;
	uint note,instrument,newpos,newpat,newline;
	uint effect,effectdata;
	uint i,printit,offset,patdelay,doport,dovol,target;
	uchar *loopP,*PatP,*PatUp;
	uint loopGo,loopLeft,newperiod;
	int volchange;
	Channel *Chan;
	MixChan *MChan;
	uint TickSize,PatLeft,TickUp,TickBeat;
	uint BufUp,BufIn,OrderUp,nyb1,nyb2;

	fd = open("/dev/mixer",O_READ|O_WRITE);
	if (fd == -1) {
		fprintf(stderr,"Could not open sound device!\n");
		return;
	}
	sendCon(fd, IO_CONTROL, 0xc0, 8, mp->Hz, 1, 2);
	
	Chan = &mp->Chans[0];
	MChan = &mp->MChans[0];
	for(i=0;i < mp->Channels;i++) {
		Chan->Volume=-1;
		Chan->LPeriod=-1;
		Chan->LVolume=-1;
		Chan->VibOn=0;
		MChan->Active=0;
		++Chan;
		++MChan;
	}
	TickSize = mp->Hz / 50;
	PatP = mp->Patterns + mp->Orders[0] * mp->PatSize;
	PatLeft = 64;
	OrderUp = 0;
	TickUp = 0;
	TickBeat = 6;
	BufUp = 0;
	BufIn = 0;
	patdelay=0;
	loopLeft=0;
	loopP = NULL;
	newpos = 0;
	newpat = 1;
	newline = 0;
	startTimer();
	while(OrderUp < mp->NumOrders) {
		if (newpat) {
			PatUp = mp->Patterns + mp->Orders[OrderUp] * mp->PatSize;
			if (newline)
				PatUp += newline * mp->LineSize;
			PatLeft = 64 - newline;
			newline = 0;
			newpat = 0;
		}
		PatP = PatUp;
		Chan = &mp->Chans[0];
		MChan = &mp->MChans[0];
		for (i=0;i < mp->Channels;i++) {
			newperiod = Chan->Period;
			volchange = Chan->Volume;
			doport = 0;
			dovol = 0;
			if (!TickUp && !patdelay) {
				offset = 0;
				getPatData(PatUp, &note, &instrument, &effect, &effectdata);
				PatUp+=4;
				Chan->Effect = effect;
				nyb1 = effectdata >> 4;
				nyb2 = effectdata & 0x0f;
				if (instrument) {
					Chan->LastS3M = &mp->Samples[--instrument];
					volchange = Chan->LastS3M->Volume;
				}
				if (effect == 9)
					offset = effectdata << 8;
				if (note) {
					uint period = AmigaPeriod[note+Chan->LastS3M->Finetune];
					Chan->VibUp = 0;
					if (effect == 3 || effect == 5) {
						Chan->Target = period;
					} else {
						newperiod = Chan->Period = period;
						playS3amp(MChan, Chan->LastS3M, offset);
					}
#ifdef SHOWNOTES
					note >>= 3;
					note--;
					printf("%s%d ",namenote(note%12),note/12);
#endif
				} 
#ifdef SHOWNOTES
				else printf("--- ");
#endif						
				printit=0;
				Chan->VibOn = 0;
				Chan->SlideOn = 0;
				Chan->Porta = 0;
				switch (effect) {
				case 0x00:
				case 0x09:
					break;
				case 0x0f:
					if (effectdata < 0x20)
						TickBeat = effectdata;
					else 
						TickSize = mp->Hz / (effectdata * 2 / 5); 
					break;
				case 0x01:
					Chan->Target = HIGHPERIOD;
					if (effectdata)
						Chan->PuSpeed = effectdata*4;
					Chan->PortSpeed = Chan->PuSpeed;
					Chan->Porta = 1;
					break; 
				case 0x02:
					Chan->Target = LOWPERIOD;
					if (effectdata)
						Chan->PdSpeed = effectdata*4;
					Chan->PortSpeed = Chan->PdSpeed;
					Chan->Porta = 1;
					break;
				case 0x03:
					if (effectdata)
						Chan->TpSpeed = effectdata*4;
					Chan->PortSpeed = Chan->TpSpeed;
					Chan->Porta = 1;
					break;
				case 0x04:
					if (nyb1)
						Chan->VibSpeed = nyb1 * 4;
					if (nyb2)
						Chan->VibDepth = nyb2;
					Chan->VibOn = 1;
					break;
				case 0x06:
					Chan->VibOn = 1;
				case 0x05:
					if (effect == 5)
						Chan->Porta = 1;
				case 0x0a:
					if (nyb1)
						Chan->Slide = nyb1;
					if (nyb2)
						Chan->Slide = -nyb2;
					Chan->SlideOn = 1;
					break;
				case 0x0d:
					if (effectdata)
						newline = nyb1 * 10 + nyb2-1;
					PatLeft = 1;
					break;
				case 0x0c:
					volchange = effectdata;
					break;
				case 0x0e:
					switch(nyb1) {
					case 0x00:
						break;
					case 0x06:
						if (!nyb2 || !loopP) {
							loopP = PatP;
							loopGo = PatLeft;
						} else {
							if (!loopLeft)
								loopLeft = nyb2;
							else loopLeft--;
							if (loopLeft>=1) 
								newpos = 1;
							else loopP = NULL;
						}
						break;
					case 0x01:
						Chan->PortSpeed = nyb2*4;
						Chan->Target = LOWPERIOD;
						doport = 1;
						break;
					case 0x02:
						Chan->PortSpeed = nyb2*4;
						Chan->Target = HIGHPERIOD;
						doport = 1;
						break; 
					case 0x0a:
						volchange += nyb2;
						break;
					case 0x0b:
						volchange -= nyb2;
						break;
					case 0x0e:
						patdelay = nyb2;
						break;
					default:
						printit=1;
					}
					break;
				default:
					printit=1;
				}
			} else {
				if (patdelay)
					patdelay--;
				if (Chan->Porta)
					doport=1;
				if (Chan->SlideOn)
					dovol=1;
			}
			if (doport)
				newperiod = portaEffect(Chan, MChan);
			if (dovol)
				volchange += Chan->Slide;
			if (volchange > 64)
				volchange = 64;
			if (volchange < 0)
				volchange = 0;
			Chan->Volume = volchange;
			if (volchange != Chan->LVolume) {
				MChan->VolTab = &VolumeTab[volchange][0];
				Chan->LVolume = volchange;
			}
			Chan->Period = newperiod;
			if (Chan->VibOn) {
				Chan->VibUp += Chan->VibSpeed;
				newperiod += (vibratotable[0][Chan->VibUp] * Chan->VibDepth) >> 3;
			}
			if (newperiod != Chan->LPeriod) {
				MChan->Speed = s3mscale / newperiod;
				Chan->LPeriod = newperiod;
			}
			Chan++;
			MChan++;
		}
#ifdef SHOWNOTES
		if (!TickUp)
			putchar('\n'); 
#endif
		if (TickSize*2 > mp->BufSize - BufUp) {
			endTimer();
			write(fd, mp->SoundBuf[BufIn], BufUp);
			BufUp = 0;
			BufIn ^= 1;
			startTimer();
		}
		mix(mp->SoundBuf[BufIn]+BufUp, TickSize, &mp->MChans[0], mp->Channels);
		BufUp += TickSize;
		TickUp++;
		if (TickUp >= TickBeat) {
			TickUp=0;
			if (!patdelay) {
				if (newpos) {
					PatUp = loopP;
					PatLeft = loopGo;
					loopP = NULL;
					newpos = 0;
				} else {
					if (!--PatLeft) {
						newpat = 1;
						OrderUp++;
					}
				}
			} 
		}
	}
	write(fd, mp->SoundBuf[BufIn], BufUp);
	close(fd); 
}

void initS3M(S3MHead *s3m, uint hz) {
	s3mscale = 14317056 / hz;
	s3mscale <<= 16;
	if (!s3m->Init) {
		s3m->Init = 1;
		s3m->Hz = hz;
		s3m->BufSize = SBUFSIZE;
		s3m->SoundBuf[0] = buf1;
		s3m->SoundBuf[1] = buf2;
	}
	return;
}

void playS3M(S3MHead *s3m) {
	int fd;
	int volchange;
	uint note,instrument,newPat,dovol,doport,newline;
	uint effect,effectdata;
	uint i,printit,offset,patdelay,ticktodo,target;
	S3MNote *loopP,*PatUp,*PatP;
	uint loopGo,loopLeft,patTop,newperiod,nyb1,nyb2;
	Channel *Chan;
	MixChan *MChan;
	uint TickSize,PatLeft,TickUp,TickBeat,BufUp,BufIn,OrderUp;

	fd = open("/dev/mixer",O_READ|O_WRITE);
	if (fd == -1) {
		fprintf(stderr,"Digi driver not loaded!\n");
		return;
	}
	sendCon(fd, IO_CONTROL, 0xc0, 8, s3m->Hz, 1, 2);

	Chan = &s3m->Chans[0];
	MChan = &s3m->MChans[0];
	for(i=0;i < s3m->Channels;i++) {
		Chan->Volume=0;
		Chan->LVolume=-1;
		Chan->LPeriod=-1;
		MChan->Active=0;
		++Chan;
		++MChan;
	}	
	TickSize = s3m->Hz / (s3m->Hd.initialtempo * 2 / 5); 
	PatLeft = 64;
	OrderUp = 0;
	TickUp = 0;
	TickBeat = s3m->Hd.initialspeed;
	BufUp = 0;
	BufIn = 0;
	newline=0;
	patdelay=0;
	loopLeft=0;
	newPat=1;
	while(OrderUp < s3m->Hd.ordernum) {
		if (newPat) {
			newPat=0;
			while (s3m->Orders[OrderUp] == 254)
				if (!s3m->Orders[++OrderUp])
					return;
			if (s3m->Orders[OrderUp] == 255)
				return;
			PatUp = (S3MNote *) ((uchar *) s3m->Patterns + s3m->Orders[OrderUp] * (uint32) s3m->PatSize);
			PatLeft = 64-newline;
			if (newline)
				PatP += newline * s3m->Channels;
			newline = 0; 
		}
		Chan = &s3m->Chans[0];
		MChan = &s3m->MChans[0];
		PatP = PatUp;
		for (i=0;i < s3m->Channels;i++) {
			newperiod = Chan->Period;
			volchange = Chan->Volume;
			doport = 0;
			dovol = 0;
			if (!TickUp && !patdelay) {
				printit=0;
				offset = 0;
				note = PatUp->note;
				instrument = PatUp->instrument;
				Chan->Effect = effect = PatUp->effect;
				effectdata = PatUp->effectdata;
				volchange = Chan->Volume;
				nyb1 = effectdata >> 4;
				nyb2 = effectdata & 0x0f;
				if (instrument) {
					Chan->LastS3M = &s3m->Instruments[--instrument];
					volchange = Chan->LastS3M->Volume;
				}
				if (PatUp->voleffect != 0xff) {
					volchange = PatUp->voleffect;
				}
				if (effect == 15)
					offset = effectdata << 8;
				if (note != 0xff) {
					if (note == 0x0fe) {
						MChan->Active=0;
					} else {
						uint period = S3MPeriod[note] / (uint) Chan->LastS3M->Finetune;
						if (effect == 7 || effect == 12) {
							Chan->Target = period;
						} else {
							Chan->VibUp = 0;
							newperiod = Chan->Period = period;
							playS3amp(MChan, Chan->LastS3M, offset);
						}
					}
				} 
				target = 0;
				Chan->Porta = 0;
				Chan->SlideOn = 0;
				Chan->VibOn = 0;
				switch (effect) {
				case 0xff:
				case 15:
					break;
				case 3:
					if (effectdata) 
						newline = nyb1 * 10 + nyb2-1;
					PatLeft = 1;
					break;
				case 20:
					if (effectdata > 0x20)
						TickSize = s3m->Hz / (effectdata * 2 / 5); 
					break;
				case 1:
					if (effectdata)
						TickBeat = effectdata;
					break;
				case 7:
					if (effectdata)
						Chan->TpSpeed = effectdata*4;
					Chan->PortSpeed = Chan->TpSpeed;
					Chan->Porta = 1;
					break;
				case 8:
					if (effectdata) {
						Chan->VibSpeed = nyb1 * 4;
						Chan->VibDepth = nyb2;				
					}
					Chan->VibOn = 1;
					break;
				case 12:
					Chan->Porta = 1;
				case 11:
					if (effect == 11)
						Chan->VibOn = 1;
				case 4:
					if (effectdata) {
						Chan->Slide=0;
						Chan->SlideNow=0;
						if (nyb1 == 0x0f) {
							Chan->SlideNow = -nyb2;
						} else 
						if (nyb2 == 0x0f) {
							Chan->SlideNow = nyb1;
						} else
						if (!nyb1) {
							Chan->Slide = -nyb2;
						} else
						if (!nyb2) {
							Chan->Slide = nyb1;
						}
					}
					if (Chan->SlideNow)
						volchange += Chan->SlideNow;
					Chan->SlideOn = 1;
					break;	
				case 5:
					target = LOWPERIOD;
					if (effectdata)
						Chan->PdSpeed = effectdata;
					Chan->PortSpeed = Chan->PdSpeed;
				case 6:
					if (!target) {
						target = HIGHPERIOD;
						if (effectdata)
							Chan->PuSpeed = effectdata;
						Chan->PortSpeed = Chan->PuSpeed;
					}
					Chan->Target = target;
					if (Chan->PortSpeed < 0xe0) {
						Chan->PortSpeed *= 4;
						Chan->Porta = 1;
						break;
					} else 
					if (Chan->PortSpeed < 0xf0) {
						Chan->PortSpeed &= 15;
					} else {
						Chan->PortSpeed = (Chan->PortSpeed & 15) *4;
					}
					doport = 1;
					break;
				default:
					printit=1;
					break;
				}
				if (printit) 
					printf("%03x\n",effect); 
				PatUp++;
			} else {
				if (patdelay)
					patdelay--;
				if (Chan->Porta)
					doport=1;
				if (Chan->SlideOn)
					dovol=1;
			}
			if (doport)
				newperiod = portaEffect(Chan, MChan);
			if (dovol)
				volchange += Chan->Slide;
			if (volchange > 64)
				volchange = 64;
			if (volchange < 0)
				volchange = 0;
			Chan->Volume = volchange;
			if (volchange != Chan->LVolume) {
				MChan->VolTab = &VolumeTab[volchange][0];
				Chan->LVolume = volchange;
			}
			Chan->Period = newperiod;
			if (Chan->VibOn) {
				Chan->VibUp += Chan->VibSpeed;
				newperiod += (vibratotable[0][Chan->VibUp] * Chan->VibDepth) >> 3;
			}
			if (newperiod != Chan->LPeriod) {
				MChan->Speed = s3mscale / newperiod;
				Chan->LPeriod = newperiod;
			}
			Chan++;
			MChan++;
		}
			
		if (TickSize*2 > s3m->BufSize - BufUp) {
			write(fd, s3m->SoundBuf[BufIn], BufUp);
			BufUp = 0;
			BufIn ^= 1;
		}
		mix(s3m->SoundBuf[BufIn]+BufUp, TickSize, &s3m->MChans[0], s3m->Channels);
		BufUp += TickSize;
		TickUp++;
		if (TickUp >= TickBeat) {
			TickUp=0;
			if (!patdelay) {
				if (!--PatLeft) {
					OrderUp++;
					newPat=1;
				}
			} 
		}
	}
	return;
}
	
int loadS3M(char *name, S3MHead *s3m) {
	int fd;
	uint chans,i,j,len;
	uint16 *upto;
	S3MInst tsamp;
	S3MSamp *instup;
	S3MNote *noteup,*dst;
	
	fd = open(name, O_READ);
	if (fd == -1) {
		perror("josmod"); 
		exit(1);
	}
	read(fd, &s3m->Hd,sizeof(S3MDisk));
	if ( /*s3m->Hd.byte1a != 0x1a || s3m->Hd.filetype != 0x10 || */strncmp(s3m->Hd.id,"SCRM",4)) {
		close(fd);
		return 0;
	}
	printf("S3M name is %s\n",s3m->Hd.songname);
	chans = 0;
	for (i=0;i<32;i++) {
		convert[i] = 0xff;
		if (s3m->Hd.chsettings[i] < 16) {
			convert[i] = chans++;
		}
	}
	printf("Contains %d channels\n",chans);
	s3m->Channels = chans;
	memset(s3m->Orders,0xff,256);
	read(fd, &s3m->Orders, s3m->Hd.ordernum);
	printf("Orders %d\n",s3m->Hd.ordernum);
	read(fd, &s3m->InstPtr, s3m->Hd.instnum * 2);
	read(fd, &s3m->PatPtr, s3m->Hd.pattnum * 2);
	s3m->Instruments = malloc(s3m->Hd.instnum * sizeof(S3MSamp));
	upto = &s3m->InstPtr[0];
	instup = s3m->Instruments;
	for (i=0;i < s3m->Hd.instnum;i++) {
		uint tempst,tempend;
		
		lseek(fd, (long) *upto << 4,SEEK_SET);
		read(fd, &tsamp, sizeof(S3MInst));
		instup->Samp = NULL;
		printf("Real name %s\n",tsamp.name);
		if (tsamp.type == 1)
			instup->Length = tsamp.length;
		else instup->Length = 0;
		instup->Finetune = tsamp.c2rate;
		instup->Volume = tsamp.vol;
		tempst = tsamp.loopstart;
		tempend = tsamp.loopend;
		if (tsamp.flags & 4) {
			tempst <<= 1;
			tempend <<= 1;
			instup->Length <<= 1;
		}
		if (tsamp.flags & 1 && tempst != tempend) {
			instup->Looped = 1;
			instup->Replen = tempend - tempst;
			instup->End = (char *) tempend;
		} else {
			instup->Looped = 0;
			instup->End = (char *) instup->Length;
		}
		instup->FilePos = (tsamp.paraptr | ((uint32) tsamp.paraptrhigh << 16)) << 4;
		upto++;
		instup++;
	}
	s3m->LineSize = sizeof(S3MNote) * chans;
	s3m->PatSize = 64 * s3m->LineSize;
	s3m->Patterns = malloc((long) s3m->PatSize * s3m->Hd.pattnum);
	noteup = s3m->Patterns;
	for (i = s3m->Hd.pattnum * 64 * chans;i;i--) {
		noteup->note = 0xff;
		noteup->voleffect = 0xff;
		noteup->effect = 0xff;
		noteup->instrument = 0;
		noteup++;
	}
	upto = &s3m->PatPtr[0];
	noteup = s3m->Patterns;
	for (i=0;i<s3m->Hd.pattnum;i++) {
		long pos = (long) *upto << 4;
		if (pos) {
			lseek(fd, pos, SEEK_SET);
			read(fd, &len, 2);
			if (len > 2) {
				uchar *buf,*src;
				len -= 2;
				src = buf = malloc(len);
				read(fd, buf, len);
				for (j=0;j<64;j++) {
					uchar ctrl;
					while (ctrl = *src++) {
						int yes = convert[ctrl & 31];
						dst = &noteup[yes];
						if (yes >= chans)
							yes = 0;
						else yes=1;
						if (ctrl & 32) {
							if (yes) {
								if (*src < 254) {
									dst->note = (*src & 15) + (*src >> 4) *12;
								} else dst->note = 0xff;
								src++;
								dst->instrument = *src++;
							} else src+=2;
						}
						if (ctrl & 64) {
							if (yes) 
								dst->voleffect = *src;
							src++;
						}
						if (ctrl & 128) {
							if (yes) {
								dst->effect = *src++;
								dst->effectdata = *src++;
							} else src+=2;
						}
					};
					noteup += chans;
				}
				free(buf);
			}
		} else {
			noteup += chans * 64;
		}
		upto++;
	}
	instup = s3m->Instruments;
	for (i=s3m->Hd.instnum;i;i--) {
		if (instup->Length) {
			instup->Samp = malloc(instup->Length+128);
			instup->End = (uint32) instup->End + instup->Samp;
			lseek(fd, instup->FilePos, SEEK_SET);
			read(fd, instup->Samp, instup->Length);
			if (s3m->Hd.fileversion == 2) {
				uchar *upto = (uchar *)instup->Samp;
				uint32 left = instup->Length;
				while (left--)
					*upto++ ^= 0x80;
			}
			fixSamp(instup->Looped, instup->End, instup->Replen, 128);
		}
		instup++;
	}
	close(fd);
	return  1;
} 

void initXM(XMHead *xm, uint hz) {
	s3mscale = 14317056 / hz;
	s3mscale <<= 16;
	if (!xm->Init) {
		xm->Init = 1;
		xm->Hz = hz;
		xm->BufSize = SBUFSIZE;
		xm->SoundBuf[0] = buf1;
		xm->SoundBuf[1] = buf2;
		xm->linTable = malloc(sizeof(uint32) * 7680);
	}
	makeLin(xm->linTable, linearfreqs, hz);
	return;
}

void freeXM(XMHead *xm) {
	uint i,j;
	XMPatPtr *pup = xm->Patterns;
	XMInst *iup = xm->Instruments;
	XMSamp *sup;
	
	for (i=0;i<xm->Hd.patterns;i++) {
		free(pup->notes);
		pup++;
	}
	free(xm->Patterns);
	for (i=0;i<xm->Hd.instruments;i++) {
		sup = iup->Samples;
		for (j=0;j<iup->Hd.numsamples;j++) {
			if (sup->Samp)
				free(sup->Samp);
			sup++;
		}
		if (iup->Hd.numsamples)
			free(iup->Samples);
		iup++;
	}
	free(xm->Instruments);
}

uint startXMnote(Channel *Chan, MixChan *mix, int note, uint offset, int linear, int reset) {
	uint newperiod=0,num;
	XMInst *this = Chan->LastXM;
	XMSamp *sam;
	
	num = this->Hd2.sampnum[note];
	sam = &this->Samples[num];
	if (sam->Samp) {
		note += sam->Hd.relative;
		if (note < 0) 
			note = 0;
		else if (note > 118) 
			note = 118;
		if (linear)
			newperiod = getlinperiod(note, sam->Hd.finetune);
		else newperiod = S3MPeriod[note] / fineTab[(sam->Hd.finetune+128)/16];
		Chan->Volume = sam->Hd.vol;
		if (reset) {
			mix->Active = 1;
			mix->Repeats = sam->Looped;
			mix->Samp = sam->Samp+offset;
			mix->End = sam->End;
			mix->Low = 0;
			mix->Replen = sam->Replen;
		}
	} else {
		printf("No sample!\n");
		mix->Active = 0;
	}
	return newperiod;
}

void playXM(XMHead *xm) {
	int fd;
	Channel *Chan;
	MixChan *MChan;
	uint patdelay,i,rows,newpat,printit,newperiod,newinst;
	uint TickSize,PatLeft,TickUp,TickBeat,BufUp,BufIn,OrderUp;
	uint note,instrument,effect,effectdata,voleffect,offset;
	int volchange,dovol,doport;
	uint lowperiod,highperiod,newline,nyb1,nyb2;
	XMNote *PatP;
	XMPatPtr *thispat;

	fd = open("/dev/mixer",O_READ|O_WRITE);
	if (fd == -1) {
		fprintf(stderr,"Couldn't open sound device!\n");
		return;
	}
	sendCon(fd, IO_CONTROL, 0xc0, 8, xm->Hz, 1, 2);

	Chan = &xm->Chans[0];
	MChan = &xm->MChans[0];
	for(i=0;i < xm->Hd.channels;i++) {
		Chan->Volume=-1;
		Chan->VibOn = 0;
		MChan->Active=0;
		Chan->LastXM = NULL;
		++Chan;
		++MChan;
	}
        if (xm->Hd.uselinear)
        {
                lowperiod = 7743;
                highperiod = 64;
        }
        else
        {
                lowperiod = 29024;
                highperiod = 28;
        }
	globvol = 64;
	OrderUp = 0;
	BufIn = 0;
	BufUp = 0;
	TickUp = 0;
	TickBeat = xm->Hd.defaulttempo;
	TickSize = xm->Hz / (xm->Hd.defaultbpmtempo * 2 / 5); 
	newpat = 1;
	newline = 0;
	startTimer();
	while (OrderUp < xm->Hd.songlength) {
		if (newpat) {
			thispat = &xm->Patterns[xm->Hd.order[OrderUp]];
			PatP = thispat->notes;
			if (newline) 
				PatP += newline*xm->Hd.channels;
			rows = thispat->rows-newline;
			newpat=0;
			newline=0;
		}
		Chan = &xm->Chans[0];
		MChan = &xm->MChans[0];
		for (i=0;i<xm->Hd.channels;i++) {
			newperiod = Chan->Period;
			volchange = Chan->Volume;
			doport = 0;
			dovol = 0;
			if (!TickUp) {
				printit=0;
				newinst = 0;
				offset = 0;
				note = PatP->note;
				instrument = PatP->instrument;
				voleffect = PatP->voleffect;
				effect = PatP->effect;
				effectdata = PatP->effectdata;
				nyb1 = effectdata >> 4;
				nyb2 = effectdata & 0x0f;
				Chan->Effect = effect;
				PatP++;
				if (effect == 9)
					offset = effectdata << 8;
				if (instrument) {
					Chan->LastXM = &xm->Instruments[--instrument];
					Chan->VibUp = 0;
					Chan->VEnvUp = 0;
					newinst = 1;
				}
				Chan->Porta = 0;
				if (note) {
					int reset=1,temp;
					if (note != XMKEYOFF) {
						--note;
					   	if (effect == 0x03 || effect == 0x05)
							reset = 0;
						temp = startXMnote(Chan, MChan, note, offset, xm->Hd.uselinear, reset);
						if (!reset) {
							Chan->Target = temp;
							Chan->Porta = 1;
						} else newperiod = temp;
						if (newinst) {
							volchange = Chan->Volume;
						}
#ifdef SHOWNOTES
						printf("%s%d ",namenote(note%12),note/12);
#endif
					} else {
#ifdef SHOWNOTES
						printf("--- ");
#endif
						volchange = 0;
						MChan->Active = 0;
					}
				} 
#ifdef SHOWNOTES
				else printf("--- ");
#endif						

				Chan->VibOn = 0;
				Chan->SlideOn = 0;
				switch (voleffect >> 4) {
					case 0x00:
						break;
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x05:
						volchange = voleffect - 0x10;
						break;
					case 0x06:
						Chan->Slide = -(voleffect & 0x0f);
						Chan->SlideOn = 1;
						break;
					case 0x07:
						Chan->Slide = voleffect & 0x0f;
						Chan->SlideOn = 1;
						break;
					case 0x08:
						volchange -= voleffect & 0x0f;
						break;
					case 0x09:
						volchange += voleffect & 0x0f;
						break; 
					case 0x0a:
						Chan->VibSpeed = voleffect & 0x0f;
						break;
					case 0x0b:
						Chan->VibDepth = voleffect & 0x0f;
						Chan->VibOn = 1;
						break;
					case 0x0c:
						break;
					default: 
						printf("Voleffect %2x\n",voleffect);
						break;
				}
				switch (effect) {
					case 0x00:
					case 0x08:
					case 0x09:
						break;
					case 0x0c:
						volchange = effectdata;
						break;
					case 0x04:
						if (nyb1)
							Chan->VibSpeed = nyb1 * 4;
						if (nyb2)
							Chan->VibDepth = nyb2;
						Chan->VibOn = 1;
						break;
					case 0x06:
						Chan->VibOn = 1;
					case 0x05:
					case 0x0a:
						if (nyb1)
							Chan->Slide = nyb1;
						if (nyb2)
							Chan->Slide = -nyb2;
						Chan->SlideOn = 1;
						break;
					case 0x0d:
						rows=1;
						if (effectdata) 
							newline = nyb1 * 10 + nyb2 - 1;
						break;
					case 0x0f:
						if (effectdata < 0x20)
							TickBeat = effectdata;
						else 
							TickSize = xm->Hz / (effectdata * 2 / 5); 
						break;
					case 0x03:
						if (effectdata)
							Chan->TpSpeed = effectdata * 4;
						Chan->PortSpeed = Chan->TpSpeed;
						Chan->Porta = 1;
						break;
					case 0x02:
						Chan->Target = lowperiod;
						if (effectdata)
							Chan->PdSpeed = effectdata;
						Chan->PortSpeed = Chan->PdSpeed;
						Chan->Porta = 1;
						break;
					case 0x01:
						Chan->Target = highperiod;
						if (effectdata)
							Chan->PuSpeed = effectdata;
						Chan->PortSpeed = Chan->PuSpeed;
						Chan->Porta = 1;
						break;
					case 0x10:
						if (effectdata)
							globvol = effectdata;
						if (globvol > 64)
							globvol = 64;
						if (globvol < 0)
							globvol = 0;
						break;
					case 0x14:
				   		break;
				 	case 0x0e:
						switch(nyb1) {
						case 0x0c:
							MChan->Active = 0;
							break;
						default:
							printit=1;
						}
						break;
					default:
						printit=1;
						break;
						
				}
				if (printit)
					printf("%03x%03x\n",effect,effectdata);
			} else {
				if (Chan->Porta)
					doport = 1;
				if (Chan->SlideOn)
					dovol = 1;
			}
			if (doport)
				newperiod = portaEffect(Chan, MChan);
			if (dovol)
				volchange += Chan->Slide;
			if (volchange < 0)
				volchange = 0;
			else if (volchange > 64)
				volchange = 64;
			Chan->Volume = volchange;
			Chan->Period = newperiod;
			Chan++;
			MChan++;
		}
#ifdef SHOWNOTES
		if (!TickUp)
			putchar('\n'); 
#endif
		Chan = &xm->Chans[0];
		MChan = &xm->MChans[0];
		for (i=0;i<xm->Hd.channels;i++) {
			uint val;
			XMInst *inst = Chan->LastXM;
			if (inst && MChan->Active) {
				if (inst->Hd2.voltype & ENV_ON) {
					val = (Chan->Volume * globvol) >> 6;
					val = (val * inst->VEnv[Chan->VEnvUp]) >> 6;
					Chan->VEnvUp++;
					if (Chan->VEnvUp > 324) {
						Chan->VEnvUp = 324;
						if (!inst->VEnv[324])
							MChan->Active = 0;
					}
					if (inst->Hd2.voltype & ENV_LOOP) {
						if (Chan->VEnvUp >= inst->VEnd) 
							Chan->VEnvUp = inst->VStart;
					}
				} else val = (globvol * Chan->Volume) >> 6;
				MChan->VolTab = &VolumeTab[val][0];
				newperiod = Chan->Period;
				if (Chan->VibOn) {
					Chan->VibUp += Chan->VibSpeed;
					newperiod += (vibratotable[0][Chan->VibUp] * Chan->VibDepth) >> 3;
				}
				if (xm->Hd.uselinear) 
					MChan->Speed = xm->linTable[newperiod]; 
				else MChan->Speed = s3mscale / newperiod;
			} 
			Chan++;
			MChan++;
		}
		if (TickSize*2 > xm->BufSize - BufUp) {
			endTimer();
			write(fd, xm->SoundBuf[BufIn], BufUp);
			BufUp = 0;
			BufIn ^= 1;
			startTimer();
		}
		mix(xm->SoundBuf[BufIn]+BufUp, TickSize, &xm->MChans[0], xm->Hd.channels);
		BufUp += TickSize;
		TickUp++;
		
		if (TickUp >= TickBeat) {
			TickUp=0;
			if (!--rows) {
				newpat=1;
				OrderUp++;
				while (xm->Hd.order[OrderUp] >= xm->Hd.patterns) 
					OrderUp++;
			}
		}
	}
	
}

void prepEnv(XMInst *inst) {
	uint i,j;
	uint x, y, x2, y2,dx;
	int dy;
	
	for (i = 0; i < inst->Hd2.numvol; ) {
		x = inst->Hd2.VolEnv[i].x;
		y = inst->Hd2.VolEnv[i].y;
		if (++i == inst->Hd2.numvol) {
			x2 = 1024;
			y2 = y;
		} else {
			x2 = inst->Hd2.VolEnv[i].x;
			y2 = inst->Hd2.VolEnv[i].y;
		}
		dx = x2 - x;
		dy = y2 - y;
		if (dx) 
			for (j = 0; j < dx; j++) {
				if (j+x < 1024) 
					inst->VEnv[j + x] = y + (int)(dy * j) / (int)dx;

			}
	}
	inst->VStart = inst->Hd2.VolEnv[inst->Hd2.volstart].x;
	inst->VEnd = inst->Hd2.VolEnv[inst->Hd2.volend].x;
	if (inst->VStart == inst->VEnd)
		inst->Hd2.voltype &= ~ENV_LOOP;
}

int loadXM(char *name, XMHead *xm) {
	int fd;
	uint i,j;
	XMPatPtr *ptrup;
	XMNote *noteup;
	XMInst *instup;
	XMSamp *sampup;
	
	fd = open(name, O_READ);
	if (fd == -1) {
		perror("josmod"); 
		exit(1);
	}
	read(fd, &xm->ID,sizeof(XMId));
	if (strncmp("Extended Module:", xm->ID.id, 16) || xm->ID.idbyte != 0x1a || xm->ID.version < 0x0103) {
		close(fd);
		return 0;
	}
	xm->ID.idbyte = 0;
	xm->ID.version = 0;
	printf("Song name %s\nTracker name %s\n",xm->ID.modname, xm->ID.trackername);
	read(fd, &xm->Hd, sizeof(XMDisk));
	lseek(fd, xm->Hd.headersize - sizeof(XMDisk), SEEK_CUR);
	printf("Channels %d\n", xm->Hd.channels);
	ptrup = xm->Patterns = malloc(xm->Hd.patterns * sizeof(XMPatPtr));
	
	for (i=0;i<xm->Hd.patterns;i++) {
		XMPatHead pathead;
		int patsize;
		read(fd, &pathead, sizeof(XMPatHead));
		lseek(fd, pathead.headersize - sizeof(XMPatHead), SEEK_CUR);
		ptrup->rows = pathead.rows;
		patsize = 5 * pathead.rows * xm->Hd.channels;
		noteup = ptrup->notes = malloc(patsize);
		memset(noteup, 0 , patsize);
		if (pathead.packsize) {
			uchar *src,*buf;
			uint ctrl;
			buf = src = malloc(pathead.packsize);
			read(fd, src, pathead.packsize);
			while (pathead.packsize) {
				ctrl = *src++;
				pathead.packsize--;
				if (!(ctrl & 0x80)) {
					noteup->note = ctrl;
					ctrl = 0x1e;
				}
				if (ctrl & 0x01) {
					noteup->note = *src++;
					pathead.packsize--;
				}
				if (ctrl & 0x02) {
					noteup->instrument = *src++;
					pathead.packsize--;
				}
				if (ctrl & 0x04) {
					noteup->voleffect = *src++;
					pathead.packsize--;
				}
				if (ctrl & 0x08) {
					noteup->effect = *src++;
					pathead.packsize--;
				}
				if (ctrl & 0x10) {
					noteup->effectdata = *src++;
					pathead.packsize--;
				}
				noteup++;
			}
			free(buf);
		}
		ptrup++;
	}
	printf("Instruments %d\n",xm->Hd.instruments);
	instup = xm->Instruments = malloc(sizeof(XMInst) * xm->Hd.instruments);
	for (i=0;i<xm->Hd.instruments;i++) {
		read(fd, &instup->Hd, sizeof(XMInstHead));
		instup->type = instup->Hd.type;
		instup->Hd.type = 0;
		printf("Inst %d: %s\n",i,instup->Hd.name);
		if (instup->Hd.numsamples) {
			read(fd, &instup->Hd2, sizeof(XMInstHead2));
			lseek(fd, instup->Hd.headersize - (sizeof(XMInstHead2) + sizeof(XMInstHead)), SEEK_CUR);
			sampup = instup->Samples = malloc(sizeof(XMSamp) * instup->Hd.numsamples);
			if (!instup->Hd2.numvol)
				instup->Hd2.voltype = 0;
			if (instup->Hd2.voltype & ENV_ON) 
				prepEnv(instup);
			for (j=0;j<instup->Hd.numsamples;j++) {
				read(fd, &sampup->Hd, sizeof(XMSampHead));
				lseek(fd, instup->Hd2.headersize - sizeof(XMSampHead), SEEK_CUR);
				sampup->Hd.name[21] = 0;
				if (sampup->Hd.length)
					sampup->Samp = malloc(sampup->Hd.length+1024);
				else sampup->Samp = NULL;
				sampup++;
			}
			sampup = instup->Samples;
			for (j=0;j<instup->Hd.numsamples;j++) {
				if (sampup->Samp) {
					read32(fd, sampup->Samp, sampup->Hd.length);
					if (sampup->Hd.type & XMSF_16BIT) {
						uint32 left = sampup->Hd.length >> 1;
						uint16 old = 0;
						uint16 *src = (uint16 *) sampup->Samp;
						uchar *dst = (uchar *) sampup->Samp;
						sampup->Hd.length = left;
						sampup->Hd.looplength >>= 1;
						sampup->Hd.loopstart >>= 1;
						while (left--) {
							old = *src += old;
							*dst = * ((uchar *)src + 1);
							dst++;
							src++;
						}
					} else {
						uint32 left = sampup->Hd.length;
						uchar old=0;
						uchar *upto = (uchar *) sampup->Samp;
						while (left--) {
							old = *upto += old;
							upto++;
						}
					}
					if ((sampup->Hd.type & XMSF_PING || sampup->Hd.type & XMSF_LOOPED) && sampup->Hd.looplength) {
						sampup->End = sampup->Samp + sampup->Hd.looplength + sampup->Hd.loopstart;
						sampup->Replen = sampup->Hd.looplength;
						sampup->Looped = 1;
					} else {
						sampup->End = sampup->Samp + sampup->Hd.length;
						sampup->Looped = 0;
					}
					fixSamp(sampup->Looped, sampup->End, sampup->Replen, 1024);
				}
				sampup++;
			}
		} else {
			lseek(fd, instup->Hd.headersize - sizeof(XMInstHead), SEEK_CUR);
		}
		instup++;
	}
	close(fd);
	return 1;
} 

void main(int argc, char *argv[]) {
	
	ModHead *mp=&globmod;
	S3MHead *s3m=&globs3m;
	XMHead *xm=&globxm;
	
	uint hz=22000;
	uint s3mhz = 13500;
	int opt;
	
	while ((opt = getopt(argc, argv, "h:")) != EOF) {
		switch(opt) {
		case 'h': 
			hz = atoi(optarg);
			s3mhz = hz;
			break;
		}
	}
	if (optind >= argc) {
		fprintf(stderr,"Usage: josmod [-h hz] mod1 mod2..\n");
		exit(-1);
	}
	getMixer();
	buf1 = malloc(SBUFSIZE);
	buf2 = malloc(SBUFSIZE);
	prepTables(&VolumeTab[0][0]);
	while (optind < argc) {
		if (loadMod(argv[optind],mp)) {
			initMod(mp,hz);
			playMod(mp);
			freeMod(mp);
		} else  
		if (loadS3M(argv[optind],s3m)) {
			initS3M(s3m,s3mhz);
			playS3M(s3m);
			freeS3M(s3m);
		} else  
		if (loadXM(argv[optind],xm)) {
			initXM(xm,s3mhz);
			playXM(xm);
			freeXM(xm);
		} else printf("Unrecognised mod:%s\n", argv[optind]);
		optind++;
	}
		
}
