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
#ifndef DEBUG_UTILS_HPP
#define DEBUG_UTILS_HPP

#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "stream_utils.hpp"


enum class OUTPUT_TYPE {MFATAL, MWARNING, MDEBUG};

#ifdef ENABLE_GUI
#include <QObject>
#include <QString>
#include <QSharedPointer>

class LogForwarder: public QObject
{
	Q_OBJECT
public:
	LogForwarder();
    void forwardLog(const std::string &logStr, OUTPUT_TYPE level) const;
    void enableLogFile(const std::string &fileName);
    void disableLogFile();
signals:
//    void fatalMessage(const QString &fatalMessage) const;
//	void logMessage(const QString &logSgr) const;
//	void fatalMessage(QSharedPointer<QString> fatalMessage) const;
//	void logMessage(QSharedPointer<QString> logSgr) const;
	void fatalMessage(QSharedPointer<std::string> fatalMessage) const;
	void logMessage(QSharedPointer<std::string> logSgr) const;

public:
    std::unique_ptr<std::ofstream> ofs_;
};
namespace global {

// ここでGUIモードならグローバルなLogForwarderを作成する。
#ifdef DEFINE_GLOBAL_
	LogForwarder logFwd;   
#else
    extern LogForwarder logFwd;
#endif
#undef DEFINE_GLOBAL_

}  // end namespace global

#endif  // ENABLE_GUIおわり



/*
 * 可変長引数を使って文字列化しつつ連結したい。
 * ・コンストラクタで型を問わず可変長の変数をとり、それらを連結して出力する。
 * ・operator<<でもメッセージを入力できる。
 * ・mFatalの場合はコンストラクタの終わりで例外を投げる(GUI)かexit(CUI)する。
 */
template <OUTPUT_TYPE OTYPE>
class MessengerBase {
public:
	// コンストラクタ
	template<class... Args>
	MessengerBase(Args... args) : message_(conc(args...)), spacing_(true)
	{
		static const std::unordered_map<OUTPUT_TYPE, std::string> kind{
			{OUTPUT_TYPE::MFATAL, "Fatal: "},
			{OUTPUT_TYPE::MWARNING, "Warning: "},
			{OUTPUT_TYPE::MDEBUG, ""}
		};

#ifdef ENABLE_GUI
		fwd_ = &global::logFwd;
#endif
        ss_ << kind.at(OTYPE) << message_;
		// fatalの場合はコンストラクタでメッセージ出力し、最後に例外を投げる(GUI)か終了する(CUI)。
        if(OTYPE == OUTPUT_TYPE::MFATAL) {
			std::string total_message = ss_.str();
			std::cerr << total_message << std::endl;

			// GUI実行時にはログ転送、警告ダイアログ、例外送出をする。CUIでは即終了する。
#ifdef ENABLE_GUI
			if(fwd_) fwd_->forwardLog(total_message, OTYPE);
	#ifdef FATAL_ABORT
			abort();
	#else
			// GUIの場合runtime_errorを投げて、これはMainWindow::readSimulationObjectFromFile内ではキャッチされる。
			//std::cerr << "throw runtime_error from tID===" << std::this_thread::get_id();
			throw std::runtime_error(total_message);
	#endif
#else
			std::exit(EXIT_FAILURE);
#endif
        }  // end if MFatal
	}

	// デストラクタ
	~MessengerBase()
	{
		message_ = ss_.str();

		switch(OTYPE)
		{
        case OUTPUT_TYPE::MFATAL:
			assert(ss_.str().empty());  // fatalの場合は<<でメッセージ入力されていないはずなのでチェック
			break;
        case OUTPUT_TYPE::MWARNING:
            std::cerr << message_ << std::endl;
            // GUIではエラー出力に加えてlog転送もする。warningはダイアログは出さないし例外も出さない
            #ifdef ENABLE_GUI
                if(fwd_) fwd_->forwardLog(message_, OTYPE);
            #endif

			break;
        case OUTPUT_TYPE::MDEBUG:
#ifndef NO_DEBUG_OUT
            std::cout << ss_.str() << std::endl;
    #ifdef ENABLE_GUI
            if(fwd_) fwd_->forwardLog(message_, OTYPE);
    #endif
#endif  // end ifndef NO_DEBUG_OUT
            break;
		default:
            std::cerr << "This case is wrong" << std::endl;
            abort();
		}
	}

	template <class T>
	MessengerBase& operator<<(T&& val) {
		this->output(val);
		return *this;
	}
//	operator<<(const T &val)は↑のユニバーサル参照に包含されるので不要

	MessengerBase& setSpacing(bool spacingFlag) {
		spacing_ = spacingFlag;
		return *this;
	}

private:
	std::string message_; // コンストラクタ引数を連結したメッセージ
	std::stringstream ss_; // operator<<をためておくストリーム
	bool spacing_;
//    static const std::unordered_map<OUTPUT_TYPE, std::string> kind;
#ifdef ENABLE_GUI
	const LogForwarder* fwd_;
#endif

	template <class T>
	void output(T val) {
		// 文字列の中身を吟味してスペースを入れる・入れない判断するので、先に一度sstを使って文字列化する。
		std::stringstream sst;
		if(std::is_same<T, bool>::value) {
			sst << std::boolalpha << val;
		} else {
			sst << val;
		}
		std::string valStr = sst.str();
		// 自動的にスペースを入れるが引用符の前後やピリオドの前にはスペースを入れない。
		if(spacing_ && !valStr.empty() && valStr.front() != '\"' && valStr.front() != '.') {
			if(!ss_.str().empty() && ss_.str().back() != ' ' && ss_.str().back() != '\"') {
				ss_ << " ";
			}
		}
		ss_ << valStr;
	}

	// 連結用関数
	// 引数なしコンストラクタの時の引数連結用ダミー
	static std::string conc() {
		return "";
	}
	// 連結の末端
	template<class T> static std::string conc(T v) {
		std::stringstream ss;
		ss << v;
		return ss.str();
	}
	// 再帰連結関数
	template<class T, class... Args> static std::string conc(T v, Args... args)
	{
		std::stringstream ss;
		ss << v;
		return ss.str() + " " + conc(args...);
	}
};



using mFatal = MessengerBase<OUTPUT_TYPE::MFATAL>;
using mWarning = MessengerBase<OUTPUT_TYPE::MWARNING>;
using mDebug = MessengerBase<OUTPUT_TYPE::MDEBUG>;









// ファイル書き込みとかもできるようにしたい。包括的なログ管理を考える
// グローバル空間に標準出力、標準エラー出力を開いておいて適時そこへ出力すれば良い
//namespace global {
//#ifdef DEFINE_GLOBAL_
//	std::ofstream stdofs("stdout.txt");
//	std::ofstream errofs("stderr.txt");
//#else
//	extern std::ofstream stdofs;
//	extern std::ofstream errofs;
//#endif
//#undef DEFINE_GLOBAL_
//}



#ifdef ENABLE_GUI
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QSize>
#include <QTreeWidgetItem>
std::ostream &operator <<(std::ostream &os, const QPoint &qpt);
std::ostream &operator <<(std::ostream &os, const QString &qstr);
std::ostream &operator <<(std::ostream &os, const QSize &sz);
std::ostream &operator <<(std::ostream &os, const QByteArray& ba);
std::ostream &operator <<(std::ostream &os, const QStringList& qstrlist);
std::ostream &operator <<(std::ostream &os, Qt::CheckState state);
#endif


#endif // DEBUG_UTILS_HPP
