#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <vector>
//--- 필요한 헤더파일 include
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

//--- 필요한 변수 선언
GLint width = 800, height = 600;
GLuint shaderProgramID; //--- 세이더 프로그램 이름
GLuint vertexShader; //--- 버텍스 세이더 객체
GLuint fragmentShader; //--- 프래그먼트 세이더 객체

// 랜덤 생성기
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

// 객체 구조체
struct Shape {
    std::vector<float> vertices;
    std::vector<unsigned int> index;
    std::vector<float> colors;
    glm::vec3 position;
    GLuint VAO, VBO[2], EBO;
};
Shape robot[7]; // 0 머리, 1 몸통, 2 왼팔, 3 오른팔, 4 왼다리, 5 오른다리, 6 코
glm::mat4 robot_animation[2]; glm::mat4 robot_movement;
bool movemotion; float robot_speed = 0.05f;
//glm::vec3 robot_pos; glm::vec3 next_pos;

// 카메라
struct Camera {
    glm::vec3 eye;
    glm::vec3 at;
    glm::vec3 up;
} camera = { glm::vec3(0.0f, 0.0f, 5.0f),
             glm::vec3(0.0f, 0.0f, 0.0f),
             glm::vec3(0.0f, 1.0f, 0.0f) };
float angle; float cos_angle; float sin_angle;
float new_eye_x; float new_eye_z; bool ortho = false;

char* filetobuf(const char* file) {
    FILE* fptr;
    long length;
    char* buf;
    fptr = fopen(file, "rb"); // Open file for reading
    if (!fptr) // Return NULL on failure
        return NULL;
    fseek(fptr, 0, SEEK_END); // Seek to the end of the file
    length = ftell(fptr); // Find out how many bytes into the file we are
    buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
    fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
    fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
    fclose(fptr); // Close the file
    buf[length] = 0; // Null terminator
    return buf; // Return the buffer
}

//--- 사용자 정의 함수 선언
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
GLvoid specialKeyboard(int key, int x, int y);
GLvoid specialKeyboardUp(int key, int x, int y);
void TimerFunction(int value);
void InitBuffers(Shape& shape);
void CreateCube(Shape& cube, float x, float y, float z);
void CreateRobot();
void Robot_Animation();
void drawrobot(unsigned int transformLocation);
bool AABB(glm::vec3 pos1, float size1, glm::vec3 pos2, float size2) { // 충돌 검사 함수
    return (pos1.x - size1 <= pos2.x + size2 && pos1.x + size1 >= pos2.x - size2 &&
        pos1.z - size1 <= pos2.z + size2 && pos1.z + size1 >= pos2.z - size2);
}
bool wall_collision(glm::vec3 pos1, float size1, float size2) { // 벽 충돌 검사 함수
    return (pos1.x - size1 <= -size2 || pos1.x + size1 >= size2 ||
        pos1.z - size1 <= -size2 || pos1.z + size1 >= size2);
}

// 메뉴
void menu() {
    std::cout << "o: 직각투영" << std::endl;
    std::cout << "p: 원근투영" << std::endl;
    std::cout << "z/Z: 원근 투영시 카메라 z축 이동" << std::endl;
    std::cout << "m/M: 육면체 위아래 애니메이션" << std::endl;
    std::cout << "y/Y: 카메라 바닥 기준 y축 회전" << std::endl;
    std::cout << "r: 미로생성" << std::endl;
    std::cout << "v: 육면체 낮은높이로 고정" << std::endl;
    std::cout << "s: 로봇 생성" << std::endl;
    std::cout << "방향키: 로봇 이동" << std::endl;
    std::cout << "+/-: 로봇 속도 조절" << std::endl;
    std::cout << "1/3: 카메라 시점 변경" << std::endl;
    std::cout << "c: 초기화" << std::endl;
    std::cout << "q: 종료" << std::endl;
}

