
TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

#CONFIG += object_parallel_to_source

#FIXME ELLの解釈ミス
#FIXME phits-TRの最後の入力M=２への対応



TARGET = gxsview.core
target.path = ../tests/bin/
INSTALLS += target

DEFINES += DIRECTUSE_LIBACEXS
# 拡張子inpファイルのモード判定は基本的にはAUTOとする。
DEFINES += INP_AS_PHITS # .inpをphits入力として扱う
#DEFINES += INP_AS_MCNP # .inpをmcnp入力として扱う


INCLUDEPATH += ../

unix {
QMAKE_CXXFLAGS += -ansi -pedantic
LIBS += -lpthread
#QMAKE_CXX = clang++
}

# vc++では/utf-8つけないとエラーになる。
win32{
    mingw{
        # MinGW
        COMPILER=mingw
    } else {
        # VC++
        COMPILER=msvc
# c4503:変数名の長さ　c4267:size_tから整数への変換
		QMAKE_CXXFLAGS += /utf-8 /wd4503 /wd4267
    }
}

#release:message(Rrelease)
#debug:message(Debug)
CONFIG(debug,debug|release) {
  #DEFINES += ENABLE_SHELL_COMMAND
}


# google プロファイラ
# -lprofilerにリンクして実行その後google-pprofで適当なフォーマットに変換
#
# 参考DOT http://qiita.com/kmikmy/items/672efc5e2cde4826bbc7
# google-pprof  ../../acdev prof.out --dot > prof.dot
# dot -T pdf prof.dot  > prof.pdf
#
# 参考 callgrind http://nu-pan.hatenablog.com/entry/20140410/1397099300
# google-pprof --callgrind hoge hoge.prof > hoge.cg
# kcachegrind hoge.cg
# unix:LIBS += -lprofiler

# gprof何故か動かない
# gprof 参考 http://minus9d.hatenablog.com/entry/20140112/1389502918
# -pg付けてコンパイル
# gprof a.out gmon.out
#QMAKE_CXXFLAGS +=  -pg
#QMAKE_LFLAGS +=  -pg -fno-omit-frame-pointer

# Linux perf Linuxならこれをつかう。
# linux-perf  http://int.main.jp/txt/perf/
# 可視化スクリプト http://d.hatena.ne.jp/yohei-a/20150706/1436208007
# Qiita具体例 https://qiita.com/saikoro-steak/items/bf066241eeef1141ef5f
# perf record -ag gxsview test.mcn
# perf report -g
# -a で全演算 -gでグラフを有効化





#QMAKE_CXXFLAGS += -g -O2 -march=native
#QMAKE_LFLAGS += -g -lpthread

#QMAKE_CXXFLAGS += -O2 -march=native -fopenmp
#QMAKE_LFLAGS += -lpthread -lgomp

#QMAKE_CXXFLAGS += -O3 -march=native -fopenmp -D_GLIBCXX_PARALLEL
#QMAKE_LFLAGS += -lpthread -lgomp



SOURCES += main.cpp


PROJECT = $$PWD/..
include (core.pri)



LIBACEXS_SRCDIR = ../component/libacexs/libsrc
include (../component/libacexs/libacexs.pri)

HEADERS += \


