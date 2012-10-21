#include "tiles_generic.h"

#include "driver.h"
extern "C" {
#include "ay8910.h"
}

unsigned char DrvJoy1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char DrvJoy2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char BjDip[2]   = {0, 0};
static unsigned char DrvReset = 0;
static int bombjackIRQ = 0;
static int latch;

static int nCyclesDone[2], nCyclesTotal[2];
static int nCyclesSegment;

static unsigned char *Mem    = NULL;
static unsigned char *MemEnd = NULL;
static unsigned char *RamStart= NULL;
static unsigned char *RamEnd = NULL;
static unsigned char *BjGfx =NULL;
static unsigned char *BjMap =NULL;
static unsigned char *BjRom =NULL;
static unsigned char *BjRam =NULL;
static unsigned char *BjColRam =NULL;
static unsigned char *BjVidRam =NULL;
static unsigned char *BjSprRam =NULL;
// sound cpu
static unsigned char *SndRom =NULL;
static unsigned char *SndRam =NULL;

//graphics tiles
static unsigned char *text=NULL;
static unsigned char *sprites=NULL;
static unsigned char *tiles=NULL;
//pallete
static unsigned char *BjPalSrc=NULL;
static unsigned int *BjPalReal=NULL;

//static unsigned char* pTileData;


static short* pFMBuffer;
static short* pAY8910Buffer[9];
// Dip Switch and Input Definitions
static struct BurnInputInfo DrvInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 0,	  "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 1,	  "p1 start" },

	{"P1 Up"        , BIT_DIGITAL  , DrvJoy1 + 2, 	"p1 up"    },
	{"P1 Down"      , BIT_DIGITAL  , DrvJoy1 + 3, 	"p1 down"  },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 4, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 5, 	"p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 6,		"p1 fire 1"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy2 + 0,	  "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 1,	  "p2 start" },

	{"P2 Up"        , BIT_DIGITAL  , DrvJoy2 + 2, 	"p2 up"    },
	{"P2 Down"      , BIT_DIGITAL  , DrvJoy2 + 3, 	"p2 down"  },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 4, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 5, 	"p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 6,		"p2 fire 1"},

	{"Reset"        , BIT_DIGITAL  , &DrvReset  ,		"reset"    },
	{"Dip Sw(1)"    , BIT_DIPSWITCH, BjDip + 0  ,	  "dip"      },
	{"Dip Sw(2)"    , BIT_DIPSWITCH, BjDip + 1  ,	  "dip"      },
};

STDINPUTINFO(Drv);

