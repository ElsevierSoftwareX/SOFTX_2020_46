/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#include "customterminal.hpp"
#include "core/utils/utils.hpp"

#include <cassert>

namespace {
const size_t MAX_HISTORY = 20;
const size_t MAX_BUFF = 255;
}

// NOTE markの位置によってバグる。markがnposの時に演算して不適切な値になっている箇所がある。
//      根本的に撲滅するにはmark, cposのようなカーソル位置を別クラスにして適時rangeチェックする。
/*
 * カーソルキーでヒストリ参照するには
 * 1．curserライブラリ
 *		環境に依存しにくいが外部ライブラリが必要
 *
 * 2．環境依存
 *		・Linux:sys/stat.h などをincludeして/dev/ttyを非同期読み取り(O_RDONLY|O_NONBLOCK))で経由してアクセス
 *				あるいはtermiosとか。http://qiita.com/Ki4mTaria/items/36feda5f6aa54643775a
 *		・Windows:keyboard関数等
 *
 * とりあえずLinux環境依存でやる。windows標準コンソールはヒストリ機能があるからほっておく。
 */

#if defined(__unix__) && !defined(ENABLE_GUI)
// pos1 と pos2の間のテキストに色をつける。
void DrawColorText(const std::string prompt, const std::string buff,
				std::string::size_type markpos, std::string::size_type cupos)
{
	if(markpos == std::string::npos) return;
	auto spos = std::min(markpos, cupos) - prompt.size();
	auto len = markpos > cupos ? markpos - cupos : cupos - markpos;

	std::cout << "\r" << prompt << buff.substr(0, spos)
//			  << "\033[1;31m" << buff.substr(spos,len) << "\033[0m"
			  << "\033[7m" << buff.substr(spos,len) << "\033[0m"
			  << buff.substr(spos + len);
	for(size_t i = 0; i < prompt.size() + buff.size()-cupos; ++i) std::cout << "\b";
}
// カーソルをcuposの位置にして再描画この時前回出力をprompt.size() + cleanSize文字消去する。
void RedrawConsole(const std::string &prompt, const std::string &buff,
					std::string::size_type cleanSize, std::string::size_type cursorPos)
{
	std::cout << "\r" << utils::spaces(prompt.size() + cleanSize);
	std::cout << "\r" << prompt << buff;
	assert(prompt.size() + buff.size() >= cursorPos);
	for(size_t i = 0; i < prompt.size() + buff.size()-cursorPos; ++i) std::cout << "\b";
}



class Console{
public:

	Console(const std::string &prompt)
        :prompt_(prompt), cursorPos_(prompt_.size()), markPos_(std::string::npos) {}
	std::string buffer() const {return buff_;}

	// 現在の内部状態でコンソール表示をアップデート(再描画)
	void update() {
		RedrawConsole(prompt_, buff_, buff_.size(), cursorPos_); // cposにカーソルを移動させて再描画
		DrawColorText(prompt_, buff_, markPos_, cursorPos_);  //markとcposの間を色つける
	}
	// posまでのコンソール表示を消去
//	void erase(std::string::size_type pos) {
//		std::cout << "\r" << utils::spaces(prompt_.size() + pos);
//	}
	// 現在のコンソール表示を消去する。
	void clear() {
		std::cout << "\r" << utils::spaces(prompt_.size() + buff_.size());
	}

	// 改行
	void lineFeed() {
		std::cout << std::endl;
		buff_.clear();
		cursorPos_ = prompt_.size();
		markPos_ = std::string::npos;

	}

	// 現在のカーソル位置をマーク
	void setMark() {
		assert(cursorPos_ != std::string::npos);
		markPos_ = cursorPos_;
	}
	// Ctrl-Aで先頭へ移動
	void moveCursorTop() {
		cursorPos_ = prompt_.size();
		update();
	}

	void moveCursorBack() {
		cursorPos_ = prompt_.size() + buff_.size();
		update();
	}

	void clearMark() {
		markPos_ = std::string::npos;
		update();
	}

	void killBuffer() {
		clear();
		clipBoard_ = buff_.substr(cursorPos_ - prompt_.size());

		if(cursorPos_< prompt_.size() + buff_.size()) {
			buff_ = buff_.erase(cursorPos_ - prompt_.size());  // cursor以降を削除
			markPos_ = std::string::npos; // mark-setはクリアする。
		}
		update();
	}

	// 現在のカーソル位置にclipBoard中身を貼り付け
	void yank(){
		buff_.insert(cursorPos_ - prompt_.size(), clipBoard_);
		markPos_ = std::string::npos;
		update();
	}

