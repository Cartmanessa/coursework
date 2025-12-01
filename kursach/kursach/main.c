#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <GL/freeglut.h>

// размеры поля и экрана
#define PLAY_WIDTH 10
#define PLAY_HEIGHT 20
#define BLOCK_SIZE 30
#define SIDEBAR_WIDTH 150
#define WINDOW_WIDTH 450
#define WINDOW_HEIGHT 600

static const int BTN_WIDTH = 120, BTN_HEIGHT = 40;
static const int MENU_BTN_WIDTH = 240;
static const int MENU_BTN_HEIGHT = 80;
static const int MENU_BTN_RASSTONIE = 60;

int btnX, btnY;

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER
};
enum GameState gameState = MENU;

static float getStrokeTextWidth(const char* text, void* font);
void loadHighScore(void);
void saveHighScore(void);
void restartGame(void);
int checkstolknovenie(int g[PLAY_HEIGHT][PLAY_WIDTH], int s, int r, int x, int y);
void fixPiece(void);
void clearLines(void);
void spawnPiece(void);
void drawCellTransparent(int row, int col, int c, float alpha);
void display(void);
void timer(int v);
void specialKeys(int key, int x, int y);
void mouseClick(int button, int state, int x, int y);

// --------------------------
// Прототипы новых функций бота
// --------------------------
int simulate_place_and_score(int g[PLAY_HEIGHT][PLAY_WIDTH], int piece, int rot, int x, int *out_lines_cleared);
int drop_height_for(int g[PLAY_HEIGHT][PLAY_WIDTH], int piece, int rot, int x);
void bot_place_best_move(void);

// --------------------------
// Флаг: бот играет (включается при Play/Retry из меню)
// --------------------------
bool bot_enabled = false;

// --------------------------
// BENCHMARK GLOBALS (ADDED)
// lastBotTimeMs - время последнего решения (ms)
// totalBotTimeMs - суммарное время всех решений (ms)
// botMoveCount - количество решений, сделанных ботом
// --------------------------
double lastBotTimeMs = 0.0;
double totalBotTimeMs = 0.0;
int botMoveCount = 0;