static struct BurnDIPInfo BjDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0x00, NULL},
	{0x10, 0xff, 0xff, 0x00, NULL},

	// Dip Sw(1)
	{0   , 0xfe, 0   , 4   , "Coin A"},
	{0x0f, 0x01, 0x03, 0x00, "1 coin 1 credit"},
	{0x0f, 0x01, 0x03, 0x01, "1 coin 2 credits"},
	{0x0f, 0x01, 0x03, 0x02, "1 coin 3 credits"},
	{0x0f, 0x01, 0x03, 0x03, "1 coin 6 credits"},

	{0   , 0xfe, 0   , 4   , "Coin B"},
	{0x0f, 0x01, 0x0c, 0x00, "1 coin 1 credit"},
	{0x0f, 0x01, 0x0c, 0x04, "2 coins 1 credit"},
	{0x0f, 0x01, 0x0c, 0x08, "1 coin 2 credits"},
	{0x0f, 0x01, 0x0c, 0x0c, "1 coin 3 credits"},

	{0   , 0xfe, 0   , 4   , "Lives"},
	{0x0f, 0x01, 0x30, 0x00, "3"},
	{0x0f, 0x01, 0x30, 0x10, "4"},
	{0x0f, 0x01, 0x30, 0x20, "5"},
	{0x0f, 0x01, 0x30, 0x30, "2"},	

	{0   , 0xfe, 0   , 2   , "Cabinet"},
	{0x0f, 0x01, 0x40, 0x00, "Cocktail"},
	{0x0f, 0x01, 0x40, 0x40, "Upright"},

	{0   , 0xfe, 0   , 2   , "Demo sounds"},
	{0x0f, 0x01, 0x80, 0x00, "Off"},
	{0x0f, 0x01, 0x80, 0x80, "On"},

	// Dip Sw(2)
	{0   , 0xfe, 0   , 4   , "Initial high score"},
	{0x10, 0x01, 0x07, 0x00, "10000"},
	{0x10, 0x01, 0x07, 0x01, "100000"},
	{0x10, 0x01, 0x07, 0x02, "30000"},
	{0x10, 0x01, 0x07, 0x03, "50000"},
	{0x10, 0x01, 0x07, 0x04, "100000"},
	{0x10, 0x01, 0x07, 0x05, "50000"},
	{0x10, 0x01, 0x07, 0x06, "100000"},
	{0x10, 0x01, 0x07, 0x07, "50000"},

	{0   , 0xfe, 0   , 4   , "Bird speed"},
	{0x10, 0x01, 0x18, 0x00, "Easy"},
	{0x10, 0x01, 0x18, 0x08, "Medium"},
	{0x10, 0x01, 0x18, 0x10, "Hard"},
	{0x10, 0x01, 0x18, 0x18, "Hardest"},

	{0   , 0xfe, 0   , 4   , "Enemies number & speed"},
	{0x10, 0x01, 0x60, 0x00, "Medium"},
	{0x10, 0x01, 0x60, 0x20, "Easy"},
	{0x10, 0x01, 0x60, 0x40, "Hard"},
	{0x10, 0x01, 0x60, 0x60, "Hardest"},

	{0   , 0xfe, 0   , 2   , "Special coin"},
	{0x10, 0x01, 0x80, 0x00, "Easy"},
	{0x10, 0x01, 0x80, 0x80, "Hard"},
};

STDDIPINFO(Bj);

// Bomb Jack
static struct BurnRomInfo BombjackRomDesc[] = {
	{ "09_j01b.bin",    0x2000, 0xc668dc30, BRF_ESS | BRF_PRG },			 //  0 Z80 code
	{ "10_l01b.bin",    0x2000, 0x52a1e5fb, BRF_ESS | BRF_PRG },			 //  1
	{ "11_m01b.bin",    0x2000, 0xb68a062a, BRF_ESS | BRF_PRG },			 //  2
	{ "12_n01b.bin",    0x2000, 0x1d3ecee5, BRF_ESS | BRF_PRG },			 //  3
	{ "13.1r",          0x2000, 0x70e0244d, BRF_ESS | BRF_PRG },			 //  4

	// graphics 3 bit planes:
	{ "03_e08t.bin",    0x1000, 0x9f0470d5, BRF_GRA },			 // chars
	{ "04_h08t.bin",    0x1000, 0x81ec12e6, BRF_GRA },
	{ "05_k08t.bin",    0x1000, 0xe87ec8b1, BRF_GRA },

	{ "14_j07b.bin",    0x2000, 0x101c858d, BRF_GRA },			 // sprites
	{ "15_l07b.bin",    0x2000, 0x013f58f2, BRF_GRA },
	{ "16_m07b.bin",    0x2000, 0x94694097, BRF_GRA },

	{ "06_l08t.bin",    0x2000, 0x51eebd89, BRF_GRA },			 // background tiles
	{ "07_n08t.bin",    0x2000, 0x9dd98e9d, BRF_GRA },
	{ "08_r08t.bin",    0x2000, 0x3155ee7d, BRF_GRA },

	{ "02_p04t.bin",    0x1000, 0x398d4a02, BRF_GRA },			 // background tilemaps

	{ "01_h03t.bin",    0x2000, 0x8407917d, BRF_ESS | BRF_SND },			 //Sound CPU
};

STD_ROM_PICK(Bombjack);
STD_ROM_FN(Bombjack);


