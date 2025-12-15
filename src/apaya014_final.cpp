#include <stdlib.h> 
#include "timerISR.h"
#include "helper.h"
#include "spiAVR.h"
#include "serialATmega.h"
#include "sprite.h"
#include "periph.h"
#include "time.h"
#include "LCD.h"

#define NUM_TASKS 11 
#define SCREEN_WIDTH 130
#define SCREEN_HEIGHT 130
#define SHIP_HEIGHT 16
#define SHIP_WIDTH 16
#define ALIEN1_HEIGHT 8
#define ALIEN1_WIDTH 8
#define BULLET_WIDTH 8 
#define BULLET_HEIGHT 8
#define MAX_ALIENS 4

typedef struct {
    int x, y;
    int spriteColor;
    bool alive;
} Alien;

Alien aliens[MAX_ALIENS];

const unsigned int* alienSprites[] = { alien1, alien2, alien3, alien4 };
void initAliens() {
    for (unsigned int i = 0; i < MAX_ALIENS; i++) {
        //spawn aliens in random locations at the top of the screen
        aliens[i].x = rand() % (SCREEN_WIDTH - ALIEN1_WIDTH); 
        aliens[i].y = 0; //start from top of screen
        aliens[i].spriteColor = rand() % 4; //choose random colored aliens
        aliens[i].alive = 1;
    }
}

//variables for spaceship
//starting position for spaceship - bottom middle of screen
int spaceshipX = (130 - 16) / 2;
int spaceshipY = 109; 
//static int normalShipSpeed = 2; 
int lives = 3;
const int leftThreshold = 700;
const int rightThreshold = 400;


//bullet variables
int bulletX = 0;
int bulletY = 0;
static bool bulletOut = 0;

//score tracker
unsigned char score = 0;
unsigned char highScore = 0;

//alien variables
static int alienSpeed = 5;
static int wave = 1;
static int aliveAliens = MAX_ALIENS;

//game variables 
static bool gameOver = 0;
static bool gamereset = 0;
static bool gameStarted = 0;
static bool screenReady = 0;
// Power-up variables
static bool powerUpActive = 0;
static bool powerupReady = 0;
static unsigned long powerUpTimer = 0;
static int powerUpDuration = 300; //3 seconds
static int powerUpWaveCounter = 0;



void resetGame() {
    score = 0;
    lives = 3;
    wave = 1;
    alienSpeed = 5;
    bulletOut = 0;
    gameOver = 0;
    gameStarted = 0;
    initAliens();
    aliveAliens = MAX_ALIENS;
    powerUpActive = 0;
    powerupReady = 0;
    powerUpTimer = 0;
    powerUpWaveCounter = 0;
    PORTC = SetBit(PORTC, 5, 0);
    lcd_clear();
    lcd_goto_xy(0, 1); 
    lcd_write_str("Score: ");
    lcd_goto_xy(0, 0);             
    lcd_write_character(0);
    lcd_goto_xy(1, 0);              
    lcd_write_character(1);
    lcd_write_str("lives: ");                
}



typedef struct _task {
    signed char state; 
    unsigned long period; 
    unsigned long elapsedTime; 
    int (*TickFct)(int); 
} task;

const unsigned long backgroundPeriod = 100;
const unsigned long shipPeriod = 50;
const unsigned long alien1Period = 50;
const unsigned long scorePeriod = 50;
const unsigned long joystickMovePeriod = 20;
const unsigned long joystickButtonPeriod = 20;
const unsigned long bulletMovePeriod = 20;
const unsigned long displayLivesPeriod = 50;
const unsigned long drawShipPeriod = 10;
const unsigned long resetButtonPeriod = 50;
const unsigned long powerUpPeriod = 10;
const unsigned long GCD_PERIOD = 10;

task tasks[NUM_TASKS]; 

void TimerISR() {
    for (unsigned int i = 0; i < NUM_TASKS; i++) { 
        if (tasks[i].elapsedTime >= tasks[i].period) { 
            tasks[i].state = tasks[i].TickFct(tasks[i].state); 
            tasks[i].elapsedTime = 0; 
        }
        tasks[i].elapsedTime += GCD_PERIOD; 
    }
}

enum screenResetTasks {screenReset, done};
int TickFct_screenReset(int state) {
    switch(state) {
        case screenReset:
        //background sprite too big for drawsprite function, so do it manual
            sendCommand(0x2A); // CASET
            sendData(0x00); 
            sendData(0);       
            sendData(0x00); 
            sendData(SCREEN_WIDTH); 
            
            sendCommand(0x2B); // RASET
            sendData(0x00); 
            sendData(0);        
            sendData(0x00); 
            sendData(SCREEN_HEIGHT); 
            
            sendCommand(0x2C); // RAMWR
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                sendData(0x18);  
                sendData(0x40); // space background color 0x1840, very dark navy
            }
            screenReady = 1;  
            state = done; //prevent drawing over spaceship 
            break;
            
        case done:
        if(gamereset == 1) {
            screenReady = 0;
            state = screenReset;
        }
            break;
    }
    return state;
}

