#include "pool/thread_pool.h"
#include <functional>
#include <iostream>
using namespace std;
int main() {
    thread_pool p;
    int i = 0;
    while (i++ <= 100) {
        function<void()> func = [_i=i] {
            cout << "jiuzhe? " << _i << endl;
            this_thread::sleep_for(chrono::seconds(1));
        };
        p.add_task(move(func));
    }
    getchar();
}
