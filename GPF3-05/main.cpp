
#include "FrameBufferEmulator.h"

int main(int argc, char** argv)
{
    FrameBufferEmulator frameBuffer;
    frameBuffer.init();
    frameBuffer.loadImage("Sample960x540.bmp");
    frameBuffer.loadFONT("PixelMplus12-Regular.ttf");

    frameBuffer.initUser();
    frameBuffer.drawLoop();

    frameBuffer.destroy();

    return 0;
}
