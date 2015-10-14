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
////////////////////////////////////////////////////////////////////////////////////////
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

int main(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  f3d_uart_init();
  delay(10);
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

  f3d_lcd_fillScreen(BLACK);

  f_mount(0, &Fatfs);

  printf("Reset\n");
  int jx;
  int counterNun = 0;
  int counterPlus = 0;
  int z;
  int c;
  int i,j;
  char footer[20];
  f3d_nunchuk_read(&nunData);


  while(c != 1) {
    f3d_nunchuk_read(&nunData);
    c = nunData.c;
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

    while(c != 1) {
      printf("\nOpen ufo2.wav\n");
      rc = f_open(&fid, "ufo2.wav", FA_READ);
      playWhatever();
      break;
    }
    break;
  }
  f3d_lcd_drawString(20,130,"PRESS C TO START",WHITE,BLACK);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
/* Infinite loop */
/* Use GDB to find out why we're here */
  while (1);
}
#endif

/* main.c ends here */
