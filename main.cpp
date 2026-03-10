#include <iostream>
#include <cassert>
#include <mutex>

using namespace std;

class PageFrame;

class PinGuard {
    PageFrame* frame = nullptr;
public:
    explicit PinGuard(PageFrame* pf);
    PinGuard(PinGuard&& other) noexcept;
    PinGuard& operator=(PinGuard&& other) noexcept;

    PinGuard(const PinGuard&) = delete;
    PinGuard& operator=(const PinGuard&) = delete;

    ~PinGuard();
};

class PageFrame {
    int pin_count = 0;
    mutex mtx_;
    bool isDirty = false;
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
    void mark_dirty_internal() {
        isDirty = true;
    }
    bool is_dirty() const {
        return isDirty;
    }
    void lock_internal() {
        mtx_.lock();
    }

    void unlock_internal() {
        mtx_.unlock();
    }
    PinGuard pin() {return PinGuard(this);};
};

class ReadGuard {
    PageFrame* frame = nullptr;
public:
    explicit ReadGuard(PageFrame* pf) {
        frame = pf;
        if (frame != nullptr) {
            frame->pin_internal();
            frame->lock_internal();
        }
    }
    ~ReadGuard() {
        if (frame != nullptr) {
            frame->unlock_internal();
            frame->unpin_internal();
        }
    }
};

class WriteGuard {
    PageFrame* frame = nullptr;
public:
    explicit WriteGuard(PageFrame* pf) {
        frame = pf;
        if (frame != nullptr) {
            frame->pin_internal();
            frame->lock_internal();
        }
    }
    ~WriteGuard() {
        if (frame != nullptr) {
            frame->unlock_internal();
            frame->unpin_internal();
        }
    }
    void mark_dirty() const {
        if (frame != nullptr) {
            frame->mark_dirty_internal();
        }
    }
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

PinGuard::PinGuard(PinGuard&& other) noexcept : frame(other.frame) {
    other.frame = nullptr;
}

PinGuard& PinGuard::operator=(PinGuard&& other) noexcept{
    if (this == &other) {return *this;}
    if (frame != nullptr) {
        frame->unpin_internal();
    }
    this->frame = other.frame;
    other.frame = nullptr;
    return *this;
}


int main() {
    PageFrame pf;

    cout << "init: " << pf.get_pin_count() << "\n";

    {
        auto g1 = pf.pin();
        cout << "after g1: " << pf.get_pin_count() << "\n"; // 1

        auto g2 = std::move(g1);
        cout << "after move ctor: " << pf.get_pin_count() << "\n"; // should still be 1
    }
    cout << "after scope1: " << pf.get_pin_count() << "\n"; // 0

    {
        auto a = pf.pin();
        auto b = pf.pin();
        cout << "after a+b: " << pf.get_pin_count() << "\n"; // 2

        b = std::move(a);
        cout << "after move assign: " << pf.get_pin_count() << "\n"; // should be 1
    }
    cout << "after scope2: " << pf.get_pin_count() << "\n"; // 0
}