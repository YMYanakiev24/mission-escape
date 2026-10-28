#include "app/App.h"

int main(int argc, char* argv[]) {
    app::App application(app::parseLaunchOptions(argc, argv));
    return application.run();
}