// Bomb Jack (set 2)
static struct BurnRomInfo Bombjac2RomDesc[] = {
	{ "09_j01b.bin",    0x2000, 0xc668dc30, BRF_ESS | BRF_PRG },			 //  0 Z80 code
	{ "10_l01b.bin",    0x2000, 0x52a1e5fb, BRF_ESS | BRF_PRG },			 //  1
	{ "11_m01b.bin",    0x2000, 0xb68a062a, BRF_ESS | BRF_PRG },			 //  2
	{ "12_n01b.bin",    0x2000, 0x1d3ecee5, BRF_ESS | BRF_PRG },			 //  3
	{ "13_r01b.bin",    0x2000, 0xbcafdd29, BRF_ESS | BRF_PRG },			 //  4

	// graphics 3 bit planes:
	{ "03_e08t.bin",    0x1000, 0x9f0470d5, BRF_GRA },			 // chars
	{ "04_h08t.bin",    0x1000, 0x81ec12e6, BRF_GRA },
	{ "05_k08t.bin",    0x1000, 0xe87ec8b1, BRF_GRA },

	{ "14_j07b.bin",    0x2000, 0x101c858d, BRF_GRA },			 // sprites
	{ "15_l07b.bin",    0x2000, 0x013f58f2, BRF_GRA },
	{ "16_m07b.bin",    0x2000, 0x94694097, BRF_GRA },

	{ "06_l08t.bin",    0x2000, 0x51eebd89, BRF_GRA },			 // background tiles
	{ "07_n08t.bin",    0x2000, 0x9dd98e9d, BRF_GRA },
	{ "08_r08t.bin",    0x2000, 0x3155ee7d, BRF_GRA },

	{ "02_p04t.bin",    0x1000, 0x398d4a02, BRF_GRA },			 // background tilemaps

	{ "01_h03t.bin",    0x2000, 0x8407917d, BRF_ESS | BRF_SND },			 //Sound CPU
};

STD_ROM_PICK(Bombjac2);
STD_ROM_FN(Bombjac2);



static int DrvDoReset()
{
	bombjackIRQ = 0;
	latch = 0;
	for (int i = 0; i < 2; i++) {
		ZetOpen(i);
		ZetReset();
		ZetClose();
	}

	for (int i = 0; i < 3; i++) {
		AY8910Reset(i);
	}
	return 0;
}


unsigned char __fastcall BjMemRead(unsigned short addr)
{
	unsigned char inputs=0;

	if (addr==0xb000) {
		if (DrvJoy1[5])
			inputs|=0x01;
		if (DrvJoy1[4])
			inputs|=0x02;
		if (DrvJoy1[2])
			inputs|=0x04;
		if (DrvJoy1[3])
			inputs|=0x08;
		if (DrvJoy1[6])
			inputs|=0x10;
		return inputs;
	}
	if (addr==0xb001) {
		if (DrvJoy2[5])
			inputs|=0x01;
		if (DrvJoy2[4])
			inputs|=0x02;
		if (DrvJoy2[2])
			inputs|=0x04;
		if (DrvJoy2[3])
			inputs|=0x08;
		if (DrvJoy2[6])
			inputs|=0x10;
		return inputs;
	}
	if (addr==0xb002) {
		if (DrvJoy1[0])
			inputs|=0x01;
		if (DrvJoy1[1])
			inputs|=0x04;
		if (DrvJoy2[0])
			inputs|=0x02;
		if (DrvJoy2[1])
			inputs|=0x08;
		return inputs;
	}
	if (addr==0xb004) {
		return BjDip[0]; //Dip Sw(1)
	}
	if (addr==0xb005) {
		return BjDip[1]; //Dip Sw(2)
	}
	return 0;
}

void __fastcall BjMemWrite(unsigned short addr,unsigned char val)
{
	if (addr== 0xb000)
	{
		bombjackIRQ = val;
	}
	if(addr==0xb800)
	{
		latch=val;
		return;
	}
	BjRam[addr]=val;
}