enum shipCreateStates {createShip, shipDone};
int TickFct_shipCreate(int state) {
    switch(state) {
        case createShip:
            if (screenReady) {
                drawSprite(ship, spaceshipX, spaceshipY, SHIP_WIDTH, SHIP_HEIGHT);
                gamereset = 0;
                state = shipDone; 
            }
            break;
        case shipDone:
            if(gamereset == 1) {
                state = createShip;
            }

            break;
    }
    return state;
}

enum alienStates {alienWave};
int TickFct_alien(int state) {
    static int tickCounter = 0;
    if (wave == 1 && !gameStarted && GetBit(PINC, 4) == 0) { //C4 is start button 
        gameStarted = 1;
    }
    if (!gameStarted || gameOver) { 
        return state;
    }

    switch (state) {
        case alienWave:
            if (++tickCounter >= alienSpeed) {
                tickCounter = 0;
                for (int i = 0; i < MAX_ALIENS; i++) {
                    if (aliens[i].alive == 0) continue; //if current alien is defeated, check next alien's state

                    drawSprite(blankSprite, aliens[i].x, aliens[i].y, ALIEN1_WIDTH, ALIEN1_HEIGHT);
                    //update currAlien y position and redraw
                    aliens[i].y += 1;
                    drawSprite(alienSprites[aliens[i].spriteColor], aliens[i].x, aliens[i].y, ALIEN1_WIDTH, ALIEN1_HEIGHT);

                    //alien-bullet collision checker (+2 and -2 to make hitbox more forgiving)
                    if (bulletOut && (bulletX - 2) < aliens[i].x + ALIEN1_WIDTH && (bulletX + 2) + BULLET_WIDTH > aliens[i].x &&
                        (bulletY - 2) < aliens[i].y + ALIEN1_HEIGHT &&
                        (bulletY + 2) + BULLET_HEIGHT > aliens[i].y) {
                        //draws over defeated alien and gets rid of bullet sprite     
                        drawSprite(blankSprite, aliens[i].x, aliens[i].y, ALIEN1_WIDTH, ALIEN1_HEIGHT);
                        drawSprite(blankSprite, bulletX, bulletY, BULLET_WIDTH, BULLET_HEIGHT);
                        aliens[i].alive = 0;
                        bulletOut = 0;
                        score++;
                        aliveAliens--;
                    }
                    //Ship collision checker and handles game over and minus lives
                    if (spaceshipX < aliens[i].x + ALIEN1_WIDTH && spaceshipX + SHIP_WIDTH > aliens[i].x 
                    && spaceshipY < aliens[i].y + ALIEN1_HEIGHT && spaceshipY + SHIP_HEIGHT > aliens[i].y) {

                    lives--;    
                    drawSprite(blankSprite, spaceshipX, spaceshipY, SHIP_WIDTH, SHIP_HEIGHT);
                    if (lives <= 0) {
                        lcd_goto_xy(0, 0);
                        lcd_write_str("GAME OVER!");
                        if (score > highScore) {
                            highScore = score;
                        }
                        char high_str[4]; 
                        itoa(highScore, high_str, 10); 
                        lcd_goto_xy(1, 0);
                        lcd_write_str("High Score: ");
                        lcd_goto_xy(1, 12);
                        lcd_write_str(high_str);
                        gameOver = 1;
                    return state;
                    } 
                    else {
                    //restart wave if player loses 1 life
                        aliveAliens = MAX_ALIENS;
                        bulletOut = 0;
                    for (int j = 0; j < MAX_ALIENS; j++) {
                        drawSprite(blankSprite, aliens[j].x, aliens[j].y, ALIEN1_WIDTH, ALIEN1_HEIGHT);
                    }
                    initAliens();
                    return state;
                    }
                }
                //if aliens reach bottom of screen, minus 1 to score per alien
                if(aliens[i].y >= SCREEN_HEIGHT - 5) {
                    if(score > 0){
                        score -= 1; 
                    }
                    drawSprite(blankSprite, aliens[i].x, aliens[i].y, ALIEN1_WIDTH, ALIEN1_HEIGHT);
                    aliens[i].y = 0;    //all alive aliens move back to top
                }
            }  
                //if wave cleared
                if (aliveAliens == 0) {
                    wave++;
                    powerUpWaveCounter++;
                    aliveAliens = MAX_ALIENS;
                if (powerUpWaveCounter >= 3) {
                    powerupReady = 1;
                    powerUpWaveCounter = 0;
                    PORTC = SetBit(PORTC, 5, 1); //blue LED
                }
                //increase alien speed after each wave
                if (alienSpeed > 1) {
                    alienSpeed = alienSpeed - 1;
                } 
                else {
                    alienSpeed = 1;
                }
                initAliens();
            }
        }
        break;
    }
    return state;
}