// то как выглядят фигуры в разных положениях (0, 90, 180, 270)
static const int shapes[7][4][4][4] = {
    // палка
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    // J фигура
    {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
    // L фигура
    {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // квадрат
    {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
    // S фигура
    {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // T фигура
    {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // Z фигура
    {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}
};

// цвета каждой из фигур
static const float colors[7][3] = {
    {0.0f,1.0f,1.0f},{0.0f,0.0f,1.0f},{1.0f,0.5f,0.0f},
    {1.0f,1.0f,0.0f},{0.0f,1.0f,0.0f},{0.5f,0.0f,0.5f},
    {1.0f,0.0f,0.0f}
};

int grid[PLAY_HEIGHT][PLAY_WIDTH];
int currentPiece; // текущая фигура
int nextPiece; // следующая фигура
int currentRot; // текущий поворот
int currentX, currentY;
int score, Record, Lines;
bool gameOver;
float alphaValue;
time_t startTime, lastSpeedUpdate;
int Speed; // (миллисекунды)

// ширина текста (центрировать)
static float getStrokeTextWidth(const char* text, void* font) {
    float w = 0;
    while (*text) {
        w += glutStrokeWidth(font, *text++);
    }
    return w;
}

void loadHighScore() {
    FILE* f = fopen("Record.txt", "r");
    if (f) {
        fscanf(f, "%d", &Record);
        fclose(f);
    } else {
        Record = 0;
    }
}

void saveHighScore() {
    FILE* f = fopen("Record.txt", "w");
    if (f) {
        fprintf(f, "%d", Record);
        fclose(f);
    }
}

// создание фигуры
void spawnPiece() {
    currentPiece = nextPiece;
    currentRot = 0;
    currentX = PLAY_WIDTH / 2 - 2;
    currentY = -2;
    nextPiece = rand() % 7;
    if (checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY)) {
        gameOver = true;
        gameState = GAME_OVER;
    }
}

void restartGame() {
    memset(grid, 0, sizeof(grid));
    score = Lines = 0;
    gameOver = false;
    alphaValue = 1.0f;
    startTime = lastSpeedUpdate = time(NULL);
    Speed = 1000;
    nextPiece = rand() % 7; // инициализация следующей фигуры

    // BENCHMARK RESET
    lastBotTimeMs = 0.0;
    totalBotTimeMs = 0.0;
    botMoveCount = 0;

    spawnPiece();
    glutPostRedisplay();
    glutTimerFunc(Speed, timer, 0);
}

int checkstolknovenie(int g[PLAY_HEIGHT][PLAY_WIDTH], int s, int r, int x, int y) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[s][r][i][j]) {
                int gx = x + j;
                int gy = y + i;
                if (gx < 0 || gx >= PLAY_WIDTH || gy >= PLAY_HEIGHT) {
                    return 1;
                }
                if (gy >= 0 && g[gy][gx]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void fixPiece() {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) if (shapes[currentPiece][currentRot][i][j]) {
        int gy = currentY + i;
        if (gy >= 0 && gy < PLAY_HEIGHT)
            grid[gy][currentX + j] = currentPiece + 1;
    }
    for (int j = 0; j < PLAY_WIDTH; j++) {
        if (grid[0][j]) {
            gameOver = true;
            gameState = GAME_OVER;
            break;
        }
    }
}

void clearLines() {
    for (int i = PLAY_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < PLAY_WIDTH; j++) if (!grid[i][j]) { full = false; break; }
        if (full) {
            for (int k = i; k > 0; k--) memcpy(grid[k], grid[k - 1], sizeof(grid[k]));
            memset(grid[0], 0, sizeof(grid[0]));
            score += 100;
            Lines++;
            i++;
        }
    }
}

void drawCellTransparent(int row, int col, int c, float alpha) {
    if (c < 1) return;
    const float* colr = colors[c - 1];
    glColor4f(colr[0], colr[1], colr[2], alpha);
    int x = col * BLOCK_SIZE + SIDEBAR_WIDTH;
    int y = WINDOW_HEIGHT - (row + 1) * BLOCK_SIZE;
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + BLOCK_SIZE, y);
    glVertex2i(x + BLOCK_SIZE, y + BLOCK_SIZE);
    glVertex2i(x, y + BLOCK_SIZE);
    glEnd();
    glColor4f(0, 0, 0, alpha);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + BLOCK_SIZE, y);
    glVertex2i(x + BLOCK_SIZE, y + BLOCK_SIZE);
    glVertex2i(x, y + BLOCK_SIZE);
    glEnd();
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == MENU) {
        glColor3f(0.0f, 0.0f, 0.0f);
        glRecti(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        const char* title = "TETRIS";
        float titleScale = 0.8f;
        float titleWidth = getStrokeTextWidth(title, GLUT_STROKE_ROMAN) * titleScale;
        glLineWidth(12);
        glPushMatrix();
        glTranslatef((WINDOW_WIDTH - titleWidth) / 2, WINDOW_HEIGHT / 2 + 150, 0);
        glScalef(titleScale, titleScale, 1);
        for (int i = 0; title[i]; i++) {
            switch (title[i]) {
            case 'T': glColor3f(1.0f, 0.0f, 0.0f); break;
            case 'E': glColor3f(1.0f, 0.5f, 0.0f); break;
            case 'R': glColor3f(0.0f, 1.0f, 0.0f); break;
            case 'I': glColor3f(0.0f, 0.0f, 1.0f); break;
            case 'S': glColor3f(0.5f, 0.0f, 0.5f); break;
            default: glColor3f(1.0f, 1.0f, 0.0f); break;
            }
            glutStrokeCharacter(GLUT_STROKE_ROMAN, title[i]);
        }
        glPopMatrix();
        glLineWidth(1);

        int playX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int playY = WINDOW_HEIGHT / 2 + MENU_BTN_RASSTONIE / 2;
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2i(playX, playY);
        glVertex2i(playX + MENU_BTN_WIDTH, playY);
        glVertex2i(playX + MENU_BTN_WIDTH, playY + MENU_BTN_HEIGHT);
        glVertex2i(playX, playY + MENU_BTN_HEIGHT);
        glEnd();
        const char* playText = "Play";
        float textScale = 0.36f;
        float playTextWidth = getStrokeTextWidth(playText, GLUT_STROKE_ROMAN) * textScale;
        glPushMatrix();
        glLineWidth(7.0f);
        glTranslatef(playX + (MENU_BTN_WIDTH - playTextWidth) / 2, playY + MENU_BTN_HEIGHT / 2 - 10 - 5, 0);
        glScalef(textScale, textScale, 1);
        glColor3f(1, 1, 1);
        for (const char* c = playText; *c; ++c) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
        glPopMatrix();
        glLineWidth(1.0f);

        int exitX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int exitY = WINDOW_HEIGHT / 2 - MENU_BTN_HEIGHT - MENU_BTN_RASSTONIE / 2;
        glBegin(GL_QUADS);
        glColor3f(0.6f, 0.2f, 0.8f);
        glVertex2i(exitX, exitY);
        glVertex2i(exitX + MENU_BTN_WIDTH, exitY);
        glColor3f(0.4f, 0.1f, 0.5f);
        glVertex2i(exitX + MENU_BTN_WIDTH, exitY + MENU_BTN_HEIGHT);
        glVertex2i(exitX, exitY + MENU_BTN_HEIGHT);
        glEnd();
        const char* exitText = "Exit";
        float exitTextWidth = getStrokeTextWidth(exitText, GLUT_STROKE_ROMAN) * textScale;
        glPushMatrix();
        glLineWidth(6.0f);
        glTranslatef(exitX + (MENU_BTN_WIDTH - exitTextWidth) / 2, exitY + MENU_BTN_HEIGHT / 2 - 10 - 5, 0);
        glScalef(textScale, textScale, 1);
        glColor3f(1, 1, 1);
        for (const char* c = exitText; *c; ++c) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
        glPopMatrix();
        glLineWidth(1.0f);
    }
    else if (gameState == PLAYING) {
        glColor3f(0.2f, 0.2f, 0.2f);
        glRecti(0, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT);
        // Рамка под следующую фигуру
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(20, WINDOW_HEIGHT - 130);
        glVertex2i(120, WINDOW_HEIGHT - 130);
        glVertex2i(120, WINDOW_HEIGHT - 30);
        glVertex2i(20, WINDOW_HEIGHT - 30);
        glEnd();
        glLineWidth(1.0f);

        // Отрисовка следующей фигуры
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (shapes[nextPiece][0][i][j]) {
                    int x = 35 + j * 19;
                    int y = WINDOW_HEIGHT - 80 - i * 20;
                    const float* colr = colors[nextPiece];
                    glColor4f(colr[0], colr[1], colr[2], 1.0f);
                    glBegin(GL_QUADS);
                    glVertex2i(x, y);
                    glVertex2i(x + 20, y);
                    glVertex2i(x + 20, y + 20);
                    glVertex2i(x, y + 20);
                    glEnd();
                    glColor4f(0, 0, 0, 1.0f);
                    glBegin(GL_LINE_LOOP);
                    glVertex2i(x, y);
                    glVertex2i(x + 20, y);
                    glVertex2i(x + 20, y + 20);
                    glVertex2i(x, y + 20);
                    glEnd();
                }
            }
        }

        time_t now = time(NULL);
        int t = (int)(now - startTime), m = t / 60, s = t % 60;

        char buf[64];
        glColor3f(1, 1, 1);

        // We'll use a small HUD column (like in the example)
        int startY = WINDOW_HEIGHT - 160;
        int stepY = 25;

        // время сначала игры
        sprintf(buf, "Time: %02d:%02d", m, s);
        glRasterPos2i(10, startY);
        for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

        // результат
        sprintf(buf, "Score: %d", score);
        glRasterPos2i(10, startY - stepY);
        for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

        // кол-во линий
        sprintf(buf, "Lines: %d", Lines);
        glRasterPos2i(10, startY - 2 * stepY);
        for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

        // Record
        sprintf(buf, "Record: %d", Record);
        glRasterPos2i(10, startY - 3 * stepY);
        for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

        // --------- BENCHMARK HUD (ADDED) -------------
        if (bot_enabled) {
            // Workbench - последний ход
            sprintf(buf, "Workbench: %.4f ms", lastBotTimeMs);
            glRasterPos2i(10, startY - 4 * stepY);
            for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

            // среднее
            double avg = botMoveCount > 0 ? totalBotTimeMs / botMoveCount : 0.0;
            sprintf(buf, "Average: %.4f ms", avg);
            glRasterPos2i(10, startY - 5 * stepY);
            for (char* c = buf; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        // ----------------------------------------------

        for (int i = 0; i < PLAY_HEIGHT; i++)
            for (int j = 0; j < PLAY_WIDTH; j++)
                drawCellTransparent(i, j, grid[i][j], 1.0f);

        if (!gameOver) {
            for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
                if (shapes[currentPiece][currentRot][i][j]) {
                    int row = currentY + i, col = currentX + j;
                    if (row >= 0) drawCellTransparent(row, col, currentPiece + 1, alphaValue);
                }
        }
    }
    else if (gameState == GAME_OVER) {
        if (score > Record) {
            Record = score;
            saveHighScore();
        }

        glColor4f(0, 0, 0, 0.7f);
        glRecti(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        const char* title = "GAME OVER";
        float tS = 0.5f, wT = getStrokeTextWidth(title, GLUT_STROKE_ROMAN) * tS;
        glColor3f(1, 0, 0);
        glLineWidth(6);
        glPushMatrix();
        glTranslatef((WINDOW_WIDTH - wT) / 2, WINDOW_HEIGHT / 2 + 120, 0);
        glScalef(tS, tS, 1);
        for (const char* c = title; *c; ++c)
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();
        glLineWidth(1);

        char sBuf2[32];
        sprintf(sBuf2, "Score: %d", score);
        float sS = 0.3f, wS = getStrokeTextWidth(sBuf2, GLUT_STROKE_ROMAN) * sS;
        glColor3f(1, 1, 1);
        glLineWidth(4);
        glPushMatrix();
        glTranslatef((WINDOW_WIDTH - wS) / 2, WINDOW_HEIGHT / 2 + 60, 0);
        glScalef(sS, sS, 1);
        for (const char* c = sBuf2; *c; ++c)
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        glPopMatrix();
        glLineWidth(1);

        // --------- BENCHMARK ON GAME OVER (ADDED) -------------
        if (bot_enabled && botMoveCount > 0) {
            double avg = totalBotTimeMs / botMoveCount;
            char avgBuf[64];
            sprintf(avgBuf, "Average: %.4f ms", avg);
            float aS = 0.25f;
            float aW = getStrokeTextWidth(avgBuf, GLUT_STROKE_ROMAN) * aS;
            glColor3f(1, 1, 1);
            glLineWidth(3);
            glPushMatrix();
            glTranslatef((WINDOW_WIDTH - aW) / 2, WINDOW_HEIGHT / 2 + 20, 0);
            glScalef(aS, aS, 1);
            for (const char* c = avgBuf; *c; ++c)
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();

            // последний Workbench (последнее время)
            char lastBuf[64];
            sprintf(lastBuf, "Last decision: %.4f ms", lastBotTimeMs);
            float lS = 0.22f;
            float lW = getStrokeTextWidth(lastBuf, GLUT_STROKE_ROMAN) * lS;
            glColor3f(1, 1, 1);
            glLineWidth(2);
            glPushMatrix();
            glTranslatef((WINDOW_WIDTH - lW) / 2, WINDOW_HEIGHT / 2 - 10, 0);
            glScalef(lS, lS, 1);
            for (const char* c = lastBuf; *c; ++c)
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
            glPopMatrix();
        }
        // ------------------------------------------------------

        btnX = (WINDOW_WIDTH - BTN_WIDTH) / 2;
        btnY = WINDOW_HEIGHT / 2 - BTN_HEIGHT - 20;
        glBegin(GL_QUADS);
        glColor3f(0.2f, 0.6f, 1.0f); glVertex2i(btnX, btnY);
        glVertex2i(btnX + BTN_WIDTH, btnY);
        glColor3f(0.1f, 0.3f, 0.8f); glVertex2i(btnX + BTN_WIDTH, btnY + BTN_HEIGHT);
        glVertex2i(btnX, btnY + BTN_HEIGHT);
        glEnd();
        const char* rt2 = "Retry";
        int rtW2 = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)rt2), rtH2 = 18;
        glColor3f(1, 1, 1);
        glRasterPos2i(btnX + (BTN_WIDTH - rtW2) / 2, btnY + (BTN_HEIGHT - rtH2) / 2 + rtH2 - 15);
        for (const char* c = rt2; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        int mmX = btnX, mmY = btnY - BTN_HEIGHT - 10;
        glBegin(GL_QUADS);
        glColor3f(0.6f, 0.2f, 0.8f); glVertex2i(mmX, mmY);
        glVertex2i(mmX + BTN_WIDTH, mmY);
        glColor3f(0.4f, 0.1f, 0.5f); glVertex2i(mmX + BTN_WIDTH, mmY + BTN_HEIGHT);
        glVertex2i(mmX, mmY + BTN_HEIGHT);
        glEnd();
        const char* mm2 = "Main Menu";
        int mmW2 = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)mm2), mmH2 = 18;
        glColor3f(1, 1, 1);
        glRasterPos2i(mmX + (BTN_WIDTH - mmW2) / 2, mmY + (BTN_HEIGHT - mmH2) / 2 + mmH2 - 15);
        for (const char* c = mm2; *c; ++c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glutSwapBuffers();
}

int drop_height_for(int grid[PLAY_HEIGHT][PLAY_WIDTH], int piece, int rot, int x) {
    int y = -5;
    while (!checkstolknovenie(grid, piece, rot, x, y + 1)) {
        y++;
        if (y > PLAY_HEIGHT) {
            break;
        }
    }
    return y;
}

// Симулируем размещение фигуры и считаем эвристику.
// Возвращаем числовую оценку (чем выше — лучше). out_lines_cleared заполняется количеством собранных линий.
int simulate_place_and_score(int g_in[PLAY_HEIGHT][PLAY_WIDTH], int piece, int rot, int x, int *out_lines_cleared) {
    int g[PLAY_HEIGHT][PLAY_WIDTH];
    memcpy(g, g_in, sizeof(g)); // работаем на копии сетки

    // Находим y куда упадёт фигура
    int y = drop_height_for(g, piece, rot, x);

    // Если при y фигура выходит выше верхней границы — недопустимый ход
    if (y < -1) {
        if (out_lines_cleared) *out_lines_cleared = 0;
        return -1000000; // очень плохой ход
    }

    // Наложим фигуру на копию сетки
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shapes[piece][rot][i][j]) {
                int gx = x + j;
                int gy = y + i;
                if (gy >= 0 && gy < PLAY_HEIGHT && gx >= 0 && gx < PLAY_WIDTH) {
                    g[gy][gx] = piece + 1;
                }
            }
        }
    }

    // Считаем, сколько линий собирается при этом
    int lines_cleared = 0;
    for (int row = 0; row < PLAY_HEIGHT; ++row) {
        bool full = true;
        for (int col = 0; col < PLAY_WIDTH; ++col) if (!g[row][col]) { full = false; break; }
        if (full) lines_cleared++;
    }

    // Считаем высоты колонок, дыры и bumpiness
    int heights[PLAY_WIDTH];
    for (int col = 0; col < PLAY_WIDTH; ++col) {
        int h = 0;
        for (int row = 0; row < PLAY_HEIGHT; ++row) {
            if (g[row][col]) { h = PLAY_HEIGHT - row; break; }
        }
        heights[col] = h;
    }

    int aggregate_height = 0;
    for (int c = 0; c < PLAY_WIDTH; ++c) aggregate_height += heights[c];

    // Считаем дырки: пустые клетки ниже первой заполненной в колонке
    int holes = 0;
    for (int col = 0; col < PLAY_WIDTH; ++col) {
        bool block_found = false;
        for (int row = 0; row < PLAY_HEIGHT; ++row) {
            if (g[row][col]) block_found = true;
            else if (block_found) holes++;
        }
    }

    // bumpiness = сумма разниц по соседним столбцам
    int bumpiness = 0;
    for (int c = 0; c < PLAY_WIDTH - 1; ++c) bumpiness += abs(heights[c] - heights[c + 1]);

    // Эвристика (веса можно менять при необходимости)
    int score_calc = 0;
    score_calc += lines_cleared * 1000;      // сильный приоритет за очистку линий
    score_calc -= aggregate_height * 10;     // чем меньше суммарная высота — лучше
    score_calc -= holes * 150;               // дыры — очень плохо
    score_calc -= bumpiness * 10;            // неровность тоже плоха

    if (out_lines_cleared) *out_lines_cleared = lines_cleared;
    return score_calc;
}

