/* Author: Sam Matthews
 * This program uses the included allcode api to make a Microchip PIC navigate mazes. This uses the left hand rule for exploration and attempts to form a basic map of the environment. It is meant to operate in a specific 4x4 grid maze and also does not implement searching algorithms for navigating using the model unfortunately. However, it is fairly robust and minimal compared to alternative implementations.
 * */

#include "allcode_api.h"

unsigned long leftTurnEndTime = 0;
int leftTurnDelay = 285; //Time waited until deciding to turn left after detecting a line
bool onLine = false; //Whether or not the robot is currently traversing a line
int orientation = 0; //Which direction the robot is facing (0 - 3 = forward - left counting clockwise)
struct Cell { //Stores information about a square in the maze
  bool visited;
  /*Each element in sides is a direction that represents a wall (0) or an opening (1) 
    The index of the numer corresponds with the orientation*/
	int sides[4]; 
}grid[4][4]; //The main maze model
int nest[2] = {4,4}; //The coordinates of the nest in the maze initially set to an impossible value
int xVal = 1; //Current x coordinate of the robot
int yVal = 0; //Current y coordinate of the robot
int cellCount = 0; //How many squares have been visited

/*
 * Avoiding behaviour function for turning left and right 90 degrees in the maze
 * and follows the left wall
 */
void avoid() {
	FA_SetMotors(42,30);

  /*Turn left at the gap due to left wall following*/
	if(FA_ReadIR(IR_LEFT) < 5) { //Could put these two if statements into one but that actually results in poorer behaviour
		if(FA_ClockMS() >= leftTurnEndTime && leftTurnEndTime > 0 && FA_ReadEncoder(CHANNEL_LEFT) > 200 && FA_ReadEncoder(CHANNEL_LEFT) > 200) {
			FA_Left(91);  
			leftTurnEndTime = 0; 
      FA_ResetEncoders();
      orientation = (orientation == 0) ? 3 : orientation - 1;
		}
	}

  /*Detect a wall turn right 90 degrees*/
	else if(FA_ReadIR(IR_FRONT) > 300 && FA_ReadIR(IR_FRONT) > 10) {
    if(FA_ReadIR(IR_LEFT) > 10) {
		  //FA_Backwards(10);
		  FA_Right(88);
      orientation = (orientation == 3) ? 0 : orientation + 1;
    }
    /*Just in case robot needs to turn left to follow the wall as an emergency*/
    else if(leftTurnEndTime > 0) {
      FA_Left(91);
      leftTurnEndTime = 0;
      orientation = (orientation == 0) ? 3 : orientation - 1;
    }
	}
}   

/*Function for making minor corrections when the robot is too close to a wall*/
void keepStraight() {
	if((FA_ReadIR(IR_RIGHT) > 1000 || FA_ReadIR(IR_FRONT_RIGHT) > 1000) && FA_ReadIR(IR_FRONT) < 200 && !onLine) {
		FA_Left(3);
  }
	if((FA_ReadIR(IR_LEFT) > 1000 || FA_ReadIR(IR_FRONT_LEFT) > 1000) && FA_ReadIR(IR_FRONT) < 200 && !onLine) {
		FA_Right(3);
  }
}

/*Updates information about the maze after a line is traversed*/
void locationDetails() {
  if(!grid[xVal][yVal].visited) {
    cellCount++;
    grid[xVal][yVal].visited = true;
    /*Setting each element according to the read values accounting for orientation*/
    grid[xVal][yVal].sides[orientation] = (FA_ReadIR(IR_FRONT) > 200) ? 0 : 1;
    grid[xVal][yVal].sides[(orientation + 1) % 4] = (FA_ReadIR(IR_RIGHT) > 200) ? 0 : 1;
    grid[xVal][yVal].sides[(orientation + 2) % 4] = (FA_ReadIR(IR_REAR) > 200) ? 0 : 1; //Only happens at start
    grid[xVal][yVal].sides[(orientation + 3) % 4] = (FA_ReadIR(IR_LEFT) > 200) ? 0 : 1;
  }
}

/*Updates the location of the robot in the internal model when a line is traversed*/
void updateLocation() {
  switch(orientation) {
    case 0:
      yVal = (yVal + 1) % 4;
      break;
    case 1:
      xVal = (xVal + 1) % 4;
      break;
    case 2:
      yVal = (yVal > 0) ? (yVal - 1) % 4 : 3;
      break;
    case 3:
      xVal = (xVal > 0) ? (xVal - 1) % 4 : 3;
      break;
  }
}

/*Detects the lines on the floor and initiates a timer for turning left if necessary*/
void readLines() {
	if(FA_ReadLine(0) < 200 && FA_ReadLine(1) < 200 && !onLine) {
		onLine = true;
    leftTurnEndTime = 0;
	}
	else if(FA_ReadLine(0) > 400 && FA_ReadLine(1) > 400 && onLine && leftTurnEndTime == 0) {
		onLine = false;
		updateLocation();
		leftTurnEndTime = FA_ClockMS() + leftTurnDelay;
    FA_ResetEncoders();
	}
}

/*Dance that is done at the end of the program once the maze has been explored entirely*/
void victoryDance() {
  FA_Forwards(60);
  FA_SetMotors(0,0);
  FA_DelayMillis(500);
  FA_Left(20);
  FA_Forwards(20);
  FA_Backwards(20);
  FA_Right(40);
  FA_Forwards(20);
  FA_Backwards(20);
  FA_Right(320);
}

int main() {
	FA_RobotInit();
  FA_LCDBacklight(50);
  /*Setting all of the cells to not visited and not marked for revisiting*/
  int i = 0;
  int j = 0;
  for(i; i < 4; i++) {
    for(j; j < 4; j++)
      grid[i][j].visited = false;
  }
  /*Main exploration loop*/
   while(cellCount < 16 || nest[0] > 3) {   
    locationDetails();
		avoid();
		keepStraight();
		readLines();
		if(FA_ReadLight() < 700 && nest[0] > 3) {
			nest[0] = xVal;
			nest[1] = yVal;
		}
  }
  victoryDance();
  /*Exploring until nest is found*/
  while(FA_ReadLight() > 600) {
    avoid();
    keepStraight();
    readLines();
  }
  /*Ensures that the robot will move forward into the nest*/
  while(FA_ReadIR(IR_FRONT) < 300)
    FA_SetMotors(42,30);
  FA_SetMotors(0,0);
 	return 0;
}
