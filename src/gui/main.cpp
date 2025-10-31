#include "gui/MainWindow.h"
#include <FL/Fl.H>

int main(int argc, char* argv[]) {
    bik::MainWindow window(800, 600, "Bik - Backup Manager");
    window.show(argc, argv);
    return Fl::run();
}