// Основная функция бота: перебирает все возможные X/rot и выбирает лучший по эвристике, затем ставит фигуру
void bot_place_best_move(void) {
    int bestScore = -1000000000;
    int bestX = currentX;
    int bestRot = currentRot;
    int bestY = currentY;

    // Перебор всех вращений
    for (int rot = 0; rot < 4; ++rot) {
        // Перебираем разумный диапазон X (от -4 до PLAY_WIDTH), чтобы покрыть все возможные смещения
        for (int x = -4; x < PLAY_WIDTH; ++x) {
            int lines_cleared = 0;
            int val = simulate_place_and_score(grid, currentPiece, rot, x, &lines_cleared);
            if (val > bestScore) {
                int y = drop_height_for(grid, currentPiece, rot, x);
                // Если y допустимо — сохраняем
                if (y >= -1) {
                    bestScore = val;
                    bestX = x;
                    bestRot = rot;
                    bestY = y;
                }
            }
        }
    }

    // Если не нашли валидного хода — это означает game over (оставляем текущее поведение)
    if (bestScore <= -100000000) {
        return;
    }

    // Применяем найденный ход: меняем поворот/позицию и фиксируем фигуру
    currentRot = bestRot;
    currentX = bestX;
    currentY = bestY;

    // Устанавливаем фигуру на поле (аналогично логике при "упоре")
    fixPiece();

    // Очищаем линии и увеличиваем счёт
    clearLines();

    // Инициализируем следующую фигуру и создаём новую
    nextPiece = rand() % 7;
    spawnPiece();

    // Проверим — если новая фигура столкнулась — game over
    if (checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY)) {
        gameOver = true;
        gameState = GAME_OVER;
        glutPostRedisplay();
    }
}

