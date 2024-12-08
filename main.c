#define _CRT_SECURE_NO_WARNINGS
#include <GL/freeglut.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 600
#define CELL_SIZE 10
#define BUTTON_HEIGHT 50
#define NUM_BUTTONS 9


bool grid[WIDTH / CELL_SIZE][(HEIGHT - BUTTON_HEIGHT) / CELL_SIZE];
bool nextGrid[WIDTH / CELL_SIZE][(HEIGHT - BUTTON_HEIGHT) / CELL_SIZE];
bool simulating = false;
bool drawing = false;
int speed = 10;
int brush = 1;
int generation_count = 0;

// Button definitions
typedef struct {
    float x, y, width, height;
    char label[10];
    int id;
} Button;

Button buttons[NUM_BUTTONS];

void initButtons() { //инициализация кнопок
    const char* brushNames[NUM_BUTTONS] = {
            "   Line", "Board1", "Board2", "Board3", "Board4",
            "Loop1", "Loop2", "Loop3", "Erase"
    };

    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].x = i * (WIDTH / NUM_BUTTONS); // х начала
        buttons[i].y = HEIGHT - BUTTON_HEIGHT; // у начала
        buttons[i].width = WIDTH / NUM_BUTTONS; // ширина
        buttons[i].height = BUTTON_HEIGHT; // высота
        sprintf(buttons[i].label, "%s", brushNames[i]); // название кнопок
        buttons[i].id = i + 1;
    }
}

void drawButtons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        glColor3f(0.0, 0.5, 0.5); //выбор цвета заднего плана кнопок
        glBegin(GL_QUADS); // постановка кисти, рисование кнопки
        glVertex2f(buttons[i].x, buttons[i].y);
        glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y);
        glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height);
        glVertex2f(buttons[i].x, buttons[i].y + buttons[i].height);
        glEnd();

        glColor3f(0.0, 0.0, 0.0); // цвет текста
        glRasterPos2f(buttons[i].x + 10, buttons[i].y + 30); // позиция текста
        for (char* c = buttons[i].label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // рисование символа
        }
    }
}

void initGrid() { // заполняем нулями все окно
    for (int i = 0; i < WIDTH / CELL_SIZE; ++i) {
        for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE; ++j) {
            grid[i][j] = false;
            nextGrid[i][j] = false;
        }
    }
    generation_count = 0;
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
    for (int i = 0; i < WIDTH / CELL_SIZE; ++i) { // рисовка поставленных клеток
        for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE; ++j) {
            if (grid[i][j]) {
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

    drawButtons();
    glutSwapBuffers();
}

void updateGrid() { // основной алгоритм программы
    int u, p;
    for (int i = 0; i < WIDTH / CELL_SIZE; ++i) {
        for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE; ++j) {
            int neighbors = 0; // считаем соседей
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    int x = i + dx;
                    int y = j + dy;
                    if (x >= 0) u = x % (WIDTH / CELL_SIZE);
                    else u = 79;
                    if (y >= 0) p = y % ((HEIGHT - BUTTON_HEIGHT) / CELL_SIZE);
                    else p = 54;
                    if (grid[u][p]) neighbors++;
                }
            }
            if (grid[i][j]) {
                nextGrid[i][j] = (neighbors == 2 || neighbors == 3);
            }
            else {
                nextGrid[i][j] = (neighbors == 3);
            }
        }
    }
    for (int i = 0; i < WIDTH / CELL_SIZE; ++i) {
        for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE; ++j) {
            grid[i][j] = nextGrid[i][j];
        }
    }
    generation_count++;
}

