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
std::uniform_real_distribution<float> high(1.0f, 2.0f);
std::uniform_real_distribution<float> low(0.5f, 0.9f);
std::uniform_real_distribution<float> speed(0.01f, 0.1f);

// 객체 구조체
struct Shape {
	std::vector<float> vertices;
	std::vector<unsigned int> index;
	std::vector<float> colors;
	glm::vec3 position;
	float max_height;
	float min_height;
	float speed;
	bool direction;
	bool is_way;
	GLuint VAO, VBO[2], EBO;
};
Shape robot[7]; // 0 머리, 1 몸통, 2 왼팔, 3 오른팔, 4 왼다리, 5 오른다리, 6 코
Shape box; Shape ground;
glm::mat4 robot_animation[2]; glm::mat4 robot_movement;
bool movemotion; bool robot_rotate = true;
float robot_speed = 0.05f; bool robot_on = false;
std::vector<std::vector<Shape>> Maze;
bool wall_animation; bool create_maze = true;
bool start = false;

// 카메라
struct Camera {
	glm::vec3 eye;
	glm::vec3 at;
	glm::vec3 up;
	bool projection = true; // true: 원근, false: 직각
	bool lookat = false; // true: 로봇 1인칭 시점, false: 로봇 3인칭 시점 
} camera = { glm::vec3(0.0f, 10.0f, 23.0f),
			 glm::vec3(0.0f, 0.0f, 0.0f),
			 glm::vec3(0.0f, 1.0f, 0.0f) };

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
void Create_Cube(Shape& cube, float x, float y, float z);
// 미로 관련 함수
void Create_Wall();
void Start_Wall_Animation();
void Draw_Wall(unsigned int transformLocation);
void Wall_Animation();
void Create_Maze();
// 로봇 관련 함수
void Create_Robot();
void Draw_Robot(unsigned int transformLocation);
void Robot_Animation();
bool Robot_collision(glm::vec3 next_position);
// 카메라 업데이트 함수
void Update_Camera();
void Camera_Rotate(float angle);
// 충돌 검사 함수
bool AABB(glm::vec3 pos1, float size1, glm::vec3 pos2, float size2) { // 충돌 검사 함수
	return (pos1.x - size1 <= pos2.x + size2 && pos1.x + size1 >= pos2.x - size2 &&
		pos1.z - size1 <= pos2.z + size2 && pos1.z + size1 >= pos2.z - size2);
}
bool wall_collision(glm::vec3 pos1, float size1, float size2) { // 벽 충돌 검사 함수
	return (pos1.x - size1 <= -size2 || pos1.x + size1 >= size2 ||
		pos1.z - size1 <= -size2 || pos1.z + size1 >= size2);
}

// 카메라 회전 함수
void Camera_Rotate(float angle) {
	float cos_angle = cos(glm::radians(angle));
	float sin_angle = sin(glm::radians(angle));

	float new_eye_x = camera.eye.x * cos_angle - camera.eye.z * sin_angle;
	float new_eye_z = camera.eye.x * sin_angle + camera.eye.z * cos_angle;

	camera.eye.x = new_eye_x;
	camera.eye.z = new_eye_z;
}

