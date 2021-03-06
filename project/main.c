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

//Color matrix needed for updating with stronger aliens
int matrix[12][11];
int nunchukFlag = 0;
int userBallFlag = 0;
int enemyBallFlag = 0;
int shipPosition = 0;
int enemyFronts[11];
int sideOffset = 0; //used for enemyHead
uint16_t alienColor = GREEN;
uint16_t bullerUser[4] = {WHITE, WHITE, WHITE, WHITE};
uint16_t bulletAlien[4] = {WHITE, WHITE, WHITE, WHITE};
uint16_t bulletReset[4] = {BLACK, BLACK, BLACK, BLACK};
nunchuk_t nunData;
int alienBulletCount = 0;
int alienCount;

int lives = 3;
uint16_t fullLife = WHITE;
uint16_t lessLife = BLUE;
uint16_t leastLife = YELLOW;


int score=0;
int moveCount=1;
int moveFlag=0;
int moveDelay = 500; 
int whichAlienMovement=1; //0 = down, 1 and 3 = right, 2 and 4 = left
char a[20]; //used for updating score
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

void gameOver()
{
  f3d_lcd_drawString(72,5,"GAMEOVER!",WHITE,BLACK);
  f3d_lcd_drawChar(40,148,'0',BLACK,BLACK);
  while(1);
}

void updateAliens(){
  int i, j;
  int counterNun;
  int jx;
  
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

//The game
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
   printf("PLEASE WORK \n");
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
  
  printf("PLEASE WORK \n");
  //Game Initializations
  initialize();
  /*delay(20);
  f3d_lcd_drawString(20,90,"Get Ready!", RED, BLACK);
  delay(300);
  f3d_lcd_drawString(20,90,"Get Ready!", BLUE, BLACK);
  delay(300);
  f3d_lcd_drawString(20,90,"Get Ready!", YELLOW, BLACK);
  delay(300);
  f3d_lcd_drawString(20,90,"Get Ready!", BLACK, BLACK);
  f3d_lcd_drawString(60,90,"GO!",WHITE,BLACK);
  delay(500);
  f3d_lcd_drawString(60,90,"GO!",BLACK,BLACK);*/
  
  int jx, z, c, tempRandom;
  union randomNumber
  {
    int intValue;
    float floatValue;
  } random;
  int counterNun = 0;
  int counterPlus = 0;
  int ballPosition; //Used for animation
  
  while(1) //GAME LOOP
    {
      counterNun++;
      counterPlus++; //How does this work?
      
      //Updates aliens movement
      moveCount = (moveCount+1)%moveDelay;
      
      if(userBallFlag)
	{
	  printf("userBall.row is: %d\n", userBall.row);
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
	      printf("Userball.col + sideOffset: %d\n", userBall.col+sideOffset);
	      userBallFlag = 0;
	      f3d_lcd_drawChar(9+10*userBall.col,25+10*(userBall.row),'A',BLACK, BLACK);
	      //Explosion effect goes here
	      printf("User ball row: %d, User ball col: %d\n", userBall.row, userBall.col);
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
	  if(alienBulletCount)
	    alienBulletCount = (alienBulletCount+1)%(moveDelay/2);
	  //RANDOMLY GENERATE BULLET
	  else
	    {
	      f3d_nunchuk_read(&nunData);
	      printf("Before read\n");
	      random.floatValue = (nunData.ax);
	      tempRandom = random.intValue%9;
	      //printf("After read, %d, %d\n", tempRandom, enemyFronts[tempRandom]);
	      alienCount=0;
	      while(1)
		{
		  if(enemyFronts[tempRandom+1]!=0)
		    break;
		  else if (alienCount > 9)
		    {
		      win();
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


#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line) {
  /* Infinite loop */
  /* Use GDB to find out why we're here */
  while (1);
}
#endif
