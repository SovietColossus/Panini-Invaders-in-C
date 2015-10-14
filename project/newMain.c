#include <stm32f30x.h> // Pull in include files for F30x standard drivers 
#include <f3d_led.h> // Pull in include file for the local drivers
#include <f3d_uart.h>
#include <f3d_gyro.h>
#include <f3d_lcd_sd.h>
#include <f3d_i2c.h>
#include <f3d_accel.h>
#include <f3d_mag.h>
#include <f3d_nunchuk.h>
#include <f3d_rtc.h>
#include <f3d_systick.h>
#include <f3d_timer2.h>
#include <f3d_dac.h>
#include <f3d_systick.h>
#include <ff.h>
#include <diskio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIMER 20000


//////////////////////////////////////////////////AUDIO STUFF!!!!
#define AUDIOBUFSIZE 256 //128

extern uint8_t Audiobuf[AUDIOBUFSIZE];
extern int audioplayerHalf;
extern int audioplayerWhole;

nunchuk_t nunData;
void die (FRESULT rc) {
  printf("Failed with rc=%u.\n", rc);
  while (1);
}

FATFS Fatfs;		/* File system object */
FIL Fil;		/* File object */
FIL fid;
BYTE Buff[128];		/* File read buffer */
FRESULT rc;     /* Result code */
DIR dir;      /* Directory object */
FILINFO fno;      /* File information object */
UINT bw, br;
unsigned int retval;
unsigned int retval;
uint16_t color[128];
int ret;
int wav, flag;

struct bmpfile_magic {
  unsigned char magic [2];
};
struct bmpfile_header {
  uint32_t filesz ;
  uint16_t creator1 ;
  uint16_t creator2 ;
  uint32_t bmp_offset ;
};
typedef struct {
  uint32_t header_sz ;
  int32_t width ;
  int32_t height ;
  uint16_t nplanes ;
  uint16_t bitspp ;
  uint32_t compress_type ;
  uint32_t bmp_bytesz ;
  int32_t hres;
  int32_t vres;
  uint32_t ncolors ;
  uint32_t nimpcolors ;
} BITMAPINFOHEADER ;

struct bmppixel { // little endian byte order
  uint8_t b;
  uint8_t g;
  uint8_t r;
};

struct ckhd {
  uint32_t ckID;
  uint32_t cksize;
};

struct fmtck {
  uint16_t wFormatTag; 
  uint16_t nChannels;
  uint32_t nSamplesPerSec;
  uint32_t nAvgBytesPerSec;
  uint16_t nBlockAlign;
  uint16_t wBitsPerSample;
};

void readckhd(FIL *fid, struct ckhd *hd, uint32_t ckID) {
  f_read(fid, hd, sizeof(struct ckhd), &ret);
  if (ret != sizeof(struct ckhd))
    exit(-1);
  if (ckID && (ckID != hd->ckID))
    exit(-1);
}

struct bmpfile_magic magic;
struct bmpfile_header header;
struct bmppixel killme[128];
BITMAPINFOHEADER info;

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t Red = r >> 3;
  uint16_t Green = g >> 2;
  uint16_t Blue = b >> 3;
  Red = Red << 11;
  Green = Green << 5;
  return Red | Green | Blue;
}

/* void writeHighScore(int finalScore) { */
/*   char c[1000]; */
/*   FILE *fptr; */
/*   fptr=fopen("highscore.txt","w"); */
/*   if(fptr==NULL){ */
/*     printf("Error!"); */
/*     exit(1); */
/*   } */
/*   fprintf(fptr,"%s",c); */
/*   fclose(fptr); */
/* } */

