#include<stdio.h>
#include<tchar.h>
#include<graphics.h>
#include<string>
#include<vector>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;
#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"winmm.lib")

bool is_game_started = false;
bool running = true;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}
class Atlas {
public:
	Atlas(LPCTSTR path, int num) {
		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++) {
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}
	~Atlas() {
		for (size_t i = 0; i < frame_list.size(); i++) delete frame_list[i];
	}
public:
	std::vector<IMAGE*> frame_list;
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;

class Animation {
public:
	Animation(Atlas* atlas, int interval) {
		anim_atlas = atlas;
		interval_ms = interval;
	}
	~Animation() = default;
	void Play(int x, int y, int delta/*图片刷新间隔*/) {
		timer += delta;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}
private:
	int timer = 0;
	int idx_frame = 0;
	int interval_ms = 0;
private:
	Atlas* anim_atlas;
};

class Button {
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) {
		region = rect;

		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	}

	~Button() = default;

public:
	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message) {
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}
	void Draw() {
		switch (status) {
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;

		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;

		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}
private:
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}
protected:
	virtual void OnClick() = 0;
private:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};
private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
};

class StartGameButton :public Button {
public:StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	  ~StartGameButton() = default;
protected:
	void OnClick() {
		is_game_started = true;
	}
};
class QuitGameButton :public Button {
public:QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	  ~QuitGameButton() = default;
protected:
	void OnClick() {
		running = false;
	}
};

class Player {
public:
	Player() {
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left, 45);
		anim_right = new Animation(atlas_player_right, 45);
	}
	~Player() {
		delete anim_left;
		delete anim_right;
	}
public:
	const POINT& GetPosition() const { return position; }
	void ProcessEvent(const ExMessage& msg) {
		if (msg.message == WM_KEYDOWN) {
			switch (msg.vkcode) {
			case VK_UP:
				is_move_up = true;
				break;
			case VK_DOWN:
				is_move_down = true;
				break;
			case VK_LEFT:
				is_move_left = true;
				break;
			case VK_RIGHT:
				is_move_right = true;
				break;
			}
		}
		else if (msg.message == WM_KEYUP) {
			switch (msg.vkcode) {
			case VK_UP:
				is_move_up = false;
				break;
			case VK_DOWN:
				is_move_down = false;
				break;
			case VK_LEFT:
				is_move_left = false;
				break;
			case VK_RIGHT:
				is_move_right = false;
				break;
			}
		}
	}
	void Move() {
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
		if (position.x < 0) position.x = 0;
		if (position.y < 0) position.y = 0;
		if (position.x + FRAME_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - FRAME_WIDTH;
		if (position.y + FRAME_HEIGHT > WINDOW_HEIGHT) position.y = WINDOW_HEIGHT - FRAME_HEIGHT;
	}
	void Draw(int delta) {
		int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = position.y + FRAME_HEIGHT - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		static bool facing_left = false;
		int dir_x = is_move_right - is_move_left;

		if (dir_x < 0) facing_left = true;
		else if (dir_x > 0) facing_left = false;

		if (facing_left) anim_left->Play(position.x, position.y, delta);
		else anim_right->Play(position.x, position.y, delta);
	}
public:
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;
private:
	const int PLAYER_ANIM_NUM = 6;
	const int SPEED = 3;
	const int SHADOW_WIDTH = 32;
private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 500,500 };
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
};

class Bullet {
public:
	POINT position = { 0,0 };
public:
	Bullet() = default;
	~Bullet() = default;
public:
	void Draw() const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10;
};

class Enemy {
public:
	Enemy() {
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_right, 45);

		//出生的边界位置
		enum class SpawnEdge {
			Up = 0,
			Down,
			Left,
			Right
		};
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge) {
		case SpawnEdge::Up:
			position.x = rand() % WINDOW_WIDTH;
			position.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down:
			position.x = rand() % WINDOW_WIDTH;
			position.y = FRAME_HEIGHT;
			break;
		case SpawnEdge::Left:
			position.x = -FRAME_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			position.x = FRAME_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}
	~Enemy() {
		delete anim_left;
		delete anim_right;
	}