//--- 메인 함수
void main(int argc, char** argv) {
    //--- 윈도우 생성하기
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("Example20");
    //--- GLEW 초기화하기
    glewExperimental = GL_TRUE;
    glewInit();
    //--- 세이더 읽어와서 세이더 프로그램 만들기: 사용자 정의함수 호출
    make_vertexShaders(); //--- 버텍스 세이더 만들기
    make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
    shaderProgramID = make_shaderProgram();
    //--- 콜백 함수 등록
    menu();
	CreateRobot(); // 로봇 생성
    glutTimerFunc(50, TimerFunction, 1); // 타이머 함수 등록
    glutDisplayFunc(drawScene); // 출력 함수의 지정
    glutReshapeFunc(Reshape); // 다시 그리기 함수 지정
    glutKeyboardFunc(Keyboard); // 키보드 입력
	glutSpecialFunc(specialKeyboard); // 방향키 입력
	glutSpecialUpFunc(specialKeyboardUp);
    glutMainLoop();
}

// 큐브 생성 함수
void CreateCube(Shape& cube, float x, float y, float z) {
    cube.vertices = {
        // 앞면
       -x, y, z,
       -x, -y, z,
       x, -y, z,
       x, y, z,

       // 뒷면
       -x, -y, -z,
       -x, y, -z,
       x, y, -z,
       x, -y, -z,

       // 윗면
       -x, y, -z,
       -x, y, z,
       x, y, z,
       x, y, -z,

       // 아래면 
       -x, -y, z,
       -x, -y, -z,
       x, -y, -z,
       x, -y, z,

       // 왼면 
       -x, y, -z,
       -x, -y, -z,
       -x, -y, z,
       -x, y, z,

       // 오른면
       x, y, z,
       x, -y, z,
       x, -y, -z,
       x, y, -z
    };
    cube.index = {
        // 앞면
        0, 1, 2, 0, 2, 3,
        // 뒷면
        4, 5, 6, 4, 6, 7,
        // 아래면
        8, 9, 10, 8, 10, 11,
        // 위면
        12, 13, 14, 12, 14, 15,
        // 오른면
        16, 17, 18, 16, 18, 19,
        // 왼면
        20, 21, 22, 20, 22, 23
    };
    cube.colors = {
        // 앞면 - 연한회색
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,

        // 뒷면 - 연한 회색
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,
        0.8f, 0.8f, 0.8f,

        // 아랫면 - 진한 회색
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,

        // 윗면 - 진한 회색
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,
        0.3f, 0.3f, 0.3f,

        // 오른면 - 회색
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,

        // 왼면 - 파란색
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f
    };
    InitBuffers(cube);
}

// 로봇 생성 함수
void CreateRobot() {
	// 로봇 애니메이션 초기화
	robot_animation[0] = glm::mat4(1.0f);
	robot_animation[1] = glm::mat4(1.0f);
	robot_movement = glm::mat4(1.0f);
	movemotion = false;
	// 로봇 부위별 생성
    CreateCube(robot[0], 0.1f, 0.1f, 0.1f); // 머리
    CreateCube(robot[1], 0.2f, 0.4f, 0.1f); // 몸통
    CreateCube(robot[2], 0.05f, 0.3f, 0.05f); // 왼팔
    CreateCube(robot[3], 0.05f, 0.3f, 0.05f); // 오른팔
    CreateCube(robot[4], 0.05f, 0.3f, 0.05f); // 왼다리
    CreateCube(robot[5], 0.05f, 0.3f, 0.05f); // 오른다리
    CreateCube(robot[6], 0.02f, 0.05f, 0.02f); // 코
	// 위치 설정
    robot[0].position = glm::vec3(0.0f, 1.35f, 0.0f); // 머리
    robot[1].position = glm::vec3(0.0f, 0.85f, 0.0f); // 몸통
    robot[2].position = glm::vec3(-0.25f, 0.95f, 0.0f); // 왼팔
    robot[3].position = glm::vec3(0.25f, 0.95f, 0.0f); // 오른팔
    robot[4].position = glm::vec3(-0.1f, 0.3f, 0.0f); // 왼다리
    robot[5].position = glm::vec3(0.1f, 0.3f, 0.0f); // 오른다리
    robot[6].position = glm::vec3(0.0f, 1.3f, 0.1f); // 코
	// 색상 설정
    for (int i = 0; i < 7; i++) {
        robot[i].colors.clear();
    }
    for (size_t j = 0; j < robot[0].vertices.size() / 3; j++) {
        // 머리 - 빨간색
        robot[0].colors.push_back(1.0f);
        robot[0].colors.push_back(0.0f);
        robot[0].colors.push_back(0.0f);
        // 몸통 - 초록색
        robot[1].colors.push_back(0.0f);
        robot[1].colors.push_back(1.0f);
        robot[1].colors.push_back(0.0f);
        // 왼팔 - 파란색
        robot[2].colors.push_back(0.0f);
        robot[2].colors.push_back(0.0f);
        robot[2].colors.push_back(1.0f);
        // 오른팔 - 시안색
        robot[3].colors.push_back(0.0f);
        robot[3].colors.push_back(1.0f);
        robot[3].colors.push_back(1.0f);
        // 왼다리 - 노란색
        robot[4].colors.push_back(1.0f);
        robot[4].colors.push_back(1.0f);
        robot[4].colors.push_back(0.0f);
        // 오른다리 - 마젠타색
        robot[5].colors.push_back(1.0f);
        robot[5].colors.push_back(0.0f);
        robot[5].colors.push_back(1.0f);
        // 코 - 검정색
        robot[6].colors.push_back(0.0f);
        robot[6].colors.push_back(0.0f);
        robot[6].colors.push_back(0.0f);
    }
    for (int i = 0; i < 7; i++) {
        InitBuffers(robot[i]);
    }
}

