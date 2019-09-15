#include "StoryMode.hpp"

#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "gl_errors.hpp"
#include "Sound.hpp"
#include <sstream>

Sprite const *minesweeper_block = nullptr;
Sprite const *minesweeper_chosen_block = nullptr;
Sprite const *minesweeper_red_flag = nullptr;
Sprite const *minesweeper_mine = nullptr;
Sprite const *minesweeper_sweeper = nullptr;

enum block_type {
    mine_hidden, mine_marked, mine_exploded, blank_hidden, blank_marked, blank_wrong_marked
};
const int matrix_size = 6;
std::vector<std::vector<block_type>>
    matrix(matrix_size, std::vector<block_type>(matrix_size, blank_hidden));
int mouse_x = 0, mouse_y = 0;
int mine_num = 10, wrong_marked = 0;
float time_elapsed = 0.;
bool is_finished = false;
bool firstTime = true;

Load< SpriteAtlas > sprites(LoadTagDefault, []() -> SpriteAtlas const * {
	SpriteAtlas const *ret = new SpriteAtlas(data_path("sound-minesweeper"));

    minesweeper_block = &ret->lookup("block");
    minesweeper_chosen_block = &ret->lookup("chosen-block");

    minesweeper_red_flag = &ret->lookup("red-flag");
    minesweeper_mine = &ret->lookup("mine");
    minesweeper_sweeper = &ret->lookup("sweeper");

	return ret;
});

Load< Sound::Sample > music_sand(LoadTagDefault, []() -> Sound::Sample * {
	return new Sound::Sample(data_path("sand.opus"));
});

StoryMode::StoryMode() {
}

StoryMode::~StoryMode() {
}

bool StoryMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_r) {
            firstTime = true;
            return false;
        }
    }

    if (evt.type == SDL_MOUSEMOTION) {
        mouse_x = evt.motion.x;
        mouse_y = view_max.y - evt.motion.y;
    }

    if (evt.type == SDL_MOUSEBUTTONUP && !is_finished) {
        int offset_x = evt.button.x - 42, offset_y = view_max.y - evt.button.y - 42, bx, by;
        if (offset_x / 53 < matrix_size && offset_x % 53 < 47 && offset_x % 53 > 3 &&
            offset_y / 53 < matrix_size && offset_y % 53 < 47 && offset_y % 53 > 3) {
            bx = offset_x / 53;
            by = offset_y / 53;
        } else {
            return false;
        }

        if (evt.button.button == SDL_BUTTON_LEFT) {
            if (matrix[bx][by] == mine_hidden) {
                is_finished = true;
                for (unsigned i = 0; i < matrix.size(); ++i) {
                    for (unsigned j = 0; j < matrix[i].size(); ++j) {
                        if (matrix[i][j] == mine_hidden) {
                            matrix[i][j] = mine_exploded;
                        } else if (matrix[i][j] == blank_hidden || matrix[i][j] == blank_wrong_marked) {
                            matrix[i][j] = blank_marked;
                        }
                    }
                }
            } else if (matrix[bx][by] == blank_hidden) {
                matrix[bx][by] = blank_marked;
            }
        } else if (evt.button.button == SDL_BUTTON_RIGHT) {
            if (matrix[bx][by] == mine_hidden) {
                matrix[bx][by] = mine_marked;
                mine_num--;
            } else if (matrix[bx][by] == mine_marked) {
                matrix[bx][by] = mine_hidden;
                mine_num++;
            } else if (matrix[bx][by] == blank_hidden) {
                matrix[bx][by] = blank_wrong_marked;
                wrong_marked++;
            } else if (matrix[bx][by] == blank_wrong_marked) {
                matrix[bx][by] = blank_hidden;
                wrong_marked--;
            }
        }

        if (!mine_num && !wrong_marked) {
            is_finished = true;
        }
    }
	return false;
}