void playWhatever() {
 if(rc) die(rc);
  if (!rc) {
    struct ckhd hd;
    uint32_t waveid;
    struct fmtck fck;    

    readckhd(&fid, &hd, 'FFIR');

    f_read(&fid, &waveid, sizeof(waveid), &ret);
    if ((ret != sizeof(waveid)) || (waveid != 'EVAW'))
      return -1;

    readckhd(&fid, &hd, ' tmf');

    f_read(&fid, &fck, sizeof(fck), &ret);

    if (hd.cksize != 16) {
      printf("extra header info %d\n", hd.cksize - 16);
      f_lseek(&fid, hd.cksize - 16);
    }

    printf("audio format 0x%x\n", fck.wFormatTag);
    printf("channels %d\n", fck.nChannels);
    printf("sample rate %d\n", fck.nSamplesPerSec);
    printf("data rate %d\n", fck.nAvgBytesPerSec);
    printf("block alignment %d\n", fck.nBlockAlign);
    printf("bits per sample %d\n", fck.wBitsPerSample);

    // now skip all non-data chunks !

    while(1){
      readckhd(&fid, &hd, 0);
      if (hd.ckID == 'atad')
	break;
      f_lseek(&fid, hd.cksize);
    }

    printf("Samples %d\n", hd.cksize);

    // Play it !
    // audioplayerInit(fck.nSamplesPerSec);

    f_read(&fid, Audiobuf, AUDIOBUFSIZE, &ret);
    hd.cksize -= ret;
    audioplayerStart();
    while (hd.cksize) {

      int next = hd.cksize > AUDIOBUFSIZE/2 ? AUDIOBUFSIZE/2 : hd.cksize;
      if (audioplayerHalf) {
	if (next < AUDIOBUFSIZE/2)
	  bzero(Audiobuf, AUDIOBUFSIZE/2);
	
	f_read(&fid, Audiobuf, next, &ret);
	
	hd.cksize -= ret;

	audioplayerHalf = 0;

      }
      
      if (audioplayerWhole) {
	if (next < AUDIOBUFSIZE/2)
	  bzero(&Audiobuf[AUDIOBUFSIZE/2], AUDIOBUFSIZE/2);
	f_read(&fid, &Audiobuf[AUDIOBUFSIZE/2], next, &ret);
	hd.cksize -= ret;
	audioplayerWhole = 0;
      }
    }
    audioplayerStop();
  }

  printf("\nClose the file.\n");
  rc = f_close(&fid);

  if (rc) die(rc);
  //while (1); This tripped us up, nooooot cool.
}

////////////////////////////////////////////////////////////////















//Color matrix needed for updating with stronger aliens
int matrix[12][11];
int enemyFronts[11];
uint16_t alienColor = GREEN;
uint16_t bullerUser[4] = {WHITE, WHITE, WHITE, WHITE};
uint16_t bulletAlien[4] = {WHITE, WHITE, WHITE, WHITE};
uint16_t bulletReset[4] = {BLACK, BLACK, BLACK, BLACK};
uint16_t fullLife = WHITE;
uint16_t lessLife = BLUE;
uint16_t leastLife = YELLOW;
int selectionFlag=0;
char a[20]; //used for updating score
nunchuk_t nunData;

int nunchukFlag;// = 0;
int userBallFlag;// = 0;
int enemyBallFlag;// = 0;
int shipPosition;// = 0;
int sideOffset;// = 0; //used for enemyHead

int alienCount;// = 0;
int alienBulletCount;// = 0;
int lives;// = 3;
int score;//=0;
int moveCount;//=1;
int moveFlag;//=0;
int moveDelay;// = 500; 
int whichAlienMovement;//=1; //0 = down, 1 and 3 = right, 2 and 4 = left

int offset=1; //How far down have the aliens moved?

struct ball
{
  int row;
  int col;
};
  
struct ball userBall = {11, 0};
struct ball enemyBall;