// 로봇 팔 다리 움직임 함수
void Robot_Animation() {
    static float robot_angle = 0.0f;
    static bool increasing = true;
    if (increasing) {
        robot_angle += 2.0f;
        if (robot_angle >= 20.f) {
            robot_angle = 20.0f;
            increasing = false;
        }
    }
    else {
        robot_angle -= 2.0f;
        if (robot_angle <= -20.0f) {
            robot_angle = -20.f;
            increasing = true;
        }
    }

    robot_animation[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(robot_angle), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.3f, 0.0f));
    robot_animation[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-robot_angle), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.3f, 0.0f));
}

// 버퍼 설정 함수
GLvoid InitBuffers(Shape& shape) {
    glGenVertexArrays(1, &shape.VAO);           // 버텍스 배열 객체id 생성
    glBindVertexArray(shape.VAO);               // 버텍스 배열 객체 바인딩

    glGenBuffers(2, shape.VBO);                 // 버퍼id 생성

    glBindBuffer(GL_ARRAY_BUFFER, shape.VBO[0]);   // 좌표
    glBufferData(GL_ARRAY_BUFFER, shape.vertices.size() * sizeof(float), shape.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &shape.EBO);                 // 인덱스 버퍼id 생성
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape.index.size() * sizeof(unsigned int), shape.index.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, shape.VBO[1]);   // 색상
    glBufferData(GL_ARRAY_BUFFER, shape.colors.size() * sizeof(float), shape.colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
}