void timer(int v) {
    if (gameState == PLAYING && !gameOver) {
        time_t now = time(NULL);
        if (now - lastSpeedUpdate >= 30) {
            Speed = Speed > 200 ? Speed - 100 : 200;
            lastSpeedUpdate = now;
        }

        if (bot_enabled) {
            // BENCHMARK: измеряем время выполнения принятия решения ботом
            clock_t start = clock();
            bot_place_best_move();
            clock_t end = clock();
            // lastBotTimeMs — время последнего решения
            lastBotTimeMs = ((double)(end - start)) * 1000.0 / CLOCKS_PER_SEC;
            // накопление и счётчик
            totalBotTimeMs += lastBotTimeMs;
            botMoveCount++;

            // Обновляем дисплей и планируем следующий таймер
            glutPostRedisplay();
            glutTimerFunc(Speed, timer, 0);
            return;
        }
        // -------------------------------------------------------------------------------

        // Оригинальное поведение — падение фигуры по таймеру (осталось для случаев ручного управления)
        if (!checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY + 1)) {
            currentY++;
        }
        else {
            fixPiece();
            clearLines();
            nextPiece = rand() % 7; // инициализация следующей фигуры
            spawnPiece();
            if (checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY)) {
                gameOver = true;
                gameState = GAME_OVER;
                glutPostRedisplay();
            }
        }
        glutPostRedisplay();
        glutTimerFunc(Speed, timer, 0);
    }
}

