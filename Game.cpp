//
// Created by kali on 2/28/22.
//

#include "Game.h"
#include <SDL2/SDL.h>
#include <iostream>
#include "arduino/controller.h"

void fatalError(std::string errorString)
{
    std::cout << errorString << std::endl;
    std::cout << "Enter any key to quit...";
    int tmp;
    std::cin >> tmp;
    SDL_Quit();
}

const Game* gameInstance;
arduino::Controller* pc;
Game::Game(int screenWidth, int screenHeight)
: window(nullptr)
, screenWidth(screenWidth)
, screenHeight(screenHeight)
, gameState(GameState::START)
{
    gameInstance = this;
    pc = new arduino::Controller();
}

bool Shape::isInvalidPosition(int x,int y)
{
    return x < 0
        || y < 0
        || x >= Game::COLS
        || y >= Game::ROWS
        || !(gameInstance->grid[x][y] == RGB());
}

Game::~Game()
{

}

void Game::run()
{
    initSystems();
    gameLoop();
}

void Game::initSystems()
{
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    // ／(•ㅅ•)＼ Initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
            "Tetris",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screenWidth,
            screenHeight,
            SDL_WINDOW_OPENGL);

    if(window == nullptr)
    {
        fatalError("SDL Window could not be created");
    }

    this->renderer = SDL_CreateRenderer(window, -1, rendererFlags);

}

int curPiece = 0;
Shape* cyclePiece(int inc) {
    int N = Shape::T+1;
    curPiece = (curPiece + N + inc) % N;
    return new Shape(static_cast<Shape::Piece>(curPiece));
}

void Game::setCurShape(Shape *shape) {
    // todo it might be nice to make curShape a reference rather than a pointer, but it would require either a getter method that autocalls this one or more delicate handling of the curShape variable to ensure it never holds a null pointer.
    shape->setPos(COLS/2);
    if(shape->isInvalidState()) {
        gameState = GameState::GAME_OVER;
        pc->playAnimation(arduino::Controller::Animation::FLATLINE);
        return; // terminate logic
    }
    delete curShape;
    curShape = shape;
}

bool holdUsed = false;
void Game::holdShape() {
    if(holdUsed) return;
    auto tmp = heldShape;
    heldShape = new Shape(curShape->piece);
    if(tmp == nullptr) {
        loadNewShape();
    } else {
        holdUsed = true;
        setCurShape(tmp);
    }
}

void Game::loadNewShape() {
    holdUsed = false;
    setCurShape(nxtShape);
    nxtShape = new Shape(bag.draw());
}

bool Game::moveCurShapeDown() {
    if(curShape->moveDown()) return true;
    placeShape();
    return false;
}

void Game::incLevel() {
    std::cout << "level: " << ++level << " (speed = " << DELAY/(time = dropDelay()) << "G/" << dropDelay() << "ms)" << std::endl;
    pc->playAnimation(arduino::Controller::Animation::TOWER_LIGHT);
    toNextLevel = LEVEL_CLEAR;
}

void Game::placeShape() {
    Shape& s = *curShape;
    for(int i=0; i < Shape::N_SQUARES; i++) {
        grid[s[i].x+s.x][s[i].y+s.y] = curShape->color;
    }
    for(int i = 0; i < Shape::N_SQUARES; i++)
    {
        if(rowComplete(s[i].y + s.y))
        {
            moveRows(s[i].y + s.y);
        }
    }
    loadNewShape();
}

void Game::play() {
    if(gameState == GameState::GAME_OVER) {
        for (auto &col: grid)
            for (auto &cell: col)
                cell = RGB();
    }
    gameState = GameState::PLAY;
    level=0; incLevel();
    bag = Bag(); // refresh the bag
    // gravity logic
    time = dropDelay();
    fastFall = false;
    locked = false;
    delete nxtShape;
    nxtShape = new Shape(bag.draw());
    loadNewShape();
}

struct {
    int move=0, rotate=0;
    bool instantDrop=false,fastDrop=false;
    bool holdPiece=false;
} cur, prev;

