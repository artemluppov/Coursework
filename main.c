#define _CRT_SECURE_NO_WARNINGS
#include <GL/freeglut.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#define WIDTH 800
#define HEIGHT 600
#define CELL_SIZE 10
#define BUTTON_HEIGHT 50
#define NUM_BUTTONS 9
#define width (WIDTH / CELL_SIZE)
#define height ((HEIGHT - BUTTON_HEIGHT) / CELL_SIZE)


bool* grid = NULL;
bool* nextGrid = NULL;
bool simulating = false;
bool drawing = false;
int speed = 10;
int brush = 1;
int generation_count = 0;
int cycle_count = 0;

// Button definitions
typedef struct {
    float x, y, widthb, heightb;
    char label[10];
    int id;
} Button;

Button buttons[NUM_BUTTONS];

void allocateGrids();
void freeGrids();
void initButtons();
void drawButtons();
void initGrid();
void drawGrid();
void cycleCounter();
void updateGrid();
int check_valid_index_x(int x);
int check_valid_index_y(int y);
void applyBrush(int cellX, int cellY);
bool checkButtonPress(int x, int y);
void mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void timer(int value);
void reshape(int w, int h);

int main(int argc, char** argv) {
    glutInit(&argc, argv); //инициализация библиотеки glut
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // установка режимов дисплея
    glutInitWindowSize(WIDTH, HEIGHT); // размер окна
    glutCreateWindow("Game of Life with GUI"); // создание окна с именем

    glClearColor(1.0, 1.0, 1.0, 1.0); //установка цвета заднего плана
    allocateGrids();
    initGrid();
    initButtons();

    glutDisplayFunc(drawGrid);
    glutMouseFunc(mouse); // позиция мыши на экране без рисования
    glutMotionFunc(mouseMotion); // позиция мыши на экране с рисования
    glutKeyboardFunc(keyboard); // нажатия кнопок на клавиатуре
    glutReshapeFunc(reshape); // сохранение окна
    glutTimerFunc(speed, timer, 0); // для постоянной анимации

    glutMainLoop(); // Запускаем цикл обработки сообщений GLUT
    freeGrids();
    return 0;
}

void allocateGrids() {
    grid = (bool*)malloc(height * width * sizeof(bool*));
    nextGrid = (bool*)malloc(height * width * sizeof(bool*));
    
}

void freeGrids() {
    free(grid);
    free(nextGrid);
}

void initButtons() { //инициализация кнопок
    const char* brushNames[NUM_BUTTONS] = {
            "   Line", "Board1", "Board2", "Board3", "Board4",
            "Loop1", "Loop2", "Loop3", "Erase"
    };

    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].x = i * (WIDTH / NUM_BUTTONS); // х начала
        buttons[i].y = HEIGHT - BUTTON_HEIGHT; // у начала
        buttons[i].widthb = WIDTH / NUM_BUTTONS; // ширина
        buttons[i].heightb = BUTTON_HEIGHT; // высота
        sprintf(buttons[i].label, "%s", brushNames[i]); // название кнопок
        buttons[i].id = i + 1;
    }
}

void drawButtons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        glColor3f(0.0, 0.5, 0.5); //выбор цвета заднего плана кнопок
        glBegin(GL_QUADS); // постановка кисти, рисование кнопки
        glVertex2f(buttons[i].x, buttons[i].y);
        glVertex2f(buttons[i].x + buttons[i].widthb, buttons[i].y);
        glVertex2f(buttons[i].x + buttons[i].widthb, buttons[i].y + buttons[i].heightb);
        glVertex2f(buttons[i].x, buttons[i].y + buttons[i].heightb);
        glEnd();

        glColor3f(0.0, 0.0, 0.0); // цвет текста
        glRasterPos2f(buttons[i].x + 10, buttons[i].y + 30); // позиция текста
        for (char* c = buttons[i].label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // рисование символа
        }
    }
}

void initGrid() { // заполняем нулями все окно
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            grid[i*height + j] = false;
            nextGrid[i*height + j] = false;
        }
    }
    generation_count = 0; cycle_count = 0;
}