//Set up the matrix
void initialize()
{
  f3d_lcd_fillScreen(BLACK);
  f3d_lcd_drawString(5,5,"Score:",WHITE,BLACK);
  f3d_lcd_drawString(40,5,"0",WHITE,BLACK);
  f3d_lcd_drawString(5,148,"Lives:",RED,BLACK);
  f3d_lcd_drawChar(40,148,'0',leastLife,BLACK);
  f3d_lcd_drawChar(50,148,'0',lessLife,BLACK);
  f3d_lcd_drawChar(60,148,'0',fullLife,BLACK);

  int i, j;
  for(i=0; i<12; i++)
    {
      for(j=0; j<11; j++)
	{
	  if(i>=1 && i<=5 && j>=1 && j<=9)
	    {
	      matrix[i][j] = 1;
	      f3d_lcd_drawChar(9+10*j,25+10*i,'P',alienColor,BLACK);
	    }
	  else if(i==11 && j==5) 
	    {
	      matrix[i][j] = -1;
	      shipPosition = 5;
	      f3d_lcd_drawChar(9+10*j,25+10*i,'S',WHITE,BLACK);
	    }
	  else
	    {
	      matrix[i][j] = 0;
	      //f3d_lcd_drawChar(9+10*j,25+10*i,'F',BLUE,BLACK);
	    }
	}
    }
  for(i=1; i<=11; i++)
    {
      enemyFronts[i] = 5;
    }
  enemyFronts[0]=0;
  enemyFronts[10]=0;
}

//Moves ship left or right
void updateShip(int direction)
{
  if (direction == 0) 
    {
      if (shipPosition != 0) 
	{
	  f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', BLACK, BLACK);
	  shipPosition--;
	  if(lives==3)
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', fullLife, BLACK);
	  else if(lives==2)
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', lessLife, BLACK);
	  else
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', leastLife, BLACK);
	} 
    } 
  else if (direction == 1) 
    {
      if (shipPosition != 10) 
	{
	  f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', BLACK, BLACK);
	  shipPosition++;
	  if(lives==3)
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', fullLife, BLACK);
	  else if(lives==2)
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', lessLife, BLACK);
	  else
	    f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', leastLife, BLACK);
	} 
    }
  else
    {
      if(lives==2)
	{
	  f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', lessLife, BLACK);
	  f3d_lcd_drawChar(60,148,'0',BLACK,BLACK);
	}
      else
	{
	  f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', leastLife, BLACK);
	  f3d_lcd_drawChar(50,148,'0',BLACK,BLACK);
	}
    }
}

void win()
{
  int z;
  f3d_lcd_drawString(25,40,"Congratulations!", WHITE, BLACK);
  f3d_lcd_drawString(25,50,"A Winner is You!", WHITE, BLACK);
  printf("\nOpen win.wav\n");
  delay(10);
  rc = f_open(&fid, "win.wav", FA_READ);
  playWhatever();
  delay(10);
  f3d_lcd_drawString(10,100,"PRESS Z TO WITNESS",WHITE,BLACK);
  f3d_lcd_drawString(10,112,"PANERA BREAD MENU.",WHITE,BLACK);
  delay(100);

  while(selectionFlag == 1)
    {
      f3d_nunchuk_read(&nunData);
      z = nunData.z;
      printf("Just in this loop!2\n");
      if(z==1) 
	{
	  selectionFlag = 0;
	  z = 0;
	}
    }
  lives=3;
}

void gameOver()
{
  int z;
  f3d_lcd_drawString(72,5,"GAMEOVER!",WHITE,BLACK);
  f3d_lcd_drawChar(40,148,'0',BLACK,BLACK);
  printf("\nOpen thermo.wav\n");
  delay(10);
  rc = f_open(&fid, "thermo.wav", FA_READ);
  playWhatever();
  delay(10);
  f3d_lcd_drawString(20,100,"PRESS Z TO WITNESS MAIN MENU",WHITE,BLACK);
  delay(100);

  while(selectionFlag == 1)
    {
      f3d_nunchuk_read(&nunData);
      z = nunData.z;
      printf("Just in this loop!2\n");
      if(z==1) 
	{
	  selectionFlag = 0;
	  z = 0;
	}
    }
  lives=3;
}

