

# mcmodeとphits_metacardは相互依存なのでmcmode.priでphits_metacard.priをincludeしてはならない
# 故にmcmodeの間接依存ファイルもここに書く。

include($$PWD/dataline.pri)

HEADERS *= \
	$$PROJECT/core/io/input/mcmode.hpp \
	$$PROJECT/core/io/input/phits/phits_metacards.hpp \
	$$PROJECT/core/utils/string_utils.hpp \


SOURCES *= \
	$$PROJECT/core/io/input/mcmode.cpp \
	$$PROJECT/core/io/input/phits/phits_metacards.cpp \
	$$PROJECT/core/utils/string_utils.cpp \