// 카메라 업데이트 함수
void Update_Camera() {
	if (robot_on) {
		if (camera.lookat) {
			// === 1인칭 시점 개선 ===
			// 로봇의 현재 위치 계산
			glm::vec3 robot_pos = glm::vec3(robot_movement[3][0], robot_movement[3][1], robot_movement[3][2]);
			
			// 로봇의 방향 벡터 계산 (로봇이 바라보는 방향)
			glm::vec4 forward_vec = robot_movement * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			glm::vec3 forward = glm::normalize(glm::vec3(forward_vec.x, 0.0f, forward_vec.z));
			
			// 카메라를 로봇의 눈 높이에 위치 (머리보다 약간 낮게)
			glm::vec3 eye_position = robot_pos + glm::vec3(0.0f, 1.2f, 0.0f);
			
			// 벽과의 충돌을 고려하여 카메라 위치 미세 조정
			// 로봇의 코 앞쪽으로 약간 이동시켜 벽에 너무 가깝지 않게 함
			eye_position = eye_position + forward * 0.1f;
			
			// 시야 방향 설정 (로봇이 바라보는 방향으로 더 멀리)
			glm::vec3 look_direction = eye_position + forward * 10.0f;
			
			camera.eye = eye_position;
			camera.at = look_direction;
			camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else {
			// === 3인칭 시점 (기존 코드 개선) ===
			glm::vec3 robot_pos = glm::vec3(robot_movement[3][0], robot_movement[3][1], robot_movement[3][2]);
			glm::vec4 forward_vec = robot_movement * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			glm::vec3 forward = glm::normalize(glm::vec3(forward_vec.x, 0.0f, forward_vec.z));
			
			// 카메라를 더 높이, 더 멀리 위치시키고, 위에서 내려다보는 각도로 조정
			camera.eye = robot_pos - forward * 5.0f + glm::vec3(0.0f, 6.0f, 0.0f);
			camera.at = robot_pos + glm::vec3(0.0f, 1.0f, 0.0f);
			camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		}
	}
}

// 파일 읽기 함수
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
	std::cout << "+/-: 육면체 애니메이션 속도 조절" << std::endl;
	std::cout << "1/3: 카메라 시점 변경" << std::endl;
	std::cout << "c: 초기화" << std::endl;
	std::cout << "q: 종료" << std::endl;
}

// 큐브 생성 함수
void Create_Cube(Shape& cube, float x, float y, float z) {
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
	if (x <= 0.3f) {
		cube.colors = {
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f
		};
	}
	else {
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
	}
	
	InitBuffers(cube);
}

// 벽 생성 함수
void Create_Wall() {
	int row, col;
	std::cout << "크기 제한 5 ~ 25" << std::endl;
	std::cout << "행과 열을 입력해 주세요: ";
	// 입력 받기
	while (!(std::cin >> row >> col)) {
		std::cout << "잘못된 입력입니다. 숫자를 입력해주세요: ";
		std::cin.clear();
		std::cin.ignore(10000, '\n');
	}

	// 유효성 검사
	if (row < 5 || col < 5) {
		std::cout << "최소 5 이상이어야 합니다." << std::endl;
		Create_Wall(); // 재귀 호출로 다시 입력받기
		return;
	}
	if (row > 25 || col > 25) {
		std::cout << "최대 25 이하이어햐 합니다." << std::endl;
		Create_Wall(); // 재귀 호출로 다시 입력받기
		return;
	}
	std::cout << "크기가 " << row << "x" << col << "로 설정되었습니다." << std::endl;

	// 초기화
	Maze.clear();
	Maze.resize(row, std::vector<Shape>(col));

	// 땅 생성
	Create_Cube(ground, col / 2.0f, 0.0f, row / 2.0f);

	// 벽 생성
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			Maze[i][j].max_height = high(gen);
			Maze[i][j].min_height = low(gen);
			Maze[i][j].speed = speed(gen);
			Maze[i][j].position = glm::vec3(j - col / 2.0f, 0.0f, i - row / 2.0f);
			Create_Cube(Maze[i][j], 0.5f, 0.0f, 0.5f);
			Maze[i][j].direction = true;
		}
	}
	start = true;
}

// 벽 생성 애니메이션 함수
void Start_Wall_Animation() {
	for (int i = 0; i < Maze.size(); i++) {
		for (int j = 0; j < Maze[i].size(); j++) {
			float current_height = Maze[i][j].position.y;
			if (Maze[i][j].direction) {
				current_height += Maze[i][j].speed;
				if (current_height > Maze[i][j].max_height) {
					current_height = Maze[i][j].max_height;
					Maze[i][j].direction = false;
				}
			}
			// y 위치 업데이트
			Maze[i][j].position.y = current_height;
			Create_Cube(Maze[i][j], 0.5f, current_height, 0.5f);
		}
	}
	for (int i = 0; i < Maze.size(); i++) {
		for (int j = 0; j < Maze[i].size(); j++) {
			if (Maze[i][j].direction) return; // 아직 애니메이션 중인 벽이 있음 
		}
	}
	std::cout << "미로가 생성되었습니다." << std::endl;
	menu(); // 메뉴 목록 출력
	start = false;
}