bool held = true; // press a button to start the game.
void Game::processInput()
{
    // fixme there is massive duplication here
    SDL_Event evnt;
    while(SDL_PollEvent(&evnt))
    {
        switch(evnt.type)
        {
            case SDL_WINDOWEVENT:
                switch(evnt.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        screenWidth = evnt.window.data1;
                        screenHeight = evnt.window.data2;
                        break;
                };
                break;
            case SDL_QUIT:
                gameState = GameState::EXIT;
                break;
//            case SDL_MOUSEMOTION:
//                //prepareScene(/*evnt.motion.x * 255 / screenWidth, evnt.motion.y * 255 / screenHeight, 255*/);
//                std::cout << evnt.motion.x << " " << evnt.motion.y << std::endl;
//                break;
            case SDL_KEYUP:
                held = false;
                if(gameState != GameState::PLAY) break;
                if(evnt.key.keysym.scancode == SDL_SCANCODE_S) {
                    toggleFastDrop(false);
                    //std::cout << "fast fall off" << std::endl;
                }
                break;
            case SDL_KEYDOWN:
                if(held) break; else held = true;
                if(gameState == GameState::START || gameState == GameState::GAME_OVER) {
                    if(evnt.key.keysym.scancode == 40) {
                        play();
                    }
                    return;
                }
                // interact with our piece
                // break locks, continue does not.
                switch (evnt.key.keysym.scancode) {
                    case SDL_SCANCODE_W:
                        curShape->y--;
                        break;
                    case SDL_SCANCODE_S:
                        toggleFastDrop(true);
                        //std::cout << "fast fall on" << std::endl;
                        continue; // already reset timer.
                    case SDL_SCANCODE_A:
                        curShape->moveL();
                        break;
                    case SDL_SCANCODE_D:
                        curShape->moveR();
                        break;
                    case SDL_SCANCODE_Q:
                        curShape->rotateL();
                        break;
                    case SDL_SCANCODE_E:
                        curShape->rotateR();
                        break;
                    case SDL_SCANCODE_TAB:
                        holdShape();
                        break;
                    case SDL_SCANCODE_EQUALS:
                        incLevel();
                        break;
                    case SDL_SCANCODE_MINUS:
                        std::cout << "level: " << --level << " --- speed=" << (time = dropDelay()) << std::endl;
                        break;
                        // worth noting this cycles draw, so if you want it to start cycling you need to do it twice.
                    case SDL_SCANCODE_RIGHT:
                        loadNewShape();
                        nxtShape = cyclePiece(+1);
                        break;
                    case SDL_SCANCODE_LEFT:
                        loadNewShape();
                        nxtShape = cyclePiece(-1);
                        break;
                    case 40:
                        // move it down all the way
                        while(moveCurShapeDown());
                        break;
                    case SDL_SCANCODE_DELETE:
                        loadNewShape();
                        continue;
                    default:
                        continue;
                }
                lockPiece();
                return; // don't check arduino
                //std::cout << evnt.key.keysym.scancode << std::endl;
        }
    }
    // ARDUINO CONTROLS
    if(gameState == GameState::START || gameState == GameState::GAME_OVER) {
        if(pc->startButton) play(); else return;
    }
    prev = cur;
    cur = {
            pc->moveLeft - (int)pc->moveRight,
            pc->rotateLeft - (int)pc->rotateRight,
            pc->instantDrop,
            pc->fastDrop,
            pc->selectButton,
    };
    if(cur.holdPiece && !prev.holdPiece) holdShape();
    bool lock = false;
    if(cur.move != prev.move) lock = curShape->move(cur.move);
    if(cur.rotate != prev.rotate) lock = curShape-> rotate(cur.rotate) || lock;
    if(cur.fastDrop != prev.fastDrop) {
        lock = false;
        toggleFastDrop(cur.fastDrop);
    }
    if(cur.instantDrop && !prev.instantDrop) {
        lock = false;
        instantDrop();
    }
    if(lock) lockPiece();
}

// ／(^ㅅ^)＼ Checks if a given row y is complete
bool Game::rowComplete(int y) const {
    // ／(^ㅅ^)＼ Loop through columns in the row
    for(int i = 0; i < COLS; i++)
    {
        // ／(^ㅅ^)＼ If a square in the row is equal to the default, the row is incomplete
        if(grid[i][y] == RGB())
        {
            return false;
        }
    }
    // ／(^ㅅ^)＼ If no squares in the row are equal to default, the row is complete
    return true;
}

// ／(^ㅅ^)＼ Move all rows above y down a single row
bool Game::moveRows(int y)
{
    // ／(^ㅅ^)＼ Loop from y to 1
    for(int i = y; i > 0; i--)
    {
        // ／(^ㅅ^)＼ Move the row above to the current row
        for(int j = 0; j < COLS; j++)
        {
            grid[j][i] = grid[j][i - 1];
        }
    }
    // ／(^ㅅ^)＼ Clear row 0
    return clearRow(0);
}

// ／(^ㅅ^)＼ Clears a given row y
bool Game::clearRow(int y)
{
    // ／(^ㅅ^)＼ Loops through all columns in the row and sets each square to default
    for(int i = 0; i < COLS; i++)
    {
        grid[i][y] = RGB();
    }
    // ／(^ㅅ^)＼ Increment level if required
    if(--toNextLevel == 0) incLevel();
    return true;
}

void Game::gameLoop()
{
    while (true)
    {
        prepareScene();
        presentScene();
        SDL_Delay(DELAY);
        pc->refreshArduinoStatus();
        processInput();
        if(gameState == GameState::EXIT) break;
        if(gameState == GameState::PLAY) applyGravity();
    }
}
void Game::applyGravity()
{
    time -= DELAY;
    while(time <= 0) {
        if(curShape->moveDown()) {
            time += dropDelay();
            locked = false;
        } else {
            // shape cannot move down anymore.
            fastFall = false;
            if(locked) {
                // lock delay has been spent, place the shape.
                locked = false;
                placeShape();
                // return to standard drop delay
                time = dropDelay();
            }
            else
            {
                // this prevents the shape from being placed instantly, allowing the player to quickly move it before it places.
                lockPiece();
            }
        }
    }
}