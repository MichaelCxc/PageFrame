#include <array>
#include <iostream>
#include <cassert>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstddef>

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
    static constexpr size_t kPageSize = 4096;
    array<byte,kPageSize> data_;

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
    byte* data_internal() {
        return data_.data();
    }

    const byte* data_internal() const {
        return data_.data();
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

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;
    ~ReadGuard() {
        if (frame != nullptr) {
            frame->unlock_internal();
            frame->unpin_internal();
        }
    }

    ReadGuard(ReadGuard&& other) noexcept : frame(other.frame) {
        other.frame = nullptr;
    }

    ReadGuard& operator=(ReadGuard&& other) noexcept {
        if (this == &other) {return *this;}
        if (frame != nullptr) {
            frame->unlock_internal();
            frame->unpin_internal();
        }
        this->frame = other.frame;
        other.frame = nullptr;
        return *this;
    }

    const byte* data() const {
        return frame->data_internal();
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

    WriteGuard(const WriteGuard&) = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;
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

    WriteGuard(WriteGuard&& other) noexcept : frame(other.frame) {
        other.frame = nullptr;
    }

    WriteGuard& operator=(WriteGuard&& other) noexcept {
        if (this == &other) {return *this;}
        if (frame != nullptr) {
            frame->unlock_internal();
            frame->unpin_internal();
        }
        this->frame = other.frame;
        other.frame = nullptr;
        return *this;
    }

    byte* data() {
        frame->mark_dirty_internal();
        return frame->data_internal();
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

    // {
    //     auto g1 = pf.pin();
    //     cout << "after g1: " << pf.get_pin_count() << "\n"; // 1
    //
    //     auto g2 = std::move(g1);
    //     cout << "after move ctor: " << pf.get_pin_count() << "\n"; // should still be 1
    // }
    // cout << "after scope1: " << pf.get_pin_count() << "\n"; // 0
    //
    // {
    //     auto a = pf.pin();
    //     auto b = pf.pin();
    //     cout << "after a+b: " << pf.get_pin_count() << "\n"; // 2
    //
    //     b = std::move(a);
    //     cout << "after move assign: " << pf.get_pin_count() << "\n"; // should be 1
    // }
    // cout << "after scope2: " << pf.get_pin_count() << "\n"; // 0
    //
    //
    // cout << "pin before read: " << pf.get_pin_count() << "\n";
    // {
    //     ReadGuard rg(&pf);
    //     cout << "pin inside read: " << pf.get_pin_count() << "\n";
    // }
    // cout << "pin after read: " << pf.get_pin_count() << "\n";
    //
    // cout << "dirty before: " << pf.is_dirty() << "\n";
    // {
    //     WriteGuard wg(&pf);
    //     wg.mark_dirty();
    //     cout << "dirty inside: " << pf.is_dirty() << "\n";
    // }
    // cout << "dirty after: " << pf.is_dirty() << "\n";
    //
    //
    // auto t1 = std::thread([&pf]() {
    //     WriteGuard wg(&pf);
    //     cout << "t1 acquired\n";
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //     cout << "t1 releasing\n";
    // });
    //
    // auto t2 = std::thread([&pf]() {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //     ReadGuard rg(&pf);
    //     cout << "t2 acquired\n";
    // });
    //
    // t2.join();
    // t1.join();

    {
        WriteGuard wg(&pf);
        auto p = wg.data();
        for (int i = 0; i <3 ; i++) {
            p[i] = static_cast<byte>(i);
        }
    }

    //cout << "The 0 - 2 index: " << p[0] << " " << p[1] << " " << p[2] << "\n";
    {
        ReadGuard rg(&pf);
        auto p2 = rg.data();
        for (int i = 0; i <3 ; i++) {
            cout << "Curren Byte is: " << i  << to_integer<int>(p2[i]) << "\n";
        }
    }

}