// 벽 그리기 함수
void Draw_Wall(unsigned int transformLocation) {
	for (int i =0; i < Maze.size(); i++) {
		for (int j = 0; j < Maze[i].size(); j++) {
			if (Maze[i][j].is_way) continue;
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, Maze[i][j].position);
			glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(model));
			glBindVertexArray(Maze[i][j].VAO);
			glDrawElements(GL_TRIANGLES, Maze[i][j].index.size(), GL_UNSIGNED_INT, 0);
		}
	}
}

// 벽 애니메이션 함수
void Wall_Animation() {
	for (int i = 0; i < Maze.size(); i++) {
		for (int j = 0; j < Maze[i].size(); j++) {
			float current_height = Maze[i][j].position.y;
			if (Maze[i][j].direction) {
				current_height += Maze[i][j].speed;
				if (current_height > Maze[i][j].max_height) {
					current_height = Maze[i][j].max_height;
					Maze[i][j].direction = false;
				}
			}
			else {
				current_height -= Maze[i][j].speed;
				if (current_height < Maze[i][j].min_height) {
					current_height = Maze[i][j].min_height;
					Maze[i][j].direction = true;
				}
			}
			// y 위치 업데이트
			Maze[i][j].position.y = current_height;
			Create_Cube(Maze[i][j], 0.5f, current_height, 0.5f);
		}
	}
}

// 미로 생성 함수
void Create_Maze() {
	// 모든 셀을 벽으로 초기화
	for (int i = 0; i < Maze.size(); i++) {
		for (int j = 0; j < Maze[0].size(); j++) {
			Maze[i][j].is_way = false; // 벽으로 설정
		}
	}
	// 시작점과 출구 설정
	Maze[0][0].is_way = true;
	Maze[1][0].is_way = true;
	// 행 및 열 크기
	int rows = static_cast<int>(Maze.size());
	int cols = static_cast<int>(Maze[0].size());
	// Maze 배열의 크기 그대로 활용하며, 방문 기록을 위한 별도 배열을 정의합니다.
	std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
	// 재귀 백트래킹을 위한 스택 (함수 내부 변수)
	std::vector<std::pair<int, int>> stack;
	// 방향 정의 (2칸씩 점프하는 방식 사용)
	// 이 방식은 홀수 인덱스의 셀에서 홀수 인덱스의 셀로 이동하며 벽을 허뭅니다.
	int dx[] = { 0, 2, 0, -2 };
	int dy[] = { -2, 0, 2, 0 };
	// 시작점을 설정
	int start_x = 1; int start_y = 1;
	Maze[start_y][start_x].is_way = true;
	visited[start_y][start_x] = true;
	stack.push_back({ start_x, start_y });
	// 스택이 빌 때까지 반복
	while (!stack.empty()) {
		// 현재 위치
		int current_x = stack.back().first;
		int current_y = stack.back().second;
		// 현재 셀에서 방문하지 않은 이웃들 찾기
		std::vector<int> unvisited_neighbors;
		for (int dir = 0; dir < 4; dir++) {
			int next_x = current_x + dx[dir];
			int next_y = current_y + dy[dir];
			// 경계 내에 있고 (0과 rows-1 사이), 아직 방문하지 않은 셀인 경우
			// 가장자리를 남겨두어 벽으로 사용
			if (next_x > 0 && next_x < cols - 1 &&
				next_y > 0 && next_y < rows - 1 &&
				!visited[next_y][next_x]) unvisited_neighbors.push_back(dir);
		}
		// 방문하지 않은 이웃이 있으면
		if (!unvisited_neighbors.empty()) {
			// 랜덤하게 방향 선택
			std::uniform_int_distribution<int> rand_dir(0, static_cast<int>(unvisited_neighbors.size()) - 1);
			int chosen_dir = unvisited_neighbors[rand_dir(gen)];
			// --- 셀 사이의 벽 허물기 ---
			int next_x = current_x + dx[chosen_dir];
			int next_y = current_y + dy[chosen_dir];
			Maze[next_y][next_x].is_way = true;
			int wall_row = current_y + dy[chosen_dir] / 2;
			int wall_col = current_x + dx[chosen_dir] / 2;
			Maze[wall_row][wall_col].is_way = true;
			// 다음 셀을 방문 표시하고 스택에 추가
			visited[next_y][next_x] = true;
			stack.push_back({ next_x, next_y });
		}
		// 막다른 길이면 백트래킹
		else stack.pop_back();
	}
	// 행이 짝수일 경우 마지막 행(rows-1)의 벽을 길로 만듭니다.
	if (rows % 2 == 0 && rows > 0) {
		for (int j = 0; j < cols; ++j) {
			Maze[rows - 1][j].is_way = true;
		}
	}
	// 열이 짝수일 경우 마지막 열(cols-1)의 벽을 길로 만듭니다.
	if (cols % 2 == 0 && cols > 0) {
		for (int i = 0; i < rows; ++i) {
			Maze[i][cols - 1].is_way = true;
		}
	}
	// 출구 설정
	Maze[rows - 1][cols - 1].is_way = true;
	Maze[rows - 1][cols - 2].is_way = true;
	Maze[rows - 2][cols - 2].is_way = true;
	Maze[rows - 2][cols - 3].is_way = true;	
}