void StoryMode::update(float elapsed) {
    if (firstTime) {
        firstTime = false;

        time_elapsed = 0.f;
        mine_num = 10;
        wrong_marked = 0;
        is_finished = false;

        //sand storm
        if (!background_music) {
            background_music = Sound::play(*music_sand, 1.f, 0.0f, true);
        }


        //mine detector
        if (!mine_sound) {
            std::vector< float > data(size_t(48000), 0.0f);
            for (uint32_t i = 0; i < data.size(); ++i) {
                float t = i / float(48000);
                //phase-modulated sine wave (creates some metal-like sound):
                data[i] = std::sin(3.1415926f * 2.0f * 220.0f * t + std::sin(3.1415926f * 2.0f * 200.0f * t));
                //sin wave modulation, feel like unstable:
                data[i] *= (2 + 0.4 * std::sin(3.1415926f * 2.0f * 1.0f * t));
            }
            mine_sound = Sound::play(*new Sound::Sample(data), 1.0f, 0.0f, true);
        }

        for (unsigned i = 0; i < matrix.size(); ++i) {
            for (unsigned j = 0; j < matrix[i].size(); ++j) {
                matrix[i][j] = blank_hidden;
            }
        }

        // random mine position
        int total = matrix_size * matrix_size, pos;
        srand(time(NULL));
        for (int i = 0; i < mine_num; ++i) {
            pos = rand() % total;
            if (matrix[pos/matrix_size][pos%matrix_size] == mine_hidden) {
                --i;
            } else {
                matrix[pos/matrix_size][pos%matrix_size] = mine_hidden;
            }
        }
	}

    // volume up when close to mine
    float mine_volume = 0.f;
    for (unsigned i = 0; i < matrix.size(); ++i) {
        for (unsigned j = 0; j < matrix[i].size(); ++j) {
            if (matrix[i][j] == mine_hidden) {
                float x = 67.f + i * 53.f, y = 67.f + j * 53.f;
                mine_volume += std::exp(-((x - mouse_x) * (x - mouse_x) + (y - mouse_y) * (y - mouse_y)) / 1600.f);
            }
        }
    }
    mine_sound->set_volume(mine_volume);

    if (!is_finished) {
        time_elapsed += elapsed;
    }
}

void StoryMode::draw(glm::uvec2 const &drawable_size) {
	//clear the color buffer:
	glClearColor(1.0f, 0.9725f, 0.8827f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

    std::ostringstream stringStream;
    stringStream.precision(1);

	{ //use a DrawSprites to do the drawing:
		DrawSprites draw(*sprites, view_min, view_max, drawable_size, DrawSprites::AlignSloppy);
        glm::vec2 base = glm::vec2(42.0f, 42.0f);

        stringStream << "Time: " << std::fixed << time_elapsed << "s";
        draw.draw_text(
                stringStream.str(), glm::vec2(45.0f, 412.0f), 2.0, glm::u8vec4(0x00, 0x00, 0x00, 0xff)
        );

        stringStream.str("");
        stringStream << "Mines: " << mine_num;
        draw.draw_text(
                stringStream.str(), glm::vec2(220.0f, 412.0f), 2.0, glm::u8vec4(0x00, 0x00, 0x00, 0xff)
        );

        if (is_finished) {
            if (mine_num == 0) {
                draw.draw_text(
                        "You WON!!", glm::vec2(140.0f, 450.0f), 2.0, glm::u8vec4(0xff, 0x00, 0x00, 0xff)
                );
            } else {
                draw.draw_text(
                        "You Lost...", glm::vec2(140.0f, 450.0f), 2.0, glm::u8vec4(0x00, 0x00, 0x00, 0xff)
                );
            }
        }

		for (unsigned i = 0; i < matrix.size(); ++i) {
		    for (unsigned j = 0; j < matrix[i].size(); ++j) {
		        switch (matrix[i][j]) {
		            case blank_hidden:
                    case mine_hidden:
                        draw.draw(*minesweeper_block, base + glm::vec2(53.0f * i, 53.0f * j));
                        break;
		            case mine_marked:
		            case blank_wrong_marked:
                        draw.draw(*minesweeper_red_flag, base + glm::vec2(53.0f * i, 53.0f * j));
                        break;
                    case mine_exploded:
                        draw.draw(*minesweeper_mine, base + glm::vec2(53.0f * i, 53.0f * j));
                        break;
		            case blank_marked:
                    default:
                        break;
		        }

		    }
		}

		int offset_x = mouse_x - base.x, offset_y = mouse_y - base.y, bx, by;
		if (offset_x / 53 < matrix_size && offset_x % 53 < 47 && offset_x % 53 > 3) {
            bx = offset_x / 53;
            if (offset_y / 53 < matrix_size && offset_y % 53 < 47 && offset_y % 53 > 3) {
                by = offset_y / 53;
                if (matrix[bx][by] == blank_hidden || matrix[bx][by] == mine_hidden) {
                    draw.draw(*minesweeper_chosen_block, base + glm::vec2(53.0f * bx, 53.0f * by));
                }
            }
		}

        draw.draw(*minesweeper_sweeper, glm::vec2(mouse_x - 20.0f, mouse_y - 13.0f));
        draw.draw_text(
                "press 'r' to restart", glm::vec2(268.0f, 7.0f), 1.0, glm::u8vec4(0x00, 0x00, 0x00, 0xff)
        );
	}
	GL_ERRORS(); //did the DrawSprites do something wrong?
}