unsigned char __fastcall SndMemRead(unsigned short a)
{
	if (a==0xFF00)
	{
		return 0x7f;
	}
	if(a==0x6000)
	{
		int res;
		res = latch;
		latch = 0;
		return res;
	}
	return 0;
}



void __fastcall SndPortWrite(unsigned short a, unsigned char d)
{
	a &= 0xff;
	switch (a) {
		case 0x00: {
			AY8910Write(0, 0, d);
			return;
				   }
		case 0x01: {
			AY8910Write(0, 1, d);
			return;
				   }
		case 0x10: {
			AY8910Write(1, 0, d);
			return;
				   }
		case 0x11: {
			AY8910Write(1, 1, d);
			return;
				   }
		case 0x80: {
			AY8910Write(2, 0, d);
			return;
				   }
		case 0x81: {
			AY8910Write(2, 1, d);
			return;
				   }
	}
}

int BjZInit()
{
	// Init the z80
	ZetInit(2);
	// Main CPU setup
	ZetOpen(0);

	ZetMapArea    (0x0000,0x7fff,0,BjRom+0x0000); // Direct Read from ROM
	ZetMapArea    (0x0000,0x7fff,2,BjRom+0x0000); // Direct Fetch from ROM
	ZetMapArea    (0xc000,0xdfff,0,BjRom+0x8000); // Direct Read from ROM
	ZetMapArea    (0xc000,0xdfff,2,BjRom+0x8000); // Direct Fetch from ROM

	ZetMapArea    (0x8000,0x8fff,0,BjRam+0x8000);
	ZetMapArea    (0x8000,0x8fff,1,BjRam+0x8000);

	ZetMapArea    (0x9000,0x93ff,0,BjVidRam);
	ZetMapArea    (0x9000,0x93ff,1,BjVidRam);

	ZetMapArea    (0x9400,0x97ff,0,BjColRam);
	ZetMapArea    (0x9400,0x97ff,1,BjColRam);

	ZetMapArea    (0x9820,0x987f,0,BjSprRam);
	ZetMapArea    (0x9820,0x987f,1,BjSprRam);

	ZetMapArea    (0x9c00,0x9cff,0,BjPalSrc);
	ZetMapArea    (0x9c00,0x9cff,1,BjPalSrc);

	ZetMapArea    (0x9e00,0x9e00,0,BjRam+0x9e00);
	ZetMapArea    (0x9e00,0x9e00,1,BjRam+0x9e00);

//	ZetMapArea    (0xb000,0xb000,0,BjRam+0xb000);
	//ZetMapArea    (0xb000,0xb000,1,BjRam+0xb000);

	//	ZetMapArea    (0xb800,0xb800,0,BjRam+0xb800);
	//	ZetMapArea    (0xb800,0xb800,1,BjRam+0xb800);

	ZetSetReadHandler(BjMemRead);
	ZetSetWriteHandler(BjMemWrite);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea    (0x0000,0x1fff,0,SndRom); // Direct Read from ROM
	ZetMapArea    (0x0000,0x1fff,2,SndRom); // Direct Fetch from ROM
	ZetMapArea    (0x4000,0x43ff,0,SndRam);
	ZetMapArea    (0x4000,0x43ff,1,SndRam);
	ZetMapArea    (0x4000,0x43ff,2,SndRam); // fetch from ram?
	ZetMapArea    (0xFF00,0xFFFF,0,SndRam);
	ZetMapArea    (0xFF00,0xFFFF,1,SndRam);
	ZetMapArea    (0xFF00,0xFFFF,2,SndRam); // more fetch from ram? What the hell . .

	//	ZetMapArea    (0x6000,0x6000,0,BjRam+0xb800);
	//	ZetMapArea    (0x6000,0x6000,1,BjRam+0xb800);
	ZetSetReadHandler(SndMemRead);
	ZetSetOutHandler(SndPortWrite);
	ZetMemEnd();
	ZetClose();

	pAY8910Buffer[0] = pFMBuffer + nBurnSoundLen * 0;
	pAY8910Buffer[1] = pFMBuffer + nBurnSoundLen * 1;
	pAY8910Buffer[2] = pFMBuffer + nBurnSoundLen * 2;
	pAY8910Buffer[3] = pFMBuffer + nBurnSoundLen * 3;
	pAY8910Buffer[4] = pFMBuffer + nBurnSoundLen * 4;
	pAY8910Buffer[5] = pFMBuffer + nBurnSoundLen * 5;
	pAY8910Buffer[6] = pFMBuffer + nBurnSoundLen * 6;
	pAY8910Buffer[7] = pFMBuffer + nBurnSoundLen * 7;
	pAY8910Buffer[8] = pFMBuffer + nBurnSoundLen * 8;

	AY8910Init(0, 1500000, nBurnSoundRate, NULL, NULL, NULL, NULL);
	AY8910Init(1, 1500000, nBurnSoundRate, NULL, NULL, NULL, NULL);
	AY8910Init(2, 1500000, nBurnSoundRate, NULL, NULL, NULL, NULL);

	// remember to do ZetReset() in main driver

	DrvDoReset();
	return 0;
}