void drawGrid() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.7, 0.7, 0.7); // цвет сетки
    glBegin(GL_LINES);
    for (int i = 0; i <= WIDTH; i += CELL_SIZE) { // рисование сетки
        glVertex2i(i, 0);
        glVertex2i(i, HEIGHT - BUTTON_HEIGHT);
    }
    for (int j = 0; j <= HEIGHT - BUTTON_HEIGHT; j += CELL_SIZE) {
        glVertex2i(0, j);
        glVertex2i(WIDTH, j);
    }
    glEnd();

    glColor3f(0.0, 0.0, 0.0);
    glPointSize(CELL_SIZE);
    glBegin(GL_POINTS);
    for (int i = 0; i < width; ++i) { // рисовка поставленных клеток
        for (int j = 0; j < height; ++j) {
            if (grid[i * height + j]) {
                glVertex2i(i * CELL_SIZE + CELL_SIZE / 2, j * CELL_SIZE + CELL_SIZE / 2);
            }
        }
    }
    glEnd();

    glColor3f(0.0, 0.0, 0.0); // рисовка надписи с генерациями
    glRasterPos2f(10, HEIGHT - BUTTON_HEIGHT - 20);
    char gen_count_str[50];
    sprintf(gen_count_str, "Generations: %d", generation_count);
    for (char* c = gen_count_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glRasterPos2f(200, HEIGHT - BUTTON_HEIGHT - 20);
    char cycle_label_str[50];
    sprintf(cycle_label_str, "Cycles: %d", cycle_count);
    for (char* c = cycle_label_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    drawButtons();
    glutSwapBuffers();
}


void cycleCounter() {
    cycle_count = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            if (grid[i*height + j]) {
                //printf("%d %d\n", i, j);
                if (grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + j]) cycle_count += 1;

                else if (!grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[i * height + (j - 2 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[i * height + j] && grid[i * height + (j + 1 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[i * height + (j + 3 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 2 + width) % width) * height + j] && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 3 + width) % width) * height + j] && grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 3 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 4 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 4 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 4 + width) % width) * height + j] && !grid[((i - 4 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 4 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 4 + width) % width) * height + (j + 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[i * height + (j - 2 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[i * height + j] && grid[i * height + (j + 1 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[i * height + (j + 3 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 3 + width) % width) * height + j] && !grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 3 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 4 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 4 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 4 + width) % width) * height + j] && !grid[((i - 4 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 4 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 4 + width) % width) * height + (j + 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j]\
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j]\
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height]\
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + j] && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + j] && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + j] && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + j] && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 3 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 2 + width) % width) * height + (j - 4 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height])  cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height])  cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && grid[i * height + (j - 3 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 4 + height) % height] && !grid[i * height + (j - 4 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 4 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 4 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] && !grid[i * height + (j + 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] && !grid[i * height + (j + 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] && !grid[i * height + (j + 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 3 + height) % height] && !grid[i * height + (j + 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 3 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && !grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 1 + width) % width) * height + (j + 2 + height) % height] && !grid[i * height + (j + 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && !grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && grid[((i + 3 + width) % width) * height + j] && !grid[((i + 4 + width) % width) * height + j] \
                    && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 2 + height) % height] \
                    && !grid[((i - 1 + width) % width) * height + (j - 3 + height) % height] && !grid[i * height + (j - 3 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 3 + height) % height] && !grid[((i + 4 + width) % width) * height + (j - 3 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && !grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height]) cycle_count += 1;

                else if (!grid[((i - 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j + 1 + height) % height] && !grid[i * height + (j + 1 + height) % height] && !grid[((i + 1 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 2 + width) % width) * height + (j + 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j + 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + j] && grid[((i - 1 + width) % width) * height + j] && grid[i * height + j] && grid[((i + 1 + width) % width) * height + j] && !grid[((i + 2 + width) % width) * height + j] && !grid[((i + 3 + width) % width) * height + j] \
                    && !grid[((i - 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 1 + height) % height] && grid[i * height + (j - 1 + height) % height] && grid[((i + 1 + width) % width) * height + (j - 1 + height) % height] && grid[((i + 2 + width) % width) * height + (j - 1 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 1 + height) % height] \
                    && !grid[((i - 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i - 1 + width) % width) * height + (j - 2 + height) % height] && !grid[i * height + (j - 2 + height) % height] && !grid[((i + 1 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 2 + width) % width) * height + (j - 2 + height) % height] && !grid[((i + 3 + width) % width) * height + (j - 2 + height) % height]) cycle_count += 1;
            }
        }
    }
}

void updateGrid() { // основной алгоритм программы
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            int neighbors = 0; // считаем соседей
            if (grid[(i - 1 + width)%width*height + (j - 1 + height) % height]) neighbors++;
            if (grid[(i - 1 + width)%width*height + (j  + height) % height]) neighbors++;
            if (grid[(i - 1 + width)%width*height + (j + 1 + height) % height]) neighbors++;
            if (grid[(i + width)%width*height +  (j - 1 + height) % height]) neighbors++;
            if (grid[(i + width)%width*height +  (j + 1 + height) % height]) neighbors++;
            if (grid[(i + 1 + width)%width*height +  (j - 1 + height) % height]) neighbors++;
            if (grid[(i + 1 + width)%width*height +  (j  + height) % height]) neighbors++;
            if (grid[(i + 1 + width)%width*height +  (j + 1 + height) % height]) neighbors++;
            if (grid[i*height + j]) {
                nextGrid[i*height + j] = (neighbors == 2 || neighbors == 3);
            }
            else {
                nextGrid[i * height + j] = (neighbors == 3);
            }
        }
    }
    bool* temp = grid;
    grid = nextGrid;
    nextGrid = temp;

    generation_count++;
    cycleCounter();
}