// 로봇 생성 함수
void Create_Robot() {
	// 로봇 애니메이션 초기화
	robot_animation[0] = glm::mat4(1.0f);
	robot_animation[1] = glm::mat4(1.0f);

	// 로봇 초기 위치 설정
	robot_movement = glm::mat4(1.0f);
	robot_movement = glm::translate(glm::mat4(1.0f), glm::vec3(Maze[0][0].position.x, 0.0f, Maze[0][0].position.z));
	
	movemotion = false;
	// 로봇 부위별 생성
	Create_Cube(robot[0], 0.1f, 0.1f, 0.1f); // 머리
	Create_Cube(robot[1], 0.2f, 0.4f, 0.1f); // 몸통
	Create_Cube(robot[2], 0.05f, 0.3f, 0.05f); // 왼팔
	Create_Cube(robot[3], 0.05f, 0.3f, 0.05f); // 오른팔
	Create_Cube(robot[4], 0.05f, 0.3f, 0.05f); // 왼다리
	Create_Cube(robot[5], 0.05f, 0.3f, 0.05f); // 오른다리
	Create_Cube(robot[6], 0.02f, 0.05f, 0.02f); // 코
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

// 로봇 그리기 함수
void Draw_Robot(unsigned int transformLocation) {
	for (int i = 0; i < 7; i++) {
		if (camera.lookat) continue; // 로봇 시점일 때 그리지 않음
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

// 로봇 충돌 검사 함수
bool Robot_collision(glm::vec3 next_position) {
    for (int i = 0; i < static_cast<int>(Maze.size()); i++) {
        for (int j = 0; j < static_cast<int>(Maze[0].size()); j++) {
            if (Maze[i][j].is_way) continue;
			if (AABB(next_position, 0.2f, Maze[i][j].position, 0.5f)) return false; // 충돌 발생
        }
    }
	return true;
}

//--- 메인 함수
void main(int argc, char** argv) {
	//--- 윈도우 생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("25_숙제");
	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glewInit();
	//--- 세이더 읽어와서 세이더 프로그램 만들기: 사용자 정의함수 호출
	make_vertexShaders(); //--- 버텍스 세이더 만들기
	make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
	shaderProgramID = make_shaderProgram();
	//--- 콜백 함수 등록
	Create_Wall(); // 벽 생성
	glutTimerFunc(50, TimerFunction, 1); // 타이머 함수 등록
	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정
	glutKeyboardFunc(Keyboard); // 키보드 입력
	glutSpecialFunc(specialKeyboard); // 방향키 입력
	glutSpecialUpFunc(specialKeyboardUp);
	glutMainLoop();
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

	// ========== 메인 화면 ==========
	// 메인 뷰포트
	glViewport(0, 0, width, height);
	// 투영 행렬
	glm::mat4 projection = glm::mat4(1.0f);
	// 원근 투영
	if (camera.projection) projection = glm::perspective(glm::radians(90.0f), (float)width / (float)height, 0.1f, 100.0f);
	// 직각 투영
	else  projection = glm::ortho(-5.0f, 5.0f, -5.0f * (float)height / (float)width, 5.0f * (float)height / (float)width, 0.1f, 100.0f); 
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	// 뷰 행렬
	glm::mat4 view = glm::lookAt(camera.eye, camera.at, camera.up);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	// 땅 그리기
	glm::mat4 ground_model = glm::mat4(1.0f);
	glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(ground_model));
	glBindVertexArray(ground.VAO);
	glDrawElements(GL_TRIANGLES, ground.index.size(), GL_UNSIGNED_INT, 0);
	// 벽 그리기
	Draw_Wall(transformLocation);
	// 로봇 그리기
	if (robot_on) Draw_Robot(transformLocation);

	// ========== 미니맵 ==========
	// 깊이 테스트를 잠시 비활성화하여 미니맵이 항상 보이도록 함
	glDisable(GL_DEPTH_TEST);
	// 미니맵 뷰포트 설정
	int minimap_size = std::min(width, height) / 4;
	// 여백을 충분히 두어 화면 경계와 겹치지 않도록 함
	int margin = 15;
	glViewport(width - minimap_size - margin, height - minimap_size - margin, minimap_size, minimap_size);
	// 미로 크기에 맞춘 직각 투영 행렬
	float maze_rows = static_cast<float>(Maze.size());
	float maze_cols = static_cast<float>(Maze[0].size());
	float max_dimension = std::max(maze_rows, maze_cols);
	float view_range = (max_dimension + 2.0f) / 2.0f; // 미로 전체가 보이도록 여유 공간 추가
	// 미니맵용 투영 행렬 (항상 직각 투영)
	glm::mat4 minimap_projection = glm::ortho(-view_range, view_range, -view_range, view_range, -10.0f, 10.0f);
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(minimap_projection));
	// 미니맵용 뷰 행렬 (위에서 내려다보는 시점으로 고정)
	glm::mat4 minimap_view = glm::lookAt(
		glm::vec3(0.0f, 5.0f, 0.0f),   // 카메라 위치 (위쪽에서)
		glm::vec3(0.0f, 0.0f, 0.0f),   // 바라보는 지점 (미로 중심)
		glm::vec3(0.0f, 0.0f, -1.0f)   // 업 벡터
	);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(minimap_view));
	// 미니맵 배경
	glClearDepth(1.0f);
	// 벽 그리기
	Draw_Wall(transformLocation);
	// 로봇 위치 표시 
	if (robot_on) {
		glm::mat4 robot_minimap_model = glm::mat4(1.0f);
		robot_minimap_model = robot_movement * robot_minimap_model;
		robot_minimap_model = glm::scale(robot_minimap_model, glm::vec3(1.5f, 1.0f, 1.5f));
		glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(robot_minimap_model));
		glBindVertexArray(box.VAO);
		glDrawElements(GL_TRIANGLES, box.index.size(), GL_UNSIGNED_INT, 0);
	}
	// 깊이 테스트 다시 활성화
	glEnable(GL_DEPTH_TEST);

	glutSwapBuffers();
}

