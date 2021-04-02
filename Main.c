#include <stdio.h>
#include <limits.h>
#include <SDL.h>
#include "Constants.h"

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

struct Player {
	float x;
	float y;
	float width;
	float height;
	int turnDirection; // -1 if left, +1 if right
	int walkDirection; // -1 if back, +1 if front
	float rotationAngle;
	float rotationSpeed;
	float walkSpeed;
	float turnSpeed;
} player;

struct Ray {
	float rayAngle;
	float wallHitX;
	float wallHitY;
	float distance;
	int wasHitVertical;
	int isRayFacingUp;
	int isRayFacingDown;
	int isRayFacingRight;
	int isRayFacingLeft;
	int wallHitContent;
} rays[NUM_RAYS];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int isGameRunning = FALSE;
float ticksLastFrame = 0;

Uint32* colorBuffer = NULL;
SDL_Texture* colorBufferTexture = NULL;

int initializeWindow() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL.\n");
		return FALSE;
	}

	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_BORDERLESS
	);
	if (!window) {
		fprintf(stderr, "Error creating SDL window.\n");
		return FALSE;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Error creating SDL renderer.\n");
		return FALSE;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	return TRUE;
}

void setup() {
	player.x = WINDOW_WIDTH / 2;
	player.y = WINDOW_HEIGHT / 2;
	player.width = PLAYER_WIDTH;
	player.height = PLAYER_HEIGHT;
	player.turnDirection = 0;
	player.walkDirection = 0;
	player.rotationAngle = PI / 2;
	player.walkSpeed = PLAYER_WALK_SPEED;
	player.turnSpeed = PLAYER_TURN_SPEED;

	colorBuffer = (Uint32*) malloc(sizeof(Uint32) * (Uint32)WINDOW_WIDTH * (Uint32)WINDOW_HEIGHT);
	colorBufferTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH,
		WINDOW_HEIGHT
	);
}

void processInput() {
	SDL_Event event;
	SDL_PollEvent(&event);

	switch (event.type) {
		case SDL_QUIT:
			isGameRunning = FALSE;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				isGameRunning = FALSE;
			if (event.key.keysym.sym == SDLK_UP)
				player.walkDirection = +1;
			if (event.key.keysym.sym == SDLK_DOWN)
				player.walkDirection = -1;
			if (event.key.keysym.sym == SDLK_RIGHT)
				player.turnDirection = +1;
			if (event.key.keysym.sym == SDLK_LEFT)
				player.turnDirection = -1;
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_UP)
				player.walkDirection = 0;
			if (event.key.keysym.sym == SDLK_DOWN)
				player.walkDirection = 0;
			if (event.key.keysym.sym == SDLK_RIGHT)
				player.turnDirection = 0;
			if (event.key.keysym.sym == SDLK_LEFT)
				player.turnDirection = 0;
			break;
	}
}

void renderMap() {
	for (int i = 0; i < MAP_NUM_ROWS; i++) {
		for (int j = 0; j < MAP_NUM_COLS; j++) {
			int tileX = j * TILE_SIZE;
			int tileY = i * TILE_SIZE;
			int tileColor = map[i][j] != 0 ? 255 : 0;
			
			SDL_SetRenderDrawColor(renderer, tileColor, tileColor, tileColor, 255);
			SDL_Rect mapTileRect = {
				tileX * MINIMAP_SCALE_FACTOR,
				tileY * MINIMAP_SCALE_FACTOR,
				TILE_SIZE * MINIMAP_SCALE_FACTOR,
				TILE_SIZE * MINIMAP_SCALE_FACTOR,
			};
			SDL_RenderFillRect(renderer, &mapTileRect);
		}
	}
}

void renderPlayer() {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Rect playerRect = {
		player.x * MINIMAP_SCALE_FACTOR,
		player.y * MINIMAP_SCALE_FACTOR,
		player.width * MINIMAP_SCALE_FACTOR,
		player.height * MINIMAP_SCALE_FACTOR
	};
	SDL_RenderFillRect(renderer, &playerRect);

	SDL_RenderDrawLine(
		renderer,
		MINIMAP_SCALE_FACTOR * player.x + (PLAYER_WIDTH / 2),
		MINIMAP_SCALE_FACTOR * player.y + (PLAYER_HEIGHT / 2),
		MINIMAP_SCALE_FACTOR * player.x + cos(player.rotationAngle) * 40,
		MINIMAP_SCALE_FACTOR * player.y + sin(player.rotationAngle) * 40
	);
}

int checkWallColision(float newPlayerX, float newPlayerY) {
	if (newPlayerX < 0 || newPlayerX > WINDOW_WIDTH || newPlayerY < 0 || newPlayerY > WINDOW_HEIGHT)
		return TRUE;

	int mapGridIndexX = floor(newPlayerX) / TILE_SIZE;
	int mapGridIndexY = floor(newPlayerY) / TILE_SIZE;

	return (map[mapGridIndexY][mapGridIndexX] == 1);
}