int check_valid_index_x(int x) {
    if (x < 0) return 80 + x;
    else return x % 80;
}

int check_valid_index_y(int y) {
    if (y < 0) return 55 + y;
    else return y % 55;
}

void applyBrush(int cellY, int cellX) {
    if (cellY >= 0 && cellY < height && cellX >= 0 && cellX < width) {
        switch (brush) {
        case 1:
            grid[cellY + cellX*height] = true;
            break;
        
        case 2:
            if (cellX + 4 < width && cellY + 3 < height) {
                grid[(cellX + 2) * height + cellY] = true; grid[(cellX + 3) * height + cellY] = true;
                grid[(cellX)*height + cellY + 1] = true; grid[(cellX + 1)*height + cellY + 1] = true; grid[(cellX + 3)*height + cellY + 1] = true; grid[(cellX + 4)*height + cellY + 1] = true;
                grid[(cellX)*height + cellY + 2] = true; grid[(cellX + 1)*height + cellY + 2] = true; grid[(cellX + 2)*height + cellY + 2] = true; grid[(cellX + 3)*height + cellY + 2] = true;
                grid[(cellX + 1)*height + cellY + 3] = true; grid[(cellX + 2)*height + cellY + 3] = true;
            }
            break;
        case 3:
            if (cellY + 4 < height && cellX + 5 < width) {
                grid[(cellX + 2) * height + cellY] = true;
                grid[(cellX)*height + cellY + 1] = true; grid[(cellX + 4)*height + cellY + 1] = true;
                grid[(cellX + 5)*height + cellY + 2] = true;
                grid[(cellX)*height + cellY + 3] = true; grid[(cellX + 5)*height + cellY + 3] = true;
                grid[(cellX + 1)*height + cellY + 4] = true; grid[(cellX + 2)*height + cellY + 4] = true; grid[(cellX + 3)*height + cellY + 4] = true; grid[(cellX + 4)*height + cellY + 4] = true; grid[(cellX + 5)*height + cellY + 4] = true;
            }
            break;
        case 4:
            if (cellY + 10 < height && cellX + 12 < width) {
                grid[(cellX + 12) * height + cellY] = true;
                grid[(cellX + 9)*height + cellY + 1] = true; grid[(cellX + 12)*height + cellY + 1] = true;
                grid[(cellX + 8)*height + cellY + 2] = true; grid[(cellX + 10)*height + cellY + 2] = true;
                grid[(cellX + 10)*height + cellY + 3] = true; grid[(cellX + 12)*height + cellY + 3] = true;
                grid[(cellX)*height + cellY + 4] = true; grid[(cellX + 1)*height + cellY + 4] = true; grid[(cellX + 5)*height + cellY + 4] = true; grid[(cellX + 12)*height + cellY + 4] = true;
                grid[(cellX)*height + cellY + 5] = true; grid[(cellX + 1)*height + cellY + 5] = true; grid[(cellX + 5)*height + cellY + 5] = true; grid[(cellX + 9)*height + cellY + 5] = true; grid[(cellX + 10)*height + cellY + 5] = true; grid[(cellX + 11)*height + cellY + 5] = true;
                grid[(cellX + 1)*height + cellY + 6] = true; grid[(cellX + 3)*height + cellY + 6] = true; grid[(cellX + 5)*height + cellY + 6] = true; grid[(cellX + 7)*height + cellY + 6] = true; grid[(cellX + 10)*height + cellY + 6] = true;
                grid[(cellX + 2)*height + cellY + 8] = true; grid[(cellX + 3)*height + cellY + 8] = true; grid[(cellX + 5)*height + cellY + 8] = true; grid[(cellX + 6)*height + cellY + 8] = true;
                grid[(cellX + 3)*height + cellY + 9] = true; grid[(cellX + 4)*height + cellY + 9] = true; grid[(cellX + 5)*height + cellY + 9] = true;
                grid[(cellX + 4)*height + cellY + 10] = true;
            }
            break;
        case 5:
            if (cellY + 10 < height && cellX + 18 < width) {
                grid[(cellX) * height + cellY] = true; grid[(cellX + 7)*height + cellY] = true; grid[(cellX + 8)*height + cellY] = true; grid[(cellX + 10)*height + cellY] = true; grid[(cellX + 11)*height + cellY] = true; grid[(cellX + 18)*height + cellY] = true;
                grid[(cellX)*height + cellY + 1] = true; grid[(cellX + 8)*height + cellY + 1] = true; grid[(cellX + 10)*height + cellY + 1] = true; grid[(cellX + 18)*height + cellY + 1] = true;
                grid[(cellX + 2)*height + cellY + 2] = true; grid[(cellX + 3)*height + cellY + 2] = true; grid[(cellX + 7)*height + cellY + 2] = true; grid[(cellX + 8)*height + cellY + 2] = true; grid[(cellX + 10)*height + cellY + 2] = true; grid[(cellX + 11)*height + cellY + 2] = true; grid[(cellX + 15)*height + cellY + 2] = true; grid[(cellX + 16)*height + cellY + 2] = true;
                grid[(cellX + 2)*height + cellY + 3] = true; grid[(cellX + 5)*height + cellY + 3] = true; grid[(cellX + 8)*height + cellY + 3] = true; grid[(cellX + 10)*height + cellY + 3] = true; grid[(cellX + 13)*height + cellY + 3] = true; grid[(cellX + 16)*height + cellY + 3] = true;
                grid[(cellX + 6)*height + cellY + 4] = true; grid[(cellX + 8)*height + cellY + 4] = true; grid[(cellX + 10)*height + cellY + 4] = true; grid[(cellX)*height + cellY + 4] = true; grid[(cellX + 12)*height + cellY + 4] = true; grid[(cellX + 18)*height + cellY + 4] = true;
                grid[(cellX)*height + cellY + 5] = true; grid[(cellX + 1)*height + cellY + 5] = true; grid[(cellX + 2)*height + cellY + 5] = true; grid[(cellX + 8)*height + cellY + 5] = true; grid[(cellX + 10)*height + cellY + 5] = true; grid[(cellX + 16)*height + cellY + 5] = true; grid[(cellX + 17)*height + cellY + 5] = true; grid[(cellX + 18)*height + cellY + 5] = true;
                grid[(cellX + 1)*height + cellY + 6] = true; grid[(cellX + 6)*height + cellY + 6] = true; grid[(cellX + 8)*height + cellY + 6] = true; grid[(cellX + 10)*height + cellY + 6] = true; grid[(cellX + 12)*height + cellY + 6] = true; grid[(cellX + 17)*height + cellY + 6] = true;
                grid[(cellX + 8)*height + cellY + 7] = true; grid[(cellX + 10)*height + cellY + 7] = true;
                grid[(cellX + 6)*height + cellY + 8] = true; grid[(cellX + 7)*height + cellY + 8] = true; grid[(cellX + 11)*height + cellY + 8] = true; grid[(cellX + 12)*height + cellY + 8] = true;
                grid[(cellX + 3)*height + cellY + 9] = true; grid[(cellX + 4)*height + cellY + 9] = true; grid[(cellX + 6)*height + cellY + 9] = true; grid[(cellX + 7)*height + cellY + 9] = true; grid[(cellX + 11)*height + cellY + 9] = true; grid[(cellX + 12)*height + cellY + 9] = true; grid[(cellX + 14)*height + cellY + 9] = true; grid[(cellX + 15)*height + cellY + 9] = true;
                grid[(cellX + 5)*height + cellY + 10] = true; grid[(cellX + 13)*height + cellY + 10] = true;
            }
            break;
        
        case 6:
            if (cellY + 8 < height && cellX + 35 < width) {
                grid[(cellX + 10) * height + cellY] = true;
                grid[(cellX + 5) * height + cellY + 1] = true;grid[(cellX + 10) * height + cellY + 1] = true;grid[(cellX + 11) * height + cellY + 1] = true;grid[(cellX + 12) * height + cellY + 1] = true;grid[(cellX + 13) * height + cellY + 1] = true;
                grid[(cellX + 5) * height + cellY + 2] = true;grid[(cellX + 11) * height + cellY + 2] = true;grid[(cellX + 12) * height + cellY + 2] = true;grid[(cellX + 13) * height + cellY + 2] = true;grid[(cellX + 14) * height + cellY + 2] = true;grid[(cellX + 22) * height + cellY + 2] = true;
                grid[(cellX)*height + cellY + 3] = true;grid[(cellX + 1) * height + cellY + 3] = true;grid[(cellX + 11) * height + cellY + 3] = true;grid[(cellX + 14) * height + cellY + 3] = true;grid[(cellX + 21) * height + cellY + 3] = true;grid[(cellX + 23) * height + cellY + 3] = true;
                grid[(cellX)*height + cellY + 4] = true;grid[(cellX + 1) * height + cellY + 4] = true;grid[(cellX + 11) * height + cellY + 4] = true;grid[(cellX + 12) * height + cellY + 4] = true;grid[(cellX + 13) * height + cellY + 4] = true;grid[(cellX + 14) * height + cellY + 4] = true;grid[(cellX + 19) * height + cellY + 4] = true;grid[(cellX + 20) * height + cellY + 4] = true;grid[(cellX + 24) * height + cellY + 4] = true;
                grid[(cellX + 10) * height + cellY + 5] = true;grid[(cellX + 11) * height + cellY + 5] = true;grid[(cellX + 12) * height + cellY + 5] = true;grid[(cellX + 13) * height + cellY + 5] = true;grid[(cellX + 19) * height + cellY + 5] = true;grid[(cellX + 20) * height + cellY + 5] = true;grid[(cellX + 24) * height + cellY + 5] = true;grid[(cellX + 34) * height + cellY + 5] = true;grid[(cellX + 35) * height + cellY + 5] = true;
                grid[(cellX + 10) * height + cellY + 6] = true;grid[(cellX + 19) * height + cellY + 6] = true;grid[(cellX + 20) * height + cellY + 6] = true;grid[(cellX + 24) * height + cellY + 6] = true;grid[(cellX + 34) * height + cellY + 6] = true;grid[(cellX + 35) * height + cellY + 6] = true;
                grid[(cellX + 21) * height + cellY + 7] = true;grid[(cellX + 23) * height + cellY + 7] = true;
                grid[(cellX + 22) * height + cellY + 8] = true;
            }
            break;
        case 7:
            if (cellY + 5 < height && cellX + 8 < width) {
                grid[(cellX + 1) * height + cellY] = true;grid[(cellX + 2) * height + cellY] = true;grid[(cellX + 6) * height + cellY] = true;grid[(cellX + 7) * height + cellY] = true;
                grid[(cellX)*height + cellY + 1] = true;grid[(cellX + 1) * height + cellY + 1] = true;grid[(cellX + 3) * height + cellY + 1] = true;grid[(cellX + 5) * height + cellY + 1] = true;grid[(cellX + 7) * height + cellY + 1] = true;grid[(cellX + 8) * height + cellY + 1] = true;
                grid[(cellX + 3) * height + cellY + 2] = true;grid[(cellX + 5) * height + cellY + 2] = true;
                grid[(cellX + 3) * height + cellY + 3] = true;grid[(cellX + 5) * height + cellY + 3] = true;
                grid[(cellX + 2) * height + cellY + 5] = true;grid[(cellX + 3) * height + cellY + 5] = true;grid[(cellX + 5) * height + cellY + 5] = true;grid[(cellX + 6) * height + cellY + 5] = true;
            }
            break;
        case 8:
            if (cellY + 6 < height && cellX + 6 < width) {
                grid[(cellX + 1) * height + cellY] = true;grid[(cellX + 2) * height + cellY] = true;
                grid[(cellX + 1) * height + cellY + 1] = true;
                grid[(cellX + 2) * height + cellY + 2] = true;grid[(cellX + 4) * height + cellY + 2] = true;
                grid[(cellX)*height + cellY + 4] = true;grid[(cellX + 2) * height + cellY + 4] = true;grid[(cellX + 4) * height + cellY + 4] = true;grid[(cellX + 5) * height + cellY + 4] = true;
                grid[(cellX)*height + cellY + 5] = true;grid[(cellX + 1) * height + cellY + 5] = true;
                grid[(cellX + 4) * height + cellY + 6] = true;grid[(cellX + 5) * height + cellY + 6] = true;grid[(cellX + 6) * height + cellY + 6] = true;
            }
            break;
        case 9:
            grid[cellX*height + cellY] = false;
            break;
        }
    }
}

