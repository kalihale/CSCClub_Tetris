//
// Created by Zrp200 on 4/7/2022.
//

#include "Game.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <regex>

SDL_Renderer* renderer;
TTF_Font* font;
void Game::initScene() {
    // ／(•ㅅ•)＼ Initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
            "Tetris",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screenWidth,
            screenHeight,
            SDL_WINDOW_OPENGL);

    if(window == nullptr) {
        throw "SDL Window could not be created";
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // ／(^ㅅ^)＼ Prepare font
    if(TTF_Init() == -1) {
        std::cout << "Could not initialize SDL2 ttf, error: " << TTF_GetError() << std::endl;
    }
    else {
        std::cout << "SDL2 ttf ready to go" << std::endl;
    }

    // ／(^ㅅ^)＼ Font is changed here
    // find current directory
    std::string fontPath = std::regex_replace(
            SDL_GetBasePath(),
            std::regex("cmake-build-debug(/|\\\\)$"),
            "")
                    + "FiraMono-Medium.ttf";
    std::cout << "Font: " << fontPath << std::endl;
    font = TTF_OpenFont(fontPath.c_str(), 22);

    if(font == nullptr){
        std::cout << TTF_GetError() << std::endl;
        exit(1);
    }
}

void drawSquare(int x, int y);
void drawSquare(int x, int y, Color c);
void drawShape(const Shape&);

void Game::presentScene()
{
    SDL_RenderPresent(renderer);
}

void renderText(const char* label, int x, int y, Color color=0xFFFFFF, int scale=1) {
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, label, {color.r,color.g,color.b});

    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_FreeSurface(textSurface);

    Game::Rect rect(x, y,4 * scale,scale);
    SDL_RenderCopy(renderer, textTexture, nullptr, &rect);
}

void Game::renderPreview(int offset, Shape* shape, const char* label) {
    if(gameState == GameState::PLAY) {
        // ／(^ㅅ^)＼ Next Piece text
        renderText(label,offset, 2);
    }

    Color previewColor = 0x888888;
    // this generates the prototype preview area.
    for(int i=0; i < PREVIEW_SIZE; i++) {
        // height is half the length (rounded up), as pieces are initially situated horizontally.
        for(int j=0; j < (PREVIEW_SIZE+1)/2; j++) {
            // padding is applied to ensure the preview area doesn't touch the grid
            drawSquare( i+offset, j, previewColor);
        }
    }
    if(shape != nullptr) {
        // offset it to fit inside the preview area.
        shape->setPos(offset);
        drawShape(*shape);
    }
}

void Game::prepareScene(Color backgroundColor)
{
    if(gameState != GameState::PLAY)
        backgroundColor = backgroundColor.hex()/2;
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);
    SDL_RenderClear(renderer);
    // render grid
    for(int x=0; x < COLS; x++) for(int y=0; y < ROWS; y++) {
            drawSquare(x,y,grid[x][y]);
        }
    renderPreview(PADDING-GRID_LEFT, nxtShape, "Next Piece"); // next piece on left
    // fixme it would be cool to be able to just put GRID_RIGHT here.
    renderPreview(COLS+PADDING, heldShape, "Held Piece"); // hold piece on right
    if(curShape != nullptr) drawShape(*curShape);

}

// x and y correspond to grid tiles.
// fixme this sets the default color to black.
void drawSquare(int x, int y) {
    Game::Rect rect = {x,y};
    SDL_RenderFillRect(renderer, &rect);
    // this is startlingly inefficient.
    SDL_SetRenderDrawColor(renderer,0,0,0,0);
    SDL_RenderDrawRect(renderer,&rect);
}
void drawSquare(int x, int y, Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    drawSquare(x,y);
}

void drawShape(const Shape& shape) {
    for(int i=0; i < Shape::N_SQUARES; i++) {
        drawSquare(shape.x+shape[i].x,shape.y+shape[i].y, shape.color);
    }
}