enum joystickButtonStates {joystickButtonSample};
int TickFct_joystickButton(int state) {
    unsigned char currJoystickButton = GetBit(PINC, 2); // joystick button on PC2
    switch(state){
        case joystickButtonSample:
        //draw bullet when joystick button is pressed 
        if (currJoystickButton == 1 && bulletOut == 0) {
            bulletOut = 1; //only 1 bullet at a time
            bulletX = spaceshipX + (SHIP_WIDTH / 2) - (BULLET_WIDTH / 2); 
            bulletY = spaceshipY - BULLET_HEIGHT; 
        }
        break;
    }
    return state;
}

enum bulletStates {moveBullet};
int TickFct_bulletMove(int state) {
    static int prevBulletY = 0;

    switch(state) {
        case moveBullet:
            if (bulletOut) {
                if (prevBulletY >= 0) {
                    drawSprite(blankSprite, bulletX, prevBulletY, BULLET_WIDTH, BULLET_HEIGHT);
                }

               //bullet moves 2 pixels per tick
               if(powerUpActive) {
                    bulletY -= 6; //double speed if power-up is active
                } else {
                    bulletY -= 2; //normal speed
                }
                //bulletY -= 2;
                drawSprite(bullet, bulletX, bulletY, BULLET_WIDTH, BULLET_HEIGHT);
                prevBulletY = bulletY;
                //if bullet hits top, reset
                if (bulletY <= 0) {
                    drawSprite(blankSprite, bulletX, bulletY, BULLET_WIDTH, BULLET_HEIGHT);
                    bulletOut = 0;
                    prevBulletY = 0;
                }
            }
            break;
    }
    return state;
}


enum joystickMoveStates {readJoystick};
int TickFct_joystickMove(int state) {
    int xAxis = ADC_read(1); //vrY in C1, acts as moving left and right
    int currentSpeed = 2;
    if(powerUpActive) {
        currentSpeed = 6;
    }
    else {
        currentSpeed = 2; 
    }
    switch(state) {
        case readJoystick:
            if (xAxis > leftThreshold) {
                spaceshipX -= currentSpeed;
            } else if (xAxis < rightThreshold) {
                spaceshipX += currentSpeed;
            }
            //bound checks
            if (spaceshipX < 0) {
                spaceshipX = 0;
            }
            if (spaceshipX > SCREEN_WIDTH - SHIP_WIDTH) {
                spaceshipX = SCREEN_WIDTH - SHIP_WIDTH;
            }
            break;
    }
    return state;
}

enum shipDrawStates {drawShip};
int TickFct_drawShip(int state) {
    static int prevX = 0;

    switch(state) {
        case drawShip:
            if (spaceshipX != prevX) {
                //only draw over old ship when moving
                drawSprite(blankSprite, prevX, spaceshipY, SHIP_WIDTH, SHIP_HEIGHT);
                //draw new location 
                drawSprite(ship, spaceshipX, spaceshipY, SHIP_WIDTH, SHIP_HEIGHT);
                // Update previous position
                prevX = spaceshipX;
            }
            break;
    }

    return state;
}


enum displayScoreStates {displayScore};
int TickFct_displayScore(int state) {
    static unsigned char prev_score = 255; //prints 0 first 
    char scoreSTR[4]; 
    
    switch(state) {
        case displayScore:
            if (score != prev_score && !gameOver) {
                itoa(score, scoreSTR, 10);     //10 is decimal base
                lcd_goto_xy(0, 8);
                lcd_write_str(" "); //clear currScore and decrement or increment if needed       
                lcd_goto_xy(0, 8);          
                lcd_write_str(scoreSTR);   
                
                prev_score = score;
            }
            break;
    }
    return state;
}

enum displayLivesStates {displayLives};
int TickFct_displayLives(int state) {
    static int prev_lives = 0; 
    char livesSTR[2]; 

    switch(state) {
        case displayLives:
            if (!gameOver && lives != prev_lives) {
                itoa(lives, livesSTR, 10); //10 is decimal base
                lcd_goto_xy(1, 8);         
                lcd_write_str(" "); //clear currLives and decrement if needed       
                lcd_goto_xy(1, 8);
                lcd_write_str(livesSTR); 

                prev_lives = lives;
            }
            break;
    }

    return state;
}