public:
	bool CheckBulletCollision(const Bullet& bullet) {
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}
	bool CheckPlayerCollision(const Player& player) {
		POINT check_position = { position.x + FRAME_WIDTH / 2,position.y + FRAME_HEIGHT / 2 };
		bool is_overlap_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.FRAME_WIDTH;
		bool is_overlap_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y + player.FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}
	void Move(const Player& player) {
		const POINT& player_position = player.GetPosition();
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
		if (dir_x < 0) facing_left = true;
		else if (dir_x > 0) facing_left = false;
	}
	void GetHurted() {
		alive = false;
	}
	bool CheckAlive() {
		return alive;
	}
	void Draw(int delta) {
		int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = position.y + FRAME_HEIGHT - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		if (facing_left) anim_left->Play(position.x, position.y, delta);
		else anim_right->Play(position.x, position.y, delta);
	}

private:
	const int ENEMY_ANIM_NUM = 6;
	const int SPEED = 2;
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;
	const int SHADOW_WIDTH = 48;
private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0,0 };
	bool facing_left = false;
};

void TryGenerateEnemy(std::vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) enemy_list.push_back(new Enemy());
}
void UpdateBullet(std::vector<Bullet>& bullet_list, const Player& player) {
	const double RADIAL_SPEED = 0.0045;//径向速度
	const double TANGENT_SPEED = 0.0055;//切向速度
	double radian_interval = 2 * 3.14159 / bullet_list.size();//子弹之间弧度制间隔
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);//子弹当前的环绕半径
	for (size_t i = 0; i < bullet_list.size(); i++) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//子弹当前所在弧度
		bullet_list[i].position.x = player_position.x + player.FRAME_WIDTH / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.FRAME_HEIGHT / 2 + (int)(radius * cos(radian));
	}
}
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分：%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

int main() {
	initgraph(1280, 720);

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);

	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 6);

	RECT region_btn_start_game, region_btn_quit_game;
	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;
	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	IMAGE img_background;
	IMAGE img_menu;
	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_background, _T("img/background.png"));

	Player player;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullet> bullet_list(3);

	int score = 0;

	ExMessage msg;

	BeginBatchDraw();
	while (running) {
		DWORD start_time = GetTickCount();

		//消息处理
		while (peekmessage(&msg)) {
			if (is_game_started) {
				player.ProcessEvent(msg);
			}
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		//逻辑部分
		if (is_game_started) {
			//玩家
			player.Move();
			//子弹
			UpdateBullet(bullet_list, player);

			TryGenerateEnemy(enemy_list);
			for (Enemy* enemy : enemy_list) enemy->Move(player);
			for (Enemy* enemy : enemy_list) {
				if (enemy->CheckPlayerCollision(player)) {
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分：%d ！"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}
			for (Enemy* enemy : enemy_list) {
				for (const Bullet& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						enemy->GetHurted();
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
					}
				}
			}
			for (size_t i = 0; i < enemy_list.size(); i++) {
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					score++;
					delete enemy;
				}
			}
		}

		//渲染画面部分
		cleardevice();

		if (is_game_started) {
			putimage(0, 0, &img_background);

			player.Draw(1000 / 144);

			for (Bullet bullet : bullet_list) bullet.Draw();

			for (Enemy* enemy : enemy_list) enemy->Draw(1000 / 144);

			DrawPlayerScore(score);
		}

		else {
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}

		FlushBatchDraw();//从缓冲区中读取画面并输出

		//当前帧渲染结束
		DWORD end_time = GetTickCount();
		DWORD delta_time = start_time - end_time;
		if (delta_time < 1000 / 144) {
			Sleep(1000 / 144 - delta_time);
		}
	
	}

	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;

	EndBatchDraw();
	return 0;
}