void DecodeTiles(unsigned char *TilePointer, int num,int off1,int off2, int off3)
{
	int c,y,x,dat1,dat2,dat3,col;
	for (c=0;c<num;c++)
	{
		for (y=0;y<8;y++)
		{
			dat1=BjGfx[off1 + (c * 8) + y];
			dat2=BjGfx[off2 + (c * 8) + y];
			dat3=BjGfx[off3 + (c * 8) + y];
			for (x=0;x<8;x++)
			{
				col=0;
				if (dat1&1){ col |= 4;}
				if (dat2&1){ col |= 2;}
				if (dat3&1){ col |= 1;}
				TilePointer[(c * 64) + ((7-x) * 8) + (7 - y)]=col;
				dat1>>=1;
				dat2>>=1;
				dat3>>=1;
			}
		}
	}
}


static int MemIndex()
{
	unsigned char *Next; Next = Mem;

	BjRom		= Next; Next += (0x2000*5);
	SndRom		= Next; Next += 0x2000;
	RamStart	= Next;
	BjRam		= Next; Next += 0xffff;
	SndRam		= Next; Next += 0xffff;
	BjMap		= Next; Next += 0x1000;
	BjGfx		= Next; Next += 0x15000;
	BjPalSrc	= Next; Next += 0x00200;
	BjVidRam =Next; Next +=0x400;
	BjColRam =Next;Next += 0x400;
	BjSprRam =Next;Next +=0x100;
	pFMBuffer	= (short*)Next; Next += nBurnSoundLen * 9 * sizeof(short);
	RamEnd		= Next;
	BjPalReal	= (unsigned int*)Next; Next += 0x00100 * sizeof(unsigned int);
	text		=Next; Next+=0x8000;
	sprites		=Next; Next+=0x10000;
	tiles		=Next; Next+=0x10000;
	MemEnd		= Next;

	return 0;
}


int BjInit()
{
	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

	for (int i =0; i<5 ; i++)
	{
		BurnLoadRom(BjRom+(0x2000*i),i,1); // load code roms
	}

	for (int i=0;i<3;i++)
	{
		BurnLoadRom(BjGfx+(0x1000*i),i+5,1);
	}

	//	for (int i=0;i<5;i++)
	//	{
	//		BurnLoadRom(BjGfx+0x3000+(0x2000*i),i+8,1);
	//	}

	BurnLoadRom(BjGfx+0x3000,8,1);
	BurnLoadRom(BjGfx+0x5000,9,1);
	BurnLoadRom(BjGfx+0x7000,10,1);

	BurnLoadRom(BjGfx+0x9000,11,1);
	BurnLoadRom(BjGfx+0xB000,12,1);
	BurnLoadRom(BjGfx+0xD000,13,1);

	BurnLoadRom(BjMap,14,1); // load Background tile maps
	BurnLoadRom(SndRom,15,1); // load Background tile maps

	// Set memory access & Init
	BjZInit();

	DecodeTiles(text,512,0,0x1000,0x2000);
	DecodeTiles(sprites,1024,0x7000,0x5000,0x3000);
	DecodeTiles(tiles,1024,0x9000,0xB000,0xD000);
	// done

	GenericTilesInit();

	DrvDoReset();
	return 0;
}