void applyBrush(int cellX, int cellY) {
    if (cellX >= 0 && cellX < WIDTH / CELL_SIZE && cellY >= 0 && cellY < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
        switch (brush) {
        case 1:
            grid[cellX][cellY] = true;
            break;
        case 2:
            if (cellX + 4 < WIDTH / CELL_SIZE && cellY + 3 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 1] = true; grid[cellX][cellY + 2] = true;
                grid[cellX + 1][cellY + 1] = true; grid[cellX + 1][cellY + 2] = true; grid[cellX + 1][cellY + 3] = true;
                grid[cellX + 2][cellY] = true; grid[cellX + 2][cellY + 2] = true; grid[cellX + 2][cellY + 3] = true;
                grid[cellX + 3][cellY] = true; grid[cellX + 3][cellY + 1] = true; grid[cellX + 3][cellY + 2] = true;
                grid[cellX + 4][cellY + 1] = true;
            }
            break;
        case 3:
            if (cellX + 5 < WIDTH / CELL_SIZE && cellY + 4 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 1] = true; grid[cellX][cellY + 3] = true;
                grid[cellX + 1][cellY + 4] = true;
                grid[cellX + 2][cellY] = true; grid[cellX + 2][cellY + 4] = true;
                grid[cellX + 3][cellY + 4] = true;
                grid[cellX + 4][cellY + 1] = true; grid[cellX + 4][cellY + 4] = true;
                grid[cellX + 5][cellY + 2] = true; grid[cellX + 5][cellY + 3] = true; grid[cellX + 5][cellY + 4] = true;
            }
            break;
        case 4:
            if (cellX + 12 < WIDTH / CELL_SIZE && cellY + 10 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 4] = true; grid[cellX][cellY + 5] = true;
                grid[cellX + 1][cellY + 4] = true; grid[cellX + 1][cellY + 5] = true; grid[cellX + 1][cellY + 6] = true;
                grid[cellX + 2][cellY + 8] = true;
                grid[cellX + 3][cellY + 6] = true; grid[cellX + 3][cellY + 8] = true; grid[cellX + 3][cellY + 9] = true;
                grid[cellX + 4][cellY + 9] = true; grid[cellX + 4][cellY + 10] = true;
                grid[cellX + 5][cellY + 4] = true; grid[cellX + 5][cellY + 5] = true; grid[cellX + 5][cellY + 6] = true; grid[cellX + 5][cellY + 8] = true; grid[cellX + 5][cellY + 9] = true;
                grid[cellX + 6][cellY + 8] = true;
                grid[cellX + 7][cellY + 6] = true;
                grid[cellX + 8][cellY + 2] = true;
                grid[cellX + 9][cellY + 1] = true; grid[cellX + 9][cellY + 5] = true;
                grid[cellX + 10][cellY + 2] = true; grid[cellX + 10][cellY + 3] = true; grid[cellX + 10][cellY + 5] = true; grid[cellX + 10][cellY + 6] = true;
                grid[cellX + 11][cellY + 5] = true;
                grid[cellX + 12][cellY] = true; grid[cellX + 12][cellY + 1] = true; grid[cellX + 12][cellY + 3] = true; grid[cellX + 12][cellY + 4] = true;
            }
            break;
        case 5:
            if (cellX + 18 < WIDTH / CELL_SIZE && cellY + 10 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY] = true; grid[cellX][cellY + 1] = true; grid[cellX][cellY + 4] = true; grid[cellX][cellY + 5] = true;
                grid[cellX + 1][cellY + 5] = true; grid[cellX + 1][cellY + 6] = true;
                grid[cellX + 2][cellY + 2] = true; grid[cellX + 2][cellY + 3] = true; grid[cellX + 2][cellY + 5] = true;
                grid[cellX + 3][cellY + 2] = true; grid[cellX + 3][cellY + 9] = true;
                grid[cellX + 4][cellY + 9] = true;
                grid[cellX + 5][cellY + 3] = true; grid[cellX + 5][cellY + 10] = true;
                grid[cellX + 6][cellY + 4] = true; grid[cellX + 6][cellY + 6] = true; grid[cellX + 6][cellY + 8] = true; grid[cellX + 6][cellY + 9] = true;
                grid[cellX + 7][cellY] = true; grid[cellX + 7][cellY + 2] = true; grid[cellX + 7][cellY + 8] = true; grid[cellX + 7][cellY + 9] = true;
                grid[cellX + 8][cellY] = true; grid[cellX + 8][cellY + 1] = true; grid[cellX + 8][cellY + 2] = true; grid[cellX + 8][cellY + 3] = true; grid[cellX + 8][cellY + 4] = true; grid[cellX + 8][cellY + 5] = true; grid[cellX + 8][cellY + 6] = true; grid[cellX + 8][cellY + 7] = true;
                grid[cellX + 10][cellY] = true; grid[cellX + 10][cellY + 1] = true; grid[cellX + 10][cellY + 2] = true; grid[cellX + 10][cellY + 3] = true; grid[cellX + 10][cellY + 4] = true; grid[cellX + 10][cellY + 5] = true; grid[cellX + 10][cellY + 6] = true; grid[cellX + 10][cellY + 7] = true;
                grid[cellX + 11][cellY] = true; grid[cellX + 11][cellY + 2] = true; grid[cellX + 11][cellY + 8] = true; grid[cellX + 11][cellY + 9] = true;
                grid[cellX + 12][cellY + 4] = true; grid[cellX + 12][cellY + 6] = true; grid[cellX + 12][cellY + 8] = true; grid[cellX + 12][cellY + 9] = true;
                grid[cellX + 13][cellY + 3] = true; grid[cellX + 13][cellY + 10] = true;
                grid[cellX + 14][cellY + 9] = true;
                grid[cellX + 15][cellY + 2] = true; grid[cellX + 15][cellY + 9] = true;
                grid[cellX + 16][cellY + 2] = true; grid[cellX + 16][cellY + 3] = true; grid[cellX + 16][cellY + 5] = true;
                grid[cellX + 17][cellY + 5] = true; grid[cellX + 17][cellY + 6] = true;
                grid[cellX + 18][cellY] = true; grid[cellX + 18][cellY + 1] = true; grid[cellX + 18][cellY + 4] = true; grid[cellX + 18][cellY + 5] = true;
            }
            break;

        case 6:
            if (cellX + 35 < WIDTH / CELL_SIZE && cellY + 8 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 3] = true; grid[cellX][cellY + 4] = true;
                grid[cellX + 1][cellY + 3] = true; grid[cellX + 1][cellY + 4] = true;
                grid[cellX + 5][cellY + 1] = true; grid[cellX + 5][cellY + 2] = true;
                grid[cellX + 10][cellY] = true; grid[cellX + 10][cellY + 1] = true; grid[cellX + 10][cellY + 5] = true; grid[cellX + 10][cellY + 6] = true;
                grid[cellX + 11][cellY + 1] = true; grid[cellX + 11][cellY + 2] = true; grid[cellX + 11][cellY + 3] = true; grid[cellX + 11][cellY + 4] = true; grid[cellX + 11][cellY + 5] = true;
                grid[cellX + 12][cellY + 1] = true; grid[cellX + 12][cellY + 2] = true; grid[cellX + 12][cellY + 4] = true; grid[cellX + 12][cellY + 5] = true;
                grid[cellX + 13][cellY + 1] = true; grid[cellX + 13][cellY + 2] = true; grid[cellX + 13][cellY + 4] = true; grid[cellX + 13][cellY + 5] = true;
                grid[cellX + 14][cellY + 2] = true; grid[cellX + 14][cellY + 3] = true; grid[cellX + 14][cellY + 4] = true;
                grid[cellX + 19][cellY + 4] = true; grid[cellX + 19][cellY + 5] = true; grid[cellX + 19][cellY + 6] = true;
                grid[cellX + 20][cellY + 4] = true; grid[cellX + 20][cellY + 5] = true; grid[cellX + 20][cellY + 6] = true;
                grid[cellX + 21][cellY + 3] = true; grid[cellX + 21][cellY + 7] = true;
                grid[cellX + 22][cellY + 2] = true; grid[cellX + 22][cellY + 8] = true;
                grid[cellX + 23][cellY + 3] = true; grid[cellX + 23][cellY + 7] = true;
                grid[cellX + 24][cellY + 4] = true; grid[cellX + 24][cellY + 5] = true; grid[cellX + 24][cellY + 6] = true;
                grid[cellX + 34][cellY + 5] = true; grid[cellX + 34][cellY + 6] = true;
                grid[cellX + 35][cellY + 5] = true; grid[cellX + 35][cellY + 6] = true;
            }
            break;
        case 7:
            if (cellX + 8 < WIDTH / CELL_SIZE && cellY + 5 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 1] = true;
                grid[cellX + 1][cellY] = true; grid[cellX + 1][cellY + 1] = true;
                grid[cellX + 2][cellY] = true; grid[cellX + 2][cellY + 5] = true;
                grid[cellX + 3][cellY + 1] = true; grid[cellX + 3][cellY + 2] = true; grid[cellX + 3][cellY + 3] = true; grid[cellX + 3][cellY + 5] = true;
                grid[cellX + 5][cellY + 1] = true; grid[cellX + 5][cellY + 2] = true; grid[cellX + 5][cellY + 3] = true; grid[cellX + 5][cellY + 5] = true;
                grid[cellX + 6][cellY] = true; grid[cellX + 6][cellY + 5] = true;
                grid[cellX + 7][cellY] = true; grid[cellX + 7][cellY + 1] = true;
                grid[cellX + 8][cellY + 1] = true;
            }
            break;
        case 8:
            if (cellX + 6 < WIDTH / CELL_SIZE && cellY + 6 < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE) {
                grid[cellX][cellY + 4] = true; grid[cellX][cellY + 5] = true;
                grid[cellX + 1][cellY] = true; grid[cellX + 1][cellY + 1] = true; grid[cellX + 1][cellY + 5] = true;
                grid[cellX + 2][cellY] = true; grid[cellX + 2][cellY + 2] = true; grid[cellX + 2][cellY + 4] = true;
                grid[cellX + 4][cellY + 2] = true; grid[cellX + 4][cellY + 4] = true; grid[cellX + 4][cellY + 6] = true;
                grid[cellX + 5][cellY + 4] = true; grid[cellX + 5][cellY + 6] = true;
                grid[cellX + 6][cellY + 6] = true;
            }
            break;
        case 9:
            grid[cellX][cellY] = false;
            break;
        }
    }
}