void updateAliens(){
  int i, j;
  //printf("Updating aliens. Movement type: %d\n", whichAlienMovement);
  if(!whichAlienMovement)
    { //zero movement = down
      for(i=offset+5; i>=offset; i--)
	{
	  for(j=1; j<=9; j++)
	    {
	      if(matrix[i][j]!=matrix[i-1][j]) //Change cell
		{
		  if(matrix[i-1][j]==0) //Cell above was empty
		    {
		      f3d_lcd_drawChar(9+10*j,25+10*i,'P',BLACK,BLACK); //Become occupied
		    }
		  else //(matrix[i-1][j]>0) Cell above was an alien
		    {
		      if(i==11) //The alien attacks!
			{
			  f3d_lcd_drawChar(9+10*j,25+10*(i-1),'P',BLACK,BLACK);
			  f3d_lcd_drawChar(9+10*j,25+10*i,'P',RED,BLACK);
			  gameOver();
			  break;
			}
		      else //The alien moves
			f3d_lcd_drawChar(9+10*j,25+10*i,'P',alienColor,BLACK); //Become empty
		    }
		  matrix[i][j]=matrix[i-1][j]; //New Entry
		}
	    }
	}
      //In case the front row has already been killed off but aliens are still moving!
      if(offset<7)
	offset++;
    }
  else if(whichAlienMovement==1 || whichAlienMovement==4)
    { //odd movement = right
      for(i=offset; i<=offset+4; i++)
	{
	  for(j=10; j>=0; j--)
	    {
	      if(i<11) //Don't repaint the ship!
		{
		  if(j==0 && !matrix[i][j])
		    f3d_lcd_drawChar(9+10*j,25+10*i,'P',BLACK,BLACK); //Become empty
		  else if(matrix[i][j]!=matrix[i][j-1]) //Change cell
		    {
		      if(matrix[i][j]==0)
			{
			  f3d_lcd_drawChar(9+10*j,25+10*i,'P',alienColor,BLACK); //Become occupied
			}
		      else if(matrix[i][j]>0) //Was an alien
			{
			  f3d_lcd_drawChar(9+10*j,25+10*i,'P',BLACK,BLACK); //Become empty
			}
		  
		      matrix[i][j]=matrix[i][j-1]; //New Entry
		    }
		}
	    }
	}
      sideOffset--;
    }
  else
    { //even nonzero movement = left
      for(i=offset; i<=offset+4; i++)
	{
	  for(j=0; j<=10; j++)
	    {
	      if(i<11) //Don't repaint the ship!
		{
		  if(j==10 && !matrix[i][j])
		    f3d_lcd_drawChar(9+10*j,25+10*i,'P',BLACK,BLACK); //Become empty
		  else if(matrix[i][j]!=matrix[i][j+1]) //Change cell
		    {
		      if(matrix[i][j]==0)
			{
			  f3d_lcd_drawChar(9+10*j,25+10*i,'P',alienColor,BLACK); //Become occupied
			}
		      else if(matrix[i][j]>0) //Was an alien
			{
			  f3d_lcd_drawChar(9+10*j,25+10*i,'P',BLACK,BLACK); //Become empty
			}
		  
		      matrix[i][j]=matrix[i][j+1]; //New Entry
		    }
		}
	    }
	}
      sideOffset++;
    }
  whichAlienMovement=(whichAlienMovement+1)%5;
}

////////////////////////////////////////////////MAIN MENU