	void cutMarked() {
		if(markPos_ != std::string::npos) {
			// とりあえず一度コンソールは消す。面倒だから全部消す
			clear();

			std::string::size_type markStart = std::min(markPos_, cursorPos_) - prompt_.size();
			std::string::size_type len = markPos_ > cursorPos_ ? markPos_ - cursorPos_ : cursorPos_ - markPos_;
			clipBoard_ = buff_.substr(markStart, len);
			buff_ = buff_.substr(0, markStart) + buff_.substr(markStart + len);
			cursorPos_ = prompt_.size() + buff_.substr(0, markStart).size();
			markPos_ = std::string::npos;
			update();
		}
	}

	void backSpace() {
		if(cursorPos_ > prompt_.size()) {
			clear();
			if(cursorPos_ < markPos_ && markPos_ != std::string::npos) --markPos_;
			buff_ = buff_.erase(cursorPos_ - prompt_.size()-1, 1);
			cursorPos_ -= 1;
			update();
		}
	}


	void upKey(std::list<std::string>::iterator &itr, std::list<std::string> *historyList) {
		if(itr == historyList->begin()) return;  // 既にヒストリをさかのぼりきっている場合はリターン

		--itr;  // 一つ前のヒストリを参照
		clear();
		buff_ = *itr;
		cursorPos_ = prompt_.size() + buff_.size();
		markPos_ = std::string::npos;
		update();
	}
	void downKey(std::list<std::string>::iterator &itr, std::list<std::string> *historyList) {
		if(itr == historyList->end()) return;  // itrがヒストリ末端まで来ていたら何もせずリターンしないとout_of_rangeになる。
		++itr;
		if(itr == historyList->end()) {
			// ヒストリの次の要素が末端なら参照すべきヒストリが無いのでバッファーをクリアする。
			clear();
			buff_.clear();
			cursorPos_ = prompt_.size() + buff_.size();
			update();
			return;
		}
		clear();
		buff_ = *itr;
		cursorPos_ = prompt_.size() + buff_.size();
		markPos_ = std::string::npos;
		update();
	}

	void rightKey() {
		// カーソル右移動
		if(cursorPos_ < prompt_.size() + buff_.size()) {
			//std::cout << buff_.at(cursorPos_ - prompt_.size());
			cursorPos_ += 1;
		}
		update();
	}

	void leftKey() {
		// カーソル左移動
		if(cursorPos_ > prompt_.size()) {
			//std::cout << buff_.at(cursorPos_ - prompt_.size());
			cursorPos_ -= 1;
		}
		update();
	}
	void deleteKey() {
		if(cursorPos_ < prompt_.size() + buff_.size()) {
			clear();

			// markがカーソルより右にある場合はずらす。cursorとmarkが重なったらmarkは無効化
			if(cursorPos_ < markPos_ && markPos_ != std::string::npos) {
				--markPos_;
			}

			buff_ = buff_.erase(cursorPos_ - prompt_.size(), 1);
			// コンソール表示を更新
			update();
		}
	}

	void input(const char inp) {
		if(buff_.size() <= MAX_BUFF) buff_.insert(cursorPos_ - prompt_.size(), std::string{inp});
		++cursorPos_;
		update();
	}

	void back() {
		std::cout << "\033[80A";
		std::cout << "\033[2J"; // 画面クリア

		update();
	}

private:
	std::string prompt_;  // プロンプト文字列
	std::string buff_;		// 現在の入力文字列
	std::string prevBuffer_;  // 前回のbuffer
	std::string clipBoard_;  // クリップボード
	std::string::size_type cursorPos_;  // カーソル位置
	std::string::size_type markPos_;  // 選択開始位置
	// ConsoleはENTER後の次のコマンド入力までデータを保持できないので
	// ヒストリ管理はしない。
};


class EscInput {
public:
	EscInput():hasReceived_(false) {;}
	bool hasReceived() const {return hasReceived_;}
	void pushChar(const char inp) { inputData.push_back(inp);}
	size_t count() const {return inputData.size();}
	void clear() {
		*this = EscInput();
//		hasReceived_ = false;
//		inputData.clear();
	}
	void setReceivedFlag(bool f) {hasReceived_ = f;}
	const std::vector<char>& data() const {return inputData;}
	bool isDelKey() {
		if(inputData.size() != 3) {
			return false;
		} else {
			return inputData.at(0) == 0x5b
					&& inputData.at(1) == 0x33
					&& inputData.at(2) == 0x7e;
		}
	}

private:
	bool hasReceived_;
	std::vector<char> inputData;
};
#endif