bool checkButtonPress(int x, int y) {
    y = HEIGHT - y;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].widthb &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].heightb) {
            brush = buttons[i].id;
            return true;
        }
    }
    return false;
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (!checkButtonPress(x, y)) {
            drawing = true;
            int cellX = x / CELL_SIZE;
            int cellY = (HEIGHT - y) / CELL_SIZE;
            applyBrush(cellY, cellX);
        }
    }
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        drawing = false;
    }
}

void mouseMotion(int x, int y) {
    if (!drawing)
        return;
    int cellX = x / CELL_SIZE;
    int cellY = (HEIGHT - y) / CELL_SIZE;
    applyBrush(cellY, cellX);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case ' ':
        simulating = !simulating;
        break;
    case 'w':
        speed -= 10;
        if (speed < 0) speed = 0;
        break;
    case 's':
        speed += 10;
        break;
    case 'r':
        if (!simulating) {
            initGrid();
            glutPostRedisplay();
        }
        break;
    case 't':
    {
        bool empty = true;
        for (int i = 0; i < width && empty; ++i) {
            for (int j = 0; j < height && empty; ++j) {
                if (grid[i*height + j]) {
                    empty = false;
                }
            }
        }
        if (empty) {
            srand(time(NULL));
            for (int i = 0; i < width; ++i) {
                for (int j = 0; j < height; ++j) {
                    if (rand() % 2 == 0) {
                        grid[i*height + j] = true;
                    }
                }
            }
            glutPostRedisplay();
        }
    }
    break;
    }
}

void timer(int value) {
    if (simulating) {
        updateGrid();
    }
    glutPostRedisplay();
    glutTimerFunc(speed, timer, 0);
}

void reshape(int w, int h) {
    glutReshapeWindow(WIDTH, HEIGHT);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}