int BjExit()
{
	ZetExit();

	for (int i = 0; i < 3; i++) {
		AY8910Exit(i);
	}

	GenericTilesExit();
	free(Mem);
	Mem	= NULL;
	return 0;
}

static unsigned int CalcCol(unsigned short nColour)
{
	int r, g, b;

	r = (nColour >> 0) & 0x0f;
	g = (nColour >> 4) & 0x0f;
	b = (nColour >> 8) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	return BurnHighCol(r, g, b, 0);
}

int CalcAll()
{
	for (int i = 0; i < 0x100; i++) {
		BjPalReal[i / 2] = CalcCol(BjPalSrc[i & ~1] | (BjPalSrc[i | 1] << 8));
	}

	return 0;
}

static int BjDraw()
{
	int x,y;
	int c;
	int tile;
	int colour;
	int BgSel=BjRam[0x9e00];
	int pos=0,pos2=0,pos3=0x200*(BgSel&7),pos4;
	pos4=pos3+0x100;
	int attrib;

	BurnTransferClear();
	CalcAll();

	if (BgSel&0x10)
	{
		for (x=15;x>-1;x--)
		{
			for (y=0;y<16;y++)
			{
				attrib=BjMap[pos4];
				if (attrib&0x80)
				{
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2), (x<<4)+8,(y<<4), attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+1, (x<<4)+8,(y<<4)+8, attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+2,(x<<4),(y<<4), attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+3,(x<<4),(y<<4)+8, attrib, 3, 0, 0, tiles);

				}
				else
				{
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2),(x<<4)+8,(y<<4), attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+1,(x<<4)+8,(y<<4)+8, attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+2,(x<<4),(y<<4), attrib, 3, 0, 0, tiles);
					Render8x8Tile_Mask(pTransDraw, (BjMap[pos3]<<2)+3,(x<<4),(y<<4)+8, attrib, 3, 0, 0, tiles);
				}
				pos3++;
				pos4++;
			}
		}
	}
	for (x=0;x<32;x++)
	{
		for (y=0;y<32;y++)
		{
			if (BgSel&0x10)
			{
				if (BjColRam[pos2]&0x10)
				{
					Render8x8Tile_Mask(pTransDraw, BjVidRam[pos++]+256,(31-x)<<3,y<<3,BjColRam[pos2++]&15, 3, 0, 0, text);
				}
				else
				{
					Render8x8Tile_Mask(pTransDraw, BjVidRam[pos++],(31-x)<<3,y<<3,BjColRam[pos2++]&15, 3, 0, 0, text);
				}
			}
			else
			{
				if (BjColRam[pos2]&0x10)
				{
					Render8x8Tile_Mask(pTransDraw, BjVidRam[pos++]+256,(31-x)<<3,y<<3,BjColRam[pos2++]&15, 3, 0, 0, text);
				}
				else
				{
					Render8x8Tile_Mask(pTransDraw, BjVidRam[pos++],(31-x)<<3,y<<3,BjColRam[pos2++]&15, 3, 0, 0, text);
				}
			}
		}
	}
	pos=0;
	for (c=0;c<24;c++)
	{
		//	flipx = spriteram[offs+1] & 0x40;
		//	flipy =	spriteram[offs+1] & 0x80;
		x=BjSprRam[pos+2]-1;
		y=BjSprRam[pos+3];
		if (x>8&&y>8)
		{
			tile=BjSprRam[pos]&0x7F;
			colour=BjSprRam[pos+1]&0x0F;
			attrib=BjSprRam[pos+1]&0xc0;

			switch (attrib){
			case 0:
				if (!(BjSprRam[pos]&0x80))
				{
					Render8x8Tile_Mask(pTransDraw, (tile<<2)  , x+8, y   ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<2)+1, x+8, y+8 ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<2)+2, x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<2)+3, x,y+8,colour, 3, 0, 0, sprites);
				}
				else
				{
					tile&=31;

					Render8x8Tile_Mask(pTransDraw, (tile<<4)+512,x+8+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+513,x+8+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+514,x+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+515,x+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+516,x+8+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+517,x+8+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+518,x+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+519,x+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+520,x+8,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+521,x+8,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+522,x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+523,x,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+524,x+8,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+525,x+8,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+526,x,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask(pTransDraw, (tile<<4)+527,x,y+8+16,colour, 3, 0, 0, sprites);
				}
				break;
			case 0x80:
				if (!(BjSprRam[pos]&0x80))
				{
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<2)+2, x+8,y   ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<2)+3, x+8,y+8 ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<2), x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<2)+1, x,y+8,colour, 3, 0, 0, sprites);
				}
				else
				{
					tile&=31;
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+512,x+8+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+513,x+8+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+514,x+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+515,x+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+516,x+8+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+517,x+8+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+518,x+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+519,x+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+520,x+8,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+521,x+8,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+522,x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+523,x,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+524,x+8,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+525,x+8,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+526,x,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipX(pTransDraw, (tile<<4)+527,x,y+8+16,colour, 3, 0, 0, sprites);
				}
				break;
			case 0x40:
				if (!(BjSprRam[pos]&0x80))
				{
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<2)+1, x+8,y   ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<2), x+8,y+8 ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<2)+3, x,y, 3,colour, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<2)+2, x,y+8,colour, 3, 0, 0, sprites);
				}
				else
				{
					tile&=31;
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+512,x+8+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+513,x+8+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+514,x+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+515,x+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+516,x+8+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+517,x+8+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+518,x+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+519,x+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+520,x+8,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+521,x+8,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+522,x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+523,x,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+524,x+8,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+525,x+8,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+526,x,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipY(pTransDraw, (tile<<4)+527,x,y+8+16,colour, 3, 0, 0, sprites);
				}
				break;
			case 0xc0:
				if (!(BjSprRam[pos]&0x80))
				{
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<2)+3, x+8,y   ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<2)+2, x+8,y+8 ,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<2)+1, x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<2), x,y+8,colour, 3, 0, 0, sprites);
				}
				else
				{
					tile&=31;
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+512,x+8+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+513,x+8+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+514,x+16,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+515,x+16,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+516,x+8+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+517,x+8+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+518,x+16,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+519,x+16,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+520,x+8,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+521,x+8,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+522,x,y,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+523,x,y+8,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+524,x+8,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+525,x+8,y+8+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+526,x,y+16,colour, 3, 0, 0, sprites);
					Render8x8Tile_Mask_FlipXY(pTransDraw, (tile<<4)+527,x,y+8+16,colour, 3, 0, 0, sprites);
				}
				break;
			default:
				bprintf(PRINT_NORMAL, PRINT_NORMAL, PRINT_NORMAL, "attrib %x\n",attrib);
				break;
			}
		}
		pos+=4;
	}
	BurnTransferCopy(BjPalReal);
	return 0;
}
// end