term::CustomTerminal::CustomTerminal(const std::string &prompt)
	:prompt_(prompt)
{
#if defined(__unix__) && !defined(ENABLE_GUI)
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	save_ = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);  // エコーバックとカノニカルモードを停止してセット
#endif
}


std::istream &term::CustomTerminal::customGetline(std::istream &ist, std::string &buff)
{
	buff.clear();
	std::cout << prompt_;

#if defined(__unix__) && !defined(ENABLE_GUI)

	// http://www.mm2d.net/main/prog/c/console-05.html 参考
	EscInput esc;
	Console console(prompt_);
	auto historyItr = history_.end();

	while(true) {

		int input = fgetc(stdin);
		//std::cerr << "INPUTED=" << std::string{static_cast<char>(tmp)} << std::endl;
		//std::cerr << "INPUT=" << tmp;

		/*
		 * キーボードの
		 * Deleteキーは 0x1b 0x5b 0x33 0x7e
		 *              27   91   51   126
		 * Back spaceキーは 0x7f
		 *
		 * カーソルキー右は 0x1b 0x5b 0x41
		 *
		 */


		// リターンかエラーなら入力終了なのでbreak
		if (input < 0 || input == 0x0A) {
			buff = console.buffer();
			console.lineFeed();

			// buffが空白ではなく、最後のヒストリと異なっていれば最後の入力値をヒストリの一番後ろに追加する。
			if(buff.find_first_not_of(" \t") != std::string::npos && (history_.empty() || buff != history_.back())) {
				history_.push_back(buff);
			}
			if(history_.size() > MAX_HISTORY) history_.pop_front();
			break;

		} else if (input == 0x1B) {
			// ESCが来たらフラグを立てる。
			esc.setReceivedFlag(true);

		} else if (input == 0) {
			// Ctrl-space, mark-set
			console.setMark(); // 現在位置をmark

		} else if (input == 1) {
			// Ctrl-a  先頭へ移動
			console.moveCursorTop();

		} else if (input == 5) {
			// Ctrl-e カーソルを末尾へ移動
			console.moveCursorBack();

		} else if (input == 7) {
			// Ctrl-g mark解除
			console.clearMark();

		}else if (input == 11) {
			// Ctrl-k  カーソル以降を削除してクリップボードへ
			console.killBuffer();
			// デリートキー処理を行末まで。

		} else if(input == 12) {
			//Ctrl-l
			console.back();

		} else if (input == 23) {
			// Ctrl-w
			console.cutMarked();

		} else if (input == 25) {
			// Ctrl-y 貼り付け。
			console.yank();

		} else if  (input == 0x7f) { // 127 0x7f
			// backspace. BackSpaceキーを押すと ASCIIでのDelete 0x7fが来る。
			console.backSpace();

		} else if (esc.hasReceived()) {
			// ESC受信状態なら後続文字最大3文字は別のバッファに入れる。
			esc.pushChar(input);
			// 2文字読んだらカーソルキーを判定し、カーソルキーならESCフラグは解除、カウントリセットする。
			if(esc.count() == 2) {
				// ここまできたらtmp2にESC後の2文字が入っていることになる。
				if(esc.data().at(0) == 0x5b) {
					switch(esc.data().at(1)) {

					// 矢印上キーは 0x1b(ESC), 0x5b, 0x41の3連発入力になる。
					case 0x41:  // ↑
						esc.clear();
						console.upKey(historyItr, &history_);
						break;

					case 0x42:  // ↓
						esc.clear();
						console.downKey(historyItr, &history_);
						break;


					case 0x43:  // →
						esc.clear();
						console.rightKey();
						break;

					case 0x44:  // ←
						esc.clear();
						console.leftKey();
						break;

					default:
						break;
					}
				}
			} else if (esc.count() == 3) {
				// デリートキー処理 0x1b(ESC) 0x5b 0x33 0x7eの4連発
				if(esc.isDelKey()) {
					esc.clear();
					console.deleteKey();

				}
				// ESCのあと3文字来たらそこでおしまい。
				esc.clear();
			}

		} else if (!iscntrl(input)  && !esc.hasReceived()) {
			// コントロールシーケンス以外かつESC受信状態ではない。ならバッファに保存
			console.input(static_cast<char>(input));
		}
	}

#else
	std::getline(ist, buff);
#endif

	return ist;
}