void movePlayer(float deltaTime) {
	player.rotationAngle += player.turnDirection * player.turnSpeed * deltaTime;
	float moveStep = player.walkDirection * player.walkSpeed * deltaTime;

	float newPlayerX = player.x + cos(player.rotationAngle) * moveStep;
	float newPlayerY = player.y + sin(player.rotationAngle) * moveStep;

	if (!checkWallColision(newPlayerX, newPlayerY)) {
		player.x = newPlayerX;
		player.y = newPlayerY;
	}
}

float normalizeAngle(float rayAngle) {
	rayAngle = remainder(rayAngle, TWO_PI);

	if (rayAngle < 0)
		rayAngle += TWO_PI;	

	return rayAngle;
}

float distanceBetweenPoints(float playerX, float playerY, float horzWallHitX, float horzWallHitY) {
	return sqrt((horzWallHitX - playerX) * (horzWallHitX - playerX) + (horzWallHitY - playerY) * (horzWallHitY - playerY));
}

void castRay(float rayAngle, int stripId) {
	rayAngle = normalizeAngle(rayAngle);

	int isRayFacingDown = rayAngle > 0 && rayAngle < PI;
	int isRayFacingUp = !isRayFacingDown;

	int isRayFacingRight = rayAngle < 0.5 * PI || rayAngle > 1.5 * PI;
	int isRayFacingLeft = !isRayFacingRight;

	float xIntercept, yIntercept;
	float xStep, yStep;

	///////////////////////////////////////////
	// HORIZONTAL RAY-GRID INTERSECTION CODE //
	///////////////////////////////////////////

	int foundHorzWallHit = FALSE;
	float horzWallHitX = 0;
	float horzWallHitY = 0;
	int horzWallContent = 0;

	yIntercept = floor(player.y / TILE_SIZE) * TILE_SIZE;
	yIntercept += isRayFacingDown ? TILE_SIZE : 0;

	xIntercept = player.x + (yIntercept - player.y) / tan(rayAngle);

	yStep = TILE_SIZE;
	yStep *= isRayFacingUp ? -1 : 1;

	xStep = TILE_SIZE / tan(rayAngle);
	xStep *= (isRayFacingLeft && xStep > 0) ? -1 : 1;
	xStep *= (isRayFacingRight && xStep < 0) ? -1 : 1;

	float nextHorzTouchX = xIntercept;
	float nextHorzTouchY = yIntercept;

	while (nextHorzTouchX >= 0 && nextHorzTouchX <= WINDOW_WIDTH && nextHorzTouchY >= 0 && nextHorzTouchY <= WINDOW_HEIGHT) {
		float xToCheck = nextHorzTouchX;
		float yToCheck = nextHorzTouchY - (isRayFacingUp ? 1 : 0);

		if (checkWallColision(xToCheck, yToCheck)) {
			horzWallHitX = nextHorzTouchX;
			horzWallHitY = nextHorzTouchY;
			foundHorzWallHit = TRUE;
			horzWallContent = map[(int)floor(nextHorzTouchY / TILE_SIZE)][(int)floor(nextHorzTouchX / TILE_SIZE)];
			break;
		}
		else {
			nextHorzTouchX += xStep;
			nextHorzTouchY += yStep;
		}
	}

	/////////////////////////////////////////
	// VERTICAL RAY-GRID INTERSECTION CODE //
	/////////////////////////////////////////

	int foundVertWallHit = FALSE;
	float vertWallHitX = 0;
	float vertWallHitY = 0;
	int vertWallContent = 0;

	xIntercept = floor(player.x / TILE_SIZE) * TILE_SIZE;
	xIntercept += isRayFacingRight ? TILE_SIZE : 0;

	yIntercept = player.y + (xIntercept - player.x) * tan(rayAngle);

	xStep = TILE_SIZE;
	xStep *= isRayFacingLeft ? -1 : 1;

	yStep = TILE_SIZE * tan(rayAngle);
	yStep *= (isRayFacingUp && yStep > 0) ? -1 : 1;
	yStep *= (isRayFacingDown && yStep < 0) ? -1 : 1;

	float nextVertTouchX = xIntercept;
	float nextVertTouchY = yIntercept;

	while (nextVertTouchX >= 0 && nextVertTouchX <= WINDOW_WIDTH && nextVertTouchY >= 0 && nextVertTouchY <= WINDOW_HEIGHT) {
		float xToCheck = nextVertTouchX - (isRayFacingLeft ? 1 : 0);
		float yToCheck = nextVertTouchY;

		if (checkWallColision(xToCheck, yToCheck)) {
			vertWallHitX = nextVertTouchX;
			vertWallHitY = nextVertTouchY;
			foundVertWallHit = TRUE;
			vertWallContent = map[(int)floor(nextVertTouchY / TILE_SIZE)][(int)floor(nextVertTouchX / TILE_SIZE)];
			break;
		}
		else {
			nextVertTouchX += xStep;
			nextVertTouchY += yStep;
		}
	}

	float horzHitDistance = foundHorzWallHit
		? distanceBetweenPoints(player.x, player.y, horzWallHitX, horzWallHitY)
		: INT_MAX;
	float vertHitDistance = foundVertWallHit
		? distanceBetweenPoints(player.x, player.y, vertWallHitX, vertWallHitY)
		: INT_MAX;

	if (vertHitDistance < horzHitDistance) {
		rays[stripId].distance = vertHitDistance;
		rays[stripId].wallHitX = vertWallHitX;
		rays[stripId].wallHitY = vertWallHitY;
		rays[stripId].wallHitContent = vertWallContent;
		rays[stripId].wasHitVertical = TRUE;
	}
	else {
		rays[stripId].distance = horzHitDistance;
		rays[stripId].wallHitX = horzWallHitX;
		rays[stripId].wallHitY = horzWallHitY;
		rays[stripId].wallHitContent = horzWallContent;
		rays[stripId].wasHitVertical = FALSE;
	}
	rays[stripId].rayAngle = rayAngle;
	rays[stripId].isRayFacingUp = isRayFacingUp;
	rays[stripId].isRayFacingDown = isRayFacingDown;
	rays[stripId].isRayFacingLeft = isRayFacingLeft;
	rays[stripId].isRayFacingRight = isRayFacingRight;

}

