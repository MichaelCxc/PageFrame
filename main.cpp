#include <iostream>
#include <cassert>

using namespace std;

class PageFrame;

class PinGuard {
    PageFrame* frame;
public:
    explicit PinGuard(PageFrame* pf);

    PinGuard(const PinGuard&) = delete;
    PinGuard& operator=(const PinGuard&) = delete;

    ~PinGuard();
};

class PageFrame {
    int pin_count = 0;
public:
    void pin_internal() {
        pin_count++;
    }
    void unpin_internal() {
        pin_count--;
    }
    int get_pin_count() const{
        return pin_count;
    }

    PinGuard pin() {return PinGuard(this);};
};

PinGuard::PinGuard(PageFrame *pf) {
    frame = pf;
    if (frame != nullptr) {
        frame->pin_internal();
    }
}

PinGuard::~PinGuard() {
    if (frame != nullptr) {
        frame->unpin_internal();
    }
}


int main() {
    PageFrame pf;
    cout << "init: " << pf.get_pin_count() << "\n";
    {
        auto g = pf.pin();
        cout << "PinGuard after init: " << pf.get_pin_count() << "\n";
        //PinGuard pg2(pg);
    }

    cout << "PinGuard destroyed: " << pf.get_pin_count() << "\n";


}