void specialKeys(int key, int x, int y) {
    if (gameState != PLAYING || gameOver) return;
    switch (key) {
    case GLUT_KEY_LEFT:
        if (!checkstolknovenie(grid, currentPiece, currentRot, currentX - 1, currentY))
            currentX--;
        break;
    case GLUT_KEY_RIGHT:
        if (!checkstolknovenie(grid, currentPiece, currentRot, currentX + 1, currentY))
            currentX++;
        break;
    case GLUT_KEY_DOWN:
        if (!checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY + 1))
            currentY++;
        break;
    case GLUT_KEY_UP: {
        int nr = (currentRot + 1) % 4;
        if (!checkstolknovenie(grid, currentPiece, nr, currentX, currentY))
            currentRot = nr;
    }break;
    case GLUT_KEY_HOME:
        while (!checkstolknovenie(grid, currentPiece, currentRot, currentX, currentY + 1))
            currentY++;
        break;
    }
    glutPostRedisplay();
}

void mouseClick(int button, int state, int x, int y) {
    if (gameState == MENU && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int my = WINDOW_HEIGHT - y;
        int playX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int playY = WINDOW_HEIGHT / 2 + MENU_BTN_RASSTONIE / 2;
        if (x >= playX && x <= playX + MENU_BTN_WIDTH && my >= playY && my <= playY + MENU_BTN_HEIGHT) {
            gameState = PLAYING;
            restartGame();
            bot_enabled = true;
        }
        int exitX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int exitY = WINDOW_HEIGHT / 2 - MENU_BTN_HEIGHT - MENU_BTN_RASSTONIE / 2;
        if (x >= exitX && x <= exitX + MENU_BTN_WIDTH && my >= exitY && my <= exitY + MENU_BTN_HEIGHT) {
            exit(0);
        }
    }
    else if (gameState == GAME_OVER && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int my = WINDOW_HEIGHT - y;
        if (x >= btnX && x <= btnX + BTN_WIDTH && my >= btnY && my <= btnY + BTN_HEIGHT) {
            gameState = PLAYING;
            gameOver = false;
            restartGame();
            bot_enabled = true;
        }
        int mmY = btnY - BTN_HEIGHT - 10;
        if (x >= btnX && x <= btnX + BTN_WIDTH && my >= mmY && my <= mmY + BTN_HEIGHT) {
            gameState = MENU;
            gameOver = false;
            bot_enabled = false;
            glutPostRedisplay();
        }
    }
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    loadHighScore();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Tetris");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);

    glutDisplayFunc(display);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseClick);
    restartGame();
    glutMainLoop();
    return 0;
}