bool checkButtonPress(int x, int y) {
    y = HEIGHT - y;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].width &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].height) {
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
            applyBrush(cellX, cellY);
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
    applyBrush(cellX, cellY);
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
        for (int i = 0; i < WIDTH / CELL_SIZE && empty; ++i) {
            for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE && empty; ++j) {
                if (grid[i][j]) {
                    empty = false;
                }
            }
        }
        if (empty) {
            srand(time(NULL));
            for (int i = 0; i < WIDTH / CELL_SIZE; ++i) {
                for (int j = 0; j < (HEIGHT - BUTTON_HEIGHT) / CELL_SIZE; ++j) {
                    if (rand() % 2 == 0) {
                        grid[i][j] = true;
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

int main(int argc, char** argv) {
    glutInit(&argc, argv); //инициализация библиотеки glut
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // установка режимов дисплея
    glutInitWindowSize(WIDTH, HEIGHT); // размер окна
    glutCreateWindow("Game of Life with GUI"); // создание окна с именем

    glClearColor(1.0, 1.0, 1.0, 1.0); //установка цвета заднего плана
    initGrid();
    initButtons();

    glutDisplayFunc(drawGrid);
    glutMouseFunc(mouse); // позиция мыши на экране без рисования
    glutMotionFunc(mouseMotion); // позиция мыши на экране с рисования
    glutKeyboardFunc(keyboard); // нажатия кнопок на клавиатуре
    glutReshapeFunc(reshape); // сохранение окна
    glutTimerFunc(speed, timer, 0); // для постоянной анимации

    glutMainLoop(); // Запускаем цикл обработки сообщений GLUT

    return 0;
}