//--- 버텍스 세이더 객체 만들기
void make_vertexShaders() {
    GLchar* vertexSource;
    vertexSource = filetobuf("vertex.glsl");
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

//--- 프래그먼트 세이더 객체 만들기
void make_fragmentShaders() {
    GLchar* fragmentSource;
    fragmentSource = filetobuf("fragment.glsl");
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

//--- 세이더 프로그램 만들고 세이더 객체 링크하기
GLuint make_shaderProgram() {
    GLuint shaderID;
    GLint result;
    GLchar errorLog[512];
    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return false;
    }
    glUseProgram(shaderID);
    return shaderID;
}

//--- 출력 콜백 함수
GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수
{
    // 배경을 흰색으로 설정
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glUseProgram(shaderProgramID);

    unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projection");
    unsigned int viewLocation = glGetUniformLocation(shaderProgramID, "view");
    unsigned int transformLocation = glGetUniformLocation(shaderProgramID, "model");

    // 투영 행렬 설정
    glm::mat4 projection = glm::mat4(1.0f);
	if (ortho) projection = glm::ortho(-5.0f, 5.0f, -5.0f * (float)height / (float)width, 5.0f * (float)height / (float)width, 0.1f, 100.0f);
    else projection = glm::perspective(glm::radians(90.0f), (float)width / (float)height, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    // 뷰 행렬 설정
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(camera.eye, camera.at, camera.up);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

    // 육면체 그리기
    /*glBindVertexArray(box.VAO);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(model));
    glDrawElements(GL_TRIANGLES, box.index.size(), GL_UNSIGNED_INT, 0);*/

    // 로봇 그리기
    drawrobot(transformLocation);

    glutSwapBuffers();
}

// 로봇 그리기 함수
void drawrobot(unsigned int transformLocation) {
    for (int i = 0; i < 7; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = robot_movement * model;
        model = glm::translate(model, robot[i].position);
        if (i == 2 || i == 5) model = model * robot_animation[0];
        else if (i == 3 || i == 4) model = model * robot_animation[1];
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(robot[i].VAO);
        glDrawElements(GL_TRIANGLES, robot[i].index.size(), GL_UNSIGNED_INT, 0);
    }
}

//--- 다시그리기 콜백 함수
GLvoid Reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

// 키보드 콜백 함수
GLvoid Keyboard(unsigned char key, int x, int y) {
    switch (key) {
        // 직각 투영
        case 'o':
			ortho = true;
            break;
		// 원근 투영
        case 'p':
			ortho = false;
			break;
        // 카메라 z축 이동
        case 'z': 
            camera.eye.z -= 0.1f;
            break;
        case 'Z':
            camera.eye.z += 0.1f;
            break;
		// 육면체 애니메이션
        case 'm': 
			break;
		case 'M':
			break;
        // 카메라 y축 공전
        case 'y':
            angle = glm::radians(5.0f);
            cos_angle = cos(angle);
            sin_angle = sin(angle);

            new_eye_x = camera.eye.x * cos_angle - camera.eye.z * sin_angle;
            new_eye_z = camera.eye.x * sin_angle + camera.eye.z * cos_angle;

            camera.eye.x = new_eye_x;
            camera.eye.z = new_eye_z;
            break;
        case 'Y':
            angle = glm::radians(-5.0f);
            cos_angle = cos(angle);
            sin_angle = sin(angle);

            new_eye_x = camera.eye.x * cos_angle - camera.eye.z * sin_angle;
            new_eye_z = camera.eye.x * sin_angle + camera.eye.z * cos_angle;

            camera.eye.x = new_eye_x;
            camera.eye.z = new_eye_z;
            break;
		// 미로 생성
        case 'r':
			break;
        // 육면체 낮은높이로 고정
        case 'v':
            break;
		// 로봇 생성
        case 's':
            break; 
        // 로봇 속도 조절
        case '+':
            if (robot_speed < 0.1f) robot_speed += 0.01f;
            break;
        case '-':
            if (robot_speed > 0.01f) robot_speed -= 0.01f;
            break;
		// 카메라 시점 변경
        case '1':
            break;
		case '3':
			break;
        // 초기화
        case 'c':
		    CreateRobot();
            break;
	    // 종료
        case 'q':
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// 방향키 콜백 함수
GLvoid specialKeyboard(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP: // 로봇 이동
        static glm::vec3 robot_pos = glm::vec3(robot_movement[3][0], robot_movement[3][1], robot_movement[3][2]);
        static glm::vec3 next_pos = robot_pos + glm::vec3(robot_movement * glm::vec4(0.0f, 0.0f, robot_speed, 1.0f));
		// 충돌 검사
        robot_movement = glm::translate(robot_movement, glm::vec3(0.0f, 0.0f, robot_speed));
        movemotion = true;
        break;
    case GLUT_KEY_DOWN:
        robot_movement = glm::rotate(robot_movement, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        break;
    case GLUT_KEY_LEFT:
        robot_movement = glm::rotate(robot_movement, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        break;
    case GLUT_KEY_RIGHT:
        robot_movement = glm::rotate(robot_movement, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        break;
    }
    glutPostRedisplay();
}

// 방향키 떼기 콜백 함수
GLvoid specialKeyboardUp(int key, int x, int y) {
    if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN || key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT) {
        movemotion = false;
		robot_animation[0] = glm::mat4(1.0f);
		robot_animation[1] = glm::mat4(1.0f);
    }
    glutPostRedisplay();
}

// 타이머 콜백 함수
void TimerFunction(int value) {
    if (movemotion) Robot_Animation();
    glutPostRedisplay();
    glutTimerFunc(50, TimerFunction, 1);
}