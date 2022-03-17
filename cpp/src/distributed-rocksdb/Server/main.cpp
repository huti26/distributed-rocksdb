#include "DrdbServerApp.h"

int main(int argc, char *argv[]) {
    DrdbServerApp drdbServerApp{argc, argv};
    return drdbServerApp.start();
}