//////////////////////////////////////////////////THE GAME
int main(void) 
{
  //Initialize/Configure/Enable Hardware
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  f3d_led_init();
  delay(10);
  f3d_uart_init();
  delay(10);
   printf("INITS \n");
  f3d_timer2_init();
  delay(10);
  f3d_i2c1_init();
  delay(100);
  f3d_nunchuk_init();
  delay(10);
  f3d_dac_init();
  delay(10);
  f3d_delay_init();
  delay(10);
  f3d_rtc_init();
  delay(10);
  f3d_systick_init();
  delay(10);
  f3d_lcd_init();
  delay(10);

  f_mount(0, &Fatfs);
  
  while(1)
    {
      printf("Made it into loop!\n");
      while(selectionFlag==0)
	{
	  int z, i, j;
	  char footer[20];
	    
	  f3d_lcd_fillScreen(BLACK);
	  printf("Made it here!\n");

	  rc = f_open(&Fil, "mainMenu.bmp", FA_READ);
	  if (rc) die(rc);
	  for (;;) 
	    {
	      rc = f_read(&Fil, &magic, sizeof magic, &br); /* Read a chunk of file */
	      if (rc || !br) break;      /* Error or end of file */
	      rc = f_read(&Fil, &header, sizeof header, &br); /* Read a chunk of file */
	      if (rc || !br) break;      /* Error or end of file */
	      rc = f_read(&Fil, &info, sizeof info, &br); /* Read a chunk of file */
	      if (rc || !br) break;      /* Error or end of file */
	      for (i = 159; i >= 0; i--) 
		{
		  rc = f_read(&Fil, &killme, sizeof killme, &br); /* Read a chunk of file */
		  if (rc || !br) die(rc);                         /* Error or end of file */
		  else {
		    f3d_lcd_setAddrWindow (0,i,ST7735_width-1,i,MADCTLGRAPHICS);
		    for (j = 0; j < 128; j++) {
		      color[j] = convertColor(killme[j].r, killme[j].g, killme[j].b);
		    }
		    f3d_lcd_pushColor(color,128);
		  }
		}
	    }
	  
	  if (rc) die(rc);
	  rc = f_close(&Fil);

	  printf("\nOpen ufo2.wav\n");
	  delay(10);
	  rc = f_open(&fid, "ufo2.wav", FA_READ);
	  playWhatever();
	  delay(100);
	  
	  f3d_lcd_drawString(20,130,"PRESS Z TO START",WHITE,BLACK);
	  delay(100);
	  //printf("Is selection flag zero? %d", selectionFlag);
	  z = 0;
	  selectionFlag=0;
	  if(selectionFlag == 0)
	    {
	      //printf("Just in this loop!\n");
	      f3d_nunchuk_read(&nunData);
	      z = nunData.z;
	      while(z != 1)
		{
		  f3d_nunchuk_read(&nunData);
		  delay(30);
		  z = nunData.z;
		  printf("Stuck in this loop with z = %d\n", z);
		}
	      //if(z==1)
		selectionFlag = 1;
	    }
	}

      if(selectionFlag==1)
	{
	  //Game Initializations
	  initialize();
	  //delay(20);
	  f3d_lcd_drawString(20,90,"Get Ready!", RED, BLACK);
	    delay(300);
	    f3d_lcd_drawString(20,90,"Get Ready!", BLUE, BLACK);
	    delay(300);
	    f3d_lcd_drawString(20,90,"Get Ready!", YELLOW, BLACK);
	    delay(300);
	    f3d_lcd_drawString(20,90,"Get Ready!", BLACK, BLACK);
	    f3d_lcd_drawString(60,90,"GO!",WHITE,BLACK);
	    delay(500);
	    f3d_lcd_drawString(60,90,"GO!",BLACK,BLACK);
	  
	  int jx, z, c, tempRandom;
	  union randomNumber
	  {
	    int intValue;
	    float floatValue;
	  } random;
	  int counterNun = 0;
	  int counterPlus = 0;
	  int ballPosition; //Used for animation

	  nunchukFlag = 0;
	  userBallFlag = 0;
	  enemyBallFlag = 0;
	  sideOffset = 0; //used for enemyHead
	  
	  alienBulletCount = 0;
	  lives = 3;
	  score=0;
	  moveCount=1;
	  moveFlag=0;
	  moveDelay = 500; 
	  whichAlienMovement=1; //0 = down, 1 and 3 = right, 2 and 4 = left
	  
	  offset=1; //How far down have the aliens moved?
	  
	  while(selectionFlag==1) //GAME LOOP
	    {
	      counterNun++;
	      counterPlus++; //How does this work?
	      
	      //Updates aliens movement
	      moveCount = (moveCount+1)%moveDelay;
	      
	      if(userBallFlag)
		{
		  if(userBall.row == enemyBall.row && userBall.col == enemyBall.col)
		    {
		      userBallFlag = 0;
		      enemyBallFlag = 0;
		      userBall.row = 12;
		      enemyBall.row = 0;
		    }
		  if(userBall.row == 0) //Hit the top
		    {
		      userBallFlag = 0;
		    }
		  else if(userBall.col+sideOffset >= 1 && userBall.col+sideOffset <= 9 && enemyFronts[userBall.col+sideOffset] != 0 && (userBall.row == (enemyFronts[userBall.col+sideOffset]+(offset-1)))) //Does it hit an alien?
		    {
		      userBallFlag = 0;
		      f3d_lcd_drawChar(9+10*userBall.col,25+10*(userBall.row),'A',BLACK, BLACK);
		      //Explosion effect goes here
		      matrix[userBall.row][userBall.col] = 0;
		      enemyFronts[userBall.col+sideOffset]--;
		      
		      //Update score
		      score += 20;
		      sprintf(a, "%i", score);
		      f3d_lcd_drawString(40,5, a, WHITE, BLACK);
		    }
		  else //Draw one frame and allow ship to move
		    {
		      int counterNunTemp = 0;
		      for(ballPosition=(25+userBall.row*10); ballPosition>((25+(userBall.row-1)*10)); ballPosition--) 
			{
			  f3d_lcd_setAddrWindow (12+10*userBall.col,ballPosition-1,13+10*userBall.col,ballPosition,MADCTLGRAPHICS);
			  f3d_lcd_pushColor(bullerUser,4);
			  
			  //Updates the alien movement
			  moveCount = (moveCount+20)%moveDelay;
			  if(moveCount==0)
			    moveFlag=1;
			  
			  /*f3d_nunchuk_read(&nunData);
			    
			  //Nunchuk movement loop for during animation time
			  if(jx > 253 || jx < 2 && counterNunTemp > 6) 
			  {
			  if(nunchukFlag) 
			  {
			  if(jx > 253 && counterNunTemp > 6) 
			  {
			  updateShip(1); //going right
			  counterNunTemp = 0;
			  }
			  else if(jx < 2 && counterNunTemp > 6) 
			  {
			  updateShip(0); //going left
			  counterNunTemp = 0;
			  }
			  nunchukFlag = 0;
			  }
			  else 
			  nunchukFlag = 1;
			  }
			  counterNunTemp++;*/
			  
			  //Delete ball just drawn
			  f3d_lcd_setAddrWindow (12+10*userBall.col,ballPosition-1,13+10*userBall.col,userBall.row,MADCTLGRAPHICS);
			  f3d_lcd_pushColor(bulletReset,4); 
			}
		      userBall.row--;
		    }
		}
	      
	      if(enemyBallFlag)
		{
		  if(enemyBall.row == userBall.row && enemyBall.col == userBall.col)
		    {
		      userBallFlag = 0;
		      enemyBallFlag = 0;
		      userBall.row = 12;
		      enemyBall.row = 0;
		    }
		  else //Draw one frame and check for ship death
		    {
		      int counterNunTemp2 = 0;
		      for(ballPosition=(25+(enemyBall.row+1)*10); ballPosition<((25+(enemyBall.row+2)*10)); ballPosition++) 
			{
			  f3d_lcd_setAddrWindow (12+10*enemyBall.col,ballPosition+1,13+10*enemyBall.col,enemyBall.row,MADCTLGRAPHICS);
			  f3d_lcd_pushColor(bulletAlien,4);
			  f3d_nunchuk_read(&nunData);
			  
			  //Updates the alien movement
			  moveCount = (moveCount+20)%moveDelay;
			  if(moveCount==0)
			    moveFlag=1;
			  
			  //Nunchuk movement loop for during animation time
			  if(jx > 253 || jx < 2 && counterNunTemp2 > 6) 
			    {
			      if(nunchukFlag) 
				{
				  if(jx > 253 && counterNunTemp2 > 6) 
				    {
				      updateShip(1); //going right
				      counterNunTemp2 = 0;
				    }
				  else if(jx < 2 && counterNunTemp2 > 6) 
				    {
				      updateShip(0); //going left
				      counterNunTemp2 = 0;
				    }
				  nunchukFlag = 0;
				}
			      else 
				nunchukFlag = 1;
			    }
			  counterNunTemp2++;
			  
			  //Delete ball just drawn
			  f3d_lcd_setAddrWindow (12+10*enemyBall.col,ballPosition+1,13+10*enemyBall.col,enemyBall.row,MADCTLGRAPHICS);
			  f3d_lcd_pushColor(bulletReset,4); 
			}
		      if(enemyBall.row==10)
			{
			  if(shipPosition == enemyBall.col)
			    {
			      lives--;
			      updateShip(2);
			      //enemyBall.row++;
			      enemyBallFlag=0;
			      enemyBall.row = 0;
			      if(lives==0)
				{
				  f3d_lcd_drawChar(9+10*shipPosition, 135, 'S', RED, BLACK);
				  gameOver();
				}
			    }
			  else
			    enemyBallFlag=0;
			}
		      else
			enemyBall.row++;
		    }
		}
	      else
		{
		  enemyBallFlag=0;
		  if(alienBulletCount)
		    alienBulletCount = (alienBulletCount+1)%(moveDelay/2);
		  //RANDOMLY GENERATE BULLET
		  else
		    {
		      f3d_nunchuk_read(&nunData);
		      random.floatValue = (nunData.ax);
		      tempRandom = random.intValue%9;
		      //printf("After read, %d, %d\n", tempRandom, enemyFronts[tempRandom]);
		      alienCount = 0;
		      while(1)
			{
			  printf("BLOO\n");
			  if(enemyFronts[tempRandom+1]!=0)
			    break;
			  else if(alienCount > 9)
			    {
			      win();
			      break;
			    }
			  else
			    {
			      tempRandom=(tempRandom+1)%9;
			      alienCount++;
			    }
			}
		      
		      enemyBallFlag = 1;
		      enemyBall.row = enemyFronts[tempRandom+1]+(offset-1);
		      printf("Enemy Fronts: %d\n", enemyBall.row);
		      enemyBall.col = (tempRandom+1) - sideOffset;
		    }
		}
	      
	      //Update positions
	      if(!moveCount || moveFlag)
		{
		  moveFlag=0;
		  updateAliens();
		  }
	      
	      //Get nunchuk info
	      f3d_nunchuk_read(&nunData);
	      jx = nunData.jx;
	      z = nunData.z;
	      c = nunData.c;
	      
	      //Temporary Update
	      if(c == 1 && counterPlus > 10) 
		{
		  //Update score
		  score += 42;
		  sprintf(a, "%i", score);
		  f3d_lcd_drawString(40,5, a, WHITE, BLACK);
		  updateAliens();
		}
	      
	      //Shoot
	      if(z == 1 && counterPlus > 10) 
		{
		  if(!userBallFlag)
		    {
		      userBallFlag = 1;
		      userBall.row = 11;
		      userBall.col = shipPosition;
		    }
		}
	      
	      //Change Ship Position
	      if(jx > 245 || jx < 10 && counterNun > 10) 
		{
		  if(nunchukFlag) 
		    {
		      if(jx > 245 && counterNun > 10) 
			{
			  updateShip(1); //going right
			  counterNun = 0;
			}
		      else if(jx < 10 && counterNun > 10) 
			{
			  updateShip(0); //going left
			  counterNun = 0;
			}
		      nunchukFlag = 0;
		    }
		  else 
		    nunchukFlag = 1;
		}
	    }
	}
    }
}


#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line) {
  /* Infinite loop */
  /* Use GDB to find out why we're here */
  while (1);
}
#endif