enum resetStates {checkResetButton};
int TickFct_checkResetButton(int state) {
    unsigned char resetButton = GetBit(PINC, 3);
    //serial_println(resetButton);
    switch (state) {
        case checkResetButton:
            if (gameOver && resetButton == 0) {
                resetGame();
                gamereset = 1;
            }
            break;
    }
    return state;
}

enum powerUpStates { checkPowerUp };
int TickFct_powerUp(int state) {
    //serial_println(GetBit(PINC, 4)); 
    switch(state) {
        case checkPowerUp:
        if(powerupReady && GetBit(PINC, 4) == 0) {
            powerUpActive = 1; //activate power-up
            powerUpTimer = 0; 
            powerupReady = 0; 
        }
            
        if (powerUpActive) {
            powerUpTimer++;
            PORTC = SetBit(PORTC, 5, 1); 
            if (powerUpTimer % 10 == 0) { //flash to show that power up is being used 
                PORTC = SetBit(PORTC, 5, 0); 
            }
            // End power-up after duration
            if (powerUpTimer >= powerUpDuration || lives <= 0) {
                PORTC = SetBit(PORTC, 5, 0);
                powerUpActive = 0;
                powerUpTimer = 0;
            }
        }
        break;
    }
    return state;
}


int main(void) {
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS) | (1 << PIN_DC) | (1 << PIN_RESET);
    DDRC = 0x00;
    PORTC = 0x00; 
    PORTC = (1 << PC3) | (1 << PC4); //reset button
    DDRC |= (1 << PC2); //joystick button       
    PORTC |= (0 << PC2);
    DDRC |= (1 << PC5); //blue led


    serial_init(9600);
    SPI_INIT();
    ST7735_init();
    lcd_init();
    lcd_clear();
    ADC_init();
    srand(time(NULL)); //random seed using time function 
    initAliens();
    createBlankSprite();
    //custom chars for LCD
    lcdCreateCustomChar(0, alienLCD);    
    lcd_goto_xy(0, 0);             
    lcd_write_character(0);        
    lcd_goto_xy(0, 1); 
    lcd_write_str("Score: "); 
    lcdCreateCustomChar(1, shipLCD);     
    lcd_goto_xy(1, 0);              
    lcd_write_character(1);
    lcd_write_str("lives: ");         
    

    unsigned int i = 0;
    tasks[i].period = backgroundPeriod;
    tasks[i].state = screenReset;
    tasks[i].elapsedTime = backgroundPeriod;
    tasks[i].TickFct = &TickFct_screenReset;
    i++;
    tasks[i].period = shipPeriod;
    tasks[i].state = createShip;
    tasks[i].elapsedTime = shipPeriod;
    tasks[i].TickFct = &TickFct_shipCreate;
    ++i;
    tasks[i].period = alien1Period;
    tasks[i].state = alienWave;
    tasks[i].elapsedTime = alien1Period;
    tasks[i].TickFct = &TickFct_alien;
    ++i;
    tasks[i].period = scorePeriod;
    tasks[i].state = displayScore;
    tasks[i].elapsedTime = scorePeriod;
    tasks[i].TickFct = &TickFct_displayScore;
    ++i;
    tasks[i].period = joystickButtonPeriod;
    tasks[i].state = joystickButtonSample;
    tasks[i].elapsedTime = joystickButtonPeriod;
    tasks[i].TickFct = &TickFct_joystickButton;
    ++i;
    tasks[i].period = joystickMovePeriod;
    tasks[i].state = readJoystick;
    tasks[i].elapsedTime = joystickMovePeriod;
    tasks[i].TickFct = &TickFct_joystickMove;
    ++i;
    tasks[i].period = bulletMovePeriod;
    tasks[i].state = moveBullet;
    tasks[i].elapsedTime = bulletMovePeriod;
    tasks[i].TickFct = &TickFct_bulletMove;
    ++i;
    tasks[i].period = resetButtonPeriod;
    tasks[i].state = checkResetButton;
    tasks[i].elapsedTime = resetButtonPeriod;
    tasks[i].TickFct = &TickFct_checkResetButton;
    ++i;
    tasks[i].period = drawShipPeriod;
    tasks[i].state = drawShip;
    tasks[i].elapsedTime = drawShipPeriod;
    tasks[i].TickFct = &TickFct_drawShip;
    ++i;
    tasks[i].period = displayLivesPeriod;
    tasks[i].state = displayLives;
    tasks[i].elapsedTime = displayLivesPeriod;
    tasks[i].TickFct = &TickFct_displayLives;
    ++i;
    tasks[i].period = powerUpPeriod; 
    tasks[i].state = checkPowerUp;
    tasks[i].elapsedTime = powerUpPeriod;
    tasks[i].TickFct = &TickFct_powerUp;
    i++;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}
}