# 全テスト共通設定


PROJECT = $$PWD/..   # PWD はtestsなのでその上がroot


# *.priファイル内ではSOURCE, HEADERSは$$PWDではなく$$PROJECTで絶対パス指定する。
# そうでなければ *= 演算子による一意性が確保できなくなるため。
# .priファイルからpriファイルをインクルードするときはそのような問題は発生しないので相対パスで良い。


INCLUDEPATH += $$PROJECT

TESTS = $$PROJECT/tests
#LIBS += -L$$PROJECT/libacexs/lib -lacexs
LIBACEXS_SRCDIR = $$PROJECT/component/libacexs/libsrc
DEFINES += DIRECTUSE_LIBACEXS
CONFIG += c++17
win32{
	QMAKE_CXXFLAGS += /utf-8 /wd4503 /wd4267
}
# File truncated でエラー 並列コンパイルできなくなるのでやはり遅い。
#OBJECTS_DIR = $$PROJECT/tmp
#これはobjを一箇所に集めるかどうかというだけで、他プロジェクトとobjを共有するわけではない。
#CONFIG += object_parallel_to_source
