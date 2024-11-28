#include <iostream>
#include <sstream>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <queue>
#include <assert.h>
#include <algorithm>
#include <functional>
#include <math.h>
#include <utility>

struct Coord {
	int y_;
	int x_;
	Coord(const int y = 0, const int x = 0) :y_(y), x_(x) {}
};

std::mt19937 mt_for_action(0);

constexpr const int H = 5;
constexpr const int W = 5;
constexpr int END_TURN = 5;
constexpr int CHARACTER_N = 3;

using ScoreType = int64_t;
constexpr const ScoreType INF = 1000000000LL;

class AutoMoveMazeState {
private:
	static constexpr const int dx[4] = { 1, -1, 0, 0 };
	static constexpr const int dy[4] = { 0, 0, 1, -1 };
	int points_[H][W] = {};
	int turn_;
	Coord characters_[CHARACTER_N] = {};
	void movePlayer(const int character_id) {
		Coord& character = this->characters_[character_id];
		int best_point = -INF;
		int best_action_index = 0;
		for (int action = 0; action < 4; action++) {
			int ty = character.y_ + dy[action];
			int tx = character.x_ + dx[action];
			if (ty >= 0 && ty < H && tx >= 0 && tx < W) {
				auto point = this->points_[ty][tx];
				if (point > best_point) {
					best_point = point;
					best_action_index = action;
				}
			}
		}
		character.y_ += dy[best_action_index];
		character.x_ += dx[best_action_index];
	}
	void advance() {
		for (int character_id = 0; character_id < CHARACTER_N; character_id++)
			movePlayer(character_id);
		for (auto& character : this->characters_) {
			auto& point = this->points_[character.y_][character.x_];
			this->game_score_ += point;
			point = 0;
		}
		++this->turn_;
	}
public:
	int game_score_;
	ScoreType evaluated_score_;
	AutoMoveMazeState(const int seed) :turn_(0), game_score_(0), evaluated_score_(0) {
		auto mt_for_construct = std::mt19937(seed);
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++)
				points_[y][x] = mt_for_construct() % 9 + 1;
	}
	void setCharacter(const int character_id, const int y, const int x) {
		this->characters_[character_id].y_ = y;
		this->characters_[character_id].x_ = x;
	}
	bool isDone() const {
		return this->turn_ == END_TURN;
	}
	std::string toString() const {
		std::stringstream ss;
		ss << "turn:\t" << this->turn_ << "\n";
		ss << "score:\t" << this->game_score_ << "\n";
		auto board_chars = std::vector<std::vector<char>>(H, std::vector<char>(W, '.'));
		for (int h = 0; h < H; h++) {
			for (int w = 0; w < W; w++) {
				bool is_written = false;
				for (const auto& character : this->characters_) {
					if (character.y_ == h && character.x_ == w) {
						ss << "@";
						is_written = true;
						break;
					}
					board_chars[character.y_][character.x_] = '@';
				}
				if (!is_written) {
					if (this->points_[h][w] > 0)
						ss << points_[h][w];
					else
						ss << '.';
				}
			}
			ss << "\n";
		}
		return ss.str();
	}
	ScoreType getScore(bool is_print = false)const {
		auto tmp_state = *this;
		for (auto& character : this->characters_) {
			auto& point = tmp_state.points_[character.y_][character.x_];
			point = 0;
		}
		while (!tmp_state.isDone()) {
			tmp_state.advance();
			if (is_print)
				std::cout << tmp_state.toString() << std::endl;
		}
		return tmp_state.game_score_;
	}
	void init() {
		for (auto& character : this->characters_) {
			character.y_ = mt_for_action() % H;
			character.x_ = mt_for_action() % W;
		}
	}
	void transition() {
		auto& character = this->characters_[mt_for_action() % CHARACTER_N];
		character.y_ = mt_for_action() % H;
		character.x_ = mt_for_action() % W;
	}
};

using State = AutoMoveMazeState;

using AIFunction = std::function<State(const State&)>;
using StringAIPair = std::pair<std::string, AIFunction>;

State hillClimb(const State& state, int number)
{
	State now_state = state;
	now_state.init();
	ScoreType best_score = now_state.getScore();
	for (int i = 0; i < number; i++)
	{
		auto next_state = now_state;
		next_state.transition();
		auto next_score = next_state.getScore();
		if (next_score > best_score)
		{
			best_score = next_score;
			now_state = next_state;
		}
	}
	return now_state;
}

State simulatedAnnealing(const State& state, int number, double start_temp, double end_temp) {
	State now_state = state;
	now_state.init();
	ScoreType best_score = now_state.getScore();
	ScoreType now_score = best_score;
	auto best_state = now_state;

	for (int i = 0; i < number; i++) {
		auto next_state = now_state;
		next_state.transition();
		auto next_score = next_state.getScore();
		double temp = start_temp + (end_temp - start_temp) * (i / number);
		double probability = exp((next_score - now_score) / temp);
		bool is_force_next = probability > (mt_for_action() % INF) / (double)INF;
		if (next_score > now_score || is_force_next) {
			now_score = next_score;
			now_state = next_state;
		}
		if (next_score > best_score) {
			best_score = next_score;
			best_state = next_state;
		}
	}
	return best_state;
}

void testAiScore(const StringAIPair& ai, const int game_number) {
	using std::cout;
	using std::endl;
	std::mt19937 mt_for_construct(0);
	double score_mean = 0;
	for (int i = 0; i < game_number; i++) {
		auto state = State(mt_for_construct());
		state = ai.second(state);
		auto score = state.getScore(false);
		score_mean += score;
	}
	score_mean /= (double)game_number;
	cout << "Score of " << ai.first << ":\t" << score_mean << endl;
}

void playGame(const StringAIPair& ai, const int seed) {
	using std::cout;
	using std::endl;
	auto state = State(seed);
	state = ai.second(state);
	cout << state.toString() << endl;
	auto score = state.getScore(true);
	cout << "Score of " << ai.first << ": " << score << endl;
}

int main() {
	int simulate_number = 10000;
	const std::vector<StringAIPair> ais = {
		StringAIPair("hillClimb", [&](const State& state) {return hillClimb(state, simulate_number); }),
		StringAIPair("simulatedAnnealing", [&](const State& state) {return simulatedAnnealing(state, simulate_number, 500, 10); })
	};
	int game_number = 1000;
	for (const auto& ai : ais)
		testAiScore(ai, game_number);

	return 0;
}