//--- 다시그리기 콜백 함수
GLvoid Reshape(int w, int h) {
	glViewport(0, 0, w, h);
}

// 키보드 콜백 함수
GLvoid Keyboard(unsigned char key, int x, int y) {
	if (!start) {
		if (robot_on) {
			// 1인칭 시점
			if (key == '1') {
				camera.lookat = true;
				std::cout << "카메라 시점이 1인칭으로 변경되었습니다." << std::endl;
			}
			// 3인칭 시점
			else if (key == '3') {
				camera.lookat = false;
				std::cout << "카메라 시점이 3인칭으로 변경되었습니다." << std::endl;
			}
		}
		else {
			// 직각 투영
			if (key == 'o') camera.projection = false;
			// 원근 투영
			else if (key == 'p') camera.projection = true;
			if (camera.projection) {
				// 카메라 z축 앞으로 이동
				if (key == 'z') camera.eye.z -= 0.1f;
				// 카메라 z축 뒤로 이동
				else if (key == 'Z') camera.eye.z += 0.1f;
				// 카메라 바닥 기준 y축 양 회전
				else if (key == 'y') Camera_Rotate(5.0f);
				// 카메라 바닥 기준 y축 음 회전
				else if (key == 'Y') Camera_Rotate(-5.0f);
			}
			// 미로생성
			if (key == 'r') {
				if (create_maze) {
					Create_Maze();
					create_maze = false;
				}
			}
			// 로봇 생성
			if (!create_maze) {
				if (key == 's') {
					Create_Robot();
					Create_Cube(box, 0.3f, 0.0f, 0.3f);
					camera.lookat = true;
					robot_on = true;
				}
			}
		}
		// 육면체 위아래 애니메이션
		if (key == 'm') wall_animation = true;
		// 육면체 애니메이션 정지
		else if (key == 'M') wall_animation = false;
		// 육면체 낮은높이로 고정
		else if (key == 'v') {
			if (wall_animation) {
				wall_animation = false;
				for (int i = 0; i < Maze.size(); i++) {
					for (int j = 0; j < Maze[i].size(); j++) {
						Maze[i][j].position.y = Maze[i][j].min_height;
						Create_Cube(Maze[i][j], 0.5f, Maze[i][j].min_height, 0.5f);
					}
				}
			}
			else wall_animation = true;
		}
		// 육면체 애니메이샨 속도 증가
		else if (key == '+') {
			for (int i = 0; i < Maze.size(); i++) {
				for (int j = 0; j < Maze[i].size(); j++) {
					Maze[i][j].speed += 0.01f;
					if (Maze[i][j].speed > 0.2f) Maze[i][j].speed = 0.2f;
				}
			}
		}
		// 육면체 애니메이션 속도 감소
		else if (key == '-') {
			for (int i = 0; i < Maze.size(); i++) {
				for (int j = 0; j < Maze[i].size(); j++) {
					Maze[i][j].speed -= 0.01f;
					if (Maze[i][j].speed < 0.01f) Maze[i][j].speed = 0.01f;
				}
			}
		}
		// 초기화
		else if (key == 'c') {
			// 미로 초기화
			for (int i = 0; i < Maze.size(); i++) {
				for (int j = 0; j < Maze[i].size(); j++) {
					Maze[i][j].is_way = false;
					Maze[i][j].position.y = Maze[i][j].max_height;
					Create_Cube(Maze[i][j], 0.5f, Maze[i][j].max_height, 0.5f);
					Maze[i][j].direction = true;
					Maze[i][j].speed = speed(gen);
				}
			}
			wall_animation = false;
			create_maze = true;
			// 로봇 초기화
			robot_on = false;
			robot_speed = 0.05f;
			// 카메라 초기화
			camera.eye = glm::vec3(0.0f, 10.0f, 23.0f);
			camera.at = glm::vec3(0.0f, 0.0f, 0.0f);
			camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
			camera.projection = true;
		}
	}
	// 종료
	if (key == 'q') exit(0);
	
	glutPostRedisplay();
}

