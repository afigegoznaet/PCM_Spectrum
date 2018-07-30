#include(../spectrum.pri)
#INCLUDEPATH += $$PWD/spectrum/ft2xx/
FT2XX_DIR = $$PWD/ft2xx
INCLUDEPATH += $$FT2XX_DIR
DEPENDPATH += $$FT2XX_DIR

win32{

LIBS += -l$$FT2XX_DIR/ftd2xx \
-l$$FT2XX_DIR/LibFT4222
}

#-l$$PWD/ft2xx/LibFT4222.dll