int BjFrame()
{
	if (DrvReset) {	// Reset machine
		DrvDoReset();
	}

	int nInterleave = 10;
	int nSoundBufferPos = 0;

	nCyclesTotal[0] = 4000000 / 60;
	nCyclesTotal[1] = 3072000 / 60;
	nCyclesDone[0] = nCyclesDone[1] = 0;

	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		// Run Z80 #1
		nCurrentCPU = 0;
		ZetOpen(nCurrentCPU);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += ZetRun(nCyclesSegment);
		if (i == 1) 
		{
			if(bombjackIRQ)
			{
				ZetNmi();
			}
		}
		ZetClose();

		// Run Z80 #2
		nCurrentCPU = 1;
		ZetOpen(nCurrentCPU);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();

		// Render Sound Segment
		if (pBurnSoundOut) {
			int nSample;
			int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			AY8910Update(0, &pAY8910Buffer[0], nSegmentLength);
			AY8910Update(1, &pAY8910Buffer[3], nSegmentLength);
			AY8910Update(2, &pAY8910Buffer[6], nSegmentLength);
			for (int n = 0; n < nSegmentLength; n++) {
				nSample  = pAY8910Buffer[0][n];
				nSample += pAY8910Buffer[1][n];
				nSample += pAY8910Buffer[2][n];
				nSample += pAY8910Buffer[3][n];
				nSample += pAY8910Buffer[4][n];
				nSample += pAY8910Buffer[5][n];
				nSample += pAY8910Buffer[6][n];
				nSample += pAY8910Buffer[7][n];
				nSample += pAY8910Buffer[8][n];

				nSample /= 4;

				if (nSample < -32768) {
					nSample = -32768;
				} else {
					if (nSample > 32767) {
						nSample = 32767;
					}
				}

				pSoundBuf[(n << 1) + 0] = nSample;
				pSoundBuf[(n << 1) + 1] = nSample;
			}
			nSoundBufferPos += nSegmentLength;
		}
	}

	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSample;
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
		if (nSegmentLength) {
			AY8910Update(0, &pAY8910Buffer[0], nSegmentLength);
			AY8910Update(1, &pAY8910Buffer[3], nSegmentLength);
			AY8910Update(2, &pAY8910Buffer[6], nSegmentLength);
			for (int n = 0; n < nSegmentLength; n++) {
				nSample  = pAY8910Buffer[0][n];
				nSample += pAY8910Buffer[1][n];
				nSample += pAY8910Buffer[2][n];
				nSample += pAY8910Buffer[3][n];
				nSample += pAY8910Buffer[4][n];
				nSample += pAY8910Buffer[5][n];
				nSample += pAY8910Buffer[6][n];
				nSample += pAY8910Buffer[7][n];
				nSample += pAY8910Buffer[8][n];

				nSample /= 4;

				if (nSample < -32768) {
					nSample = -32768;
				} else {
					if (nSample > 32767) {
						nSample = 32767;
					}
				}

				pSoundBuf[(n << 1) + 0] = nSample;
				pSoundBuf[(n << 1) + 1] = nSample;
			}
		}
	}
	/*ZetOpen(0);
	if (BjRam[0xb000])
		ZetNmi();
	ZetClose();*/

	ZetOpen(1);
	ZetNmi();
	ZetClose();


	if (pBurnDraw != NULL)
	{

		BjDraw();	// Draw screen if needed

	}
	return 0;
}