void castAllRays() {
	float rayAngle = player.rotationAngle - (FOV_ANGLE / 2);

	for (int stripId = 0; stripId < NUM_RAYS; stripId++) {
		castRay(rayAngle, stripId);
		rayAngle += FOV_ANGLE / NUM_RAYS;
	}
}

void renderRays() {
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (int i = 0; i < NUM_RAYS; i++) {
		SDL_RenderDrawLine(
			renderer,
			MINIMAP_SCALE_FACTOR * player.x,
			MINIMAP_SCALE_FACTOR * player.y,
			MINIMAP_SCALE_FACTOR * rays[i].wallHitX,
			MINIMAP_SCALE_FACTOR * rays[i].wallHitY
		);
	}
}

void render3DProjectedWall() {
	for (int i = 0; i < NUM_RAYS; i++) {
		float rayDistance = rays[i].distance * cos(rays[i].rayAngle - player.rotationAngle);
		float distanceProjectionPlane = (WINDOW_WIDTH / 2) / (tan(FOV_ANGLE / 2));

		float wallStripHeight = (TILE_SIZE / rayDistance) * distanceProjectionPlane;

		int wallTopPixel = WINDOW_HEIGHT / 2 - (wallStripHeight / 2);
		wallTopPixel = wallTopPixel < 0 ? 0 : wallTopPixel;

		int wallBottomPixel = WINDOW_HEIGHT / 2 + (wallStripHeight / 2);
		wallBottomPixel = wallBottomPixel > WINDOW_HEIGHT ? WINDOW_HEIGHT : wallBottomPixel;

		for (int j = 0; j < wallTopPixel; j++) {
			colorBuffer[(WINDOW_WIDTH * j) + i] = 0xFF408DF7;
		}

		for (int j = wallTopPixel; j < wallBottomPixel; j++) {
			colorBuffer[(WINDOW_WIDTH * j) + i] = (rays[i].wasHitVertical) ? 0x636087 : 0xFF8686BB;
		}

		for (int j = wallBottomPixel; j < WINDOW_HEIGHT; j++) {
			colorBuffer[(WINDOW_WIDTH * j) + i] = 0xFF539457;
		}
	}
}

void clearColorBuffer(Uint32 color) {
	for (int x = 0; x < WINDOW_WIDTH; x++) {
		for (int y = 0; y < WINDOW_HEIGHT; y++) {
			colorBuffer[(WINDOW_WIDTH * y) + x] = color;
		}
	}
}

void renderColorBuffer() {
	SDL_UpdateTexture(
		colorBufferTexture,
		NULL,
		colorBuffer,
		(int)((Uint32)WINDOW_WIDTH * sizeof(Uint32))
	);
	SDL_RenderCopy(renderer, colorBufferTexture, NULL, NULL);
}

void update() {
	float timeToWait = FRAME_TIME_LENGHT - (SDL_GetTicks() - ticksLastFrame);
	if (timeToWait > 0 && timeToWait < FRAME_TIME_LENGHT)
		SDL_Delay(timeToWait);

	float deltaTime = (SDL_GetTicks() - ticksLastFrame) / 1000.0f;
	ticksLastFrame = SDL_GetTicks();

	movePlayer(deltaTime);
	castAllRays();
}

void render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	renderColorBuffer();
	clearColorBuffer(0xFFFFFFFF);
	render3DProjectedWall();

	renderMap();
	renderRays();
	renderPlayer();

	SDL_RenderPresent(renderer);
}

void destroyWindow() {
	free(colorBuffer);
	SDL_DestroyTexture(colorBufferTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int main(int argc, char* args[]) {
	isGameRunning = initializeWindow();

	setup();

	while (isGameRunning){
		processInput();
		update();
		render();
	}
	return 0;
}