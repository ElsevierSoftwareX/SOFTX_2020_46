!BOUNDINGBOX_PRI{
BOUNDINGBOX_PRI=1

HEADERS *= \
        $$PROJECT/core/geometry/cell/boundingbox.hpp  \
        $$PROJECT/core/geometry/cell/bb_utils.hpp  \
        $$PROJECT/core/geometry/surface/plane.hpp \
        $$PROJECT/core/utils/message.hpp \
        $$PROJECT/core/utils/numeric_utils.hpp \
        $$PROJECT/core/utils/string_utils.hpp \
        $$PROJECT/core/utils/system_utils.hpp \
        $$PROJECT/core/geometry/surface/surface.hpp \

SOURCES *= \
# BB自体が必要とするファイル
        $$PROJECT/core/geometry/cell/boundingbox.cpp \
        $$PROJECT/core/geometry/cell/bb_utils.cpp \
# ここまでboundingbox.hppで必須のファイル
        $$PROJECT/core/geometry/surface/plane.cpp \
        $$PROJECT/core/utils/message.cpp \
        $$PROJECT/core/utils/numeric_utils.cpp \
        $$PROJECT/core/utils/string_utils.cpp \
        $$PROJECT/core/utils/system_utils.cpp \
# ここまでboundingbox.cppで直接必要なファイル
# 派生して必要となっているファイル
        $$PROJECT/core/geometry/surface/surface.cpp \ # Planeの基底クラス

}