static int BjScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x029521;
	}

	if (nAction & ACB_VOLATILE) {		// Scan volatile ram		
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		ZetScan(nAction);			// Scan Z80

		// Scan critical driver variables
		SCAN_VAR(latch);
		SCAN_VAR(DrvJoy1);
		SCAN_VAR(DrvJoy2);		
		SCAN_VAR(BjDip);
	}

	return 0;
}

struct BurnDriver BurnDrvBombjack = {
	"bombjack", NULL, NULL, "1984",
	"Bomb Jack (set 1)\0", NULL, "Tehkan", "Bomb Jack",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_MISC,
	NULL,BombjackRomInfo,BombjackRomName,DrvInputInfo,BjDIPInfo,
	BjInit,BjExit,BjFrame,NULL,BjScan,
	NULL,256,256,3,4
};

struct BurnDriver BurnDrvbombjac2 = {
	"bombjac2", "bombjack", NULL, "1984",
	"Bomb Jack (set 2)\0", NULL, "Tehkan", "Bomb Jack",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE,2,HARDWARE_MISC_MISC,
	NULL,Bombjac2RomInfo,Bombjac2RomName,DrvInputInfo,BjDIPInfo,
	BjInit,BjExit,BjFrame,NULL,BjScan,
	NULL,256,256,3,4
};
