#pragma once


#include <iostream>


namespace cmd {
    void clear();
    void pause();

    namespace ostream {
        void newline();
        void fbuffer();
    }
}