// 방향키 콜백 함수
GLvoid specialKeyboard(int key, int x, int y) {
	if (robot_on) {
		if (key == GLUT_KEY_UP) {
			glm::vec3 robot_pos = glm::vec3(robot_movement[3][0], robot_movement[3][1], robot_movement[3][2]);
			glm::vec3 next_pos = robot_pos + glm::vec3(glm::mat3(robot_movement) * glm::vec3(0.0f, 0.0f, robot_speed));	
			if (Robot_collision(next_pos)) {
				robot_movement = glm::translate(robot_movement, glm::vec3(0.0f, 0.0f, robot_speed));
				movemotion = true;
			}
		}
		else {
			// 회전은 충돌 검사 없이 수행
			if (robot_rotate) {
				robot_rotate = false;
				if (key == GLUT_KEY_DOWN) robot_movement = glm::rotate(robot_movement, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				else if (key == GLUT_KEY_LEFT) robot_movement = glm::rotate(robot_movement, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				else if (key == GLUT_KEY_RIGHT) robot_movement = glm::rotate(robot_movement, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
		}
	}
	glutPostRedisplay();
}

// 방향키 떼기 콜백 함수
GLvoid specialKeyboardUp(int key, int x, int y) {
	if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN || key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT) {
		movemotion = false;
		robot_rotate = true;
		robot_animation[0] = glm::mat4(1.0f);
		robot_animation[1] = glm::mat4(1.0f);
	}
	glutPostRedisplay();
}

// 타이머 콜백 함수
void TimerFunction(int value) {
	if (start) Start_Wall_Animation();
	if (movemotion) Robot_Animation();
	if (wall_animation) Wall_Animation();
	Update_Camera(); // 카메라 업데이트
	
	glutPostRedisplay();
	glutTimerFunc(50, TimerFunction, 1);
}